#include "ubitx_menu.h"

#include <Arduino.h>

#include "hardware.h"

#include "encoder.h"
#include "ubitx.h"
#include "ubitx_main.h"
#include "ubitx_si5351.h"
#include "ubitx_ui.h"

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

// Set to 2 when the menu is being displayed, if a menu item sets it to zero,
// the menu is exited.
char menu_state = 0;

char screen_dirty;  // Used across functions to signal redrawing

// This mode of menus shows extended menus to calibrate the oscillators and
// choose the proper
char extended_menu = 0;

static char NeedRedraw() {
  char ret = screen_dirty;
  screen_dirty = 0;
  return ret;
}

unsigned char wait_knob_right = 0;
void DrawWaitKnobScreen(const char* title, const char *units) {
  u8x8.clear();
  u8x8.draw1x2String(1, 6, title);
  wait_knob_right = 16 - strlen(units);
  u8x8.draw1x2String(wait_knob_right, 4, units);
  wait_knob_right--;
}

// A generic control to read variable values
long int WaitKnobValue(long int minimum, long int maximum, long int step_size,
                       int initial, void (*ValueCallback)(long int),
                       unsigned char silent) {
  int knob = 0;
  long int knob_value = initial;

  screen_dirty = 1;
  while (!BtnDown()) {
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
      if (ValueCallback) ValueCallback(knob_value);
      if (!silent) {
        ltoa(knob_value, b, 10);
        int8_t i = wait_knob_right - strlen(b) * 2;
        if (i > 0) {
          u8x8.setFont(U8X8_DIGITFONT);
          u8x8.drawString(i, 3, b);
          u8x8.setFont(U8X8_MAINFONT);
          while (i-- > 1) {
            u8x8.draw1x2Glyph(i, 3, ' ');
            u8x8.drawGlyph(i, 5, ' ');
          }
        }
      }
    }
    ActiveDelay(20);
  }
  BtnWaitUp();

  return knob_value;
}


//# Menu: 1
static const char* STR_ON = "ON";
static const char* STR_OFF = "OFF";
static const char* STR_WPM = "WPM";
static const char* STR_BAND_SELECT = "BAND SELECT";
static const char* STR_CW_DELAY = "CW DELAY";
static const char* STR_CW_TONE = "CW TONE";
static const char* STR_CW_KEY = "CW KEY";
static const char* STRS_IAMBIC[3] = {"STRIGHT", "IAMBIC-A", "IAMBIC-B"};
static const char* STRS_ADC[4] = {"FBUTTON", "PTT", "KEYER", "VOLTAGE"};
static const int   PINS_ADC[4] = { FBUTTON, PTT, ANALOG_KEYER, ANALOG_V};

void MenuBand(int btn) {
  int knob = 0;

  if (!btn) {
    if (NeedRedraw())
      PrintStatusValue(STR_BAND_SELECT, "..");
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
      PrintStatusValue("RIT", shift_mode == SHIFT_RIT ? STR_ON : STR_OFF);
    }
    return;
  }

  if (shift_mode == SHIFT_RIT)
    RitDisable();
  else
    RitEnable(frequency);

  menu_state = 1;
}


// Menu #3
void MenuVfoToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue("VFO", vfo_active == VFO_ACTIVE_A ? "A" : "B");
    }
    return;
  }

  VfoSwap(/* save=*/1);

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

  if (is_usb)
    SidebandSet(0);
  else
    SidebandSet(1);

  menu_state = 1;
}

// Split communication using VFOA and VFOB by KD8CEC
// Menu #5
void MenuSplitToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue("SPLIT", shift_mode == SHIFT_SPLIT ? STR_ON : STR_OFF);
    }
    return;
  }

  if (shift_mode == SHIFT_SPLIT)
    SplitDisable();
  else
    SplitEnable();

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

  DrawWaitKnobScreen("CW SPEED", STR_WPM);
  wpm = WaitKnobValue(1, 100, 1,  wpm, 0, 0);

  CwSpeedSet(1200 / wpm);

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
  u8x8.draw1x2String(1, 4, "NOT IMPLEMENTED");
  ActiveDelay(2000);
  menu_state = 1;
  // calibrateClock();
}

void PreviewCarrier(long int adjust) {
  unsigned long long carrier = 11000000 + adjust;
  si5351bx_setfreq(0, carrier);
  ultoa(carrier, c, DEC);
  PrintLine(4, c);
}

void MenuSetupCarrier(int btn) {
  unsigned long long carrier;
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatus("CAL BFO");
    }
    return;
  }

  DrawWaitKnobScreen("CALIBRATE BFO", "");
  // Values from 11000000 to 11099999
  carrier = 11000000 + WaitKnobValue(0, 99999, 1, usb_carrier - 11000000,
                                     PreviewCarrier, 1);
  SetUsbCarrier(carrier);
  menu_state = 1;
}

void PreviewSidetone(long int value) {
  tone(CW_TONE, value);
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

  DrawWaitKnobScreen(STR_CW_TONE, "HZ");
  unsigned int tone = WaitKnobValue(100, 2000, 10, cw_side_tone,
                                    PreviewSidetone, 0);
  noTone(CW_TONE);
  CwToneSet(tone);

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

  DrawWaitKnobScreen(STR_CW_DELAY, "MS");
  // TODO MOVE to ubitx.cpp
  cw_delay_time = WaitKnobValue(10, 1010, 50, cw_delay_time, 0, 0);

  menu_state = 1;
}

void PreviewKeyer(long int value) {
  u8x8.draw1x2String(4, 3, STRS_IAMBIC[value]);
  for (unsigned char i = strlen(STRS_IAMBIC[value]) + 4; i <= 15; i++) {
    u8x8.draw1x2Glyph(i, 3, ' ');
  }
}

void MenuSetupKeyer(int btn) {
  if (!btn) {
    if (NeedRedraw())
      PrintStatusValue(STR_CW_KEY, STRS_IAMBIC[iambic_key]);
    return;
  }

  DrawWaitKnobScreen(STR_CW_KEY, "");

  IambicKeySet(WaitKnobValue(0, 2, 1, iambic_key, PreviewKeyer, 1));

  menu_state = 1;
}

void MenuTxToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      PrintStatusValue("TX INHIBIT", tx_inhibit ? STR_ON : STR_OFF);
    }
    return;
  }

  if (tx_inhibit)
    tx_inhibit = 0;
  else
    tx_inhibit = 1;

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

void MenuResetSettings(int btn) {
  if (!btn) {
    if (NeedRedraw())
      PrintStatus("RESET");
    return;
  }

  ResetSettingsAndHalt();
}


int select, active;

void MenuTaskLoop() {
  if (menu_state == 0) { // exiting
    master.setCallback(&MasterTask);
    u8x8.clear();
    UpdateDisplay();
  }

  int btnState = BtnDown();
  if (btnState)
    BtnWaitUp();

  select += EncRead();
  if (extended_menu && select > 159)
    select = 159;
  if (!extended_menu && select > 79)
    select = 79;
  if (select < 0)
    select = 0;

  if (menu_state == 1) {  // request to close menu
    menu_state = 0;
    btnState = 0;  // draw menu without pressed button one last time
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
    case 12: MenuTxToggle(btnState); break;
    case 13: MenuReadADC1(btnState); break;
    case 14: MenuResetSettings(btnState); break;
    case 15: MenuExit(btnState);
  }

  if (menu_state == 0) {  // leaving
    u8x8.setInverseFont(0);
    ActiveDelay(300);
  }
}

void MenuTaskStart() {
  BtnWaitUp();

  menu_state = 2;
  select = 0;
  active = -1;
  master.setCallback(&MenuTaskLoop);
}
