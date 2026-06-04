#include "kernel.h"
#include <circle/util.h>
#include <circle/logger.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Timer (&m_Interrupt),
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
	u8 report[PS4_REPORT_SIZE];
	u8 nCounter = 0;

	while (TRUE)
	{
		m_PS4Gadget.UpdatePlugAndPlay ();

		CGPIOController::TButtonState state;
		m_GPIO.Read (&state);

		BuildReport (state, report, nCounter++);
		m_PS4Gadget.SendReport (report);

		m_Timer.MsDelay (5);
	}

	return ShutdownHalt;
}

void CKernel::BuildReport (const CGPIOController::TButtonState &state,
			    u8 *pReport, u8 nCounter)
{
	memset (pReport, 0, PS4_REPORT_SIZE);

	pReport[0] = 0x01;   // Report ID
	pReport[1] = 0x80;   // LX center
	pReport[2] = 0x80;   // LY center
	pReport[3] = 0x80;   // RX center
	pReport[4] = 0x80;   // RY center

	// D-pad hat (0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=neutral)
	u8 hat = 8;
	if      (state.bDpadUp   && state.bDpadRight) hat = 1;
	else if (state.bDpadRight && state.bDpadDown)  hat = 3;
	else if (state.bDpadDown && state.bDpadLeft)  hat = 5;
	else if (state.bDpadLeft  && state.bDpadUp)   hat = 7;
	else if (state.bDpadUp)                       hat = 0;
	else if (state.bDpadRight)                    hat = 2;
	else if (state.bDpadDown)                     hat = 4;
	else if (state.bDpadLeft)                     hat = 6;

	// Byte 5: hat[3:0] | Square[4] | Cross[5] | Circle[6] | Triangle[7]
	pReport[5] = (hat & 0x0F)
		   | (state.bWest  ? 0x10 : 0)   // Button 1 – Square
		   | (state.bSouth ? 0x20 : 0)   // Button 2 – Cross
		   | (state.bEast  ? 0x40 : 0)   // Button 3 – Circle
		   | (state.bNorth ? 0x80 : 0);  // Button 4 – Triangle

	// Byte 6: L1/R1/L2/R2/Share/Options/L3/R3 (all unconnected → 0)
	pReport[6] = 0x00;

	// Byte 7: PS[0] Touch[1] counter[7:2]
	pReport[7] = (u8) ((nCounter & 0x3F) << 2);

	// Bytes 8-9: L2/R2 analog (not wired → 0)
	// Bytes 10-63: IMU/battery/touchpad padding → already 0
}
