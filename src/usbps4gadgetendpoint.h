#ifndef _usbps4gadgetendpoint_h
#define _usbps4gadgetendpoint_h

#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CUSBPS4Gadget;

#define PS4_REPORT_SIZE  64

class CUSBPS4GadgetEndpoint : public CDWUSBGadgetEndpoint
{
public:
	CUSBPS4GadgetEndpoint (const TUSBEndpointDescriptor *pDesc, CUSBPS4Gadget *pGadget);
	~CUSBPS4GadgetEndpoint (void);

	// Thread-safe: can be called from the main loop while transfers are in flight.
	void SendReport (const u8 *pReport);

	void OnActivate (void) override;
	void OnDeactivate (void) override;
	void OnTransferComplete (boolean bIn, size_t nLength) override;
	void OnSuspend (void) override;

private:
	volatile boolean m_bActive;

	// m_PendingBuffer is written by SendReport(); m_DMABuffer is used for live DMA.
	u8 m_PendingBuffer[PS4_REPORT_SIZE];
	DMA_BUFFER (u8, m_DMABuffer, PS4_REPORT_SIZE);

	CSpinLock m_SpinLock;
};

#endif
