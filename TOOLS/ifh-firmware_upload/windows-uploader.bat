@echo off

:startLabel
@echo ..............................
@echo Available MCUs:
@echo     [1] LLHCU   [2] CLU
@echo     [3] UICU    [4] BLDCCU
@echo     [5] STEPCU  [6] ATM_STEP   [7] INO_STEP
@echo ..............................

:label0
SET /p mcu=Enter prfix of MCU that will be programmed: 

if %mcu% == 1 (
	@echo here 1
)else if %mcu% == 2 (
	@echo here 2
)else if %mcu% == 3 (
	@echo here 3
)else if %mcu% == 4 (
".tool-avrdude-win/bin/avrdude" -v -p atmega328p -C ".tool-avrdude-win/etc/avrdude.conf" -c usbasp-clone -U flash:w:../../OFFICIAL/BLDC/.pioenvs/pro16MHzatmega328/firmware.hex:i
)else if %mcu% == 5 (
".tool-avrdude-win/bin/avrdude" -v -p atmega328p -C ".tool-avrdude-win/etc/avrdude.conf" -c usbasp-clone -U flash:w:../../OFFICIAL/STEP/.pioenvs/pro16MHzatmega328/firmware.hex:i
)else if %mcu% == 6 (
".tool-avrdude-win/bin/avrdude" -v -p atmega328p -C ".tool-avrdude-win/etc/avrdude.conf" -c usbasp-clone -U flash:w:../../OFFICIAL/STEP_ATMEL/STEP_ATMEL/Debug/STEP_ATMEL.hex:i
)else if %mcu% == 7 (
".tool-avrdude-win/bin/avrdude" -v -p atmega328p -C ".tool-avrdude-win/etc/avrdude.conf" -c usbasp-clone -U flash:w:../../OFFICIAL/STEP_INO/STEP_INO/STEP_INO/Debug/STEP_INO.hex:i

)else (
	@echo .
	@echo Invalid number has been entered!
	goto label0
)

goto startLabel