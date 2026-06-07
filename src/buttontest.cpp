#include "buttontest.h"
#include "usbds4gadgetendpoint.h"

// Log button action if there's any
static void LogButtonAction (const char *pPinName, unsigned prevAct, unsigned curAct)
{
    if (prevAct == curAct) return;

    // Release is 1 (pull-up)
    CLogger::Get ()->Write (
        "buttontest", LogNotice, "Pin %s: %s", pPinName, curAct ? "released" : "pressed");
}

void CButtonTest::TestGPIO (void)
{
    CGPIOPin pinA (PinA, GPIOModeInputPullUp);
    CGPIOPin pinB (PinB, GPIOModeInputPullUp);

    unsigned nPrevA = 1, nPrevB = 1;

    CLogger::Get ()->Write ("buttontest", LogNotice, "Button GPIO test ready");

    while (1)
    {
        unsigned nCurA = pinA.Read ();
        unsigned nCurB = pinB.Read ();

        LogButtonAction ("A", nPrevA, nCurA);
        LogButtonAction ("B", nPrevB, nCurB);

        nPrevA = nCurA;
        nPrevB = nCurB;
    }
}

void CButtonTest::TestDS4Simulation (CUSBDS4Gadget *pGadget, CTimer *pTimer)
{
    // Report per 5ms
    // Ref: bInterval of Endpoint Descriptor (IN) at
    // https://www.psdevwiki.com/ps4/DS4-USB

    // Press+release on each button for 2s
    // Each button is pressed for 200ms
    const unsigned nPressCycles = 40;    // 200 ms
    const unsigned nReleaseCycles = 360; // 1800 ms
    const unsigned nTotalCycles = nPressCycles + nReleaseCycles;

    const TDS4ButtonState s_Sequence[] =
    {
        { TRUE,  FALSE, FALSE, FALSE,  FALSE, FALSE, FALSE, FALSE },  // North
        { FALSE, TRUE,  FALSE, FALSE,  FALSE, FALSE, FALSE, FALSE },  // East
        { FALSE, FALSE, TRUE,  FALSE,  FALSE, FALSE, FALSE, FALSE },  // South
        { FALSE, FALSE, FALSE, TRUE,   FALSE, FALSE, FALSE, FALSE },  // West
        { FALSE, FALSE, FALSE, FALSE,  TRUE,  FALSE, FALSE, FALSE },  // D-pad Up
        { FALSE, FALSE, FALSE, FALSE,  FALSE, TRUE,  FALSE, FALSE },  // D-pad Right
        { FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE, TRUE,  FALSE },  // D-pad Down
        { FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE, FALSE, TRUE  },  // D-pad Left
    };
    const unsigned nSteps = sizeof s_Sequence / sizeof *s_Sequence;
    const TDS4ButtonState s_Neutral = { FALSE, FALSE, FALSE, FALSE,
                                        FALSE, FALSE, FALSE, FALSE };

    u8 report[DS4_REPORT_SIZE];
    u8 nCounter = 0;
    unsigned nStep = 0;
    unsigned nCycle = 0;

    while (1)
    {
        pGadget->UpdatePlugAndPlay ();

        const TDS4ButtonState *pState = (nCycle < nPressCycles)
                                        ? &s_Sequence[nStep]
                                        : &s_Neutral;

        CUSBDS4GadgetEndpoint::CreateDS4Report (report, pState, nCounter++);
        pGadget->SendDS4Report (report);

        if (++nCycle >= nTotalCycles)
        {
            nCycle = 0;
            nStep  = (nStep + 1) % nSteps;
        }

        pTimer->MsDelay (5);
    }
}