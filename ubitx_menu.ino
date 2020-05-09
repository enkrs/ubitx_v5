/** Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls doMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */

uint8_t screenDirty = 1; // Used across functions to signal redrawing


/** A generic control to read variable values
*/
int getValueByKnob(int minimum, int maximum, int step_size,  int initial, const char* title, const char *postfix)
{
  int knob = 0;
  int knob_value;
  screenDirty = 1;

  knob_value = initial;

  u8x8.clear();
  u8x8.draw1x2String(1,1,title);

  // 0123456789012345
  //           XX  **
  //             **
  //           **
  //         
  int8_t right = 16 - strlen(postfix) * 2;
  u8x8.draw2x2String(right, 4, postfix);
   
  while(!btnDown()) {
    knob = enc_read();
    if (knob != 0) {
      if (knob < 0) knob_value -= step_size;
      if (knob > 0) knob_value += step_size;
      knob_value = (knob_value - minimum) / step_size * step_size + minimum;
      if (knob_value < minimum) knob_value = minimum;
      if (knob_value > maximum) knob_value = maximum;
      screenDirty = 1;
    }
    if (screenDirty) {
      itoa(knob_value, b, 10);
      int8_t i = right - strlen(b) * 2;
      if (i > 0) {
        u8x8.draw2x2String(i, 4, b);
        while (i-- > 1) u8x8.draw1x2Glyph(i, 4, ' ');
      }
      screenDirty = 0;
    }
    checkCAT();
  }
  btnWaitUp();
  u8x8.clear();

  return knob_value;
}

//# Menu: 1
static const char* STR_ON = "ON";
static const char* STR_OFF = "OFF";
static const char* STR_WPM = "WPM";
static const char* STR_BAND_SELECT = "BAND SELECT";
static const char* STR_CW_DELAY = "CW DELAY";
static const char* STR_MENU_CW = "MENU CW";

void menuBand(int btn) {
  int knob = 0;

  if (!btn) {
    printLine6value(STR_BAND_SELECT,"..");
    return;
  }

  printLine6(STR_BAND_SELECT);
  ritDisable();

  while (!btnDown()) {
    knob = enc_read();
    if (knob != 0) {
      if (knob < 0 && frequency - 200000l > LOWEST_FREQ)
        setFrequency(frequency - 200000l);
      if (knob > 0 && frequency + 200000l < HIGHEST_FREQ)
        setFrequency(frequency + 200000l);
      isUSB = frequency > 10000000l ? 1 : 0;
      updateDisplay();
    }
    activeDelay(20);
  }
  btnWaitUp();

  menuState = 1;
}

// Menu #2
void menuRitToggle(int btn) {
  if (!btn) {
    printLine6value("RIT", ritOn ? STR_ON : STR_OFF);
    return;
  }

  if (ritOn == 0)
    ritEnable(frequency);
  else
    ritDisable();

  menuState = 1;
}


//Menu #3
void menuVfoToggle(int btn) {
  if (!btn) {
    printLine6value("VFO", vfoActive == VFO_A ? "A" : "B");
    return;
  }

  if (vfoActive == VFO_B) {
    if (vfoB != frequency) {
      vfoB = frequency;
      EEPROM.put(VFO_B, vfoB);
    }
    if (vfoBUsb != isUSB) {
      vfoBUsb = isUSB;
      EEPROM.put(VFO_B_USB, vfoBUsb);
    }

    vfoActive = VFO_A;
    frequency = vfoA;
    isUSB = vfoAUsb;
  } else {
    if (vfoA != frequency) {
      vfoA = frequency;
      EEPROM.put(VFO_A, vfoA);
    }
    if (vfoAUsb != isUSB) {
      vfoAUsb = isUSB;
      EEPROM.put(VFO_A_USB, vfoAUsb);
    }

    vfoActive = VFO_B;
    frequency = vfoB;
    isUSB = vfoBUsb;
  }
  ritDisable();
  setFrequency(frequency);

  menuState = 1;
}

// Menu #4
void menuSidebandToggle(int btn) {
  if (!btn) {
    printLine6value("MODE", isUSB ? "USB" : "LSB");
    return;
  }

  if (isUSB == 1)
    isUSB = 0;
  else
    isUSB = 1;

  //Added by KD8CEC
  if (vfoActive == VFO_B)
    vfoBUsb = isUSB;
  else
    vfoBUsb = isUSB;

  menuState = 1;
}

//Split communication using VFOA and VFOB by KD8CEC
//Menu #5
void menuSplitToggle(int btn) {
  if (!btn) {
    printLine6value("SPLIT", splitOn ? STR_ON : STR_OFF);
    return;
  }

  if (splitOn == 1) {
    splitOn = 0;
  } else {
    splitOn = 1;
    if (ritOn == 1)
      ritOn = 0;
  }
  menuState = 1;
}

void menuCWSpeed(int btn) {
   int wpm;

   wpm = 1200 / cwSpeed;
     
   if (!btn) {
     itoa(wpm, b, 10);
     strcat(b, " ");
     strcat(b, STR_WPM);
     printLine6value("CW", b);
     return;
   }

   wpm = getValueByKnob(1, 100, 1,  wpm, "CW SPEED", STR_WPM);
   cwSpeed = 1200 / wpm;
   EEPROM.put(CW_SPEED, cwSpeed);

   menuState = 1;
}

void menuExit(int btn) {
  if (!btn) {
    printLine6("EXIT MENU");
    return;
  }

  menuState = 0;
}

/**
 * The calibration routines are not normally shown in the menu as they are rarely used
 * They can be enabled by choosing this menu option
 */
int menuSetup(int btn) {
  if (!btn) {
    printLine6value("ADVANCED", extendedMenu ? STR_ON : "..");
    return 0;
  }

  if (!extendedMenu) {
    extendedMenu = 1;
    return 1;
  }
  extendedMenu = 0;
  menuState = 1;
  return 0;
}

 //this is used by the si5351 routines in the ubitx_5351 file
/*
extern int32_t masterCal;
extern uint32_t si5351bx_vcoa;

void calibrateClock() {
  int knob = 0;

  digitalWrite(TX_LPF_A, 0);
  digitalWrite(TX_LPF_B, 0);
  digitalWrite(TX_LPF_C, 0);

  masterCal = 0;

  isUSB = 1;

  //turn off the second local oscillator and the bfo
  si5351_set_calibration(masterCal);
  startTx(TX_CW);
  si5351bx_setfreq(2, 10000000l); 
  
  strcpy(b, "#1 10 MHz cal:");
  ltoa(masterCal/8750, c, 10);
  strcat(b, c);
  printLine6(b);     

  while (!btnDown()) {
    if (digitalRead(PTT) == LOW && !keyDown)
      cwKeydown();
    if (digitalRead(PTT)  == HIGH && keyDown)
      cwKeyUp();
      
    knob = enc_read();

    if (knob > 0)
      masterCal += 875;
    else if (knob < 0)
      masterCal -= 875;
    else 
      continue; //don't update the frequency or the display
      
    si5351_set_calibration(masterCal);
    si5351bx_setfreq(2, 10000000l);
    //strcpy(b, "#1 10 MHz cal:");
    ltoa(masterCal/8750, c, 10);
    strcat(b, c);
    printLine6(b);     
  }
  btnWaitUp();

  cwTimeout = 0;
  keyDown = 0;
  stopTx();

  EEPROM.put(MASTER_CAL, masterCal);
  initOscillators();
  setFrequency(frequency);    
}
*/

void menuSetupCalibration(int btn) {
  if (!btn) {
    printLine6("CALIBRATE");
    return;
  }

  u8x8.clear();
  u8x8.draw1x2String(1,4,"NOT IMPLEMENTED");
  activeDelay(2000);
  //calibrateClock();
}

void printCarrierFreq(unsigned long freq) {

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(freq, b, DEC);
  
  strncat(c, b, 2);
  strcat(c, ".");
  strncat(c, &b[2], 3);
  strcat(c, ".");
  strncat(c, &b[5], 1);
  printLine6(c);    
}

void menuSetupCarrier(int btn) {
  int knob = 0;
   
  if (!btn) {
    printLine6("CAL BFO");
    return;
  }

  // usbCarrier = 11053000l;
  si5351bx_setfreq(0, usbCarrier);
  u8x8.clear();
  u8x8.draw1x2String(1,1,"CALIBRATE BFO");
  screenDirty = 1;

  while (!btnDown()) {
    knob = enc_read();

    if (knob != 0) {
      usbCarrier += knob;
      if (usbCarrier < 11000000) usbCarrier = 11000000;
      if (usbCarrier > 11099999) usbCarrier = 11099999;
      
      si5351bx_setfreq(0, usbCarrier);
      setFrequency(frequency); // This was not in original. Why?
      screenDirty = 1;
    }
    if (screenDirty) {
      ultoa(usbCarrier, c, DEC);
      printLine(4, c);
    }
    checkCAT();
  }
  btnWaitUp();
  u8x8.clear();

  EEPROM.put(USB_CARRIER, usbCarrier);
  
  si5351bx_setfreq(0, usbCarrier);          
  setFrequency(frequency);    

  menuState = 1; 
}

void menuSetupCwTone(int btn) {
  int knob = 0;
     
  if (!btn) {
    itoa(cwSideTone, b, 10);
    strcat(b, " HZ");
    printLine6value("CW TONE", b);
    return;
  }

  tone(CW_TONE, cwSideTone);
  while (!btnDown()) {
    knob = enc_read();

    if (knob > 0 && cwSideTone < 2000)
      cwSideTone += 10;
    else if (knob < 0 && cwSideTone > 100 )
      cwSideTone -= 10;
    else
      continue; //don't update the frequency or the display
        
    tone(CW_TONE, cwSideTone);
    itoa(cwSideTone, b, 10);
    printLine6(b);

    activeDelay(20);
  }
  btnWaitUp();
  noTone(CW_TONE);
  //save the setting
  EEPROM.put(CW_SIDE_TONE, cwSideTone);
    
  menuState = 1; 
}

void menuSetupCwDelay(int btn) {
  if (!btn) {
    itoa(cwDelayTime, b, 10);
    strcat(b, " MS");
    printLine6value(STR_CW_DELAY, b);
    return;
  }

  cwDelayTime = getValueByKnob(10, 1010, 50,  cwDelayTime, STR_CW_DELAY, "MS");

  menuState = 1;
}

void menuCWMenuSpeed(int btn) {
    int wpm;

    wpm = 1200 / cwMenuSpeed;
     
    if (!btn) {
     itoa(wpm, b, 10);
     strcat(b, " ");
     strcat(b, STR_WPM);
     printLine6value(STR_MENU_CW, b);
     return;
    }

    wpm = getValueByKnob(1, 100, 1,  wpm, STR_MENU_CW, STR_WPM);
    cwMenuSpeed = 1200 / wpm;
    EEPROM.put(CW_MENU_SPEED, cwMenuSpeed);

    menuState = 1;
}


void menuSetupKeyer(int btn) {
  const char* title = "13 CW TYPE";
  if (!btn) {
    switch(iambicKey) {
      case 0:
        printLine6value(title, "STR");
      break;
      case 1:
        printLine6value(title, "IAMB-A");
      break;
      case 2:
        printLine6value(title, "IAMB-B");
      break;
    }
    return;
  }
  
  activeDelay(500);

  /* TODO: implement
  if (!iambicKey)
    tmp_key = 0; //hand key
  else if (keyerControl & IAMBICB)
    tmp_key = 2; //Iambic B
  else 
    tmp_key = 1;
 
  while (!btnDown())
  {
    knob = enc_read();
    if (knob < 0 && tmp_key > 0)
      tmp_key--;
    if (knob > 0)
      tmp_key++;

    if (tmp_key > 2)
      tmp_key = 0;
      
    if (tmp_key == 0)
      printLine1("Hand Key?");
    else if (tmp_key == 1)
      printLine1("Iambic A?");
    else if (tmp_key == 2)  
      printLine1("Iambic B?");  
  }

  activeDelay(500);
  if (tmp_key == 0)
    iambicKey = 0;
  else if (tmp_key == 1) {
    iambicKey = 1;
    keyerControl &= ~IAMBICB;
  }
  else if (tmp_key == 2) {
    iambicKey = 1;
    keyerControl |= IAMBICB;
  }
  
  EEPROM.put(CW_KEY_TYPE, tmp_key);
  */
  
  menuState = 1;  
}

void menuReadADC1(int btn) {
  int adc;
  
  if (!btn) {
    adc = 801; //YL3AME:analogRead(ANALOG_KEYER);
    itoa(adc, b, 10);
    printLine6value("14 ADC", b);
    return;
  }

  menuState = 1;
}

extern void resetSettings();
void menuResetSettings(int btn) {
  if (!btn) {
    printLine6("RESET");
    return;
  }

  resetSettings();
  u8x8.clear();
  printLine(3,"EEPROM RESET");
  printLine(4,"TURN OFF POWER");
  while (1) {};
}

void doMenu() {
  int select = 0, active = -1, btnState;

  btnWaitUp();
  
  menuState = 2;
  
  while (menuState) {
    btnState = btnDown();
    if (btnState)
      btnWaitUp();

    select += enc_read();
    if (extendedMenu && select > 159)
      select = 159;
    if (!extendedMenu && select > 79)
      select = 79;
    if (select < 0)
      select = 0;

    if (menuState == 1) { // request to close menu
      menuState = 0;
      btnState = 0; // draw menu without pressed button one last time
      u8x8.setInverseFont(1);
    }

    if (select / 10 != active) {
      active = select / 10;
    }

    switch (active) {
      case 0: menuBand(btnState); break;
      case 1: menuRitToggle(btnState); break;
      case 2: menuVfoToggle(btnState); break;
      case 3: menuSidebandToggle(btnState); break;
      case 4: menuSplitToggle(btnState); break;
      case 5: menuCWSpeed(btnState); break;
      case 6:
        if (menuSetup(btnState)) select = 70;
        break;
      case 7:
        if (extendedMenu)
          menuSetupCalibration(btnState);
        else
          menuExit(btnState);
        break;
      case 8: menuSetupCarrier(btnState); break;
      case 9: menuSetupCwTone(btnState); break;
      case 10: menuSetupCwDelay(btnState); break;
      case 11: menuCWMenuSpeed(btnState); break;
      case 12: menuSetupKeyer(btnState); break;
      case 13: menuReadADC1(btnState); break;
      case 14: menuResetSettings(btnState); break;
      case 15: menuExit(btnState);  
    }

    if (menuState == 0) { // leaving
      u8x8.setInverseFont(0);
      activeDelay(250);
    }
  }
  u8x8.clear();
  updateDisplay();
  checkCAT();
}

