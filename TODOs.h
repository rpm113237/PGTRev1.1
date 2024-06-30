//R. P. McClellan 27Jun2023; latest 9/9/23; latest 29Jun24
/*

NOTE!!!: 0.33(+/- 0.05)* TGT, not to far off from 0.33*TGT * .90 to 1.00

TODO-- make the scale live during rest; led off?

SWSTART is GPIO1--check for OK--Seems OK; has to be set pinmode = input for some reason.

TODO--currently, the PGT Starts when the charger is plugged in & battery discharged;(Why??) check reason for reset--if not wake, don't start.
TODO (Hardware--what happens when battery discharges?  Does current go low enough? Disable linear reg on battery below something??)

NOTE!!!  OLD Squeezer19 used 0.33 as tgt pcnt with hi/low at +-.05 of that!!!
initial impression is that is too tight.  Currently, 20 percent of tgt
TODO--use start switch to bypass wait for connect ???

TODO--add in 20? second prompt on HOLD & HOLD REST
)
Apparently, NIMBLE (with UART defintion?) sends whole 20? bytes

TODO--add in ADC read of batsns; new cmd E: (Done)

TODO--why in the hell does the Soundwakeup sound twice on first call?  

TODO--Consider making LED blinking a PWM??















*/