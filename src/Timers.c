/***********************************************************************************************************************
Maximite

timers.c

This module manages various timers (counting variables), the date/time,
counting inputs and generates the sound.  All this is contained within the timer 4 interrupt.

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


#define INCLUDE_FUNCTION_DEFINES

#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "main.h"
// timer variables
volatile unsigned int SoundPlay;
volatile unsigned int SecondsTimer = 0;
volatile unsigned int PauseTimer = 0;
volatile unsigned int IntPauseTimer = 0;
volatile unsigned int InkeyTimer = 0;
volatile unsigned int WDTimer = 0;
volatile unsigned int Timer1=0, Timer2=0, Timer3=0, Timer4=0;		                       //1000Hz decrement timer
volatile int ds18b20Timer = -1;
volatile unsigned long long UpTimeCounter;
volatile long long int mSecTimer = 0;								// this is used to count mSec
volatile int USBtime=0;
volatile int second = 0;											// date/time counters
volatile int minute = 0;
volatile int hour = 0;
volatile int day = 1;
volatile int month = 1;
volatile int milliseconds = 1;
volatile int year = 2000;
volatile int day_of_week=1;
volatile int processtick = 1;
volatile unsigned int GPSTimer = 0;
volatile unsigned int AHRSTimer = 0;
volatile int keytimer=0;
const char DaysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
unsigned char PulsePin[NBR_PULSE_SLOTS];
int PulseCnt[NBR_PULSE_SLOTS];
int PulseActive;
volatile unsigned long long UpTimeCounter;
extern TIM_HandleTypeDef htim2;
extern void audio_checks(void);
extern unsigned int CFuncmSec;
extern void CallCFuncmSec(void);
extern volatile uint64_t Count5High;
extern void disk_timerproc(void);
extern TIM_HandleTypeDef htim10;
extern int LCD_BL_Period;
extern volatile BYTE SDCardStat;
volatile unsigned int SDtimer=1000, checkSD=0;
#include "usbd_cdc_if.h"
extern volatile int ConsoleTxBufHead;
extern volatile int ConsoleTxBufTail;
extern char ConsoleTxBuf[CONSOLE_TX_BUF_SIZE];
extern USBD_HandleTypeDef hUsbDeviceFS;

/***************************************************************************************************
InitTimers
Initialise the 1 mSec timer used for internal timekeeping.
****************************************************************************************************/

volatile uint32_t msTicks; /* Counts 1ms timeticks */
void Timer1msHandler(void) {                            /* ----- SysTick_Handler - */
if(processtick){
    static int IrTimeout, IrTick, NextIrTick;
    int ElapsedMicroSec, IrDevTmp, IrCmdTmp;
//    static unsigned int mSecCheck;
//    static unsigned int BacklightCount;
	/////////////////////////////// count up timers /////////////////////////////////////

	// if we are measuring period increment the count
	if(ExtCurrentConfig[INT1PIN] == EXT_PER_IN) INT1Count++;
	if(ExtCurrentConfig[INT2PIN] == EXT_PER_IN) INT2Count++;
	if(ExtCurrentConfig[INT3PIN] == EXT_PER_IN) INT3Count++;
	if(ExtCurrentConfig[INT4PIN] == EXT_PER_IN) INT4Count++;
	mSecTimer++;													// used by the TIMER function
	PauseTimer++;													// used by the PAUSE command
	IntPauseTimer++;												// used by the PAUSE command inside an interrupt
	InkeyTimer++;													// used to delay on an escape character
    GPSTimer++;
    AHRSTimer++;
    if(CFuncmSec) CallCFuncmSec();                                  // the 1mS tick for CFunctions (see CFunction.c)
    keytimer++;
    SDtimer--;
    if(SDtimer==0){
    	SDtimer=1000;
    	if(!(SDCardStat & STA_NOINIT))checkSD=1; //card supposed to be mounted so set a check
    }
    if((Timer4 % 16) == 0){ //process USB console output every 16 msec
    	audio_checks();
    	if(Option.SerialConDisabled){
    		if(ConsoleTxBufHead!=ConsoleTxBufTail){
    			if(ConsoleTxBufHead>ConsoleTxBufTail){
    				if(CDC_Transmit_FS((uint8_t *)&ConsoleTxBuf[ConsoleTxBufTail],ConsoleTxBufHead-ConsoleTxBufTail ) != USBD_BUSY)ConsoleTxBufTail=ConsoleTxBufHead;
    			} else {
    				if(CDC_Transmit_FS((uint8_t *)&ConsoleTxBuf[ConsoleTxBufTail],CONSOLE_TX_BUF_SIZE-ConsoleTxBufTail ) != USBD_BUSY){
    					ConsoleTxBufTail=0;
    				}
    			}
    		}
    	}
    }
//    if(LCD_BL_Period){
//    	__HAL_TIM_SET_COUNTER(&htim10, 0);
//    	htim10.Instance->ARR=100-LCD_BL_Period;
//    	HAL_TIM_Base_Start_IT(&htim10);
//    	LL_GPIO_ResetOutputPin(PWM_1D_LCD_BL_GPIO_Port, PWM_1D_LCD_BL_Pin);
//    }
    Timer2--;
	Timer1--;
    Timer3++;
    Timer4++;
    if(InterruptUsed) {
    	int i;
	    for(i = 0; i < NBRSETTICKS; i++) TickTimer[i]++;			// used in the interrupt tick
	}

	if(WDTimer) {
    	if(--WDTimer == 0) {
            _excep_code = WATCHDOG_TIMEOUT;
            SoftReset();                                            // crude way of implementing a watchdog timer.
        }
    }


    ds18b20Timer++;
    // check if any pulse commands are running
    if(PulseActive) {
        int i;
        for(PulseActive = i = 0; i < NBR_PULSE_SLOTS; i++) {
            if(PulseCnt[i] > 0) {                                   // if the pulse timer is running
                PulseCnt[i]--;                                      // and decrement our count
                if(PulseCnt[i] == 0)                                // if this is the last count reset the pulse
                    PinSetBit(PulsePin[i], LATINV);
                else
                    PulseActive = true;                             // there is at least one pulse still active
            }
        }
    }

    // Handle any IR remote control activity
    ElapsedMicroSec = readusclock();
    if(IrState > IR_WAIT_START && ElapsedMicroSec > 15000) IrReset();
    IrCmdTmp = -1;
    
    // check for any Sony IR receive activity
    if(IrState == SONY_WAIT_BIT_START && ElapsedMicroSec > 2800 && (IrCount == 12 || IrCount == 15 || IrCount == 20)) {
        IrDevTmp = ((IrBits >> 7) & 0b11111);
        IrCmdTmp = (IrBits & 0b1111111) | ((IrBits >> 5) & ~0b1111111);
    }
    
    // check for any NEC IR receive activity
    if(IrState == NEC_WAIT_BIT_END && IrCount == 32) {
        // check if it is a NON extended address and adjust if it is
        if((IrBits >> 24) == ~((IrBits >> 16) & 0xff)) IrBits = (IrBits & 0x0000ffff) | ((IrBits >> 8) & 0x00ff0000);
        IrDevTmp = ((IrBits >> 16) & 0xffff);
        IrCmdTmp = ((IrBits >> 8) & 0xff);
    }

    // now process the IR message, this includes handling auto repeat while the key is held down
    // IrTick counts how many mS since the key was first pressed
    // NextIrTick is used to time the auto repeat
    // IrTimeout is used to detect when the key is released
    // IrGotMsg is a signal to the interrupt handler that an interrupt is required
    if(IrCmdTmp != -1) {
        if(IrTick > IrTimeout) {
            // this is a new keypress
            IrTick = 0;
            NextIrTick = 650;
        }
        if(IrTick == 0 || IrTick > NextIrTick) {
            if(IrVarType & 0b01)
                *(MMFLOAT *)IrDev = IrDevTmp;
            else
                *(long long int *)IrDev = IrDevTmp;
            if(IrVarType & 0b10)
                *(MMFLOAT *)IrCmd = IrCmdTmp;
            else
                *(long long int *)IrCmd = IrCmdTmp;
            IrGotMsg = true;
            NextIrTick += 250;
        }
        IrTimeout = IrTick + 150;
        IrReset();
    }
    IrTick++;

    // check on the touch panel, is the pen down?

    TouchTimer++;
    if(CheckGuiFlag) CheckGuiTimeouts();                            // are blinking LEDs in use?  If so count down their timers

    if(Option.TOUCH_CS && TOUCH_GETIRQTRIS){                       // is touch enabled and the PEN IRQ pin an input?
        if(TOUCH_DOWN) {                                            // is the pen down
            if(!TouchState) {                                       // yes, it is.  If we have not reported this before
                TouchState = TouchDown = true;                      // set the flags
                TouchUp = false;
            }
        } else {
            if(TouchState) {                                        // the pen is not down.  If we have not reported this before
                TouchState = TouchDown = false;                     // set the flags
                TouchUp = true;
            }
        }
    }

    if(ClickTimer) {
        ClickTimer--;
        if(Option.TOUCH_Click) PinSetBit(Option.TOUCH_Click, ClickTimer ? LATSET : LATCLR);
    }

	if(++CursorTimer > CURSOR_OFF + CURSOR_ON) CursorTimer = 0;		// used to control cursor blink rate

    // if we are measuring frequency grab the count for the last second
    if(ExtCurrentConfig[INT1PIN] == EXT_FREQ_IN && --INT1Timer <= 0) { INT1Value = INT1Count; INT1Count = 0; INT1Timer = INT1InitTimer; }
    if(ExtCurrentConfig[INT2PIN] == EXT_FREQ_IN && --INT2Timer <= 0) { INT2Value = INT2Count; INT2Count = 0; INT2Timer = INT2InitTimer; }
    if(ExtCurrentConfig[INT3PIN] == EXT_FREQ_IN && --INT3Timer <= 0) { INT3Value = INT3Count; INT3Count = 0; INT3Timer = INT3InitTimer; }
    if(ExtCurrentConfig[INT4PIN] == EXT_FREQ_IN && --INT4Timer <= 0) { INT4Value = INT4Count; INT4Count = 0; INT4Timer = INT4InitTimer; }

}

}
void mT4IntEnable(int status){
	if(status){
		processtick=1;
	} else{
		processtick=0;
	}
}

