#!/bin/bash
echo ..............................
echo Available MCUs:
echo     [1] LLHCU
echo     [2] CLU
echo     [3] UICU    
echo     [4] BLDCCU
echo     [5] STEPCU
echo ..............................

read -p 'Enter MCU prefix: ' mcu

if [ $mcu == 1 ]; then
	echo here 1
elif [ $mcu == 2 ]; then
	echo here 2
elif [ $mcu == 3 ]; then
	read -p 'Enter Port Name: ' port
	".tool-avrdude/bin/avrdude" -C".tool-avrdude/etc/avrdude.conf" -v -patmega328    p -carduino -P/dev/$port -b115200 -D -Uflash:w:.firmws/003-firmware.hex:i
elif [ $mcu == 4 ]; then
	".tool-avrdude/bin/avrdude" -C".tool-avrdude/etc/avrdude.conf" -v -patmega328p -cusbasp -Pu    sb -Uflash:w:.firmws/004-firmware.hex:i
elif [ $mcu == 5 ]; then
	".tool-avrdude/bin/avrdude" -C".tool-avrdude/etc/avrdude.conf" -v -patmega328p -cusbasp -Pu    sb -Uflash:w:../.firmws/005-firmware.hex:i
else
	echo Invalid MCU has been selected!
fi

echo .
echo .
echo Press \'Enter\' key for close...


read -sp '' dump
