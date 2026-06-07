#include "kernel.h"
#include "buttontest.h"
#include <circle/util.h>
#include <circle/logger.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:    m_Timer (&m_Interrupt),
    m_Logger (m_Options.GetLogLevel (), &m_Timer),
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
    if (bOK) bOK = m_PS4Gadget.Initialize ();

    if (bOK)
        CLogger::Get ()->Write (FromKernel, LogNotice,
                    "PS4 gadget ready — waiting for USB host");

    return bOK;
}

TShutdownMode CKernel::Run (void)
{
    CButtonTest test;
    test.test_ds4_simulation (&m_PS4Gadget, &m_Timer);

    return ShutdownHalt;
}

