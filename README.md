#  FlashForge

#### The following GCode commands have been updated:
`M106 [P<index>] [S<speed>]`
`M107 [P<index>]`

```
P - fan index, 0 - cooling fan (used also if parameter was omitted), 1 - rear case fan
S - speed, from 0 to 255. S255 provides 100% duty cycle, S128 produces 50% and etc
Examples:
M107 P1 - disable rear case fan ( equal to M107 P1 S0 )
M106 S128 - set cooling fan to 50% duty cycle ( equal to M106 P0 S128 )
M106 P1 S200 - for rear case fan, speed is nominal value, it can be only ON/OFF state
```
#### GCode controlled temperature limits was raised up to 280°C ( bed and hotend ). GUI menu limits stay same. Make sure what know what you doing with this.

----
#### Supported printers:

- DreamerNX (PWM/REAR FAN/280°C)
- Dreamer (PWM/REAR FAN/280°C)
- Creator Max (PWM/REAR FAN/280°C)
- Guider (PWM/280°C) ( `untested` )
- Finder (PWM/280°C) ( `untested` )
- Inventor (PWM/REAR FAN/280°C) ( `untested` )
- Creator Pro 2 (280°C) ( `untested` )
