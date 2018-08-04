# USBasp Driver Install - Windows Only #

- Navigate to __ifh-firmware_upload__ directory.
- Go to __USBasp-drivers__ and run __zadig-2.3.exe__.
- In menu bar of Zadig window, go to *Options* and select __List All Devices__.
- In dropdown menu select USBasp and for Driver select __libusbK(v3.0.7.0)__. Configuration should look as shown in figure 1.
- Press on button __Install Driver__.
- If everything was successful, go to device manager and confirm that USBasp is recognized, as shown in figure 2.

![Figure 1](https://i.imgur.com/EzBmAVn.jpg)

*Figure 1 - Setup of Zedig application for libusb driver install.*

![Figure 2](https://i.imgur.com/sASg1MR.jpg)

*Figure 2 - Confirming that libusb driver was successfully installed.*

# FTDI Driver Install - Windows Only #

- Navigate to directory __ftdi-driver__, run __ftdi-driver.exe__ and follow the setup guide.
- Confirm that drivers are installed by going to __Device Manager__, as shown in figure 3.
- Remember on which port the device is connected.

![Figure 3](https://i.imgur.com/0O7ubQx.jpg)

*Figure 3 - Confirming that FTDI driver was successfully installed.*

# Burning Firmware #

## UICU ##

- Connect USB cable to UPROG port on the Motherboard and computer.
- Determen on which port the device is connected to:
	- On Windows go to __Device Manager__ and find port name in __Port__ section.
	- On Linux terminal run command __ls /dev__. If no other USB devices are connected to the computer, the device should be connected to __ttyUSB0__.
- On the Motherboard, with __SW801__ select UICU for programming.
- Navigate to main directory and run __windows-uploader.bar__ (for Windows) or __linux-uploader.sh__ (for Linux, run as a root user).
- In console application enter number 3 for UICU MCU and type name of the communication port on which the device is connected (for Windows it should be COMx and for Linux ttyUSBx).
- After process is done, MCU should have latest version of the firmware.

## BLDCCU and STEPCU ##

- Connect USBasp to the computer.
- Connect cable from USBasp to MCU SPI header.
- Navigate to main directory and run __windows-uploader.bar__ (for Windows) or __linux-uploader.sh__ (for Linux, run as a root user).
- In console application enter number 4 for BLDCCU or 5 for STEPCU MCU.
- After process is done, MCU should have latest version of the firmware.
