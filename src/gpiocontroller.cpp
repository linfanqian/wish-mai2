#include "gpiocontroller.h"

CGPIOController::CGPIOController (void)
:    m_PinNorth     (4,  GPIOModeInputPullUp),
    m_PinEast      (17, GPIOModeInputPullUp),
    m_PinSouth     (27, GPIOModeInputPullUp),
    m_PinWest      (22, GPIOModeInputPullUp),
    m_PinDpadUp    (5,  GPIOModeInputPullUp),
    m_PinDpadRight (6,  GPIOModeInputPullUp),
    m_PinDpadDown  (13, GPIOModeInputPullUp),
    m_PinDpadLeft  (19, GPIOModeInputPullUp)
{
}

void CGPIOController::Read (TButtonState *pState)
{
    // Pull-up means LOW when pressed.
    pState->bNorth     = (m_PinNorth.Read ()     == LOW);
    pState->bEast      = (m_PinEast.Read ()      == LOW);
    pState->bSouth     = (m_PinSouth.Read ()     == LOW);
    pState->bWest      = (m_PinWest.Read ()      == LOW);
    pState->bDpadUp    = (m_PinDpadUp.Read ()    == LOW);
    pState->bDpadRight = (m_PinDpadRight.Read () == LOW);
    pState->bDpadDown  = (m_PinDpadDown.Read ()  == LOW);
    pState->bDpadLeft  = (m_PinDpadLeft.Read ()  == LOW);
}
