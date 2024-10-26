void RxStringParse(void) {

  String tagStr = "";
  String valStr = "";
  ;  //C:12.3; C is tag, 12.3 is val

  if ((rxValue.length() > 0)) {  //nothing to do if len = 0
    Serial.println("rxValue " + rxValue);
    int indxsemi = rxValue.indexOf(':');            //-1 if no semi
    if (indxsemi < 0) indxsemi = rxValue.length();  // make it work for single tags w/o semi
    tagStr = rxValue.substring(0, indxsemi);
    tagStr.toUpperCase();
    //Serial.print("tagstring upper = ");
    //Serial.println(tagStr);
    if (rxValue.length() > (indxsemi + 1)) {
      valStr = rxValue.substring((indxsemi + 1), rxValue.length());
    } else valStr = "";  //null for checking


    if (tagStr == "X") GoToSleep("App X cmd , Going to Deep Sleep");
    else if (tagStr == "S") SetSSID(valStr);
    else if (tagStr == "P") SetPwd(valStr);
    else if (tagStr == "O") DoOTA();
    else if (tagStr == "C") CalibrateScale(valStr);
    else if (tagStr == "TR") DoTare();            //not sure why we need this
    else if (tagStr == "FRQ") SetFFRate(valStr);  //Start sending FF data at rate int valstr
    else if (tagStr == "BP") BatSnsCk();          // query only.
    else if (tagStr == "ET") SetEpochTime(valStr);
    else if (tagStr == "R") StringBLETX("R:" + REV_LEVEL);  // this is a report--
    else Serial.println("Unknown Tag =" + tagStr);
    rxValue.clear();  //erases
  }
}

void SetFFRate(String valStr) {

  if (valStr.length() > 0) {
    Force.FFRate = atoi(valStr.c_str());
    Force.FFReportTime = 1000 / Force.FFRate;  //this could be made settable.
    Force.FFReport = true;
    Force.EpochStart = millis();
    Force.EpochTime = millis() - Force.EpochStart;
    //if samp rate = 80; rate = 2--> scaleSamples = 40
  } else {
    Force.FFReport = false;  //if sample rate = 10, rate = 2, scaleSamples = 5
  }
}

unsigned long getEpochTime(void) {
  return (millis() - Force.EpochStart);
}

void SetEpochTime(String valStr) {
  //set EpochTime to valStr; if ValStr = null, set it to zero
  if (valStr.length() > 0) {
    EpochTime = atoi(valStr.c_str());
  } else EpochTime = 0;
}


float getFloatADC(int numtimes) {
  float adcavg = 0.0;
  int i;  //old FORTRAN pgmr
  for (i = 1; i <= numtimes; i++) {
    adcavg = adcavg + (float)analogRead(BatSns);
  }
  return adcavg / numtimes;
}

void CalibrateADC(String strval) {                              //TODO pass string, do float converstion in procedure
  float BatCalValue = 3.75;                                     //3.75 is default--TODO--move to defines
  if (strval.length() > 0) BatCalValue = atof(strval.c_str());  //stops at first non numeric
                                                                // Serial.printf("Use Batt Value of %f volts for calibaration",BatCalValue);
  int numrdgs = 10;                                             //probably needs to be in squeezer.h TODO
  float adcrdg = getFloatADC(numrdgs);
  BatSnsFactor = BatCalValue / adcrdg;  // l
  Serial.printf("Calibrate ADC to %f Volts, CalFactor = %f adcraw average = %f over %d rdgs\n", BatCalValue, BatSnsFactor, adcrdg, numrdgs);
  prefs.putFloat("BatADCScale", BatSnsFactor);
  //TODO put the 3.75 default  and numrdg in defines
}
void SetSSID(String ValStr) {
  //S:Valstr; if Valstr ="", then revert to default--is this a good idea??
  if (ValStr.length() > 0) strcpy(SSstr, ValStr.c_str());
  else strcpy(SSstr, DefaultSSID.c_str());
  String tempStr = String(SSstr);    //new or default,
  prefs.putString("SSID", tempStr);  //store in flash as c++ String
  Serial.println("SetSSid string = " + tempStr);
  return;
}

void SetPwd(String ValStr) {

  //S:Valstr; if Valstr ="", then revert to default--is this a good idea??
  if (ValStr.length() > 0) strcpy(PWDstr, ValStr.c_str());
  else strcpy(PWDstr, DefaultPWD.c_str());
  String tempStr = String(PWDstr);  //new or default,
  prefs.putString("PWD", tempStr);  //store in flash as c++ String
  Serial.println("SetPWD string = " + tempStr);
  return;
}

void ResetSwitch(void) {

  if ((digitalRead(StartButton) == LOW)) {
    //Serial.printf("Start button is LOW, Go To Sleep\n");
    GoToSleep("Start button is LOW, Go To Sleep");
  }
}

void DoOTA(void) {
  Serial.println("DoOTA");
  ConnectWiFi();
  Serial.println("Connected, hung up awaiting boot");
  SleepTimerStart = millis() / 1000;  //reset sleep timer, give five minutes to connect
  while (1) {
    server.handleClient();
    ElegantOTA.loop();
    SleepChecker.update();  //check for timeout
  }
  return;
}
void CalibrateScale(String strval) {  //C:float known weight
                                      //assumes tared prior to known weight attached
                                      //we want to get to .01 lbs, multiply weight by 100; then multiply scale factor by 100
  int calweight = roundf(100.0 * strval.toFloat());
  scale.calibrate_scale(calweight, NumTare);
  scaleCalVal = scale.get_scale() * 100.0;  //adjust for 100X
  scale.set_scale(scaleCalVal);
  prefs.putFloat("ScaleScale", scaleCalVal);
  Serial.printf("Calibration done; calweight = %f;  scale factor = %f\n", calweight / 100.0, scaleCalVal);
}

void DoTare(void) {
  //tare
  Serial.printf("***Doing tare***\n");
  scale.tare(20);  //20X should be plenty
  long scaleoffset = scale.get_offset();
  Serial.printf("Tare done, offset = %ld\n", scaleoffset);
}


void ConnectWiFi(void) {
  // ssid = (prefs.getString("SSID","" )).c_str();    //stored as cpp String
  // if (ssid == ""){
  //   Serial.printf("No ssid stored, using default = %s\n", DefaultSSID);
  // }
  WiFi.mode(WIFI_STA);
  //ssid, password are pointers used by WiFi; they point to SSstr & PWDstr, respectively
  strcpy(SSstr, prefs.getString("SSID", DefaultSSID).c_str());  //SSstr is init in squeezer.h
  strcpy(PWDstr, prefs.getString("PWD", DefaultPWD).c_str());   //PWDstr is init in squeezer.h
  //Serial.printf("SSID = %s, Pwd = %s\n", SSstr, PWDstr);
  WiFi.begin(ssid, password);
  Serial.printf("Waiting to connect to SSID = %s, Pwd = %s\n", SSstr, PWDstr);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ; IP Address = ");
  Serial.print(ssid);
  // Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", []() {
    server.send(200, "text/plain", "Hello from PGT Rev1 ElegantOTA!!!");
  });

  ElegantOTA.begin(&server);  // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}


void BLEReconnect(void) {
  //attempt to reconnect
  //not sure if it works.
  if (!deviceConnected && oldDeviceConnected) {
    delay(10);                    // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println(" in loop reconnect ;start advertising");
    setLED(250, clrs.BLUE);
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Connected; setting old device; This should happen once");
    timesInit();    //reset the world to the connect time.
    Serial.println("Revision level = " + REV_LEVEL);
    setLED(0, clrs.BLUE);  //for ledBlink
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

float cumAvg(float oldAvg, float newForce, int NumSamps) {
  /*calculates cum avg ca(n+1)= ca(n)+ (hf(n+1)-ca(n))/(n+1)
  where n is the number of samples==samprate * seconds for mean
  */
  // ForceMean = ForceMean + (force-ForceMean)/(scaleSamples);
  float newavg = oldAvg + (newForce - oldAvg) / (NumSamps + 1);
  //Serial.printf("\toldavg = %.2f\tnewForce = %.2f\tSamps =%d\tNewAvg = %.2f\n", oldAvg,newForce, NumSamps, newavg);
  return newavg;
}



void CheckForce(void) {  //PGT1 changed

  if (scale.is_ready()) {           //.get_units is blocking.
    scaleVal = scale.get_units(1);  //get samples as fast as available

    Force.BaseVal = scaleVal;
    Force.FFVal = cumAvg(Force.FFVal, scaleVal, Force.BaseRate / Force.FFRate);
    Force.HFVal = cumAvg(Force.HFVal, scaleVal, Force.BaseRate / Force.HFRate);
    Force.MeanVal = cumAvg(Force.MeanVal, scaleVal, Force.BaseRate * Force.MeanTime);
    if (Force.HFVal > MinForce) SleepTimerStart = millis() / 1000;  //reset sleep timer.Reset this on HF; base rate may be pretty noisy
  }
}

void MeanSend(void) {
  //called every millisecond, actually sends every MeanTime*1000
  unsigned long elmillis;
  if (!Force.MeanReport) return;
  elmillis = millis() - Force.MeanLastReport;  //incremented every call--which is once per ms
  if (elmillis >= Force.MeanReportTime) {
    Force.MeanLastReport = millis();
    Serial.printf("Meansend, Elapsed ms = %lu\t", elmillis);  //diagnostic
    Serial.printf("Epoch Time = %lu\t", getEpochTime());
    StringBLETX("M:" + String(Force.MeanVal));                  //note that this can be stale by up to 1ms.

  }  //sends out mean if mean interval has elapsed
}


void FFSend(void) {  //TODO--this has to be in Carter's Format
  // rate = FFReport time--default 100ms??

  unsigned long elmillis = millis() - Force.FFLastReport;
  if (!Force.FFReport) return;  //ReportFF is set/reset by FRQ:XXXX
  if (elmillis >= Force.FFReportTime) {
    Force.FFLastReport = millis();
    Serial.printf("FFsend: FF = %.2f\tElapsed ms = %lu\t", Force.FFVal, elmillis);  //diagnostic
    Serial.printf("Epoch Time = %lu\t", getEpochTime());
    StringBLETX("FF:" + String(getEpochTime()) + String(Force.FFVal));
  }
}




void HFSend(void) {
  // rate = HFReport time--default 200ms
  unsigned long elmillis = 0;
  if (!Force.HFReport) return;
  //Serial.printf("HFsend, ms = %d\n", milliscount);
  //milliscount++;  //incremented every call--which is once per ms
  elmillis = (millis() - Force.HFLastReport);
  if (elmillis >= Force.HFReportTime) {
    Force.HFLastReport = millis();
    Serial.printf("HFsend, Elapsed ms = %lu\t", elmillis);  //diagnostic
    Serial.printf("Epoch Time = %lu\t", getEpochTime());
    StringBLETX("HF:" + String(Force.HFVal));
  }
}

void clrTxString(void) {
  int i;
  int Txsize;
  Txsize = sizeof(TxString) / sizeof(TxString[0]);
  for (i = 0; i < Txsize; i++) TxString[i] = 0x00;  //needs cleared for some reason.
}

void setLED(int btime, int clrarray[3]) {  //incorporate into LEDBlink
  BlinkTime = btime;                         //passed in commmon
  for (int i = 0; i < 3; i++) { clrs.WKCLRS[i] = clrarray[i]; }
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


void StringBLETX(String msg) {
  if (msg.length() > 19) Serial.println("String too long, truncated, length = " + String(msg.length()));
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg);
    pTxCharacteristic->notify();
    Serial.println("BLE:\t" + msg);
    //delay(1000);
    // bluetooth stack will go into congestion, if too many packets are sent  }
  } else Serial.println("OFFline:\t" + msg);  //maybe need a debug switch
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
      Serial.printf("Wakeup was not caused by deep sleep; reason code = %d\n", wakeup_reason);
      break;
  }
}



void BatSnsCk(void) {

  int battpcnt;
  int battvoltx100 = 0;  //batt volts *100, rounded to int
  float battrdg = 0;     //raw
  BatSnsFactor = prefs.getFloat("BatADCScale");
  if (isnan(BatSnsFactor)) {
    BatSnsFactor = BatMultDefault;
    Serial.printf("ADC Mult not initialized, using default = %f\n", BatSnsFactor);
  }

  battrdg = getFloatADC(NumADCRdgs);
  battvolts = battrdg * BatSnsFactor;
  Serial.printf("floatADCRaw = %f\tbatt mult = %f\t batt volts = %f \t numrdgs = %d\n", battrdg, BatSnsFactor, battvolts, NumADCRdgs);

  //reference: https://blog.ampow.com/lipo-voltage-chart/
  battvoltx100 = roundf(battvolts * 100);
  if (battvoltx100 >= 420) battpcnt = 100;
  else if (battvoltx100 > 415) battpcnt = 95;
  else if (battvoltx100 > 411) battpcnt = 90;
  else if (battvoltx100 > 408) battpcnt = 85;
  else if (battvoltx100 > 402) battpcnt = 80;
  else if (battvoltx100 > 398) battpcnt = 75;
  else if (battvoltx100 > 395) battpcnt = 70;
  else if (battvoltx100 > 391) battpcnt = 65;
  else if (battvoltx100 > 387) battpcnt = 60;
  else if (battvoltx100 > 385) battpcnt = 55;
  else if (battvoltx100 > 384) battpcnt = 50;
  else if (battvoltx100 > 382) battpcnt = 45;
  else if (battvoltx100 > 380) battpcnt = 40;
  else if (battvoltx100 > 379) battpcnt = 35;
  else if (battvoltx100 > 377) battpcnt = 30;
  else if (battvoltx100 > 375) battpcnt = 25;
  else if (battvoltx100 > 373) battpcnt = 20;
  else if (battvoltx100 > 371) battpcnt = 15;
  else if (battvoltx100 > 369) battpcnt = 10;
  else if (battvoltx100 > 361) battpcnt = 05;

  if (battvoltx100 < 377) setLED(0, clrs.YELLOW);
  if (battvoltx100 < 371) setLED(0, clrs.RED);
  if (battvoltx100 < 365) {
    GoToSleep(" Battery critically low = " + String(battvoltx100/100) + " ,volts; going to sleep");
  }
  StringBLETX("BP:" + String(battpcnt) + String((battpcnt * BattFullTime) / 100));
}



void GoToSleep(String DSmsg) {
  scale.power_down();
  Serial.println(DSmsg);                                                                 // tell why shutting down.
  pixels.setPixelColor(LEDSelect, pixels.Color(clrs.OFF[0], clrs.OFF[1], clrs.OFF[2]));  //turn LED's OFF
  pixels.show();                                                                         // Send the updated pixel colors to the hardware.
  MorseChar(SHAVE_HAIRCUT);
  //Serial.printf("******Low Battery Deep Sleep; wakeup by GPIO %d*****\n", StartButton);
  esp_deep_sleep_enable_gpio_wakeup(1 << StartButton, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
  Serial.println("This never happens");
}

void SoundBuzz(u_long cwFreq, int sound_ms) {
  //sounds cwFreq for sound_ms
  //note--this is blocking
  u_long smillis;
  // Serial.printf(" Freq =%d HZ, for %d ms\n", cwFreq, sound_ms);
  smillis = millis();
  ledcAttach(buzzPin, freq, resolution);  //eight bit resolution--why? (Jun24?)
  ledcWrite(buzzPin, dutycycle);
  while ((millis() - smillis) < sound_ms) {}
  ledcWrite(buzzPin, 0);  //off
}

void Soundwakeup(void) {
  //the start button has been pushed
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

void RunTimeCheck() {
  //increments sleep timer by seconds
  static bool firsttime = true;
  int runtimeseconds = (millis() / 1000 - SleepTimerStart);
  Serial.printf("\t\tidle time = %d seconds\n", runtimeseconds);
  if ((runtimeseconds > (SleepTimeMax - 30)) && (firsttime == true)) {
    Serial.println("Idlewarning");
    MorseChar('s');
    MorseChar('o');
    MorseChar('s');
    firsttime = false;
  }
  if (runtimeseconds > SleepTimeMax) {
    //Serial.printf("No activity for %lu seconds, go to sleep\n", SleepTimeMax);
    GoToSleep("No activity for " + (String)SleepTimeMax + "seconds, go to sleep");
  }
}

void timesInit(void) {
  SleepTimer = 0;
  SleepTimerStart = millis() / 1000;  //reset the sleeptimers
  Force.EpochStart = millis();
  Force.FFLastReport = millis();
  Force.HFLastReport = millis();
  Force.MeanLastReport = millis();
}
