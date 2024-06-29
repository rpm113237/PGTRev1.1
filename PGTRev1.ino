
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
// TickTwo LEDtimer(LEDBlink, 10, 0, MILLIS);
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

  // FindCalibration();


  //Serial.printf("before switch, state = %d. el_time = %d\n",sqstate, (millis()-oldmillis));
  switch (sqstate) {
    case START:
      //hand_num set to zero in setup.
      rep_num = 0;
      BLETX("R:%d", rep_num + 1);  // "REP" R:0 or R:1
      BLETX("ST:%d:R:MAX", hand_num);
      if (hand_num == 0) BLETX("V:MAX Effort Right");  //Display state in BLUE(?)
      else BLETX("V:MAX Effort Left");
      BLETX("V:in 3 Seconds");
      PingAndCheckForce(3);
      BLETX("V:MAX NOW");      
      newseqstate(MAX_FORCE, HI_LOW_LED, 0, clrs.WHITE);  //resets oldmillis
      break;

    case MAX_FORCE:
      // Serial.printf("MAXForce entry\n");
      el_time = millis() - oldmillis;  //newseqstate reset oldmillis.
      if (el_time <= sys_params.MAX_FORCE_TM) {
        // Serial.printf("MAXForce time = %d\n", el_time);
        if (scale.is_ready()) {  //.get_units is blocking.
          scaleVal = scale.get_units(scaleSamples / 3);
          BLETX("HF:%d:R:%.1f", hand_num, 0, scaleVal);  //report max //simulate
          if (scaleVal > hands[hand_num].MVE_MAX) {
            hands[hand_num].MVE_MAX = scaleVal;
            //Serial.printf("******SeqState: MAX_FORCE, time = %d, Force = %.1f********\n", el_time / 1000, hands[hand_num].MVE_MAX);
            BLETX("V:%.1f, ", 0, 0, hands[hand_num].MVE_MAX);
            BLETX("MF:%d:R:%.1f", hand_num, 0, hands[hand_num].MVE_MAX);
          }  //default parameters only work on right
          BLETX("TT:%d:0:%d", hand_num, (sys_params.MAX_FORCE_TM - el_time) / 1000);
        } else break;  // only run stuff if data

      } else {  //finished

        hands[hand_num].HOLDTgt = (hands[hand_num].MVE_MAX * sys_params.TGT_PCNT) / 100;
        hands[hand_num].HOLDMax = hands[hand_num].HOLDTgt * (1 + (float)sys_params.HOLD_ERR / 100.0);
        hands[hand_num].HOLDMin = hands[hand_num].HOLDTgt * (1 - (float)sys_params.HOLD_ERR / 100.0);
        // Serial.println(hands[hand_num].MVE_MAX);
        //BLETX("V:Max %.1f, ", 0, 0, hands[hand_num].MVE_MAX);
        delay(1000);
        //BLETX("V:Target %.1f ", 0, 0, hands[hand_num].HOLDTgt);
        delay(1000);  //need to add in delay time to bletx?
                      //while(1);

        if (!deviceConnected) MorseChar('s');

        BLETX("MF:%d:0:%.1f", hand_num, 0, hands[hand_num].MVE_MAX);  //BLETX checks for connection. Also delays.  TODO--starts advertising if not connected?
        BLETX("TF:%d:0:%.1f", hand_num, 0, hands[hand_num].HOLDTgt);
        BLETX("LL:%d:0:%.1f", hand_num, 0, hands[hand_num].HOLDMin);
        BLETX("HL:%d:0:%.1f", hand_num, 0, hands[hand_num].HOLDMax);
        BLETX("HF:%d:0:%.1f", hand_num, 0, 0.0);  //make hold force go to zero????
        sprintf(TxString, "V: Rest %ld Seconds", sys_params.MAX_FORCE_REST / MS_TO_SEC);
        BLETX(TxString);  //no need to delay, going to rest
        newseqstate(MAX_FORCE_REST, HI_LOW_LED, 0, clrs.OFF);

        break;  //need?
      }
      break;

    case MAX_FORCE_REST:
      // just time out.  change light to green when moving to HOLD
      // CheckForce();
      el_time = millis() - oldmillis;  //newseqstate reset oldmillis.
      if (el_time >= sys_params.MAX_FORCE_REST) {
        BLETX("ST:%d:R:HOLD", hand_num);  //put this in newsqstate?
        //Serial.println("Next State HOLD");  //debug
        if (hand_num == 0) BLETX("V:Right Hand HOLD");
        if (hand_num == 1) BLETX("V:Left Hand HOLD");
        sprintf(TxString, "V: Rep %d", rep_num + 1);
        BLETX(TxString);
        //delay(1250);
        BLETX("V:in 3 Seconds");
        BLETX("V:Target %.1f ", 0, 0, hands[hand_num].HOLDTgt);
        PingAndCheckForce(3);        
        //BLETX("S:BxngBell");
        BLETX("V:HOLD NOW");
        if (!deviceConnected) MorseChar('s');
        newseqstate(HOLD, HI_LOW_LED, 0, clrs.OFF);
      } else {        
        if (((el_time % 1000) == 0) && prntonce) {  //coded to sent max, in red for hand_num
          BLETX("TT:%d:0:%d", hand_num, ((sys_params.MAX_FORCE_REST - el_time) / 1000));
          prntonce = false;   //if this doesn't get called within 1ms, doesn't happen
        } else if ((el_time % 1000 != 0)) prntonce = true;
        
      }
      break;

    case HOLD:
      //light is set green
      //sound buzzer--play "charge"
      el_time = millis() - oldmillis;  //newseqstate reset oldmillis.
      if (el_time <= sys_params.HOLD_TM) {
        if (scale.is_ready()) {  //.get_units
          scaleVal = scale.get_units(scaleSamples);
          //Serial.printf("*****SeqState: HOLD, hand = %d; rep = %d; time = %d, Force = %.1f*********\n", hand_num, rep_num, el_time / 1000, scaleVal);
          BLETX("TT:%d:0:%d", hand_num, ((sys_params.HOLD_TM - el_time) / 1000));
          hands[hand_num].TOT_HITS++;
          if (scaleVal > hands[hand_num].HOLDMax) {
            hands[hand_num].HI_HITS++;
            setLED(HI_LOW_LED, 0, clrs.RED);
            sprintf(TxString, "HF:%d:R:%.1f", hand_num, scaleVal);
            BLETX("HF:%d:R:%.1f", hand_num, 0, scaleVal);
            BLETX("V:Easier");
          }  //HIGHLOW RED Sound buzzer?
          else if (scaleVal < hands[hand_num].HOLDMin) {
            hands[hand_num].LO_HITS++;
            setLED(HI_LOW_LED, 0, clrs.YELLOW);
            BLETX("HF:%d:Y:%.1f", hand_num, 0, scaleVal);
            BLETX("V:Harder");
          }  //HI_LOW_LED YELLOW
          else {
            hands[hand_num].ON_TGT_HITS++;
            setLED(HI_LOW_LED, 0, clrs.GREEN);
            BLETX("HF:%d:G:%.1f", hand_num, 0, scaleVal);
          }
          hands[hand_num].ON_TGT_PCNT = ((float)hands[hand_num].ON_TGT_HITS * 100) / hands[hand_num].TOT_HITS;
          //sprintf(TxString, "TP:%d:G:%.1f", hand_num, hands[hand_num].ON_TGT_PCNT);
          BLETX("TP:%d:G:%.1f", hand_num, 0, hands[hand_num].ON_TGT_PCNT);

          hands[hand_num].HI_PCNT = ((float)hands[hand_num].HI_HITS * 100) / hands[hand_num].TOT_HITS;
          //sprintf(TxString, "HP:%d:R:%.1f", hand_num, hands[hand_num].HI_PCNT);
          BLETX("HP:%d:R:%.1f", hand_num, 0, hands[hand_num].HI_PCNT);

          hands[hand_num].LO_PCNT = ((float)hands[hand_num].LO_HITS * 100) / hands[hand_num].TOT_HITS;
          BLETX("LP:%d:Y:%.1f", hand_num, 0, hands[hand_num].LO_PCNT);
        }  // else break;  //end of if (scale.is_ready()

        //break;
      } else {  //Hold time over;  TODO clean up the redundancy in the following "if"
        //BLETX("S:PING");
        if (!deviceConnected) MorseChar('s');

        BLETX("ST:%d:R:REST", hand_num);  //put this in newsqstate?
        //sprintf(TxString, "HF:%d:R:%s", hand_num, "----");  //what for?
        BLETX("HF:%d:R:-----", hand_num);
        BLETX("V:Good Hold");
        //delay(1250);

        if (rep_num < (sys_params.NUM_TESTS - 1)) {  //num tests starts at 1; rep_nums at zero.
          //Serial.printf("rep_num in test = %d\n");   //Display state in BLUE(?)
          sprintf(TxString, "V:Rest %ld Seconds", sys_params.HOLD_REST_TM / MS_TO_SEC);
          BLETX(TxString);
          rep_num++;                   //
          BLETX("R:%d", rep_num + 1);  // "REP" R:0 or R:1; adjusted for display
          newseqstate(HOLD_REST, HI_LOW_LED, 0, clrs.OFF);

        } else {       //reps done, check if it is left hand
          hand_num++;  //START resets rep_num
          // Serial.printf("\nDid we get to hand-num++ = %d\n", hand_num);
          if (hand_num < 2) {
            sprintf(TxString, "V:Rest %ld Seconds", sys_params.BETWEEN_HANDS_TM / MS_TO_SEC);
            BLETX(TxString);
            newseqstate(BETWEEN_HANDS, HI_LOW_LED, 0, clrs.GREEN);  //BETWEEN HANS kicks out on third hand
          }                                                         //hand already bumped; between hands will kick out if hand = 2; (Awkward)
          else {
            BLETX("V:Congratulations");
            newseqstate(WRAPUP, HI_LOW_LED, 0, clrs.OFF);  //resets oldmillis
          }
        }
      }
      break;

    case BETWEEN_HANDS:  //hold between hands
      // just time out. go to start
      el_time = millis() - oldmillis;  //newseqstate reset oldmillis.
      if (el_time <= sys_params.BETWEEN_HANDS_TM) {

        if (((el_time % 1000) == 0) && prntonce) {
          BLETX("TT:%d:0:%d", hand_num, ((sys_params.HOLD_REST_TM - el_time) / 1000));
          prntonce = false;
        } else if (el_time % 1000 != 0) prntonce = true;
      } else {
        newseqstate(START, HI_LOW_LED, 0, clrs.OFF);  //call at end ; reset oldmillis
      }
      break;

    case HOLD_REST:
      // just time out.  change light to green when moving to HOLD
      CheckForce();
      el_time = millis() - oldmillis;  //newseqstate reset oldmillis.
      if (el_time <= sys_params.HOLD_REST_TM) {        
        if (((el_time % 1000) == 0) && prntonce) {
          BLETX("TT:%d:0:%d", hand_num, ((sys_params.HOLD_REST_TM - el_time) / 1000));
          prntonce = false;
        } else if (el_time % 1000 != 0) prntonce = true;
      } else {
        BLETX("ST:%d:R:HOLD", hand_num);  //put this in newsqstate?
        if (hand_num == 0) BLETX("V:Right Hand HOLD");
        if (hand_num == 1) BLETX("V:Left Hand HOLD");
        sprintf(TxString, "V: Rep %d", rep_num + 1);
        BLETX(TxString);
        BLETX("V:in 3 Seconds");
        BLETX("V:Target %.1f ", 0, 0, hands[hand_num].HOLDTgt);
        PingAndCheckForce(3);
        //BLETX("S:BxngBell");
        BLETX("V:HOLD NOW");
        newseqstate(HOLD, HI_LOW_LED, 0, clrs.OFF);  //call at end ; reset oldmillis
        break;
      }
      break;
    case WRAPUP:
      Serial.printf("WRAPUP\n");
      WrapupTX();
      BatSnsCk();
      //pixels.clear(); //turn off lights; set LED isn't going to get called.
      pixels.setPixelColor(HI_LOW_LED, pixels.Color(clrs.OFF[0], clrs.OFF[1], clrs.OFF[2]));
      pixels.setPixelColor(CHG_CONNECT_LED, pixels.Color(clrs.OFF[0], clrs.OFF[1], clrs.OFF[2]));
      pixels.show();  // Send the updated pixel colors to the hardware.
      MorseChar(SHAVE_HAIRCUT);
      Serial.printf("******Going to Deep Sleep; wakeup by GPIO %d*****\n", StartButton);
      esp_deep_sleep_enable_gpio_wakeup(1 << StartButton, ESP_GPIO_WAKEUP_GPIO_LOW);
      esp_deep_sleep_start();
      break;

    case sqPause:
      /*This is called if Switch 1 pressed long enough to be recognized
      right now, this is just holder for first vs second click

      */
      el_time = millis() - oldmillis;  //newseqstate reset oldmillis.
      if (el_time >= sys_params.PAUSE_WAIT_TM) {
        Serial.println(" pause timeout detected; go to sleep");
        BLETX("V:Power Off");
        MorseChar('5');
        MorseChar('5');
        newseqstate(WRAPUP, HI_LOW_LED, 0, clrs.OFF);  //resets oldmillis
      }
      break;
    default:
      Serial.println("Default in state machine; Should not get here");
      break;
  }  //end of case
}  //end of loop
