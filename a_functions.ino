void RxStringParse(void){
    //Serial.printf("length of rxValue in RxParseString = %d\n", rxValue.length()); 
    if ((digitalRead(StartButton)==LOW)) Serial.printf("Start button is LOW\n");
    // else Serial.printf("Start button is HIGH\n");
    if ((rxValue.length() > 0) || (digitalRead(StartButton)==LOW) ) {   //awkward, do we need to check length?
      Serial.printf("rxValue %s\n", rxValue.c_str()); 
      Serial.printf ("rxValue[0], to upper is %c\n", toupper(rxValue[0]));
      if ((toupper(rxValue[0]) =='X') || (digitalRead(StartButton)==LOW)){
        //Serial.printf (" X found\n");
      scale.power_down();
      MorseChar(SHAVE_HAIRCUT);
      pixels.setPixelColor(LEDSelect, pixels.Color(clrs.OFF[0], clrs.OFF[1], clrs.OFF[2]));
      pixels.show();  // Send the updated pixel colors to the image.pngimage.pnghardware.
      Serial.printf("******Going to Deep Sleep; wakeup by GPIO %d*****\n", StartButton);
      esp_deep_sleep_enable_gpio_wakeup(1 << StartButton, ESP_GPIO_WAKEUP_GPIO_LOW);
      esp_deep_sleep_start(); 
      Serial.println("This never happens");
        }
        /*Procedure for calibration is:
        --app sends an X for tare.  tare is done on start up, but good practice. Tare takes 20 samples--2 sec +/-
        --app sends X:NN.N.  NN.N is in lbs/kilograms Note: the calibration is actually 
        the scale calibrates, stores the calibration factor.

        */
      else if (toupper(rxValue[0]) =='C'){
        Serial.printf (" C found, do calibration now\n");
        std::string calstring;    //string that is weight 
        calstring = rxValue.substr(1, rxValue.length());
        if (calstring.length()>0){
        uint32_t calweight = uint32_t (stof(calstring) *10);    //have to divide the readings by ten
        Serial.printf("calweight (*10) = %ld\n", calweight);
        scale.calibrate_scale(calweight,20); //20 times should be plenty
        float calcscale = scale.get_scale();
        Serial.printf(" calscale = %f \n", calcscale );
        }
        else Serial.printf("Zero length cal value string\n");
      }
      else if (toupper(rxValue[0] == 'T')){   //tare
      Serial.printf("***Doing tare***\n");
        scale.tare(20);   //20X should be plenty
        long scaleoffset = scale.get_offset();
        Serial.printf ("Tare done, offset = %ld\n", scaleoffset);
      

      }
      else {Serial.printf("Character 0 is %c \n", toupper(rxValue[0])); }
      rxValue.clear();  //erases     
          
    }
      

      


}


void BLEReconnect(void) {
  //attempt to reconnect
  //not sure if it works.
  if (!deviceConnected && oldDeviceConnected) {
    delay(10);                    // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println(" in loop reconnect ;start advertising");
    setLED(250,clrs.BLUE);
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println(" setting old device; This should happen once");
    // pixels.setPixelColor(LEDSelect, pixels.Color(clrs.BLUE[0], clrs.BLUE[1], clrs.BLUE[2]));
    // pixels.show();  // Send the updated pixel colors to the hardware.
    setLED(0,clrs.BLUE);
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}


void FindCalibration() {    //never checked out.
  //set to something like 50 lbs
  // set cal factor to rawreading/50 (or whatever)
  float lclrawval = 0;
  //float lclcalfactor = 0.0;
  while (1) {
    if (scale.is_ready()) {
      scaleVal = scale.get_units(scaleSamples);
      // BLETX("HF:%d:R:%.1f", hand_num, 0, scaleVal);  //report max //simulate
      Serial.printf("scale units reading = %.4f \n", scaleVal);
    }
    delay(2000);
    if (scale.is_ready()) {
      lclrawval = scale.read_average(scaleSamples);
      Serial.printf("Read 10X; lcl raw val = %.2f\n", lclrawval);  // print the average of 20 readings from the ADC
                                                                   // lclcalfactor = (float)scaleVal/lclrawval;
                                                                   // Serial.printf("suggest cal factor = %.1f\n", lclcalfactor);
    }
    delay(2000);
    Serial.println("****************\n");
  }
}

int floatcomp(const void* elem1, const void* elem2)
{
    if(*(const float*)elem1 < *(const float*)elem2)
        return -1;
    return *(const float*)elem1 > *(const float*)elem2;
}

void CheckForce(void) { //PGT1 changed
  int i = 0; //force array index
  int arraysz = sizeof(Force.ForceArray)/sizeof(Force.ForceArray[0]);
  if (scale.is_ready()) {  //.get_units is blocking.
    scaleVal = scale.get_units(scaleSamples);
    for (i = (arraysz-1); i >0; i--){
      Force.ForceArray[i]= Force.ForceArray[i-1];   //move everything up one spot
    }
    Force.ForceArray[0] = scaleVal;
    //now find max, min, mean
    Force.ForceMax = scaleVal;   
    Force.ForceMin = scaleVal;
    Force.ForceMean = 0;
    for (i=0; i<arraysz; i++){
      // Serial.printf("%.3f ",Force.ForceArray[i]);
      if (Force.ForceArray[i]>Force.ForceMax) Force.ForceMax=Force.ForceArray[i];
      if (Force.ForceArray[i]<Force.ForceMin) Force.ForceMin=Force.ForceArray[i];
      Force.ForceMean += Force.ForceArray[i];      
    }
    // Serial.printf("\nForceMeanR = %.3f, \tArray len = %d\n", Force.ForceMean, arraysz);
    Force.ForceMean= Force.ForceMean/arraysz;
    float wkarray[FORCE_ARRAY_LEN];    //temp array for sorting
    // Serial.printf("Force = %.3f\n", scaleVal);
    // Serial.printf("unsorted\t");
    // for(i = 0; i < 10; i++)
    //  Serial.printf("%.3f\t", Force.ForceArray[i]);
    // Serial.printf("\n");   

    for (i=0;i<arraysz; i++) wkarray[i]=Force.ForceArray[i];
    qsort(wkarray, arraysz, sizeof(float), floatcomp);
  //   Serial.printf("  sorted\t");
  //  for(i = 0; i < 10; i++)
  //     Serial.printf("%.3f\t", wkarray[i]);
  //  Serial.printf("\n");   
   if (arraysz %2 ==0) Force.ForceMedian = (wkarray[arraysz/2] +wkarray[arraysz/2 + 1])/2;
   else Force.ForceMedian = wkarray[arraysz/2 + 1];
   //Serial.printf("median = %.4f\n\n", Force.ForceMedian);
   


    // Serial.printf("ForceMeanA = %.3f, \tArray len = %d\n", Force.ForceMean,arraysz);
    // Serial.printf("\tHF= %.3f\t",scaleVal);
    // Serial.printf("Mean= %.3f\t",Force.ForceMean);
    // Serial.printf("Max= %.3f\t", Force.ForceMax);
    // Serial.printf("Min= %.3f\t", Force.ForceMin);
    // Serial.printf("Delta = %.3f\n", (Force.ForceMax-Force.ForceMin));

    sprintf(TxString, "HF:%.3f", scaleVal);
    BLETX();  //transmits TxString
    
  }
}

void VibSend() {
  
  sprintf(TxString, "MX:%.3f,%.3f", Force.ForceMax, Force.ForceMin);
  BLETX();
  sprintf(TxString, "MN:%.3f,%.3f", Force.ForceMean, Force.ForceMedian);
  BLETX();
}

void setLED(int btime, int clrarray[3]) {  //incorporate into LEDBlink                                                    
  int i = 0;
  BlinkTime = btime;
  for (i = 0; i < 3; i++) {
    //Serial.printf("i = %d; clr = %d\t", i, clrarray[i] );
    clrs.WKCLRS[i] = clrarray[i];
  }
  Serial.printf("\n");
  //LEDSelect = LedNo;
}

void LEDBlink() {
  static u_long lclmillis = 0;
  static bool ON_OFF = true;

  if (BlinkTime > 0) {
    if (((millis() - lclmillis) > BlinkTime) && ON_OFF) {
      pixels.setPixelColor(LEDSelect, pixels.Color(clrs.OFF[0], clrs.OFF[1], clrs.OFF[2]));
      pixels.show();  // Send the updated pixel colors to the hardware.
      ON_OFF = false;
      lclmillis = millis();

    } else if (((millis() - lclmillis) > BlinkTime) && !ON_OFF) {
      pixels.setPixelColor(LEDSelect, clrs.WKCLRS[0], clrs.WKCLRS[1], clrs.WKCLRS[2]);
      pixels.show();
      ON_OFF = true;
      lclmillis = millis();
    }
  } else {
    pixels.setPixelColor(LEDSelect, clrs.WKCLRS[0], clrs.WKCLRS[1], clrs.WKCLRS[2]);
  }  //0 blink time is on solid
  pixels.show();
}

// void LEDBlink(void) {   //PGT1 Revised
//   static u_long lclmillis = 0;
//   static bool ON_OFF = true;  //true = ON??

//   if (BlinkTime > 0) {
//     if (((millis() - lclmillis) > BlinkTime) && ON_OFF) {
//       if (UseRedLED) digitalWrite(LEDRED, HIGH);
//       if (UseBlueLED) digitalWrite(LEDBLUE, HIGH);
//       ON_OFF = false;
//       lclmillis = millis();

//     } else if (((millis() - lclmillis) > BlinkTime) && !ON_OFF) {
//       if (UseRedLED) digitalWrite(LEDRED, LOW);
//       if (UseBlueLED) digitalWrite(LEDBLUE, LOW);
//       ON_OFF = true;
//       lclmillis = millis();
//     }
//   } else {
//     if (UseRedLED) digitalWrite(LEDRED, HIGH);
//     if (UseBlueLED) digitalWrite(LEDBLUE, HIGH);
//   }  //0 blink time is on solid
// }

void BLETX(void) {

  if (strlen(TxString) > 19) Serial.println("String >19, truncated");
  if (deviceConnected) {
    pTxCharacteristic->setValue(TxString);
    pTxCharacteristic->notify();
    Serial.printf("BLE\t%20s\n", TxString);
                                           // bluetooth stack will go into congestion, if too many packets are sent
  } else Serial.printf("OFFline\t%20s\n", TxString);
}


void print_wakeup_reason() {

  Serial.printf("Boot number: %d\n", ++bootCount);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_GPIO:  // 05 - this is used by ESP32-C3 as EXT0/EXT1 does not available in C3
      Serial.printf("***Wakeup number %d by GPIO %d  ***\n", ++bootCount, StartButton);
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
}


void BatSnsCk(void) {
  //reads the ADC on BatSns
  //int batSOC = 0;
  int battrdg = 0;  //raw
  //float battvolts = 0.0;
  battrdg = analogRead(BatSns);
  battvolts = ((float)battrdg * 2 * 3.2 * 210 / 310) / 4095;  //fudge; something wrong with voltage divider--impedance too high--r11 = 210k? WTF??
  Serial.printf("******Battery: Raw = %d;  volts = %.2f****** \n", battrdg, battvolts);
  sprintf(TxString, "E:%.2f", battvolts);
  if (battvolts <= Batt_LO_Lvl) {
    UseRedLED = true;
    UseBlueLED = false;
  } else {
    UseBlueLED = true;
    UseRedLED = false;
  }
  BLETX();  
  //
}

void SoundBuzz(u_long cwFreq, int sound_ms) {
  //sounds cwFreq for sound_ms
  //note--this is blocking
  u_long smillis;
  // Serial.printf(" Freq =%d HZ, for %d ms\n", cwFreq, sound_ms);
  smillis = millis();
  ledcAttach(buzzPin, freq, resolution);  //eight bit resolution--why? (Jun24?)
  //ledcSetup(ledChannel, cwFreq, resolution);
  ledcWrite(buzzPin, dutycycle);
  while ((millis() - smillis) < sound_ms) {}
  ledcWrite(buzzPin, 0);  //off
}

void Soundwakeup(void) {
  //the start buttom has been pushed
  MorseChar('o');
}

void SoundElement(int elementTime) {
  SoundBuzz(cwFreq, elementTime);  //dit
  delay(ditTime);
}

void MorseChar(int cwChar) {
  //cwChar = 'a', 'b', etc.
  // Morse code for s (dot dot dot)
  int dahTime;
  dahTime = 3 * ditTime;  //there is a space time defined in .h
  cwChar = tolower(cwChar);
  //Serial.printf("character %d called\n", cwChar);
  if (cwChar == 's') {

    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    delay(dahTime);
  } else if (cwChar == 'o') {
    //daht-dah-dah
    SoundElement(dahTime);  //dah
    SoundElement(dahTime);  //dah
    SoundElement(dahTime);  //dah
    delay(dahTime);
  }

  else if (cwChar == 'r') {
    //dit-dah-dit
    SoundElement(ditTime);  //dit
    SoundElement(dahTime);  //dah
    SoundElement(ditTime);  //dit
    delay(dahTime);
  } else if (cwChar == 'l') {
    //dit-dah-dit-dit
    SoundElement(ditTime);  //dit
    SoundElement(dahTime);  //dah
    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    delay(dahTime);
  } else if (cwChar == ' ') {
    //space = delay of seven dit times
    delay(7 * ditTime);
  } else if (cwChar == 'e') {
    //dit
    SoundElement(ditTime);  //dit
    delay(dahTime);
  } else if (cwChar == SHAVE_HAIRCUT) {  //use "ESC = 0x1b" for shave & haircut
    SoundElement(dahTime);               //dah
    SoundElement(1 * ditTime);           //di
    SoundElement(1 * ditTime);           //di
    SoundElement(dahTime);               //dah
    SoundElement(ditTime);               //dit
    delay(2 * ditTime);
    SoundElement(1 * ditTime);  //di
    SoundElement(1 * ditTime);  //di
    delay(7 * ditTime);         //??
  } else if (cwChar == '5') {
    //Serial.println("hit the 5");
    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    SoundElement(ditTime);  //dit
    delay(dahTime);

  } else Serial.printf("character %d not recognized\n", cwChar);
}

