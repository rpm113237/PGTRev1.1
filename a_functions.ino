void RxStringParse(void){
    //Serial.printf("length of rxValue in RxParseString = %d\n", rxValue.length()); 
    if ((digitalRead(StartButton)==LOW)) Serial.printf("Start button is LOW\n");
    // else Serial.printf("Start button is HIGH\n");
    if ((rxValue.length() > 0) || (digitalRead(StartButton)==LOW) ) {   //awkward, do we need to check length?
      Serial.printf("rxValue %s\n", rxValue.c_str()); 
      if ((toupper(rxValue[0]) =='X') || (digitalRead(StartButton)==LOW)){
        //Serial.printf (" X found\n");
      
      MorseChar(SHAVE_HAIRCUT);
      pixels.setPixelColor(LEDSelect, pixels.Color(clrs.OFF[0], clrs.OFF[1], clrs.OFF[2]));
      pixels.show();  // Send the updated pixel colors to the hardware.
      Serial.printf("******Going to Deep Sleep; wakeup by GPIO %d*****\n", StartButton);
      esp_deep_sleep_enable_gpio_wakeup(1 << StartButton, ESP_GPIO_WAKEUP_GPIO_LOW);
      esp_deep_sleep_start(); 
      Serial.println("This never happens");
        }
      else if (toupper(rxValue[0]) =='C'){
        Serial.printf (" C found, do calibration now\n");

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
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println(" setting old device; This should happen once");
    pixels.setPixelColor(LEDSelect, pixels.Color(clrs.BLUE[0], clrs.BLUE[1], clrs.BLUE[2]));
    pixels.show();  // Send the updated pixel colors to the hardware.
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



void CheckForce(void) { //PGT1 changed

  if (scale.is_ready()) {  //.get_units is blocking.
    scaleVal = scale.get_units(scaleSamples);
    sprintf(TxString, "HF:%.1f", scaleVal);
    BLETX();  //transmits TxString
  }
}


// void click(Button2& btn) {
//   //Serial.println("click--pause/unpause");
//   //do restore
//   if (sqstate == sqPause) {
//     oldmillis = millis() - pause_el_time;
//     sqstate = paused_State;
//     BLETX("V:Resuming");
//     Serial.printf("Un pausing....\n");
//     MorseChar('o');

//   } else {
//     //do store & got to sqState
//     Serial.printf("Pausing....\n");
//     BLETX("V:Pausing");
//     MorseChar('5');
//     pause_el_time = el_time;  //el_time can be used in sqPause
//     paused_State = sqstate;
//     sqstate = sqPause;
//     oldmillis = millis();
//   }
// }
// void longClickDetected(Button2& btn) {
//   Serial.println("long click detected; go to sleep");
//   MorseChar('5');
//   MorseChar('5');
//   BLETX("V:Goodbye");
//   //newseqstate(WRAPUP, HI_LOW_LED, 0, clrs.OFF);  //resets oldmillis
// }
// void longClick(Button2& btn) {
//   Serial.println("long click\n");
// }

// void initVals(void) {

//   for (hand_num = 0; hand_num < 2; hand_num++) {
//     //zero out app display
//     hands[hand_num].MVE_MAX = 0;
//     BLETX("MF:%d:R:%.1f", hand_num, 0, hands[hand_num].MVE_MAX);  //zero out max
//     hands[hand_num].HOLDTgt = 0;
//     BLETX("TF:%d:R:%.1f", hand_num, 0, hands[hand_num].HOLDTgt);  //zero out hold tgt
//     hands[hand_num].HOLDMin = 0;
//     BLETX("LL:%d:R:%.1f", hand_num, 0, hands[hand_num].HOLDMin);
//     hands[hand_num].HOLDMax = 0;
//     BLETX("HL:%d:R:%.1f", hand_num, 0, hands[hand_num].HOLDMax);
//     BLETX("HF:%d:0:%.1f", hand_num, 0, 0.0);  //make hold force go to zero
//     hands[hand_num].ON_TGT_PCNT = 0;
//     BLETX("TP:%d:G:%.1f", hand_num, 0, hands[hand_num].ON_TGT_PCNT);
//     hands[hand_num].HI_PCNT = 0;
//     BLETX("HP:%d:G:%.1f", hand_num, 0, hands[hand_num].HI_PCNT);
//     hands[hand_num].LO_PCNT = 0;
//     BLETX("LP:%d:G:%.1f", hand_num, 0, hands[hand_num].LO_PCNT);
//     //zero out stats
//     hands[hand_num].ON_TGT_HITS = 0;
//     hands[hand_num].HI_HITS = 0;
//     hands[hand_num].LO_HITS = 0;
//     hands[hand_num].TOT_HITS = 0;
//   }
// }

void LEDBlink(void) {   //PGT1 Revised
  static u_long lclmillis = 0;
  static bool ON_OFF = true;  //true = ON??

  if (BlinkTime > 0) {
    if (((millis() - lclmillis) > BlinkTime) && ON_OFF) {
      if (UseRedLED) digitalWrite(LEDRED, HIGH);
      if (UseBlueLED) digitalWrite(LEDBLUE, HIGH);
      ON_OFF = false;
      lclmillis = millis();

    } else if (((millis() - lclmillis) > BlinkTime) && !ON_OFF) {
      if (UseRedLED) digitalWrite(LEDRED, LOW);
      if (UseBlueLED) digitalWrite(LEDBLUE, LOW);
      ON_OFF = true;
      lclmillis = millis();
    }
  } else {
    if (UseRedLED) digitalWrite(LEDRED, HIGH);
    if (UseBlueLED) digitalWrite(LEDBLUE, HIGH);
  }  //0 blink time is on solid
}

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
  //Serial.printf("******Battery: Raw = %d;  volts = %.2f****** \n", battrdg, battvolts);
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
// void WrapupTX(void) {
//   int i = 0;
//   for (i = 0; i < 2; i++) {
//     if (i == 0) BLETX("V:Right Percent");
//     else BLETX("V:Left Percent");
//     delay(1000);
//     BLETX("V:On Target %.1f", 0, 0, hands[i].ON_TGT_PCNT);
//     delay(1000);
//     BLETX("V:Low %.1f", 0, 0, hands[i].LO_PCNT);
//     delay(1000);  //need to add in delay time to bletx?
//     BLETX("V:High %.1f", 0, 0, hands[i].HI_PCNT);
//     delay(1000);  //need to add in delay time to bletx?
//   }
// }
