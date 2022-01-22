/***********************************************************************************************************************
MMBasic

Flash.c

Handles saving and restoring from flash.

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



#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
//#include "stm32f4xx_flash.h"
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
// The CFUNCTION data comes after the program in program memory
// and this is used to point to its start
unsigned char *CFunctionFlash = NULL;
unsigned char *CFunctionLibrary = NULL;
volatile struct option_s Option, *SOption;
char * ProgMemory;
static FLASH_EraseInitTypeDef EraseInitStruct;
uint32_t SectorError = 0;
volatile union u_flash {
  uint32_t i32;
  uint8_t  i8[4];
} FlashWord, FlashWordSave;
volatile int i8p=0;
extern volatile int ConsoleTxBufHead, ConsoleTxBufTail;
int sectorsave;
// globals used when writing bytes to flash
volatile uint32_t realflashpointer;
uint32_t GetSector(uint32_t Address);
extern RTC_HandleTypeDef hrtc;

// erase the flash and init the variables used to buffer bytes for writing to the flash
void FlashWriteInit(int sector) {
//	__IO uint32_t SectorsWRPStatus = 0xFFF;
    // Unlock the Flash to enable the flash control register access
	HAL_SuspendTick();
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
	HAL_FLASH_Unlock();
    i8p=0;
	FlashWord.i32=0xFFFFFFFF;
	sectorsave=sector;
    // Clear pending flags (if any)

    // Get the number of the start and end sectors

       // Device voltage range supposed to be [2.7V to 3.6V], the operation will
       //  be done by word
      if(sector == PROGRAM_FLASH){
    	  realflashpointer=FLASH_PROGRAM_ADDR;
    	  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    	  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    	  EraseInitStruct.Sector        = GetSector(FLASH_PROGRAM_ADDR);
    	  EraseInitStruct.NbSectors     = 1;
       	  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
       		  uSec(1000);
           	  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
           		  error("Program flash erase");
           	  }
       	  }
      }
/*      if(sector == SAVED_OPTIONS_FLASH){
      	  realflashpointer=FLASH_SAVED_OPTION_ADDR ;
    	  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    	  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    	  EraseInitStruct.Sector        = GetSector(FLASH_SAVED_OPTION_ADDR);
    	  EraseInitStruct.NbSectors     = 1;
       	  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
       		  uSec(1000);
           	  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
           		  error("Option flash erase");
           	  }
       	  }
     }
     if(sector == SAVED_VARS_FLASH){
       	  realflashpointer=FLASH_SAVED_VAR_ADDR ;
    	  EraseInitStruct.Banks			= FLASH_BANK_1;
    	  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    	  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    	  EraseInitStruct.Sector        = GetSector(FLASH_SAVED_VAR_ADDR);
    	  EraseInitStruct.NbSectors     = 1;
       	  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
       		  uSec(1000);
           	  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
           		  error("Saved variable flash erase");
           	  }
       	  }
    }*/

    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
	HAL_ResumeTick();
}
void FlashWriteBlock(void){
    int i;
    uint32_t address=realflashpointer-4;
    uint32_t *there = (uint32_t *)address;

    if(address % 4)error("Flash write address");
    if(sectorsave == PROGRAM_FLASH && (address<FLASH_PROGRAM_ADDR || address>=FLASH_PROGRAM_ADDR+PROG_FLASH_SIZE))error("PROGRAM_FLASH location &",address);
//    if(sectorsave == SAVED_OPTIONS_FLASH && (address<FLASH_SAVED_OPTION_ADDR || address>=FLASH_SAVED_OPTION_ADDR+0x4000))error("SAVED_OPTIONS_FLASH location &",address);
//    if(sectorsave == SAVED_VARS_FLASH && (address<FLASH_SAVED_VAR_ADDR || address>=FLASH_SAVED_VAR_ADDR+0x4000))error("SAVED_VARS_FLASH location &",address);

    	if(*there!=0xFFFFFFFF) error("flash not erased");

	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, FlashWord.i32) != HAL_OK)
	{
		error("Flash write fail");
	}
	for(i=0;i<4;i++)FlashWord.i8[i]=0xFF;
}
// write a byte to flash
// this will buffer four bytes so that the write to flash can be a word
void FlashWriteByte(unsigned char b) {
	realflashpointer++;
	FlashWord.i8[i8p]=b;
	i8p++;
	i8p %= 4;
	if(i8p==0){
		FlashWriteBlock();
	}
}

void FlashWriteAlign(void) {
	  while(i8p != 0) {
		  FlashWriteByte(0x0);
	  }
	  FlashWriteWord(0xFFFFFFFF);
}

// utility routine used by SaveProgramToFlash() and cmd_var to write a byte to flash while erasing the page if needed
void FlashWriteWord(unsigned int i) {
	FlashWriteByte(i & 0xFF);
	FlashWriteByte((i>>8) & 0xFF);
	FlashWriteByte((i>>16) & 0xFF);
	FlashWriteByte((i>>24) & 0xFF);
}


// flush any bytes in the buffer to flash
void FlashWriteClose(void) {
	  while(i8p != 0) {
		  FlashWriteByte(0xff);
	  }
}



/*******************************************************************************************************************
 Code to execute a CFunction saved in flash
*******************************************************************************************************************/
/*******************************************************************************************************************
 VARSAVE and VARSAVE RESTORE commands

 Variables are saved in flash as follows:
 Numeric variables:
     1 byte  = variable type
     ? bytes = the variable's name in uppercase
     1 byte  = zero byte terminating the variable's name
     4 or 8 bytes = the value of the variable
 String variables:
     1 byte  = variable type
     ? bytes = the variable's name in uppercase
     1 byte  = zero byte terminating the variable's name
     1 bytes = length of the variable's string
     ? bytes = the variables string

********************************************************************************************************************/


/*******************************************************************************************************************
 The variables are stored in a reserved flash area (which in total is 2K).
 The first few bytes are used for the options. So we must save the options in RAM before we erase, then write the
 options back.  The variables saved by this command are then written to flash starting just after the options.
********************************************************************************************************************/
void cmd_var(void) {
    char *p, *buf, *bufp, *varp, *vdata, lastc;
    int i, j, nbr = 1, nbr2=1, array, type, SaveDefaultType;
    int VarList[MAX_ARG_COUNT];
    char *VarDataList[MAX_ARG_COUNT];
    char *SavedVarsFlash;
    char *w;
    if((p = checkstring(cmdline, "CLEAR"))) {
        checkend(p);
        ClearSavedVars();
        return;
    }
    if((p = checkstring(cmdline, "RESTORE"))) {
        char b[MAXVARLEN + 3];
        checkend(p);
        SavedVarsFlash = (char*)SAVED_VAR_RAM_ADDR;      // point to where the variables were saved
//        if(SavedVarsFlash[4] == 0) return;                          // zero in this location means that nothing has ever been saved
        SaveDefaultType = DefaultType;                              // save the default type
        bufp = SavedVarsFlash;   // point to where the variables were saved
        while(*bufp != 0xff) {                                      // 0xff is the end of the variable list
            type = *bufp++;                                         // get the variable type
            array = type & 0x80;  type &= 0x7f;                     // set array to true if it is an array
            DefaultType = TypeMask(type);                           // and set the default type to this
            if(array) {
                strcpy(b, bufp);
                strcat(b, "()");
                vdata = findvar(b, type | V_EMPTY_OK | V_NOFIND_ERR);     // find an array
            } else
                vdata = findvar(bufp, type | V_FIND);               // find or create a non arrayed variable
            if(TypeMask(vartbl[VarIndex].type) != TypeMask(type)) error("$ type conflict", bufp);
            if(vartbl[VarIndex].type & T_CONST) error("$ is a constant", bufp);
            bufp += strlen((char *)bufp) + 1;                       // step over the name and the terminating zero byte
            if(array) {                                             // an array has the data size in the next two bytes
                nbr = *bufp++;
                nbr |= (*bufp++) << 8;
                nbr |= (*bufp++) << 16;
                nbr |= (*bufp++) << 24;
                nbr2 = 1;
                for(j = 0; vartbl[VarIndex].dims[j] != 0 && j < MAXDIM; j++)
                    nbr2 *= (vartbl[VarIndex].dims[j] + 1 - OptionBase);
                if(type & T_STR) nbr2 *= vartbl[VarIndex].size +1;
                if(type & T_NBR) nbr2 *= sizeof(MMFLOAT);
                if(type & T_INT) nbr2 *= sizeof(long long int);
                if(nbr2!=nbr)error("Array size");
            } else {
               if(type & T_STR) nbr = *bufp + 1;
               if(type & T_NBR) nbr = sizeof(MMFLOAT);
               if(type & T_INT) nbr = sizeof(long long int);
            }
            while(nbr--) *vdata++ = *bufp++;                        // copy the data
        }
        DefaultType = SaveDefaultType;
        return;
    }

     if((p = checkstring(cmdline, "SAVE"))) {
        getargs(&p, (MAX_ARG_COUNT * 2) - 1, ",");                  // getargs macro must be the first executable stmt in a block
        if(argc && (argc & 0x01) == 0) error("Invalid syntax");

        // befor we start, run through the arguments checking for errors
        // before we start, run through the arguments checking for errors
        for(i = 0; i < argc; i += 2) {
            checkend(skipvar(argv[i], false));
            VarDataList[i/2] = findvar(argv[i], V_NOFIND_ERR | V_EMPTY_OK);
            VarList[i/2] = VarIndex;
            if((vartbl[VarIndex].type & (T_CONST | T_PTR)) || vartbl[VarIndex].level != 0) error("Invalid variable");
            p = &argv[i][strlen(argv[i]) - 1];                      // pointer to the last char
            if(*p == ')') {                                         // strip off any empty brackets which indicate an array
                p--;
                if(*p == ' ') p--;
                if(*p == '(')
                    *p = 0;
                else
                    error("Invalid variable");
            }
        }
        // load the current variable save table into RAM
        // while doing this skip any variables that are in the argument list for this save
        bufp = buf = GetTempMemory(SAVED_VAR_RAM_SIZE);           // build the saved variable table in RAM
        SavedVarsFlash = (char*)SAVED_VAR_RAM_ADDR;      // point to where the variables were saved
        varp = SavedVarsFlash;   // point to where the variables were saved
        while(SavedVarsFlash[4] != 0 && *varp != 0xff) {            // 0xff is the end of the variable list, SavedVarsFlash[4] = 0 means that the flash has never been written to
            type = *varp++;                                         // get the variable type
            array = type & 0x80;  type &= 0x7f;                     // set array to true if it is an array
            vdata = varp;                                           // save a pointer to the name
            while(*varp) varp++;                                    // skip the name
            varp++;                                                 // and the terminating zero byte
            if(array) {                                             // an array has the data size in the next two bytes
                 nbr = (varp[0] | (varp[1] << 8) | (varp[2] << 16) | (varp[3] << 24)) + 4;
            } else {
                if(type & T_STR) nbr = *varp + 1;
                if(type & T_NBR) nbr = sizeof(MMFLOAT);
                if(type & T_INT) nbr = sizeof(long long int);
            }
            for(i = 0; i < argc; i += 2) {                          // scan the argument list
                p = &argv[i][strlen(argv[i]) - 1];                  // pointer to the last char
                lastc = *p;                                         // get the last char
                if(lastc <= '%') *p = 0;                            // remove the type suffix for the compare
                if(strncasecmp(vdata, argv[i], MAXVARLEN) == 0) {   // does the entry have the same name?
                    while(nbr--) varp++;                            // found matching variable, skip over the entry in flash (ie, do not copy to RAM)
                    i = 9999;                                       // force the termination of the for loop
                }
                *p = lastc;                                         // restore the type suffix
            }
            // finished scanning the argument list, did we find a matching variable?
            // if not, copy this entry to RAM
            if(i < 9999) {
                *bufp++ = type | array;
                while(*vdata) *bufp++ = *vdata++;                   // copy the name
                *bufp++ = *vdata++;                                 // and the terminating zero byte
                while(nbr--) *bufp++ = *varp++;                     // copy the data
            }
        }


        // initialise for writing to the flash
//        FlashWriteInit(SAVED_VARS_FLASH);
        w=(char*)SAVED_VAR_RAM_ADDR;
        for(i=0;i<SAVED_VAR_RAM_SIZE;i++)*w++=0xFF;

        // now write the variables in RAM recovered from the var save list
        w=(char*)SAVED_VAR_RAM_ADDR;
        while(buf < bufp){
        	*w++=*buf++;
        }

        // now save the variables listed in this invocation of VAR SAVE
        for(i = 0; i < argc; i += 2) {
            VarIndex = VarList[i/2];                                // previously saved index to the variable
            vdata = VarDataList[i/2];                               // pointer to the variable's data
            type = TypeMask(vartbl[VarIndex].type);                 // get the variable's type
            type |= (vartbl[VarIndex].type & T_IMPLIED);            // set the implied flag
            array = (vartbl[VarIndex].dims[0] != 0);

            nbr = 1;                                                // number of elements to save
            if(array) {                                             // if this is an array calculate the number of elements
                for(j = 0; vartbl[VarIndex].dims[j] != 0 && j < MAXDIM; j++)
                    nbr *= (vartbl[VarIndex].dims[j] + 1 - OptionBase);
                type |= 0x80;                                       // an array has the top bit set
            }

            if(type & T_STR) {
                if(array)
                    nbr *= (vartbl[VarIndex].size + 1);
                else
                    nbr = *vdata + 1;                               // for a simple string variable just save the string
            }
            if(type & T_NBR) nbr *= sizeof(MMFLOAT);
            if(type & T_INT) nbr *= sizeof(long long int);
            if((uint32_t)w - (uint32_t)SavedVarsFlash + 36 + nbr > SAVED_VAR_RAM_SIZE) {
//                FlashWriteClose();
                error("Not enough memory");
            }
            *w++=type;                              // save its type
            for(j = 0, p = vartbl[VarIndex].name; *p && j < MAXVARLEN; p++, j++)
                *w++=*p;                            // save the name
            *w++=0;                                 // terminate the name
            if(array) {                                             // if it is an array save the number of data bytes
               *w++=nbr; *w++=(nbr >> 8); *w++=(nbr >>16); *w++=(nbr >>24);
            }
            while(nbr--) *w++=(*vdata++);             // write the data
        }
//        FlashWriteClose();
        return;
     }
    error("Unknown command");
}

/**********************************************************************************************
   These routines are used to load or save the global Option structure from/to flash.
   These options are stored in the beginning of the flash used to save stored variables.
***********************************************************************************************/


/*void SaveOptions(void) {
	int i;
    char *p, *SavedOptionsFlash;
    SavedOptionsFlash=(char*)FLASH_SAVED_OPTION_ADDR;
    p = (char *)&Option;
    for(i = 0; i < sizeof(struct option_s); i++) {
    	if(SavedOptionsFlash[i] != *p) goto skipreturn;
    	p++;
    }
    return;                                                         // exit if the option has already been set (ie, nothing to do)
    skipreturn:

    while(!(ConsoleTxBufHead == ConsoleTxBufTail)){};                    // wait for the console UART to send whatever is in its buffer
    p = (char *)&Option;
    FlashWriteInit(SAVED_OPTIONS_FLASH);                // erase the page
    for(i = 0; i < sizeof(struct option_s); i++){
    	FlashWriteByte(*p);    // write the changed page back to flash
    	p++;
    }
    FlashWriteClose();

}


void LoadOptions(void) {
    memcpy((struct option_s *)&Option, (struct option_s *)FLASH_SAVED_OPTION_ADDR, sizeof(struct option_s));
}*/

void SaveOptions(void) {
	uint32_t *p, *q;
	int i,readback[20];
    p = q = (uint32_t *)&Option;
    for(i = 0; i < 20; i++){
    	HAL_RTCEx_BKUPWrite (&hrtc, i, *p++);
    }
    for(i = 0; i < 20; i++){
		readback[i] = HAL_RTCEx_BKUPRead (&hrtc, i);
	}
    for(i=0; i < 20; i++){
    	if(readback[i]!=*q++)error("Options not saved");
    }
}


void LoadOptions(void) {
	uint32_t *p;
    p = (uint32_t *)&Option;
	int i;
    for(i = 0; i < 20; i++){
		*p++ = HAL_RTCEx_BKUPRead (&hrtc, i);
	}
}


// erase all flash memory and reset the options to their defaults
// used on initial firmware run
void ResetAllOptions(void) {
    Option.Height = SCREENHEIGHT;                                   // reset the options to their defaults
    Option.Width = SCREENWIDTH;
    Option.PIN = 0;
    Option.Baudrate = CONSOLE_BAUDRATE;
    Option.Autorun = false;
    Option.Listcase = CONFIG_TITLE;
    Option.Tab = 2;
    Option.Invert = false;
    Option.ColourCode = true;
    Option.DISPLAY_TYPE = ILI9341_16;
    Option.DISPLAY_ORIENTATION = RLANDSCAPE;
    Option.TOUCH_SWAPXY = 0;
    Option.TOUCH_XSCALE = 0.0944;
    Option.TOUCH_YSCALE = 0.0668;
    Option.TOUCH_XZERO = 355;
    Option.TOUCH_YZERO = 205;
    Option.TOUCH_CS = 51;
    Option.TOUCH_IRQ = 34;
   	Option.LCD_CD = 0;
   	Option.LCD_Reset = 0;
   	Option.LCD_CS = 0;
    Option.DISPLAY_CONSOLE = 0;
    Option.DefaultFont = 0x01;
    Option.DefaultFC = WHITE;
    Option.DefaultBC = BLACK;
    Option.RTC_Calibrate = 0;

    Option.SerialConDisabled = 1;
    Option.KeyboardConfig = NO_KEYBOARD ;
    Option.SSDspeed = 0;
    Option.TOUCH_Click = 0;
    Option.DefaultBrightness = 100;
    Option.SerialPullup = 1;
    Option.MaxCtrls=201;
    Option.Refresh = 0;
	Option.DISPLAY_WIDTH = 0;
	Option.DISPLAY_HEIGHT = 0;

}
//void ClearSavedVars(void) {
 //   FlashWriteInit(SAVED_VARS_FLASH);                              // initialise for writing to the flash
//}
void ClearSavedVars(void) {
	int i;
	char *w;
    w=(char*)SAVED_VAR_RAM_ADDR;
    for(i=0;i<SAVED_VAR_RAM_SIZE;i++)*w++=0xFF;
}

// erase all flash memory and reset the options to their defaults
// used on initial firmware run or when the user shorts pins 9 an 10 together on startup
void ResetAllFlash(void) {
	ResetAllOptions();
	PROG_FLASH_SIZE = 0x20000;
//    PROG_FLASH_SIZE = PROG_FLASH_SIZE;
	SaveOptions();                                     //  and write them to flash
	ClearSavedVars();					           	   // erase saved vars
//    FlashWriteInit(SAVED_VARS_FLASH);           	   // erase saved vars
    FlashWriteInit(PROGRAM_FLASH);                     // erase program memory
    FlashWriteByte(0); FlashWriteByte(0);              // terminate the program in flash
    FlashWriteClose();


}

/**
  * @brief  Gets the sector of a given address
  * @param  Address Address of the FLASH Memory
  * @retval The sector of a given address
  */
uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;
  }
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11) */
  {
    sector = FLASH_SECTOR_11;
  }

  return sector;
}
