void RxStringParse(void) {

  String tagStr = "";
  String valStr = "";
  ;  //C:12.3; C is tag, 12.3 is val

  if ((rxValue.length() > 0)) {  //nothing to do if len = 0
    //Serial.println("rxValue " + rxValue);
    int indxsemi = rxValue.indexOf(':');            //-1 if no semi
    if (indxsemi < 0) indxsemi = rxValue.length();  // make it work for single tags w/o semi
    tagStr = rxValue.substring(0, indxsemi);
    tagStr.toUpperCase();
    //Serial.print("tagstring upper = ");
    //Serial.println(tagStr);
    if (rxValue.length() > (indxsemi + 1)) {
      valStr = rxValue.substring((indxsemi + 1), rxValue.length());
    } else valStr = "";  //null for checking
    if (tagStr == "X") GoToSleep("The Boss said GoodNight, power off");
    else if (tagStr == "S") SetSSID(valStr);
    else if (tagStr == "P") SetPwd(valStr);
    else if (tagStr == "O") DoOTA();
    else if (tagStr == "C") CalibrateScale(valStr);
    else if (tagStr == "TR") DoTare();                       //not sure why we need this
    else if (tagStr == "FRQ") SetFFRate(valStr);             //Start sending FF data at rate int valstr
    else if (tagStr == "BP") StringBLETX(BatSnsCk(), true);  // query only.
    else if (tagStr == "ET") SetEpochTime(valStr);
    else if (tagStr == "R") StringBLETX(("R:" + REV_LEVEL), true);  // this is a report--
    // else if (tagStr == "V") CalibrateADC(valStr);
    else if (tagStr == "HF") SetHFRate(valStr);  //Set HF reporting Rate
    // else if (tagStr == "MR") SetMeanReportTime(valStr);  //Set Rate at which Mean is reported
    else if (tagStr == "MT") SetMeanTime(valStr);      // Seet Time over which Mean is calculated.
    else if (tagStr == "NUM") SetBootNum(valStr);      //Set BootNum; if null reset
    else if (tagStr == "TT") SetTotalRunTime(valStr);  //set Total Run Time; if null reset
    else Serial.println("Unknown Tag =" + tagStr);
    rxValue.clear();  //erases
  }
}

void SetBootNum(String valStr) {  //mean calculation time (seconds)  
  bootCount = atoi(valStr.c_str());   //atoi returns zero if invalid  
  prefs.putUInt("NumBoots", bootCount);
}

void SetTotalRunTime(String valStr) {  //mean calculation time (seconds)  
char *end;
  Force.TotalRuntime = strtoll(valStr.c_str(), &end,0);   //atoi returns zero if invalid  
  prefs.putUInt("TotalRunTime", Force.TotalRuntime);
  //end of run will update to time in THIS run.
}


void SetMeanTime(String valStr) {  //mean calculation time (seconds)
  if (valStr.length() > 0) {
    if (atoi(valStr.c_str()) > 0) {
      Force.MeanTime = atoi(valStr.c_str());
      Force.MeanReportTime = MS_TO_SEC * Force.MeanTime;  //note minimum of one second
      Force.MNNSamp = BaseSampleRate * Force.MeanTime;
      Force.MeanReport = true;
    }
    //if samp rate = 80; rate = 5--> scaleSamples = 16
  } else {
    Force.MeanReport = false;
  }
}

void SetHFRate(String valStr) {
  if (valStr.length() > 0) {
    if (atoi(valStr.c_str()) > 0) {
      Force.HFRate = atoi(valStr.c_str());
      Force.HFReportTime = 1000 / Force.HFRate;  //this could be made indpendablely settable.
      Force.HFReport = true;
    }
    //if samp rate = 80; rate = 5--> scaleSamples = 16
  } else {
    Force.HFReport = false;
  }
}

void SetFFRate(String valStr) {

  if (valStr.length() > 0) {
    if (atoi(valStr.c_str()) > 0) {
      Force.FFRate = atoi(valStr.c_str());
      Force.FFReportTime = MS_TO_SEC / Force.FFRate;  //this could be made settable.
      Force.FFReport = true;
      Force.EpochStart = millis();
      Force.EpochTime = millis() - Force.EpochStart;
    }
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
    Force.EpochStart = atoi(valStr.c_str()) + millis();

  } else Force.EpochStart = millis();
  //Force.EpochTime - millis() - Force.EpochStart;
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

  if ((digitalRead(StartButton) == HIGH)) {
    //Serial.printf("Start button is LOW, Go To Sleep\n");
    GoToSleep("Start button is PRESSED, Power Down\t TODO flash boot count");
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
                                      //TODO--if strVal len = 0, default to 3.3V
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
  Serial.println(ssid);
  StringBLETX("IP:" + String(WiFi.localIP()), true);
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
  //Serial.printf("BLEreconnect; deviceConnected = %d\t oldDeviceConnected = %d\n", deviceConnected, oldDeviceConnected);
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
    timesInit();  //reset the world to the connect time.
    Serial.println("Revision level = " + REV_LEVEL);
    setLED(0, clrs.BLUE);  //for ledBlink
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

float cumAvg(float oldAvg, float newForce, int NumSamps) {
  /*calculates cum avg ca(n+1)= ca(n)+ (hf(n+1)-ca(n))/(n+1)
  where n is the number of samples==samprate * seconds for mean
  https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
  */
  float newavg = oldAvg + (newForce - oldAvg) / (NumSamps + 1);
  //Serial.printf("\toldavg = %.2f\tnewForce = %.2f\tSamps =%d\tNewAvg = %.2f\n", oldAvg,newForce, NumSamps, newavg);
  return newavg;
}



void CheckForce(void) {  //PGT1 changed

  if (scale.is_ready()) {           //.get_units is blocking.
    scaleVal = scale.get_units(1);  //get samples as fast as available

    //Force.BaseVal = scaleVal;
    Force.FFVal = cumAvg(Force.FFVal, scaleVal, Force.BaseRate / Force.FFRate);  // baserate/ffrate = num samples for avg.
    Force.HFVal = cumAvg(Force.HFVal, scaleVal, Force.BaseRate / Force.HFRate);
    Force.MeanVal = cumAvg(Force.MeanVal, scaleVal, Force.BaseRate * Force.MeanTime);
    if (Force.HFVal > MinForce) SleepTimerStart = millis() / 1000;  //reset sleep timer.Reset this on HF; base rate may be pretty noisy
  }
}

void MeanSend(unsigned long ET) {
  static bool sentalready = false;
  if (!Force.MeanReport) return;
  if ((ET % Force.MeanReportTime <= 1) && !sentalready) {                            //Mean Time
    Serial.printf("MNsend: MN = %.2f\tEpoch Time (ms) = %lu\n", Force.MeanVal, ET);  //diagnostic
    StringBLETX("MN:" + String(Force.MeanVal), true);
    sentalready = true;
  }
  if ((ET % Force.MeanReportTime > 1)) sentalready = false;
  //sends out mean if mean interval has elapsed
}

void FFSend(unsigned long ET) {  //TODO--this has to be in Carter's Format
  static bool sentalready = false;
  unsigned long wkul = 123456;
  if (!Force.FFReport) return;
  if ((ET % Force.FFReportTime <= 1) && !sentalready) {
    Serial.printf("FFTSnd: ET= %lu\t FF = %.2f\n", ET, Force.FFVal);  //diagnostic
    //StringBLETX("FF:" + String(getEpochTime()) + "," + String(Force.FFVal));
    wkul = getEpochTime();
    memcpy(TxString, &wkul, sizeof(unsigned long));
    memcpy((TxString + sizeof(unsigned long)), &Force.FFVal, sizeof(float));
    StringBLETX(String(TxString, HEX), false);  //don't send serial in BLETX
    sentalready = true;
  }
  if ((ET % Force.FFReportTime > 1)) sentalready = false;
}

void HFSend(unsigned long ET) {
  // rate = HFReport time--default 200ms
  static bool sentalready = false;
  if (!Force.HFReport) return;
  if ((ET % Force.HFReportTime <= 1) && !sentalready) {
    Serial.printf("HFsend: HF = %.2f\tEpoch Time (ms) = %lu\n", Force.HFVal, ET);  //diagnostic
    StringBLETX("HF:" + String(Force.HFVal), true);
    sentalready = true;
  }
  if ((ET % Force.HFReportTime > 1)) sentalready = false;
}

void setLED(int btime, int clrarray[3]) {  //incorporate into LEDBlink
  BlinkTime = btime;                       //passed in commmon
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


void StringBLETX(String msg, bool SndSer) {
  if (msg.length() > 19) Serial.println("String too long, truncated, length = " + String(msg.length()));
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg);
    pTxCharacteristic->notify();
    if (SndSer) Serial.println("BLE:\t" + msg);
    //delay(1000);
    // bluetooth stack will go into congestion, if too many packets are sent  }
  } else if (SndSer) Serial.println("OFFline:\t" + msg);  //maybe need a debug switch
}



void print_wakeup_reason() {

  //see https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/reset_reason.html
  esp_reset_reason_t reset_reason = esp_reset_reason();  //this should only be for non deep sleep.

  switch (reset_reason) {
    case ESP_RST_UNKNOWN:
      Serial.println("Reset reason can not be determined.");
      break;
    case ESP_RST_POWERON:
      Serial.println("Reset due to power-on event.");
      break;
    case ESP_RST_SW:
      Serial.println("Software Reset via esp_restart");
      break;
    case ESP_RST_PANIC:
      Serial.println("Software Exception due to exception/panic");
      break;

    default:
      Serial.printf("Reset Default reason code = %d\n", reset_reason);
      break;
  }
}


String BatSnsCk(void) {

  int battpcnt;
  int battvoltx100 = 0;                               //batt volts *100, rounded to int
  battvolts = adcSensor.getSingleEndedMillivolts(0);  //Batt_SW is on channel zero.
  Serial.printf("VBAT_SW = %.0f millivolts\n", battvolts);

  //reference: https://blog.ampow.com/lipo-voltage-chart/
  battvoltx100 = roundf(battvolts / 10);  //battvolts is in mv now
  battpcnt = 0;                           //default
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

  //Serial.printf("ADCRaw = %.1f\tbatt mult = %f\t batt volts = %.3f \t numrdgs = %d\t", battrdg, BatSnsFactor, battvolts, NumADCRdgs);
  //Serial.println("BP:" + String(battpcnt) + String((battpcnt * BattFullTime) / 100));
  if (battpcnt < BattShutDownPcnt) GoToSleep(" Battery critically low = " + String(battvolts) + "millivolts; going to sleep");
  setLED(BlinkTime, clrs.BLUE);
  if (battpcnt < BattWarnPcnt) setLED(BlinkTime, clrs.YELLOW);
  if (battpcnt < BattCritPcnt) setLED(BlinkTime, clrs.RED);
  return String("BP:" + String(battpcnt) + String((battpcnt * BattFullTime) / 100));
}

void GoToSleep(String DSmsg) {

  Serial.println(DSmsg);  // tell why shutting down.
  unsigned long runtime = (Force.TotalRuntime + (millis() - Force.EpochStart) / (unsigned long)(60000));
  // Serial.println("Elapsed time in ms this run = "+ String(millis()-Force.EpochStart));
  // Serial.println("Force.TotalRuntime @ shutdown =" + String(runtime));
  prefs.putULong("TotalRunTime", runtime);
  MorseChar(SHAVE_HAIRCUT);
  digitalWrite(ShutDownPin, HIGH);  //power off
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
  Serial.printf("idle time = %d seconds\n", runtimeseconds);
  if ((runtimeseconds > (SleepTimeMax - 30)) && (firsttime == true)) {
    Serial.println("Idlewarning");
    MorseChar('s');
    MorseChar('o');
    MorseChar('s');
    firsttime = false;
  }
  if (runtimeseconds > SleepTimeMax) {
    //Serial.printf("No activity for %lu seconds, go to sleep\n", SleepTimeMax);
    GoToSleep("No activity for " + (String)SleepTimeMax + "seconds, power down");
  }
}

void timesInit(void) {
  //init Force struct
  if (Force.FFRate > 0) {
    Force.FFReportTime = MS_TO_SEC / Force.FFRate;
    Force.FFNSamp = BaseSampleRate / Force.FFRate;
  } else Serial.println("FFrate screwed; <=0 = " + String(Force.FFRate));

  if (Force.HFRate > 0) {
    Force.HFReportTime = MS_TO_SEC / Force.HFRate;
    Force.HFNSamp = BaseSampleRate / Force.HFRate;
  } else Serial.println("HFrate screwed; <=0 = " + String(Force.HFRate));

  if (Force.MeanTime > 0) {
    Force.MeanReportTime = MS_TO_SEC * Force.MeanTime;  //note minimum of one second
    Force.MNNSamp = BaseSampleRate * Force.MeanTime;
  } else Serial.println("Mean Time screwed; <=0 = " + String(Force.FFRate));

  //Force struct initialized

  SleepTimer = 0;
  SleepTimerStart = millis() / 1000;  //reset the sleeptimers
  Force.EpochStart = millis();
  Force.FFLastReport = millis();
  Force.HFLastReport = millis();
  Force.MeanLastReport = millis();
}

void setFlashDefaults(void)  //sets Scale defaults, prints out bootnum, runtime
{

  scaleCalVal = prefs.getFloat("ScaleScale");
  if (isnan(scaleCalVal) || (scaleCalVal <= 0)) {  //scale cal should always be positive
    scaleCalVal = scaleCalDeflt;
    prefs.putFloat("ScaleScale", scaleCalDeflt);
    Serial.println("Scale Cal not initialized, using default = " + String(scaleCalVal, 2));
  } else Serial.println("Scale flash calibration = " + String(scaleCalVal));
  scale.set_scale(scaleCalVal);
  bootCount = prefs.getUInt("NumBoots") + 1;
  prefs.putUInt("NumBoots", bootCount);
  Serial.println("Boot Number = " + String(bootCount));
  Force.TotalRuntime = prefs.getULong("TotalRunTime");
  Serial.println("Total Run Time @ boot = " + String(Force.TotalRuntime));
}
