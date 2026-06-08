#ifndef _gpiocontroller_h
#define _gpiocontroller_h

#include <circle/gpiopin.h>
#include <circle/types.h>
#include "usbds4gadgetendpoint.h"

// GPIO pin assignments
//
//  GPIO 4  → North  (Triangle)     GPIO 17 → D-pad Up
//  GPIO 5  → East   (Circle)       GPIO 20 → D-pad Right
//  GPIO 6  → South  (Cross)        GPIO 21 → D-pad Down
//  GPIO 16 → West   (Square)       GPIO 22 → D-pad Left

class CUSBDS4Gadget;

class CGPIOController
{
public:
    CGPIOController (void);

    // Poll button GPIO states and send to the host
    void PollAndSend (CUSBDS4Gadget *pGadget);

private:
    enum TPinNumber
    {
        PinNorth     = 4,
        PinEast      = 5,
        PinSouth     = 6,
        PinWest      = 16,
        PinDpadUp    = 17,
        PinDpadRight = 20,
        PinDpadDown  = 21,
        PinDpadLeft  = 22
    };

    CGPIOPin m_PinNorth;
    CGPIOPin m_PinEast;
    CGPIOPin m_PinSouth;
    CGPIOPin m_PinWest;
    CGPIOPin m_PinDpadUp;
    CGPIOPin m_PinDpadRight;
    CGPIOPin m_PinDpadDown;
    CGPIOPin m_PinDpadLeft;

    u8 nCounter;
};

#endif
