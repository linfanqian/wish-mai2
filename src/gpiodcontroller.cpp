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

void CGPIOController::Read (TDS4ButtonState *pState)
{
    // Pressed = read 0
    pState->bNorth     = !m_PinNorth.Read ();
    pState->bEast      = !m_PinEast.Read ();
    pState->bSouth     = !m_PinSouth.Read ();
    pState->bWest      = !m_PinWest.Read ();
    pState->bDpadUp    = !m_PinDpadUp.Read ();
    pState->bDpadRight = !m_PinDpadRight.Read ();
    pState->bDpadDown  = !m_PinDpadDown.Read ();
    pState->bDpadLeft  = !m_PinDpadLeft.Read ();
}

void CGPIOController::Send (CUSBDS4Gadget *pGadget, const TDS4ButtonState &state)
{
    u8 report[DS4_REPORT_SIZE];

    // D-pad conflict: up+down or left+right are invalid for DS4
    boolean conflict = (state.bDpadUp && state.bDpadDown) || (state.bDpadLeft && state.bDpadRight);

    if (conflict)
    {
        TDS4ButtonState state1 = { state.bNorth, state.bEast, state.bSouth, state.bWest,
                                   state.bDpadUp, false, false, state.bDpadLeft };
        CUSBDS4GadgetEndpoint::CreateDS4Report (report, &state1, nCounter++);
        pGadget->SendDS4Report (report);

        TDS4ButtonState state2 = { state.bNorth, state.bEast, state.bSouth, state.bWest,
                                   false, state.bDpadRight, state.bDpadDown, false };
        CUSBDS4GadgetEndpoint::CreateDS4Report (report, &state2, nCounter++);
        pGadget->SendDS4Report (report);
    }
    else
    {
        CUSBDS4GadgetEndpoint::CreateDS4Report (report, &state, nCounter++);
        pGadget->SendDS4Report (report);
    }
}
