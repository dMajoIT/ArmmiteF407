/***********************************************************************************************************************
MMBasic

Serial.h

Include file that contains the globals and defines for serial.c in MMBasic.

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

/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
void fun_sprite(void);
#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE
#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef OTHERDISPLAYS_HEADER
#define OTHERDISPLAYS_HEADER
extern void ConfigDisplayOther(char *p);
extern void InitDisplayOther(int fullinit);
extern void Display_Refresh(void);
extern int low_y, high_y, low_x, high_x;
extern void ReadBufferBuffFast(int x1, int y1, int x2, int y2, char* p);
extern void DrawBufferBuffFast(int x1, int y1, int x2, int y2, char* p);
extern void ReadBufferBuffFast8(int x1, int y1, int x2, int y2, char* p);
extern void DrawBufferBuffFast8(int x1, int y1, int x2, int y2, char* p);
extern unsigned short colcount[256];
extern void DrawBitmapBuff(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap);
extern void DrawBufferBuff(int x1, int y1, int x2, int y2, char* p);
extern void ReadBufferBuff(int x1, int y1, int x2, int y2, char* p);
extern void DrawRectangleBuff(int x1, int y1, int x2, int y2, int c);
extern unsigned char *screenbuff;
extern void ScrollBuff(int lines);
extern void ScrollBuff8(int lines);
enum {
  TRANSFER_WAIT,
  TRANSFER_COMPLETE,
  TRANSFER_ERROR
};

#endif
#endif
