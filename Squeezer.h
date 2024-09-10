#ifndef _SQUEEZER_H
#define _SQUEEZER_H
#include <Adafruit_NeoPixel.h>

//#define DEBUGSQUEEZER  //comment this out for real run times
#define CNCT_LED_BLINK_TIME 1000  //BLINK TIME FOR CONNECT LED

#define MS_TO_SEC 1000     //convert to secs
#define CONN_WAIT_TM 10 * MS_TO_SEC //time to wait to connect 
#define SHAVE_HAIRCUT 0x1b  //use esc for shave and haircut

RTC_DATA_ATTR int bootCount = 0;   //keep track of how many times since power on


unsigned long oldmillis;  //to time the states
unsigned long int el_time = 0;  //elapsed time

float scaleVal = 0.0;  //scale data
float scaleCal = 12496.0;  //estimated at 70 lbs

char TxString[25];  // used to transmit

//HX711 pins:
const int HX711_dout = 7;  //mcu > HX711 dout pin
const int HX711_sck = 3;   //mcu > HX711 sck pin
// const int scaleSamples = 10;
const int scaleSamples = 2;
#define FORCE_ARRAY_SECS 2      //how many seconds long is the force array?
#define FORCE_ARRAY_LEN FORCE_ARRAY_SECS*10/scaleSamples //The constant 10 will need redefined if 80 sps is implemented
struct ForceRecord {
  float ForceArray[FORCE_ARRAY_LEN];   
  float ForceMax;
  float ForceMin;
  float ForceMedian;
  float ForceMean;
  float ForceACRMS;
  float ForceFundFreq;

} Force;
const int VIB_SND_INTERVAL = 1000;  //ms

int ditTime = 100, chSpTime = 300;  //dit and dah
u_long cwFreq = 2500;

int freq = 500;
const int ledChannel = 0;
const int resolution = 8;
const int dutycycle = 127;  //50 percent +/-
const int buzzPin = 19;
const int LEDRED = 4;   //RED LED HIGH = ON
const int LEDBLUE = 5; //BLUE LED HIGH = ON
bool UseRedLED = false;   //if true, light off red
bool UseBlueLED = false;  //if true, light off blue led

const int NEOPIN = 8;      //
const int NEOPIXELS = 2;   //two led's
Adafruit_NeoPixel pixels(NEOPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);  //2 ea sk6812 on pin 8
struct COLORS {
  int RED[3] = { 255, 0, 0 };
  int GREEN[3] = { 0, 255, 0 };
  int BLUE[3] = { 0, 0, 255 };
  int YELLOW[3] = { 255, 255, 0 };
  int WHITE[3] = { 255, 255, 255 };
  int OFF[3] = { 0, 0, 0 };
  int WKCLRS[3] = { 0, 0, 0 };  //used for the LED task.

} clrs;

int BlinkTime = CNCT_LED_BLINK_TIME;  //blink ON/OFF TIME; if ==0, ON
int LEDSelect = 0;     //0 or 1; make enum
const int BatSns = 2;
float battvolts = 0.0;
float Batt_HI_Lvl = 3.6;
float Batt_OK_Lvl = 3.5;
float Batt_LO_Lvl = 3.3;
const int Batt_CK_Interval = 60 * MS_TO_SEC;

//StartSwitch--TODO--second switch, if needed.
const int StartButton = 1;  //
const int LongClickTime = 3000; // time for long click to shut down?
//const int FastPin = 18;   //set =1 for 80 samplses/sec

//const int numSamples = 2;
long int scaleRead = 0;

//protos
void setLED(int LedNo, int btime, int clrarray[]);
void VibSend(void);
void BLETX(void);
void LEDBlink(void);
void BatSnsCk(void);
void SoundBuzz(u_long cwFreq = 2500, int sound_ms = 100);

#endif