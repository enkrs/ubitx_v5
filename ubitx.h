#ifndef UBITX_H_
#define UBITX_H_

namespace ubitx {

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

// limits the tuning and working range of the ubitx between 1 MHz and 30 MHz
const unsigned long LOWEST_FREQ  =  1000000l;
const unsigned long HIGHEST_FREQ = 30000000l;

//we directly generate the CW by programming the Si5351 to the cw tx frequency, hence, both are different modes
//these are the parameter passed to TxStart
const int TX_SSB = 0;
const int TX_CW = 1;

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

const int SHIFT_NONE = 0;
const int SHIFT_RIT = 1;
const int SHIFT_SPLIT = 2;

const int VFO_ACTIVE_A = 0;
const int VFO_ACTIVE_B = 1;


extern int cw_delay_time;

extern struct Status {
  unsigned char shift_mode;
  bool vfo_a_active;
  bool is_usb;
  bool tx_inhibit;
} status;

extern struct Settings {
  long master_cal;
  unsigned long usb_carrier;
  unsigned int cw_side_tone;
  unsigned long vfo_a;
  unsigned long vfo_b;
  unsigned int cw_speed;
  bool vfo_a_usb;
  bool vfo_b_usb;
  unsigned char iambic_key;
} settings;

extern unsigned long frequency;

extern char in_tx;

// Utility functions
void ActiveDelay(unsigned int delay_by);

// General functions
void InitSettings();
void InitPorts();
void InitOscillators();
void ResetSettingsAndHalt();

// Radio functions
void RitDisable();
void RitEnable(unsigned long f);
void CwSpeedSet(unsigned int wpm);
void CwToneSet(unsigned int tone);
void SetFrequency(unsigned long f);
void SetUsbCarrier(unsigned long long carrier);
void SidebandSet(bool usb);
void IambicKeySet(unsigned char key);
void SplitDisable();
void SplitEnable();
void TxStart(char tx_mode);
void TxStop();
void VfoSwap(unsigned char save);

}  // namespace

#endif  // UBITX_H_
