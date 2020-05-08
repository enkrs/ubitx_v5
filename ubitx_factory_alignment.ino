#ifdef HAS_FACTORY_ALIGNMENT

/**
 * This procedure is only for those who have a signal generator/transceiver tuned to exactly 7.150 and a dummy load 
 */

void btnWaitForClick(){
  while(!btnDown())
    activeDelay(50);
  while(btnDown())
    activeDelay(50);
 activeDelay(50);
}

/**
 * Take a deep breath, math(ematics) ahead
 * The 25 mhz oscillator is multiplied by 35 to run the vco at 875 mhz
 * This is divided by a number to generate different frequencies.
 * If we divide it by 875, we will get 1 mhz signal
 * So, if the vco is shifted up by 875 hz, the generated frequency of 1 mhz is shifted by 1 hz (875/875)
 * At 12 Mhz, the carrier will needed to be shifted down by 12 hz for every 875 hz of shift up of the vco
 * 
 */


void factory_alignment(){
        
  calibrateClock();

  if (calibration == 0){
    printLine6("Setup Aborted");
    return;
  }

  //move it away to 7.160 for an LSB signal
  setFrequency(7170000l);
  updateDisplay();
  printLine6("#2 BFO");
  activeDelay(1000);

  usbCarrier = 11053000l;
  menuSetupCarrier(1);

  if (usbCarrier == 11994999l){
    printLine6("Setup Aborted");
    return;
  }
  
  printLine6("#3:Test 3.5MHz");
  isUSB = 0;
  setFrequency(3500000l);
  updateDisplay();

  while (!btnDown()){
    checkPTT();
    activeDelay(100);
  }

  btnWaitForClick();
  printLine6("#4:Test 7MHz");

  setFrequency(7150000l);
  updateDisplay();
  while (!btnDown()){
    checkPTT();
    activeDelay(100);
  }

  btnWaitForClick();
  printLine6("#5:Test 14MHz");

  isUSB = 1;
  setFrequency(14000000l);
  updateDisplay();
  while (!btnDown()){
    checkPTT();
    activeDelay(100);
  }

  btnWaitForClick();
  printLine6("#6:Test 28MHz");

  setFrequency(28000000l);
  updateDisplay();
  while (!btnDown()){
    checkPTT();
    activeDelay(100);
  }

  printLine6("Alignment done");
  activeDelay(1000);

  isUSB = 0;
  setFrequency(7150000l);
  updateDisplay();  
  
}

#endif
