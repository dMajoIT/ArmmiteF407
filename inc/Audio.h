/***********************************************************************************************************************
MMBasic

Audio.h

Include file that contains the globals and defines for Music.c in the Maximite version of MMBasic.
  
Copyright 2011 - 2014 Geoff Graham.  All Rights Reserved.

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
    void cmd_play(void);
    void CloseAudio(void);
    void StopAudio(void);
    void audioInterrupt(void);
    void CheckAudio(void);
    extern volatile int vol_left, vol_right;
#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE
	{ "Play",        	T_CMD,				0, cmd_play	    },
#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is T_NA, T_FUN, T_FNA or T_OPER augmented by the types T_STR and/or T_NBR
// and P is the precedence (which is only used for operators)

#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef AUDIO_HEADER
#define AUDIO_HEADER
typedef enum { P_NOTHING, P_PAUSE_TONE, P_TONE, P_WAV, P_PAUSE_WAV, P_FLAC, P_MP3, P_MOD, P_PAUSE_MOD, P_PAUSE_FLAC, P_TTS, P_DAC, P_SYNC} e_CurrentlyPlaying;
extern volatile e_CurrentlyPlaying CurrentlyPlaying; 
extern int PWM_FREQ;
extern char *sbuff1, *sbuff2;
extern unsigned char *stress; //numbers from 0 to 8
extern unsigned char *phonemeLength; //tab40160
extern unsigned char *phonemeindex;
extern volatile int playreadcomplete;
extern volatile unsigned int bcount[3];
extern unsigned char *phonemeIndexOutput; //tab47296
extern unsigned char *stressOutput; //tab47365
extern unsigned char *phonemeLengthOutput; //tab47416
extern unsigned char speed;
extern unsigned char pitch;
extern unsigned char mouth;
extern unsigned char throat;
extern unsigned char *pitches; // tab43008
extern unsigned char *frequency1;
extern unsigned char *frequency2;
extern unsigned char *frequency3;
extern void Audio_Interrupt(void);
extern unsigned char *amplitude1;
extern unsigned char *amplitude2;
extern unsigned char *amplitude3;
extern volatile int wav_filesize;                                    // head and tail of the ring buffer for com1

unsigned char *sampledConsonantFlag; // tab44800
#endif
#endif
