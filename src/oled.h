/* This file implements SSD1306 128x64 OLED display driver.
  
   Code is ported from my assignment for
   https://github.com/dddrrreee/cs240lx-26spr/tree/main/labs/15-oled-display */

#ifndef _oled_h
#define _oled_h

#include <circle/i2cmaster.h>
#include <circle/types.h>

class COLED
{
public:
    COLED (CI2CMaster *pI2CMaster);

    boolean Initialize (void);

    void Clear (void);
    void Show (void);

    void DrawPixel (unsigned nX, unsigned nY, boolean bOn);
    void DrawLine (unsigned nX0, unsigned nY0, unsigned nX1, unsigned nY1, boolean bOn);
    void DrawFillRect (unsigned nX, unsigned nY, unsigned nW, unsigned nH, boolean bOn);
    void DrawButton   (unsigned nCX, unsigned nCY, boolean bPressed);

    // Draw a single character at pixel position (x, y), scale 1 = 5x7 px.
    void DrawChar (unsigned nX, unsigned nY, char c, unsigned nScale = 1);
    // Draw a NUL-terminated string starting at (nX, nY).
    void DrawText (unsigned nX, unsigned nY, const char *pText, unsigned nScale = 1);

private:
    // Send a byte over I2C
    void SendCommand (u8 cmd);

    CI2CMaster *m_pI2CMaster;

    // Display buffer: 1 control byte + 128*8 data bytes
    enum
    {
        DisplayWidth      = 128,
        DisplayHeight     = 64,
        DisplayBufferSize = DisplayWidth * ((DisplayHeight + 7) / 8),
        I2CBufferSize     = DisplayBufferSize + 1,
        I2CAddress        = 0x3C
    };
    
    u8 m_I2CBuffer[I2CBufferSize];   // m_Buffer[0] = 0x40 control byte
    u8 *m_DisplayBuffer;
};

#endif
