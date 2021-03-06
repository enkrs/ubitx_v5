/**
 CW Keyer
 CW Key logic change with ron's code (ubitx_keyer.cpp)
 Ron's logic has been modified to work with the original uBITX by KD8CEC

 Original Comment ----------------------------------------------------------------------------
 * The CW keyer handles either a straight key or an iambic / paddle key.
 * They all use just one analog input line. This is how it works.
 * The analog line has the internal pull-up resistor enabled. 
 * When a straight key is connected, it shorts the pull-up resistor, analog input is 0 volts
 * When a paddle is connected, the dot and the dash are connected to the analog pin through
 * a 10K and a 2.2K resistors. These produce a 4v and a 2v input to the analog pins.
 * So, the readings are as follows :
 * 0v - straight key
 * 1-2.5 v - paddle dot
 * 2.5 to 4.5 v - paddle dash
 * 2.0 to 0.5 v - dot and dash pressed
 * 
 * The keyer is written to transparently handle all these cases
 * 
 * Generating CW
 * The CW is cleanly generated by unbalancing the front-end mixer
 * and putting the local oscillator directly at the CW transmit frequency.
 * The sidetone, generated by the Arduino is injected into the volume control
 */

#include "keyer.h"
#include <Arduino.h>
#include "hw.h"
#include "ubitx.h"

namespace keyer {

char key_down = 0;  //in cw mode, denotes the carrier is being transmitted
                    //frequency when it crosses the frequency border of 10 MHz
char keyer_control;

unsigned long cw_timeout = 0;  //milliseconds to go before the cw transmit line is released and the radio goes back to rx mode

 //CW ADC Range
int cw_adc_st_from = 0;
int cw_adc_st_to = 50;
int cw_adc_both_from = 51;
int cw_adc_both_to = 300;
int cw_adc_dot_from = 301;
int cw_adc_dot_to = 600;
int cw_adc_dash_from = 601;
int cw_adc_dash_to = 800;

char delay_before_cw_start_time = 50;

// in milliseconds, this is the parameter that determines how long the tx will hold between cw key downs
#define PADDLE_DOT 1
#define PADDLE_DASH 2
#define PADDLE_BOTH 3
#define PADDLE_STRAIGHT 4

/**
 * Starts transmitting the carrier with the sidetone
 * It assumes that we have called cwTxStart and not called cwTxStop
 * each time it is called, the cwTimeOut is pushed further into the future
 */
void CwKeydown() {
  key_down = 1;  //tracks the CW_KEY
  tone(hw::CW_TONE, ubitx::settings.cw_side_tone); 
  digitalWrite(hw::CW_KEY, 1);

  cw_timeout = millis() + ubitx::settings.cw_delay_time * 10;  
}

/**
 * Stops the cw carrier transmission along with the sidetone
 * Pushes the cw_timeout further into the future
 */
void CwKeyUp() {
  key_down = 0;
  noTone(hw::CW_TONE);
  digitalWrite(hw::CW_KEY, 0);
  
  //Modified by KD8CEC, for CW Delay Time save to eeprom
  cw_timeout = millis() + ubitx::settings.cw_delay_time * 10;
}

//Variables for Ron's new logic
#define DIT_L 0x01 // DIT latch
#define DAH_L 0x02 // DAH latch
#define DIT_PROC 0x04 // DIT is being processed
#define PDLSWAP 0x08 // 0 for normal, 1 for swap
#define IAMBICB 0x10 // 0 for Iambic A, 1 for Iambic B
enum KSTYPE {IDLE, CHK_DIT, CHK_DAH, KEYED_PREP, KEYED, INTER_ELEMENT };
static unsigned long ktimer;
char keyerState = IDLE;

//Below is a test to reduce the keying error. do not delete lines
//create by KD8CEC for compatible with new CW Logic
char UpdatePaddleLatch(char isUpdateKeyState) {
  char tmp_keyer_control = 0;
  
  // int paddle = analogRead(ANALAG_KEYER);
  int paddle = 801;  // always off
  // int paddle = digitalRead(PTT) == LOW ? 25 : 801;  // Emulate paddle with PTT button

  if (paddle >= cw_adc_dash_from && paddle <= cw_adc_dash_to)
    tmp_keyer_control |= DAH_L;
  else if (paddle >= cw_adc_dot_from && paddle <= cw_adc_dot_to)
    tmp_keyer_control |= DIT_L;
  else if (paddle >= cw_adc_both_from && paddle <= cw_adc_both_to)
    tmp_keyer_control |= (DAH_L | DIT_L) ;     
  else 
  {
    if (ubitx::settings.iambic_key)
      tmp_keyer_control = 0 ;
    else if (paddle >= cw_adc_st_from && paddle <= cw_adc_st_to)
      tmp_keyer_control = DIT_L ;
     else
       tmp_keyer_control = 0 ; 
  }
  
  if (isUpdateKeyState == 1)
    keyer_control |= tmp_keyer_control;

  return tmp_keyer_control;
}

/*****************************************************************************
// New logic, by RON
// modified by KD8CEC
******************************************************************************/
void CwKeyerIambic() {
  char tmp_keyer_control = 0;

  while (1) {
    switch (keyerState) {
      case IDLE:
        tmp_keyer_control = UpdatePaddleLatch(0);
        if (tmp_keyer_control == DAH_L ||
            tmp_keyer_control == DIT_L || 
            tmp_keyer_control == (DAH_L | DIT_L) ||
            (keyer_control & 0x03)) {
            UpdatePaddleLatch(1);
            keyerState = CHK_DIT;
        } else {
          if (0 < cw_timeout && cw_timeout < millis()) {
            cw_timeout = 0;
            ubitx::TxStop();
          }
          return;
        }
        break;
      case CHK_DIT:
        if (keyer_control & DIT_L) {
          keyer_control |= DIT_PROC;
          ktimer = ubitx::settings.cw_speed;
          keyerState = KEYED_PREP;
        } else {
          keyerState = CHK_DAH;
        }
        break;
      case CHK_DAH:
        if (keyer_control & DAH_L) {
          ktimer = ubitx::settings.cw_speed * 3;
          keyerState = KEYED_PREP;
        } else {
          keyerState = IDLE;
        }
        break;
      case KEYED_PREP:
        if (!ubitx::in_tx) {
          ubitx::ActiveDelay(delay_before_cw_start_time * 2);
          
          key_down = 0;
          cw_timeout = millis() + ubitx::settings.cw_delay_time * 10;
          ubitx::TxStartCw();
        }
        ktimer += millis();  // set ktimer to interval end time
        keyer_control &= ~(DIT_L + DAH_L);  // clear both paddle latch bits
        keyerState = KEYED;  // next state
        
        CwKeydown();
        break;
      case KEYED:
        if (millis() > ktimer) {  // are we at end of key down ?
          CwKeyUp();
          ktimer = millis() + ubitx::settings.cw_speed;  // inter-element time
          keyerState = INTER_ELEMENT;  // next state
        } else if (keyer_control & IAMBICB) {
          UpdatePaddleLatch(1);  // early paddle latch in Iambic B mode
        }
        break;
      case INTER_ELEMENT:  // Insert time between dits/dahs
        UpdatePaddleLatch(1);  // latch paddle state
        if (millis() > ktimer) {  // are we at end of inter-space ?
          if (keyer_control & DIT_PROC) {  // was it a dit or dah ?
            keyer_control &= ~(DIT_L + DIT_PROC);  // clear two bits
            keyerState = CHK_DAH;  // dit done, check for dah
          } else {
            keyer_control &= ~(DAH_L);  // clear dah latch
            keyerState = IDLE;  // go idle
          }
        }
        break;
    }
  }
}

void CwKeyerStraight() {
  while(1) {
    if (UpdatePaddleLatch(0) == DIT_L) {
      // if we are here, it is only because the key is pressed
      if (!ubitx::in_tx) {
        //DelayTime Option
        ubitx::ActiveDelay(delay_before_cw_start_time * 2);
        
        key_down = 0;
        cw_timeout = millis() + ubitx::settings.cw_delay_time * 10;
        ubitx::TxStartCw();
      }
      CwKeydown();
      
      while (UpdatePaddleLatch(0) == DIT_L) {}
        
      CwKeyUp();
    } else {
      if (0 < cw_timeout && cw_timeout < millis()) {
        cw_timeout = 0;
        key_down = 0;
        ubitx::TxStop();
      }
      return;                   //Tx stop control by Main Loop
    }
  }
}

void Run() {
  if (ubitx::settings.iambic_key) CwKeyerIambic();
  else CwKeyerStraight();
}

}  // namespace
