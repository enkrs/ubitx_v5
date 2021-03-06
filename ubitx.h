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

extern struct Status {
  unsigned char shift_mode; // 0 none, 1 rit, 2 split
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
  unsigned char iambic_key; // 0 stright, 1 a, 2 b
  int cw_delay_time;
} settings;

// Size before this struct/array : 20576 / 1090
// Size after                      20874 / 1307 .. thats a lot
struct BandList {
  char name[5];
  unsigned int min_khz;
  unsigned int max_khz;
};

const BandList BAND_LIST[] = {
  {"160M",  1810,  2000},
  {" 80M",  3500,  3800},
  {" 60M",  5351,  5367},
  {" 40M",  7000,  7200},
  {" 30M", 10100, 10150},
  {" 20M", 14000, 14350},
  {" 17M", 18068, 18168},
  {" 15M", 21000, 21450},
  {" 12M", 24890, 24990},
  {"  CB", 26953, 27417},
  {" 10M", 28000, 29700},
  {"    ",     0, 30000}
};
extern unsigned char active_band;

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
void CwDelayTimeSet(unsigned int delay_time);
void SetFrequency(unsigned long f);
void SetUsbCarrier(unsigned long long carrier);
void SetMasterCal(long int cal);
void SidebandSet(bool usb);
void IambicKeySet(unsigned char key);
void SplitDisable();
void SplitEnable();
void TxStartSsb();
void TxStartCw();
void TxStop();
void VfoSwap(bool save);
void VfoCopy(bool save);

}  // namespace

#endif  // UBITX_H_
