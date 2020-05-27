#include "mainloop.h"
#include <Arduino.h>
#include "cat.h"
#include "encoder.h"
#include "hw.h"
#include "keyer.h"
#include "menu.h"
#include "ubitx.h"
#include "ui.h"

namespace mainloop {

Buttons buttons;
unsigned long FBTN_HOLD_TIMEOUT = 500;

void (*DoActiveApp)();

// returns 1 if the button is pressed
// handles debounce
bool FBtnDown() {
  bool now = false, prev = false;
  int i = 0;

  do {
    now = (PINC & PC2_FBUTTON) == 0; // 0 means button short to ground
    if (prev != now) {
      prev = now;
      i = 0;
    }
  } while (i++ < debounce_count);

  return now;
}

bool PttDown() {
  bool now = false, prev = false;
  int i = 0;

  do {
    now = (PINC & PC3_PTT) == 0; // 0 means button short to ground
    if (prev != now) {
      prev = now;
      i = 0;
    }
  } while (i++ < debounce_count);

  return now;
}
    
void CheckButtons() {
  static unsigned long hold_time = 0;
  static bool held_dispatched = false;

  bool prev_f_down = buttons.f_down;
  buttons.f_down = FBtnDown();

  if (buttons.f_down == true) {
    unsigned long now = millis();
    if (prev_f_down == false) // pushing button down
      hold_time = now + FBTN_HOLD_TIMEOUT;

    if (!held_dispatched && hold_time && now > hold_time) { // time passed
      buttons.f_held = true;
      held_dispatched = true;
    }
  }

  if (buttons.f_down == false) {
    if (prev_f_down == true && !held_dispatched) // releasing button
      buttons.f_clicked = true;
    hold_time = 0;
    held_dispatched = false;
  }

  buttons.ptt_down = PttDown();
}

/**
 * this function is repeatedly called to handle transmission
 */
void DoTx() {
  enum DO_TX_STATES {
    STATE_INITIAL,
    STATE_IN_TX
  };
  static unsigned char state = STATE_INITIAL;

  switch (state) {
    case STATE_INITIAL:
      state = STATE_IN_TX;
      ubitx::TxStartSsb();
      break;
    case STATE_IN_TX:
      if (!buttons.ptt_down) { // ptt has been released
        ubitx::TxStop();
      }
      if (!ubitx::in_tx) { // tx has been canceled
        state = STATE_INITIAL;
        DoActiveApp = DoTuning;
        break;
      }
      // TODO update power/swr readings here
      break;
  }
}

/**
 * This function is the default radio screen. Called repeatedly
 * while tuning. 
 */
void DoTuning() {
  enum DO_TUNING_STATES {
    STATE_INITIAL,
    STATE_LOOP
  };
  static unsigned char state = STATE_INITIAL;

  switch (state) {
    case STATE_INITIAL:
      ui::UpdateDisplay();
      state = STATE_LOOP;
      break;
    case STATE_LOOP: // loop
      ui::UpdateVoltage();

      if (buttons.f_clicked) { // enter the menu
        buttons.f_clicked = false;
        state = STATE_INITIAL;
        DoActiveApp = menu::DoMenu;
        break;
      }

      if (buttons.f_held) {
        buttons.f_held = false;
        if (ubitx::status.shift_mode == ubitx::SHIFT_RIT) {
          ubitx::RitDisable();
        } else {
          ubitx::VfoCopy(/*save=*/true);
        }
        ui::UpdateDisplay();
        break;
      }

      if ((keyer::cw_timeout == 0)  // don't check for ptt when transmitting cw
          && (!cat::tx_cat)         // don't check for ptt tx down by cat
          && (buttons.ptt_down)) {
        state = 0;
        DoActiveApp = DoTx;
        break;
      }

      int knob = encoder::Read();
      if (knob == 0) break;
      if (ubitx::status.shift_mode == ubitx::SHIFT_RIT) {
        // Rit tuning
        if (knob < 0) ubitx::frequency -= 100l;
        else if (knob > 0) ubitx::frequency += 100;
      } else {
        // Normal tuning
        if (abs(knob) > 6) ubitx::frequency += knob * 1000;
        else ubitx::frequency += knob * 50;
      }

      ubitx::SetFrequency(ubitx::frequency);
      ui::UpdateDisplay();
      break;
  }
}

// The main loop has multiple states affected by the buttons
//
// DoTuning: display shows frequency, dail adjusts freq or rit
// DoMenu: display shows menu list or selected item and adjusts
// DoTx: radio is transmitting
//

void Run() {
  CheckButtons(); // update buttons - debounces, clicks
  DoActiveApp(); // DoTuning, DoMenu or DoTx
}

}  // namespace

// Out of namespace
// Arduino setup function

void setup() {
  Serial.begin(38400);
  Serial.flush();  

  ubitx::InitPorts();     
  
  ui::u8x8.begin();
  // the "_f" version uses extra 1280 bytes of storage space
  // u8x8.setFont(u8x8_font_amstrad_cpc_extended_f); 
  ui::u8x8.setFont(U8X8_MAINFONT); 

  //we print this line so this shows up even if the raduino 
  //crashes later in the code
  ui::u8x8.draw1x2String(1, 1, "YL3AME"); 

  ubitx::InitSettings();
  if (mainloop::FBtnDown())
    ubitx::ResetSettingsAndHalt();

  encoder::Init();
  ubitx::InitOscillators();

  ubitx::SetFrequency(ubitx::settings.vfo_a);
  ubitx::SidebandSet(ubitx::settings.vfo_a_usb);

  mainloop::DoActiveApp = mainloop::DoTuning;
}

// Arduino loop function

void loop() { 
  cat::Run();
  keyer::Run();
  mainloop::Run();
}
