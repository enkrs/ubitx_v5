#ifndef EEPROM_H_
#define EEPROM_H_

namespace eeprom {

/**
 * These are the indices where these user changable settinngs are stored  in the EEPROM
 */
const unsigned char MAGIC_NR = 0x1d;  // (Magic nr value)
const int MAGIC_ADDR =     0;  // EEPROM MAGIC NUMBER
const int MASTER_CAL =     1;  // ,2,3,4 long
const int USB_CARRIER =    5;  // ,6,7,8 unsigned long
const int CW_SIDE_TONE =   9;  // 10,11,12 unsigned int
const int VFO_A =         13;  // 14,15,16 unsigned long
const int VFO_B =         17;  // 18,19,20 unsigned long
const int CW_SPEED =      21;  // 22       int
const int VFO_A_USB =     23;  // char
const int VFO_B_USB =     24;  // char
const int IAMBIC_KEY =    25;  // char 
const int CW_DELAY_TIME = 26;  // char 
}  // namespace

#endif  // EEPROM_H_

