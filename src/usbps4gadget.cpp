#include "usbps4gadget.h"
#include <circle/usb/usb.h>
#include <circle/util.h>
#include <assert.h>

#define DESCRIPTOR_HID         0x21
#define DESCRIPTOR_HID_REPORT  0x22

// ---------------------------------------------------------------------------
// HID report descriptor
//
// Report ID 0x01, 64 bytes total (1 ID + 63 data):
//   [0]    Report ID = 0x01
//   [1-4]  LX, LY, RX, RY axes (0x80 = center)
//   [5]    bits[3:0]=D-pad hat  bits[7:4]=Square/Cross/Circle/Triangle
//   [6]    bits[7:0]=L1/R1/L2/R2/Share/Options/L3/R3
//   [7]    bits[1:0]=PS/Touch  bits[7:2]=6-bit counter
//   [8]    L2 analog
//   [9]    R2 analog
//   [10-63] vendor padding (IMU, battery, touchpad – zeroed)
// ---------------------------------------------------------------------------
static const u8 s_HIDReportDescriptor[] =
{
	0x05, 0x01,        // Usage Page (Generic Desktop)
	0x09, 0x05,        // Usage (Game Pad)
	0xA1, 0x01,        // Collection (Application)
	0x85, 0x01,        //   Report ID (1)

	// Axes: LX LY RX RY  →  4 bytes
	0x09, 0x30,        //   Usage (X)
	0x09, 0x31,        //   Usage (Y)
	0x09, 0x32,        //   Usage (Z)
	0x09, 0x35,        //   Usage (Rz)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x04,        //   Report Count (4)
	0x81, 0x02,        //   Input (Data,Var,Abs)

	// D-pad hat switch  →  4 bits, null state for neutral (value 8)
	0x09, 0x39,        //   Usage (Hat switch)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x07,        //   Logical Maximum (7)
	0x35, 0x00,        //   Physical Minimum (0)
	0x46, 0x3B, 0x01,  //   Physical Maximum (315)
	0x65, 0x14,        //   Unit (Eng Rot:Angular Pos)
	0x75, 0x04,        //   Report Size (4)
	0x95, 0x01,        //   Report Count (1)
	0x81, 0x42,        //   Input (Data,Var,Abs,Null)

	// Buttons 1-14  →  14 bits
	// 1=Square 2=Cross 3=Circle 4=Triangle 5=L1 6=R1 7=L2 8=R2
	// 9=Share 10=Options 11=L3 12=R3 13=PS 14=Touchpad
	0x65, 0x00,        //   Unit (None)
	0x05, 0x09,        //   Usage Page (Button)
	0x19, 0x01,        //   Usage Minimum (1)
	0x29, 0x0E,        //   Usage Maximum (14)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x01,        //   Logical Maximum (1)
	0x75, 0x01,        //   Report Size (1)
	0x95, 0x0E,        //   Report Count (14)
	0x81, 0x02,        //   Input (Data,Var,Abs)

	// Padding to byte boundary  →  6 bits
	0x75, 0x06,        //   Report Size (6)
	0x95, 0x01,        //   Report Count (1)
	0x81, 0x03,        //   Input (Const,Var,Abs)

	// L2 / R2 analog  →  2 bytes
	0x05, 0x01,        //   Usage Page (Generic Desktop)
	0x09, 0x33,        //   Usage (Rx)
	0x09, 0x34,        //   Usage (Ry)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x02,        //   Report Count (2)
	0x81, 0x02,        //   Input (Data,Var,Abs)

	// Vendor-defined padding covering IMU/battery/touchpad  →  54 bytes
	0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
	0x09, 0x20,        //   Usage (0x20)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x36,        //   Report Count (54)
	0x81, 0x02,        //   Input (Data,Var,Abs)

	0xC0               // End Collection
};

// ---------------------------------------------------------------------------
// Descriptor structs (local to this file)
// ---------------------------------------------------------------------------

struct TUSBHIDDescriptorLocal
{
	u8  bLength;
	u8  bDescriptorType;
	u16 bcdHID;
	u8  bCountryCode;
	u8  bNumDescriptors;
	u8  bDescriptorType2;
	u16 wDescriptorLength;
} PACKED;

struct TPS4ConfigDescriptor
{
	TUSBConfigurationDescriptor Config;
	TUSBInterfaceDescriptor     Interface;
	TUSBHIDDescriptorLocal      HID;
	TUSBEndpointDescriptor      EndpointIn;
} PACKED;

static TUSBDeviceDescriptor s_DeviceDescriptor =
{
	sizeof (TUSBDeviceDescriptor),
	DESCRIPTOR_DEVICE,
	0x0200,           // bcdUSB (USB 2.0)
	0, 0, 0,          // class/subclass/protocol defined at interface
	64,               // bMaxPacketSize0
	PS4_USB_VID,
	PS4_USB_PID,
	0x0100,           // bcdDevice
	1, 2, 0,          // iManufacturer, iProduct, iSerialNumber
	1                 // bNumConfigurations
};

static const TPS4ConfigDescriptor s_ConfigDescriptor =
{
	{   // Configuration
		sizeof (TUSBConfigurationDescriptor),
		DESCRIPTOR_CONFIGURATION,
		sizeof (TPS4ConfigDescriptor),
		1,              // bNumInterfaces
		1,              // bConfigurationValue
		0,              // iConfiguration
		0x80,           // bmAttributes (bus-powered)
		250             // bMaxPower (500 mA)
	},
	{   // Interface
		sizeof (TUSBInterfaceDescriptor),
		DESCRIPTOR_INTERFACE,
		0, 0,           // bInterfaceNumber, bAlternateSetting
		1,              // bNumEndpoints
		0x03,           // bInterfaceClass (HID)
		0x00,           // bInterfaceSubClass (no boot)
		0x00,           // bInterfaceProtocol (none)
		0               // iInterface
	},
	{   // HID
		9,
		DESCRIPTOR_HID,
		0x0111,         // bcdHID 1.11
		0,              // bCountryCode
		1,              // bNumDescriptors
		DESCRIPTOR_HID_REPORT,
		sizeof (s_HIDReportDescriptor)
	},
	{   // Endpoint IN (EP1, Interrupt)
		sizeof (TUSBEndpointDescriptor),
		DESCRIPTOR_ENDPOINT,
		0x81,           // EP1 IN
		0x03,           // Interrupt
		64,             // wMaxPacketSize
		5               // bInterval (5 ms)
	}
};

static const char *const s_StringDescriptor[] =
{
	"\x04\x03\x09\x04",              // Language ID (English US)
	"Sony Interactive Entertainment",
	"Wireless Controller"
};

// ---------------------------------------------------------------------------
// DS4 feature reports required by the Linux playstation / iOS GameController
// drivers before they will accept the device as a fully functional gamepad.
//
// 0x12  Pairing info (16 bytes) — drivers read bytes 1-6 as the MAC address.
// 0xa3  Firmware version (49 bytes) — hw_version @ [35-38], fw_version @ [41-42].
// 0x02  IMU calibration (37 bytes) — all denominator fields must be non-zero.
// ---------------------------------------------------------------------------

static const u8 s_FeatureReport12[16] =
{
	0x12,                                     // Report ID
	0x00, 0x1A, 0x7D, 0xDA, 0x00, 0x01,      // Fake MAC: 00:1A:7D:DA:00:01
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       // padding
	0x00, 0x00, 0x00
};

static const u8 s_FeatureReportA3[49] =
{
	0xa3,                                     // Report ID
	0x44, 0x75, 0x61, 0x6C, 0x53, 0x68,      // "DualSh"
	0x6F, 0x63, 0x6B, 0x34, 0x00, 0x00,      // "ock4\0\0"
	0x00, 0x00, 0x00,                         // reserved
	// bytes 16-34: padding
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// bytes 35-38: hw_version = 0x00000103 (little-endian)
	0x03, 0x01, 0x00, 0x00,
	// bytes 39-40: padding
	0x00, 0x00,
	// bytes 41-42: fw_version = 0x0100 (little-endian)
	0x00, 0x01,
	// bytes 43-48: padding
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Gyro and accel calibration values.  Encoding: signed 16-bit little-endian.
//
// Actual driver field layout (hid-playstation.c):
//   buf[1-2]   gyro_pitch_bias   buf[3-4]   gyro_yaw_bias   buf[5-6]   gyro_roll_bias
//   buf[7-8]   gyro_pitch_plus   buf[9-10]  gyro_pitch_minus
//   buf[11-12] gyro_yaw_plus     buf[13-14] gyro_yaw_minus
//   buf[15-16] gyro_roll_plus    buf[17-18] gyro_roll_minus
//   buf[19-20] gyro_speed_plus   buf[21-22] gyro_speed_minus
//   buf[23-24] acc_x_plus        buf[25-26] acc_x_minus
//   buf[27-28] acc_y_plus        buf[29-30] acc_y_minus
//   buf[31-32] acc_z_plus        buf[33-34] acc_z_minus
//   buf[35-36] padding
//
// Denominators (abs(plus)+abs(minus)) and (plus-minus) must all be non-zero.
static const u8 s_FeatureReport02[37] =
{
	0x02,                 // Report ID
	0x00, 0x00,           // gyro_pitch_bias = 0
	0x00, 0x00,           // gyro_yaw_bias   = 0
	0x00, 0x00,           // gyro_roll_bias  = 0
	0x40, 0x1F,           // gyro_pitch_plus  = +8000
	0xC0, 0xE0,           // gyro_pitch_minus = -8000
	0x40, 0x1F,           // gyro_yaw_plus    = +8000
	0xC0, 0xE0,           // gyro_yaw_minus   = -8000
	0x40, 0x1F,           // gyro_roll_plus   = +8000
	0xC0, 0xE0,           // gyro_roll_minus  = -8000
	0x30, 0x11,           // gyro_speed_plus  = +4400
	0x30, 0x11,           // gyro_speed_minus = +4400
	0xA4, 0x1F,           // acc_x_plus  = +8100
	0x5C, 0xE0,           // acc_x_minus = -8100
	0xA4, 0x1F,           // acc_y_plus  = +8100
	0x5C, 0xE0,           // acc_y_minus = -8100
	0xA4, 0x1F,           // acc_z_plus  = +8100
	0x5C, 0xE0,           // acc_z_minus = -8100
	0x00, 0x00            // padding
};

static int FillFeatureReport (u8 *pDst, unsigned nMaxLen,
			      const u8 *pSrc, unsigned nSrcLen)
{
	unsigned nLen = nMaxLen < nSrcLen ? nMaxLen : nSrcLen;
	memcpy (pDst, pSrc, nLen);
	return (int) nLen;
}

// ---------------------------------------------------------------------------

CUSBPS4Gadget::CUSBPS4Gadget (CInterruptSystem *pInterruptSystem)
:	CDWUSBGadget (pInterruptSystem, FullSpeed),
	m_pEP {nullptr}
{
	memset (m_CurrentReport, 0, PS4_REPORT_SIZE);
	m_CurrentReport[0] = 0x01;
	m_CurrentReport[1] = 0x80;
	m_CurrentReport[2] = 0x80;
	m_CurrentReport[3] = 0x80;
	m_CurrentReport[4] = 0x80;
	m_CurrentReport[5] = 0x08;   // D-pad neutral
}

CUSBPS4Gadget::~CUSBPS4Gadget (void)
{
	assert (0);
}

void CUSBPS4Gadget::SendReport (const u8 *pReport)
{
	memcpy (m_CurrentReport, pReport, PS4_REPORT_SIZE);
	if (m_pEP[EPIn])
		m_pEP[EPIn]->SendReport (pReport);
}

// ---------------------------------------------------------------------------
// CDWUSBGadget overrides
// ---------------------------------------------------------------------------

const void *CUSBPS4Gadget::GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength)
{
	assert (pLength);

	u8 uchDescType  = wValue >> 8;
	u8 uchDescIndex = wValue & 0xFF;

	switch (uchDescType)
	{
	case DESCRIPTOR_DEVICE:
		if (!uchDescIndex)
		{
			*pLength = sizeof s_DeviceDescriptor;
			return &s_DeviceDescriptor;
		}
		break;

	case DESCRIPTOR_CONFIGURATION:
		if (!uchDescIndex)
		{
			*pLength = sizeof s_ConfigDescriptor;
			return &s_ConfigDescriptor;
		}
		break;

	case DESCRIPTOR_STRING:
		switch (uchDescIndex)
		{
		case 0:
			*pLength = 4;
			return s_StringDescriptor[0];
		case 1:
		case 2:
			return ToStringDescriptor (s_StringDescriptor[uchDescIndex], pLength);
		}
		break;

	case DESCRIPTOR_HID:
		if (!uchDescIndex)
		{
			*pLength = sizeof s_ConfigDescriptor.HID;
			return &s_ConfigDescriptor.HID;
		}
		break;

	case DESCRIPTOR_HID_REPORT:
		if (!uchDescIndex)
		{
			*pLength = sizeof s_HIDReportDescriptor;
			return s_HIDReportDescriptor;
		}
		break;
	}

	return nullptr;
}

void CUSBPS4Gadget::AddEndpoints (void)
{
	assert (!m_pEP[EPIn]);
	m_pEP[EPIn] = new CUSBPS4GadgetEndpoint (&s_ConfigDescriptor.EndpointIn, this);
}

void CUSBPS4Gadget::CreateDevice (void)
{
	// Endpoint drives itself; nothing extra to create.
}

void CUSBPS4Gadget::OnSuspend (void)
{
	if (m_pEP[EPIn])
	{
		m_pEP[EPIn]->OnSuspend ();
		delete m_pEP[EPIn];
		m_pEP[EPIn] = nullptr;
	}
}

int CUSBPS4Gadget::OnClassOrVendorRequest (const TSetupData *pSetupData, u8 *pData)
{
	assert (pSetupData);

	switch (pSetupData->bRequest)
	{
	case 0x01:   // GET_REPORT
		if (pSetupData->bmRequestType == 0xA1)
		{
			u8 nReportType = pSetupData->wValue >> 8;
			u8 nReportID   = pSetupData->wValue & 0xFF;

			if (nReportType == 0x03)   // Feature report
			{
				switch (nReportID)
				{
				case 0x12:
					// Pairing info: report ID + 6-byte MAC + padding
					// The playstation driver extracts bytes 1-6 as MAC.
					return FillFeatureReport (pData, pSetupData->wLength,
								  s_FeatureReport12,
								  sizeof s_FeatureReport12);

				case 0xa3:
					// Firmware version info (49 bytes)
					return FillFeatureReport (pData, pSetupData->wLength,
								  s_FeatureReportA3,
								  sizeof s_FeatureReportA3);

				case 0x02:
					// IMU calibration data (37 bytes)
					return FillFeatureReport (pData, pSetupData->wLength,
								  s_FeatureReport02,
								  sizeof s_FeatureReport02);

				default:
					break;
				}
			}
			else   // Input report
			{
				unsigned nLen = pSetupData->wLength < PS4_REPORT_SIZE
					      ? pSetupData->wLength : PS4_REPORT_SIZE;
				memcpy (pData, m_CurrentReport, nLen);
				return (int) nLen;
			}
		}
		break;

	case 0x09:   // SET_REPORT (rumble / LED colour — ignored for now)
		if (pSetupData->bmRequestType == 0x21)
			return 0;
		break;

	case 0x02:   // GET_IDLE
		if (pSetupData->bmRequestType == 0xA1)
		{
			pData[0] = 0;
			return 1;
		}
		break;

	case 0x0A:   // SET_IDLE
		if (pSetupData->bmRequestType == 0x21)
			return 0;
		break;

	case 0x03:   // GET_PROTOCOL
		if (pSetupData->bmRequestType == 0xA1)
		{
			pData[0] = 0x01;   // Report protocol
			return 1;
		}
		break;

	case 0x0B:   // SET_PROTOCOL
		if (pSetupData->bmRequestType == 0x21)
			return 0;
		break;
	}

	return CDWUSBGadget::OnClassOrVendorRequest (pSetupData, pData);
}

// ---------------------------------------------------------------------------

const void *CUSBPS4Gadget::ToStringDescriptor (const char *pString, size_t *pLength)
{
	size_t nStrLen = 0;
	while (pString[nStrLen])
		nStrLen++;

	size_t nDescLen = 2 + 2 * nStrLen;
	assert (nDescLen <= sizeof m_StringDescBuffer);

	m_StringDescBuffer[0] = (u8) nDescLen;
	m_StringDescBuffer[1] = DESCRIPTOR_STRING;

	for (size_t i = 0; i < nStrLen; i++)
	{
		m_StringDescBuffer[2 + 2 * i] = (u8) pString[i];
		m_StringDescBuffer[3 + 2 * i] = 0;
	}

	*pLength = nDescLen;
	return m_StringDescBuffer;
}
