/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * ....
 */
#include "ui.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include "hw.h"
#include "ubitx.h"
#include "keyer.h"

namespace ui {

unsigned long last_v_update = 0;
int prev_voltage = -1;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// returns 1 if the button is pressed
char BtnDown() {
  //if (digitalRead(hw::FBUTTON) == HIGH)
  // button must be down (0) for debounce_count samples in a row
  for (int i = 0; i < debounce_count; i++)
    if ((PINC & (1<<PC2)) != 0) return 0;

  return 1;
}

void BtnWaitUp() {
  // button must be up (1) for 10 samples in a row
  int i = 0;
  while (i < debounce_count) {
    i++;
    // reset counter if button down (0)
    if ((PINC & (1<<PC2)) == 0) i = 0;
  }
}

// The generic routine to display one line on the LCD
void PrintLine(unsigned char linenmbr, const char *c) {
  if (c[0] == 0) {
    u8x8.clearLine(linenmbr);
    u8x8.clearLine(linenmbr + 1);
    return;
  }

  u8x8.draw1x2String(1, linenmbr, c);

  // add white spaces until the end of the 16 characters line is reached
  for (unsigned char i = strlen(c); i < 15; i++) {
    u8x8.draw1x2Glyph(i + 1, linenmbr, ' ');
  }
}


//  short cut to print to the first line
void PrintStatus(const char *c) {
  PrintLine(6, c);
}
void PrintStatusValue(const char *c, const char *v) {
  u8x8.draw1x2String(1, 6, c);
  u8x8.draw1x2String(15 - strlen(v) + 1, 6, v);
  for (unsigned char i = strlen(c) + 1; i <= 15 - strlen(v); i++) {
    u8x8.draw1x2Glyph(i, 6, ' ');
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
    // ______RIT_USB_A
    //           13.7V
    switch (ubitx::shift_mode) {
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

    u8x8.draw1x2String(11, 1, ubitx::is_usb ? "USB " : "LSB ");
    u8x8.draw1x2Glyph(15, 1, ubitx::vfo_active == ubitx::VFO_ACTIVE_A ? 'A' : 'B');
  }

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

  last_v_update = 0; prev_voltage = -1;
  UpdateVoltage();
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
