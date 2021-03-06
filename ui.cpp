/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * ....
 */
#include "ui.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include "hw.h"
#include "ubitx.h"
#include "mainloop.h"
#include "keyer.h"

namespace ui {

unsigned long last_v_update = 0;
int prev_voltage = -1;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// The generic routine to display one line on the LCD
void PrintLine(unsigned char line_nr, const char *c) {
  if (c[0] == 0) {
    u8x8.clearLine(line_nr);
    u8x8.clearLine(line_nr + 1);
    return;
  }

  u8x8.draw1x2String(1, line_nr, c);

  // add white spaces until the end of the 16 characters line is reached
  for (unsigned char i = strlen(c); i < 15; i++) {
    u8x8.draw1x2Glyph(i + 1, line_nr, ' ');
  }
}


void PrintLineValue(unsigned char line_nr, const char *c, const char *v) {
  u8x8.draw1x2String(1, line_nr, c);
  u8x8.draw1x2String(15 - strlen(v) + 1, line_nr, v);
  for (unsigned char i = strlen(c) + 1; i <= 15 - strlen(v); i++) {
    u8x8.draw1x2Glyph(i, line_nr, ' ');
  }
}

// this builds up the top line of the display with frequency and mode
void UpdateDisplay() {
  if (ubitx::in_tx) {
    // 123456789012345
    // ___________=TX=
    u8x8.draw1x2String(1, 1, "           ");
    u8x8.setInverseFont(1);
    u8x8.draw1x2String(12, 1, keyer::cw_timeout > 0 ? " CW " : " TX ");
    u8x8.setInverseFont(0);
  } else {
    // 123456789012345
    // ....__RIT_USB_A
    //           13.7V
    switch (ubitx::status.shift_mode) {
      case 0:
        u8x8.draw1x2String(7, 1, "   ");
        break;
      case 1:
        u8x8.draw1x2String(7, 1, "RIT");
        break;
      case 2:
        u8x8.draw1x2String(7, 1, "SPL");
        break;
    }
    u8x8.draw1x2Glyph(10, 1, ' ');

    u8x8.draw1x2String(11, 1, ubitx::status.is_usb ? "USB " : "LSB ");
    u8x8.draw1x2Glyph(15, 1, ubitx::status.vfo_a_active ? 'A' : 'B');
  }
  PrintFrequency();

  last_v_update = 0; prev_voltage = -1;
  UpdateVoltage();
}

void PrintFrequency() {
  char b[11]; // holds string up to "4294967295\0"
  ultoa(ubitx::frequency, b, DEC);

  u8x8.setFont(U8X8_DIGITFONT);
  // one mhz digit if less than 10 M, two digits if more
  unsigned char n = 0;
  u8x8.drawGlyph(1, 3, ubitx::frequency < 10000000l ? ' ' : b[n++]);
  u8x8.drawGlyph(3, 3, b[n++]);
  u8x8.drawGlyph(5, 3, b[n++]);
  u8x8.drawGlyph(7, 3, b[n++]);
  u8x8.drawGlyph(9, 3, b[n++]);
  // .
  u8x8.drawGlyph(12, 3, b[n++]);
  u8x8.drawGlyph(14, 3, b[n++]);
  u8x8.setFont(U8X8_MAINFONT);
}

void UpdateVoltage() {
  if (last_v_update + 500 > millis()) return;
  last_v_update = millis();
  // 3.7 volts were tead as 189
  // 11.9V volts were read as 552:
  int cur_voltage = map(analogRead(hw::ANALOG_V), 189, 552, 37, 119);
  if (cur_voltage < 10) {
    u8x8.draw1x2String(11, 6, "     ");
    return;
  }
  if (cur_voltage != prev_voltage) {
    char b[7]; // holds string up to "-32767\0"
    prev_voltage = cur_voltage;
    itoa(cur_voltage, b, DEC);
    int n = 0;
    u8x8.draw1x2Glyph(11, 6, cur_voltage < 100 ? ' ' : b[n++]);
    u8x8.draw1x2Glyph(12, 6, b[n++]);
    u8x8.draw1x2Glyph(13, 6, '.');
    u8x8.draw1x2Glyph(14, 6, b[n++]);
    u8x8.draw1x2Glyph(15, 6, 'V');
  }
}

}  // namespace
