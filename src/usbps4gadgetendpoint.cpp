#include "usbps4gadgetendpoint.h"
#include "usbps4gadget.h"
#include <circle/util.h>
#include <assert.h>

static void NeutralReport (u8 *p)
{
	memset (p, 0, PS4_REPORT_SIZE);
	p[0] = 0x01;   // Report ID
	p[1] = 0x80;   // LX center
	p[2] = 0x80;   // LY center
	p[3] = 0x80;   // RX center
	p[4] = 0x80;   // RY center
	p[5] = 0x08;   // D-pad neutral (hat = 8, out of range → null state)
}

CUSBPS4GadgetEndpoint::CUSBPS4GadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
					      CUSBPS4Gadget *pGadget)
:	CDWUSBGadgetEndpoint (pDesc, pGadget),
	m_bActive (FALSE)
{
	NeutralReport (m_PendingBuffer);
	NeutralReport (m_DMABuffer);
}

CUSBPS4GadgetEndpoint::~CUSBPS4GadgetEndpoint (void)
{
}

void CUSBPS4GadgetEndpoint::SendReport (const u8 *pReport)
{
	m_SpinLock.Acquire ();
	memcpy (m_PendingBuffer, pReport, PS4_REPORT_SIZE);
	m_SpinLock.Release ();
}

void CUSBPS4GadgetEndpoint::OnActivate (void)
{
	m_bActive = TRUE;
	m_SpinLock.Acquire ();
	memcpy (m_DMABuffer, m_PendingBuffer, PS4_REPORT_SIZE);
	m_SpinLock.Release ();
	BeginTransfer (TransferDataIn, m_DMABuffer, PS4_REPORT_SIZE);
}

void CUSBPS4GadgetEndpoint::OnDeactivate (void)
{
	m_bActive = FALSE;
	CancelTransfer ();
}

void CUSBPS4GadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	if (bIn && m_bActive)
	{
		// Previous IN completed; copy latest state and arm the next poll.
		m_SpinLock.Acquire ();
		memcpy (m_DMABuffer, m_PendingBuffer, PS4_REPORT_SIZE);
		m_SpinLock.Release ();
		BeginTransfer (TransferDataIn, m_DMABuffer, PS4_REPORT_SIZE);
	}
}

void CUSBPS4GadgetEndpoint::OnSuspend (void)
{
	m_bActive = FALSE;
	CancelTransfer ();
}
