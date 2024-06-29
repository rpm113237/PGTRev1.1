
void FindCalibration() {
  //set to something like 50 lbs
  // set cal factor to rawreading/50 (or whatever)
  float lclrawval = 0;
  //float lclcalfactor = 0.0;
  while (1) {
    if(scale.is_ready()){      
    scaleVal = scale.get_units(scaleSamples);
    BLETX("HF:%d:R:%.1f", hand_num, 0, scaleVal);  //report max //simulate
    Serial.printf("scale units reading = %.4f \n", scaleVal);
    }
    delay(2000);
    if(scale.is_ready()){      
    lclrawval = scale.read_average(scaleSamples);
    Serial.printf("Read 10X; lcl raw val = %.2f\n", lclrawval);  // print the average of 20 readings from the ADC
                                                                 // lclcalfactor = (float)scaleVal/lclrawval;
                                                                 // Serial.printf("suggest cal factor = %.1f\n", lclcalfactor);  
    }
    delay(2000);
  Serial.println("****************\n");
  
  }
}

void PingAndCheckForce(int numTimes) {
  int i;
  for (i = 1; i <= numTimes; i++) {
    BLETX("S:PING");         //needs shorter time in bletx, scale read is 250ms or so
    if (scale.is_ready()) {  //.get_units is blocking.
      scaleVal = scale.get_units(scaleSamples / 3);
      BLETX("HF:%d:B:%.1f", hand_num, 0, scaleVal);  //report max //simulate
    }
  }
}

void CheckForce(void) {

  if (scale.is_ready()) {  //.get_units is blocking.
    scaleVal = scale.get_units(scaleSamples / 3);
    BLETX("HF:%d:B:%.1f", hand_num, 0, scaleVal);  //report max //simulate
  }
}


void click(Button2& btn) {
  //Serial.println("click--pause/unpause");
  //do restore
  if (sqstate == sqPause) {
    oldmillis = millis() - pause_el_time;
    sqstate = paused_State;
    BLETX("V:Resuming");
    Serial.printf("Un pausing....\n");
    MorseChar('o');

  } else {
    //do store & got to sqState
    Serial.printf("Pausing....\n");
    BLETX("V:Pausing");
    MorseChar('5');
    pause_el_time = el_time;  //el_time can be used in sqPause
    paused_State = sqstate;
    sqstate = sqPause;
    oldmillis = millis();
  }
}
void longClickDetected(Button2& btn) {
  Serial.println("long click detected; go to sleep");
  MorseChar('5');
  MorseChar('5');
  BLETX("V:Goodbye");
  newseqstate(WRAPUP, HI_LOW_LED, 0, clrs.OFF);  //resets oldmillis
}
void longClick(Button2& btn) {
  Serial.println("long click\n");
}

void initVals(void) {

  for (hand_num = 0; hand_num < 2; hand_num++) {
    //zero out app display
    hands[hand_num].MVE_MAX = 0;
    BLETX("MF:%d:R:%.1f", hand_num, 0, hands[hand_num].MVE_MAX);  //zero out max
    hands[hand_num].HOLDTgt = 0;
    BLETX("TF:%d:R:%.1f", hand_num, 0, hands[hand_num].HOLDTgt);  //zero out hold tgt
    hands[hand_num].HOLDMin = 0;
    BLETX("LL:%d:R:%.1f", hand_num, 0, hands[hand_num].HOLDMin);
    hands[hand_num].HOLDMax = 0;
    BLETX("HL:%d:R:%.1f", hand_num, 0, hands[hand_num].HOLDMax);
    BLETX("HF:%d:0:%.1f", hand_num, 0, 0.0);  //make hold force go to zero
    hands[hand_num].ON_TGT_PCNT = 0;
    BLETX("TP:%d:G:%.1f", hand_num, 0, hands[hand_num].ON_TGT_PCNT);
    hands[hand_num].HI_PCNT = 0;
    BLETX("HP:%d:G:%.1f", hand_num, 0, hands[hand_num].HI_PCNT);
    hands[hand_num].LO_PCNT = 0;
    BLETX("LP:%d:G:%.1f", hand_num, 0, hands[hand_num].LO_PCNT);
    //zero out stats
    hands[hand_num].ON_TGT_HITS = 0;
    hands[hand_num].HI_HITS = 0;
    hands[hand_num].LO_HITS = 0;
    hands[hand_num].TOT_HITS = 0;
  }
}

void LEDBlink(void) {
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


void setLED(int LedNo, int btime, int clrarray[3]) {  //incorporate into LEDBlink
                                                      //legacy of Vtask
  int i = 0;
  BlinkTime = btime;
  for (i = 0; i < 3; i++) {
    //Serial.printf("i = %d; clr = %d\t", i, clrarray[i] );
    clrs.WKCLRS[i] = clrarray[i];
  }
  //Serial.printf("\n");
  LEDSelect = LedNo;
}

void BLETX(const char* fmtstr, int intparam1, u_long ulparam, float fltparam, const char* strparam) {
  char wkstr[75];
  /* searches through, uses param as appropriate
*/
  // Serial.printf("fmtstr = %s\n", fmtstr);
  // Serial.printf("fltparam = %.1f\n");
  if (strstr(fmtstr, "MF:") != NULL) { sprintf(wkstr, fmtstr, intparam1, fltparam); }  // ckd
  else if (strstr(fmtstr, "HF:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  } else if (strstr(fmtstr, "TF:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  } else if (strstr(fmtstr, "LL:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  } else if (strstr(fmtstr, "HL:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  } else if (strstr(fmtstr, "ST:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1);
  } else if (strstr(fmtstr, "HP:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  } else if (strstr(fmtstr, "TP:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  } else if (strstr(fmtstr, "LP:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, fltparam);
  }
  //ckd
  else if (strstr(fmtstr, "V:") != NULL) {
    sprintf(wkstr, fmtstr, fltparam);
    // Serial.printf("strcpy str=%s\n",wkstr);  //ckd
  } else if (strstr(fmtstr, "TT:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1, ulparam);
  } else if (strstr(fmtstr, "S:") != NULL) {
    strcpy(wkstr, fmtstr);
  } else if (strstr(fmtstr, "E:") != NULL) {
    strcpy(wkstr, fmtstr);
  } else if (strstr(fmtstr, "R:") != NULL) {
    sprintf(wkstr, fmtstr, intparam1);  //rep cnt "R:0" or "R:1"
  } else if (strstr(fmtstr, "SOC") != NULL) {
    strcpy(wkstr, fmtstr);
  }

  else Serial.printf("*******Unknown string = %s*******\n", fmtstr);
  int i = 0;

  for (i = 0; i < sizeof(TxString); i++) { TxString[i] = 0x00; }  //apparently sends the whole payload, null it out
  strcpy(TxString, wkstr);


  if (strlen(TxString) > 19) Serial.println("String >19, truncated");
  if (deviceConnected) {
    pTxCharacteristic->setValue(TxString);
    pTxCharacteristic->notify();
    Serial.printf("BLE\thand:%d\trep:%d\tlen=%d\t%20s\tSTATE= %s\n", hand_num, rep_num, strlen(TxString), TxString, stateLabels[sqstate]);
    if (strstr(fmtstr, "S:") != NULL) {
      C3Delay(1250);  //give sound effects speech time?? Needed?? probably not unless following V:?
      //button.loop();  //hokey, call every now and then.  Better on interrupt?  Or, timer call?
    }
    if (strstr(fmtstr, "V:") != NULL) {
      C3Delay(1250);  //give text to speech time
      //button.loop();  //hokey, call every now and then.  Better on interrupt?  Or, timer call?
    }  //delay(5);                                       // bluetooth stack will go into congestion, if too many packets are sent
  } else Serial.printf("OFFline\thand:%d\trep:%d\tlen=%d\t%20s\tSTATE = %s\n", hand_num, rep_num, strlen(TxString), TxString, stateLabels[sqstate]);
}

void C3Delay(int numMs) {
  //delays numMs msseconds while calling LEDBlink;
  //used by BLETX ; keep LED's alive while text to speech
  //ESP32 S3--led's in task.
  int i = 0;
  for (i = 0; i <= numMs; i++) {
    delay(1);
    LEDBlink();
    //if (digitalRead(StartButton==LOW))click();
  }
}

void print_wakeup_reason() {

  //Serial.printf("Boot number: %d\n", ++bootCount);

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
      Serial.printf("***Wakeup number %d by GPIO %d++++\n", ++bootCount, StartButton);
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
}

void newseqstate(SqueezerState newst, int ledno, int ledblink, int ledclr[]) {
  setLED(ledno, ledblink, ledclr);  //n
  oldmillis = millis();
  sqstate = newst;  //make this state chang
  //Serial.printf("newstate = %d\n", newst);
}
void BatSnsCk(void) {
  //reads the ADC on BatSns
  int batSOC = 0;
  int battrdg = 0;  //raw
  float battvolts = 0.0;
  battrdg = analogRead(BatSns);
  battvolts = ((float)battrdg * 2 * 3.2 * 210 / 310) / 4095;  //fudge; something wrong with voltage divider--impedance too high--r11 = 210k? WTF??
  //Serial.printf("******Battery: Raw = %d;  volts = %.2f****** \n", battrdg, battvolts);
  if (battvolts >= Batt_HI_Lvl) {
    sprintf(TxString, "E:0:G:%.2f", battvolts);
  } else if (battvolts >= Batt_OK_Lvl) {
    sprintf(TxString, "E:0:Y:%.2f", battvolts);
  } else if (battvolts <= Batt_LO_Lvl) {
    sprintf(TxString, "E:0:R:%.2f", battvolts);
  }
  //Serial.printf("batt str = %s\n", TxString);
  BLETX(TxString);
  batSOC = battvolts * 10;  //should be 25 to 35, over 35 charging, may need tweaked
  if (batSOC < 25) BLETX("SOC:0:R:0");
  else if (batSOC > 35) BLETX("SOC:0:G:CHG");
  else {
    sprintf(TxString, "SOC:0:%s:%d", SOCclrs[batSOC - 25], battState[batSOC - 25]);
    BLETX(TxString);
  }

  //
}

void SoundBuzz(u_long cwFreq, int sound_ms) {
  //sounds cwFreq for sound_ms
  //note--this is blocking
  u_long smillis;
  // Serial.printf(" Freq =%d HZ, for %d ms\n", cwFreq, sound_ms);
  smillis = millis();
  ledcAttach(buzzPin, freq, resolution);   //eight bit resolution--why? (Jun24?)
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
void WrapupTX(void) {
  int i = 0;
  for (i = 0; i < 2; i++) {
    if (i == 0) BLETX("V:Right Percent");
    else BLETX("V:Left Percent");
    delay(1000);
    BLETX("V:On Target %.1f", 0, 0, hands[i].ON_TGT_PCNT);
    delay(1000);
    BLETX("V:Low %.1f", 0, 0, hands[i].LO_PCNT);
    delay(1000);  //need to add in delay time to bletx?
    BLETX("V:High %.1f", 0, 0, hands[i].HI_PCNT);
    delay(1000);  //need to add in delay time to bletx?
  }
}
