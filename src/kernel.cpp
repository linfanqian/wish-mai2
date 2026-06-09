#include "kernel.h"
#include "buttontest.h"
#include <circle/util.h>
#include <circle/logger.h>

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
	

    while (1)
    {
        m_GPIO.PollAndSend (&m_PS4Gadget);
    }

    return ShutdownHalt;
}

