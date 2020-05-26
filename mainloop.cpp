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
unsigned char active_screen = 0; // 0 tuning, 1 menu
unsigned long FBTN_HOLD_TIMEOUT = 500;


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

/**
 * The PTT is checked only if we are not already in a cw transmit session
 * If the PTT is pressed, we shift to the ritbase if the rit was on
 * flip the T/R line to T and update the display to denote transmission
 */

void CheckTx() {	
  //we don't check for ptt when transmitting cw
  if (keyer::cw_timeout > 0)
    return;
  //we don't check for ptt tx down by cat
  if (cat::tx_cat)
    return;
    
  if (ubitx::in_tx == 0 && buttons.ptt_down)
    ubitx::TxStartSsb();
	
  if (ubitx::in_tx == 1 && !buttons.ptt_down)
    ubitx::TxStop();
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

bool FButtonClicked() {
  if (!buttons.f_clicked)
    return false;

  buttons.f_clicked = false;
  return true;
}

unsigned char EnterTuning() {
  ui::UpdateDisplay();
  return SCREEN_TUNING;
}


/**
 * The tuning jumps by 50 Hz on each step when you tune slowly
 * As you spin the encoder faster, the jump size also increases 
 * This way, you can quickly move to another band by just spinning the 
 * tuning knob
 */
unsigned char DoTuning() {
  ui::UpdateVoltage();

  if (FButtonClicked()) { // enter the menu
    return menu::EnterMenu();
  }

  if (buttons.f_held) {
    buttons.f_held = false;
    if (ubitx::status.shift_mode == ubitx::SHIFT_RIT) {
      ubitx::RitDisable();
    } else {
      ubitx::VfoSwap(/*save=*/true);
    }
    ui::UpdateDisplay();
    return SCREEN_TUNING;
  }

  if (ubitx::in_tx) return SCREEN_TUNING; // Tuning only when RX
  int knob = encoder::Read();

  if (knob == 0) return SCREEN_TUNING;

  if (ubitx::status.shift_mode == ubitx::SHIFT_RIT) {
    // Rit tuning
    if (knob < 0)
      ubitx::frequency -= 100l;
    else if (knob > 0)
      ubitx::frequency += 100;
  } else {
    // Normal tuning
    unsigned long prev_freq = ubitx::frequency;
    if (abs(knob) > 5)
      ubitx::frequency += knob * 1000;
    else 
      ubitx::frequency += knob * 50;

    if (prev_freq < 10000000l && ubitx::frequency >= 10000000l)
      ubitx::status.is_usb = true;
      
    if (prev_freq >= 10000000l && ubitx::frequency < 10000000l)
      ubitx::status.is_usb = false;
  }

  ubitx::SetFrequency(ubitx::frequency);
  ui::UpdateDisplay();
  return SCREEN_TUNING;
}

// TODO:
// The main loop has multiple states affected by the buttons
//
// TUNING: display shows frequency, dail adjusts freq or rit
// MENU: display shows menu list or selected item and adjusts
// TX: radio is transmitting
//
// The button monitoring returns values
// ptt_event  PTT_UP > PTT_FALLING > PTT_DOWN > PTT_RAISING
// fbtn_event F_UP > F_CLICK
//                 > F_HOLD


void Run() {
  CheckButtons();
  CheckTx();

  switch (active_screen) {
    case SCREEN_TUNING: active_screen = DoTuning(); break;
    case SCREEN_MENU: active_screen = menu::DoMenu(); break;
  }
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

  mainloop::active_screen = mainloop::EnterTuning();
}

// Arduino loop function

void loop() { 
  cat::Run();
  keyer::Run();
  mainloop::Run();
}
