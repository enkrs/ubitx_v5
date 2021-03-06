#ifndef HARDWARE_H_
#define HARDWARE_H_

#define PC2_FBUTTON (1<<PC2)
#define PC3_PTT     (1<<PC3)

namespace hw {

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
const int OLED_ENABLE  =  8; // PORTB 0 output PB0
const int ENC_A        =  9; // PINB 1 input PB1
const int ENC_B        = 10; // PINB 2 input PB2

const int FBUTTON      = A2; // PINC 2 input PC2
const int PTT          = A3; // PINC 3 input PC3
const int ANALOG_KEYER = A6; // PINC 6 input PC6
const int ANALOG_V     = A7; // PINC 7 input PC7

// examples
// PORTD &= ~(1<<PD4 | 1<<PD5);  // A, B off
// PORTD |= 1<<PD3;              // C on

// A0 A1 Are original encoder pins, also usable

/** 
 *  The second set of 16 pins on the Raduino's bottom connector are have the three clock outputs and the digital lines to control the rig.
 *  This assignment is as follows :
 *    Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16
 *         GND +5V CLK0  GND  GND  CLK1 GND  GND  CLK2  GND  D2   D3   D4   D5   D6   D7  
 *  These too are flexible with what you may do with them, for the Raduino, we use them to :
 *  - TX_RX line : Switches between Transmit and Receive after sensing the PTT or the morse keyer
 *  - CW_KEY line : turns on the carrier for CW
 */
const int CW_KEY   = 2; // PORTD 2 output PD2
const int TX_LPF_C = 3; // PORTD 3 output PD3
const int TX_LPF_B = 4; // PORTD 4 output PD4
const int TX_LPF_A = 5; // PORTD 5 output PD5
const int CW_TONE  = 6; // PORTD 6 output PD6
const int TX_RX    = 7; // PORTD 7 output PD7

}  // namespace

#endif  // HARDWARE_H_
