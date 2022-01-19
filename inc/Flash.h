/***********************************************************************************************************************
MMBasic

Flash.h

Include file that contains the globals and defines for flash save/load in MMBasic.
  
Copyright 2011 - 2019 Geoff Graham.  All Rights Reserved.

This file and modified versions of this file are supplied to specific individuals or organisations under the following 
provisions:

- This file, or any files that comprise the MMBasic source (modified or not), may not be distributed or copied to any other
  person or organisation without written permission.

- Object files (.o and .hex files) generated using this file (modified or not) may not be distributed or copied to any other
  person or organisation without written permission.

- This file is provided in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

************************************************************************************************************************/
#include "stdint.h"
#define FLASH_BASE_ADDR      (uint32_t)(FLASH_BASE)
#define FLASH_END_ADDR       (uint32_t)(0x08200000)

		#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
		#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
		#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
		#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
		#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
		#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
		#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
		#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
		#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
		#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
		#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
		#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
		#define FLASH_PROGRAM_ADDR       ADDR_FLASH_SECTOR_7   /* Start Basic Program flash area */
		#define FLASH_SAVED_OPTION_ADDR  ADDR_FLASH_SECTOR_1   /* Start of Saved Options flash area */
//		#define FLASH_SAVED_VAR_ADDR     ADDR_FLASH_SECTOR_2   /* Start of Saved Variables flash area */
//		#define SAVEDVARS_FLASH_SIZE 16384  // amount of flash reserved for saved variables
	#define SAVED_VAR_RAM_ADDR     ((uint32_t)0x40024000)   /* Start of Saved Variables flash area */
//	#define FLASH_PROGRAM_ADDR       ADDR_FLASH_SECTOR_0_BANK2   /* Start Basic Program flash area */
	#define SAVED_VAR_RAM_SIZE 0x1000  // amount of flash reserved for saved variables

/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE) && !defined(FLASH_INCLUDED)
#define FLASH_INCLUDED

    // IMPORTANT: Change the string constant in cmd_memory() if you change PROG_FLASH_SIZE
#define EDIT_BUFFER_SIZE  ((unsigned int)(RAMEND - (unsigned int)RAMBase - 1024))  // this is the maximum RAM that we can get
//#define SAVED_OPTIONS_FLASH 2
//#define SAVED_VARS_FLASH 2
#define PROGRAM_FLASH 1

struct option_s {
    char Autorun;
    char Tab;
    char Invert;
    char Listcase;
    char Height;
    char Width;
    char  ColourCode;
    char DISPLAY_TYPE;
    char DISPLAY_ORIENTATION;
    unsigned char TOUCH_CS;
    unsigned char TOUCH_IRQ;
    char TOUCH_SWAPXY;
    unsigned char LCD_CD;
    unsigned char LCD_CS;
    unsigned char LCD_Reset;
    char SerialConDisabled;
    char SSDspeed;
    char DISPLAY_CONSOLE;
    char DefaultFont;
    char KeyboardConfig;
    unsigned char TOUCH_Click;
    char DefaultBrightness;         // default backlight brightness
    char SerialPullup;
    char fulltime;
    char Refresh;
    char dummy[3];
    short MaxCtrls;                // maximum number of controls allowed
    short RTC_Calibrate;  //32 bytes
    int DISPLAY_WIDTH;
    int DISPLAY_HEIGHT; //40
    uint32_t  PIN;
    uint32_t  Baudrate;  //48
    MMFLOAT TOUCH_XSCALE;
    MMFLOAT TOUCH_YSCALE; //64 bytes
    unsigned int ProgFlashSize;    // used to store the size of the program flash (also start of the LIBRARY code)
    int DefaultFC, DefaultBC;      // the default colours
    short  TOUCH_XZERO;
    short  TOUCH_YZERO; //80 bytes

};

extern volatile struct option_s Option, *SOption;
extern unsigned char *CFunctionFlash, *CFunctionLibrary;
extern volatile uint32_t  realflashpointer;
void ResetAllOptions(void);
void ResetAllFlash(void);
void SaveOptions(void);
void LoadOptions(void);
void FlashWriteInit(int sector);
void FlashWriteByte(unsigned char b);
void FlashWriteWord(unsigned int i);
void FlashWriteAlign(void);
void FlashWriteClose(void);
void UpdateFlash(uint32_t address, uint32_t data);
int GetFlashOption(const unsigned int *w) ;
void SetFlashOption(const unsigned int *w, int x) ;
uint32_t GetSector(uint32_t Address);
void cmd_var(void);
long long int CallCFunction(char *CmdPtr, char *ArgList, char *DefP, char *CallersLinePtr);
extern char * ProgMemory;
extern void ClearSavedVars(void);
void RoundDoubleFloat(MMFLOAT *ff);


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


#endif


/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

	{ "VAR",	    	T_CMD,				0, cmd_var	},

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE

#endif
