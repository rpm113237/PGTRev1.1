//R. P. McClellan 27Jun2023; latest 9/9/23; latest 29Jun24
/*
09Oct23--clean up; get rid of some comments, make a subroutine or two.  TODO--put ledbLINK CALL INTO delay function to keep
led's alive during delay for kodular to do text to speech. Make lights go on during last part of hold rest 
NOTE!!!: 0.33(+/- 0.05)* TGT, not to far off from 0.33*TGT * .90 to 1.00
TODO Use ESP32 S3, dual core, put led's and scale reading in tasks
TODO-- make the scale live during rest; led off?
TODO--in S3 version,run the leds during the three second warning--let user get a running start.
9/14/23--move the functions off into their own file/tab. 
9/12 works, changed state machine around
9/12/23-- clean up rep counter, etc. 9/10 doesn't do a hold rest after second time through right hand.  Fix
Integrate BAtSns ADC; deep sleep works.new CMD E:clr:val
Squeezer_9_10_23 works with HX711; goes into deep sleep at finish; restarts with START SW
TODO--change over to the AdaFruit library from EasyneoPixel
SWSTART is GPIO1--check for OK--Seems OK; has to be set pinmode = input for some reason.

NOTE!!!  OLD Squeezer19 used 0.33 as tgt pcnt with hi/low at +-.05 of that!!!
initial impression is that is too tight.  Currently, 20 percent of tgt
TODO; LONG BLE strings don't work-- Can only use for VOICE?? V:; have to prefix strings with V:?
Either break up long voice strings, or do it in code.
Basically, the text to speech block doesn't wait to complete; either timeout (1500ms seems to work?)
in the ESP code or possibly use the results property or event to do it in the app
probably not worth the effort; could use an escape in the text to indicate another is coming?
Right now, keep the strings under 19/(20?) 23?
Rewrite BLETX (again) to eliminate the long string stuff.  Or fix it for long voice commands
TODO--use start switch to bypass wait for connect
TODO--right now, system really only works with app.  

TODO--add in 20? second prompt on HOLD & HOLD REST
TODO-- sound bell at start of HOLD and HOLDMAX--there seems to be a bug which causes the app to see
multiple bells, even if only one S: command is sent.  (BUG--the Bell used was 30 seconds long)
Apparently, NIMBLE (with UART defintion?) sends whole 20? bytes

TODO--add in ADC read of batsns; new cmd E: (Done)
TODO-- figure out and define the delay for the app text to speech, make it a #define
TODO--add it command to send out which rep it is on R:Right(Left)

TODO--why in the hell does the Soundwakeup sound twice on first call?  

TODO--Blink only works for one LED--not sure it works reliably there.  Class?















*/