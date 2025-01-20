#ifndef _SQUEEZER_H
#define _SQUEEZER_H
//#include <Adafruit_NeoPixel.h>
String REV_LEVEL = "5Jan25 03ec5d0 ";  //last part of commit number

#define CNCT_LED_BLINK_TIME 1000     //BLINK TIME FOR CONNECT LED
#define MS_TO_SEC 1000               //convert to secs
#define CONN_WAIT_TM 25 * MS_TO_SEC  //time to wait to connect
#define SHAVE_HAIRCUT 0x1b           //use esc for shave and haircut

//Defaults*********************************************************

//Note--number of samples are caclulated in timesinit(); i.e HFrate = 5 => 80/6 = 16 samples
const int BaseSampleRate = 80;  //scale 80 sps
const int DfltFFRate = 20;      // samples persecond
const int DfltHFRate = 5;       //samples per second
//const int DefaultHFReportTime = 200;  //ms
const int DfltMeanTime = 2;  // Note--period as opposed to Time
const String DefaultSSID = "McClellan_Workshop";
const String DefaultPWD = "Rangeland1";

unsigned int bootCount = 0;  //keep track of how many times since power on TODO--put this in flash

unsigned long oldmillis;        //to time the states
unsigned long int el_time = 0;  //elapsed time
//unsigned long EpochTime;    //for FF reporting

float scaleVal = 0.0;                 //scale data
float scaleCalVal = 8545.85;          //replace with typical number.
const float scaleCalDeflt = 8545.85;  //measured on SN10
const int NumWarmup = 10;
const int NumTare = 10;
char TxString[25];  // used to transmit

//efine pins
//HX711 pins:
const int HX711_dout = 39;  //mcu > HX711 dout pin
const int HX711_sck = 38;   //mcu > HX711 sck pin
//GPIO
const int StartButton = 21;  //
const int ShutDownPin = 20;
const int buzzPin = 48;
const int RatePin = 45;  // = 0 for 10 sps, 1 for 80 sps
//NeoPins
const int NEOPIN = 47; 
const int NEOPIXELS = 1;    //One LED

//const int ADC_addr_rpm = 0X48;
const int sda_rpm = 5;
const int scl_rpm = 4;
double BattVolts = 0; // Variable to keep track of LiPo voltage
double BattSOC = 0; // Variable to keep track of LiPo state-of-charge (SOC)
double BattLife;    // calculated from lipo.getchangerate
//bool alert; // Variable to keep track of whether alert has been triggered

struct ForceStruct {
  /* Force is accumulated and averages calculated based on scale. Reporting is synchronous tied to TickTwo
  for now; ony rate for FF, HF is settable; the report time will be set as (1000/rate) milliseconds.
  */
  unsigned long EpochStart;       //Start of PGT EpochStart in ms
  unsigned long EpochTime;        //in ms == millis()-EpochStartTime
  unsigned long TotalRuntime;     //in seconds, in flash, updated at shutdown = millis() - epochstart
  int BaseRate = BaseSampleRate;  //for Rev1--10 BASERATE #define 80--this is obsolete, I think
  //float BaseVal;  //updated every sample

  bool FFReport = true;     // if true, report FF
  int FFRate = DfltFFRate;  //reports/sec
  int FFReportTime;         //report at this rate (ms)init in timesinit
  int FFNSamp;
  unsigned long FFLastReport;  //millis of last report
  float FFVal;                 //moving average over last BaseRate/FFRate Samples

  bool HFReport = true;
  int HFRate = DfltHFRate;  //This is the samples per second the scale runs at
  int HFReportTime;         // number of milliseconds to report at init in inittimes()
  int HFNSamp;
  unsigned long HFLastReport;  //millis of last hf report
  float HFVal;                 //moving average over last BaseRate/HF rate samples


  bool MeanReport = true;
  int MeanTime = DfltMeanTime;  //time(seconds) that Mean is calculated over
  int MeanReportTime;           // ms to report mean;  calc in inittimes
  int MNNSamp;
  unsigned long MeanLastReport;
  float MeanVal;  //moving average over last MeanTime * BaseRate samples.
} Force;

const int VIB_SND_INTERVAL = 1000;  //ms

int ditTime = 75, chSpTime = 225;  //dit and dah
u_long cwFreq = 2500;

//int freq = 2000;
const int ledChannel = 0;
const int resolution = 8;
const int dutycycle = 127;  //50 percent +/-

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
//float battvolts = 0.0;
float Batt_HI_Lvl = 3.6;
float Batt_OK_Lvl = 3.5;
float Batt_LO_Lvl = 3.3;
float BatMultDefault = 0.001448;  //TODO -find the nominal value
float BatSnsFactor = 0.0;
const int Batt_CK_Interval = 100 * MS_TO_SEC;
const int BattWarnPcnt = 40;      //turn connect LED Yellow/Orange
const int BattCritPcnt = 30;      //turn connect LED Red
const int BattShutDownPcnt = 20;  //go to Sleep.pcnt
#define Battmah 1000
#define Runmah 70
#define BattFullTime (Battmah / Runmah) * 60  //in minutes

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
void setLED(int btime, int clrarray[]);
void VibSend(void);
void StringBLETX(String msg, bool SndSer);
void LEDBlink(void);
void BatSnsCk(void);
void BLEReconnect(void);
void CheckForce(void);

void SoundBuzz(u_long cwFreq = 2500, int sound_ms = 100);
void CalibrateADC(String strval);
void SetFFRate(String valStr);
void SetHFRate(String valStr);
void SetEpochTime(String valStr);
void ConnectWiFi(void);
void MorseChar(int cwChar);
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
void MeanSend(unsigned long ET);
void HFSend(unsigned long ET);
void FFSend(unsigned long ET);
void print_wakeup_reason();
void Soundwakeup(void);
void timesInit(void);
void setFlashDefaults(void);
unsigned long getEpochTime(void);

#endif