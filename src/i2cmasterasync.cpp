#include "i2cmasterasync.h"
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/logger.h>

/* See circle/lib/i2cmaster.cpp for the following registers.
   Also you can see BCM2835 p29-p32. */
// Control register
#define C_I2CEN  (1 << 15)
#define C_INTT   (1 << 9)   // interrupt on TX not full
#define C_INTD   (1 << 8)   // interrupt on TX Done
#define C_ST     (1 << 7)   // start transfer
#define C_CLEAR  (1 << 5)   // clear FIFO

// Status register
#define S_CLKT   (1 << 9)   // clock stretch timeout (write 1 to clear)
#define S_ERR    (1 << 8)   // ACK error (write 1 to clear)
#define S_TXD    (1 << 4)   // FIFO can accept data
#define S_TXW    (1 << 2)   // FIFO needs writing (interrupt flag)
#define S_DONE   (1 << 1)   // transfer done (write 1 to clear)
#define S_TA     (1 << 0)   // transfer active

#define BSC_BASE      ARM_BSC1_BASE
#define BSC_FIFO_SIZE 16

// Register offsets
#define C_OFF    ARM_BSC_C__OFFSET
#define S_OFF    ARM_BSC_S__OFFSET
#define DLEN_OFF ARM_BSC_DLEN__OFFSET
#define A_OFF    ARM_BSC_A__OFFSET
#define FIFO_OFF ARM_BSC_FIFO__OFFSET
#define DIV_OFF  ARM_BSC_DIV__OFFSET

CI2CMasterAsync::CI2CMasterAsync (CInterruptSystem *pInterrupt)
:   m_SDA (2, GPIOModeAlternateFunction0),
    m_SCL (3, GPIOModeAlternateFunction0),
    m_pInterrupt (pInterrupt),
    m_bBusy (FALSE),
    m_pBuf (0),
    m_nRemaining (0)
{
}

boolean CI2CMasterAsync::Initialize ()
{
    // Enable BSC
    write32 (BSC_BASE + C_OFF, C_I2CEN);

    // Set clock divider, up to 400kHz
    unsigned nCoreClk = CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE);
    write32 (BSC_BASE + DIV_OFF, nCoreClk / 400000);

    // Clear status register - CLKT, ERR, and DONE
    write32 (BSC_BASE + S_OFF, S_CLKT | S_ERR | S_DONE);

    // Register interrupt handler
    m_pInterrupt->ConnectIRQ (ARM_IRQ_I2C, IRQWrapper, this);
    CInterruptSystem::EnableIRQ (ARM_IRQ_I2C);

    CLogger::Get ()->Write ("i2casync", LogNotice, "BSC1 async ready");
    return TRUE;
}

// Normal blocking write
// For short-lived stack buffers
void CI2CMasterAsync::Write (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
    const u8 *pBuf  = (const u8 *) pBuffer;
    unsigned  nLeft = nCount;

    // Wait until transfer is not active
    while (read32 (BSC_BASE + S_OFF) & S_TA)
        ;

    // Mark transfer start
    write32 (BSC_BASE + S_OFF, S_CLKT | S_ERR | S_DONE);
    write32 (BSC_BASE + A_OFF, ucAddress);
    write32 (BSC_BASE + DLEN_OFF, nCount);

    // Clear FIFO and start transfer
    write32 (BSC_BASE + C_OFF, C_I2CEN | C_CLEAR | C_ST);

    while (!(read32 (BSC_BASE + S_OFF) & S_DONE))
    {
        while (nLeft > 0 && (read32 (BSC_BASE + S_OFF) & S_TXD))
        {
            write32 (BSC_BASE + FIFO_OFF, *pBuf++);
            nLeft--;
        }
    }

    // Clear DONE
    write32 (BSC_BASE + S_OFF, S_DONE);
}

// Fills FIFO, starts transfer, and returns immediately.
// Remaining bytes are fed by IRQHandler when TX is not full.
void CI2CMasterAsync::WriteAsync (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
    m_pBuf = (const u8 *) pBuffer;
    m_nRemaining = nCount;
    m_bBusy = TRUE;
    // Make sure all above are flushed before used
    DataSyncBarrier ();

    // Wait until transfer is not active
    while (read32 (BSC_BASE + S_OFF) & S_TA)
        ;

    // Mark transfer start
    write32 (BSC_BASE + S_OFF, S_CLKT | S_ERR | S_DONE);
    write32 (BSC_BASE + A_OFF, ucAddress);  // set device addr
    write32 (BSC_BASE + DLEN_OFF, nCount);  // set device len

    // Clear FIFO and fill it before kicking off the transfer
    write32 (BSC_BASE + C_OFF, C_I2CEN | C_CLEAR);
    FillFIFO ();

    // Set C to start transfer with interrupts enabled
    write32 (BSC_BASE + C_OFF, C_I2CEN | C_ST | C_INTT | C_INTD);
}

void CI2CMasterAsync::FillFIFO ()
{
    while (m_nRemaining > 0 && (read32 (BSC_BASE + S_OFF) & S_TXD))
    {
        write32 (BSC_BASE + FIFO_OFF, *m_pBuf++);
        m_nRemaining--;
    }
}

void CI2CMasterAsync::IRQHandler ()
{
    u32 nStatus = read32 (BSC_BASE + S_OFF);

    if (nStatus & S_ERR)
    {
        // Silently clear the error and mark transfer done
        m_bBusy = FALSE;
        write32 (BSC_BASE + S_OFF, S_ERR);     // clear ERR
        write32 (BSC_BASE + C_OFF, C_I2CEN);   // interrupt disabled
        return;
    }

    if (nStatus & S_TXW)
        FillFIFO ();

    if (nStatus & S_DONE)
    {
        m_bBusy = FALSE;
        write32 (BSC_BASE + S_OFF, S_DONE);    // clear DONE
        write32 (BSC_BASE + C_OFF, C_I2CEN);   // disable interrupt
    }
}

void CI2CMasterAsync::IRQWrapper (void *pParam)
{
    ((CI2CMasterAsync *) pParam)->IRQHandler ();
}
