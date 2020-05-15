#ifndef HARDWARE_H_
#define HARDWARE_H_

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
#define ENC_A        (9)
#define ENC_B        (10)
#define FBUTTON      (A2)
#define PTT          (A3)
#define ANALOG_KEYER (A6)
#define ANALOG_V     (A7)
// A0 A1 Are original encoder pins, also usable
#define OLED_ENABLE  (8)

/** 
 *  The second set of 16 pins on the Raduino's bottom connector are have the three clock outputs and the digital lines to control the rig.
 *  This assignment is as follows :
 *    Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16
 *         GND +5V CLK0  GND  GND  CLK1 GND  GND  CLK2  GND  D2   D3   D4   D5   D6   D7  
 *  These too are flexible with what you may do with them, for the Raduino, we use them to :
 *  - TX_RX line : Switches between Transmit and Receive after sensing the PTT or the morse keyer
 *  - CW_KEY line : turns on the carrier for CW
 */
#define CW_KEY   (2)
#define TX_LPF_C (3)
#define TX_LPF_B (4)
#define TX_LPF_A (5)
#define CW_TONE  (6)
#define TX_RX    (7)

#endif  // HARDWARE_H_
