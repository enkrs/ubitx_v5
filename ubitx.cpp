/**
 * This source file is under General Public License version 3.
 * 
 * This verision uses a built-in Si5351 library
 * Most source code are meant to be understood by the compilers and the computers. 
 * Code that has to be hackable needs to be well understood and properly documented. 
 * Donald Knuth coined the term Literate Programming to indicate code that is written be 
 * easily read and understood.
 * 
 * The Raduino is a small board that includes the Arduin Nano, a 16x2 LCD display and
 * an Si5351a frequency synthesizer. This board is manufactured by Paradigm Ecomm Pvt Ltd
 * 
 * To learn more about Arduino you may visit www.arduino.cc. 
 * 
 * The Arduino works by starts executing the code in a function called setup() and then it 
 * repeatedly keeps calling loop() forever. All the initialization code is kept in setup()
 * and code to continuously sense the tuning knob, the function button, transmit/receive,
 * etc is all in the loop() function. If you wish to study the code top down, then scroll
 * to the bottom of this file and read your way up.
 * 
 * Below are the libraries to be included for building the Raduino 
 * The EEPROM library is used to store settings like the frequency memory, caliberation data, 
 * callsign etc .
 *
 *  The main chip which generates upto three oscillators of various frequencies in the
 *  Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet 
 *  from www.silabs.com although, strictly speaking it is not a requirment to understand this code. 
 *  Instead, you can look up the Si5351 library written by xxx, yyy. You can download and 
 *  install it from www.url.com to complile this file.
 *  The Wire.h library is used to talk to the Si5351 and we also declare an instance of 
 *  Si5351 object to control the clocks.
 */
#include "ubitx.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "eeprom.h"
#include "hw.h"
#include "keyer.h"
#include "mainloop.h"
#include "menu.h"
#include "si5351.h"
#include "ui.h"

namespace ubitx {

/**
 * The Arduino, unlike C/C++ on a regular computer with gigabytes of RAM, has very little memory.
 * We have to be very careful with variables that are declared inside the functions as they are 
 * created in a memory region called the stack. The stack has just a few bytes of space on the Arduino
 * if you declare large strings inside functions, they can easily exceed the capacity of the stack
 * and mess up your programs. 
 * We circumvent this by declaring a few global buffers as  kitchen counters where we can 
 * slice and dice our strings. These strings are mostly used to control the display or handle
 * the input and output from the USB port. We must keep a count of the bytes used while reading
 * the serial port as we can easily run out of buffer space. This is done in the serial_in_count variable.
 */

unsigned long first_if;

Status status;
Settings settings;


unsigned char active_band;
// TODO: can we live with only one RIT variable?
unsigned long frequency;
unsigned long rit_rx_frequency;
unsigned long rit_tx_frequency;  //frequency is the current frequency on the dial

/**
 * Raduino needs to keep track of current state of the transceiver. These are a few variables that do it
 */
char in_tx = 0;  //it is set to 1 if in transmit mode (whatever the reason : cw, ptt or cat)

/**
 * Below are the basic functions that control the uBitx. Understanding the functions before 
 * you start hacking around
 */

/**
 * Our own delay. During any delay, the raduino should still be processing a few times. 
 */

void ActiveDelay(unsigned int delay_by) {
  unsigned long time_start = millis();

  while (millis() - time_start <= delay_by) {
    // Here was CheckCat(), but it's not needed as is now handled
    // with task scheduler.
    // Goal is to refactor all functions calling ActiveDelay
    // to run friendly with taskscheduler
  }
}

/**
 * Select the properly tx harmonic filters
 * The four harmonic filters use only three relays
 * the four LPFs cover 30-21 Mhz, 18 - 14 Mhz, 7-10 MHz and 3.5 to 5 Mhz
 * Briefly, it works like this, 
 * - When KT1 is OFF, the 'off' position routes the PA output through the 30 MHz LPF
 * - When KT1 is ON, it routes the PA output to KT2. Which is why you will see that
 *   the KT1 is on for the three other cases.
 * - When the KT1 is ON and KT2 is off, the off position of KT2 routes the PA output
 *   to 18 MHz LPF (That also works for 14 Mhz) 
 * - When KT1 is On, KT2 is On, it routes the PA output to KT3
 * - KT3, when switched on selects the 7-10 Mhz filter
 * - KT3 when switched off selects the 3.5-5 Mhz filter
 * See the circuit to understand this
 */
void SetTxFilters(unsigned long freq) {
  if (freq > 21000000L) {  // the default filter is with 35 MHz cut-off
    digitalWrite(hw::TX_LPF_A, 0);
    digitalWrite(hw::TX_LPF_B, 0);
    digitalWrite(hw::TX_LPF_C, 0);
  } else if (freq >= 14000000L) { //thrown the KT1 relay on, the 30 MHz LPF is bypassed and the 14-18 MHz LPF is allowd to go through
    digitalWrite(hw::TX_LPF_A, 1);
    digitalWrite(hw::TX_LPF_B, 0);
    digitalWrite(hw::TX_LPF_C, 0);
  } else if (freq > 7000000L) {
    digitalWrite(hw::TX_LPF_A, 0);
    digitalWrite(hw::TX_LPF_B, 1);
    digitalWrite(hw::TX_LPF_C, 0);
  } else {
    digitalWrite(hw::TX_LPF_A, 0);
    digitalWrite(hw::TX_LPF_B, 0);
    digitalWrite(hw::TX_LPF_C, 1);
  }
}

/**
 * This is the most frequently called function that configures the 
 * radio to a particular frequeny, sideband and sets up the transmit filters
 * 
 * The transmit filter relays are powered up only during the tx so they dont
 * draw any current during rx. 
 * 
 * The carrier oscillator of the detector/modulator is permanently fixed at
 * uppper sideband. The sideband selection is done by placing the second oscillator
 * either 12 Mhz below or above the 45 Mhz signal thereby inverting the sidebands 
 * through mixing of the second local oscillator.
 */
 
void SetFrequency(unsigned long f) {
  if (f < LOWEST_FREQ)
    f = LOWEST_FREQ;
  if (f > HIGHEST_FREQ)
    f = HIGHEST_FREQ;

  SetTxFilters(f);

  if (status.is_usb) {
    si5351::SetFreq(2, first_if + f);
    si5351::SetFreq(1, first_if + settings.usb_carrier);
  } else{
    si5351::SetFreq(2, first_if + f);
    si5351::SetFreq(1, first_if - settings.usb_carrier);
  }
    
  frequency = f;

  unsigned int khz = frequency / 1000;
  unsigned char i = 0;
  for (i = 0; i < 12; i++) {
    if ((khz >= BAND_LIST[i].min_khz) &&
        (khz <= BAND_LIST[i].max_khz)) {
      break;
    }
  }
  active_band = i;
}

/**
 * TxStart is called by the PTT, cw keyer and CAT protocol to
 * put the uBitx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 * CW offest is calculated as lower than the operating frequency when in LSB mode, and vice versa in USB mode
 */
void TxStart(bool start_cw) {
  if (!status.tx_inhibit)
    digitalWrite(hw::TX_RX, 1);
  in_tx = 1;
  
  if (status.shift_mode == SHIFT_RIT) { // rit
    //save the current as the rx frequency
    rit_rx_frequency = frequency;
    SetFrequency(rit_tx_frequency);
  } else if (status.shift_mode == SHIFT_SPLIT) { // split
    VfoSwap(/*save=*/false);
  }

  if (start_cw && !status.tx_inhibit) {
    //turn off the second local oscillator and the bfo
    si5351::SetFreq(0, 0);
    si5351::SetFreq(1, 0);

    //shif the first oscillator to the tx frequency directly
    //the key up and key down will toggle the carrier unbalancing
    //the exact cw frequency is the tuned frequency + sidetone
    if (status.is_usb)
      si5351::SetFreq(2, frequency + settings.cw_side_tone);
    else
      si5351::SetFreq(2, frequency - settings.cw_side_tone); 
  }
  ui::UpdateDisplay();
}

void TxStartSsb() {
  TxStart(/*start_cw=*/false);
}
void TxStartCw() {
  TxStart(/*start_cw=*/true);
}


void TxStop() {
  in_tx = 0;

  digitalWrite(hw::TX_RX, 0);
  si5351::SetFreq(0, settings.usb_carrier);  //set back the carrrier oscillator, cw tx switches it off

  if (status.shift_mode == SHIFT_RIT ) { // rit
    frequency = rit_rx_frequency;
  } else if (status.shift_mode == SHIFT_SPLIT ) { // split
    VfoSwap(/*save=*/false);
  }
  SetFrequency(frequency);
  ui::UpdateDisplay();
}

/**
 * RitEnable is called with a frequency parameter that determines
 * what the tx frequency will be
 */
void RitEnable(unsigned long f) {
  status.shift_mode = SHIFT_RIT;
  //save the non-rit frequency back into the VFO memory
  //as RIT is a temporary shift, this is not saved to EEPROM
  rit_tx_frequency = f;
}

void RitDisable() {
  if (status.shift_mode == SHIFT_RIT) {
    status.shift_mode = SHIFT_NONE;
    SetFrequency(rit_tx_frequency);
    ui::UpdateDisplay();
  }
}

void CwSpeedSet(unsigned int speed) {
  settings.cw_speed = speed;
  EEPROM.put(eeprom::CW_SPEED, settings.cw_speed);
}

void CwToneSet(unsigned int tone) {
  settings.cw_side_tone = tone;
  EEPROM.put(eeprom::CW_SIDE_TONE, settings.cw_side_tone);
}

void CwDelayTimeSet(unsigned int delay_time) {
  settings.cw_delay_time = delay_time;
  EEPROM.put(eeprom::CW_DELAY_TIME, settings.cw_delay_time);
}

void IambicKeySet(unsigned char key) {
  settings.iambic_key = key;
  EEPROM.put(eeprom::IAMBIC_KEY, settings.iambic_key);

  if (settings.iambic_key == 1)
    keyer::keyer_control &= ~0x10;  // IAMBICB bit
  if (settings.iambic_key == 2)
    keyer::keyer_control |= 0x10;  // IAMBICB bit
}


void SidebandSet(bool usb) {
  status.is_usb = usb;
  SetFrequency(frequency);
}

void VfoSwap(bool save) {
  RitDisable();
  if (status.vfo_a_active) {
    settings.vfo_a = frequency;
    settings.vfo_a_usb = status.is_usb;
    if (save) {
      EEPROM.put(eeprom::VFO_A, settings.vfo_a);
      EEPROM.put(eeprom::VFO_A_USB, settings.vfo_a_usb);
    }

    status.vfo_a_active = false;
    frequency = settings.vfo_b;
    status.is_usb = settings.vfo_b_usb;
  } else {
    settings.vfo_b = frequency;
    settings.vfo_b_usb = status.is_usb;
    if (save) {
      EEPROM.put(eeprom::VFO_B, settings.vfo_b);
      EEPROM.put(eeprom::VFO_B_USB, settings.vfo_b_usb);
    }

    status.vfo_a_active = true;
    frequency = settings.vfo_a;
    status.is_usb = settings.vfo_a_usb;
  }
  SetFrequency(frequency);
}

void VfoCopy(bool save) {
  RitDisable();
  settings.vfo_a = frequency;
  settings.vfo_a_usb = status.is_usb;
  settings.vfo_b = frequency;
  settings.vfo_b_usb = status.is_usb;
  if (save) {
    EEPROM.put(eeprom::VFO_A, settings.vfo_a);
    EEPROM.put(eeprom::VFO_A_USB, settings.vfo_a_usb);
    EEPROM.put(eeprom::VFO_B, settings.vfo_b);
    EEPROM.put(eeprom::VFO_B_USB, settings.vfo_b_usb);
  }
}

void SplitEnable() {
  status.shift_mode = SHIFT_SPLIT;
}

void SplitDisable() {
  if (status.shift_mode == SHIFT_SPLIT) {
    status.shift_mode = SHIFT_NONE;
  }
}

void SetUsbCarrier(unsigned long long carrier) {
  settings.usb_carrier = carrier;
  EEPROM.put(eeprom::USB_CARRIER, settings.usb_carrier);

  si5351::SetFreq(0, settings.usb_carrier);
  SetFrequency(frequency);
}

void SetMasterCal(long int cal) {
  settings.master_cal = cal;
  EEPROM.put(eeprom::MASTER_CAL, settings.master_cal);
  InitOscillators();
}

/**
 * Basic User Interface Routines. These check the front panel for any activity
 */

void ResetSettingsAndHalt() {
  // Jaunākā kalibrācija veikta 2020-06-02 22:03 MASTER 161000, BFO 11056586
  settings.master_cal = 161000l;
  settings.usb_carrier = 11056586l;
  settings.cw_side_tone = 800;
  settings.vfo_a = 3573000l;
  settings.vfo_b = 7074000l;
  settings.cw_speed = 100;
  settings.vfo_a_usb = true;
  settings.vfo_b_usb = true;
  settings.iambic_key = 1;
  settings.cw_delay_time = 60;

  EEPROM.put(eeprom::MASTER_CAL, settings.master_cal);
  EEPROM.put(eeprom::USB_CARRIER, settings.usb_carrier);
  EEPROM.put(eeprom::CW_SIDE_TONE, settings.cw_side_tone);
  EEPROM.put(eeprom::VFO_A, settings.vfo_a);
  EEPROM.put(eeprom::VFO_B, settings.vfo_b);
  EEPROM.put(eeprom::CW_SPEED, settings.cw_speed);
  EEPROM.put(eeprom::VFO_A_USB, settings.vfo_a_usb);
  EEPROM.put(eeprom::VFO_B_USB, settings.vfo_b_usb);
  EEPROM.put(eeprom::IAMBIC_KEY, settings.iambic_key);
  EEPROM.put(eeprom::CW_DELAY_TIME, settings.cw_delay_time);

  char magicNr = eeprom::MAGIC_NR; // TODO unneded variable
  EEPROM.put(eeprom::MAGIC_ADDR, magicNr);

  ui::u8x8.clear();
  ui::PrintLine(2, "EEPROM RESET");
  ui::PrintLine(4, "TURN OFF POWER");
  while (1) {}
}

/**
 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void InitSettings() {
  char magicNr;

  EEPROM.get(eeprom::MAGIC_ADDR, magicNr);
  if (magicNr != eeprom::MAGIC_NR) {
    ResetSettingsAndHalt();
    return;
  }

  EEPROM.get(eeprom::MASTER_CAL, settings.master_cal);
  EEPROM.get(eeprom::USB_CARRIER, settings.usb_carrier);
  EEPROM.get(eeprom::CW_SIDE_TONE, settings.cw_side_tone);
  EEPROM.get(eeprom::VFO_A, settings.vfo_a);
  EEPROM.get(eeprom::VFO_B, settings.vfo_b);
  EEPROM.get(eeprom::CW_SPEED, settings.cw_speed);
  EEPROM.get(eeprom::VFO_A_USB, settings.vfo_a_usb);
  EEPROM.get(eeprom::VFO_B_USB, settings.vfo_b_usb);
  EEPROM.get(eeprom::IAMBIC_KEY, settings.iambic_key);
  EEPROM.get(eeprom::CW_DELAY_TIME, settings.cw_delay_time);

  // TODO - EEPROM
  first_if = 45005000L; // should be eeprom


  // calculate internal variables
  status.tx_inhibit = 0;
  status.vfo_a_active = true;
  status.is_usb = settings.vfo_a_usb;
  status.shift_mode = SHIFT_NONE;

   if (settings.iambic_key == 1)
     keyer::keyer_control &= ~0x10;
   else if (settings.iambic_key == 2)
     keyer::keyer_control |= 0x10;
}

void InitOscillators() {
  //initialize the SI5351
  si5351::Init();
  si5351::SetCalibration(settings.master_cal);
  si5351::SetFreq(0, settings.usb_carrier);
}

void InitPorts() {
  pinMode(hw::ENC_A, INPUT_PULLUP);
  pinMode(hw::ENC_B, INPUT_PULLUP);
  pinMode(hw::FBUTTON, INPUT_PULLUP);

  pinMode(hw::PTT, INPUT_PULLUP);
  pinMode(hw::ANALOG_V, INPUT_PULLUP);

  pinMode(hw::CW_TONE, OUTPUT);
  digitalWrite(hw::CW_TONE, 0);

  pinMode(hw::TX_RX, OUTPUT);
  digitalWrite(hw::TX_RX, 0);

  pinMode(hw::TX_LPF_A, OUTPUT);
  pinMode(hw::TX_LPF_B, OUTPUT);
  pinMode(hw::TX_LPF_C, OUTPUT);
  digitalWrite(hw::TX_LPF_A, 0);
  digitalWrite(hw::TX_LPF_B, 0);
  digitalWrite(hw::TX_LPF_C, 0);

  pinMode(hw::CW_KEY, OUTPUT);
  digitalWrite(hw::CW_KEY, 0);

  pinMode(hw::OLED_ENABLE, OUTPUT);
  digitalWrite(hw::OLED_ENABLE, HIGH);
}

}  // namespace

