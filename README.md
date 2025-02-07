Files to build the Armmite F4 MMBasic running on the STM32F407VET6



The STM32CubeIDE project is directory ArmmiteF407. This should be placed in your STM32CubeIDE workspace.   
e.g. workspace/ArmmiteF407 and opened and compiled using STM32CubeIDE ver 1.9.0   
Use GCC 10.3-2021.10 to ensure the CCRAM is not exceeded. The later version seem to use more CCRAM  
A compiled binary version is under the the binaries directory.
A user manual for MMBasic on the STM32F407 is under the docs directory.
A compiled binary version is under the the binaries directory.  
A user manual for MMBasic on the STM32F407 is under the docs directory.  


Change list from V5.07.00  


V5.07.02b4:  
An alternate firmware ARMmiteF407PC13Reset.bin is provided to move the testing for an MMBasic Reset off PE3 (K1)     
and also the testing for Option Serial Console off PE4(K0). Pin PC13 is now used to trigger these events.   
This allows PE4 and PE3 to be used without risking an unintended MMBasic Reset or switch to Serial Console.   
3.3V on PC13 will trigger an MMBasic Reset  
GND on PC13 will trigger a switch to the Serial Console.  
!!! MMBasic Reset !!! Now printed on console after an MMBasic Reset is forced.  
MM.INFO(BOOT) will return the reason for the last MMBasic Restart.  
“Power On”, ”Reset Switch”, “MMBasic Reset”, ”CPU RESTART”, “Watchdog”, “EXECUTE Timout” or “HEAP Restart”.  
These could be written to a log file to see the history of an unattended device.    
LCDPANEL ILI9341_I removed.    
OPTION LCDPANEL accepts [,INVERT] to invert colours on ILI9341 and ILI9488   
In the commandline editor the automatic OVR mode on right arrow removed to lineup with the Picomites.  
Comments are no longer tokenised as per Picomites.  
MATH V_PRINT array() [,HEX]  outputs the contents in hex   
LONGSTRING PRINT allows ; to suppress CRLF  
CAN OPEN syntax adjusted slightly  
Function INKEY$ changed so it automatically resolves Function and special keys  
as per picomites and CMM2.  
SHIFT F3-F8 Added as per picomite 6.00.02B0  
//NB: SHIFT F1, F2, F9, F10, F11, and F12 don't appear to generate anything  
AUTOSAVE APPEND  added.  
MEMORY PACK/UNPACK removed to recover flash.   

V5.07.02b3:  
Implements the new functions for parsing command parameters as used in the Picomite code.(i.e.MATH,LONGSTRING,ADC, commands). This has resulted in the freeing up of significant Flash, allowing some commands previously omitted to save space to be now included.

CAN [OPEN|CLOSE|START|STOP|FILTER|SEND|READ] commands added  
FSMC pins now available for MMBasic use if no parallel display configured.  
MATH C_ADD etc commands that act on cells added as per picomites.  
MATH SHIFT added  as per picomites  
MATH WINDOW added as per picomites  
MATH WINDOW fixed when input and output matrix are not the same type  
MATH SD formula changed to Sample Standard Deviation as per Picomite  
MEMORY PACK/UNPACK added.  
LINE PLOT command added as per picomites  
LINE GRAPH command added as per picomites  
LINE command updated getargaddress() function from Picomite with fix for % in LINE  
Fix for MATH M_MULT not  giving correct results.  
Improved MATH M_MULT as per Pico RC5 (Added but not yet tested)  
Improved MATH M_PRINT as per Pico RC5 (Added but not yet tested)  
MATH command accepts either C_MUL or C_MULT. (as per Picomite)  
Added MM.INFO(NBRPINS)  returns 100  
Added MM.INFO(OPTION VCC) returns current value for   
Updates to CommandLine Buffer as per Picomite.    
BITBANG becomes DEVICE as per picomite latest release.    
DEVICE BITSTREAM  
DEVICE LCD  
DEVICE HUMID   
DEVICE WS2812  
DEVICE BITSTREAM error in calculation fixed   
LIBRARY CHECK becomes LIBRARY RESTORE is more closely match its function   
Fix to allow no space after = in DIM i%=12345 as per picomite.   
Fix to command READ not finding DATA statement if token 255 is in use.(as per picomites)  
Fix for EXECUTE command in beta2 not having a timeout value.  
Print datetime$(epoch(now)) no longer gives error.  
Extend PEEK range to allow PEEK(VARADDR var) for non array integers and floats.   
MEMORY command shows variables again.   
MM.INFO(PINNO pin ) now returns correct value.  
OPTION ANGLE now defaults to RADIANS after CNTRL C or END command.  
COM4 TX and RX were partially reversed, A pullup on RX possibly not released when   
COM4 closed . Now resolved.  
ADC START now accepts additional parameters as per the Picomites that allow mapping of the the output data to a desired range.   
ADC TRIGGER now allows a timeout value to be set. 'timeout' is the number of ADC samples to take before abandoning the wait for    
the trigger condition. Setting a value equal to the frequency of the sampling would give a timeout of 1 second.  
ADC TRIGGER,channel,level[,timeout]  
Fix for freeze at command prompt after CNTRL C, ERROR , WATCHDOG if DAC was running.  
Fix for incorrect frequency and pulse width generated on SERVO 3 command.  
Fix for error when datetime$(epoch(now)) used.  
Added ~ option to error() as per picomite and call it from getint() function  
to allow correct error message format.   
Improved scrolling for Editor and Console for non SSD1963 parallel displays:   
 Editor rewrites in lieu of scrolling and  
 OPTION LCDPANEL CONSOLE ,,,,,, NOSCROLL added for 16bit parallel non SSD displays  
 to improve scrolling when used as a Console.  
REMOVED - OPTION LCDPANEL CONSOLE support on SPI LCD PANELS  



V5.07.02b2:  
Added Multiline comments to editor.       
Fixed potential issue with FILES command.       
Closed SPI before saving Library incase it was open from a previous issue.   
BACKLIGHT syntax updated to   
BACKLIGHT percentage% [, DEFAULT|REVERSE] 
Using \\000 and \&00 (with OPTION ESCAPE) now gives error message to use CHR$(0) as per picomites  
Fix for OPTION ANGLE not defaulting to RADIANS on new RUN command.  
Fix for OPTION ANGLE not in function TAN()  
Fix for OPTION ANGLE not in function MATH (ATAN3 x,y)  
Licence text added to source files.  

V5.07.02b1:  
Added ST7735S Display  
Fixed 'Not Enough Memory' error when using EDIT command with OPTION LCDPANEL CONSOLE on with  non SSD1963 display.

V5.07.02b0:  
Initial release of V5.07.02






*****************************************************************************   
Armmite F407 MMBasic   
MMBasic  for STM32F407VET6 (Armmite F4)

Copyright 2011-2023 Geoff Graham and  Peter Mather.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************  

 In addition the software components from STMicroelectronics are provided   
 subject to the license as detailed below:   
   
  ******************************************************************************   
  * @attention   
  *
  * <center>&copy; Copyright (c) 2019 STMicroelectronics.   
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license   
  * SLA0044, the "License"; You may not use this file except in compliance with   
  * the License. You may obtain a copy of the License at:  
  *                             www.st.com/SLA0044  
  *
  ******************************************************************************  
 
