/**
 * The CAT protocol is used by many radios to provide remote control to comptuers through
 * the serial port.
 * 
 * This is very much a work in progress. Parts of this code have been liberally
 * borrowed from other GPLicensed works like hamlib.
 * 
 * WARNING : This is an unstable version and it has worked with fldigi, 
 * it gives time out error with WSJTX 1.8.0  
 */

static unsigned long rx_buffer_arrive_time = 0;
static byte rx_buffer_check_count = 0;
#define CAT_RECEIVE_TIMEOUT 500
static byte cat[5]; 
static byte inside_cat = 0; 

//for broken protocol
#define CAT_RECEIVE_TIMEOUT 500

#define CAT_MODE_LSB            0x00
#define CAT_MODE_USB            0x01
#define CAT_MODE_CW             0x02
#define CAT_MODE_CWR            0x03
#define CAT_MODE_AM             0x04
#define CAT_MODE_FM             0x08
#define CAT_MODE_DIG            0x0A
#define CAT_MODE_PKT            0x0C
#define CAT_MODE_FMN            0x88

#define ACK 0

byte SetHighNibble(byte b,byte v) {
  // Clear the high nibble
  b &= 0x0f;
  // Set the high nibble
  return b | ((v & 0x0f) << 4);
}

byte SetLowNibble(byte b,byte v) {
  // Clear the low nibble
  b &= 0xf0;
  // Set the low nibble
  return b | (v & 0x0f);
}

byte GetHighNibble(byte b) {
  return (b >> 4) & 0x0f;
}

byte GetLowNibble(byte b) {
  return b & 0x0f;
}

// Takes a number and produces the requested number of decimal digits, staring
// from the least significant digit.  
//
void GetDecimalDigits(unsigned long number,byte* result,int digits) {
  for (int i = 0; i < digits; i++) {
    // "Mask off" (in a decimal sense) the LSD and return it
    result[i] = number % 10;
    // "Shift right" (in a decimal sense)
    number /= 10;
  }
}

// Takes a frequency and writes it into the CAT command buffer in BCD form.
//
void WriteFreq(unsigned long freq,byte* cmd) {
  // Convert the frequency to a set of decimal digits. We are taking 9 digits
  // so that we can get up to 999 MHz. But the protocol doesn't care about the
  // LSD (1's place), so we ignore that digit.
  byte digits[9];
  GetDecimalDigits(freq,digits,9);
  // Start from the LSB and get each nibble 
  cmd[3] = SetLowNibble(cmd[3],digits[1]);
  cmd[3] = SetHighNibble(cmd[3],digits[2]);
  cmd[2] = SetLowNibble(cmd[2],digits[3]);
  cmd[2] = SetHighNibble(cmd[2],digits[4]);
  cmd[1] = SetLowNibble(cmd[1],digits[5]);
  cmd[1] = SetHighNibble(cmd[1],digits[6]);
  cmd[0] = SetLowNibble(cmd[0],digits[7]);
  cmd[0] = SetHighNibble(cmd[0],digits[8]);  
}

// This function takes a frquency that is encoded using 4 bytes of BCD
// representation and turns it into an long measured in Hz.
//
// [12][34][56][78] = 123.45678? Mhz
//
unsigned long ReadFreq(byte* cmd) {
    // Pull off each of the digits
    byte d7 = GetHighNibble(cmd[0]);
    byte d6 = GetLowNibble(cmd[0]);
    byte d5 = GetHighNibble(cmd[1]);
    byte d4 = GetLowNibble(cmd[1]); 
    byte d3 = GetHighNibble(cmd[2]);
    byte d2 = GetLowNibble(cmd[2]); 
    byte d1 = GetHighNibble(cmd[3]);
    byte d0 = GetLowNibble(cmd[3]); 
    return  
      (unsigned long)d7 * 100000000L +
      (unsigned long)d6 * 10000000L +
      (unsigned long)d5 * 1000000L + 
      (unsigned long)d4 * 100000L + 
      (unsigned long)d3 * 10000L + 
      (unsigned long)d2 * 1000L + 
      (unsigned long)d1 * 100L + 
      (unsigned long)d0 * 10L; 
}

//void ReadEEPRom_FT817(byte fromType)
void CatReadEeprom(void)
{
  //for remove warnings
  byte temp0 = cat[0];
  byte temp1 = cat[1];
/*
  itoa((int) cat[0], b, 16);
  strcat(b, ":");
  itoa((int) cat[1], c, 16);
  strcat(b, c);
  PrintStatus(b);
*/

  cat[0] = 0;
  cat[1] = 0;
  //for remove warnings[1] = 0;
  
  switch (temp1)
  {
    case 0x45 : //
      if (temp0 == 0x03)
      {
        cat[0] = 0x00;
        cat[1] = 0xD0;
      }
      break;
    case 0x47 : //
      if (temp0 == 0x03)
      {
        cat[0] = 0xDC;
        cat[1] = 0xE0;
      }
      break;
    case 0x55 :
      //0 : VFO A/B  0 = VFO-A, 1 = VFO-B
      //1 : MTQMB Select  0 = (Not MTQMB), 1 = MTQMB ("Memory Tune Quick Memory Bank")
      //2 : QMB Select  0 = (Not QMB), 1 = QMB  ("Quick Memory Bank")
      //3 :
      //4 : Home Select  0 = (Not HOME), 1 = HOME memory
      //5 : Memory/MTUNE select  0 = Memory, 1 = MTUNE
      //6 :
      //7 : MEM/VFO Select  0 = Memory, 1 = VFO (A or B - see bit 0)
      cat[0] = 0x80 + (vfo_active == VFO_B ? 1 : 0);
      cat[1] = 0x00;
      break;
    case 0x57 : //
      //0 : 1-0  AGC Mode  00 = Auto, 01 = Fast, 10 = Slow, 11 = Off
      //2  DSP On/Off  0 = Off, 1 = On  (Display format)
      //4  PBT On/Off  0 = Off, 1 = On  (Passband Tuning)
      //5  NB On/Off 0 = Off, 1 = On  (Noise Blanker)
      //6  Lock On/Off 0 = Off, 1 = On  (Dial Lock)
      //7  FST (Fast Tuning) On/Off  0 = Off, 1 = On  (Fast tuning)

      cat[0] = 0xC0;
      cat[1] = 0x40;
      break;
    case 0x59 : //  band select VFO A Band Select  0000 = 160 M, 0001 = 75 M, 0010 = 40 M, 0011 = 30 M, 0100 = 20 M, 0101 = 17 M, 0110 = 15 M, 0111 = 12 M, 1000 = 10 M, 1001 = 6 M, 1010 = FM BCB, 1011 = Air, 1100 = 2 M, 1101 = UHF, 1110 = (Phantom)
      //http://www.ka7oei.com/ft817_memmap.html
      //CAT_BUFF[0] = 0xC2;
      //CAT_BUFF[1] = 0x82;
      break;
    case 0x5C : //Beep Volume (0-100) (#13)
      cat[0] = 0xB2;
      cat[1] = 0x42;
      break;
    case 0x5E :
      //3-0 : CW Pitch (300-1000 Hz) (#20)  From 0 to E (HEX) with 0 = 300 Hz and each step representing 50 Hz
      //5-4 :  Lock Mode (#32) 00 = Dial, 01 = Freq, 10 = Panel
      //7-6 :  Op Filter (#38) 00 = Off, 01 = SSB, 10 = CW
      //CAT_BUFF[0] = 0x08;
      cat[0] = (cw_side_tone - 300)/50;
      cat[1] = 0x25;
      break;
    case 0x61 : //Sidetone (Volume) (#44)
      cat[0] = cw_side_tone % 50;
      cat[1] = 0x08;
      break;
    case  0x5F : //
      //4-0  CW Weight (1.:2.5-1:4.5) (#22)  From 0 to 14 (HEX) with 0 = 1:2.5, incrementing in 0.1 weight steps
      //5  420 ARS (#2)  0 = Off, 1 = On
      //6  144 ARS (#1)  0 = Off, 1 = On
      //7  Sql/RF-G (#45)  0 = Off, 1 = On
      cat[0] = 0x32;
      cat[1] = 0x08;
      break;
    case 0x60 : //CW Delay (10-2500 ms) (#17)  From 1 to 250 (decimal) with each step representing 10 ms
      cat[0] = cw_delay_time;
      cat[1] = 0x32;
      break;
    case 0x62 : //
      //5-0  CW Speed (4-60 WPM) (#21) From 0 to 38 (HEX) with 0 = 4 WPM and 38 = 60 WPM (1 WPM steps)
      //7-6  Batt-Chg (6/8/10 Hours (#11)  00 = 6 Hours, 01 = 8 Hours, 10 = 10 Hours
      //CAT_BUFF[0] = 0x08;
      cat[0] = 1200 / cw_speed - 4;
      cat[1] = 0xB2;
      break;
    case 0x63 : //
      //6-0  VOX Gain (#51)  Contains 1-100 (decimal) as displayed
      //7  Disable AM/FM Dial (#4) 0 = Enable, 1 = Disable
      cat[0] = 0xB2;
      cat[1] = 0xA5;
      break;
    case 0x64 : //
      break;
    case 0x67 : //6-0  SSB Mic (#46) Contains 0-100 (decimal) as displayed
      cat[0] = 0xB2;
      cat[1] = 0xB2;
      break;      case 0x69 : //FM Mic (#29)  Contains 0-100 (decimal) as displayed
    case 0x78 :
      if (is_usb)
        cat[0] = CAT_MODE_USB;
      else
        cat[0] = CAT_MODE_LSB;
        
      if (cat[0] != 0) cat[0] = 1 << 5;
      break;
    case  0x79 : //
      //1-0  TX Power (All bands)  00 = High, 01 = L3, 10 = L2, 11 = L1
      //3  PRI On/Off  0 = Off, 1 = On
      //DW On/Off  0 = Off, 1 = On
      //SCN (Scan) Mode  00 = No scan, 10 = Scan up, 11 = Scan down
      //ART On/Off  0 = Off, 1 = On
      cat[0] = 0x00;
      cat[1] = 0x00;
      break;
    case 0x7A : //SPLIT
      //7A  0 HF Antenna Select 0 = Front, 1 = Rear
      //7A  1 6 M Antenna Select  0 = Front, 1 = Rear
      //7A  2 FM BCB Antenna Select 0 = Front, 1 = Rear
      //7A  3 Air Antenna Select  0 = Front, 1 = Rear
      //7A  4 2 M Antenna Select  0 = Front, 1 = Rear
      //7A  5 UHF Antenna Select  0 = Front, 1 = Rear
      //7A  6 ? ?
      //7A  7 SPL On/Off  0 = Off, 1 = On

      cat[0] = (split_on ? 0xFF : 0x7F);
      break;
    case 0xB3 : //
      cat[0] = 0x00;
      cat[1] = 0x4D;
      break;

  }

  // sent the data
  Serial.write(cat, 2);
}

void ProcessCatCommand(byte* cmd) {
  byte response[5];
  unsigned long f;

  switch(cmd[4]){
/*  case 0x00:
    response[0]=0;
    Serial.write(response, 1);
    break;
*/    
  case 0x01:
    //set frequency
    f = ReadFreq(cmd);
    SetFrequency(f);   
    UpdateDisplay();
    response[0]=0;
    Serial.write(response, 1);
    //sprintf(b, "set:%ld", f); 
    //PrintStatus(b);
    break;

  case 0x02:
    //split on 
    split_on =  1;
    break;
  case 0x82:
    //split off
    split_on = 0;
    break;
    
  case 0x03:
    WriteFreq(frequency,response); // Put the frequency into the buffer
    if (is_usb)
      response[4] = 0x01; //USB
    else
      response[4] = 0x00; //LSB
    Serial.write(response,5);
    //PrintStatus("cat:getfreq");
    break;
    
  case 0x07: // set mode
    if (cmd[0] == 0x00 || cmd[0] == 0x03)
      is_usb = 0;
    else
      is_usb = 1;
    response[0] = 0x00;
    Serial.write(response, 1);
    SetFrequency(frequency);
      //PrintStatus("cat: mode changed");
    //UpdateDisplay();
    break;   
 
  case 0x08: // PTT On
    if (!in_tx) {
      response[0] = 0;
      tx_cat = 1;
      StartTx(TX_SSB);
      UpdateDisplay();
    } else {
      response[0] = 0xf0;
    } 
    Serial.write(response,1);
    UpdateDisplay();
    break;

  case 0x88 : //PTT OFF
    if (in_tx) {
      StopTx();
      tx_cat = 0;
    }
    response[0] = 0;
    Serial.write(response,1);
    UpdateDisplay();
    break;

  case 0x81:
    //toggle the VFOs
    response[0] = 0;
    MenuVfoToggle(1); // '1' forces it to change the VFO
    Serial.write(response,1);
    UpdateDisplay();
    break;

 case 0xBB:  //Read FT-817 EEPROM Data  (for comfirtable)
    CatReadEeprom();
    break;

  case 0xe7 : 
    // get receiver status, we have hardcoded this as
    //as we dont' support ctcss, etc.
    response[0] = 0x09;
    Serial.write(response,1);
    break;
       
  case 0xf7:
    {
      uint8_t isHighSWR = 0;
      uint8_t isSplitOn = 0;
  
      /*
        Inverted -> *ptt = ((p->tx_status & 0x80) == 0); <-- souce code in ft817.c (hamlib)
      */
      response[0] = ((in_tx ? 0 : 1) << 7) +
        ((isHighSWR ? 1 : 0) << 6) +  //hi swr off / on
        ((isSplitOn ? 1 : 0) << 5) + //Split on / off
        (0 << 4) +  //dummy data
        0x08;  //P0 meter data

      Serial.write(response, 1);
    }
    break;
    
  default:
    //somehow, get this to print the four bytes
    ultoa(*((unsigned long *)cmd), c, 16);
    itoa(cmd[4], b, 16);
    strcat(b, ">");
    strcat(b, c);
    u8x8.drawString(1,5,b);
    response[0] = 0x00;
    Serial.write(response[0]);
  }

  inside_cat = 0;
}

int catCount = 0;
void CheckCat()
{
  byte i;

  //Check Serial Port Buffer
  if (Serial.available() == 0) {      //Set Buffer Clear status
    rx_buffer_check_count = 0;
    return;
  } else if (Serial.available() < 5) {                         //First Arrived
    if (rx_buffer_check_count == 0){
      rx_buffer_check_count = Serial.available();
      rx_buffer_arrive_time = millis() + CAT_RECEIVE_TIMEOUT;  //Set time for timeout
    } else if (rx_buffer_arrive_time < millis()){                //Clear Buffer
      for (i = 0; i < Serial.available(); i++)
        rx_buffer_check_count = Serial.read();
      rx_buffer_check_count = 0;
    } else if (rx_buffer_check_count < Serial.available()){      // Increase buffer count, slow arrive
      rx_buffer_check_count = Serial.available();
      rx_buffer_arrive_time = millis() + CAT_RECEIVE_TIMEOUT;  //Set time for timeout
    }
    return;
  }

    
  //Arived CAT DATA
  for (i = 0; i < 5; i++)
    cat[i] = Serial.read();


  //this code is not re-entrant.
  if (inside_cat == 1)
    return;
  inside_cat = 1;

/**
 *  This routine is enabled to debug the cat protocol
**/
  catCount++;

  /*
  if (cat[4] != 0xf7 && cat[4] != 0xbb && cat[4] != 0x03){
    //sprintf(b, "%d %02x %02x%02x%02x%02x", catCount, cat[4],cat[0], cat[1], cat[2], cat[3]);  
    itoa(catCount, b, 10);
    strcat(b, " ");
    itoa(cat[4], c, 16); strcat(b, c);
    strcat(b, " ");
    itoa(cat[3], c, 16); strcat(b, c);
    itoa(cat[2], c, 16); strcat(b, c);
    itoa(cat[1], c, 16); strcat(b, c);
    itoa(cat[1], c, 16); strcat(b, c);
    u8x8.drawString(1,5,b);
  }
  */
  
  ProcessCatCommand(cat);
  inside_cat = 0;
}


