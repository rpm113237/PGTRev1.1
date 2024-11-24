//R. P. McClellan 27Jun2023; latest 9/9/23; latest 29Jun24
/*

N

Apparently, NIMBLE (with UART defintion?) sends whole 20? bytes

TODO--why in the hell does the Soundwakeup sound twice on first call?  



Start Switch:  In this implementation, the start switch wakes up from deep sleep.  Optionally, goes into deep sleep on long hold?  Pause, restart, etc. goes away. need only long click for operator shutdown?  Send
notification to app of operator shutdown??  
TODO  Since it is entirely app driven, do we even need a power off function on the PGT?T


Scale:  Currently hard wired to 10 sps; 

TODO check ADC accuracy.

TODO:  !!!! on going into deep sleep, invoke scale .powerdown() see 
void HX711::power_down()
{
  //  at least 60 us HIGH
  digitalWrite(_clockPin, HIGH);
  delayMicroseconds(64);
}

Thge LED blinking code is messy.  LedSelect no longer relevant--only one LED.  Combine LEDSelect into LEDBlink.  Messey

***The BLE initial connect and the in loop reconnect seem redundant and awkward.

!!!! look at aclibrated ADC reads, also cal routine.

TODO???? Run cal routines off the serial monitor at setup?
TODO 

*******************  Sampling Rate vs Reporting Rate  *************************
IN 21Sp24 and subsequent branch revisons, the scale is sampled at 80 sps.  (Is there any need to do 10?) 
The actual reported number (HF, FF, Mean) is calculated over some time--or number of samples.  The scale is asychronous and not necessarily 
time accurate.  That is, the 80 sps is unlikely to be exactly 12.5ms.
Reporting, OTOH, is synchronous to the ESP32 clock. "Epoch Time"; initialized on power up or from the App.
As it stands, the Forces are done via a stochastic approximation-- "cumulative average"--which is actually an exponentially weighted past
approach.
 https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
 In the current implementation, the cum average is calculated over the nominal corresponding time period of the report.  That is, if HF



Rev5Nov24

Squawks:
1) BOOT (IO8) NOT PULLED HIGH.  IDIOT!  (Note IO9 is managed by programmer)
2) Pull RATE LOW--10k.  Probably not needed.
3)  For 5Nov version.  How do we know if it is a fresh boot

	GPIO	Function	          Comment

  IO0   NA
  IO1   SWStart_MPU         High if SWStart closed; if =HI, power up was due to switch
  IO2   NC                  
  IO3   PD_SCK              HX711 Serial Clock
	IO4		SCL			            ADS1015; use Tillart's lib or Sparkfun--maybe better
  IO5   SDA                 ADS1015
  IO6   ADC_RDY             From ADS1015--Not sure needed
  IO7   HX_DOUT             HX711 DOUT
  IO8   SK_DIN              SK6812 DIN
  IO9   BOOT                --see squawk 1)
  IO10  RATE                HX711 L = 10 sps, H = 80 sps
  RXD                       PIN 11; TX output of Programmer
  TXD                       PIN 12; RX Input of PGMR
  IO18  ShutDown            HIGH Turns Power OFF (latched)
  IO19  BuzzPWM             Buzzer PWM



















*/