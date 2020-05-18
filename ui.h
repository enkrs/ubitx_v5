#ifndef UBITX_UI_H_
#define UBITX_UI_H_

#include <U8x8lib.h>

#define U8X8_MAINFONT u8x8_font_amstrad_cpc_extended_u
#define U8X8_DIGITFONT u8x8_font_profont29_2x3_n

namespace ui {

extern U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8;
const int debounce_count = 500;

char BtnDown();
void BtnWaitUp();
void PrintLine(unsigned char linenmbr, const char *c);
void PrintStatus(const char *c);
void PrintStatusValue(const char *c, const char *v);
void UpdateDisplay();
void UpdateVoltage();

}  // namespace

#endif  // UBITX_UI_H_