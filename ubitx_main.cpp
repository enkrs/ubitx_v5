#include "ubitx_main.h"

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#include "hardware.h"

#include "encoder.h"
#include "ubitx.h"
#include "ubitx_cat.h"
#include "ubitx_keyer.h"
#include "ubitx_menu.h"
#include "ubitx_ui.h"

/**
 * The PTT is checked only if we are not already in a cw transmit session
 * If the PTT is pressed, we shift to the ritbase if the rit was on
 * flip the T/R line to T and update the display to denote transmission
 */

void CheckPtt() {	
  //we don't check for ptt when transmitting cw
  if (cw_timeout > 0)
    return;
    
  if (digitalRead(PTT) == LOW && in_tx == 0) {
    TxStart(TX_SSB);
    ActiveDelay(50); //debounce the PTT
  }
	
  if (digitalRead(PTT) == HIGH && in_tx == 1)
    TxStop();
}

void CheckButton() {
  //only if the button is pressed
  if (!BtnDown()) return;
  ActiveDelay(50);
  if (!BtnDown()) return;  //debounce
 
  master.setCallback(&MenuTaskStart);
}

/**
 * The tuning jumps by 50 Hz on each step when you tune slowly
 * As you spin the encoder faster, the jump size also increases 
 * This way, you can quickly move to another band by just spinning the 
 * tuning knob
 */
void DoTuning() {
  int s;
  unsigned long prev_freq;

  s = EncRead();
  if (s != 0) {
    prev_freq = frequency;

    if (s > 4)
      frequency += 10000l;
    else if (s > 2)
      frequency += 500;
    else if (s > 0)
      frequency +=  50l;
    else if (s > -2)
      frequency -= 50l;
    else if (s > -4)
      frequency -= 500l;
    else
      frequency -= 10000l;

    if (prev_freq < 10000000l && frequency >= 10000000l)
      is_usb = 1;
      
    if (prev_freq >= 10000000l && frequency < 10000000l)
      is_usb = 0;

    SetFrequency(frequency);
    UpdateDisplay();
  }
}

/**
 * RIT only steps back and forth by 100 hz at a time
 */
void DoRit() {
  int knob = EncRead();
  unsigned long prev_freq = frequency;

  if (knob < 0)
    frequency -= 100l;
  else if (knob > 0)
    frequency += 100;
 
  if (prev_freq != frequency) {
    SetFrequency(frequency);
    UpdateDisplay();
  }
}


/**
 * Master task loop, checks for ptt, tuning, menu
 */
void MasterTask() {
  if (!tx_cat) CheckPtt();
  CheckButton();

  //tune only when not tranmsitting 
  if (!in_tx) {
    if (shift_mode == SHIFT_RIT)
      DoRit();
    else
      DoTuning();
  }
  UpdateVoltage();
}

Task master(TASK_IMMEDIATE, TASK_FOREVER, &MasterTask);
