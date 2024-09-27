void RxStringParse(void) {
  //Serial.printf("length of rxValue in RxParseString = %d\n", rxValue.length());
  String tagStr, valStr;  //C:12.3; C is tag, 12.3 is val
  float FloatVal = 4.2;   //valStr converted to float 4.2 is +/-charge voltage
  if ((digitalRead(StartButton) == LOW)) {
    Serial.printf("Start button is LOW\n");
    GoToSleep();
  }
  // else Serial.printf("Start button is HIGH\n");
  if ((rxValue.length() > 0)) {  //nothing to do if len = 0
    Serial.println("rxValue "+ rxValue);
    int indxsemi = rxValue.indexOf(':');
    Serial.printf("Index of semi colon is %d\n" , indxsemi);
    if (indxsemi <0) indxsemi =1;   // make it work for single tags w/o semi
    tagStr = rxValue.substring(0, indxsemi);
    tagStr.toUpperCase();
    Serial.print("tagstring upper = ");
    Serial.println(tagStr);
    if (rxValue.length()> (indxsemi+1)){
      Serial.println( "A value exists\n");    //leave it as a string
      valStr = rxValue.substring((indxsemi+1), rxValue.length());
      Serial.println("Value String = "+ valStr); 
    }
    //FloatVal = tagStr.toFloat();

    if (tagStr == "X") {
      // Serial.printf("X is found, hooray\n");
      Serial.printf(" Tag 'X' received, Going to Deep Sleep; wakeup by GPIO %d*****\n", StartButton);
      GoToSleep();     
    }
    else if (tagStr == "V") CalibrateADC(valStr);
    else if (tagStr == "S") SetSSID();
    else if (tagStr == "P") SetPwd();
    else if (tagStr == "O") DoOTA();   
    else if (tagStr == "C") CalibrateScale();
    else if (tagStr== "T") DoTare();    //not sure why we need this
       /*Procedure for calibration is:
        --app sends an X for tare.  tare is done on start up, but good practice. Tare takes 20 samples--2 sec +/-
        --app sends X:NN.N.  NN.N is in lbs/kilograms Note: the calibration is actually 
        the scale calibrates, stores the calibration factor.

        */
      // Serial.printf(" C found, do calibration now\n");
      // std::string calstring;  //string that is weight
      // calstring = rxValue.substr(1, rxValue.length());
      // if (calstring.length() > 0) {
      //   uint32_t calweight = uint32_t(stof(calstring) * 10);  //have to divide the readings by ten
      //   Serial.printf("calweight (*10) = %ld\n", calweight);
      //   scale.calibrate_scale(calweight, 20);  //20 times should be plenty
      //   float calcscale = scale.get_scale();
      //   Serial.printf(" calscale = %f \n", calcscale);
      // } else Serial.printf("Zero length cal value string\n");
     
     else {
      Serial.println("Unknown Tag =" + tagStr);
    }
    rxValue.clear();  //erases
  }
}
void CalibrateADC(String strval){  //TODO pass string, do float converstion in procedure
  float batactual = strval.toFloat();
  float adcavg=0;
  int i;
  int numrdgs = 10;
  int adcrdg;
  for (i=1; i<=numrdgs; i++){    //find out how much noise
  adcrdg = (float)analogRead(BatSns);  
  Serial.printf("instantaneous adc rdg = %d\n", adcrdg);
  adcavg = (float) adcrdg+adcavg;
  }
  adcavg = adcavg/10;
  Serial.printf("Calibrate ADC, adcaverage = %f over %d rdgs\n",adcavg,numrdgs);
  BatSnsFactor = batactual/adcavg;  // replace with  calc floatval
  Serial.printf ("BatSns factor = %f\n",BatSnsFactor);
  //TODO 
}
void SetSSID(void) {
  Serial.println ("SetSSID");
  return;
}

void SetPwd(void){
  Serial.println("SetPWD");
  return;
}

void DoOTA(void){
  Serial.println ("DoOTA");
  return;
}
void CalibrateScale(void){
  Serial.println ("CalibrateS cale");
}

void DoTare(void){
   //tare
      Serial.printf("***Doing tare***\n");
      scale.tare(20);  //20X should be plenty
      long scaleoffset = scale.get_offset();
      Serial.printf("Tare done, offset = %ld\n", scaleoffset);
  
}


void ConnectWiFi(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Hello World OTA");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
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
    Serial.println(" setting old device; This should happen once");
    // pixels.setPixelColor(LEDSelect, pixels.Color(clrs.BLUE[0], clrs.BLUE[1], clrs.BLUE[2]));
    // pixels.show();  // Send the updated pixel colors to the hardware.
    setLED(0, clrs.BLUE);
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}


void FindCalibration() {  //never checked out.
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

int floatcomp(const void* elem1, const void* elem2) {
  if (*(const float*)elem1 < *(const float*)elem2)
    return -1;
  return *(const float*)elem1 > *(const float*)elem2;
}

void CheckForce(void) {  //PGT1 changed
  int i = 0;             //force array index
  int arraysz = sizeof(Force.ForceArray) / sizeof(Force.ForceArray[0]);
  if (scale.is_ready()) {  //.get_units is blocking.
    scaleVal = scale.get_units(scaleSamples);
    for (i = (arraysz - 1); i > 0; i--) {
      Force.ForceArray[i] = Force.ForceArray[i - 1];  //move everything up one spot
    }
    Force.ForceArray[0] = scaleVal;
    //now find max, min, mean
    Force.ForceMax = scaleVal;
    Force.ForceMin = scaleVal;
    Force.ForceMean = 0;
    for (i = 0; i < arraysz; i++) {
      if (Force.ForceArray[i] > Force.ForceMax) Force.ForceMax = Force.ForceArray[i];
      if (Force.ForceArray[i] < Force.ForceMin) Force.ForceMin = Force.ForceArray[i];
      Force.ForceMean += Force.ForceArray[i];
    }

    Force.ForceMean = Force.ForceMean / arraysz;
    float wkarray[FORCE_ARRAY_LEN];  //temp array for sorting

    for (i = 0; i < arraysz; i++) wkarray[i] = Force.ForceArray[i];
    qsort(wkarray, arraysz, sizeof(float), floatcomp);
    //   Serial.printf("  sorted\t");
    //  for(i = 0; i < 10; i++)
    //     Serial.printf("%.3f\t", wkarray[i]);
    //  Serial.printf("\n");
    if (arraysz % 2 == 0) Force.ForceMedian = (wkarray[arraysz / 2] + wkarray[arraysz / 2 + 1]) / 2;
    else Force.ForceMedian = wkarray[arraysz / 2 + 1];

    clrTxString();
    sprintf(TxString, "HF:%.1f", scaleVal);
    BLETX();  //transmits TxString
  }
}

void clrTxString(void) {
  int i;
  int Txsize;
  Txsize = sizeof(TxString) / sizeof(TxString[0]);
  for (i = 0; i < Txsize; i++) TxString[i] = 0x00;  //needs cleared for some reason.
}

void VibSend() {
  clrTxString();
  sprintf(TxString, "M:%.1f,%.1f,%.1f", Force.ForceMax, Force.ForceMin, Force.ForceMean);
  BLETX();
  // clrTxString();
  // sprintf(TxString, "MN:%.3f,%.3f", Force.ForceMean, Force.ForceMedian);
  // BLETX();----median not being sent
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
      Serial.printf("Wakeup was not caused by deep sleep; reason code = %d\n", wakeup_reason);
      break;
  }
}

// float ReadVoltage(byte ADC_Pin) {
//   float calibration  = 1.000; // Adjust for ultimate accuracy when input is measured using an accurate DVM, if reading too high then use e.g. 0.99, too low use 1.01
//   float vref = 1100;
//   float voltage_divider_multiplier =  2.174;   //this should be a eeprom stored value in if calibration needed.
//   esp_adc_cal_characteristics_t adc_chars;
//   esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
//   vref = adc_chars.vref; // Obtain the device ADC reference voltage
//   return (analogRead(ADC_Pin) / 4095.0) * 3.3 * voltage_divider_multiplier * (1100 / vref) * calibration;  // ESP by design reference voltage in mV
// }

void BatSnsCk(void) {
  //reads the ADC on BatSns
  int battpcnt;
  int battvoltx100 = 0;  //batt volts *100, rounded to int
  int battrdg = 0;       //raw
  battrdg = analogRead(BatSns);
  //battvolts = ((float)battrdg * 2 * 3.2 * 210 / 310) / 4095;  //fudge; something wrong with voltage divider--impedance too high--r11 = 210k? WTF??
  battvolts = ((float)battrdg / 955);  //fudge for now; later do a calibration routine
  Serial.printf("******Battery: Raw = %d;  volts = %.2f****** \n", battrdg, battvolts);
  //sprintf(TxString, "E:%.2f", battvolts);
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

  if (battvoltx100 < 371) UseRedLED = true;
  if (battvoltx100 < 3.5) {
    GoToSleep();
  }
  sprintf(TxString, "BP:%d,%d", battpcnt, (battpcnt * BattFullTime) / 100);
  BLETX();
  //
}

void GoToSleep(void) {
  scale.power_down();
  MorseChar(SHAVE_HAIRCUT);
  Serial.printf("******Low Battery Deep Sleep; wakeup by GPIO %d*****\n", StartButton);
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
