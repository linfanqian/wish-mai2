#ifndef _usbds4gadget_h
#define _usbds4gadget_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/usb.h>
#include <circle/interrupt.h>
#include <circle/macros.h>
#include <circle/types.h>
#include "usbds4gadgetendpoint.h"

// Sony DualShock 4 v1 USB identity
#define PS4_USB_VID  0x054C
#define PS4_USB_PID  0x05C4

class Cusbds4gadget : public CDWUSBGadget
{
public:
	Cusbds4gadget (CInterruptSystem *pInterruptSystem);
	~Cusbds4gadget (void);

	// Update the report sent to the host on the next IN poll.
	// Safe to call from the main loop.
	void SendReport (const u8 *pReport);

protected:
	const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) override;
	void AddEndpoints (void) override;
	void CreateDevice (void) override;
	void OnSuspend (void) override;
	int  OnClassOrVendorRequest (const TSetupData *pSetupData, u8 *pData) override;

private:
	const void *ToStringDescriptor (const char *pString, size_t *pLength);

	enum TEPNumber
	{
		EPIn = 1,
		NumEPs
	};

	CUSBDS4GadgetEndpoint *m_pEP[NumEPs];

	// Kept for GET_REPORT responses on EP0.
	u8 m_CurrentReport[DS4_REPORT_SIZE];

	u8 m_StringDescBuffer[80];
};

#endif
