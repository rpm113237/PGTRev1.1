#include <stdint.h>
#ifndef _SQUEEZER_H
#define _SQUEEZER_H
#include <Adafruit_NeoPixel.h>

//#define DEBUGSQUEEZER  //comment this out for real run times
#define CNCT_LED_BLINK_TIME 1000  //BLINK TIME FOR CONNECT LED
#define MS_TO_SEC 1000               //convert to secs
#define CONN_WAIT_TM 25 * MS_TO_SEC  //time to wait to connect
#define SHAVE_HAIRCUT 0x1b           //use esc for shave and haircut

//Defaults*********************************************************
String REV_LEVEL = "dv1.0";  //actually, include last part of commit number
const int DefaultBaseRate = 10;// samples persecond
const int DefaultFFRate = 10;// samples persecond
const int DefaultHFRate = 5; //samples per second
const int DefaultMeanTime = 2;
String DefaultSSID = "McClellan_Workshop";
String DefaultPWD = "Rangeland1";




RTC_DATA_ATTR int bootCount = 0;  //keep track of how many times since power on TODO--put this in flash


unsigned long oldmillis;        //to time the states
unsigned long int el_time = 0;  //elapsed time
unsigned long EpochTime;    //for FF reporting
//unsigned long EpochTimeStart;

float scaleVal = 0.0;           //scale data
float scaleCalVal = 8545.85;    //replace with typical number.
float scaleCalDeflt = 8545.85;  //measured on SN10
const int NumWarmup = 10;
const int NumTare = 10;
char TxString[25];  // used to transmit

//HX711 pins:
const int HX711_dout = 7;  //mcu > HX711 dout pin
const int HX711_sck = 3;   //mcu > HX711 sck pin


struct ForceStruct{  
  /* Force is accumulated and averages calculated based on scale. Reporting is synchronous tied to TickTwo
  for now; ony rate for FF, HF is settable; the report time will be set as 1000/rate milliseconds.
  */ 
  unsigned long EpochStart;	//Start of PGT EpochStart
  unsigned long EpochTime; //in ms == millis()-EpochStartTime
  int BaseRate = DefaultBaseRate;    //for Rev1--10 BASERATE #define 80
  float BaseVal;  //updated every sample
  bool FFReport = true;
  int FFRate = DefaultFFRate;  //samples/sec
  int FFReportTime = 100; //report at this rate (ms)
  unsigned long FFLastReport;	//millis of last report
  float FFVal;    //moving average over last BaseRate/FFRate Samples
  bool HFReport = true;
  int HFRate = 5;   //This is the samples per second the scale runs at
  int HFReportTime = 200; // number of milliseconds to report at
  unsigned long HFLastReport;	//millis of last hf report
  //int HFSamples == BaseRate/HFRate
  float HFVal; //moving average over last BaseRate/HF rate samples
  bool MeanReport = true;
  int MeanTime = 2; //time(seconds) that Mean is calculated over
  int MeanReportTime = 1000; // ms to report mean
  unsigned long MeanLastReport; 
  float MeanVal; //moving average over last MeanTime * BaseRate samples.
} Force;

const int VIB_SND_INTERVAL = 1000;  //ms

int ditTime = 100, chSpTime = 300;  //dit and dah
u_long cwFreq = 2500;

int freq = 500;
const int ledChannel = 0;
const int resolution = 8;
const int dutycycle = 127;  //50 percent +/-
const int buzzPin = 19;

const int NEOPIN = 8;                                               //
const int NEOPIXELS = 2;                                            //two led's
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
int LEDSelect = 0;                    //0 or 1; make enum
const int BatSns = 2;
const int NumADCRdgs = 10;  //number of times to read ADC in floatADC
float battvolts = 0.0;
float Batt_HI_Lvl = 3.6;
float Batt_OK_Lvl = 3.5;
float Batt_LO_Lvl = 3.3;
float BatMultDefault = 0.001019;  //TODO -find the nominal value
float BatSnsFactor = 0.0;
const int Batt_CK_Interval = 100 * MS_TO_SEC;
const int BattWarnPcnt = 40;    //turn connect LED Yellow/Orange
const int BattCritPcnt = 15;    //turn connect LED Red
const int BattShutDown = 10;    //go to Sleep.
#define Battmah 1000
#define Runmah 70
#define BattFullTime (Battmah / Runmah) * 60  //in minutes


const int StartButton = 1;  //
const int FastPin = 18;  //set =1 for 80 samplses/sec

uint16_t SleepTimer;          // in seconds reset if HF> MinForce
uint32_t SleepTimeMax = 300;  //sleep timeout in sec
int MinForce = 1;             //if HF < MinForce, sleeptimer
uint32_t SleepTimerStart;     // if HF> MinForce, reset SleepTimerStart to current millis()/mstosec

//const int numSamples = 2;
long int scaleRead = 0;

//Flash (preferences.h) setup
char SSstr[25] = "McClellan_Workshop";  //max from ble is about 20(?)- 2 for tag.
char PWDstr[25] = "Rangeland1";
const char* ssid = SSstr;
const char* password = PWDstr;


//protos
void setLED(int LedNo, int btime, int clrarray[]);
void VibSend(void);
void BLETX(void);
void LEDBlink(void);
String BatSnsCk(void);
void SoundBuzz(u_long cwFreq = 2500, int sound_ms = 100);
void CalibrateADC(String strval);
void SetSSID(String ValStr);
void SetPwd(String ValStr);
void DoOTA(void);
void CalibrateScale(String strval);
void DoTare(void);
float getFloatADC(int numtimes);
void RunTimeCheck(void);
void ResetSwitch(void);
void GoToSleep(String DSmsg);
void SendRev(String valStr);
void MeanSend (void);
void HFSend (void);
void FFSend (void);
void print_wakeup_reason();
void Soundwakeup(void);
void timesInit(void);


#endif