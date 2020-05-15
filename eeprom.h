#ifndef EEPROM_H_
#define EEPROM_H_

/**
 * These are the indices where these user changable settinngs are stored  in the EEPROM
 */
#define MAGIC_NR   (0x1c)  // (Magic nr value)
#define MAGIC_ADDR    (0)  // EEPROM MAGIC NUMBER
#define MASTER_CAL    (1)  // ,2,3,4 long
#define USB_CARRIER   (5)  // ,6,7,8 unsigned long
#define CW_SIDE_TONE  (9)  // 10,11,12 unsigned int
#define VFO_A        (13)  // 14,15,16 unsigned long
#define VFO_B        (17)  // 18,19,20 unsigned long
#define CW_SPEED     (21)  // 22       int
#define VFO_A_USB    (23)  // char
#define VFO_B_USB    (24)  // char
#define IAMBIC_KEY   (25)  // char 


#endif  // EEPROM_H_

