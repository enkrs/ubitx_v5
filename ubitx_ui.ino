/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * ....
 */



//returns 1 if the button is pressed
char BtnDown() {
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

void BtnWaitUp() {
  while (digitalRead(FBUTTON) == LOW)
    ActiveDelay(50);
  ActiveDelay(50);
}

// The generic routine to display one line on the LCD 
void PrintLine(char linenmbr, const char *c) {
  if (c[0] == 0) {
    u8x8.clearLine(linenmbr);
    u8x8.clearLine(linenmbr + 1);
    return;
  }

  u8x8.draw1x2String(1, linenmbr, c);

  for (unsigned char i = strlen(c); i < 15; i++) { // add white spaces until the end of the 16 characters line is reached
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
  // 0123456789012345
  // X[-5-]xxxx[-6--]
  for (unsigned char i = strlen(c) + 1; i <= 15 - strlen(v); i++) { // add white spaces until the end of the 16 characters line is reached
    u8x8.draw1x2Glyph(i, 6, ' ');
  }
}

// this builds up the top line of the display with frequency and mode
void UpdateDisplay() {
  if (in_tx) {
    // 123456789012345
    // ___________=TX=
    u8x8.draw1x2String(1, 1, "           ");
    u8x8.setInverseFont(1);
    u8x8.draw1x2String(12, 1, cw_timeout > 0 ? " CW " : " TX ");
    u8x8.setInverseFont(0);
  } else {
    // 123456789012345
    // ______RIT_USB_A
    if (rit_on)
      u8x8.draw1x2String(7, 1, "RIT");
    else if (split_on)
      u8x8.draw1x2String(7, 1, "SPL");
    else
      u8x8.draw1x2String(7, 1, "   ");
    u8x8.draw1x2Glyph(10, 1, ' ');

    u8x8.draw1x2String(11, 1, is_usb ? "USB " : "LSB ");
    u8x8.draw1x2Glyph(15, 1, vfo_active == VFO_A ? 'A' : 'B');
  }

  memset(b, 0, sizeof(b));
  ultoa(frequency, b, DEC);

  u8x8.setFont(U8X8_DIGITFONT);
  //one mhz digit if less than 10 M, two digits if more
  unsigned char n = 0;
  u8x8.drawGlyph(1, 3, frequency < 10000000l ? ' ' : b[n++]);
  u8x8.drawGlyph(3, 3, b[n++]);
  u8x8.drawGlyph(5, 3, b[n++]);
  u8x8.drawGlyph(7, 3, b[n++]);
  u8x8.drawGlyph(9, 3, b[n++]);
  // .
  u8x8.drawGlyph(12, 3, b[n++]);
  u8x8.drawGlyph(14, 3, b[n++]);
  u8x8.setFont(U8X8_MAINFONT);
}
