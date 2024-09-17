
#include <NimBLEDevice.h>
#include <TickTwo.h>
//#include <EasyNeoPixels.h>  //TODO change over to underlying adafruit library
#include "Squeezer.h"
//#include <Adafruit_NeoPixel.h>
#include <Button2.h>
#include <HX711.h>  //this is non blocking.  hope it works
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
//I think EEPROM.h installed by default

//OTA includes--taken from ElegantOTA demo example
#include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
  #include <ElegantOTA.h>
   WebServer server(80);

const char* ssid = "McClellan_Workshop";
const char* password = "Rangeland1";


// if it is the C3 or other single core ESP32" Use core 0
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else  //otherwise core 1
static const BaseType_t app_cpu = 1;
#endif
//the c3 seems to run on 0 regardless of where it is set.


// define ledticker, battchecker, vibReport--have to have protos in .h
TickTwo LEDtimer(LEDBlink, 10, 0, MILLIS);  //calls LEDBlink, called every 10MS, repeats forever, resolution MS

TickTwo VibReport(VibSend,VIB_SND_INTERVAL, 0, MILLIS);   //send vibration data every VIB_SND_INTERVAL (ms), forever

TickTwo BattChecker(BatSnsCk, Batt_CK_Interval, 0, MILLIS);  //checks battery every Batt_Ck_Interval


//construct Button2
//Button2 button;   //StartButton
//comment--why not just handle it as an input? In Rev 1 what does it do other than START?


BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
float txValue = 0;
std::string rxValue{};  // so can process outside of callback; maybe not the best idea

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
  //int i = 0; // old FORTRAN programmers never die
  Serial.begin(115200);
  Serial.printf("\nHello from Protocol Grip Trainer v25Jul24\n");

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

  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");



  ledcAttach(buzzPin, freq, resolution);   //eight bit resolution--why? (Jun24?); using for PWM
  
  print_wakeup_reason();
  Soundwakeup();  //wake up feedback
  
  //set up LED's
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDBLUE, OUTPUT);
  pinMode(StartButton, INPUT);

  //start the TickTwo timers
  LEDtimer.start();     //start LED timer
  BattChecker.start();  //start Batt checker
  VibReport.start();    //start Vib report

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
  Serial.println("Waiting a client connection to notify...");
  //pinMode(StartButton, INPUT);  // Right now, used only to wake up out of deep sleep--done in Button2??

  //set up the scale
  
  scale.begin(HX711_dout, HX711_sck);
  scale.reset();    //if coming out of deep sleep; scale might be powered down
  // Serial.print("read average of 20--stabilize: \t\t");
  Serial.printf("*********Warm up read 20X; val = %.2f*********\n", scale.read_average(20));  // print the average of 20 readings from the ADC
                                                                                              //************* to stand out in log
  scale.set_scale(9631.0);  //This value needs to be default; a cal function needs to be implemented
  // scale.set_scale(scaleCal); 
                                                                    // need
  // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();  // reset the scale to 0


  oldmillis = millis();
  el_time = millis() - oldmillis;
  setLED(500, clrs.BLUE);
  // pixels.setPixelColor(LEDSelect, pixels.Color(clrs.RED[0], clrs.RED[1], clrs.RED[2]));
  // pixels.show();  // Send the updated pixel colors to the hardware.

  Serial.print("Connecting (BLE)\n");
  BlinkTime = CNCT_LED_BLINK_TIME;
  setLED(250, clrs.BLUE);
  BatSnsCk(); //sets color for connect led  
  while (!deviceConnected && (el_time < CONN_WAIT_TM)) {
    if ((el_time % 1000) <= 10){
       Serial.print(".");
       //pServer->startAdvertising();  // restart advertising
    }
    //setLED(0, clrs.BLUE);
    LEDBlink();  //has to be called, since timer isn't being called?? or call timer?
    delay(10);
    el_time = millis() - oldmillis;
  }

  if (!deviceConnected) {
    Serial.printf("****end of setup; not connected****\n");
    // newseqstate(START, CHG_CONNECT_LED, 0, clrs.OFF);
  } else {
    Serial.printf("****end of setup; BLE connected****\n");
    setLED(0, clrs.BLUE);   //for ledBlink
    // pixels.setPixelColor(LEDSelect, pixels.Color(clrs.BLUE[0], clrs.BLUE[1], clrs.BLUE[2]));
    // pixels.show();  // Send the updated pixel colors to the hardware.
    // BlinkTime = 0;
        
  }
  // initVals();
  // hand_num = 0;
  //while (1);
}

void loop() {

   server.handleClient();
  ElegantOTA.loop();
  
  BattChecker.update();  // BatSnsCk checks battery, sends voltage
  LEDtimer.update();     //should call the ledBlink every 10ms.
  VibReport.update();
  RxStringParse();
  //button.loop();    // what for?? do we want device turn off?
  BLEReconnect();   //TODO does this work???
  CheckForce();     //check force does the sending.  
  //VibSend();
  //  above commented out for battery test
  //delay(1000);
 
  
}  //end of loop
