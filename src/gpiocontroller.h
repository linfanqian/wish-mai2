#ifndef _gpiocontroller_h
#define _gpiocontroller_h

#include <circle/gpiopin.h>
#include <circle/types.h>

// GPIO pin assignments (all active-low, internal pull-up)
//
//  GPIO 4  → North  (Triangle)     GPIO 5  → D-pad Up
//  GPIO 17 → East   (Circle)       GPIO 6  → D-pad Right
//  GPIO 27 → South  (Cross)        GPIO 13 → D-pad Down
//  GPIO 22 → West   (Square)       GPIO 19 → D-pad Left
//
// Wire each button between the GPIO pin and GND.

class CGPIOController
{
public:
    struct TButtonState
    {
        boolean bNorth;      // Triangle
        boolean bEast;       // Circle
        boolean bSouth;      // Cross / X
        boolean bWest;       // Square
        boolean bDpadUp;
        boolean bDpadRight;
        boolean bDpadDown;
        boolean bDpadLeft;
    };

    CGPIOController (void);

    void Read (TButtonState *pState);

private:
    CGPIOPin m_PinNorth;
    CGPIOPin m_PinEast;
    CGPIOPin m_PinSouth;
    CGPIOPin m_PinWest;
    CGPIOPin m_PinDpadUp;
    CGPIOPin m_PinDpadRight;
    CGPIOPin m_PinDpadDown;
    CGPIOPin m_PinDpadLeft;
};

#endif
