/***********************************************************************************************************************
MMBasic

FileIO.h

Supporting header file for FileIO.c which does all the SD Card related file I/O in MMBasic.

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

//** SD CARD INCLUDES ***********************************************************
#include "ff.h"
#include "diskio.h"



/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)

void cmd_save(void);
void cmd_load(void);
void cmd_mkdir(void);
void cmd_rmdir(void);
void cmd_chdir(void);
void cmd_kill(void);
void cmd_seek(void);
void cmd_files(void);
void cmd_name(void);
void fun_cwd(void);
void fun_dir(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

	{ "Save",		T_CMD,				0, cmd_save	    },
	{ "Load",		T_CMD,				0, cmd_load	    },
	{ "Mkdir",		T_CMD,				0, cmd_mkdir	},
	{ "Rmdir",		T_CMD,				0, cmd_rmdir	},
	{ "Chdir",		T_CMD,				0, cmd_chdir	},
	{ "Kill",		T_CMD,				0, cmd_kill	    },
	{ "Seek",		T_CMD,				0, cmd_seek     },
	{ "Files",		T_CMD,				0, cmd_files    },
	{ "Name",		T_CMD,				0, cmd_name     },

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE

	{ "Cwd$",		T_FNA | T_STR,		0, fun_cwd		},
	{ "Dir$(",		T_FUN | T_STR,		0, fun_dir		},

#endif


#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
    extern void GetSdFileDescriptor(int fnbr);
    extern void FileOpen(char *fname, char *fmode, char *ffnbr);
    extern int BasicFileOpen(char *fname, int fnbr, int mode);
    extern void FileClose(int fnbr);
    extern void ForceFileClose(int fnbr);
    extern char FileGetChar(int fnbr);
    extern char FilePutChar(char c, int fnbr);
    extern void FilePutStr(int count, char *c, int fnbr);
    extern int  FileEOF(int fnbr);
    extern char *ChangeToDir(char *p);
    extern int InitSDCard(void);
    extern void ErrorCheck(int fnbr);
    extern void ErrorThrow(int e);
    extern void CheckSDCard(void);
    extern int FileLoadProgram(char *fname);
    extern int FindFreeFileNbr(void);

    extern const int ErrorMap[21];
    extern int SDCardPresent;
    extern volatile int tickspersample;
    extern void checkWAVinput(void);
    extern char *WAVInterrupt;
    extern int WAVcomplete;
    extern int WAV_fnbr;
    extern FRESULT FSerror;

    #define WAV_BUFFER_SIZE 8192
    extern int OptionFileErrorAbort;

#endif
