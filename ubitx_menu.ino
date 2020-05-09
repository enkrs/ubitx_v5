/** Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls DoMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */

uint8_t screen_dirty; // Used across functions to signal redrawing
uint8_t extended_menu = 0;     //this mode of menus shows extended menus to calibrate the oscillators and choose the proper


/** A generic control to read variable values
*/
int GetValueByKnob(int minimum, int maximum, int step_size,  int initial, const char* title, const char *postfix)
{
  int knob = 0;
  int knob_value;
  screen_dirty = 1;

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
   
  while(!BtnDown()) {
    knob = EncRead();
    if (knob != 0) {
      if (knob < 0) knob_value -= step_size;
      if (knob > 0) knob_value += step_size;
      knob_value = (knob_value - minimum) / step_size * step_size + minimum;
      if (knob_value < minimum) knob_value = minimum;
      if (knob_value > maximum) knob_value = maximum;
      screen_dirty = 1;
    }
    if (screen_dirty) {
      itoa(knob_value, b, 10);
      int8_t i = right - strlen(b) * 2;
      if (i > 0) {
        u8x8.draw2x2String(i, 4, b);
        while (i-- > 1) u8x8.draw1x2Glyph(i, 4, ' ');
      }
      screen_dirty = 0;
    }
    CheckCat();
  }
  BtnWaitUp();
  u8x8.clear();

  return knob_value;
}

//# Menu: 1
static const char* STR_ON = "ON";
static const char* STR_OFF = "OFF";
static const char* STR_WPM = "WPM";
static const char* STR_BAND_SELECT = "BAND SELECT";
static const char* STR_CW_DELAY = "CW DELAY";

void MenuBand(int btn) {
  int knob = 0;

  if (!btn) {
    PrintStatusValue(STR_BAND_SELECT,"..");
    return;
  }

  PrintStatus(STR_BAND_SELECT);
  RitDisable();

  while (!BtnDown()) {
    knob = EncRead();
    if (knob != 0) {
      if (knob < 0 && frequency - 200000l > LOWEST_FREQ)
        SetFrequency(frequency - 200000l);
      if (knob > 0 && frequency + 200000l < HIGHEST_FREQ)
        SetFrequency(frequency + 200000l);
      is_usb = frequency > 10000000l ? 1 : 0;
      UpdateDisplay();
    }
    ActiveDelay(20);
  }
  BtnWaitUp();

  menu_state = 1;
}

// Menu #2
void MenuRitToggle(int btn) {
  if (!btn) {
    PrintStatusValue("RIT", rit_on ? STR_ON : STR_OFF);
    return;
  }

  if (rit_on == 0)
    ritEnable(frequency);
  else
    RitDisable();

  menu_state = 1;
}


//Menu #3
void MenuVfoToggle(int btn) {
  if (!btn) {
    PrintStatusValue("VFO", vfo_active == VFO_A ? "A" : "B");
    return;
  }

  if (vfo_active == VFO_B) {
    if (vfo_b != frequency) {
      vfo_b = frequency;
      EEPROM.put(VFO_B, vfo_b);
    }
    if (vfo_b_usb != is_usb) {
      vfo_b_usb = is_usb;
      EEPROM.put(VFO_B_USB, vfo_b_usb);
    }

    vfo_active = VFO_A;
    frequency = vfo_a;
    is_usb = vfo_a_usb;
  } else {
    if (vfo_a != frequency) {
      vfo_a = frequency;
      EEPROM.put(VFO_A, vfo_a);
    }
    if (vfo_a_usb != is_usb) {
      vfo_a_usb = is_usb;
      EEPROM.put(VFO_A_USB, vfo_a_usb);
    }

    vfo_active = VFO_B;
    frequency = vfo_b;
    is_usb = vfo_b_usb;
  }
  RitDisable();
  SetFrequency(frequency);

  menu_state = 1;
}

// Menu #4
void MenuSidebandToggle(int btn) {
  if (!btn) {
    PrintStatusValue("MODE", is_usb ? "USB" : "LSB");
    return;
  }

  if (is_usb == 1)
    is_usb = 0;
  else
    is_usb = 1;

  //Added by KD8CEC
  if (vfo_active == VFO_B)
    vfo_b_usb = is_usb;
  else
    vfo_b_usb = is_usb;

  menu_state = 1;
}

//Split communication using VFOA and VFOB by KD8CEC
//Menu #5
void MenuSplitToggle(int btn) {
  if (!btn) {
    PrintStatusValue("SPLIT", split_on ? STR_ON : STR_OFF);
    return;
  }

  if (split_on == 1) {
    split_on = 0;
  } else {
    split_on = 1;
    if (rit_on == 1) rit_on = 0;
  }
  menu_state = 1;
}

void MenuCwSpeed(int btn) {
   int wpm;

   wpm = 1200 / cw_speed;
     
   if (!btn) {
     itoa(wpm, b, 10);
     strcat(b, " ");
     strcat(b, STR_WPM);
     PrintStatusValue("CW", b);
     return;
   }

   wpm = GetValueByKnob(1, 100, 1,  wpm, "CW SPEED", STR_WPM);
   cw_speed = 1200 / wpm;
   EEPROM.put(CW_SPEED, cw_speed);

   menu_state = 1;
}

void MenuExit(int btn) {
  if (!btn) {
    PrintStatus("EXIT MENU");
    return;
  }

  menu_state = 0;
}

/**
 * The calibration routines are not normally shown in the menu as they are rarely used
 * They can be enabled by choosing this menu option
 */
int MenuSetup(int btn) {
  if (!btn) {
    PrintStatusValue("ADVANCED", extended_menu ? STR_ON : "..");
    return 0;
  }

  if (!extended_menu) {
    extended_menu = 1;
    return 1;
  }
  extended_menu = 0;
  menu_state = 1;
  return 0;
}

 //this is used by the si5351 routines in the ubitx_5351 file
/*
extern int32_t master_cal;
extern uint32_t si5351bx_vcoa;

void calibrateClock() {
  int knob = 0;

  digitalWrite(TX_LPF_A, 0);
  digitalWrite(TX_LPF_B, 0);
  digitalWrite(TX_LPF_C, 0);

  master_cal = 0;

  is_usb = 1;

  //turn off the second local oscillator and the bfo
  si5351_set_calibration(master_cal);
  StartTx(TX_CW);
  si5351bx_setfreq(2, 10000000l); 
  
  strcpy(b, "#1 10 MHz cal:");
  ltoa(master_cal/8750, c, 10);
  strcat(b, c);
  PrintStatus(b);     

  while (!BtnDown()) {
    if (digitalRead(PTT) == LOW && !key_down)
      CwKeydown();
    if (digitalRead(PTT)  == HIGH && key_down)
      cwKeyUp();
      
    knob = EncRead();

    if (knob > 0)
      master_cal += 875;
    else if (knob < 0)
      master_cal -= 875;
    else 
      continue; //don't update the frequency or the display
      
    si5351_set_calibration(master_cal);
    si5351bx_setfreq(2, 10000000l);
    //strcpy(b, "#1 10 MHz cal:");
    ltoa(master_cal/8750, c, 10);
    strcat(b, c);
    PrintStatus(b);     
  }
  BtnWaitUp();

  cw_timeout = 0;
  key_down = 0;
  StopTx();

  EEPROM.put(MASTER_CAL, master_cal);
  initOscillators();
  SetFrequency(frequency);    
}
*/

void MenuSetupCalibration(int btn) {
  if (!btn) {
    PrintStatus("CALIBRATE");
    return;
  }

  u8x8.clear();
  u8x8.draw1x2String(1,4,"NOT IMPLEMENTED");
  ActiveDelay(2000);
  //calibrateClock();
}

void MenuSetupCarrier(int btn) {
  int knob = 0;
   
  if (!btn) {
    PrintStatus("CAL BFO");
    return;
  }

  // usb_carrier = 11053000l;
  si5351bx_setfreq(0, usb_carrier);
  u8x8.clear();
  u8x8.draw1x2String(1,1,"CALIBRATE BFO");
  screen_dirty = 1;

  while (!BtnDown()) {
    knob = EncRead();

    if (knob != 0) {
      usb_carrier += knob;
      if (usb_carrier < 11000000) usb_carrier = 11000000;
      if (usb_carrier > 11099999) usb_carrier = 11099999;
      
      si5351bx_setfreq(0, usb_carrier);
      SetFrequency(frequency); // This was not in original. Why?
      screen_dirty = 1;
    }
    if (screen_dirty) {
      ultoa(usb_carrier, c, DEC);
      PrintLine(4, c);
    }
    CheckCat();
  }
  BtnWaitUp();
  u8x8.clear();

  EEPROM.put(USB_CARRIER, usb_carrier);
  
  si5351bx_setfreq(0, usb_carrier);          
  SetFrequency(frequency);    

  menu_state = 1; 
}

void MenuSetupCwTone(int btn) {
  int knob = 0;
     
  if (!btn) {
    itoa(cw_side_tone, b, 10);
    strcat(b, " HZ");
    PrintStatusValue("CW TONE", b);
    return;
  }

  tone(CW_TONE, cw_side_tone);
  while (!BtnDown()) {
    knob = EncRead();

    if (knob > 0 && cw_side_tone < 2000)
      cw_side_tone += 10;
    else if (knob < 0 && cw_side_tone > 100 )
      cw_side_tone -= 10;
    else
      continue; //don't update the frequency or the display
        
    tone(CW_TONE, cw_side_tone);
    itoa(cw_side_tone, b, 10);
    PrintStatus(b);

    ActiveDelay(20);
  }
  BtnWaitUp();
  noTone(CW_TONE);
  //save the setting
  EEPROM.put(CW_SIDE_TONE, cw_side_tone);
    
  menu_state = 1; 
}

void MenuSetupCwDelay(int btn) {
  if (!btn) {
    itoa(cw_delay_time, b, 10);
    strcat(b, " MS");
    PrintStatusValue(STR_CW_DELAY, b);
    return;
  }

  cw_delay_time = GetValueByKnob(10, 1010, 50,  cw_delay_time, STR_CW_DELAY, "MS");

  menu_state = 1;
}

void menuSetupKeyer(int btn) {
  const char* title = "CW TYPE";
  if (!btn) {
    switch(iambic_key) {
      case 0:
        PrintStatusValue(title, "STR");
      break;
      case 1:
        PrintStatusValue(title, "IAMB-A");
      break;
      case 2:
        PrintStatusValue(title, "IAMB-B");
      break;
    }
    return;
  }
  
  ActiveDelay(500);

  /* TODO: implement
  if (!iambic_key)
    tmp_key = 0; //hand key
  else if (keyer_control & IAMBICB)
    tmp_key = 2; //Iambic B
  else 
    tmp_key = 1;
 
  while (!BtnDown())
  {
    knob = EncRead();
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

  ActiveDelay(500);
  if (tmp_key == 0)
    iambic_key = 0;
  else if (tmp_key == 1) {
    iambic_key = 1;
    keyer_control &= ~IAMBICB;
  }
  else if (tmp_key == 2) {
    iambic_key = 1;
    keyer_control |= IAMBICB;
  }
  
  EEPROM.put(CW_KEY_TYPE, tmp_key);
  */
  
  menu_state = 1;  
}

void menuReadADC1(int btn) {
  int adc;
  
  if (!btn) {
    adc = 801; //YL3AME:analogRead(ANALOG_KEYER);
    itoa(adc, b, 10);
    PrintStatusValue("14 ADC", b);
    return;
  }

  menu_state = 1;
}

extern void ResetSettings();
void menuResetSettings(int btn) {
  if (!btn) {
    PrintStatus("RESET");
    return;
  }

  ResetSettings();
  u8x8.clear();
  PrintLine(3,"EEPROM RESET");
  PrintLine(4,"TURN OFF POWER");
  while (1) {};
}

void DoMenu() {
  int select = 0, active = -1, btnState;

  BtnWaitUp();
  
  menu_state = 2;
  
  while (menu_state) {
    btnState = BtnDown();
    if (btnState)
      BtnWaitUp();

    select += EncRead();
    if (extended_menu && select > 149)
      select = 149;
    if (!extended_menu && select > 79)
      select = 79;
    if (select < 0)
      select = 0;

    if (menu_state == 1) { // request to close menu
      menu_state = 0;
      btnState = 0; // draw menu without pressed button one last time
      u8x8.setInverseFont(1);
    }

    if (select / 10 != active) {
      active = select / 10;
    }

    switch (active) {
      case 0: MenuBand(btnState); break;
      case 1: MenuRitToggle(btnState); break;
      case 2: MenuVfoToggle(btnState); break;
      case 3: MenuSidebandToggle(btnState); break;
      case 4: MenuSplitToggle(btnState); break;
      case 5: MenuCwSpeed(btnState); break;
      case 6:
        if (MenuSetup(btnState)) select = 70;
        break;
      case 7:
        if (extended_menu)
          MenuSetupCalibration(btnState);
        else
          MenuExit(btnState);
        break;
      case 8: MenuSetupCarrier(btnState); break;
      case 9: MenuSetupCwTone(btnState); break;
      case 10: MenuSetupCwDelay(btnState); break;
      case 11: menuSetupKeyer(btnState); break;
      case 12: menuReadADC1(btnState); break;
      case 13: menuResetSettings(btnState); break;
      case 14: MenuExit(btnState);  
    }

    if (menu_state == 0) { // leaving
      u8x8.setInverseFont(0);
      ActiveDelay(250);
    }
  }
  BtnWaitUp();
  u8x8.clear();
  UpdateDisplay();
}

