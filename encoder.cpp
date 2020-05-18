#include "encoder.h"
#include <Arduino.h>
#include "hw.h"

namespace encoder {
// Code adapted from https://github.com/brianlow/Rotary

// Normal encoder state
volatile char enc_count = 0;

// Values returned by 'process'
#define DIR_NONE 0x0
#define DIR_CW 0x10
#define DIR_CCW 0x20

#define R_START 0x0
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5

// Internal state
unsigned char state = R_START;

const unsigned char ttable[6][4] = {
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START}, // R_START (00)
  {R_START_M | DIR_CCW,  R_START,        R_CCW_BEGIN,  R_START}, // R_CCW_BEGIN
  {R_START_M | DIR_CW,   R_CW_BEGIN,     R_START,      R_START}, // R_CW_BEGIN
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START}, // R_START_M (11)
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW}, // R_CW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW}, // R_CCW_BEGIN_M
};

/*
 * SmittyHalibut's encoder handling, using interrupts. Should be quicker, smoother handling.
 * The Interrupt Service Routine for Pin Change Interrupts on A0-A5.
 */
ISR(PCINT0_vect) {
  unsigned char pinstate = (PINB & 0b00000110) >> 1;

  state = ttable[state & 0xf][pinstate];
  switch (state & 0x30) {
    case DIR_CW: enc_count -= 1; break;
    case DIR_CCW: enc_count += 1; break;
  }
}

/*
 * Setup the encoder interrupts and global variables.
 */
void PciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR |= bit(digitalPinToPCICRbit(pin));  // clear any outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(pin));  // enable interrupt for the group
}

void Init(void) {
  // Setup Pin Change Interrupts for the encoder inputs
  PciSetup(hw::ENC_A);
  PciSetup(hw::ENC_B);
}

int Read() {
  int ret = enc_count;
  enc_count = 0;
  return ret;
}

int ReadSlow() {
  int ret = enc_count / 2;
  if (ret) enc_count = 0;
  return ret;
}

} // namespace
