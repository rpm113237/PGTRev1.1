
#include <NimBLEDevice.h>
#include <TickTwo.h>
//#include <EasyNeoPixels.h>  //TODO change over to underlying adafruit library
#include "Squeezer.h"
//#include <Adafruit_NeoPixel.h>
//#include <Button2.h>
#include <HX711.h>  //this is non blocking.  hope it works
// #if defined(ESP8266) || defined(ESP32) || defined(AVR)
// #include <EEPROM.h>
// #endif
//I think EEPROM.h installed by default

//ADC calibration
// #include <esp_adc_cal.h>

#include <Preferences.h>
Preferences prefs;

//OTA includes--taken from ElegantOTA demo example
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
WebServer server(80);

// if it is the C3 or other single core ESP32" Use core 0
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else  //otherwise core 1
static const BaseType_t app_cpu = 1;
#endif
//the c3 seems to run on 0 regardless of where it is set.


// define ledticker, battchecker, vibReport--have to have protos in .h
//MeanReport, etc. is called every millis; Actual sending is a timer in the sub
//Haven't looked at library, but I think the call rate of TickTwo isn't modifieable

TickTwo LEDtimer(LEDBlink, 10, 0, MILLIS);  //calls LEDBlink, called every 10MS, repeats forever, resolution MS

TickTwo MeanReport(MeanSend, 1, 0, MILLIS);  //send vibration data every VIB_SND_INTERVAL (ms), forever
TickTwo HFReport(HFSend, 1, 0, MILLIS);
TickTwo FFReport(FFSend, 1, 0, MILLIS);


TickTwo BattChecker(BatSnsCk, Batt_CK_Interval, 0, MILLIS);  //checks battery every Batt_Ck_Interval

TickTwo SleepChecker(RunTimeCheck, 10000, 0, MILLIS);  //check sleeptimers every ten seconds


//construct Button2
//Button2 button;   //StartButton
//comment--why not just handle it as an input? In Rev 1 what does it do other than START?


BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
float txValue = 0;
String rxValue{};  // so can process outside of callback; maybe not the best idea

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
//TODO Get own UID's

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  //  Nordic UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// #define SERVICE_UUID "b56340e0-38a7-11ee-be56-0242ac120002"  // Custom UUID's.  nrfconnect and lightblue require Nordic's UART UUID
// #define CHARACTERISTIC_UUID_RX "b5634374-38a7-11ee-be56-0242ac120002" // to display strings.
// #define CHARACTERISTIC_UUID_TX "b56344a0-38a7-11ee-be56-0242ac120002"




/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device Connected!!");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device Disconnected!!");
    pixels.setPixelColor(LEDSelect, pixels.Color(clrs.RED[0], clrs.RED[1], clrs.RED[2]));
    pixels.show();  // Send the updated pixel colors to the hardware.
  }
  /***************** New - Security handled here ********************
  ****** Note: these are the same return values as defaults ********/
  uint32_t onPassKeyRequest() {
    Serial.println("Server PassKeyRequest");
    return 123456;
  }

  bool onConfirmPIN(uint32_t pass_key) {
    Serial.print("The passkey YES/NO number: ");
    Serial.println(pass_key);
    return true;
  }

  void onAuthenticationComplete(ble_gap_conn_desc desc) {
    Serial.println("Starting BLE work!");
  }
  /*******************************************************************/
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    rxValue = pCharacteristic->getValue();
  }
};

HX711 scale;

// the setup function runs once when you press reset or power the board
void setup() {


  // initialize serial communication at 115200 bits per second:
  u_long lagmsStart = millis();
  Serial.begin(115200);
  //Serial.printf("\nHello from Protocol Grip Trainer v25Jul24, time = \t%lu ms\n", (millis() - lagmsStart));
  Serial.println("Hello; PGT RevLevel =" + REV_LEVEL);
  //NewBLE(SndBle, SndSer, "R:"+ REV_LEVEL); SndBle, SndSer bool, Arg is String(); this sends both.

  ledcAttach(buzzPin, freq, resolution);  //eight bit resolution--why? (Jun24?); using for PWM
  setLED(00, clrs.GREEN);
  LEDBlink();             //Give them a green.
  print_wakeup_reason();  //store wakeup count
  Soundwakeup();          //wake up feedback
  //Serial.printf("After wakeup sound, green led time = %lu ms\n", (millis()-lagmsStart));
  pinMode(StartButton, INPUT);

  //start the TickTwo timers
  LEDtimer.start();     //start LED timer
  //BattChecker.start();  //start Batt checker
  MeanReport.start();   //start Mean  report
  FFReport.start();
  HFReport.start();
  SleepChecker.start();  //start sleepchecker

  // Create the BLE Device
  BLEDevice::init("Squeezer");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    NIMBLE_PROPERTY::NOTIFY
    // BLECharacteristic::PROPERTY_NOTIFY
  );

  // pTxCharacteristic->addDescriptor(new BLE2902());  Not need with NIMBLE

  BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    NIMBLE_PROPERTY::WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  // Serial.printf("BLE advertising time = %lu ms\n", (millis()-lagmsStart));

  //set up flash
  prefs.begin("BatADCScale");  //multiply adc float by this to get voltage
  prefs.begin("SSID");         //prefs library wants length of string
  prefs.begin("PWD");
  prefs.begin("RunTime");  //run time in minutes? 10K minutes = 166hrs.  Not long enough.
  prefs.begin("ScaleScale");
  //move next to connectWiFi??
  strcpy(SSstr, prefs.getString("SSID", DefaultSSID).c_str());  //SSstr is init in squeezer.h
  strcpy(PWDstr, prefs.getString("PWD", DefaultPWD).c_str());   //PWDstr is init in squeezer.h
  Serial.printf("SSID = %s \n", SSstr);
  Serial.printf("PWD = %s \n", PWDstr);

  //set up the scale

  scale.begin(HX711_dout, HX711_sck);
  scale.reset();  //if coming out of deep sleep; scale might be powered down
  // Serial.print("read average of 10--stabilize: \t\t");

  //************* to stand out in log
  scale.tare(NumTare);  // reset the scale to 0
  //Serial.printf("Scale tared, offset = %ld ", scale.get_offset());
  //Serial.printf("Warm up read= %dX\tval = %.2f\tTared %d times\n",NumWarmup, scale.read_average(NumWarmup), NumTare);  // print the average of 10 readings from the ADC
  scaleCalVal = prefs.getFloat("ScaleScale");
  if (isnan(scaleCalVal)) {
    Serial.println("Scale Cal not loaded, use default--check default typicality");
    scaleCalVal = scaleCalDeflt;
  }
  scale.set_scale(scaleCalVal);  //This value needs to be default; a cal function needs to be implemented
  Serial.printf("Scale setup, tared calvalue = %f  time = %lu ms\n", scaleCalVal, (millis() - lagmsStart));

  oldmillis = millis();
  el_time = millis() - oldmillis;
  setLED(500, clrs.BLUE);

  //Big Question:  What is the purpose of running the scale if no BLE connection?  Why not go directly to loop??

  Serial.print("Connecting (BLE)\n");
  BlinkTime = CNCT_LED_BLINK_TIME;
  setLED(250, clrs.BLUE);
  BatSnsCk();  //sets color for connect led
  while (!deviceConnected && (el_time < CONN_WAIT_TM)) {
    if ((el_time % 1000) <= 10) {
      Serial.print(".");
      //pServer->startAdvertising();  // restart advertising
    }

    LEDBlink();  //has to be called, since timer isn't being called?? or call timer?
    delay(10);
    el_time = millis() - oldmillis;
  }

  if (!deviceConnected) {
    Serial.printf("****end of setup; not connected****\n");

  } else {
    Serial.printf("****end of setup; BLE connected****\n");
    strcpy(TxString, ("R:" + REV_LEVEL).c_str());
    Serial.println(TxString);
    setLED(0, clrs.BLUE);  //for ledBlink
  }
  SleepTimer = 0;
  SleepTimerStart = millis() / 1000;  //reset the sleeptimers
  EpochTimeStart = millis();
}

void loop() {
  CheckForce();  //check force updates force structure
  BattChecker.update();   // BatSnsCk checks battery, sends voltage
  LEDtimer.update();      //should call the ledBlink every 10ms.
  FFReport.update();      //sends out FF data current FF Force, current EpochTime= millis()-EpochTimeStart
  HFReport.update();      //send out HoldForce as HF:(String(Force.HFVal))
  MeanReport.update();    // MeanReport
  SleepChecker.update();  //check for timeout
  ResetSwitch();          //check for OFF
  RxStringParse();        //check for orders from the boss (App)
  BLEReconnect();         //TODO does this work???

  //RunTimeCheck();  //checks timeouts, etc.



}  //end of loop
