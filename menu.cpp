#include "menu.h"
#include <Arduino.h>
#include "encoder.h"
#include "hw.h"
#include "mainloop.h"
#include "si5351.h"
#include "ubitx.h"
#include "ui.h"

namespace menu {

char b[12]; // holds strings up to ltoa "-2147483647\0"
            // also itoa with prefix    "-32767 HZ..\0"

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
  ui::u8x8.clear();
  ui::u8x8.draw1x2String(1, 6, title);
  wait_knob_right = 16 - strlen(units);
  ui::u8x8.draw1x2String(wait_knob_right, 4, units);
  wait_knob_right--;
}

// A generic control to read variable values
long int WaitKnobValue(long int minimum, long int maximum, long int step_size,
                       int initial, void (*ValueCallback)(long int),
                       unsigned char silent) {
  int knob = 0;
  long int knob_value = initial;

  screen_dirty = 1;
  while (!mainloop::FButtonClicked()) {
    knob = encoder::ReadSlow();
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
          ui::u8x8.setFont(U8X8_DIGITFONT);
          ui::u8x8.drawString(i, 3, b);
          ui::u8x8.setFont(U8X8_MAINFONT);
          while (i-- > 1) {
            ui::u8x8.draw1x2Glyph(i, 3, ' ');
            ui::u8x8.drawGlyph(i, 5, ' ');
          }
        }
      }
    }
  }

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
static const int   PINS_ADC[4] = { hw::FBUTTON, hw::PTT, hw::ANALOG_KEYER, hw::ANALOG_V};

void MenuBand(int btn) {
  int knob = 0;

  if (!btn) {
    if (NeedRedraw())
      ui::PrintStatusValue(STR_BAND_SELECT, ">");
    return;
  }

  ui::PrintStatus(STR_BAND_SELECT);
  ubitx::RitDisable();

  while (!mainloop::FButtonClicked()) {
    knob = encoder::ReadSlow();
    if (knob != 0) {
      if (knob < 0 && ubitx::frequency - 100000l > ubitx::LOWEST_FREQ)
        ubitx::SetFrequency(ubitx::frequency - 100000l);
      if (knob > 0 && ubitx::frequency + 100000l < ubitx::HIGHEST_FREQ)
        ubitx::SetFrequency(ubitx::frequency + 100000l);
      ubitx::status.is_usb = ubitx::frequency > 10000000l ? true : false; // TODO call SidebandSet
      ui::UpdateDisplay();
    }
  }

  menu_state = 1;
}

// Menu #2
void MenuRitToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatusValue("RIT", ubitx::status.shift_mode == ubitx::SHIFT_RIT ? STR_ON : STR_OFF);
    }
    return;
  }

  if (ubitx::status.shift_mode == ubitx::SHIFT_RIT)
    ubitx::RitDisable();
  else
    ubitx::RitEnable(ubitx::frequency);

  menu_state = 1;
}


// Menu #3
void MenuVfoToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatusValue("VFO", ubitx::status.vfo_a_active ? "A" : "B");
    }
    return;
  }

  ubitx::VfoSwap(/* save=*/1);

  menu_state = 1;
}

// Menu #4
void MenuSidebandToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatusValue("MODE", ubitx::status.is_usb ? "USB" : "LSB");
    }
    return;
  }

  ubitx::SidebandSet(!ubitx::status.is_usb);

  menu_state = 1;
}

// Split communication using VFOA and VFOB by KD8CEC
// Menu #5
void MenuSplitToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatusValue("SPLIT", ubitx::status.shift_mode == ubitx::SHIFT_SPLIT ? STR_ON : STR_OFF);
    }
    return;
  }

  if (ubitx::status.shift_mode == ubitx::SHIFT_SPLIT)
    ubitx::SplitDisable();
  else
    ubitx::SplitEnable();

  menu_state = 1;
}

void MenuCwSpeed(int btn) {
  int wpm;

  wpm = 1200 / ubitx::settings.cw_speed;

  if (!btn) {
    if (NeedRedraw()) {
      itoa(wpm, b, 10);
      strcat(b, " ");
      strcat(b, STR_WPM);
      ui::PrintStatusValue("CW", b);
    }
    return;
  }

  DrawWaitKnobScreen("CW SPEED", STR_WPM);
  wpm = WaitKnobValue(1, 100, 1,  wpm, 0, 0);

  ubitx::CwSpeedSet(1200 / wpm);

  menu_state = 1;
}

void MenuExit(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatus("EXIT MENU");
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
      ui::PrintStatusValue("ADVANCED", extended_menu ? STR_ON : ">");
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
      ui::PrintStatus("CALIBRATE");
    }
    return;
  }

  ui::u8x8.clear();
  ui::u8x8.draw1x2String(1, 4, "NOT IMPLEMENTED");
  ubitx::ActiveDelay(2000);
  menu_state = 1;
  // calibrateClock();
}

void PreviewCarrier(long int adjust) {
  unsigned long long carrier = 11000000 + adjust;
  si5351::SetFreq(0, carrier);
  ultoa(carrier, b, DEC);
  ui::PrintLine(4, b);
}

void MenuSetupCarrier(int btn) {
  unsigned long long carrier;
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatus("CAL BFO");
    }
    return;
  }

  DrawWaitKnobScreen("CALIBRATE BFO", "");
  // Values from 11000000 to 11099999
  carrier = 11000000 + WaitKnobValue(0, 99999, 1,
                                     ubitx::settings.usb_carrier - 11000000,
                                     PreviewCarrier, 1);
  ubitx::SetUsbCarrier(carrier);
  menu_state = 1;
}

void PreviewSidetone(long int value) {
  tone(hw::CW_TONE, value);
}

void MenuSetupCwTone(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      itoa(ubitx::settings.cw_side_tone, b, 10);
      strcat(b, " HZ");
      ui::PrintStatusValue(STR_CW_TONE, b);
    }
    return;
  }

  DrawWaitKnobScreen(STR_CW_TONE, "HZ");
  unsigned int tone = WaitKnobValue(100, 2000, 10, ubitx::settings.cw_side_tone,
                                    PreviewSidetone, 0);
  noTone(hw::CW_TONE);
  ubitx::CwToneSet(tone);

  menu_state = 1;
}

void MenuSetupCwDelay(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      itoa(ubitx::cw_delay_time, b, 10);
      strcat(b, " MS");
      ui::PrintStatusValue(STR_CW_DELAY, b);
    }
    return;
  }

  DrawWaitKnobScreen(STR_CW_DELAY, "MS");
  // TODO MOVE to settings
  ubitx::cw_delay_time = WaitKnobValue(10, 1010, 50, ubitx::cw_delay_time,
                                       0, 0);

  menu_state = 1;
}

void PreviewKeyer(long int value) {
  ui::u8x8.draw1x2String(4, 3, STRS_IAMBIC[value]);
  for (unsigned char i = strlen(STRS_IAMBIC[value]) + 4; i <= 15; i++) {
    ui::u8x8.draw1x2Glyph(i, 3, ' ');
  }
}

void MenuSetupKeyer(int btn) {
  if (!btn) {
    if (NeedRedraw())
      ui::PrintStatusValue(STR_CW_KEY, STRS_IAMBIC[ubitx::settings.iambic_key]);
    return;
  }

  DrawWaitKnobScreen(STR_CW_KEY, "");

  ubitx::IambicKeySet(WaitKnobValue(0, 2, 1, ubitx::settings.iambic_key,
                                    PreviewKeyer, 1));

  menu_state = 1;
}

void MenuTxToggle(int btn) {
  if (!btn) {
    if (NeedRedraw()) {
      ui::PrintStatusValue("TX INHIBIT", ubitx::status.tx_inhibit ? STR_ON : STR_OFF);
    }
    return;
  }

  ubitx::status.tx_inhibit = !ubitx::status.tx_inhibit;

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
    ui::PrintStatusValue(STRS_ADC[selected], b);
  }
}

void MenuResetSettings(int btn) {
  if (!btn) {
    if (NeedRedraw())
      ui::PrintStatus("RESET");
    return;
  }

  ubitx::ResetSettingsAndHalt();
}


int select, active;

void DoMenu() {
  if (menu_state == 3) { // first entry
    menu_state = 2;
    select = 0;
    active = -1;
    screen_dirty = 1;
  }

  int btnState = mainloop::FButtonClicked();

  select += encoder::ReadSlow();
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
    ui::u8x8.setInverseFont(1);
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
    ui::u8x8.setInverseFont(0);
    ubitx::ActiveDelay(300);
    ui::u8x8.clear();
    ui::UpdateDisplay();
  }
}

}  // namespace
