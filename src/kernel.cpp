#include "kernel.h"
#include <circle/util.h>
#include <circle/logger.h>

/* Regular octagon, Rx=58 Ry=24 centre=(64,32), vertices at 45° intervals.
    Clockwise from top-left:

    left(0) -------- north(1)
    /                      \
  down(7)                 east(2)
   |                        |
  right(6)                south(3)
    \                      /
    up(5) ---------- west(4) 
*/
struct TButtonPos { unsigned nCX; unsigned nCY; };
static const TButtonPos s_ButtonPos[] =
{
    { 42, 10},  // 0: D-pad Left  (top-left)
    { 86, 10},  // 1: North       (top-right)
    {118, 23},  // 2: East        (right-upper)
    {118, 41},  // 3: South       (right-lower)
    { 86, 54},  // 4: West        (bottom-right)
    { 42, 54},  // 5: D-pad Up    (bottom-left)
    { 10, 41},  // 6: D-pad Right (left-lower)
    { 10, 23},  // 7: D-pad Down  (left-upper)
};

static void DrawOLEDButtons (COLED *pOled, TDS4ButtonState &state)
{
    boolean bButtons[] = {
        state.bDpadLeft,   // 0: left
        state.bNorth,      // 1: north
        state.bEast,       // 2: east
        state.bSouth,      // 3: south
        state.bWest,       // 4: west
        state.bDpadUp,     // 5: up
        state.bDpadRight,  // 6: right
        state.bDpadDown,   // 7: down
    };

    pOled->Clear ();

    // Draw octagon edges
    for (unsigned i = 0; i < 8; i++)
    {
        unsigned j = (i + 1) % 8;
        pOled->DrawLine (s_ButtonPos[i].nCX, s_ButtonPos[i].nCY,
                            s_ButtonPos[j].nCX, s_ButtonPos[j].nCY, TRUE);
    }

    // Draw buttons on at vertices
    for (unsigned i = 0; i < 8; i++)
        pOled->DrawButton (s_ButtonPos[i].nCX, s_ButtonPos[i].nCY, bButtons[i]);

    pOled->Show ();
}

CKernel::CKernel (void)
:    m_Timer (&m_Interrupt),
    m_Logger (m_Options.GetLogLevel (), &m_Timer),
    m_I2CMaster (1),
    m_OLED (&m_I2CMaster),
    m_PS4Gadget (&m_Interrupt)
{
    m_ActLED.Blink (5);
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
    boolean bOK = TRUE;

    if (bOK) bOK = m_Serial.Initialize (115200);
    if (bOK) bOK = m_Logger.Initialize (&m_Serial);
    if (bOK) bOK = m_Interrupt.Initialize ();
    if (bOK) bOK = m_Timer.Initialize ();
    if (bOK) bOK = m_I2CMaster.Initialize ();
    if (bOK) bOK = m_OLED.Initialize ();
    if (bOK) bOK = m_PS4Gadget.Initialize ();

    if (bOK)
        CLogger::Get ()->Write ("kernel", LogNotice,
                                "PS4 gadget ready - waiting for USB host");

    return bOK;
}

TShutdownMode CKernel::Run (void)
{
    TDS4ButtonState prevState = {};
    DrawOLEDButtons (&m_OLED, prevState);

    while (1)
    {
        m_PS4Gadget.UpdatePlugAndPlay ();

        TDS4ButtonState state;
        m_GPIO.Read (&state);
        m_GPIO.Send (&m_PS4Gadget, state);

        // Update OLED on state change
        if (memcmp (&state, &prevState, sizeof state))
        {
            DrawOLEDButtons (&m_OLED, state);
            prevState = state;
        }

        // Ref: bInterval of Endpoint Descriptor (IN) at
        // https://www.psdevwiki.com/ps4/DS4-USB
        m_Timer.MsDelay (5);
    }

    return ShutdownHalt;
}
