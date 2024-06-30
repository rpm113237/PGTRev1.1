
#include <NimBLEDevice.h>
#include <TickTwo.h>
//#include <EasyNeoPixels.h>  //TODO change over to underlying adafruit library
#include "Squeezer.h"
// #include <Adafruit_NeoPixel.h>
#include <Button2.h>
#include <HX711.h>  //this is non blocking.  hope it works
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
//I think EEPROM.h installed by default

// if it is the C3 or other single core ESP32" Use core 0
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else  //otherwise core 1
static const BaseType_t app_cpu = 1;
#endif
//the c3 seems to run on 0 regardless of where it is set.


// define ledticker
TickTwo LEDtimer(LEDBlink, 10, 0, MILLIS);
TickTwo BattChecker(BatSnsCk, Batt_CK_Interval, 0, MILLIS);
//TickTwo DataSimtimer(SIMLD, 1000, 0, MILLIS);  //default si calling once per sec

//construct Button2
Button2 button;   //StartButton
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
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i]);

      Serial.println();
      Serial.println("*********");
    }
  }
};

HX711 scale;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 115200 bits per second:
  //int i = 0; // old FORTRAN programmers never die
  Serial.begin(115200);
  Serial.println("Hello from Squeezer v17Oct");

  ledcAttach(buzzPin, freq, resolution);   //eight bit resolution--why? (Jun24?)
  // ledcSetup(ledChannel, freq, resolution);  // have to do this first to avoid weird run time error
  // ledcAttachPin(buzzPin, ledChannel);       //attach buzzer to 19

  print_wakeup_reason();
  Soundwakeup();  //wake up feedback

  //set up Button2 button
  //Serial.println("Button Demo");

  button.begin(StartButton);
  button.setLongClickTime(3000);
  // button.setDoubleClickTime(400);

  // Serial.println("Longpress Time:\t" + String(button.getLongClickTime()) + "ms");
  // Serial.println("DoubleClick Time:\t" + String(button.getDoubleClickTime()) + "ms");
  // Serial.println();

  // button.setChangedHandler(changed);
  // button.setPressedHandler(pressed);
  // button.setReleasedHandler(released);

// doesn't all this go away?
  button.setTapHandler(click);
  button.setClickHandler(click);
  button.setLongClickDetectedHandler(longClickDetected);
  //button.setLongClickHandler(longClick);
  button.setLongClickDetectedRetriggerable(false);

  //button.setDoubleClickHandler(doubleClick);
  //button.setTripleClickHandler(tripleClick);

  //LEDtimer.start();     //start LED timer
  BattChecker.start();  //start Batt checker


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
  // Serial.print("read average of 20--stabilize: \t\t");
  Serial.printf("*********Warm up read 20X; val = %.2f*********\n", scale.read_average(20));  // print the average of 20 readings from the ADC
                                                                                              //************* to stand out in log
  scale.set_scale(9631.0);  
  // scale.set_scale(scaleCal); 
                                                                    // need
  // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();  // reset the scale to 0

  //pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)

  oldmillis = millis();
  el_time = millis() - oldmillis;
  setLED(CHG_CONNECT_LED, 500, clrs.BLUE);

  Serial.print("Connecting (BLE)\n");
  while (!deviceConnected && (el_time < sys_params.CONN_WAIT_TM)) {
    if ((el_time % 1000) <= 10){
       Serial.print(".");
       //pServer->startAdvertising();  // restart advertising
    }
    LEDBlink();  //has to be called, since timer isn't
    delay(10);
    el_time = millis() - oldmillis;
  }

  if (!deviceConnected) {
    Serial.printf("****end of setup; not connected****\n");
    newseqstate(START, CHG_CONNECT_LED, 0, clrs.OFF);
  } else {
    Serial.printf("****end of setup; BLE connected****\n");
    // BLETX("S:PING");
    C3Delay(3000);
    //give app time to announce connection.
    newseqstate(START, CHG_CONNECT_LED, 0, clrs.BLUE);  //solid blue = connected
  }
  initVals();
  hand_num = 0;
  //while (1);
}

void loop() {
  //do we want this here??
  if (!deviceConnected && oldDeviceConnected) {
    delay(10);                    // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println(" in loop;start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println(" This should happen once");
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  LEDtimer.update();     //should call the ledBlink every 10ms.
  BattChecker.update();  //coded for ten secons--put in .h
  button.loop();
  if (scale.is_ready()) {  //.get_units is blocking.
          scaleVal = scale.get_units(scaleSamples / 3);
          BLETX("HF:%d:R:%.1f", hand_num, 0, scaleVal);  //report max //simulate

  }
  
}  //end of loop
