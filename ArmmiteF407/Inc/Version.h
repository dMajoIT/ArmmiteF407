/***********************************************************************************************************************
MMBasic

Version.h

Copyright 2011 - 2015 Geoff Graham.  All Rights Reserved.

This file and modified versions of this file are supplied to specific individuals or organisations under the following
provisions:

- This file, or any files that comprise the MMBasic source (modified or not), may not be distributed or copied to any other
  person or organisation without written permission.

- Object files (.o and .hex files) generated using this file (modified or not) may not be distributed or copied to any other
  person or organisation without written permission.

- This file is provided in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

************************************************************************************************************************/

//#define DEBUGMODE

#define VERSION         "5.07.02b0"
#define MES_SIGNON  "ARMmite MMBasic Version " VERSION
#define YEAR		"2011-2023"			    // and the year
#define YEAR2          "2016-2023"
#define COPYRIGHT  "\r\nCopyright " YEAR " Geoff Graham\r\nCopyright " YEAR2 " Peter Mather\r\n"



// These options are compiled conditionally
    // Uncomment the following line if you want the Command History included
    #define CMDHISTORY
    // Uncomment the following line if you want the * RUN shortcut  included
    #define RUNSHORTCUT


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // debugging options
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  // #define DEBUGMODE                     // enable debugging macros (with reduced program memory for the Micromite)

 // VET6 MINI with ILI9486_16 FLASH_CS =77
 //#define TEST_CONFIG "OPTION LCDPANEL DISABLE : OPTION LCDPANEL ILI9486_16, LANDSCAPE : OPTION TOUCH PB12, PC5 : GUI CALIBRATE 0, 4012, 3900, -1271, -868 : BACKLIGHT 80,S : OPTION FLASH_CS 77 : OPTION SAVE : ? \"ILI9486_16 ON\""

 // VET6 ILI9341 SPI  SERIAL CONSOLE 38400
 // #define TEST_CONFIG "OPTION LCDPANEL SSD1963_5, LANDSCAPE, 48, 6 : OPTION TOUCH 1, 40, 39 : OPTION SDCARD 47 : GUI CALIBRATE 1, 108, 3849, 2050, -1342 : OPTION SAVE : ? \"SSD1963_5 ON\""

