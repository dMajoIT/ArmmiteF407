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

void fun_baudrate(void);
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

	{ "Baudrate(",	    T_FUN | T_INT,		0, fun_baudrate 	},

#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef SERIAL_HEADER
#define SERIAL_HEADER

#define	COM_DEFAULT_BAUD_RATE       9600
#define	COM_DEFAULT_BUF_SIZE	    1024
#define TX_BUFFER_SIZE              1024

// global variables
	extern int com10;														// true if COM1 is enabled
	extern int com1_buf_size;													// size of the buffer used to receive chars
	extern int com1_baud;													// determines the baud rate
	extern char *com1_interrupt, *com1_TX_interrupt;												// pointer to the interrupt routine
	extern int com1_TX_complete;
	extern int com1_ilevel;													// number nbr of chars in the buffer for an interrupt
	extern unsigned char *com1Rx_buf;											// pointer to the buffer for received characters
	extern volatile int com1Rx_head, com1Rx_tail;								// head and tail of the ring buffer for com1
	extern unsigned char *com1Tx_buf;											// pointer to the buffer for transmitted characters
	extern volatile int com1Tx_head, com1Tx_tail;								// head and tail of the ring buffer for com1
	extern volatile int com1complete;
	extern uint16_t Rx1Buffer;


	// variables for com2
	extern int com2;														// true if COM2 is enabled
	extern int com2_buf_size;													// size of the buffer used to receive chars
	extern int com2_baud;													// determines the baud rate
	extern char *com2_interrupt, *com2_TX_interrupt;												// pointer to the interrupt routine
	extern int com2_TX_complete;
	extern int com2_ilevel;													// number nbr of chars in the buffer for an interrupt
	extern unsigned char *com2Rx_buf;											// pointer to the buffer for received characters
	volatile int com2Rx_head, com2Rx_tail;								// head and tail of the ring buffer for com2 Rx
	extern unsigned char *com2Tx_buf;											// pointer to the buffer for transmitted characters
	volatile int com2Tx_head, com2Tx_tail;								// head and tail of the ring buffer for com2 Tx
	extern volatile int com2complete;
	extern uint16_t Rx2Buffer;

	#define COM2_9B       0b001                                         // 9 bit data enabled
	#define COM2_DE       0b010                                         // RS485 enable flag in use
	char com2_mode;                                                     // keeps track of the settings for com1
	extern unsigned char com2_bit9;                                        // used to track the 9th bit
	// variables for com3
	extern int com3;														// true if COM3 is enabled
	extern int com3_buf_size;													// size of the buffer used to receive chars
	extern int com3_baud;													// determines the baud rate
	extern char *com3_interrupt, *com3_TX_interrupt;												// pointer to the interrupt routine
	extern int com3_TX_complete;
	extern int com3_ilevel;													// number nbr of chars in the buffer for an interrupt
	extern unsigned char *com3Rx_buf;											// pointer to the buffer for received characters
	extern volatile int com3Rx_head, com3Rx_tail;								// head and tail of the ring buffer for com3 Rx
	extern unsigned char *com3Tx_buf;											// pointer to the buffer for transmitted characters
	extern volatile int com3Tx_head, com3Tx_tail;								// head and tail of the ring buffer for com3 Tx
	extern volatile int com3complete;
	extern uint16_t Rx3Buffer;

	// variables for com4
	extern int com4;														// true if com4 is enabled
	extern int com4_buf_size;													// size of the buffer used to receive chars
	extern int com4_baud;													// determines the baud rate
	extern char *com4_interrupt, *com4_TX_interrupt;												// pointer to the interrupt routine
	extern int com4_TX_complete;
	extern int com4_ilevel;													// number nbr of chars in the buffer for an interrupt
	extern unsigned char *com4Rx_buf;											// pointer to the buffer for received characters
	extern volatile int com4Rx_head, com4Rx_tail;								// head and tail of the ring buffer for com4 Rx
	extern unsigned char *com4Tx_buf;											// pointer to the buffer for transmitted characters
	extern volatile int com4Tx_head, com4Tx_tail;								// head and tail of the ring buffer for com4 Tx
	extern volatile int com4complete;
	extern uint16_t Rx4Buffer;


// global functions
void SerialOpen(char *spec);
void SerialClose(int comnbr);
unsigned char SerialPutchar(int comnbr, unsigned char c);
int SerialRxStatus(int comnbr);
int SerialTxStatus(int comnbr);
int SerialGetchar(int comnbr);
void setupuart(UART_HandleTypeDef  * huartx, USART_TypeDef  * USART_ID, int de, int inv,int s2,int b9,int b7, int baud);
#endif
#endif
