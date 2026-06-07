#ifndef _kernel_h
#define _kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include "usbds4gadget.h"
#include "gpiocontroller.h"

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);
	TShutdownMode Run (void);

private:
	void BuildReport (const CGPIOController::TButtonState &state, u8 *pReport,
			  u8 nCounter);

	// Member declaration order must not change: each constructor may take
	// a pointer to a previously declared member.
	CActLED            m_ActLED;
	CKernelOptions     m_Options;
	CDeviceNameService m_DeviceNameService;
	CSerialDevice      m_Serial;
	CExceptionHandler  m_ExceptionHandler;
	CInterruptSystem   m_Interrupt;
	CTimer             m_Timer;
	CLogger            m_Logger;
	Cusbds4gadget      m_PS4Gadget;
	CGPIOController    m_GPIO;
};

#endif
