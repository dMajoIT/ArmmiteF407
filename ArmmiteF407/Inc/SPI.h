/***********************************************************************************************************************
MMBasic

Audio.h

Include file that contains the globals and defines for PWM and Tone in MMBasic.

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



/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)

void cmd_spi(void);
void fun_spi(void);
void SPIClose(void);
void cmd_spi2(void);
void fun_spi2(void);
void SPI2Close(void);
void cmd_spi3(void);
void fun_spi3(void);
void SPI3Close(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

	{ "SPI",	T_CMD,				0, cmd_spi	},
	{ "SPI2",	T_CMD,					0, cmd_spi2	},

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE

	{ "SPI(",	T_FUN | T_INT,		0, fun_spi,	},
	{ "SPI2(",	T_FUN | T_INT,		0, fun_spi2,	},

#endif
