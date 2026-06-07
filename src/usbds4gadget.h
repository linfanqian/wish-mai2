/* CDWUSBGadget is the USB device driver. It talks directly to the DesignWare
   USB hardware, and manages endpoints.
   
   This file implementes the driver for DS4 (PS4 controller) logic.  */

#ifndef _usbds4gadget_h
#define _usbds4gadget_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/usb.h>
#include <circle/interrupt.h>
#include <circle/macros.h>
#include <circle/types.h>
#include "usbds4gadgetendpoint.h"

// Sony DualShock 4 v1 USB identity
// Ref: Device Descriptor from https://www.psdevwiki.com/ps4/DS4-USB
#define PS4_USB_VID  0x054C
#define PS4_USB_PID  0x05C4

class CUSBDS4Gadget : public CDWUSBGadget
{
public:
    CUSBDS4Gadget (CInterruptSystem *pInterruptSystem);
    ~CUSBDS4Gadget (void);

    const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) override;
    void AddEndpoints (void) override;
    void CreateDevice (void) override;
    void OnSuspend (void) override;
    int  OnClassOrVendorRequest (const TSetupData *pSetupData, u8 *pData) override;

    // Update the DS4 report sent to the host on the next IN (device->host) poll.
    // Thread-safe to be called from anywhere anytime
    void SendDS4Report (const u8 *pReport);

private:
    enum TEPNumber
    {
        EPIn = 1, // The only EP (other than EP0) we use (interrupt IN endpoint)
        NumEPs
    };

    CUSBDS4GadgetEndpoint *m_pEP[NumEPs];

    // State kept for responses on EP0 GET_REPORT.
    u8 m_CurrentState[DS4_REPORT_SIZE];
};

#endif
