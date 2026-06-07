/* CDWUSBGadgetEndpoint wraps one physical USB endpoint, with number, transfer 
   direction, transfer type, and transfer mode. It also properly handles DMA and 
   interrupts.
   
   This file wraps a physical USB endpoint for DS4 (PS 4 controller), specifying 
   its behavior during different stages of lifetime. */

#ifndef _usbds4gadgetendpoint_h
#define _usbds4gadgetendpoint_h

#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/spinlock.h>
#include <circle/types.h>

/* A report is the fixed-size payload sent to the host
   See DS4 Report Structure from https://www.psdevwiki.com/ps4/DS4-USB#HID_Report_Descriptor
   Also, Linux has a structure reference for DS4 input report at
   https://github.com/torvalds/linux/blob/ba3e43a9e601636f5edb54e259a74f96ca3b8fd8/drivers/hid/hid-playstation.c#L478 */
#define DS4_REPORT_SIZE  64

class CUSBDS4Gadget;

class CUSBDS4GadgetEndpoint : public CDWUSBGadgetEndpoint
{
public:
	CUSBDS4GadgetEndpoint (const TUSBEndpointDescriptor *pDesc, CUSBDS4Gadget *pGadget);
	~CUSBDS4GadgetEndpoint (void);

	void OnActivate (void) override;
	void OnDeactivate (void) override;
	void OnTransferComplete (boolean bIn, size_t nLength) override;
	void OnSuspend (void) override;

	// Buffer must be DS4_REPORT_SIZE long.
	static void InitDS4Report (u8 *buf);

	// Stage a next report. Copy the report to DMA buffer when inflight transfer completed
	// Thread-safe is guaranteed, so it's ok to call SendReport when during a DMA transfer
	void SendReport (const u8 *pReport);

private:
	volatile boolean m_bActive;

	// m_PendingBuffer is written by SendReport()
	// m_DMABuffer is used for live DMA.
	u8 m_PendingBuffer[DS4_REPORT_SIZE];
	DMA_BUFFER (u8, m_DMABuffer, DS4_REPORT_SIZE);

	// Protect m_PendingBuffer read/write
	// This cannot be a mutex since OnTransferComplete that reads m_PendingBuffer runs in 
	// interrupt context
	CSpinLock m_SpinLock;
};

#endif
