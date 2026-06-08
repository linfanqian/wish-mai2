/* This file provides some tests for button GPIO and DS4 simulation. */

#ifndef _button_test_h
#define _button_test_h

#include <circle/gpiopin.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include "usbds4gadget.h"

class CButtonTest
{
public:
    // Connect buttons to TPinNumber. 
    // Log the press and release through UART
    void TestGPIO (void);

    // Repeatedly simulate all 8 button presses (4 face + 4 D-pad) 
    // and send to host
    void TestDS4Simulation (CUSBDS4Gadget *pGadget, CTimer *pTimer);

private:
    enum TPinNumber
    {
        PinA = 20,
        PinB = 21
    };
};

#endif