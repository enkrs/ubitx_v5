/** Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls doMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */


/** A generic control to read variable values
*/
int getValueByKnob(int minimum, int maximum, int step_size,  int initial, const char* prefix, const char *postfix)
{
  int knob = 0;
  int knob_value;

  knob_value = initial;

  u8x8.clear();
   
  strcpy(b, prefix);
  itoa(knob_value, c, 10);
  strcat(b, c);
  strcat(b, postfix);
  printLine6(b);
  active_delay(300);

  while(!btnDown() && digitalRead(PTT) == HIGH){

    knob = enc_read();
    if (knob != 0){
      if (knob_value > minimum && knob < 0)
        knob_value -= step_size;
      if (knob_value < maximum && knob > 0)
        knob_value += step_size;

      printLine(2,prefix);
      itoa(knob_value, c, 10);
      strcpy(b, c);
      strcat(b, postfix);
      printLine(4,b);
    }
    checkCAT();
  }
  btnWaitUp();

  return knob_value;
}

//# Menu: 1

void menuBand(int btn){
  int knob = 0;
  static const char* BAND_SELECT = "BAND SELECT";

  if (!btn){
    printLine6icon(BAND_SELECT,'N');
    return;
  }

  printLine6(BAND_SELECT);
  u8x8.setInverseFont(0);

  //wait for the button menu select button to be lifted)
  ritDisable();

  while (!btnDown()) {
    knob = enc_read();
    if (knob != 0) {
      if (knob < 0)
        setFrequency(frequency - 200000l);
      if (knob > 0)
        setFrequency(frequency + 200000l);
      if (frequency > 10000000l)
        isUSB = true;
      else
        isUSB = false;
      updateDisplay();
    }
    active_delay(20);
  }
  btnWaitUp();

  menuOn = 0;
}

// Menu #2
void menuRitToggle(int btn){
  if (!btn) {
    if (ritOn == 1)
      printLine6icon("RIT       On",'V');
    else
      printLine6icon("RIT      Off",'V');
    return;
  }

  if (ritOn == 0) {
    //enable RIT so the current frequency is used at transmit
    ritEnable(frequency);
    //printLine6("RIT is On");
  } else {
    ritDisable();
    //printLine6("RIT is Off");
  }
  //active_delay(500);

  menuOn = 0;
}


//Menu #3
void menuVfoToggle(int btn) {
  if (!btn){
    if (vfoActive == VFO_A)
      printLine6icon("VFO        A",'V');
    else
      printLine6icon("VFO        B",'V');
    return;
  }

  if (vfoActive == VFO_B) {
    vfoB = frequency;
    isUsbVfoB = isUSB;
    EEPROM.put(VFO_B, frequency);
    if (isUsbVfoB)
      EEPROM.put(VFO_B_MODE, VFO_MODE_USB);
    else
      EEPROM.put(VFO_B_MODE, VFO_MODE_LSB);

    vfoActive = VFO_A;
    frequency = vfoA;
    isUSB = isUsbVfoA;
  } else {
    vfoA = frequency;
    isUsbVfoA = isUSB;
    EEPROM.put(VFO_A, frequency);
    if (isUsbVfoA)
      EEPROM.put(VFO_A_MODE, VFO_MODE_USB);
    else
      EEPROM.put(VFO_A_MODE, VFO_MODE_LSB);

    vfoActive = VFO_B;
    frequency = vfoB;
    isUSB = isUsbVfoB;
  }
    
  ritDisable();
  setFrequency(frequency);
  updateDisplay();
  printLine6("");
  menuOn = 0;
}

// Menu #4
void menuSidebandToggle(int btn){
  if (!btn){
    if (isUSB == true)
      printLine6icon("USB", 'V');
    else
      printLine6icon("LSB", 'V');
    return;
  }

  if (isUSB == true) {
    isUSB = false;
  } else {
    isUSB = true;
  }
  //Added by KD8CEC
  if (vfoActive == VFO_B){
    isUsbVfoB = isUSB;
  } else {
    isUsbVfoB = isUSB;
  }
  updateDisplay();
  menuOn = 0;
}

//Split communication using VFOA and VFOB by KD8CEC
//Menu #5
void menuSplitToggle(int btn){
  if (!btn) {
    if (splitOn == 0)
      printLine6icon("SPLIT    ON", 'V');
    else
      printLine6icon("SPLIT   OFF", 'V');
    return;
  }

  if (splitOn == 1) {
    splitOn = 0;
  } else {
    splitOn = 1;
    if (ritOn == 1)
      ritOn = 0;
  }
  menuOn = 0;
}

void menuCWSpeed(int btn){
    int wpm;

    wpm = 1200 / cwSpeed;
     
    if (!btn) {
     strcpy(b, "CW: ");
     itoa(wpm,c, 10);
     strcat(b, c);
     strcat(b, " WPM");
     printLine6icon(b, 'V');
     return;
    }

    wpm = getValueByKnob(1, 100, 1,  wpm, "CW: ", " WPM");
    cwSpeed = 1200 / wpm;
    EEPROM.put(CW_SPEED, cwSpeed);

    menuOn = 0;
}

void menuExit(int btn){

  if (!btn){
      printLine6("EXIT MENU");
      return;
  }

  menuOn = 0;
}

/**
 * The calibration routines are not normally shown in the menu as they are rarely used
 * They can be enabled by choosing this menu option
 */
int menuSetup(int btn){
  if (!btn){
    if (!extendedMenu)
      printLine6icon("EXTENDED",'L');
    else
      printLine6icon("EXTENDED",'V');
    return 0;
  }

  if (!extendedMenu){
    extendedMenu = true;
  } else {
    extendedMenu = false;
  }

  return 10;
}

 //this is used by the si5351 routines in the ubitx_5351 file
extern int32_t calibration;
extern uint32_t si5351bx_vcoa;

void calibrateClock(){
  int knob = 0;

  digitalWrite(TX_LPF_A, 0);
  digitalWrite(TX_LPF_B, 0);
  digitalWrite(TX_LPF_C, 0);

  calibration = 0;

  isUSB = true;

  //turn off the second local oscillator and the bfo
  si5351_set_calibration(calibration);
  startTx(TX_CW);
  si5351bx_setfreq(2, 10000000l); 
  
  strcpy(b, "#1 10 MHz cal:");
  ltoa(calibration/8750, c, 10);
  strcat(b, c);
  printLine6(b);     

  while (!btnDown()) {
    if (digitalRead(PTT) == LOW && !keyDown)
      cwKeydown();
    if (digitalRead(PTT)  == HIGH && keyDown)
      cwKeyUp();
      
    knob = enc_read();

    if (knob > 0)
      calibration += 875;
    else if (knob < 0)
      calibration -= 875;
    else 
      continue; //don't update the frequency or the display
      
    si5351_set_calibration(calibration);
    si5351bx_setfreq(2, 10000000l);
    strcpy(b, "#1 10 MHz cal:");
    ltoa(calibration/8750, c, 10);
    strcat(b, c);
    printLine6(b);     
  }
  btnWaitUp();

  cwTimeout = 0;
  keyDown = 0;
  stopTx();

  EEPROM.put(MASTER_CAL, calibration);
  initOscillators();
  setFrequency(frequency);    
}

void menuSetupCalibration(int btn){
  if (!btn){
      //       0123456789012345
    printLine6("CALIBRATE");
    return;
  }

  printLine1("Press PTT & tune");
  printLine6("to exactly 10 MHz");
  active_delay(2000);
  calibrateClock();
}

void printCarrierFreq(unsigned long freq){

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(freq, b, DEC);
  
  strncat(c, b, 2);
  strcat(c, ".");
  strncat(c, &b[2], 3);
  strcat(c, ".");
  strncat(c, &b[5], 1);
  printLine6(c);    
}

void menuSetupCarrier(int btn){
  int knob = 0;
   
  if (!btn){
      printLine6("Setup:BFO      \x7E");
    return;
  }

  printLine1("Tune to best Signal");  
  printLine6("Press to confirm. ");
  active_delay(1000);

  usbCarrier = 11053000l;
  si5351bx_setfreq(0, usbCarrier);
  printCarrierFreq(usbCarrier);

  //disable all clock 1 and clock 2 
  while (!btnDown()){
    knob = enc_read();

    if (knob > 0)
      usbCarrier -= 50;
    else if (knob < 0)
      usbCarrier += 50;
    else
      continue; //don't update the frequency or the display
      
    si5351bx_setfreq(0, usbCarrier);
    printCarrierFreq(usbCarrier);
    
    active_delay(100);
  }

  printLine6("Carrier set!    ");
  EEPROM.put(USB_CAL, usbCarrier);
  active_delay(1000);
  
  si5351bx_setfreq(0, usbCarrier);          
  setFrequency(frequency);    
  updateDisplay();
  printLine6("");
  menuOn = 0; 
}

void menuSetupCwTone(int btn){
  int knob = 0;
  int prev_sideTone;
     
  if (!btn){
    printLine6("Setup:CW Tone  \x7E");
    return;
  }

  prev_sideTone = sideTone;
  printLine1("Tune CW tone");  
  printLine6("PTT to confirm. ");
  active_delay(1000);
  tone(CW_TONE, sideTone);

  //disable all clock 1 and clock 2 
  while (digitalRead(PTT) == HIGH && !btnDown())
  {
    knob = enc_read();

    if (knob > 0 && sideTone < 2000)
      sideTone += 10;
    else if (knob < 0 && sideTone > 100 )
      sideTone -= 10;
    else
      continue; //don't update the frequency or the display
        
    tone(CW_TONE, sideTone);
    itoa(sideTone, b, 10);
    printLine6(b);

    active_delay(20);
  }
  noTone(CW_TONE);
  //save the setting
  if (digitalRead(PTT) == LOW){
    printLine6("Sidetone set!    ");
    EEPROM.put(CW_SIDETONE, sideTone);
    active_delay(2000);
  }
  else
    sideTone = prev_sideTone;
    
  printLine6("");  
  updateDisplay(); 
  menuOn = 0; 
}

void menuSetupCwDelay(int btn){
  if (!btn){
    printLine6("Setup:CW Delay \x7E");
    return;
  }

  cwDelayTime = getValueByKnob(10, 1000, 50,  cwDelayTime, "CW Delay>", " msec");

  printLine1("CW Delay Set!");  
  printLine6("");
  active_delay(500);
  updateDisplay();
  menuOn = 0;
}

void menuSetupKeyer(int btn){
  int tmp_key, knob;
  
  if (!btn){
    if (!Iambic_Key)
      printLine6("Setup:CW(Hand)\x7E");
    else if (keyerControl & IAMBICB)
      printLine6("Setup:CW(IambA)\x7E");
    else 
      printLine6("Setup:CW(IambB)\x7E");    
    return;
  }
  
  active_delay(500);

  if (!Iambic_Key)
    tmp_key = 0; //hand key
  else if (keyerControl & IAMBICB)
    tmp_key = 2; //Iambic B
  else 
    tmp_key = 1;
 
  while (!btnDown())
  {
    knob = enc_read();
    if (knob < 0 && tmp_key > 0)
      tmp_key--;
    if (knob > 0)
      tmp_key++;

    if (tmp_key > 2)
      tmp_key = 0;
      
    if (tmp_key == 0)
      printLine1("Hand Key?");
    else if (tmp_key == 1)
      printLine1("Iambic A?");
    else if (tmp_key == 2)  
      printLine1("Iambic B?");  
  }

  active_delay(500);
  if (tmp_key == 0)
    Iambic_Key = false;
  else if (tmp_key == 1){
    Iambic_Key = true;
    keyerControl &= ~IAMBICB;
  }
  else if (tmp_key == 2){
    Iambic_Key = true;
    keyerControl |= IAMBICB;
  }
  
  EEPROM.put(CW_KEY_TYPE, tmp_key);
  
  printLine1("Keyer Set!");
  active_delay(600);
  printLine1("");

  //Added KD8CEC
  printLine6("");
  updateDisplay(); 
  menuOn = 0;  
}

void menuReadADC(int btn){
  int adc;
  
  if (!btn){
    // TODO ADD SOME TEXT HERE
    adc = 801; //YL3AME:analogRead(ANALOG_KEYER);
    itoa(adc, b, 10);
    printLine6(b);
    //printLine6("6:Setup>Read ADC>");
    return;
  }

  menuOn = 0;
}

void doMenu(){
  int select = 0, btnState;

  btnWaitUp();
  
  menuOn = 2;
  
  while (menuOn) {
    btnState = btnDown();
    if (btnState)
      btnWaitUp();

    select += enc_read();
    if (extendedMenu && select > 150)
      select = 150;
    if (!extendedMenu && select > 80)
      select = 80;
    if (select < 0)
      select = 0;

    if (select < 10)
      menuBand(btnState);
    else if (select < 20)
      menuRitToggle(btnState);
    else if (select < 30)
      menuVfoToggle(btnState);
    else if (select < 40)
      menuSidebandToggle(btnState);
    else if (select < 50)
      menuSplitToggle(btnState);
    else if (select < 60)
      menuCWSpeed(btnState);
    else if (select < 70)
      select += menuSetup(btnState);
    else if (select < 80 && !extendedMenu)
      menuExit(btnState);
    else if (select < 90 && extendedMenu)
      menuSetupCalibration(btnState);   //crystal
    else if (select < 100 && extendedMenu)
      menuSetupCarrier(btnState);       //lsb
    else if (select < 110 && extendedMenu)
      menuSetupCwTone(btnState);
    else if (select < 120 && extendedMenu)
      menuSetupCwDelay(btnState);
    else if (select < 130 && extendedMenu)
      menuReadADC(btnState);
    else if (select < 140 && extendedMenu)
      menuSetupKeyer(btnState);
    else
      menuExit(btnState);  
  }
  u8x8.clear();
  updateDisplay();
  checkCAT();
}

