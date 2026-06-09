/* This file implementes the interrupt-based async I2C master for BSC1
   (GPIO 2 SDA / GPIO 3 SCL). */

#ifndef _i2cmasterasync_h
#define _i2cmasterasync_h

#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CI2CMasterAsync
{
public:
    CI2CMasterAsync (CInterruptSystem *pInterrupt);

    boolean Initialize ();

    void Write (u8 ucAddress, const void *pBuffer, unsigned nCount);
    void WriteAsync (u8 ucAddress, const void *pBuffer, unsigned nCount);

    boolean IsBusy () const { return m_bBusy; }

private:
    void FillFIFO ();
    void IRQHandler ();

    // Wrapper for handler, as handler registration requires a long-live
    // function address. It receives "this" via pParam and cast it to invoke
    // real IRQHandler.
    static void IRQWrapper (void *pParam);

    CGPIOPin          m_SDA;   // GPIO 2, ALT0
    CGPIOPin          m_SCL;   // GPIO 3, ALT0
    CInterruptSystem *m_pInterrupt;

    volatile boolean  m_bBusy;
    const u8         *m_pBuf;
    unsigned          m_nRemaining;
};

#endif
