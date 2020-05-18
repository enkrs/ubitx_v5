#include "mainloop.h"
#include <Arduino.h>
#include "cat.h"
#include "encoder.h"
#include "hw.h"
#include "keyer.h"
#include "menu.h"
#include "settings.h"
#include "si5351.h"
#include "ubitx.h"
#include "ui.h"

namespace mainloop {

/**
 * The PTT is checked only if we are not already in a cw transmit session
 * If the PTT is pressed, we shift to the ritbase if the rit was on
 * flip the T/R line to T and update the display to denote transmission
 */

void CheckPtt() {	
  //we don't check for ptt when transmitting cw
  if (keyer::cw_timeout > 0)
    return;
  //we don't check for ptt tx down by cat
  if (cat::tx_cat)
    return;
    
  if (ubitx::in_tx == 0 && (PINC & (1<<PC3)) == 0) {
    for (int i = 0; i < ui::debounce_count; i++)
      if ((PINC & (1<<PC3)) != 0) return;  // debounce
    ubitx::TxStart(ubitx::TX_SSB);
    return;
  }
	
  if (ubitx::in_tx == 1 && (PINC & (1<<PC3)) != 0) {
    for (int i = 0; i < ui::debounce_count; i++)
      if ((PINC & (1<<PC3)) == 0) return;  // debounce
    ubitx::TxStop();
    return;
  }
}

void CheckButton() {
  if (menu::menu_state != 0) return; // menu handles its own button

  if (!ui::BtnDown()) return;
  ui::BtnWaitUp();
 
  if (ubitx::in_tx) {
    ubitx::TxStop(); // stop tx with button
  } else {
    menu::menu_state = 3;
  }
}

/**
 * The tuning jumps by 50 Hz on each step when you tune slowly
 * As you spin the encoder faster, the jump size also increases 
 * This way, you can quickly move to another band by just spinning the 
 * tuning knob
 */
void DoTuning() {
  if (ubitx::in_tx) return; // Tuning only when RX

  int knob = encoder::Read();

  if (knob == 0) return;

  if (ubitx::shift_mode == ubitx::SHIFT_RIT) {
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
      ubitx::is_usb = 1;
      
    if (prev_freq >= 10000000l && ubitx::frequency < 10000000l)
      ubitx::is_usb = 0;
  }

  ubitx::SetFrequency(ubitx::frequency);
  ui::UpdateDisplay();
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
  CheckPtt();
  CheckButton();

  if (menu::menu_state) {
    menu::DoMenu();
  } else {
    DoTuning();
    ui::UpdateVoltage();
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
  if (ui::BtnDown())
    ubitx::ResetSettingsAndHalt();

  encoder::Init();
  si5351::Init();

  ubitx::SetFrequency(settings::vfo_a);
  ubitx::SidebandSet(settings::vfo_a_usb);
  ui::UpdateDisplay();
}

// Arduino loop function

void loop() { 
  cat::Run();
  keyer::Run();
  mainloop::Run();
}
