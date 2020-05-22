# ubitx_v5
Firmware for the version 5 of the ubitx

# Code milestones
Program memory / global variables
  * 20412 / 1123 CPP code conversion, before taskscheduler
  * 22164 / 1186 Added task scheduler with one main task
  * 22284 / 1186 Move actions from menu to main
  * 22168 / 1186 Calibration menu use standard ui, refactor reset
  * 21822 / 1170 EEPROM function refactor
  * 22058 / 1209 Cat control as separate task
  * 22262 / 1213 Menu/main migrated to task
  * 22544 / 1252 Keyer migrated to task
  * 17924 /  903 Tasks without scheduler lib
  * 20000 / 1111 WHAT???
  * 20128 / 1111 before #define > const change
  * 20128 / 1111 eeprom namespace
  * 20128 / 1111 hw namespace
  * 20140 / 1111 encoder namespace
  * 20140 / 1111 ui namespace
  * 20140 / 1111 cat namespace
  * 20126 / 1111 ubitx, settings namespace
  * 20052 / 1107 si5351 namespace
  * 20052 / 1107 mainloop namespace
  * 20052 / 1107 menu namespace
  * 19966 / 1123 buttons/encoder converted to PORT from digitalRead
  * 19854 / 1073 c++ structs
  * 19916 / 1073 some c++ bools
  * 19916 / 1073 settings struct
  * 19898 / 1073 struct zero init, buttons bool
  * 19896 / 1073 function parameter bool
  * 19936 / 1073 cw delay settings
  * 19956 / 1073 fix buttoclick loop
