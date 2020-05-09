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
  PrintLine(6,c);
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
    u8x8.draw1x2Glyph(11,0,' ');
    u8x8.setInverseFont(1);
    u8x8.draw1x2String(12,0,cw_timeout > 0 ? " CW " : " TX ");
    u8x8.setInverseFont(0);
  } else {
    if (rit_on)
      u8x8.draw1x2String(11,0,"RIT");
    else
      u8x8.draw1x2String(11,0, is_usb ? "USB" : "LSB");
    u8x8.draw1x2Glyph(14,0,' ');
    u8x8.draw1x2Glyph(15,0, vfo_active == VFO_A ? 'A' : 'B');
  }

  memset(b, 0, sizeof(b));
  ultoa(frequency, b, DEC);

  //one mhz digit if less than 10 M, two digits if more
  unsigned char n = 0;
  if (frequency < 10000000l) {
    u8x8.draw2x2Glyph(1, 1, b[n++]);
    u8x8.draw1x2Glyph(3, 1, '.');
    u8x8.draw2x2Glyph(4, 1, ' '); // clear last digit in change from 10. to 9.
  } else {
    u8x8.draw2x2Glyph(1, 1, b[n++]);
    u8x8.draw2x2Glyph(3, 1, b[n++]);
    u8x8.draw1x2Glyph(5, 1, '.');
  }
  u8x8.draw2x2Glyph(2, 3, b[n++]);
  u8x8.draw2x2Glyph(4, 3, b[n++]);
  u8x8.draw2x2Glyph(6, 3, b[n++]);
  u8x8.draw1x2Glyph(8, 3, '.');
  u8x8.draw2x2Glyph(9, 3, b[n++]);
  u8x8.draw2x2Glyph(11, 3, b[n++]);
  u8x8.draw2x2Glyph(13, 3, b[n]);
}


/**
 * The A7 And A6 are purely analog lines on the Arduino Nano
 * These need to be pulled up externally using two 10 K resistors
 * 
 * There are excellent pages on the Internet about how these encoders work
 * and how they should be used. We have elected to use the simplest way
 * to use these encoders without the complexity of interrupts etc to 
 * keep it understandable.
 * 
 * The enc_state returns a two-bit number such that each bit reflects the current
 * value of each of the two phases of the encoder
 * 
 * The EncRead returns the number of net pulses counted over 50 msecs. 
 * If the puluses are -ve, they were anti-clockwise, if they are +ve, the
 * were in the clockwise directions. Higher the pulses, greater the speed
 * at which the enccoder was spun
 */

char enc_state (void) {
    //return (analogRead(ENC_A) > 500 ? 1 : 0) + (analogRead(ENC_B) > 500 ? 2: 0);
    return (digitalRead(ENC_A) == LOW ? 1 : 0) + (digitalRead(ENC_B) == LOW ? 2: 0);
}

int EncRead(void) {
  static int enc_prev_state = 3;
  int result = 0; 
  int enc_speed = 0;
  char new_state;
  
  unsigned long stop_by = millis() + 50;
  
  while (millis() < stop_by) { // check if the previous state was stable
    newState = enc_state(); // Get current state  
    
    if (newState != enc_prev_state)
      delay (1);
    
    if (enc_state() != newState || newState == enc_prev_state)
      continue; 
    //these transitions point to the encoder being rotated anti-clockwise
    if ((enc_prev_state == 0 && newState == 2) || 
      (enc_prev_state == 2 && newState == 3) || 
      (enc_prev_state == 3 && newState == 1) || 
      (enc_prev_state == 1 && newState == 0)){
        result--;
      }
    //these transitions point o the enccoder being rotated clockwise
    if ((enc_prev_state == 0 && newState == 1) || 
      (enc_prev_state == 1 && newState == 3) || 
      (enc_prev_state == 3 && newState == 2) || 
      (enc_prev_state == 2 && newState == 0)){
        result++;
      }
    enc_prev_state = newState; // Record state for next pulse interpretation
    enc_speed++;
    ActiveDelay(1);
  }
  return(result);
}