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

enum DO_MENU_STATES {
  STATE_INITIAL,
  STATE_DRAW_SELECTED,
  STATE_SELECTING_MENU,
  STATE_MENU_ITEM_ACTIVE,
  STATE_OPEN_ADVANCED,
  STATE_EXIT,
  STATE_WAIT_VALUE,
  STATE_VALUE_RETURNED
};

enum MENU_HANDLER_STATES {
  EVENT_SELECTED,
  EVENT_ACTIVE,
  EVENT_VALUE,
};

// Is the advanced menu visible?
bool advanced_menu = false;

struct Value {
  int min;
  int max;
  int step;
  int current;
  void (*PreviewCallback)();
} value;

void SetWaitValues(int min, int max, int step, int current, void (*PreviewCallback)()) {
  value.min = min;
  value.max = max;
  value.step = step;
  value.current = current;
  value.PreviewCallback = PreviewCallback;
  value.PreviewCallback();
}

unsigned char wait_knob_right = 0;

void DrawWaitKnobScreen(const char* title, const char *units) {
  ui::u8x8.clear();
  ui::u8x8.draw1x2String(1, 6, title);
  wait_knob_right = 16 - strlen(units);
  ui::u8x8.draw1x2String(wait_knob_right, 4, units);
  wait_knob_right--;
}

void PreviewCurrentValue() {
  ltoa(value.current, b, 10);
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

void PreviewBand() {
  ubitx::SetFrequency((ubitx::frequency % 100000l) + (value.current * 100000l));
  ui::UpdateDisplay();
}

unsigned char MenuBand(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue(STR_BAND_SELECT, ">");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ui::PrintStatus(STR_BAND_SELECT);
      ubitx::RitDisable();

      SetWaitValues(ubitx::LOWEST_FREQ / 100000l,
                    ubitx::HIGHEST_FREQ / 100000l,
                    1,
                    ubitx::frequency / 100000l,
                    PreviewBand);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      return STATE_EXIT;
  }
  return false;
}

unsigned char MenuRitToggle(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("RIT", ubitx::status.shift_mode == ubitx::SHIFT_RIT ? STR_ON : STR_OFF);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      if (ubitx::status.shift_mode == ubitx::SHIFT_RIT)
        ubitx::RitDisable();
      else
        ubitx::RitEnable(ubitx::frequency);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuVfoToggle(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("VFO", ubitx::status.vfo_a_active ? "A" : "B");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ubitx::VfoSwap(/* save=*/true);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuVfoCopy(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("VFO", "A=B");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ubitx::VfoCopy(/* save=*/true);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuSidebandToggle(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("MODE", ubitx::status.is_usb ? "USB" : "LSB");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ubitx::SidebandSet(!ubitx::status.is_usb);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuSplitToggle(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("SPLIT", ubitx::status.shift_mode == ubitx::SHIFT_SPLIT ? STR_ON : STR_OFF);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      if (ubitx::status.shift_mode == ubitx::SHIFT_SPLIT)
        ubitx::SplitDisable();
      else
        ubitx::SplitEnable();
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuCwSpeed(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      itoa(1200 / ubitx::settings.cw_speed, b, 10);
      strcat(b, " ");
      strcat(b, STR_WPM);
      ui::PrintStatusValue("CW", b);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      DrawWaitKnobScreen("CW SPEED", STR_WPM);
      SetWaitValues(1, 100, 1, 1200 / ubitx::settings.cw_speed,
                    PreviewCurrentValue);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      ubitx::CwSpeedSet(1200 / value.current);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuExit(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatus("EXIT MENU");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

/**
 * The calibration routines are not normally shown in the menu as they are
 * rarely used. They can be enabled by choosing this menu option.
 */
unsigned char MenuAdvanced(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("ADVANCED", advanced_menu ? STR_ON : ">");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      if (!advanced_menu) {
        return STATE_OPEN_ADVANCED;
      }
      advanced_menu = false;
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

void PreviewCalibration() {
  si5351::SetCalibration(value.current * 875);
  si5351::SetFreq(0, ubitx::settings.usb_carrier);
  ubitx::SetFrequency(ubitx::frequency);

  ltoa((long)(value.current * 875), b, DEC);
  ui::PrintLine(5, b);
}

unsigned char MenuSetupCalibration(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatus("CALIBRATE");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ui::u8x8.clear();
      ui::u8x8.draw1x2String(1, 1, "ZERO BEAT TO");
      ultoa(ubitx::frequency, b, DEC);
      ui::u8x8.draw1x2String(1, 3, b);

      SetWaitValues(-1000, 1000, 1, ubitx::settings.master_cal / 875,
                    PreviewCalibration);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      ubitx::SetMasterCal(value.current * 875);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

void PreviewCarrier() {
  si5351::SetFreq(0, value.current + 11053000);
  ultoa(value.current + 11053000, b, DEC);
  ui::PrintLine(4, b);
}

unsigned char MenuSetupCarrier(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatus("CAL BFO");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      DrawWaitKnobScreen("CALIBRATE BFO", "");
      //
      // Values from 11000000 to 11099999
      SetWaitValues(-32000, 32000, 1, ubitx::settings.usb_carrier - 11053000,
                    PreviewCarrier);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      ubitx::SetUsbCarrier(value.current);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

void PreviewSidetone() {
  tone(hw::CW_TONE, value.current);
  PreviewCurrentValue();
}

unsigned char MenuSetupCwTone(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      itoa(ubitx::settings.cw_side_tone, b, 10);
      strcat(b, " HZ");
      ui::PrintStatusValue(STR_CW_TONE, b);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      DrawWaitKnobScreen(STR_CW_TONE, "HZ");
      SetWaitValues(100, 2000, 10, ubitx::settings.cw_side_tone,
                    PreviewSidetone);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      noTone(hw::CW_TONE);
      ubitx::CwToneSet(value.current);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuSetupCwDelay(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      itoa(ubitx::settings.cw_delay_time, b, 10);
      strcat(b, " MS");
      ui::PrintStatusValue(STR_CW_DELAY, b);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      DrawWaitKnobScreen(STR_CW_DELAY, "MS");
      SetWaitValues(10, 1010, 50, ubitx::settings.cw_delay_time,
                    PreviewCurrentValue);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      ubitx::CwDelayTimeSet(value.current);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

void PreviewKeyer() {
  ui::u8x8.draw1x2String(4, 3, STRS_IAMBIC[value.current]);
  for (unsigned char i = strlen(STRS_IAMBIC[value.current]) + 4; i <= 15; i++) {
    ui::u8x8.draw1x2Glyph(i, 3, ' ');
  }
}

unsigned char MenuSetupKeyer(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue(STR_CW_KEY, STRS_IAMBIC[ubitx::settings.iambic_key]);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      DrawWaitKnobScreen(STR_CW_KEY, "");
      SetWaitValues(0, 2, 1, ubitx::settings.iambic_key, PreviewKeyer);
      return STATE_WAIT_VALUE;
    case EVENT_VALUE:
      ubitx::IambicKeySet(value.current);
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuTxToggle(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatusValue("TX INHIBIT", ubitx::status.tx_inhibit ? STR_ON : STR_OFF);
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ubitx::status.tx_inhibit = !ubitx::status.tx_inhibit;
      return STATE_EXIT;
  }
  return STATE_EXIT;
}

unsigned char MenuReadADC1(unsigned char event) {
  static unsigned char selected = 0;
  static int last_adc;

  switch (event) {
    case EVENT_SELECTED: {
      int adc = analogRead(PINS_ADC[selected]);
      if (adc != last_adc) {
        last_adc = adc;
        itoa(adc, b, 10);
        ui::PrintStatusValue(STRS_ADC[selected], b);
      }
      return STATE_DRAW_SELECTED;
    }
    case EVENT_ACTIVE:
      selected = (selected + 1) % 4;
      return STATE_DRAW_SELECTED;
  }
  return STATE_EXIT;
}

unsigned char MenuResetSettings(unsigned char event) {
  switch (event) {
    case EVENT_SELECTED:
      ui::PrintStatus("RESET");
      return STATE_SELECTING_MENU;
    case EVENT_ACTIVE:
      ubitx::ResetSettingsAndHalt();
  }
  // the above EVENT_ACTIVE case halts, so technically this is unreachable:
  return STATE_EXIT;
}

unsigned char CallMenuHandler(unsigned char menu, unsigned char event) {
  switch (menu) {
    case  0: return MenuVfoToggle(event);
    case  1: return MenuVfoCopy(event);
    case  2: return MenuCwSpeed(event);
    case  3: return MenuRitToggle(event);
    case  4: return MenuBand(event);
    case  5: return MenuSidebandToggle(event);
    case  6: return MenuAdvanced(event);
    case  7: return MenuSetupCalibration(event);
    case  8: return MenuSetupCarrier(event);
    case  9: return MenuSetupCwTone(event);
    case 10: return MenuSetupCwDelay(event);
    case 11: return MenuSetupKeyer(event);
    case 12: return MenuSplitToggle(event);
    case 13: return MenuTxToggle(event);
    case 14: return MenuReadADC1(event);
    case 15: return MenuResetSettings(event);
    case 16: return MenuExit(event);
  }
  return STATE_INITIAL;
}

unsigned char StateWaitValue() {
  int knob = encoder::ReadSlow();

  if (knob != 0) {
    if (knob < 0) value.current -= value.step;
    if (knob > 0) value.current += value.step;
    value.current = (value.current - value.min)
        / value.step * value.step
        + value.min;
    if (value.current < value.min) value.current = value.min;
    if (value.current > value.max) value.current = value.max;

    if (value.PreviewCallback) value.PreviewCallback();
  }

  if (mainloop::buttons.f_clicked) {
    mainloop::buttons.f_clicked = false;
    return STATE_VALUE_RETURNED;
  }
  return STATE_WAIT_VALUE;
}

void DoMenu() {
  static unsigned char state = STATE_INITIAL;
  static unsigned char active_menu;
  static int select; // encoder state

  switch (state) {
    case STATE_INITIAL:  // init
      select = 0;
      active_menu = 0;
      state = STATE_DRAW_SELECTED;
      break;
    case STATE_DRAW_SELECTED:  // draw active menu item
      state = CallMenuHandler(active_menu, EVENT_SELECTED);
      // fall through
    case STATE_SELECTING_MENU: {  // wait encoder change to select other menu, or click to enter
      if (mainloop::buttons.f_clicked) {
        mainloop::buttons.f_clicked = false;
        state = STATE_MENU_ITEM_ACTIVE;  // handle menu item inside
        break;
      }

      if (mainloop::buttons.f_held) {
        mainloop::buttons.f_held = false;
        state = STATE_EXIT;  // exit
        break;
      }

      select += encoder::ReadSlow();
      if (advanced_menu && select > 169) select = 169;
      if (!advanced_menu && select > 79) select = 79;
      if (select < 0) select = 0;

      unsigned char new_active_menu = select / 10;
      if (!advanced_menu && new_active_menu == 7) new_active_menu = 16;

      if (new_active_menu != active_menu) { // menu changed
        active_menu = new_active_menu;
        state = STATE_DRAW_SELECTED;  // draw active menu item
        break;
      }
      break;
    }
    case STATE_MENU_ITEM_ACTIVE: // entered menu item handling
      state = CallMenuHandler(active_menu, EVENT_ACTIVE);
      break;
    case STATE_WAIT_VALUE:
      state = StateWaitValue();
      break;
    case STATE_VALUE_RETURNED:
      state = CallMenuHandler(active_menu, EVENT_VALUE);
      break;
    case STATE_OPEN_ADVANCED:
      advanced_menu = true;
      select = 75;
      active_menu = 7;
      state = STATE_DRAW_SELECTED;
      break;
    case STATE_EXIT: // exit
      // ui::u8x8.setInverseFont(1); TODO - where we do this now?
      state = STATE_INITIAL;
      ui::u8x8.clear();
      mainloop::DoActiveApp = mainloop::DoTuning;
      break;
  }
}

}  // namespace
