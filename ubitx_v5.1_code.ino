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
#include <Wire.h>
#include <EEPROM.h>

/**
    The main chip which generates upto three oscillators of various frequencies in the
    Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet
    from www.silabs.com although, strictly speaking it is not a requirment to understand this code.

    We no longer use the standard SI5351 library because of its huge overhead due to many unused
    features consuming a lot of program space. Instead of depending on an external library we now use
    Jerry Gaffke's, KE7ER, lightweight standalone mimimalist "si5351bx" routines (see further down the
    code). Here are some defines and declarations used by Jerry's routines:
*/


/**
 * We need to carefully pick assignment of pin for various purposes.
 * There are two sets of completely programmable pins on the Raduino.
 * First, on the top of the board, in line with the LCD connector is an 8-pin connector
 * that is largely meant for analog inputs and front-panel control. It has a regulated 5v output,
 * ground and six pins. Each of these six pins can be individually programmed 
 * either as an analog input, a digital input or a digital output. 
 * The pins are assigned as follows (left to right, display facing you): 
 *      Pin 1 (Violet), A7, SPARE
 *      Pin 2 (Blue),   A6, KEYER (DATA)
 *      Pin 3 (Green), +5v 
 *      Pin 4 (Yellow), Gnd
 *      Pin 5 (Orange), A3, PTT
 *      Pin 6 (Red),    A2, F BUTTON
 *      Pin 7 (Brown),  A1, ENC B
 *      Pin 8 (Black),  A0, ENC A
 *Note: A5, A4 are wired to the Si5351 as I2C interface 
 *       *     
 * Though, this can be assigned anyway, for this application of the Arduino, we will make the following
 * assignment
 * A2 will connect to the PTT line, which is the usually a part of the mic connector
 * A3 is connected to a push button that can momentarily ground this line. This will be used for RIT/Bandswitching, etc.
 * A6 is to implement a keyer, it is reserved and not yet implemented
 * A7 is connected to a center pin of good quality 100K or 10K linear potentiometer with the two other ends connected to
 * ground and +5v lines available on the connector. This implments the tuning mechanism
 */

#define ENC_A (9)
#define ENC_B (10)
#define FBUTTON (A2)
#define PTT   (A3)
#define ANALOG_KEYER (A6)
#define ANALOG_SPARE (A7)
#define OLED_ENABLE (8)

/** 
 * The Raduino board is the size of a standard 16x2 LCD panel. It has three connectors:
 * 
 * First, is an 8 pin connector that provides +5v, GND and six analog input pins that can also be 
 * configured to be used as digital input or output pins. These are referred to as A0,A1,A2,
 * A3,A6 and A7 pins. The A4 and A5 pins are missing from this connector as they are used to 
 * talk to the Si5351 over I2C protocol. 
 * 
 * Second is a 16 pin LCD connector. This connector is meant specifically for the standard 16x2
 * LCD display in 4 bit mode. The 4 bit mode requires 4 data lines and two control lines to work:
 * Lines used are : RESET, ENABLE, D4, D5, D6, D7 
 * We include the library and declare the configuration of the LCD panel too
 */

#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ OLED_ENABLE);

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

/** 
 *  The second set of 16 pins on the Raduino's bottom connector are have the three clock outputs and the digital lines to control the rig.
 *  This assignment is as follows :
 *    Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16
 *         GND +5V CLK0  GND  GND  CLK1 GND  GND  CLK2  GND  D2   D3   D4   D5   D6   D7  
 *  These too are flexible with what you may do with them, for the Raduino, we use them to :
 *  - TX_RX line : Switches between Transmit and Receive after sensing the PTT or the morse keyer
 *  - CW_KEY line : turns on the carrier for CW
 */

#define TX_RX (7)
#define CW_TONE (6)
#define TX_LPF_A (5)
#define TX_LPF_B (4)
#define TX_LPF_C (3)
#define CW_KEY (2)

/**
 * These are the indices where these user changable settinngs are stored  in the EEPROM
 */
#define MAGIC_NR 0x1c    // (Magic nr value)
#define MAGIC_ADDR 0     // 00          EEPROM MAGIC NUMBER
#define MASTER_CAL 1     // 01 02 03 04 int32t
#define USB_CARRIER 5    // 05 06 07 08 unsigned long
#define CW_SIDE_TONE 9   // 09 10 11 12 uint16t
#define VFO_A 13         // 13 14 15 16 unsigned long
#define VFO_B 17         // 17 18 19 20 unsigned long
#define CW_SPEED 21      // 21 22       int
#define VFO_A_USB 23     // 23          char
#define VFO_B_USB 24     // 24          char
#define IAMBIC_KEY 25    // 25          char 


//values that are stroed for the VFO modes
#define VFO_MODE_LSB 2
#define VFO_MODE_USB 3

/**
 * The uBITX is an upconnversion transceiver. The first IF is at 45 MHz.
 * The first IF frequency is not exactly at 45 Mhz but about 5 khz lower,
 * this shift is due to the loading on the 45 Mhz crystal filter by the matching
 * L-network used on it's either sides.
 * The first oscillator works between 48 Mhz and 75 MHz. The signal is subtracted
 * from the first oscillator to arriive at 45 Mhz IF. Thus, it is inverted : LSB becomes USB
 * and USB becomes LSB.
 * The second IF of 12 Mhz has a ladder crystal filter. If a second oscillator is used at 
 * 57 Mhz, the signal is subtracted FROM the oscillator, inverting a second time, and arrives 
 * at the 12 Mhz ladder filter thus doouble inversion, keeps the sidebands as they originally were.
 * If the second oscillator is at 33 Mhz, the oscilaltor is subtracated from the signal, 
 * thus keeping the signal's sidebands inverted. The USB will become LSB.
 * We use this technique to switch sidebands. This is to avoid placing the lsbCarrier close to
 * 12 MHz where its fifth harmonic beats with the arduino's 16 Mhz oscillator's fourth harmonic
 */

// limits the tuning and working range of the ubitx between 3 MHz and 30 MHz
#define LOWEST_FREQ   (1000000l)
#define HIGHEST_FREQ (30000000l)

//we directly generate the CW by programmin the Si5351 to the cw tx frequency, hence, both are different modes
//these are the parameter passed to StartTx
#define TX_SSB 0
#define TX_CW 1

// These variables are stored in EEPROM:
int32_t master_cal;
unsigned long usb_carrier;
// 18490
uint16_t cw_side_tone;
unsigned long vfo_a;
unsigned long vfo_b;
int cw_speed;
char vfo_a_usb;
char vfo_b_usb;
uint8_t iambic_key;

int cw_delay_time;
char rit_on;
char is_usb;                   //upper sideband was selected, this is reset to the default for the 
unsigned char keyer_control;

unsigned long first_if;
char split_on;                 //working split, uses VFO B as the transmit frequency, (NOT IMPLEMENTED YET)
char vfo_active;

unsigned long frequency, rit_rx_frequency, rit_tx_frequency;  //frequency is the current frequency on the dial

#define IAMBICB 0x10 // 0 for Iambic A, 1 for Iambic B


/**
 * Raduino needs to keep track of current state of the transceiver. These are a few variables that do it
 */
uint8_t tx_cat = 0;            //turned on if the transmitting due to a CAT command
char in_tx = 0;                //it is set to 1 if in transmit mode (whatever the reason : cw, ptt or cat)
char key_down = 0;             //in cw mode, denotes the carrier is being transmitted
                              //frequency when it crosses the frequency border of 10 MHz
byte menu_state = 0;           //set to 2 when the menu is being displayed, if a menu item sets it to zero, the menu is exited
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
      //Background Work      
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
  }
  else if (freq >= 14000000L) { //thrown the KT1 relay on, the 30 MHz LPF is bypassed and the 14-18 MHz LPF is allowd to go through
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  }
  else if (freq > 7000000L) {
    digitalWrite(TX_LPF_A, 0);
    digitalWrite(TX_LPF_B, 1);
    digitalWrite(TX_LPF_C, 0);    
  }
  else {
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
  }
  else{
    si5351bx_setfreq(2, first_if + f);
    si5351bx_setfreq(1, first_if - usb_carrier);
  }
    
  frequency = f;
}

/**
 * StartTx is called by the PTT, cw keyer and CAT protocol to
 * put the uBitx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 * CW offest is calculated as lower than the operating frequency when in LSB mode, and vice versa in USB mode
 */
 
void StartTx(byte tx_mode) {
  digitalWrite(TX_RX, 1);
  in_tx = 1;
  
  if (rit_on) {
    //save the current as the rx frequency
    rit_rx_frequency = frequency;
    SetFrequency(rit_tx_frequency);
  }
  else 
  {
    if (split_on == 1) {
      if (vfo_active == VFO_B) {
        vfo_active = VFO_A;
        is_usb = vfo_a_usb;
        frequency = vfo_a;
      }
      else if (vfo_active == VFO_A) {
        vfo_active = VFO_B;
        frequency = vfo_b;
        is_usb = vfo_b_usb;        
      }
    }
    SetFrequency(frequency);
  }

  if (tx_mode == TX_CW) {
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

void StopTx() {
  in_tx = 0;

  digitalWrite(TX_RX, 0);           //turn off the tx
  si5351bx_setfreq(0, usb_carrier);  //set back the cardrier oscillator anyway, cw tx switches it off

  if (rit_on)
    SetFrequency(rit_rx_frequency);
  else{
    if (split_on == 1) {
      //vfo Change
      if (vfo_active == VFO_B) {
        vfo_active = VFO_A;
        frequency = vfo_a;
        is_usb = vfo_a_usb;        
      }
      else if (vfo_active == VFO_A) {
        vfo_active = VFO_B;
        frequency = vfo_b;
        is_usb = vfo_b_usb;        
      }
    }
    SetFrequency(frequency);
  }
  UpdateDisplay();
}

/**
 * ritEnable is called with a frequency parameter that determines
 * what the tx frequency will be
 */
void ritEnable(unsigned long f) {
  rit_on = 1;
  //save the non-rit frequency back into the VFO memory
  //as RIT is a temporary shift, this is not saved to EEPROM
  rit_tx_frequency = f;
}

// this is called by the RIT menu routine
void RitDisable() {
  if (rit_on) {
    rit_on = 0;
    SetFrequency(rit_tx_frequency);
    UpdateDisplay();
  }
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
    
  if (digitalRead(PTT) == 0 && in_tx == 0) {
    StartTx(TX_SSB);
    ActiveDelay(50); //debounce the PTT
  }
	
  if (digitalRead(PTT) == 1 && in_tx == 1)
    StopTx();
}

void CheckButton() {
  //only if the button is pressed
  if (!BtnDown())
    return;
  ActiveDelay(50);
  if (!BtnDown()) //debounce
    return;
 
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

    if (prev_freq < 10000000l && frequency > 10000000l) // TODO: BUG HERE ON FEQ EXACT 1000000..
      is_usb = 1;
      
    if (prev_freq > 10000000l && frequency < 10000000l)
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
  unsigned long old_freq = frequency;

  if (knob < 0)
    frequency -= 100l;
  else if (knob > 0)
    frequency += 100;
 
  if (old_freq != frequency) {
    SetFrequency(frequency);
    UpdateDisplay();
  }
}



void ResetSettings() {
  master_cal = 154117;
  usb_carrier = 11056273l;
  cw_side_tone = 800;
  vfo_a = 3573000l;
  vfo_b = 7074000l;
  cw_speed = 100;
  vfo_a_usb = 1;
  vfo_b_usb = 1;
  iambic_key = 1;

  EEPROM.put(MASTER_CAL, master_cal);
  EEPROM.put(USB_CARRIER, usb_carrier);
  EEPROM.put(CW_SIDE_TONE, cw_side_tone);
  EEPROM.put(VFO_A, vfo_a);
  EEPROM.put(VFO_B, vfo_b);
  EEPROM.put(CW_SPEED, cw_speed);
  EEPROM.put(VFO_A_USB, vfo_a_usb);
  EEPROM.put(VFO_B_USB, vfo_b_usb);
  EEPROM.put(IAMBIC_KEY, iambic_key);

  unsigned char magicNr = MAGIC_NR;
  EEPROM.put(MAGIC_ADDR, magicNr);
}

/**
 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void InitSettings() {
  unsigned char magicNr;

  EEPROM.get(MAGIC_ADDR, magicNr);
  if (magicNr != MAGIC_NR) {
    ResetSettings();
    u8x8.draw1x2String(1,6,"EEPROM RESET   ");
  } else {
    EEPROM.get(MASTER_CAL, master_cal);
    EEPROM.get(USB_CARRIER, usb_carrier);
    EEPROM.get(CW_SIDE_TONE, cw_side_tone);
    EEPROM.get(VFO_A, vfo_a);
    EEPROM.get(VFO_B, vfo_b);
    EEPROM.get(CW_SPEED, cw_speed);
    EEPROM.get(VFO_A_USB, vfo_a_usb);
    EEPROM.get(VFO_B_USB, vfo_b_usb);
    EEPROM.get(IAMBIC_KEY, iambic_key);
  }

  // TODO - EEPROM
  first_if = 45005000L; // should be eeprom
  cw_delay_time = 60;

  // calculate internal variables
  vfo_active = VFO_A;
  is_usb = vfo_a_usb;
  rit_on = 0;

   if (iambic_key == 1)
     keyer_control &= ~IAMBICB;
   else if (iambic_key == 2)
     keyer_control |= IAMBICB;
}

void InitPorts() {

  analogReference(DEFAULT);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(FBUTTON, INPUT_PULLUP);
  
  //configure the function button to use the external pull-up
//  pinMode(FBUTTON, INPUT);
//  digitalWrite(FBUTTON, HIGH);

  pinMode(PTT, INPUT_PULLUP);
  //YL3AME:pinMode(ANALOG_KEYER, INPUT_PULLUP);

  pinMode(CW_TONE, OUTPUT);  
  digitalWrite(CW_TONE, 0);
  
  pinMode(TX_RX,OUTPUT);
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

void setup()
{
  Serial.begin(38400);
  Serial.flush();  
  
  u8x8.begin();
  // the "_f" version uses extra 1280 bytes of storage space
  // u8x8.setFont(u8x8_font_amstrad_cpc_extended_f); 
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_u); 
  u8x8.setPowerSave(0);

  //we print this line so this shows up even if the raduino 
  //crashes later in the code
  u8x8.drawString(1,6,"UBITX V5.1"); 
  u8x8.drawString(1,7,"YL3AME"); 

  InitSettings();
  InitPorts();     
  initOscillators();

  frequency = vfo_a;
  SetFrequency(vfo_a);
  UpdateDisplay();
}


/**
 * The loop checks for key_down, ptt, function button and tuning.
 */

void loop() { 
  
  CwKeyer(); 
  if (!tx_cat)
    CheckPtt();
  CheckButton();

  //tune only when not tranmsitting 
  if (!in_tx) {
    if (rit_on)
      DoRit();
    else 
      DoTuning();
  }
  
  //we check CAT after the encoder as it might put the radio into TX
  CheckCat();
}
