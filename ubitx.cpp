
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
#include <Wire.h>
#include <EEPROM.h>
#include <TaskSchedulerDeclarations.h>

#include "hardware.h"
#include "eeprom.h"

#include "encoder.h"
#include "ubitx_cat.h"
#include "ubitx_keyer.h"
#include "ubitx_menu.h"
#include "ubitx_si5351.h"
#include "ubitx_ui.h"


Scheduler scheduler;

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
char c[30], b[30];      

// These variables are stored in EEPROM:
long master_cal;
unsigned long usb_carrier;
unsigned int cw_side_tone;
unsigned long vfo_a;
unsigned long vfo_b;
unsigned int cw_speed;
unsigned char vfo_a_usb;
unsigned char vfo_b_usb;
unsigned char iambic_key;

unsigned long first_if;
char shift_mode;  // 0 - normal, 1 - rit, 2 - split
char vfo_active;
int cw_delay_time;
char is_usb;
char keyer_control;
char tx_inhibit;

// TODO: can we live with only one RIT variable?
unsigned long frequency;
unsigned long rit_rx_frequency;
unsigned long rit_tx_frequency;  //frequency is the current frequency on the dial

/**
 * Raduino needs to keep track of current state of the transceiver. These are a few variables that do it
 */
char in_tx = 0;  //it is set to 1 if in transmit mode (whatever the reason : cw, ptt or cat)
unsigned long cw_timeout = 0;  //milliseconds to go before the cw transmit line is released and the radio goes back to rx mode
                               //beat frequency

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
    CheckCat();
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
    digitalWrite(TX_LPF_A, 0);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  } else if (freq >= 14000000L) { //thrown the KT1 relay on, the 30 MHz LPF is bypassed and the 14-18 MHz LPF is allowd to go through
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  } else if (freq > 7000000L) {
    digitalWrite(TX_LPF_A, 0);
    digitalWrite(TX_LPF_B, 1);
    digitalWrite(TX_LPF_C, 0);    
  } else {
    digitalWrite(TX_LPF_A, 0);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 1);    
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

  if (is_usb) {
    si5351bx_setfreq(2, first_if + f);
    si5351bx_setfreq(1, first_if + usb_carrier);
  } else{
    si5351bx_setfreq(2, first_if + f);
    si5351bx_setfreq(1, first_if - usb_carrier);
  }
    
  frequency = f;
}

/**
 * TxStart is called by the PTT, cw keyer and CAT protocol to
 * put the uBitx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 * CW offest is calculated as lower than the operating frequency when in LSB mode, and vice versa in USB mode
 */
void TxStart(char tx_mode) {
  if (!tx_inhibit)
    digitalWrite(TX_RX, 1);
  in_tx = 1;
  
  if (shift_mode == SHIFT_RIT) { // rit
    //save the current as the rx frequency
    rit_rx_frequency = frequency;
    SetFrequency(rit_tx_frequency);
  } else if (shift_mode == SHIFT_SPLIT) { // split
    VfoSwap(0);
  }

  if (tx_mode == TX_CW && !tx_inhibit) {
    //turn off the second local oscillator and the bfo
    si5351bx_setfreq(0, 0);
    si5351bx_setfreq(1, 0);

    //shif the first oscillator to the tx frequency directly
    //the key up and key down will toggle the carrier unbalancing
    //the exact cw frequency is the tuned frequency + sidetone
    if (is_usb)
      si5351bx_setfreq(2, frequency + cw_side_tone);
    else
      si5351bx_setfreq(2, frequency - cw_side_tone); 
  }
  UpdateDisplay();
}

void TxStop() {
  in_tx = 0;

  digitalWrite(TX_RX, 0);           //turn off the tx
  si5351bx_setfreq(0, usb_carrier);  //set back the carrrier oscillator, cw tx switches it off

  if (shift_mode == SHIFT_RIT ) { // rit
    frequency = rit_rx_frequency;
  } else if (shift_mode == SHIFT_SPLIT ) { // split
    VfoSwap(0);
  }
  SetFrequency(frequency);
  UpdateDisplay();
}

/**
 * RitEnable is called with a frequency parameter that determines
 * what the tx frequency will be
 */
void RitEnable(unsigned long f) {
  shift_mode = SHIFT_RIT;
  //save the non-rit frequency back into the VFO memory
  //as RIT is a temporary shift, this is not saved to EEPROM
  rit_tx_frequency = f;
}

void RitDisable() {
  if (shift_mode == SHIFT_RIT) {
    shift_mode = SHIFT_NONE;
    SetFrequency(rit_tx_frequency);
    UpdateDisplay();
  }
}

void CwSpeedSet(unsigned int speed) {
  cw_speed = speed;
  EEPROM.put(CW_SPEED, cw_speed);
}

void CwToneSet(unsigned int tone) {
  cw_side_tone = tone;
  EEPROM.put(CW_SIDE_TONE, cw_side_tone);
}

void IambicKeySet(unsigned char key) {
  iambic_key = key;
  EEPROM.put(IAMBIC_KEY, iambic_key);

  if (iambic_key == 1)
    keyer_control &= ~0x10;  // IAMBICB bit
  if (iambic_key == 2)
    keyer_control |= 0x10;  // IAMBICB bit
}


void SidebandSet(char usb) {
  is_usb = usb;
  SetFrequency(frequency);
}

void VfoSwap(unsigned char save) {
  RitDisable();
  if (vfo_active == VFO_ACTIVE_A) {
    vfo_a = frequency;
    vfo_a_usb = is_usb;
    if (save) EEPROM.put(VFO_A, vfo_a);
    if (save) EEPROM.put(VFO_A_USB, vfo_a_usb);

    vfo_active = VFO_ACTIVE_B;
    frequency = vfo_b;
    is_usb = vfo_b_usb;
  } else {
    vfo_b = frequency;
    vfo_b_usb = is_usb;
    if (save) EEPROM.put(VFO_B, vfo_b);
    if (save) EEPROM.put(VFO_B_USB, vfo_b_usb);

    vfo_active = VFO_ACTIVE_A;
    frequency = vfo_a;
    is_usb = vfo_a_usb;
  }
  SetFrequency(frequency);
}

void SplitEnable() {
  shift_mode = SHIFT_SPLIT;
}

void SplitDisable() {
  if (shift_mode == SHIFT_SPLIT) {
    shift_mode = SHIFT_NONE;
  }
}

void SetUsbCarrier(unsigned long long carrier) {
  usb_carrier = carrier;
  EEPROM.put(USB_CARRIER, usb_carrier);

  si5351bx_setfreq(0, usb_carrier);
  SetFrequency(frequency);
}

/**
 * Basic User Interface Routines. These check the front panel for any activity
 */

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
 
  DoMenu();
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
void masterLoop() {
  CwKeyer(); 
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
  CheckCat();
}

Task master(TASK_IMMEDIATE, TASK_FOREVER, &masterLoop);

void ResetSettingsAndHalt() {
  master_cal = 154117;
  usb_carrier = 11056273l;
  cw_side_tone = 800;
  vfo_a = 3573000l;
  vfo_b = 7074000l;
  cw_speed = 100;
  vfo_a_usb = 1;
  vfo_b_usb = 1;
  iambic_key = 1;

  // Before type change
  // Sketch uses 18520 bytes (60%) of program storage space. Maximum is 30720 bytes.
  // Global variables use 1027 bytes (50%) of dynamic memory, leaving 1021 bytes for local variables. Maximum is 2048 bytes.
  // NEXT        18580 - 60 bytes more.. crazy..

  EEPROM.put(MASTER_CAL, master_cal);
  EEPROM.put(USB_CARRIER, usb_carrier);
  EEPROM.put(CW_SIDE_TONE, cw_side_tone);
  EEPROM.put(VFO_A, vfo_a);
  EEPROM.put(VFO_B, vfo_b);
  EEPROM.put(CW_SPEED, cw_speed);
  EEPROM.put(VFO_A_USB, vfo_a_usb);
  EEPROM.put(VFO_B_USB, vfo_b_usb);
  EEPROM.put(IAMBIC_KEY, iambic_key);

  char magicNr = MAGIC_NR;
  EEPROM.put(MAGIC_ADDR, magicNr);

  u8x8.clear();
  PrintLine(2, "EEPROM RESET");
  PrintLine(4, "TURN OFF POWER");
  while (1) {}
}

/**
 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void InitSettings() {
  char magicNr;

  EEPROM.get(MAGIC_ADDR, magicNr);
  if (magicNr != MAGIC_NR) {
    ResetSettingsAndHalt();
    return;
  }

  EEPROM.get(MASTER_CAL, master_cal);
  EEPROM.get(USB_CARRIER, usb_carrier);
  EEPROM.get(CW_SIDE_TONE, cw_side_tone);
  EEPROM.get(VFO_A, vfo_a);
  EEPROM.get(VFO_B, vfo_b);
  EEPROM.get(CW_SPEED, cw_speed);
  EEPROM.get(VFO_A_USB, vfo_a_usb);
  EEPROM.get(VFO_B_USB, vfo_b_usb);
  EEPROM.get(IAMBIC_KEY, iambic_key);

  // TODO - EEPROM
  first_if = 45005000L; // should be eeprom
  cw_delay_time = 60;
  tx_inhibit = 0;

  // calculate internal variables
  vfo_active = VFO_ACTIVE_A;
  is_usb = vfo_a_usb;
  shift_mode = SHIFT_NONE;

   if (iambic_key == 1)
     keyer_control &= ~0x10;
   else if (iambic_key == 2)
     keyer_control |= 0x10;
}

void InitPorts() {
  analogReference(DEFAULT);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(FBUTTON, INPUT_PULLUP);
  
  pinMode(PTT, INPUT_PULLUP);
  pinMode(ANALOG_V, INPUT_PULLUP);

  pinMode(CW_TONE, OUTPUT);  
  digitalWrite(CW_TONE, 0);
  
  pinMode(TX_RX, OUTPUT);
  digitalWrite(TX_RX, 0);

  pinMode(TX_LPF_A, OUTPUT);
  pinMode(TX_LPF_B, OUTPUT);
  pinMode(TX_LPF_C, OUTPUT);
  digitalWrite(TX_LPF_A, 0);
  digitalWrite(TX_LPF_B, 0);
  digitalWrite(TX_LPF_C, 0);

  pinMode(CW_KEY, OUTPUT);
  digitalWrite(CW_KEY, 0);

  pinMode(OLED_ENABLE, OUTPUT);
}

void setup() {
  Serial.begin(38400);
  Serial.flush();  
  
  u8x8.begin();
  // the "_f" version uses extra 1280 bytes of storage space
  // u8x8.setFont(u8x8_font_amstrad_cpc_extended_f); 
  u8x8.setFont(U8X8_MAINFONT); 
  u8x8.setPowerSave(0);

  //we print this line so this shows up even if the raduino 
  //crashes later in the code
  u8x8.drawString(1, 6, "UBITX V5.1"); 
  u8x8.drawString(1, 7, "YL3AME"); 

  InitSettings();
  InitPorts();     
  if (BtnDown())
    ResetSettingsAndHalt();

  InitEncoder();
  InitOscillators();

  frequency = vfo_a;
  SetFrequency(vfo_a);
  UpdateDisplay();

  scheduler.addTask(master);
  master.enable();
}

void loop() { 
  scheduler.execute();
}

