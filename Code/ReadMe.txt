/*See Readme file. V005 attempts to overcome rounding float issues that
were resulting in the displayed index being off by 1 in V004.

V2005 includes:
- Better help menu
- (new) Info command
- added manually assigned steps for each flap in flap_index.h
- added a value and a function to apply adjustments to this value
- cleaned print functions to have divider lines between

Iterated to v2006 because it works now, and some features in v2006 may
interrupt active process' and change the logical flow of the offset-setting
commands. Thanks for reading.
*/



/*You will likely need to download a windows driver depending on your
ESP32 Board of choice. You will also need to add the ESP32 boards to
your arduinno IDE. Below is an example board and driver needed:


https://a.co/d/9pYIOlR
https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads
for this board, the "Universal Windows Driver" works. Go to device manager and find
the device, then select the downloaded/extracted driver for "update driver" 

For me it showed up under the "Other Devices" as CP2102 USB to UART Bridge Controller
If you select the folder one level above x64 / x86 etc. it finds the correct driver.

In the Arduino IDE, I selected the ESP32 Dev Module as the board.
*/