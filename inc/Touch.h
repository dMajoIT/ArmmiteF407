/***********************************************************************************************************************
MMBasic

Touch.h

Supporting header file for Touch.c which does all the touch screen related I/O in MMBasic.

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

void fun_touch(void);

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

	{ "Touch(",	    T_FUN | T_INT,		0, fun_touch 	},

#endif


#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)

    #define CAL_ERROR_MARGIN        16
    #define TARGET_OFFSET           30
    #define TOUCH_SAMPLES           10
    #define TOUCH_DISCARD           2

    #define GET_X_AXIS              0
    #define GET_Y_AXIS              1
    #define PENIRQ_ON               3

    #define TOUCH_ERROR             -1

    #define CMD_MEASURE_X           0b10010000
    #define CMD_MEASURE_Y           0b11010000
    #define CMD_PENIRQ_ON           0b10010000

    extern void MIPS16 ConfigTouch(char *p);
    extern void MIPS16 InitTouch(void);
    extern void MIPS16 GetCalibration(int x, int y, int *xval, int *yval);

//    extern volatile int TouchX, TouchY;
    extern volatile int TouchState, TouchDown, TouchUp;
    extern int TOUCH_GETIRQTRIS;
    
    #define TOUCH_NOT_CALIBRATED    -9999
    #define TOUCH_ERROR             -1

    // these are defined so that the state of the touch PEN IRQ can be determined with the minimum of CPU cycles
    #define TOUCH_DOWN  (!(PinRead(Option.TOUCH_IRQ)))

    extern int GetTouchValue(int cmd);
    
    extern int (*GetTouchAxis)(int a);
    extern int GetTouchAxis2046(int);  // this needs looking at - the old mx470 code did not take any args
    extern int GetTouch(int cmd);
        
#endif
