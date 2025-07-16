# Modular Open Source Split-Flap Display
(MOSSFD)
--------------------------------------------------------------------------------------------------------------------------------
This project is a Mechanical "Split-Flap" display based on:
- ESP32-WROOM-32D microcontroller
- 28BYJ-48 5V DC Stepper Motor
- ULN2003AN BWQ 2436C motor driver
- A3144 digital hall effect sensor

The display is designed to be built with identical modules based on these components. The modules can be connected in chain to form a display. For now, individual power is supplied to each unit, but on small scales, power can be daisy-chained instead. The modules I built use a JST 5 pin connector to connect RX, TX, "Detect", GND, and 5v, but you could use whatever 5 pin you felt like. (If you use VGA connectors please send me a picture). The hardware in the array is identical, but one module operates as the "Master", which is currently able to:
- Receive input from Serial Com (For example the Arduino IDE or other program)
- Scan this "Paragraph_message" for its own unique (mac-based) ID and perform the assigned command
- Forward this message to the other modules.

The other modules in the array scan the paragraph_message for their ID, and perform the command connected to their ID.

Current commands include (with a few notes about upcoming changes)
any letter    - Check if the character is in their index/drum, then move to that character.
***INFO       - Print current information                    //update
***ZERO       - Re-index using magnet			     //Possibly add slow zero
***<number>   - Move to flap index by number                 //Verify working after combined logic
***POS        - Print current step and degree position       //Possibly integrate to info / delete
***LIST       - Print character index list                   //.
***RESET      - Restart the ESP32                            //.
***REV        - Measure steps in one revolution              //Probably delete
***COUNT      - Show magnet pass count                       //Integrate/delete
***MOVE <N>   - Move forward by N steps                      //Delete
***ZOFFSET    - Set magnet-to-flap offset in degrees         //Cleanup + clean debug
***OFFSET     - Set global step offset                       //Cleanup + clean debug
***RESETCONFIG - Reset stored config to default values       //.
***HELP       - Show this help message                       //Update to match
***LEFTMODE   - Enable left-module input mode                //Update to match new logic
****[message] - Inject paragraph_message (for dev testing)   //Note: only available in MasterMode

At present the master module is kind of buggy, and needs some notable firmware work. It is not capable yet of creating the "paragraph_message", but will hopefully be able to do so soon. For now, I've been using a python script to parse text into a use-able paragraph message, which is then sent via Serial comm (Baud 11520 for PC -> Module communication) to the master module.

The paragraph_message is composed as follows:
**** //denotes that the following is a paragraph_message
[] //the beginning/end of the contents of the paragraph_message
{} //encompasses the master ID, which is sent at the beginning of each paragraph_message. This will be utilized later in development
| //divides the message from the master into chunks that are applicable to each unit
: //divides the portion of the message that is applicable to each unit into sections


Example: 9 module display, showing:
012345678

****[{14146C1C4278}|14146C1C4278:0:0|E0F76B1C4278:1:0|0445470B65F4:2:0|F893963B015C:3:0|2C1A6D1C4278:4:0|9CDC550B65F4:5:0|5CB469BF1388:6:0|1CF861BF1388:7:0|848A6C1C4278:8:0]


For the same command being sent to all modules, FFFF can be used. Ex:
- FFFF Character Commands //Set all modules to the same character, in this example a tilde.
 	****[{FFFF}|FFFF:~:0]
- FFFF Zero Commands //"Zero" all modules, re-indexing the start position using the hall effect sensor and the magnet
 	****[{FFFF}|FFFF:***ZERO:0]
- FFFF Reset Commands //Roughly equivalent to hitting the reset button. Makes the modules perform the startup sequence, which includes zero-ing.
 	****[{FFFF}|FFFF:***RESET:0]

//note: several of these functions are probably broken as of 7/15/2025
--------------------------------------------------------------------------------------------------------------------------------
I drew a custom PCB to be optionally used to house the Arduino and motor driver and provide more streamlined assembly (Just screw in your cables, assemble the 3d printed parts, and hot glue in the Hall effect, if you are into that kind of thing. It is NOT required, and I attempted to document the parts well in the spreadsheet. (There is a BOM as well.) If you choose to use the PCB, you will need to either solder jumper wires or get a bunch of jumping pins, as the "custom" non-esp32-standard-breakout parts are almost all in the bottom half and electrically isolated in case you change your mind and just want to use it as an ESP32 breakout. For reference, I was able to order 10 PCBA (assembled) units for $20 a pop (I don't like to think about it, questionably wise), and 20 non-assembled units for a "total" of $38.34 + $28 in import fees, which equates to ~$66.34 total or $3.34 per un-assembled part-less board. The Google sheets has a much better breakdown. Essentially, if you want to pay $15 per module for electronics and $5-$8 in printing costs, don't get a PCBa order. If you want to pay $40 for electronics and also $5-$8 for printing costs but reduce the assembly to screwing down 13 to 23 labeled cables (for left com, right com, Hall effect, and then possibly top / bottom comm {which are not supported in firmware yet}) and plugging in the motor cable, get the PCBA. If you want to pay about $18 per module for electronics and have a lot of soldering to do on top of the above cable-screwing, but prefer "solder in these headers" to "Attach D34 to the hall effect sensor OUT pin, which needs to have a 10k Resistor wired to the 3.3v pin...", then get a non-assembled PCB and order the parts. If you want to figure out import/duty fees, order the pcb components for assembly from China. Otherwise, Amazon has most of the parts, and if you find the right angle SMD xt30 you are a wizard.
--------------------------------------------------------------------------------------------------------------------------------
## License

This project uses a dual license approach:

- **Code (firmware, Arduino libraries)**: [GNU GPL v3.0](./LICENSE)
- **Design files (STLs, skp, skb, 3mf, fusion, onshape, other CAD, schematics, images, etc.)**: [Creative Commons BY-SA 4.0](./LICENSE.design)

Unless otherwise noted, source code files fall under GPLv3, and non-code files under CC BY-SA.
--------------------------------------------------------------------------------------------------------------------------------
### Disclaimer of Patent Liability

These files are provided under open licenses (GPL and CC BY-SA), but no patent rights are granted or implied.  
If use of this project or any of its parts infringes on third-party patents, you use it at your own risk.  
The author makes no warranties regarding patent freedom.
--------------------------------------------------------------------------------------------------------------------------------
### Disclaimer of Warranty

This project is provided "as is", without warranty of any kind — express or implied — including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement.  
In no event shall the author be liable for any claim, damages, or other liability arising from the use of this project, whether in an action of contract, tort, or otherwise.  
Use it at your own risk.
--------------------------------------------------------------------------------------------------------------------------------
In respect to the license, essentially you can:

- [✓] Use this project for personal or commercial purposes  
- [✓] Modify the code and designs  
- [✓] Share or remix your modified versions  
- [✓] Sell hardware or software based on it  

As long as you:

- [*] **Share improvements** under the same licenses (GPL for code, CC BY-SA for designs)  
- [*] **Give credit** to the original author (Neil Buffham)  
- [*] **Include the original license texts** when redistributing  
- [ ] **Do not** apply DRM or restrict others from doing these same things 

See the full license texts for legal details:  
- `LICENSE` for code (GPL v3)  
- `LICENSE.design` for designs (CC BY-SA 4.0)

Note: This is a human-friendly summary. The full license texts are authoritative and take precedence.
--------------------------------------------------------------------------------------------------------------------------------
Used in this project:

Overpass Mono Font  
- Source: https://fonts.google.com/specimen/Overpass+Mono  
- License: SIL Open Font License 1.1 (OFL) — https://openfontlicense.org  
- Copyright: © 2024 Red Hat, Inc.  
- Permission is granted to use, modify, and redistribute the font under the terms of the OFL.  
- The font is used in a slightly modified form for the characters on the flaps

AccelStepper Library  
- Source: https://docs.arduino.cc/libraries/accelstepper/  
- GitHub: https://github.com/waspinator/AccelStepper  
- License: GNU GPL v3.0 — https://www.gnu.org/licenses/gpl-3.0.html  
- Author: Mike McCauley / AirSpayce  
- The AccelStepper library is used in firmware to control the stepper motor.
