
#include <stdint.h>
#include <NimBLEDevice.h>
#include <TickTwo.h>
#include "Squeezer.h"
#include <Adafruit_NeoPixel.h>
#include <HX711.h>  
#include <SparkFun_ADS1015_Arduino_Library.h>  //See Sketch ADS1015_RPM_Multichannel
#include <Wire.h>
#include <Preferences.h>
//OTA includes--taken from ElegantOTA demo example
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

// if it is the C3 or other single core ESP32" Use core 0
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else  //otherwise core 1
static const BaseType_t app_cpu = 1;
#endif
//the c3 seems to run on 0 regardless of where it is set.

ADS1015 adcSensor;

Preferences prefs;

WebServer server(80);

HX711 scale;

Adafruit_NeoPixel pixels(NEOPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);  //1 ea sk6812 on IO 8


TickTwo LEDtimer(LEDBlink, 10, 0, MILLIS);  //calls LEDBlink, called every 10MS, repeats forever, resolution MS
TickTwo BattChecker(BatSnsCk, Batt_CK_Interval, 0, MILLIS);  //checks battery every Batt_Ck_Interval
TickTwo SleepChecker(RunTimeCheck, 10000, 0, MILLIS);  //check sleeptimers every ten seconds

BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
float txValue = 0;
String rxValue{};  // so can process outside of callback; maybe not the best idea

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

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


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 115200 bits per second:
  u_long lagmsStart = millis();
  pinMode(StartButton, INPUT);
  pinMode(ShutDownPin, OUTPUT);    //pulled down with 100K, has to be active high
  digitalWrite(ShutDownPin, LOW);  // race against release. switch should still be closed.
  Serial.begin(115200);
  while (digitalRead(StartButton) == HIGH);// Serial.println("Waiting Start Button Release");
  Serial.println("Hello; PGT RevLevel =" + REV_LEVEL);

  ledcAttach(buzzPin, freq, resolution);  //eight bit resolution--why? (Jun24?); using for PWM
  setLED(00, clrs.GREEN);
  LEDBlink();             //Give them a green.  
  Soundwakeup();          //wake up feedback

  print_wakeup_reason();  //output and store wakeup count

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
   BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    NIMBLE_PROPERTY::WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());
  // Start the service
  pService->start();
  // Start advertising
  pServer->getAdvertising()->start();
  // Serial.printf("BLE advertising time = %lu ms\n", (millis()-lagmsStart));

  //setup ADS1015
  Wire.begin(sda_rpm, scl_rpm);
  if (adcSensor.begin(ADC_addr_rpm) == true)  // 0x48 is default
  {
    Serial.println("ADS1015 ADC found. I2C connections are good.");
  } else {
    Serial.println("ADS1015 not found. Check wiring. Stalled forever");
    while (1);  // stall out forever
  }
  // Set the Gain
  adcSensor.setGain(ADS1015_CONFIG_PGA_TWOTHIRDS);
  //set the sample rate
  adcSensor.setSampleRate(ADS1015_CONFIG_RATE_1600HZ);

  // For the fastest conversion timing, we need to check the Config Register Operational Status bit
  // to see when the conversion is complete - instead of using a fixed delay.
  adcSensor.useConversionReady(true);  
  //Serial.printf("After wakeup sound, green led time = %lu ms\n", (millis()-lagmsStart));
    
  //start the TickTwo timers
  LEDtimer.start();      //start LED timer
  BattChecker.start();   //start Batt checker
  SleepChecker.start();  //start sleepchecker
  //set up flash
  //prefs.begin("BatADCScale");  //multiply adc float by this to get voltage
  // Serial.println( "ADCalibration at start up =" +String(prefs.getFloat("BatADCScale")));
  prefs.begin("SSID");  //prefs library wants length of string
  prefs.begin("PWD");
  prefs.begin("TotalRunTime");  //run time in minutes? 10K minutes = 166hrs.  Not long enough.
  prefs.begin("ScaleScale");
  prefs.begin("NumBoots"); //number of time the unit has powered up
  //Serial.println( "Scale Calibration at start up =" + String(prefs.getFloat("ScaleScale")));
  setFlashDefaults();

  //move next to connectWiFi??
  strcpy(SSstr, prefs.getString("SSID", DefaultSSID).c_str());  //SSstr is init in squeezer.h
  strcpy(PWDstr, prefs.getString("PWD", DefaultPWD).c_str());   //PWDstr is init in squeezer.h
  Serial.printf("SSID = %s \n", SSstr);
  Serial.printf("PWD = %s \n", PWDstr);

  //set up the scale
  pinMode(RatePin, OUTPUT);
  if (BaseSampleRate == 10) digitalWrite(RatePin, LOW);
  else if (BaseSampleRate == 80) digitalWrite(RatePin, HIGH);
  else Serial.println("Unknown Base Rate =" + String(BaseSampleRate));

  scale.begin(HX711_dout, HX711_sck);
  scale.reset();  //if coming out of deep sleep; scale might be powered down
  scale.tare(NumTare);  // reset the scale to 0
  
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

  Serial.print("....Advertising for BLE connection.......\n");
  BlinkTime = CNCT_LED_BLINK_TIME;
  setLED(250, clrs.BLUE);  //blinking for connect
  Serial.println("Revision level = " + REV_LEVEL);
  Serial.println(BatSnsCk());  //sets color for connect led
  //BatSnsCk();   //do once on start up--doesn't report.

  timesInit();  //
}

//**************loop()***************************************
void loop() {
  BLEReconnect();   //
  ResetSwitch();    //check for OFF
  RxStringParse();  //check for orders from the boss (App)

  CheckForce();                          //check force updates force structure
  unsigned long lclET = getEpochTime();  //Epoch time stamp (ms)--could be done in CheckForce??

  FFSend(lclET);  // put send once in FFSend
  HFSend(lclET);
  MeanSend(lclET);

  BattChecker.update();   // BatSnsCk checks battery, doesn't send voltage--voltage send is query
  LEDtimer.update();      //should call the ledBlink every 10ms.
  SleepChecker.update();  //check for timeout



}  //end of loop
