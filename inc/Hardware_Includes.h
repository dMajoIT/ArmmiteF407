/***********************************************************************************************************************
MMBasic

Hardware_Includes.h

Defines the hardware aspects for PIC32-Generic MMBasic.

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
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_usb.h"
#include "IOPorts.h"
#include "configuration.h"
#include "Timers.h"
#include "SPI-LCD.h"
#include "ff.h"
    // global variables
    extern int MMCharPos;
    extern char *StartEditPoint;
    extern int StartEditChar;
    extern char *InterruptReturn;
    extern char IgnorePIN;
    extern char WatchdogSet;
    extern char oc1, oc2, oc3, oc4, oc5, oc6, oc7, oc8, oc9;
    extern volatile MMFLOAT VCC;
    extern int PromptFont, PromptFC, PromptBC;                          // the font and colours selected at the prompt;
    extern void TM_USART2_ReceiveHandler(uint8_t c);
    typedef struct {
    	uint8_t *Buffer;
    	uint16_t Size;
    	uint16_t Num;
    	uint16_t In;
    	uint16_t Out;
    	uint8_t Initialized;
    	uint8_t StringDelimiter;
    } TM_USART_t;
    extern volatile unsigned int WDTimer;                               // used for the watchdog timer
    extern TM_USART_t TM_USART2;
    extern TM_USART_t TM_USART3;
    extern int BasicRunning;
    extern uint32_t PROG_FLASH_SIZE;
    // console related I/O
    int MMInkey(void);
    int MMgetchar(void);
    char MMputchar(char c);
    extern void CheckAbort(void) ;
    extern void TM_USART_INT_InsertToBuffer(TM_USART_t* u, uint8_t c);
    int kbhitConsole(void);
    void putConsole(int c);
    void SoftReset(void);
    extern void SerUSBPutS(char *s);
    extern void SerUSBPutC(char c);
    void SaveProgramToFlash(char *pm, int msg);
    int getConsole(void);
    void initSerialConsole(void);
    extern int pattern_matching (	/* 0:not matched, 1:matched */
    	const TCHAR* pat,	/* Matching pattern */
    	const TCHAR* nam,	/* String to be tested */
    	int skip,			/* Number of pre-skip chars (number of ?s) */
    	int inf				/* Infinite search (* specified) */
    );
    // Use the core timer.  The maximum delay is 4 seconds
    void uSec(unsigned int us);
    void shortpause(unsigned int ticks);
    // used to control the processor reset
    extern unsigned int _excep_dummy;//  __attribute__ ((persistent)); // for some reason persistent does not work on the first variable
    extern unsigned int _excep_code;//  __attribute__ ((persistent));  // if there was an exception this is the exception code
    extern unsigned int _excep_addr;//  __attribute__ ((persistent));  // and this is the address
    extern void PInt(int n);
    extern void PIntComma(int n);
    extern void PO2Str(char *s1, const char *s2);
    extern void PO2Int(char *s1, int n);
    extern void PO3Int(char *s1, int n1, int n2);
    extern void PIntH(unsigned int n);
    extern void PIntHC(unsigned int n);
    extern void PFlt(MMFLOAT flt);
    extern void PFltComma(MMFLOAT n);
    extern void PPinName(int n);
    extern void PPinNameComma(int n);

    #ifdef __DEBUG
        void dump(char *p, int nbr);
    #endif

//    #define dp(...) {char s[140];sprintf(s,  __VA_ARGS__); MMPrintString(s); MMPrintString("\r\n");}

    #define db(i) {IntToStr(inpbuf, i, 10); MMPrintString(inpbuf); MMPrintString("\r\n");}
    #define db2(i1, i2) {IntToStr(inpbuf, i1, 10); MMPrintString(inpbuf); MMPrintString("  "); IntToStr(inpbuf, i2, 10); MMPrintString(inpbuf); MMPrintString("\r\n");}
    #define db3(i1, i2, i3) {IntToStr(inpbuf, i1, 10); MMPrintString(inpbuf); MMPrintString("  "); IntToStr(inpbuf, i2, 10); MMPrintString(inpbuf); MMPrintString("  "); IntToStr(inpbuf, i3, 10); MMPrintString(inpbuf); MMPrintString("\r\n");}
    #define ds(s) {MMPrintString(s); MMPrintString("\r\n");}
    #define ds2(s1, s2) {MMPrintString(s1); MMPrintString(s2); MMPrintString("\r\n");}
    #define ds3(s1, s2, s3) {MMPrintString(s1); MMPrintString(s2); MMPrintString(s3); MMPrintString("\r\n");}
    #define pp(i) { PinSetBit(i, TRISCLR); PinSetBit(i, ODCCLR); PinSetBit(i, LATCLR); PinSetBit(i, LATSET); uSec(30); PinSetBit(i, LATCLR); }
    #define pp2(i) { PinSetBit(i, TRISCLR); PinSetBit(i, ODCCLR); PinSetBit(i, LATCLR); PinSetBit(i, LATSET); uSec(30); PinSetBit(i, LATCLR); uSec(30); PinSetBit(i, LATSET); uSec(30); PinSetBit(i, LATCLR); }
    #define pp3(i) { PinSetBit(i, TRISCLR); PinSetBit(i, ODCCLR); PinSetBit(i, LATCLR); PinSetBit(i, LATSET); uSec(30); PinSetBit(i, LATCLR); uSec(30); PinSetBit(i, LATSET); uSec(30); PinSetBit(i, LATCLR); uSec(30); PinSetBit(i, LATSET); uSec(30); PinSetBit(i, LATCLR); }
#endif
#define VGA             1
#define SSD1963_4       2
#define SSD1963_5       3
#define SSD1963_5A      4
#define SSD1963_7       5
#define SSD1963_7A      6
#define SSD1963_8       7
#define SSD_PANEL_8     SSD1963_8    // anything less than or equal to SSD_PANEL is handled by the SSD 8-bit driver, anything more by the 16-bit or SPI driver

#define SSD1963_4_16    9
#define SSD1963_5_16    10
#define SSD1963_5A_16   11
#define SSD1963_7_16    12
#define SSD1963_7A_16   13
#define SSD1963_8_16    14
#define SSD_PANEL       SSD1963_8_16    // anything less than or equal to SSD_PANEL is handled by the SSD driver, anything more by the SPI driver

#define ILI9163         16
#define ST7735          17
#define SPI_PANEL       ST7735   // anything greater than SPI_PANEL is handled by otherdisplays

#define USER            19
#define ILI9481         20
#define ILI9341		    21
//#define RESERVED1     22
#define ILI9341_16      23
//#define RESERVED1     24
//#define RESERVED2     25
#define IPS_4_16        26

#define SSD1963_5_640   28
#define SSD1963_7_640   29
#define SSD1963_8_640   30
#define SSD1963_5_8BIT  31
#define SSD1963_7_8BIT  32
#define SSD1963_8_8BIT  33
#define HDMI			34

#define LANDSCAPE       1
#define PORTRAIT        2
#define RLANDSCAPE      3
#define RPORTRAIT       4
#define DISPLAY_LANDSCAPE   (Option.DISPLAY_ORIENTATION & 1)
#define TOUCH_NOT_CALIBRATED    -9999
#define RESET_COMMAND       9999                                // indicates that the reset was caused by the RESET command
#define WATCHDOG_TIMEOUT    9998                                // reset caused by the watchdog timer
#define PIN_RESTART         9997                                // reset caused by entering 0 at the PIN prompt
#define RESTART_NOAUTORUN   9996                                // reset required after changing the LCD or touch config
#define RESTART_HEAP		9995
#define SD_SLOW_SPI_SPEED 0
#define SD_FAST_SPI_SPEED 1
#define LCD_SPI_SPEED    2                                   // the speed of the SPI bus when talking to an SPI LCD display controller
#define TOUCH_SPI_SPEED 3
#define NONE_SPI_SPEED 4
#define IsxDigit(a) isxdigit((uint8_t)a)
#define IsDigit(a) isdigit((uint8_t)a)
#define IsAlpha(a) isalpha((uint8_t)a)
#define IsPrint(a) isprint((uint8_t)a)
#define IsAlnum(a) isalnum((uint8_t)a)

#include "Serial.h"
#include "FileIO.h"
#include "Memory.h"
#include "External.h"
#include "MM_Misc.h"
#include "MM_Custom.h"
#include "Onewire.h"
#include "I2C.h"
#include "SerialFileIO.h"
#include "PWM.h"
#include "SPI.h"
#include "Flash.h"
#include "Xmodem.h"
#include "Draw.h"
#include "editor.h"
#include "ff.h"
#include "diskio.h"
#include "touch.h"
#include "SSD1963.h"
#include "GUI.h"
#include "audio.h"
#include "OtherDisplays.h"
#include "gps.h"
#include "PS2Keyboard.h"
#include "Maths.h"

