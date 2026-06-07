#include "usbds4gadgetendpoint.h"
#include "usbds4gadget.h"
#include <circle/util.h>
#include <assert.h>

// Ref: https://github.com/torvalds/linux/blob/ba3e43a9e601636f5edb54e259a74f96ca3b8fd8/drivers/hid/hid-playstation.c#L479
#define REPORT_ID 0x01
#define STICK_CENTER 128    // stick x/y value 0-255

CUSBDS4GadgetEndpoint::CUSBDS4GadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
                                                CUSBDS4Gadget *pGadget)
:    CDWUSBGadgetEndpoint (pDesc, pGadget),
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

// ---------------------------------------------------------------------------
// DS4 USB input report structure (Report ID 0x01, 64 bytes)
//
//  [0]     Report ID = 0x01
//  [1]     LX  (0-255, center 128)
//  [2]     LY  (0-255, center 128)
//  [3]     RX  (0-255, center 128)
//  [4]     RY  (0-255, center 128)
//  [5]     buttons1:  hat[3:0]  Square[4]  Cross[5]  Circle[6]  Triangle[7]
//              hat: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=neutral
//  [6]     buttons2:  L1[0] R1[1] L2[2] R2[3] Share[4] Options[5] L3[6] R3[7]
//  [7]     buttons3:  PS[0] Touch[1] counter[7:2]
//  [8]     L2 analog (0-255)
//  [9]     R2 analog (0-255)
//  [10–12] timestamp, battery level
//  [13–18] gyro  X/Y/Z (3 × s16, little-endian)
//  [19–24] accel X/Y/Z (3 × s16, little-endian)
//  [25–63] touchpad, extensions (zeroed if unused)
// ---------------------------------------------------------------------------
void CUSBDS4GadgetEndpoint::InitDS4Report (u8 *buf)
{
    memset (buf, 0, DS4_REPORT_SIZE);
    buf[0] = REPORT_ID;
    buf[1] = STICK_CENTER;    // lx
    buf[2] = STICK_CENTER;    // ly
    buf[3] = STICK_CENTER;    // rx
    buf[4] = STICK_CENTER;    // ry
    buf[5] = 8;                // D-pad neutral (hat = 8)
}

void CUSBDS4GadgetEndpoint::CreateDS4Report (u8 *pReport, const TDS4ButtonState *pState,
                                             u8 nCounter)
{
    InitDS4Report (pReport);

    // D-pad hat (0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=neutral)
    u8 hat = 8;
    if      (pState->bDpadUp    && pState->bDpadRight) hat = 1;
    else if (pState->bDpadRight && pState->bDpadDown)  hat = 3;
    else if (pState->bDpadDown  && pState->bDpadLeft)  hat = 5;
    else if (pState->bDpadLeft  && pState->bDpadUp)    hat = 7;
    else if (pState->bDpadUp)                          hat = 0;
    else if (pState->bDpadRight)                       hat = 2;
    else if (pState->bDpadDown)                        hat = 4;
    else if (pState->bDpadLeft)                        hat = 6;

    // Byte 5: hat[3:0] | Square[4] | Cross[5] | Circle[6] | Triangle[7]
    pReport[5] = hat
               | (pState->bWest  ? 0b00010000 : 0)
               | (pState->bSouth ? 0b00100000 : 0)
               | (pState->bEast  ? 0b01000000 : 0)
               | (pState->bNorth ? 0b10000000 : 0);

    // Byte 7: counter[7:2]
    pReport[7] = (nCounter & 0b111111) << 2;
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
