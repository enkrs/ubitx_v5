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

char screen_dirty; // Used across functions to signal redrawing
char extended_menu = 0;     //this mode of menus shows extended menus to calibrate the oscillators and choose the proper

signed char knob_value_draw_right;

static char NeedRedraw() {
  char ret = screen_dirty;
  screen_dirty = 0;
  return ret;
}

long int WaitKnobValueCustom(long int minimum, long int maximum, int step_size,
                             long int initial, void (*DrawCallback)(long int)) {
  int knob = 0;
  long int knob_value = initial;

  screen_dirty = 1;
   
  while(!BtnDown()) {
    knob = EncReadSlow();
    if (knob != 0) {
      if (knob < 0) knob_value -= step_size;
      if (knob > 0) knob_value += step_size;
      knob_value = (knob_value - minimum) / step_size * step_size + minimum;
      if (knob_value < minimum) knob_value = minimum;
      if (knob_value > maximum) knob_value = maximum;
      screen_dirty = 1;
    }
    if (NeedRedraw()) {
      DrawCallback(knob_value);
    }
    ActiveDelay(20);
  }
  BtnWaitUp();

  return knob_value;
}

void KnobValueDraw(long int value) {
  ltoa(value, b, 10);
  int8_t i = knob_value_draw_right - strlen(b) * 2;
  if (i > 0) {
    u8x8.draw2x2String(i, 4, b);
    while (i-- > 1) u8x8.draw1x2Glyph(i, 4, ' ');
  }
}


// A generic control to read variable values
long int WaitKnobValue(long int minimum, long int maximum, long int step_size,
                  int initial, const char* title, const char *postfix) {
  u8x8.clear();
  u8x8.draw1x2String(1, 1, title);
  knob_value_draw_right = 16 - strlen(postfix) * 2;
  u8x8.draw2x2String(knob_value_draw_right, 4, postfix);

  return WaitKnobValueCustom(minimum, maximum, step_size, initial,
                             KnobValueDraw);
}


//# Menu: 1
static const char* STR_ON = "ON";
static const char* STR_OFF = "OFF";
static const char* STR_WPM = "WPM";
static const char* STR_BAND_SELECT = "BAND SELECT";
static const char* STR_CW_DELAY = "CW DELAY";
static const char* STR_CW_TONE = "CW TONE";
static const char* STR_CW_KEY = "CW KEY";
static const char* STRS_IAMBIC[3] = {"STRIGHT","IAMBIC-A","IAMBIC-B"};
static const char* STRS_ADC[4] = {"FBUTTON", "PTT", "KEYER", "A7"};
static const int   PINS_ADC[4] = { FBUTTON, PTT, ANALOG_KEYER, ANALOG_SPARE};

void MenuBand(int btn) {
  int knob = 0;

  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue(STR_BAND_SELECT,"..");
    }
    return;
  }

  PrintStatus(STR_BAND_SELECT);
  RitDisable();

  while (!BtnDown()) {
    knob = EncReadSlow();
    if (knob != 0) {
      if (knob < 0 && frequency - 100000l > LOWEST_FREQ)
        SetFrequency(frequency - 100000l);
      if (knob > 0 && frequency + 100000l < HIGHEST_FREQ)
        SetFrequency(frequency + 100000l);
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
    if (NeedRedraw()) {
      PrintStatusValue("RIT", rit_on ? STR_ON : STR_OFF);
    }
    return;
  }

  if (!rit_on)
    RitEnable(frequency);
  else
    RitDisable();

  menu_state = 1;
}


//Menu #3
void MenuVfoToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue("VFO", vfo_active == VFO_A ? "A" : "B");
    }
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
    if (NeedRedraw()) {
      PrintStatusValue("MODE", is_usb ? "USB" : "LSB");
    }
    return;
  }

  is_usb = !is_usb;

  if (vfo_active == VFO_B && vfo_b_usb != is_usb) {
    vfo_b_usb = is_usb;
    EEPROM.put(VFO_B_USB, vfo_b_usb);
  }
  if (vfo_active == VFO_A && vfo_a_usb != is_usb) {
    vfo_a_usb = is_usb;
    EEPROM.put(VFO_A_USB, vfo_a_usb);
  }
  SetFrequency(frequency);

  menu_state = 1;
}

//Split communication using VFOA and VFOB by KD8CEC
//Menu #5
void MenuSplitToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue("SPLIT", split_on ? STR_ON : STR_OFF);
    }
    return;
  }

  split_on = !split_on;
  if (split_on) rit_on = 0;

  menu_state = 1;
}

void MenuCwSpeed(int btn) {
  int wpm;

  wpm = 1200 / cw_speed;
     
  if (!btn) {
    if (NeedRedraw()) {
      itoa(wpm, b, 10);
      strcat(b, " ");
      strcat(b, STR_WPM);
      PrintStatusValue("CW", b);
    }
    return;
  }

  wpm = WaitKnobValue(1, 100, 1,  wpm, "CW SPEED", STR_WPM);
  cw_speed = 1200 / wpm;
  EEPROM.put(CW_SPEED, cw_speed);

  menu_state = 1;
}

void MenuExit(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatus("EXIT MENU");
    }
    return;
  }

  menu_state = 0;
}

/**
 * The calibration routines are not normally shown in the menu as they are
 * rarely used. They can be enabled by choosing this menu option.
 */
int MenuSetup(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue("ADVANCED", extended_menu ? STR_ON : "..");
    }
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

void MenuSetupCalibration(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatus("CALIBRATE");
    }
    return;
  }

  u8x8.clear();
  u8x8.draw1x2String(1,4,"NOT IMPLEMENTED");
  ActiveDelay(2000);
  menu_state = 1;
  //calibrateClock();
}

void MenuSetupCarrier(int btn) {
  int knob = 0;
   
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatus("CAL BFO");
    }
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
    if (NeedRedraw()) {
      ultoa(usb_carrier, c, DEC);
      PrintLine(4, c);
    }
    ActiveDelay(20);
  }
  BtnWaitUp();
  u8x8.clear();

  EEPROM.put(USB_CARRIER, usb_carrier);
  
  si5351bx_setfreq(0, usb_carrier);          
  SetFrequency(frequency);    

  menu_state = 1; 
}

void MenuSetupCwToneCallback(long int value) {
  cw_side_tone = (int)value;
  tone(CW_TONE, cw_side_tone);

  ltoa(value, b, 10); // TODO Deduplicate
  int8_t i = knob_value_draw_right - strlen(b) * 2;
  if (i > 0) {
    u8x8.draw2x2String(i, 4, b);
    while (i-- > 1) u8x8.draw1x2Glyph(i, 4, ' ');
  }
}

void MenuSetupCwTone(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      itoa(cw_side_tone, b, 10);
      strcat(b, " HZ");
      PrintStatusValue(STR_CW_TONE, b);
    }
    return;
  }

  u8x8.clear();
  u8x8.draw1x2String(1, 1, STR_CW_TONE);
  knob_value_draw_right = 12;
  u8x8.draw2x2String(knob_value_draw_right, 4, "HZ");

  cw_side_tone = WaitKnobValueCustom(100, 2000, 10, cw_side_tone,
                                     MenuSetupCwToneCallback);
  noTone(CW_TONE);
  //save the setting
  EEPROM.put(CW_SIDE_TONE, cw_side_tone);
    
  menu_state = 1; 
}

void MenuSetupCwDelay(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      itoa(cw_delay_time, b, 10);
      strcat(b, " MS");
      PrintStatusValue(STR_CW_DELAY, b);
    }
    return;
  }

  cw_delay_time = WaitKnobValue(10, 1010, 50,  cw_delay_time, STR_CW_DELAY, "MS");

  menu_state = 1;
}

void MenuSetupKeyerCallback(long int value) {
  u8x8.draw1x2String(4, 4, STRS_IAMBIC[value]);
  for (unsigned char i = strlen(STRS_IAMBIC[value]) + 4; i <= 15; i++) {
    u8x8.draw1x2Glyph(i, 4, ' ');
  }
}

void MenuSetupKeyer(int btn) {
  if (!btn) {
    if (NeedRedraw())
      PrintStatusValue(STR_CW_KEY, STRS_IAMBIC[iambic_key]);
    return;
  }

  u8x8.clear();
  u8x8.draw1x2String(1, 1, STR_CW_KEY);

  iambic_key = WaitKnobValueCustom(0, 2, 1, iambic_key,
                                   MenuSetupKeyerCallback);
  
  if (iambic_key == 1)
    keyer_control &= ~IAMBICB;
  if (iambic_key == 2)
    keyer_control |= IAMBICB;
  
  EEPROM.put(IAMBIC_KEY, iambic_key);
  
  menu_state = 1;  
}

void MenuReadADC1(int btn) {
  static unsigned char selected = 0;
  static int last_adc;
  int adc;
  
  if (btn) {
    selected = (selected + 1) % 4;
    screen_dirty = 1;
  }
  adc = analogRead(PINS_ADC[selected]);
  if (adc != last_adc || NeedRedraw()) {
    last_adc = adc;
    itoa(adc, b, 10);
    PrintStatusValue(STRS_ADC[selected], b);
  }
  ActiveDelay(100);
}

extern void ResetSettings();
void MenuResetSettings(int btn) {
  if (!btn) {
    if (NeedRedraw())
      PrintStatus("RESET");
    return;
  }

  ResetSettings();
  u8x8.clear();
  PrintLine(2,"EEPROM RESET");
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
      screen_dirty = 1;
      u8x8.setInverseFont(1);
    }

    if (select / 10 != active) {
      active = select / 10;
      screen_dirty = 1;
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
      case 11: MenuSetupKeyer(btnState); break;
      case 12: MenuReadADC1(btnState); break;
      case 13: MenuResetSettings(btnState); break;
      case 14: MenuExit(btnState);  
    }

    if (menu_state == 0) { // leaving
      u8x8.setInverseFont(0);
      ActiveDelay(300);
    }
  }
  u8x8.clear();
  UpdateDisplay();
}
