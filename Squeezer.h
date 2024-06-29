#ifndef _SQUEEZER_H
#define _SQUEEZER_H

//#define DEBUGSQUEEZER  //comment this out for real run times

#define MS_TO_SEC 1000     //convert to secs
#define CHG_CONNECT_LED 0  //
#define HI_LOW_LED 1       //
const int NEOPIN = 8;      //
const int NEOPIXELS = 2;   //two led's
#include <Adafruit_NeoPixel.h>
#define SHAVE_HAIRCUT 0x1b  //use esc for shave and haircut

RTC_DATA_ATTR int bootCount = 0;                                    //keep track of how many times since power on
Adafruit_NeoPixel pixels(NEOPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);  //2 ea sk6812 on pin 8

enum SqueezerState { START,
                     BETWEEN_HANDS,
                     MAX_FORCE,
                     MAX_FORCE_REST,
                     HOLD,
                     HOLD_REST,
                     sqPause,
                     WRAPUP } sqstate;
//not sure BLE_CONN is needed?  handled in Rst?

const int battState[11] = { 0, 0, 0, 8, 9, 10, 15, 25, 70, 100, 100 };  //convert to int x10, subtract 2.5 for index; if >4.1 = 100/charging
const char* SOCclrs[11] = { "R", "R", "R", "R", "R", "R", "R", "Y", "Y", "G", "G" };
const char* stateLabels[8] = { "START", "BETWEEN_HANDS", "MAX_FORCE", "MAX_FORCE_REST", "HOLD", "HOLD_REST", "sqPause","WRAPUP" };
//TODO--can do similar to tag hands
enum HandEnum { rtHand = 0,
                ltHand } handnum;  //index in array

struct SYS_PARAMS {
  int NUM_TESTS = 2;                         // number of times to repeat each hand
 // u_long CONN_WAIT_TM = 10 * MS_TO_SEC;      //ten seconds to connect.
  float CALVAL = 1000.0;                     //1000 is fake, real number goes in here.
  u_long MAX_FORCE_TM = 5 * MS_TO_SEC;       //five seconds for hard squeeze
  u_long MAX_FORCE_REST = 10 * MS_TO_SEC;    //rest ten seconds after hard squeeze
  int TGT_PCNT = 33;                         // percent of MVE_MAX (typ 33) (Different left/right)??
  int HOLD_ERR = 20;                         //pcnt above and below for error band. SQEEZER19 had this at +/- 5 percent TODO
#ifdef DEBUGSQUEEZER                         //debug times
  u_long PAUSE_WAIT_TM = 20*MS_TO_SEC;       //timeout for Pause
  u_long CONN_WAIT_TM = 200 * MS_TO_SEC;      //20for debug seconds to connect.
  u_long HOLD_TM = 20 * MS_TO_SEC;           //20 sec fordebug
  u_long HOLD_REST_TM = 10 * MS_TO_SEC;      //10
  u_long BETWEEN_HANDS_TM = 10 * MS_TO_SEC;  //10 SEC REST TIME AFTER SECOND HOLD, BEFORE MAX
#else                                        //run times
u_long PAUSE_WAIT_TM = 180*MS_TO_SEC;       //timeout for Pause--three minutes
  u_long CONN_WAIT_TM = 30 * MS_TO_SEC;      //30 seconds to connect.
  u_long HOLD_TM = 120 * MS_TO_SEC;          //120 sec
  u_long HOLD_REST_TM = 60 * MS_TO_SEC;      //60
  u_long BETWEEN_HANDS_TM = 20 * MS_TO_SEC;  //20 SEC REST TIME AFTER SECOND HOLD, BEFORE MAX
#endif

} sys_params;

struct HAND_PARAMS {
  float MVE_MAX;  //Determined in three second squeeze
  float HOLDTgt;
  float HOLDMax;
  float HOLDMin;
  //HOLD_TGT, HOLD_MAX, HOLD_MIN are calculated from pcnt, max and error
  u_long ON_TGT_HITS;  //ticks on target
  u_long HI_HITS;      //ticks HI
  u_long LO_HITS;      //ticks LO
  u_long TOT_HITS;
  float ON_TGT_PCNT;  //calculated from above
  float HI_PCNT;      // calc from above
  float LO_PCNT;      //calc from above
  float TEST_HI;      //highest for test--all iterations
  float TEST_MIN;     //lowest
};

struct HAND_PARAMS hands[2];

struct COLORS {
  int RED[3] = { 255, 0, 0 };
  int GREEN[3] = { 0, 255, 0 };
  int BLUE[3] = { 0, 0, 255 };
  int YELLOW[3] = { 255, 255, 0 };
  int WHITE[3] = { 255, 255, 255 };
  int OFF[3] = { 0, 0, 0 };
  int WKCLRS[3] = { 0, 0, 0 };  //used for the LED task.

} clrs;

unsigned long oldmillis;  //to time the states
unsigned long int el_time = 0;  //elapsed time
unsigned long int pause_el_time =0; //
SqueezerState paused_State;   //used to get back after pause


float scaleVal = 0.0;  //scale data
float scaleCal = 12496.0;  //estimated at 70 lbs
//bool simDataRdy = false;  //used for simulation
char TxString[25];  // used to transmit

//HX711 pins:
const int HX711_dout = 7;  //mcu > HX711 dout pin
const int HX711_sck = 3;   //mcu > HX711 sck pin
const int scaleSamples = 10;

int ditTime = 100, chSpTime = 300;  //dit and dah
u_long cwFreq = 2500;

int freq = 500;
const int ledChannel = 0;
const int resolution = 8;
const int dutycycle = 127;  //50 percent +/-
const int buzzPin = 19;

int BlinkTime = 1000;  //this tells the blink task how low to blink for                       //if ==0, ON
int LEDSelect = 0;     //0 or 1; make enum
const int BatSns = 2;
float Batt_HI_Lvl = 3.6;
float Batt_OK_Lvl = 3.5;
float Batt_LO_Lvl = 3.3;
const int Batt_CK_Interval = 10000;

//StartSwitch--TODO--second switch, if needed.
const int StartButton = 1;  //
//const int FastPin = 18;   //set =1 for 80 samplses/sec

//const int numSamples = 2;
long int scaleRead = 0;

//unsigned long int TTGO = 0;
int hand_num = 0, rep_num = 0;  //old fortran programmers never die
bool repsdone = false;
//hand_num = hand working on--enum ltHand, rtHand--rep_num is the number of times for the hand
//bool timecheck = true;  //used to do debug printouts of time.
bool prntonce = true;  //used to print time once per second
char buffer[20];       //???

//protos
void setLED(int LedNo, int btime, int clrarray[]);
void newseqstate(SqueezerState newst, int ledno, int ledblink, int ledclr[]);  // change states
void BLETX(const char* fmtstr, int intparam1 = 0, u_long ulparam = 0, float fltparam = 0.0, const char* strparam = "");
//void BLETX(void);
void LEDBlink(void);
void ldSimLD(float ldTGT, float ldMAX, float ldMIN, float ldPCNT);
void BatSnsCk(void);
void SoundBuzz(u_long cwFreq = 2500, int sound_ms = 100);

#endif