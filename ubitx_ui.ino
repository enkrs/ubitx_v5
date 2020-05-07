/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * and the 16x2 LCD display.
 * The upper line of the display is constantly used to display frequency and status
 * of the radio. Occasionally, it is used to provide a two-line information that is 
 * quickly cleared up.
 */


//returns true if the button is pressed
int btnDown(){
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

// The generic routine to display one line on the LCD 
void printLine(char linenmbr, const char *c) {
  if (c[0] == 0) {
    u8x8.clearLine(linenmbr * 2);
    u8x8.clearLine(linenmbr * 2 + 1);
    return;
  }
  if (strcmp(c, printBuff[linenmbr])) {     // only refresh the display when there was a change
    u8x8.draw1x2String(0, linenmbr * 2, c);
    strcpy(printBuff[linenmbr], c);

    for (byte i = strlen(c); i < 16; i++) { // add white spaces until the end of the 16 characters line is reached
      u8x8.draw1x2Glyph(i, linenmbr * 2, ' ');
    }
  }
}


//  short cut to print to the first line
void printLine1(const char *c){
  printLine(1,c);
}
//  short cut to print to the first line
void printLine2(const char *c){
  printLine(0,c);
}

// this builds up the top line of the display with frequency and mode
void updateDisplay() {
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(frequency, b, DEC);

  if (inTx){
    if (cwTimeout > 0)
      strcpy(c, "   CW:");
    else
      strcpy(c, "   TX:");
  }
  else {
    if (ritOn)
      strcpy(c, "RIT ");
    else {
      if (isUSB)
        strcpy(c, "USB ");
      else
        strcpy(c, "LSB ");
    }
    if (vfoActive == VFO_A) // VFO A is active
      strcat(c, "A:");
    else
      strcat(c, "B:");
  }



  //one mhz digit if less than 10 M, two digits if more
  if (frequency < 10000000l){
    c[6] = ' ';
    c[7]  = b[0];
    strcat(c, ".");
    strncat(c, &b[1], 3);    
    strcat(c, ".");
    strncat(c, &b[4], 3);
  }
  else {
    strncat(c, b, 2);
    strcat(c, ".");
    strncat(c, &b[2], 3);
    strcat(c, ".");
    strncat(c, &b[5], 3);    
  }

  if (inTx)
    strcat(c, " TX");
  printLine(1, c);

/*
  //now, the second line
  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  if (inTx)
    strcat(c, "TX ");
  else if (ritOn)
    strcpy(c, "RIT");

  strcpy(c, "      \xff");
  drawMeter(meter_reading);
  strcat(c, meter);
  strcat(c, "\xff");
  printLine2(c);*/
}

int enc_prev_state = 3;

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
 * The enc_read returns the number of net pulses counted over 50 msecs. 
 * If the puluses are -ve, they were anti-clockwise, if they are +ve, the
 * were in the clockwise directions. Higher the pulses, greater the speed
 * at which the enccoder was spun
 */

byte enc_state (void) {
    //return (analogRead(ENC_A) > 500 ? 1 : 0) + (analogRead(ENC_B) > 500 ? 2: 0);
    return (digitalRead(ENC_A) == LOW ? 1 : 0) + (digitalRead(ENC_B) == LOW ? 2: 0);
}

int enc_read(void) {
  int result = 0; 
  byte newState;
  int enc_speed = 0;
  
  long stop_by = millis() + 50;
  
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
    active_delay(1);
  }
  return(result);
}


