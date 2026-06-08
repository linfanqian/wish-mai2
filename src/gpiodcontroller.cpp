#include "gpiocontroller.h"
#include "usbds4gadget.h"
#include "usbds4gadgetendpoint.h"
#include <circle/logger.h>

CGPIOController::CGPIOController (void)
:    m_PinNorth     (4,  GPIOModeInputPullUp),
    m_PinEast      (5, GPIOModeInputPullUp),
    m_PinSouth     (6, GPIOModeInputPullUp),
    m_PinWest      (16, GPIOModeInputPullUp),
    m_PinDpadUp    (17,  GPIOModeInputPullUp),
    m_PinDpadRight (20,  GPIOModeInputPullUp),
    m_PinDpadDown  (21, GPIOModeInputPullUp),
    m_PinDpadLeft  (22, GPIOModeInputPullUp)
{
    CLogger::Get ()->Write ("gpiods4map", LogNotice,
        "GPIO map: North=%u East=%u South=%u West=%u "
        "DpadUp=%u DpadRight=%u DpadDown=%u DpadLeft=%u",
        PinNorth, PinEast, PinSouth, PinWest,
        PinDpadUp, PinDpadRight, PinDpadDown, PinDpadLeft);
}

void CGPIOController::PollAndSend (CUSBDS4Gadget *pGadget)
{
    // Pressed = read 0
    boolean bNorth = !m_PinNorth.Read ();
    boolean bEast  = !m_PinEast.Read ();
    boolean bSouth = !m_PinSouth.Read ();
    boolean bWest  = !m_PinWest.Read ();
    boolean bUp    = !m_PinDpadUp.Read ();
    boolean bRight = !m_PinDpadRight.Read ();
    boolean bDown  = !m_PinDpadDown.Read ();
    boolean bLeft  = !m_PinDpadLeft.Read ();

    u8 report[DS4_REPORT_SIZE];

    // D-pad conflict: up+down or left+right are invalid for DS4
    boolean conflict = (bUp && bDown) || (bLeft && bRight);

    if (conflict)
    {
        // Send Up+Left first, then Down+Right
        TDS4ButtonState state1 = { bNorth, bEast, bSouth, bWest,
                                   bUp, false, false, bLeft };
        CUSBDS4GadgetEndpoint::CreateDS4Report (report, &state1, nCounter++);
        pGadget->SendDS4Report (report);

        TDS4ButtonState state2 = { bNorth, bEast, bSouth, bWest,
                                   false, bRight, bDown, false };
        CUSBDS4GadgetEndpoint::CreateDS4Report (report, &state2, nCounter++);
        pGadget->SendDS4Report (report);
    }
    else
    {
        TDS4ButtonState state = { bNorth, bEast, bSouth, bWest,
                                  bUp, bRight, bDown, bLeft };
        CUSBDS4GadgetEndpoint::CreateDS4Report (report, &state, nCounter++);
        pGadget->SendDS4Report (report);
    }
}
