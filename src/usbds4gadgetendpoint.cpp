#include "usbds4gadgetendpoint.h"
#include "usbds4gadget.h"
#include <circle/util.h>
#include <assert.h>

// Ref: https://github.com/torvalds/linux/blob/ba3e43a9e601636f5edb54e259a74f96ca3b8fd8/drivers/hid/hid-playstation.c#L479
#define REPORT_ID 0x01
#define STICK_CENTER 128	// stick x/y value 0-255

CUSBDS4GadgetEndpoint::CUSBDS4GadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
					      					  CUSBDS4Gadget *pGadget)
:	CDWUSBGadgetEndpoint (pDesc, pGadget),
	m_bActive (FALSE)
{
	InitDS4Report (m_DMABuffer);
	InitDS4Report (m_PendingBuffer);
}

CUSBDS4GadgetEndpoint::~CUSBDS4GadgetEndpoint (void)
{
}

void CUSBDS4GadgetEndpoint::OnActivate (void)
{
	m_bActive = TRUE;
	BeginTransfer (TransferDataIn, m_DMABuffer, DS4_REPORT_SIZE);
}

void CUSBDS4GadgetEndpoint::OnDeactivate (void)
{
	m_bActive = FALSE;
	CancelTransfer ();
}

void CUSBDS4GadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	if (bIn && m_bActive)
	{
		// Previous IN completed: copy latest state and arm the next poll.
		m_SpinLock.Acquire ();
		memcpy (m_DMABuffer, m_PendingBuffer, DS4_REPORT_SIZE);
		m_SpinLock.Release ();
		BeginTransfer (TransferDataIn, m_DMABuffer, DS4_REPORT_SIZE);
	}
}

void CUSBDS4GadgetEndpoint::InitDS4Report (u8 *buf)
{
	memset (buf, 0, DS4_REPORT_SIZE);
	buf[0] = REPORT_ID;
	buf[1] = STICK_CENTER;	// lx
	buf[2] = STICK_CENTER;	// ly
	buf[3] = STICK_CENTER;	// rx
	buf[4] = STICK_CENTER;	// ry
	buf[5] = 8;				// D-pad neutral (hat = 8)
}

void CUSBDS4GadgetEndpoint::SendReport (const u8 *pReport)
{
	m_SpinLock.Acquire ();
	memcpy (m_PendingBuffer, pReport, DS4_REPORT_SIZE);
	m_SpinLock.Release ();
}

void CUSBDS4GadgetEndpoint::OnSuspend (void)
{
	OnDeactivate ();
}
