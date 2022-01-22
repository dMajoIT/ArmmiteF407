/***********************************************************************************************************************
MMBasic

SSD1963.c

Driver for the SSD1963 display controller chip
Based on code written by John Leung of TechToys Co. (www.TechToys.com.hk)

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
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
int ScrollStart;


// parameters for the SSD1963 display panel (refer to the glass data sheet)
int SSD1963HorizPulseWidth, SSD1963HorizBackPorch, SSD1963HorizFrontPorch;
int SSD1963VertPulseWidth, SSD1963VertBackPorch, SSD1963VertFrontPorch;
int SSD1963PClock1, SSD1963PClock2, SSD1963PClock3;
int SSD1963Mode1, SSD1963Mode2;
int SSD1963PixelInterface, SSD1963PixelFormat;
void ScrollSSD1963(int lines);
unsigned int RDpin,RDport;
extern void setscroll4P(int t);
void PhysicalDrawRect_16(int x1, int y1, int x2, int y2, int c);
void InitIPS_4_16(void);
//#define dx(...) {char s[140];sprintf(s,  __VA_ARGS__); SerUSBPutS(s); SerUSBPutS("\r\n");}
void ScrollILI9341(int lines);
void InitILI9341(void);
typedef struct
{
  __IO uint16_t REG;
  __IO uint16_t RAM;
}LCD_CONTROLLER_TypeDef;

#define FMC_BANK1_BASE  ((uint32_t)(0x60000000 | 0x00000000))
#define FMC_RBANK1_BASE  ((uint32_t)(0x60000000 | 0x00080000))
#define FMC_BANK1       ((LCD_CONTROLLER_TypeDef *) FMC_BANK1_BASE)
#define FMC_RBANK1       ((LCD_CONTROLLER_TypeDef *) FMC_RBANK1_BASE)
extern SRAM_HandleTypeDef hsram1;
extern int LCD_BL_Period;
extern void DoBlit(int x1, int y1, int x2, int y2, int w, int h);
extern TIM_HandleTypeDef htim1;

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions used by MMBasic to setup the display
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigDisplaySSD(char *p) {
    getargs(&p, 9,",");
//    if((argc & 1) != 1 || argc < 3) error("Argument count");
	if(checkstring(argv[0], "SSD1963_4_16")) {                         // this is the 4" glass
		Option.DISPLAY_TYPE = SSD1963_4_16;
    } else if(checkstring(argv[0], "SSD1963_5_16")) {                  // this is the 5" glass
        Option.DISPLAY_TYPE = SSD1963_5_16;
    } else if(checkstring(argv[0], "SSD1963_5A_16")) {                 // this is the 5" glass alternative version
        Option.DISPLAY_TYPE = SSD1963_5A_16;
    } else if(checkstring(argv[0], "SSD1963_7_16")) {                  // there appears to be two versions of the 7" glass in circulation, this is type 1
        Option.DISPLAY_TYPE = SSD1963_7_16;
    } else if(checkstring(argv[0], "SSD1963_7A_16")) {                 // this is type 2 of the 7" glass (high luminosity version)
        Option.DISPLAY_TYPE = SSD1963_7A_16;
    } else if(checkstring(argv[0], "SSD1963_8_16")) {                  // this is the 8" glass (EastRising)
        Option.DISPLAY_TYPE = SSD1963_8_16;
    } else if(checkstring(argv[0], "ILI9341_16")) {
    	Option.DISPLAY_TYPE = ILI9341_16;
    } else if(checkstring(argv[0], "IPS_4_16")) {
        Option.DISPLAY_TYPE = IPS_4_16;	                      /***G.A***/
    } else
        return;
    if(!(argc == 3 || argc == 5 || argc==7)) error("Argument count");
    
    if(checkstring(argv[2], "L") || checkstring(argv[2], "LANDSCAPE"))
        Option.DISPLAY_ORIENTATION = LANDSCAPE;
    else if(checkstring(argv[2], "P") || checkstring(argv[2], "PORTRAIT"))
        Option.DISPLAY_ORIENTATION = PORTRAIT;
    else if(checkstring(argv[2], "RL") || checkstring(argv[2], "RLANDSCAPE"))
        Option.DISPLAY_ORIENTATION = RLANDSCAPE;
    else if(checkstring(argv[2], "RP") || checkstring(argv[2], "RPORTRAIT"))
        Option.DISPLAY_ORIENTATION = RPORTRAIT;
    else
        error("Orientation");
		

    // disable the SPI LCD and touch
    Option.TOUCH_CS = Option.TOUCH_IRQ = Option.LCD_CD = Option.LCD_Reset = Option.LCD_CS = Option.TOUCH_Click = 0;
    Option.TOUCH_XZERO = TOUCH_NOT_CALIBRATED;                      // record the touch feature as not calibrated
    InitDisplaySSD(1);
}



// initialise the display controller
// this is used in the initial boot sequence of the Micromite
void InitDisplaySSD(int fullinit) {

	if(!((Option.DISPLAY_TYPE >= SSD1963_4 && Option.DISPLAY_TYPE <= SSD_PANEL) || Option.DISPLAY_TYPE==ILI9341_16 || Option.DISPLAY_TYPE==IPS_4_16 ) ) return;
	// ensure Option.SSDspeed is cleared at start.
	Option.SSDspeed=0;
	SaveOptions();

    switch(Option.DISPLAY_TYPE) {
        case SSD1963_4_16:
        case SSD1963_4: DisplayHRes = 480;                                  // this is the 4.3" glass
                        DisplayVRes = 272;
                        SSD1963HorizPulseWidth = 41;
                        SSD1963HorizBackPorch = 2;
                        SSD1963HorizFrontPorch = 2;
                        SSD1963VertPulseWidth = 10;
                        SSD1963VertBackPorch = 2;
                        SSD1963VertFrontPorch = 2;
                        //Set LSHIFT freq, i.e. the DCLK with PLL freq 120MHz set previously
                        //Typical DCLK is 9MHz.  9MHz = 120MHz*(LCDC_FPR+1)/2^20.  LCDC_FPR = 78642 (0x13332)
                        SSD1963PClock1 = 0x01;
                        SSD1963PClock2 = 0x33;
                        SSD1963PClock3 = 0x32;
                        SSD1963Mode1 = 0x20;                                // 24-bit for 4.3" panel, data latch in rising edge for LSHIFT
                        SSD1963Mode2 = 0;                                   // Hsync+Vsync mode
                        break;
        case SSD1963_5_16:
        case SSD1963_5: DisplayHRes = 800;                                  // this is the 5" glass
                        DisplayVRes = 480;
                        SSD1963HorizPulseWidth = 128;
                        SSD1963HorizBackPorch = 88;
                        SSD1963HorizFrontPorch = 40;
                        SSD1963VertPulseWidth = 2;
                        SSD1963VertBackPorch = 25;
                        SSD1963VertFrontPorch = 18;
                        //Set LSHIFT freq, i.e. the DCLK with PLL freq 120MHz set previously
                        //Typical DCLK is 33MHz.  30MHz = 120MHz*(LCDC_FPR+1)/2^20.  LCDC_FPR = 262143 (0x3FFFF)
                        SSD1963PClock1 = 0x03;
                        SSD1963PClock2 = 0xff;
                        SSD1963PClock3 = 0xff;
                        SSD1963Mode1 = 0x24;                                // 24-bit for 5" panel, data latch in falling edge for LSHIFT
                        SSD1963Mode2 = 0;                                   // Hsync+Vsync mode
                        break;
        case SSD1963_5A_16:
        case SSD1963_5A: DisplayHRes = 800;                                 // this is a 5" glass alternative version
                        DisplayVRes = 480;
                        SSD1963HorizPulseWidth = 128;
                        SSD1963HorizBackPorch = 88;
                        SSD1963HorizFrontPorch = 40;
                        SSD1963VertPulseWidth = 2;
                        SSD1963VertBackPorch = 25;
                        SSD1963VertFrontPorch = 18;
                        //Set LSHIFT freq, i.e. the DCLK with PLL freq 120MHz set previously
                        //Typical DCLK is 33MHz.  30MHz = 120MHz*(LCDC_FPR+1)/2^20.  LCDC_FPR = 262143 (0x3FFFF)
                        SSD1963PClock1 = 0x04;
                        SSD1963PClock2 = 0x93;
                        SSD1963PClock3 = 0xe0;
                        SSD1963Mode1 = 0x24;                                // 24-bit for 5" panel, data latch in falling edge for LSHIFT
                        SSD1963Mode2 = 0;                                   // Hsync+Vsync mode
                        break;
        case SSD1963_7_16:
        case SSD1963_7: DisplayHRes = 800;                                  // this is the 7" glass
                        DisplayVRes = 480;
                        SSD1963HorizPulseWidth = 1;
                        SSD1963HorizBackPorch = 210;
                        SSD1963HorizFrontPorch = 45;
                        SSD1963VertPulseWidth = 1;
                        SSD1963VertBackPorch = 34;
                        SSD1963VertFrontPorch = 10;
                        //Set LSHIFT freq, i.e. the DCLK with PLL freq 120MHz set previously
                        //Typical DCLK is 33.3MHz(datasheet), experiment shows 30MHz gives a stable result
                        //30MHz = 120MHz*(LCDC_FPR+1)/2^20.  LCDC_FPR = 262143 (0x3FFFF)
                        //Time per line = (DISP_HOR_RESOLUTION+DISP_HOR_PULSE_WIDTH+DISP_HOR_BACK_PORCH+DISP_HOR_FRONT_PORCH)/30 us = 1056/30 = 35.2us
                        SSD1963PClock1 = 0x03;
                        SSD1963PClock2 = 0xff;
                        SSD1963PClock3 = 0xff;
                        SSD1963Mode1 = 0x10;                                // 18-bit for 7" panel
                        SSD1963Mode2 = 0x80;                                // TTL mode
                        break;
        case SSD1963_7A_16:
        case SSD1963_7A: DisplayHRes = 800;                                 // this is a 7" glass alternative version (high brightness)
                        DisplayVRes = 480;
                        SSD1963HorizPulseWidth = 3;
                        SSD1963HorizBackPorch = 88;
                        SSD1963HorizFrontPorch = 37;
                        SSD1963VertPulseWidth = 3;
                        SSD1963VertBackPorch = 32;
                        SSD1963VertFrontPorch = 10;
                        SSD1963PClock1 = 0x03;
                        SSD1963PClock2 = 0xff;
                        SSD1963PClock3 = 0xff;
                        SSD1963Mode1 = 0x10;                                // 18-bit for 7" panel
                        SSD1963Mode2 = 0x80;                                // TTL mode
                        break;
        case SSD1963_8_16:
        case SSD1963_8: DisplayHRes = 800;                                  // this is the 8" glass (not documented because the 40 pin connector is non standard)
                        DisplayVRes = 480;
                        SSD1963HorizPulseWidth = 1;
                        SSD1963HorizBackPorch = 210;
                        SSD1963HorizFrontPorch = 45;
                        SSD1963VertPulseWidth = 1;
                        SSD1963VertBackPorch = 34;
                        SSD1963VertFrontPorch = 10;
                        //Set LSHIFT freq, i.e. the DCLK with PLL freq 120MHz set previously
                        //Typical DCLK is 33.3MHz(datasheet), experiment shows 30MHz gives a stable result
                        //30MHz = 120MHz*(LCDC_FPR+1)/2^20.  LCDC_FPR = 262143 (0x3FFFF)
                        //Time per line = (DISP_HOR_RESOLUTION+DISP_HOR_PULSE_WIDTH+DISP_HOR_BACK_PORCH+DISP_HOR_FRONT_PORCH)/30 us = 1056/30 = 35.2us
                        SSD1963PClock1 = 0x03;
                        SSD1963PClock2 = 0xff;
                        SSD1963PClock3 = 0xff;
                        SSD1963Mode1 = 0x20;
                        SSD1963Mode2 = 0x00;
                        break;
        case ILI9341_16:
        	DisplayHRes=320;
			DisplayVRes=240;
			break;
        case IPS_4_16:                                              /***G.A***/
            DisplayHRes=800;
       		DisplayVRes=480;
       		break;
    }
    if(Option.DISPLAY_TYPE > SSD_PANEL_8){
        SSD1963PixelInterface=3; //PIXEL data interface - 16-bit RGB565
        SSD1963PixelFormat=0b01010000; //PIXEL data interface RGB565
    } else {
        SSD1963PixelInterface=0; //PIXEL data interface - 8-bit
        SSD1963PixelFormat=0b01110000;	//PIXEL data interface 24-bit
    }
    if(fullinit){
        if(DISPLAY_LANDSCAPE) {
            VRes=DisplayVRes;
            HRes=DisplayHRes;
        } else {
            VRes=DisplayHRes;
            HRes=DisplayVRes;
        }

        // setup the pointers to the drawing primitives
        DrawRectangle = DrawRectangleSSD1963;
        ScrollLCD = ScrollSSD1963;
        DrawBuffer=DrawBufferSSD1963_16;
        DrawBitmap = DrawBitmapSSD1963_16;
        ReadBuffer=ReadBufferSSD1963_16;

        if(Option.DISPLAY_TYPE == ILI9341_16 || Option.DISPLAY_TYPE == IPS_4_16){
                       	 ScrollLCD = ScrollILI9341;
        }


#ifdef NotRequired
        if(Option.DISPLAY_TYPE==SSD1963_4_16){
            DrawBuffer=DrawBufferSSD1963_16;
            DrawBitmap = DrawBitmapSSD1963_16;
            ReadBuffer=ReadBufferSSD1963_16;
        } else if(Option.DISPLAY_TYPE == ILI9341_16){
            DrawBuffer=DrawBufferSSD1963_16;
            DrawBitmap = DrawBitmapSSD1963_16;
            ReadBuffer=ReadBufferSSD1963_16;
            ScrollLCD = ScrollILI9341;
        } else if(Option.DISPLAY_TYPE == IPS_4_16){
            DrawBuffer=DrawBufferSSD1963_16;
            DrawBitmap = DrawBitmapSSD1963_16;
            ReadBuffer=ReadBufferSSD1963_16;
            ScrollLCD = ScrollILI9341;       /***G.A***/
        } else if(Option.DISPLAY_TYPE > SSD_PANEL_8){
            DrawBuffer=DrawBufferSSD1963_16;
            DrawBitmap = DrawBitmapSSD1963_16;
            ReadBuffer=ReadBufferSSD1963_16;
        }
#endif

        if(Option.DISPLAY_CONSOLE)SetFont(Option.DefaultFont); // font 7 scale 1 is 0x61 ie. (7-1)<<4|1
        else SetFont((Option.DISPLAY_TYPE == ILI9341_16 ? 0x61: 1)); // font 7 scale 1 is 0x61 ie. (7-1)<<4|1
        PromptFont = gui_font;
        PromptFC = gui_fcolour = Option.DefaultFC;
        PromptBC = gui_bcolour = Option.DefaultBC;
    }
    if(Option.DISPLAY_TYPE == ILI9341_16) InitILI9341();
    else if(Option.DISPLAY_TYPE == IPS_4_16) InitIPS_4_16();
    else InitSSD1963();
	Option.DefaultBrightness = 100;
	if(Option.DISPLAY_TYPE >= SSD1963_4 && Option.DISPLAY_TYPE<=SSD_PANEL){
		SetBacklightSSD1963(Option.DefaultBrightness);
	}else if(Option.DISPLAY_TYPE==IPS_4_16){
		htim1.Instance->CCR3=1000-Option.DefaultBrightness*10;
	} else {
		htim1.Instance->CCR3=Option.DefaultBrightness*10;
	}
    ResetDisplay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The SSD1963 driver
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

// Write a command byte to the SSD1963
void WriteSSD1963Command(int cmd) {
	  /* Write 16-bit Index, then write register */
	  FMC_BANK1->REG = cmd;
	  __DSB();
}

// Slowly write a command byte to the SSD1963
//void WriteSSD1963CommandSlow(int cmd) {
	  /* Write 16-bit Index, then write register */
//	  FMC_BANK1->REG = cmd;
//	  __DSB();
//}

// Write an 8 bit data word to the SSD1963
void WriteDataSSD1963(int data) {
	  /* Write 16-bit Reg */
	  FMC_RBANK1->RAM = data;
	  __DSB();
}




//Slowly write an 8 bit data word to the SSD1963
//void WriteDataSSD1963Slow(int data) {
	  /* Write 16-bit Reg */
//	  FMC_RBANK1->RAM = data;
//	  __DSB();
//}

// Write sequential 16 bit command with the same 16 bit data word n times to the IPS_4_16
void WriteCmdDataIPS_4_16(int cmd,int n,int data) {
  while (n>0) {
	/* Write 16-bit Index, then write register */
	  FMC_BANK1->REG = cmd;
	  __DSB();

	/* Write 16-bit Reg */
	 FMC_RBANK1->RAM = data;
	  __DSB();

	  cmd++;
	  n--;
  }
}

// Write RGB colour over an 8 bit bus
void WriteColor(unsigned int c) {
	FMC_RBANK1->RAM = (c>>16) & 0xFF;__DSB();
	FMC_RBANK1->RAM = (c>>8) & 0xFF;__DSB();
	FMC_RBANK1->RAM = c & 0xFF;__DSB();
}
// Read RGB colour over an 8 bit bus
unsigned int ReadColor(void) {
	int c;
	c=(FMC_RBANK1->RAM & 0xFF)<<16;
	c|=(FMC_RBANK1->RAM & 0xFF)<<8;
	c|=(FMC_RBANK1->RAM & 0xFF);
	return c;
}
unsigned int ReadData(void) {
	return FMC_RBANK1->RAM;
}

// Read RGB colour over an 8 bit bus, first pixel
//unsigned int ReadColorSlow(void) {
//	int c;
//	c=(FMC_RBANK1->RAM & 0xFF)<<16;
//	c|=(FMC_RBANK1->RAM & 0xFF)<<8;
//	c|=(FMC_RBANK1->RAM & 0xFF);
//	return c;
//}
//// The next two functions are used in the initial setup where the SSD1963 cannot respond to fast signals




/*********************************************************************
* defines start/end coordinates for memory access from host to SSD1963
* also maps the start and end points to suit the orientation
********************************************************************/
void  SetAreaILI9341(int xstart, int ystart, int xend, int yend, int rw) {
    if(HRes == 0) error("Display not configured");
    WriteSSD1963Command(ILI9341_COLADDRSET);
    WriteDataSSD1963(xstart >> 8);
    WriteDataSSD1963(xstart);
    WriteDataSSD1963(xend >> 8);
    WriteDataSSD1963(xend);
    WriteSSD1963Command(ILI9341_PAGEADDRSET);
    WriteDataSSD1963(ystart >> 8);
    WriteDataSSD1963(ystart);
    WriteDataSSD1963(yend >> 8);
    WriteDataSSD1963(yend);
    if(rw){
    	WriteSSD1963Command(ILI9341_MEMORYWRITE);
    } else {
    	WriteSSD1963Command(ILI9341_RAMRD);
    }
}
void  SetAreaIPS_4_16(int xstart, int ystart, int xend, int yend, int rw) {
    if(HRes == 0) error("Display not configured");
    WriteCmdDataIPS_4_16(0x2A00,1,xstart>>8);
    WriteCmdDataIPS_4_16(0x2A01,1,xstart & 0xFF);
    WriteCmdDataIPS_4_16(0x2A02,1,xend>>8);
    WriteCmdDataIPS_4_16(0x2A03,1,xend & 0xFF);
    WriteCmdDataIPS_4_16(0x2B00,1,ystart>>8);
    WriteCmdDataIPS_4_16(0x2B01,1,ystart & 0xFF);
    WriteCmdDataIPS_4_16(0x2B02,1,yend>>8);
    WriteCmdDataIPS_4_16(0x2B03,1,yend & 0xFF);

    if(rw){
    	WriteSSD1963Command(0x2C00);  //write to memory
    } else {
    	WriteSSD1963Command(0x2E00);  //read from memory
    }
}
void SetAreaSSD1963(int x1, int y1, int x2, int y2) {
    int start_x, start_y, end_x, end_y;

    switch(Option.DISPLAY_ORIENTATION) {
        case LANDSCAPE:
        case RLANDSCAPE: start_x = x1;
                         end_x = x2;
                         start_y = y1;
                         end_y = y2;
                         break;
        case PORTRAIT:
        case RPORTRAIT:  start_x = y1;
                         end_x = y2;
                         start_y = (DisplayVRes - 1) - x2;
                         end_y = (DisplayVRes - 1) - x1;
                         break;
        default: return;
    }

	WriteSSD1963Command(CMD_SET_COLUMN);
	WriteDataSSD1963(start_x>>8);
	WriteDataSSD1963(start_x);
	WriteDataSSD1963(end_x>>8);
	WriteDataSSD1963(end_x);
	WriteSSD1963Command(CMD_SET_PAGE);
	WriteDataSSD1963(start_y>>8);
	WriteDataSSD1963(start_y);
	WriteDataSSD1963(end_y>>8);
	WriteDataSSD1963(end_y);
}


/*********************************************************************
* Set a GPIO pin to state high(1) or low(0)
*
* PreCondition: Set the GPIO pin an output prior using this function
*
* Arguments: BYTE pin	- 	LCD_RESET
*							LCD_SPENA
*							LCD_SPCLK
*							LCD_SPDAT
*							
*			 BOOL state - 	0 for low
*							1 for high
*********************************************************************/
static void GPIO_WR(int pin, int state) {
	int _gpioStatus = 0;

	if(state==1)
		_gpioStatus = _gpioStatus|pin;
	else
		_gpioStatus = _gpioStatus&(~pin);

	WriteSSD1963Command(CMD_SET_GPIO_VAL);                                 // Set GPIO value
	WriteDataSSD1963(_gpioStatus);
}


/*********************************************************************
* SetBacklight(BYTE intensity)
* Some boards may use of PWM feature of ssd1963 to adjust the backlight 
* intensity and this function supports that.  However, most boards have
* a separate PWM input pin and that is also supported by using the variable
*  display_backlight intimer.c
*
* Input: 	intensity = 0 (off) to 100 (full on)
*
* Note: The base frequency of PWM set to around 300Hz with PLL set to 120MHz.
*		This parameter is hardware dependent
********************************************************************/
void SetBacklightSSD1963(int intensity) {
	WriteSSD1963Command(CMD_SET_PWM_CONF);                                 // Set PWM configuration for backlight control

	WriteDataSSD1963(0x0E);                                                // PWMF[7:0] = 2, PWM base freq = PLL/(256*(1+5))/256 = 300Hz for a PLL freq = 120MHz
	WriteDataSSD1963((intensity * 255)/100);                               // Set duty cycle, from 0x00 (total pull-down) to 0xFF (99% pull-up , 255/256)
	WriteDataSSD1963(0x01);                                                // PWM enabled and controlled by host (mcu)
	WriteDataSSD1963(0x00);
	WriteDataSSD1963(0x00);
	WriteDataSSD1963(0x00);

    display_backlight = intensity/5;                                // this is used in timer.c
}

/*********************************************************************
* SetTearingCfg(BOOL state, BOOL mode)
* This function enable/disable tearing effect
*
* Input: 	BOOL state -	1 to enable
*							0 to disable
*			BOOL mode -		0:  the tearing effect output line consists
*								of V-blanking information only
*							1:	the tearing effect output line consists
*								of both V-blanking and H-blanking info.
********************************************************************/
void SetTearingCfg(int state, int mode)
{
	if(state == 1) {
		WriteSSD1963Command(CMD_SET_TEAR_ON);
		WriteDataSSD1963(mode&0x01);
	} else {
		WriteSSD1963Command(0x34);
	}

}


/***********************************************************************************************************************************
* Function:  void InitVideo()
* Resets LCD, Initialize IO ports, initialize SSD1963 for PCLK,	HSYNC, VSYNC etc
***********************************************************************************************************************************/
	void InitILI9341(void){
		DisplayHRes = 320;
		DisplayVRes = 240;
		WriteSSD1963Command(0xCB);
		WriteDataSSD1963(0x39);
		WriteDataSSD1963(0x2C);
		WriteDataSSD1963(0x00);
		WriteDataSSD1963(0x34);
		WriteDataSSD1963(0x02);

		WriteSSD1963Command(0xCF);
		WriteDataSSD1963(0x00);
		WriteDataSSD1963(0XC1);
		WriteDataSSD1963(0X30);

		WriteSSD1963Command(0xE8);
		WriteDataSSD1963(0x85);
		WriteDataSSD1963(0x00);
		WriteDataSSD1963(0x78);

		WriteSSD1963Command(0xEA);
		WriteDataSSD1963(0x00);
		WriteDataSSD1963(0x00);

		WriteSSD1963Command(0xED);
		WriteDataSSD1963(0x64);
		WriteDataSSD1963(0x03);
		WriteDataSSD1963(0X12);
		WriteDataSSD1963(0X81);

		WriteSSD1963Command(0xF7);
		WriteDataSSD1963(0x20);

		WriteSSD1963Command(0xC0);    //Power control
		WriteDataSSD1963(0x23);   //VRH[5:0]

		WriteSSD1963Command(0xC1);    //Power control
		WriteDataSSD1963(0x10);   //SAP[2:0];BT[3:0]

		WriteSSD1963Command(0xC5);    //VCM control
		WriteDataSSD1963(0x3e);   //Contrast
		WriteDataSSD1963(0x28);

		WriteSSD1963Command(0xC7);    //VCM control2
		WriteDataSSD1963(0x86);   //--

		WriteSSD1963Command(0x3A);
		WriteDataSSD1963(0x66);  // 55= RGB565 66=RGB666

		WriteSSD1963Command(0xB1);
		WriteDataSSD1963(0x00);
		WriteDataSSD1963(0x18);

		WriteSSD1963Command(0xB6);    // Display Function Control
		WriteDataSSD1963(0x08);
		WriteDataSSD1963(0x82);
		WriteDataSSD1963(0x27);

		WriteSSD1963Command(0xF2);    // 3Gamma Function Disable
		WriteDataSSD1963(0x00);

		WriteSSD1963Command(0x26);    //Gamma curve selected
		WriteDataSSD1963(0x01);

		WriteSSD1963Command(0xE0);    //Set Gamma
		WriteDataSSD1963(0x0F);
		WriteDataSSD1963(0x31);
		WriteDataSSD1963(0x2B);
		WriteDataSSD1963(0x0C);
		WriteDataSSD1963(0x0E);
		WriteDataSSD1963(0x08);
		WriteDataSSD1963(0x4E);
		WriteDataSSD1963(0xF1);
		WriteDataSSD1963(0x37);
		WriteDataSSD1963(0x07);
		WriteDataSSD1963(0x10);
		WriteDataSSD1963(0x03);
		WriteDataSSD1963(0x0E);
		WriteDataSSD1963(0x09);
		WriteDataSSD1963(0x00);

		WriteSSD1963Command(0XE1);    //Set Gamma
		WriteDataSSD1963(0x00);
		WriteDataSSD1963(0x0E);
		WriteDataSSD1963(0x14);
		WriteDataSSD1963(0x03);
		WriteDataSSD1963(0x11);
		WriteDataSSD1963(0x07);
		WriteDataSSD1963(0x31);
		WriteDataSSD1963(0xC1);
		WriteDataSSD1963(0x48);
		WriteDataSSD1963(0x08);
		WriteDataSSD1963(0x0F);
		WriteDataSSD1963(0x0C);
		WriteDataSSD1963(0x31);
		WriteDataSSD1963(0x36);
		WriteDataSSD1963(0x0F);

		uSec(120000);
		int i=0;
		switch(Option.DISPLAY_ORIENTATION) {
        	case LANDSCAPE:     i=ILI9341_Landscape; break;
        	case PORTRAIT:      i=ILI9341_Portrait; break;
        	case RLANDSCAPE:    i=ILI9341_Landscape180; break;
        	case RPORTRAIT:     i=ILI9341_Portrait180; break;
		}
		WriteSSD1963Command(0x36);    // Memory Access Control
		WriteDataSSD1963(i);
		WriteSSD1963Command(0x11);    //Exit Sleep
		uSec(120000);
		WriteSSD1963Command(0x29); //display on
		WriteSSD1963Command(0x2c); //display on
		ClearScreen(Option.DefaultBC);
}
// ----------- OTM8009A ad NT35510 Initialisation  -- Always uses 16bit commands
	void InitIPS_4_16(void){
        int t=0;
		GPIO_WR(LCD_RESET,1);
		uSec(50000);
		GPIO_WR(LCD_RESET,0);
		uSec(50000);
		GPIO_WR(LCD_RESET,1);
		uSec(50000);
		//read the id to see if OTM8009A or NT35510
		WriteSSD1963Command(0xDA00);
		t=FMC_RBANK1->RAM ; // dummy read
		t=FMC_RBANK1->RAM ; // read id
#ifdef debug
		MMPrintString("ID1=");PIntH(t);MMPrintString("\r\n");


		      // NT35510 IPS Display
	            Option.SSDspeed = 1;
	            SaveOptions();
				WriteCmdDataIPS_4_16(0xF000,1,0x55);
				WriteCmdDataIPS_4_16(0xF001,1,0xAA);
				WriteCmdDataIPS_4_16(0xF002,1,0x52);
				WriteCmdDataIPS_4_16(0xF003,1,0x08);
				WriteCmdDataIPS_4_16(0xF004,1,0x01);
				//#AVDD:manual,1,
				WriteCmdDataIPS_4_16(0xB600,3,0x34);
				//WriteCmdDataIPS_4_16(0xB601,1,0x34);
				//WriteCmdDataIPS_4_16(0xB602,1,0x34);
				WriteCmdDataIPS_4_16(0xB000,3,0x0D);//09
				//WriteCmdDataIPS_4_16(0xB001,1,0x0D);
				//WriteCmdDataIPS_4_16(0xB002,1,0x0D);
				//#AVEE:manual,1,-6V
				WriteCmdDataIPS_4_16(0xB700,3,0x24);
				//WriteCmdDataIPS_4_16(0xB701,1,0x24);
				//WriteCmdDataIPS_4_16(0xB702,1,0x24);
				WriteCmdDataIPS_4_16(0xB100,3,0x0D);
				//WriteCmdDataIPS_4_16(0xB101,1,0x0D);
				//WriteCmdDataIPS_4_16(0xB102,1,0x0D);
				//#PowerControlfor
				//VCL
				WriteCmdDataIPS_4_16(0xB800,3,0x24);
				//WriteCmdDataIPS_4_16(0xB801,1,0x24);
				//WriteCmdDataIPS_4_16(0xB802,1,0x24);
				WriteCmdDataIPS_4_16(0xB200,1,0x00);
				//#VGH:ClampEnable,1,
				WriteCmdDataIPS_4_16(0xB900,3,0x24);
				//WriteCmdDataIPS_4_16(0xB901,1,0x24);
				//WriteCmdDataIPS_4_16(0xB902,1,0x24);
				WriteCmdDataIPS_4_16(0xB300,3,0x05);
				//WriteCmdDataIPS_4_16(0xB301,1,0x05);
				//WriteCmdDataIPS_4_16(0xB302,1,0x05);
				WriteCmdDataIPS_4_16(0xBF00,1,0x01);
				//#VGL(LVGL):
				WriteCmdDataIPS_4_16(0xBA00,3,0x34);
				//WriteCmdDataIPS_4_16(0xBA01,1,0x34);
				//WriteCmdDataIPS_4_16(0xBA02,1,0x34);
				//#VGL_REG(VGLO)
				WriteCmdDataIPS_4_16(0xB500,3,0x0B);
				//WriteCmdDataIPS_4_16(0xB501,1,0x0B);
				//WriteCmdDataIPS_4_16(0xB502,1,0x0B);
				//#VGMP/VGSP:
				WriteCmdDataIPS_4_16(0xBC00,1,0x00);
				WriteCmdDataIPS_4_16(0xBC01,1,0xA3);
				WriteCmdDataIPS_4_16(0xBC02,1,0x00);
				//#VGMN/VGSN
				WriteCmdDataIPS_4_16(0xBD00,1,0x00);
				WriteCmdDataIPS_4_16(0xBD01,1,0xA3);
				WriteCmdDataIPS_4_16(0xBD02,1,0x00);
				//#VCOM=-0.1
				WriteCmdDataIPS_4_16(0xBE00,1,0x00);
				WriteCmdDataIPS_4_16(0xBE01,1,0x63);//4f
					//VCOMH+0x01;
				//#R+
				WriteCmdDataIPS_4_16(0xD100,1,0x00);
				WriteCmdDataIPS_4_16(0xD101,1,0x37);
				WriteCmdDataIPS_4_16(0xD102,1,0x00);
				WriteCmdDataIPS_4_16(0xD103,1,0x52);
				WriteCmdDataIPS_4_16(0xD104,1,0x00);
				WriteCmdDataIPS_4_16(0xD105,1,0x7B);
				WriteCmdDataIPS_4_16(0xD106,1,0x00);
				WriteCmdDataIPS_4_16(0xD107,1,0x99);
				WriteCmdDataIPS_4_16(0xD108,1,0x00);
				WriteCmdDataIPS_4_16(0xD109,1,0xB1);
				WriteCmdDataIPS_4_16(0xD10A,1,0x00);
				WriteCmdDataIPS_4_16(0xD10B,1,0xD2);
				WriteCmdDataIPS_4_16(0xD10C,1,0x00);
				WriteCmdDataIPS_4_16(0xD10D,1,0xF6);
				WriteCmdDataIPS_4_16(0xD10E,1,0x01);
				WriteCmdDataIPS_4_16(0xD10F,1,0x27);
				WriteCmdDataIPS_4_16(0xD110,1,0x01);
				WriteCmdDataIPS_4_16(0xD111,1,0x4E);
				WriteCmdDataIPS_4_16(0xD112,1,0x01);
				WriteCmdDataIPS_4_16(0xD113,1,0x8C);
				WriteCmdDataIPS_4_16(0xD114,1,0x01);
				WriteCmdDataIPS_4_16(0xD115,1,0xBE);
				WriteCmdDataIPS_4_16(0xD116,1,0x02);
				WriteCmdDataIPS_4_16(0xD117,1,0x0B);
				WriteCmdDataIPS_4_16(0xD118,1,0x02);
				WriteCmdDataIPS_4_16(0xD119,1,0x48);
				WriteCmdDataIPS_4_16(0xD11A,1,0x02);
				WriteCmdDataIPS_4_16(0xD11B,1,0x4A);
				WriteCmdDataIPS_4_16(0xD11C,1,0x02);
				WriteCmdDataIPS_4_16(0xD11D,1,0x7E);
				WriteCmdDataIPS_4_16(0xD11E,1,0x02);
				WriteCmdDataIPS_4_16(0xD11F,1,0xBC);
				WriteCmdDataIPS_4_16(0xD120,1,0x02);
				WriteCmdDataIPS_4_16(0xD121,1,0xE1);
				WriteCmdDataIPS_4_16(0xD122,1,0x03);
				WriteCmdDataIPS_4_16(0xD123,1,0x10);
				WriteCmdDataIPS_4_16(0xD124,1,0x03);
				WriteCmdDataIPS_4_16(0xD125,1,0x31);
				WriteCmdDataIPS_4_16(0xD126,1,0x03);
				WriteCmdDataIPS_4_16(0xD127,1,0x5A);
				WriteCmdDataIPS_4_16(0xD128,1,0x03);
				WriteCmdDataIPS_4_16(0xD129,1,0x73);
				WriteCmdDataIPS_4_16(0xD12A,1,0x03);
				WriteCmdDataIPS_4_16(0xD12B,1,0x94);
				WriteCmdDataIPS_4_16(0xD12C,1,0x03);
				WriteCmdDataIPS_4_16(0xD12D,1,0x9F);
				WriteCmdDataIPS_4_16(0xD12E,1,0x03);
				WriteCmdDataIPS_4_16(0xD12F,1,0xB3);
				WriteCmdDataIPS_4_16(0xD130,1,0x03);
				WriteCmdDataIPS_4_16(0xD131,1,0xB9);
				WriteCmdDataIPS_4_16(0xD132,1,0x03);
				WriteCmdDataIPS_4_16(0xD133,1,0xC1);
				//#G+
				WriteCmdDataIPS_4_16(0xD200,1,0x00);
				WriteCmdDataIPS_4_16(0xD201,1,0x37);
				WriteCmdDataIPS_4_16(0xD202,1,0x00);
				WriteCmdDataIPS_4_16(0xD203,1,0x52);
				WriteCmdDataIPS_4_16(0xD204,1,0x00);
				WriteCmdDataIPS_4_16(0xD205,1,0x7B);
				WriteCmdDataIPS_4_16(0xD206,1,0x00);
				WriteCmdDataIPS_4_16(0xD207,1,0x99);
				WriteCmdDataIPS_4_16(0xD208,1,0x00);
				WriteCmdDataIPS_4_16(0xD209,1,0xB1);
				WriteCmdDataIPS_4_16(0xD20A,1,0x00);
				WriteCmdDataIPS_4_16(0xD20B,1,0xD2);
				WriteCmdDataIPS_4_16(0xD20C,1,0x00);
				WriteCmdDataIPS_4_16(0xD20D,1,0xF6);
				WriteCmdDataIPS_4_16(0xD20E,1,0x01);
				WriteCmdDataIPS_4_16(0xD20F,1,0x27);
				WriteCmdDataIPS_4_16(0xD210,1,0x01);
				WriteCmdDataIPS_4_16(0xD211,1,0x4E);
				WriteCmdDataIPS_4_16(0xD212,1,0x01);
				WriteCmdDataIPS_4_16(0xD213,1,0x8C);
				WriteCmdDataIPS_4_16(0xD214,1,0x01);
				WriteCmdDataIPS_4_16(0xD215,1,0xBE);
				WriteCmdDataIPS_4_16(0xD216,1,0x02);
				WriteCmdDataIPS_4_16(0xD217,1,0x0B);
				WriteCmdDataIPS_4_16(0xD218,1,0x02);
				WriteCmdDataIPS_4_16(0xD219,1,0x48);
				WriteCmdDataIPS_4_16(0xD21A,1,0x02);
				WriteCmdDataIPS_4_16(0xD21B,1,0x4A);
				WriteCmdDataIPS_4_16(0xD21C,1,0x02);
				WriteCmdDataIPS_4_16(0xD21D,1,0x7E);
				WriteCmdDataIPS_4_16(0xD21E,1,0x02);
				WriteCmdDataIPS_4_16(0xD21F,1,0xBC);
				WriteCmdDataIPS_4_16(0xD220,1,0x02);
				WriteCmdDataIPS_4_16(0xD221,1,0xE1);
				WriteCmdDataIPS_4_16(0xD222,1,0x03);
				WriteCmdDataIPS_4_16(0xD223,1,0x10);
				WriteCmdDataIPS_4_16(0xD224,1,0x03);
				WriteCmdDataIPS_4_16(0xD225,1,0x31);
				WriteCmdDataIPS_4_16(0xD226,1,0x03);
				WriteCmdDataIPS_4_16(0xD227,1,0x5A);
				WriteCmdDataIPS_4_16(0xD228,1,0x03);
				WriteCmdDataIPS_4_16(0xD229,1,0x73);
				WriteCmdDataIPS_4_16(0xD22A,1,0x03);
				WriteCmdDataIPS_4_16(0xD22B,1,0x94);
				WriteCmdDataIPS_4_16(0xD22C,1,0x03);
				WriteCmdDataIPS_4_16(0xD22D,1,0x9F);
				WriteCmdDataIPS_4_16(0xD22E,1,0x03);
				WriteCmdDataIPS_4_16(0xD22F,1,0xB3);
				WriteCmdDataIPS_4_16(0xD230,1,0x03);
				WriteCmdDataIPS_4_16(0xD231,1,0xB9);
				WriteCmdDataIPS_4_16(0xD232,1,0x03);
				WriteCmdDataIPS_4_16(0xD233,1,0xC1);
				//#B+
				WriteCmdDataIPS_4_16(0xD300,1,0x00);
				WriteCmdDataIPS_4_16(0xD301,1,0x37);
				WriteCmdDataIPS_4_16(0xD302,1,0x00);
				WriteCmdDataIPS_4_16(0xD303,1,0x52);
				WriteCmdDataIPS_4_16(0xD304,1,0x00);
				WriteCmdDataIPS_4_16(0xD305,1,0x7B);
				WriteCmdDataIPS_4_16(0xD306,1,0x00);
				WriteCmdDataIPS_4_16(0xD307,1,0x99);
				WriteCmdDataIPS_4_16(0xD308,1,0x00);
				WriteCmdDataIPS_4_16(0xD309,1,0xB1);
				WriteCmdDataIPS_4_16(0xD30A,1,0x00);
				WriteCmdDataIPS_4_16(0xD30B,1,0xD2);
				WriteCmdDataIPS_4_16(0xD30C,1,0x00);
				WriteCmdDataIPS_4_16(0xD30D,1,0xF6);
				WriteCmdDataIPS_4_16(0xD30E,1,0x01);
				WriteCmdDataIPS_4_16(0xD30F,1,0x27);
				WriteCmdDataIPS_4_16(0xD310,1,0x01);
				WriteCmdDataIPS_4_16(0xD311,1,0x4E);
				WriteCmdDataIPS_4_16(0xD312,1,0x01);
				WriteCmdDataIPS_4_16(0xD313,1,0x8C);
				WriteCmdDataIPS_4_16(0xD314,1,0x01);
				WriteCmdDataIPS_4_16(0xD315,1,0xBE);
				WriteCmdDataIPS_4_16(0xD316,1,0x02);
				WriteCmdDataIPS_4_16(0xD317,1,0x0B);
				WriteCmdDataIPS_4_16(0xD318,1,0x02);
				WriteCmdDataIPS_4_16(0xD319,1,0x48);
				WriteCmdDataIPS_4_16(0xD31A,1,0x02);
				WriteCmdDataIPS_4_16(0xD31B,1,0x4A);
				WriteCmdDataIPS_4_16(0xD31C,1,0x02);
				WriteCmdDataIPS_4_16(0xD31D,1,0x7E);
				WriteCmdDataIPS_4_16(0xD31E,1,0x02);
				WriteCmdDataIPS_4_16(0xD31F,1,0xBC);
				WriteCmdDataIPS_4_16(0xD320,1,0x02);
				WriteCmdDataIPS_4_16(0xD321,1,0xE1);
				WriteCmdDataIPS_4_16(0xD322,1,0x03);
				WriteCmdDataIPS_4_16(0xD323,1,0x10);
				WriteCmdDataIPS_4_16(0xD324,1,0x03);
				WriteCmdDataIPS_4_16(0xD325,1,0x31);
				WriteCmdDataIPS_4_16(0xD326,1,0x03);
				WriteCmdDataIPS_4_16(0xD327,1,0x5A);
				WriteCmdDataIPS_4_16(0xD328,1,0x03);
				WriteCmdDataIPS_4_16(0xD329,1,0x73);
				WriteCmdDataIPS_4_16(0xD32A,1,0x03);
				WriteCmdDataIPS_4_16(0xD32B,1,0x94);
				WriteCmdDataIPS_4_16(0xD32C,1,0x03);
				WriteCmdDataIPS_4_16(0xD32D,1,0x9F);
				WriteCmdDataIPS_4_16(0xD32E,1,0x03);
				WriteCmdDataIPS_4_16(0xD32F,1,0xB3);
				WriteCmdDataIPS_4_16(0xD330,1,0x03);
				WriteCmdDataIPS_4_16(0xD331,1,0xB9);
				WriteCmdDataIPS_4_16(0xD332,1,0x03);
				WriteCmdDataIPS_4_16(0xD333,1,0xC1);
				//#R-///////////////////////////////////////////
				WriteCmdDataIPS_4_16(0xD400,1,0x00);
				WriteCmdDataIPS_4_16(0xD401,1,0x37);
				WriteCmdDataIPS_4_16(0xD402,1,0x00);
				WriteCmdDataIPS_4_16(0xD403,1,0x52);
				WriteCmdDataIPS_4_16(0xD404,1,0x00);
				WriteCmdDataIPS_4_16(0xD405,1,0x7B);
				WriteCmdDataIPS_4_16(0xD406,1,0x00);
				WriteCmdDataIPS_4_16(0xD407,1,0x99);
				WriteCmdDataIPS_4_16(0xD408,1,0x00);
				WriteCmdDataIPS_4_16(0xD409,1,0xB1);
				WriteCmdDataIPS_4_16(0xD40A,1,0x00);
				WriteCmdDataIPS_4_16(0xD40B,1,0xD2);
				WriteCmdDataIPS_4_16(0xD40C,1,0x00);
				WriteCmdDataIPS_4_16(0xD40D,1,0xF6);
				WriteCmdDataIPS_4_16(0xD40E,1,0x01);
				WriteCmdDataIPS_4_16(0xD40F,1,0x27);
				WriteCmdDataIPS_4_16(0xD410,1,0x01);
				WriteCmdDataIPS_4_16(0xD411,1,0x4E);
				WriteCmdDataIPS_4_16(0xD412,1,0x01);
				WriteCmdDataIPS_4_16(0xD413,1,0x8C);
				WriteCmdDataIPS_4_16(0xD414,1,0x01);
				WriteCmdDataIPS_4_16(0xD415,1,0xBE);
				WriteCmdDataIPS_4_16(0xD416,1,0x02);
				WriteCmdDataIPS_4_16(0xD417,1,0x0B);
				WriteCmdDataIPS_4_16(0xD418,1,0x02);
				WriteCmdDataIPS_4_16(0xD419,1,0x48);
				WriteCmdDataIPS_4_16(0xD41A,1,0x02);
				WriteCmdDataIPS_4_16(0xD41B,1,0x4A);
				WriteCmdDataIPS_4_16(0xD41C,1,0x02);
				WriteCmdDataIPS_4_16(0xD41D,1,0x7E);
				WriteCmdDataIPS_4_16(0xD41E,1,0x02);
				WriteCmdDataIPS_4_16(0xD41F,1,0xBC);
				WriteCmdDataIPS_4_16(0xD420,1,0x02);
				WriteCmdDataIPS_4_16(0xD421,1,0xE1);
				WriteCmdDataIPS_4_16(0xD422,1,0x03);
				WriteCmdDataIPS_4_16(0xD423,1,0x10);
				WriteCmdDataIPS_4_16(0xD424,1,0x03);
				WriteCmdDataIPS_4_16(0xD425,1,0x31);
				WriteCmdDataIPS_4_16(0xD426,1,0x03);
				WriteCmdDataIPS_4_16(0xD427,1,0x5A);
				WriteCmdDataIPS_4_16(0xD428,1,0x03);
				WriteCmdDataIPS_4_16(0xD429,1,0x73);
				WriteCmdDataIPS_4_16(0xD42A,1,0x03);
				WriteCmdDataIPS_4_16(0xD42B,1,0x94);
				WriteCmdDataIPS_4_16(0xD42C,1,0x03);
				WriteCmdDataIPS_4_16(0xD42D,1,0x9F);
				WriteCmdDataIPS_4_16(0xD42E,1,0x03);
				WriteCmdDataIPS_4_16(0xD42F,1,0xB3);
				WriteCmdDataIPS_4_16(0xD430,1,0x03);
				WriteCmdDataIPS_4_16(0xD431,1,0xB9);
				WriteCmdDataIPS_4_16(0xD432,1,0x03);
				WriteCmdDataIPS_4_16(0xD433,1,0xC1);
				//#G-//////////////////////////////////////////////
				WriteCmdDataIPS_4_16(0xD500,1,0x00);
				WriteCmdDataIPS_4_16(0xD501,1,0x37);
				WriteCmdDataIPS_4_16(0xD502,1,0x00);
				WriteCmdDataIPS_4_16(0xD503,1,0x52);
				WriteCmdDataIPS_4_16(0xD504,1,0x00);
				WriteCmdDataIPS_4_16(0xD505,1,0x7B);
				WriteCmdDataIPS_4_16(0xD506,1,0x00);
				WriteCmdDataIPS_4_16(0xD507,1,0x99);
				WriteCmdDataIPS_4_16(0xD508,1,0x00);
				WriteCmdDataIPS_4_16(0xD509,1,0xB1);
				WriteCmdDataIPS_4_16(0xD50A,1,0x00);
				WriteCmdDataIPS_4_16(0xD50B,1,0xD2);
				WriteCmdDataIPS_4_16(0xD50C,1,0x00);
				WriteCmdDataIPS_4_16(0xD50D,1,0xF6);
				WriteCmdDataIPS_4_16(0xD50E,1,0x01);
				WriteCmdDataIPS_4_16(0xD50F,1,0x27);
				WriteCmdDataIPS_4_16(0xD510,1,0x01);
				WriteCmdDataIPS_4_16(0xD511,1,0x4E);
				WriteCmdDataIPS_4_16(0xD512,1,0x01);
				WriteCmdDataIPS_4_16(0xD513,1,0x8C);
				WriteCmdDataIPS_4_16(0xD514,1,0x01);
				WriteCmdDataIPS_4_16(0xD515,1,0xBE);
				WriteCmdDataIPS_4_16(0xD516,1,0x02);
				WriteCmdDataIPS_4_16(0xD517,1,0x0B);
				WriteCmdDataIPS_4_16(0xD518,1,0x02);
				WriteCmdDataIPS_4_16(0xD519,1,0x48);
				WriteCmdDataIPS_4_16(0xD51A,1,0x02);
				WriteCmdDataIPS_4_16(0xD51B,1,0x4A);
				WriteCmdDataIPS_4_16(0xD51C,1,0x02);
				WriteCmdDataIPS_4_16(0xD51D,1,0x7E);
				WriteCmdDataIPS_4_16(0xD51E,1,0x02);
				WriteCmdDataIPS_4_16(0xD51F,1,0xBC);
				WriteCmdDataIPS_4_16(0xD520,1,0x02);
				WriteCmdDataIPS_4_16(0xD521,1,0xE1);
				WriteCmdDataIPS_4_16(0xD522,1,0x03);
				WriteCmdDataIPS_4_16(0xD523,1,0x10);
				WriteCmdDataIPS_4_16(0xD524,1,0x03);
				WriteCmdDataIPS_4_16(0xD525,1,0x31);
				WriteCmdDataIPS_4_16(0xD526,1,0x03);
				WriteCmdDataIPS_4_16(0xD527,1,0x5A);
				WriteCmdDataIPS_4_16(0xD528,1,0x03);
				WriteCmdDataIPS_4_16(0xD529,1,0x73);
				WriteCmdDataIPS_4_16(0xD52A,1,0x03);
				WriteCmdDataIPS_4_16(0xD52B,1,0x94);
				WriteCmdDataIPS_4_16(0xD52C,1,0x03);
				WriteCmdDataIPS_4_16(0xD52D,1,0x9F);
				WriteCmdDataIPS_4_16(0xD52E,1,0x03);
				WriteCmdDataIPS_4_16(0xD52F,1,0xB3);
				WriteCmdDataIPS_4_16(0xD530,1,0x03);
				WriteCmdDataIPS_4_16(0xD531,1,0xB9);
				WriteCmdDataIPS_4_16(0xD532,1,0x03);
				WriteCmdDataIPS_4_16(0xD533,1,0xC1);
				//#B-///////////////////////////////
				WriteCmdDataIPS_4_16(0xD600,1,0x00);
				WriteCmdDataIPS_4_16(0xD601,1,0x37);
				WriteCmdDataIPS_4_16(0xD602,1,0x00);
				WriteCmdDataIPS_4_16(0xD603,1,0x52);
				WriteCmdDataIPS_4_16(0xD604,1,0x00);
				WriteCmdDataIPS_4_16(0xD605,1,0x7B);
				WriteCmdDataIPS_4_16(0xD606,1,0x00);
				WriteCmdDataIPS_4_16(0xD607,1,0x99);
				WriteCmdDataIPS_4_16(0xD608,1,0x00);
				WriteCmdDataIPS_4_16(0xD609,1,0xB1);
				WriteCmdDataIPS_4_16(0xD60A,1,0x00);
				WriteCmdDataIPS_4_16(0xD60B,1,0xD2);
				WriteCmdDataIPS_4_16(0xD60C,1,0x00);
				WriteCmdDataIPS_4_16(0xD60D,1,0xF6);
				WriteCmdDataIPS_4_16(0xD60E,1,0x01);
				WriteCmdDataIPS_4_16(0xD60F,1,0x27);
				WriteCmdDataIPS_4_16(0xD610,1,0x01);
				WriteCmdDataIPS_4_16(0xD611,1,0x4E);
				WriteCmdDataIPS_4_16(0xD612,1,0x01);
				WriteCmdDataIPS_4_16(0xD613,1,0x8C);
				WriteCmdDataIPS_4_16(0xD614,1,0x01);
				WriteCmdDataIPS_4_16(0xD615,1,0xBE);
				WriteCmdDataIPS_4_16(0xD616,1,0x02);
				WriteCmdDataIPS_4_16(0xD617,1,0x0B);
				WriteCmdDataIPS_4_16(0xD618,1,0x02);
				WriteCmdDataIPS_4_16(0xD619,1,0x48);
				WriteCmdDataIPS_4_16(0xD61A,1,0x02);
				WriteCmdDataIPS_4_16(0xD61B,1,0x4A);
				WriteCmdDataIPS_4_16(0xD61C,1,0x02);
				WriteCmdDataIPS_4_16(0xD61D,1,0x7E);
				WriteCmdDataIPS_4_16(0xD61E,1,0x02);
				WriteCmdDataIPS_4_16(0xD61F,1,0xBC);
				WriteCmdDataIPS_4_16(0xD620,1,0x02);
				WriteCmdDataIPS_4_16(0xD621,1,0xE1);
				WriteCmdDataIPS_4_16(0xD622,1,0x03);
				WriteCmdDataIPS_4_16(0xD623,1,0x10);
				WriteCmdDataIPS_4_16(0xD624,1,0x03);
				WriteCmdDataIPS_4_16(0xD625,1,0x31);
				WriteCmdDataIPS_4_16(0xD626,1,0x03);
				WriteCmdDataIPS_4_16(0xD627,1,0x5A);
				WriteCmdDataIPS_4_16(0xD628,1,0x03);
				WriteCmdDataIPS_4_16(0xD629,1,0x73);
				WriteCmdDataIPS_4_16(0xD62A,1,0x03);
				WriteCmdDataIPS_4_16(0xD62B,1,0x94);
				WriteCmdDataIPS_4_16(0xD62C,1,0x03);
				WriteCmdDataIPS_4_16(0xD62D,1,0x9F);
				WriteCmdDataIPS_4_16(0xD62E,1,0x03);
				WriteCmdDataIPS_4_16(0xD62F,1,0xB3);
				WriteCmdDataIPS_4_16(0xD630,1,0x03);
				WriteCmdDataIPS_4_16(0xD631,1,0xB9);
				WriteCmdDataIPS_4_16(0xD632,1,0x03);
				WriteCmdDataIPS_4_16(0xD633,1,0xC1);
				//#EnablePage0
				WriteCmdDataIPS_4_16(0xF000,1,0x55);
				WriteCmdDataIPS_4_16(0xF001,1,0xAA);
				WriteCmdDataIPS_4_16(0xF002,1,0x52);
				WriteCmdDataIPS_4_16(0xF003,1,0x08);
				WriteCmdDataIPS_4_16(0xF004,1,0x00);
				//#RGBI/FSetting
				WriteCmdDataIPS_4_16(0xB000,1,0x08);
				WriteCmdDataIPS_4_16(0xB001,1,0x05);
				WriteCmdDataIPS_4_16(0xB002,1,0x02);
				WriteCmdDataIPS_4_16(0xB003,1,0x05);
				WriteCmdDataIPS_4_16(0xB004,1,0x02);
				//##SDT:
				WriteCmdDataIPS_4_16(0xB600,1,0x08);
				WriteCmdDataIPS_4_16(0xB500,1,0x50);//480x800
				//##GateEQ:
				WriteCmdDataIPS_4_16(0xB700,2,0x00);
				//WriteCmdDataIPS_4_16(0xB701,1,0x00);
				//##SourceEQ:
				WriteCmdDataIPS_4_16(0xB800,1,0x01);
				WriteCmdDataIPS_4_16(0xB801,3,0x05);
				//WriteCmdDataIPS_4_16(0xB802,1,0x05);
				//WriteCmdDataIPS_4_16(0xB803,1,0x05);
				//#Inversion:Columninversion(NVT)
				WriteCmdDataIPS_4_16(0xBC00,3,0x00);
				//WriteCmdDataIPS_4_16(0xBC01,1,0x00);
				//WriteCmdDataIPS_4_16(0xBC02,1,0x00);
				//#BOE'sSetting(default)
				WriteCmdDataIPS_4_16(0xCC00,1,0x03);
				WriteCmdDataIPS_4_16(0xCC01,2,0x00);
				//WriteCmdDataIPS_4_16(0xCC02,1,0x00);
				//#DisplayTiming:
				WriteCmdDataIPS_4_16(0xBD00,1,0x01);
				WriteCmdDataIPS_4_16(0xBD01,1,0x84);
				WriteCmdDataIPS_4_16(0xBD02,1,0x07);
				WriteCmdDataIPS_4_16(0xBD03,1,0x31);
				WriteCmdDataIPS_4_16(0xBD04,1,0x00);
				WriteCmdDataIPS_4_16(0xBA00,1,0x01);
				WriteCmdDataIPS_4_16(0xFF00,1,0xAA);
				WriteCmdDataIPS_4_16(0xFF01,1,0x55);
				WriteCmdDataIPS_4_16(0xFF02,1,0x25);
				WriteCmdDataIPS_4_16(0xFF03,1,0x01);
				WriteCmdDataIPS_4_16(0x3500,1,0x00);
				WriteCmdDataIPS_4_16(0x3A00,1,0x66); // 55=Colour 565 66=Colour 666 77=Colour 888
#endif

#ifdef alt1
		// NT35510 IPS Display ALTERNATE to test peters board
		Option.SSDspeed = 1;
		SaveOptions();
		WriteCmdDataIPS_4_16(0xF000,1,0x55);
		WriteCmdDataIPS_4_16(0xF001,1,0xAA);
		WriteCmdDataIPS_4_16(0xF002,1,0x52);
		WriteCmdDataIPS_4_16(0xF003,1,0x08);
		WriteCmdDataIPS_4_16(0xF004,1,0x01);
		//AVDD Set AVDD 5.2V
		WriteCmdDataIPS_4_16(0xB000,1,0x0D);
		WriteCmdDataIPS_4_16(0xB001,1,0x0D);
		WriteCmdDataIPS_4_16(0xB002,1,0x0D);
		//AVDD ratio
		WriteCmdDataIPS_4_16(0xB600,1,0x34);
		WriteCmdDataIPS_4_16(0xB601,1,0x34);
		WriteCmdDataIPS_4_16(0xB602,1,0x34);
		//AVEE -5.2V
		WriteCmdDataIPS_4_16(0xB100,1,0x0D);
		WriteCmdDataIPS_4_16(0xB101,1,0x0D);
		WriteCmdDataIPS_4_16(0xB102,1,0x0D);
		//AVEE ratio
		WriteCmdDataIPS_4_16(0xB700,1,0x34);
		WriteCmdDataIPS_4_16(0xB701,1,0x34);
		WriteCmdDataIPS_4_16(0xB702,1,0x34);
		//VCL -2.5V
		WriteCmdDataIPS_4_16(0xB200,1,0x00);
		WriteCmdDataIPS_4_16(0xB201,1,0x00);
		WriteCmdDataIPS_4_16(0xB202,1,0x00);
		//VCL ratio
		WriteCmdDataIPS_4_16(0xB800,1,0x24);
		WriteCmdDataIPS_4_16(0xB801,1,0x24);
		WriteCmdDataIPS_4_16(0xB802,1,0x24);
		//VGH 15V (Free pump)
		WriteCmdDataIPS_4_16(0xBF00,1,0x01);
		WriteCmdDataIPS_4_16(0xB300,1,0x0F);
		WriteCmdDataIPS_4_16(0xB301,1,0x0F);
		WriteCmdDataIPS_4_16(0xB302,1,0x0F);
		//VGH ratio
		WriteCmdDataIPS_4_16(0xB900,1,0x34);
		WriteCmdDataIPS_4_16(0xB901,1,0x34);
		WriteCmdDataIPS_4_16(0xB902,1,0x34);
		//VGL_REG -10V
		WriteCmdDataIPS_4_16(0xB500,1,0x08);
		WriteCmdDataIPS_4_16(0xB501,1,0x08);
		WriteCmdDataIPS_4_16(0xB502,1,0x08);
		WriteCmdDataIPS_4_16(0xC200,1,0x03);
		//VGLX ratio
		WriteCmdDataIPS_4_16(0xBA00,1,0x24);
		WriteCmdDataIPS_4_16(0xBA01,1,0x24);
		WriteCmdDataIPS_4_16(0xBA02,1,0x24);
		//VGMP/VGSP 4.5V/0V
		WriteCmdDataIPS_4_16(0xBC00,1,0x00);
		WriteCmdDataIPS_4_16(0xBC01,1,0x78);
		WriteCmdDataIPS_4_16(0xBC02,1,0x00);
		//VGMN/VGSN -4.5V/0V
		WriteCmdDataIPS_4_16(0xBD00,1,0x00);
		WriteCmdDataIPS_4_16(0xBD01,1,0x78);
		WriteCmdDataIPS_4_16(0xBD02,1,0x00);
		//VCOM
		WriteCmdDataIPS_4_16(0xBE00,1,0x00);
		WriteCmdDataIPS_4_16(0xBE01,1,0x64);
		//Gamma Setting
		WriteCmdDataIPS_4_16(0xD100,1,0x00);
		WriteCmdDataIPS_4_16(0xD101,1,0x33);
		WriteCmdDataIPS_4_16(0xD102,1,0x00);
		WriteCmdDataIPS_4_16(0xD103,1,0x34);
		WriteCmdDataIPS_4_16(0xD104,1,0x00);
		WriteCmdDataIPS_4_16(0xD105,1,0x3A);
		WriteCmdDataIPS_4_16(0xD106,1,0x00);
		WriteCmdDataIPS_4_16(0xD107,1,0x4A);
		WriteCmdDataIPS_4_16(0xD108,1,0x00);
		WriteCmdDataIPS_4_16(0xD109,1,0x5C);
		WriteCmdDataIPS_4_16(0xD10A,1,0x00);
		WriteCmdDataIPS_4_16(0xD10B,1,0x81);
		WriteCmdDataIPS_4_16(0xD10C,1,0x00);
		WriteCmdDataIPS_4_16(0xD10D,1,0xA6);
		WriteCmdDataIPS_4_16(0xD10E,1,0x00);
		WriteCmdDataIPS_4_16(0xD10F,1,0xE5);
		WriteCmdDataIPS_4_16(0xD110,1,0x01);
		WriteCmdDataIPS_4_16(0xD111,1,0x13);
		WriteCmdDataIPS_4_16(0xD112,1,0x01);
		WriteCmdDataIPS_4_16(0xD113,1,0x54);
		WriteCmdDataIPS_4_16(0xD114,1,0x01);
		WriteCmdDataIPS_4_16(0xD115,1,0x82);
		WriteCmdDataIPS_4_16(0xD116,1,0x01);
		WriteCmdDataIPS_4_16(0xD117,1,0xCA);
		WriteCmdDataIPS_4_16(0xD118,1,0x02);
		WriteCmdDataIPS_4_16(0xD119,1,0x00);
		WriteCmdDataIPS_4_16(0xD11A,1,0x02);
		WriteCmdDataIPS_4_16(0xD11B,1,0x01);
		WriteCmdDataIPS_4_16(0xD11C,1,0x02);
		WriteCmdDataIPS_4_16(0xD11D,1,0x34);
		WriteCmdDataIPS_4_16(0xD11E,1,0x02);
		WriteCmdDataIPS_4_16(0xD11F,1,0x67);
		WriteCmdDataIPS_4_16(0xD120,1,0x02);
		WriteCmdDataIPS_4_16(0xD121,1,0x84);
		WriteCmdDataIPS_4_16(0xD122,1,0x02);
		WriteCmdDataIPS_4_16(0xD123,1,0xA4);
		WriteCmdDataIPS_4_16(0xD124,1,0x02);
		WriteCmdDataIPS_4_16(0xD125,1,0xB7);
		WriteCmdDataIPS_4_16(0xD126,1,0x02);
		WriteCmdDataIPS_4_16(0xD127,1,0xCF);
		WriteCmdDataIPS_4_16(0xD128,1,0x02);
		WriteCmdDataIPS_4_16(0xD129,1,0xDE);
		WriteCmdDataIPS_4_16(0xD12A,1,0x02);
		WriteCmdDataIPS_4_16(0xD12B,1,0xF2);
		WriteCmdDataIPS_4_16(0xD12C,1,0x02);
		WriteCmdDataIPS_4_16(0xD12D,1,0xFE);
		WriteCmdDataIPS_4_16(0xD12E,1,0x03);
		WriteCmdDataIPS_4_16(0xD12F,1,0x10);
		WriteCmdDataIPS_4_16(0xD130,1,0x03);
		WriteCmdDataIPS_4_16(0xD131,1,0x33);
		WriteCmdDataIPS_4_16(0xD132,1,0x03);
		WriteCmdDataIPS_4_16(0xD133,1,0x6D);
		WriteCmdDataIPS_4_16(0xD200,1,0x00);
		WriteCmdDataIPS_4_16(0xD201,1,0x33);
		WriteCmdDataIPS_4_16(0xD202,1,0x00);
		WriteCmdDataIPS_4_16(0xD203,1,0x34);
		WriteCmdDataIPS_4_16(0xD204,1,0x00);
		WriteCmdDataIPS_4_16(0xD205,1,0x3A);
		WriteCmdDataIPS_4_16(0xD206,1,0x00);
		WriteCmdDataIPS_4_16(0xD207,1,0x4A);
		WriteCmdDataIPS_4_16(0xD208,1,0x00);
		WriteCmdDataIPS_4_16(0xD209,1,0x5C);
		WriteCmdDataIPS_4_16(0xD20A,1,0x00);
		WriteCmdDataIPS_4_16(0xD20B,1,0x81);
		WriteCmdDataIPS_4_16(0xD20C,1,0x00);
		WriteCmdDataIPS_4_16(0xD20D,1,0xA6);
		WriteCmdDataIPS_4_16(0xD20E,1,0x00);
		WriteCmdDataIPS_4_16(0xD20F,1,0xE5);
		WriteCmdDataIPS_4_16(0xD210,1,0x01);
		WriteCmdDataIPS_4_16(0xD211,1,0x13);
		WriteCmdDataIPS_4_16(0xD212,1,0x01);
		WriteCmdDataIPS_4_16(0xD213,1,0x54);
		WriteCmdDataIPS_4_16(0xD214,1,0x01);
		WriteCmdDataIPS_4_16(0xD215,1,0x82);
		WriteCmdDataIPS_4_16(0xD216,1,0x01);
		WriteCmdDataIPS_4_16(0xD217,1,0xCA);
		WriteCmdDataIPS_4_16(0xD218,1,0x02);
		WriteCmdDataIPS_4_16(0xD219,1,0x00);
		WriteCmdDataIPS_4_16(0xD21A,1,0x02);
		WriteCmdDataIPS_4_16(0xD21B,1,0x01);
		WriteCmdDataIPS_4_16(0xD21C,1,0x02);
		WriteCmdDataIPS_4_16(0xD21D,1,0x34);
		WriteCmdDataIPS_4_16(0xD21E,1,0x02);
		WriteCmdDataIPS_4_16(0xD21F,1,0x67);
		WriteCmdDataIPS_4_16(0xD220,1,0x02);
		WriteCmdDataIPS_4_16(0xD221,1,0x84);
		WriteCmdDataIPS_4_16(0xD222,1,0x02);
		WriteCmdDataIPS_4_16(0xD223,1,0xA4);
		WriteCmdDataIPS_4_16(0xD224,1,0x02);
		WriteCmdDataIPS_4_16(0xD225,1,0xB7);
		WriteCmdDataIPS_4_16(0xD226,1,0x02);
		WriteCmdDataIPS_4_16(0xD227,1,0xCF);
		WriteCmdDataIPS_4_16(0xD228,1,0x02);
		WriteCmdDataIPS_4_16(0xD229,1,0xDE);
		WriteCmdDataIPS_4_16(0xD22A,1,0x02);
		WriteCmdDataIPS_4_16(0xD22B,1,0xF2);
		WriteCmdDataIPS_4_16(0xD22C,1,0x02);
		WriteCmdDataIPS_4_16(0xD22D,1,0xFE);
		WriteCmdDataIPS_4_16(0xD22E,1,0x03);
		WriteCmdDataIPS_4_16(0xD22F,1,0x10);
		WriteCmdDataIPS_4_16(0xD230,1,0x03);
		WriteCmdDataIPS_4_16(0xD231,1,0x33);
		WriteCmdDataIPS_4_16(0xD232,1,0x03);
		WriteCmdDataIPS_4_16(0xD233,1,0x6D);
		WriteCmdDataIPS_4_16(0xD300,1,0x00);
		WriteCmdDataIPS_4_16(0xD301,1,0x33);
		WriteCmdDataIPS_4_16(0xD302,1,0x00);
		WriteCmdDataIPS_4_16(0xD303,1,0x34);
		WriteCmdDataIPS_4_16(0xD304,1,0x00);
		WriteCmdDataIPS_4_16(0xD305,1,0x3A);
		WriteCmdDataIPS_4_16(0xD306,1,0x00);
		WriteCmdDataIPS_4_16(0xD307,1,0x4A);
		WriteCmdDataIPS_4_16(0xD308,1,0x00);
		WriteCmdDataIPS_4_16(0xD309,1,0x5C);
		WriteCmdDataIPS_4_16(0xD30A,1,0x00);
		WriteCmdDataIPS_4_16(0xD30B,1,0x81);
		WriteCmdDataIPS_4_16(0xD30C,1,0x00);
		WriteCmdDataIPS_4_16(0xD30D,1,0xA6);
		WriteCmdDataIPS_4_16(0xD30E,1,0x00);
		WriteCmdDataIPS_4_16(0xD30F,1,0xE5);
		WriteCmdDataIPS_4_16(0xD310,1,0x01);
		WriteCmdDataIPS_4_16(0xD311,1,0x13);
		WriteCmdDataIPS_4_16(0xD312,1,0x01);
		WriteCmdDataIPS_4_16(0xD313,1,0x54);
		WriteCmdDataIPS_4_16(0xD314,1,0x01);
		WriteCmdDataIPS_4_16(0xD315,1,0x82);
		WriteCmdDataIPS_4_16(0xD316,1,0x01);
		WriteCmdDataIPS_4_16(0xD317,1,0xCA);
		WriteCmdDataIPS_4_16(0xD318,1,0x02);
		WriteCmdDataIPS_4_16(0xD319,1,0x00);
		WriteCmdDataIPS_4_16(0xD31A,1,0x02);
		WriteCmdDataIPS_4_16(0xD31B,1,0x01);
		WriteCmdDataIPS_4_16(0xD31C,1,0x02);
		WriteCmdDataIPS_4_16(0xD31D,1,0x34);
		WriteCmdDataIPS_4_16(0xD31E,1,0x02);
		WriteCmdDataIPS_4_16(0xD31F,1,0x67);
		WriteCmdDataIPS_4_16(0xD320,1,0x02);
		WriteCmdDataIPS_4_16(0xD321,1,0x84);
		WriteCmdDataIPS_4_16(0xD322,1,0x02);
		WriteCmdDataIPS_4_16(0xD323,1,0xA4);
		WriteCmdDataIPS_4_16(0xD324,1,0x02);
		WriteCmdDataIPS_4_16(0xD325,1,0xB7);
		WriteCmdDataIPS_4_16(0xD326,1,0x02);
		WriteCmdDataIPS_4_16(0xD327,1,0xCF);
		WriteCmdDataIPS_4_16(0xD328,1,0x02);
		WriteCmdDataIPS_4_16(0xD329,1,0xDE);
		WriteCmdDataIPS_4_16(0xD32A,1,0x02);
		WriteCmdDataIPS_4_16(0xD32B,1,0xF2);
		WriteCmdDataIPS_4_16(0xD32C,1,0x02);
		WriteCmdDataIPS_4_16(0xD32D,1,0xFE);
		WriteCmdDataIPS_4_16(0xD32E,1,0x03);
		WriteCmdDataIPS_4_16(0xD32F,1,0x10);
		WriteCmdDataIPS_4_16(0xD330,1,0x03);
		WriteCmdDataIPS_4_16(0xD331,1,0x33);
		WriteCmdDataIPS_4_16(0xD332,1,0x03);
		WriteCmdDataIPS_4_16(0xD333,1,0x6D);
		WriteCmdDataIPS_4_16(0xD400,1,0x00);
		WriteCmdDataIPS_4_16(0xD401,1,0x33);
		WriteCmdDataIPS_4_16(0xD402,1,0x00);
		WriteCmdDataIPS_4_16(0xD403,1,0x34);
		WriteCmdDataIPS_4_16(0xD404,1,0x00);
		WriteCmdDataIPS_4_16(0xD405,1,0x3A);
		WriteCmdDataIPS_4_16(0xD406,1,0x00);
		WriteCmdDataIPS_4_16(0xD407,1,0x4A);
		WriteCmdDataIPS_4_16(0xD408,1,0x00);
		WriteCmdDataIPS_4_16(0xD409,1,0x5C);
		WriteCmdDataIPS_4_16(0xD40A,1,0x00);
		WriteCmdDataIPS_4_16(0xD40B,1,0x81);
		WriteCmdDataIPS_4_16(0xD40C,1,0x00);
		WriteCmdDataIPS_4_16(0xD40D,1,0xA6);
		WriteCmdDataIPS_4_16(0xD40E,1,0x00);
		WriteCmdDataIPS_4_16(0xD40F,1,0xE5);
		WriteCmdDataIPS_4_16(0xD410,1,0x01);
		WriteCmdDataIPS_4_16(0xD411,1,0x13);
		WriteCmdDataIPS_4_16(0xD412,1,0x01);
		WriteCmdDataIPS_4_16(0xD413,1,0x54);
		WriteCmdDataIPS_4_16(0xD414,1,0x01);
		WriteCmdDataIPS_4_16(0xD415,1,0x82);
		WriteCmdDataIPS_4_16(0xD416,1,0x01);
		WriteCmdDataIPS_4_16(0xD417,1,0xCA);
		WriteCmdDataIPS_4_16(0xD418,1,0x02);
		WriteCmdDataIPS_4_16(0xD419,1,0x00);
		WriteCmdDataIPS_4_16(0xD41A,1,0x02);
		WriteCmdDataIPS_4_16(0xD41B,1,0x01);
		WriteCmdDataIPS_4_16(0xD41C,1,0x02);
		WriteCmdDataIPS_4_16(0xD41D,1,0x34);
		WriteCmdDataIPS_4_16(0xD41E,1,0x02);
		WriteCmdDataIPS_4_16(0xD41F,1,0x67);
		WriteCmdDataIPS_4_16(0xD420,1,0x02);
		WriteCmdDataIPS_4_16(0xD421,1,0x84);
		WriteCmdDataIPS_4_16(0xD422,1,0x02);
		WriteCmdDataIPS_4_16(0xD423,1,0xA4);
		WriteCmdDataIPS_4_16(0xD424,1,0x02);
		WriteCmdDataIPS_4_16(0xD425,1,0xB7);
		WriteCmdDataIPS_4_16(0xD426,1,0x02);
		WriteCmdDataIPS_4_16(0xD427,1,0xCF);
		WriteCmdDataIPS_4_16(0xD428,1,0x02);
		WriteCmdDataIPS_4_16(0xD429,1,0xDE);
		WriteCmdDataIPS_4_16(0xD42A,1,0x02);
		WriteCmdDataIPS_4_16(0xD42B,1,0xF2);
		WriteCmdDataIPS_4_16(0xD42C,1,0x02);
		WriteCmdDataIPS_4_16(0xD42D,1,0xFE);
		WriteCmdDataIPS_4_16(0xD42E,1,0x03);
		WriteCmdDataIPS_4_16(0xD42F,1,0x10);
		WriteCmdDataIPS_4_16(0xD430,1,0x03);
		WriteCmdDataIPS_4_16(0xD431,1,0x33);
		WriteCmdDataIPS_4_16(0xD432,1,0x03);
		WriteCmdDataIPS_4_16(0xD433,1,0x6D);
		WriteCmdDataIPS_4_16(0xD500,1,0x00);
		WriteCmdDataIPS_4_16(0xD501,1,0x33);
		WriteCmdDataIPS_4_16(0xD502,1,0x00);
		WriteCmdDataIPS_4_16(0xD503,1,0x34);
		WriteCmdDataIPS_4_16(0xD504,1,0x00);
		WriteCmdDataIPS_4_16(0xD505,1,0x3A);
		WriteCmdDataIPS_4_16(0xD506,1,0x00);
		WriteCmdDataIPS_4_16(0xD507,1,0x4A);
		WriteCmdDataIPS_4_16(0xD508,1,0x00);
		WriteCmdDataIPS_4_16(0xD509,1,0x5C);
		WriteCmdDataIPS_4_16(0xD50A,1,0x00);
		WriteCmdDataIPS_4_16(0xD50B,1,0x81);
		WriteCmdDataIPS_4_16(0xD50C,1,0x00);
		WriteCmdDataIPS_4_16(0xD50D,1,0xA6);
		WriteCmdDataIPS_4_16(0xD50E,1,0x00);
		WriteCmdDataIPS_4_16(0xD50F,1,0xE5);
		WriteCmdDataIPS_4_16(0xD510,1,0x01);
		WriteCmdDataIPS_4_16(0xD511,1,0x13);
		WriteCmdDataIPS_4_16(0xD512,1,0x01);
		WriteCmdDataIPS_4_16(0xD513,1,0x54);
		WriteCmdDataIPS_4_16(0xD514,1,0x01);
		WriteCmdDataIPS_4_16(0xD515,1,0x82);
		WriteCmdDataIPS_4_16(0xD516,1,0x01);
		WriteCmdDataIPS_4_16(0xD517,1,0xCA);
		WriteCmdDataIPS_4_16(0xD518,1,0x02);
		WriteCmdDataIPS_4_16(0xD519,1,0x00);
		WriteCmdDataIPS_4_16(0xD51A,1,0x02);
		WriteCmdDataIPS_4_16(0xD51B,1,0x01);
		WriteCmdDataIPS_4_16(0xD51C,1,0x02);
		WriteCmdDataIPS_4_16(0xD51D,1,0x34);
		WriteCmdDataIPS_4_16(0xD51E,1,0x02);
		WriteCmdDataIPS_4_16(0xD51F,1,0x67);
		WriteCmdDataIPS_4_16(0xD520,1,0x02);
		WriteCmdDataIPS_4_16(0xD521,1,0x84);
		WriteCmdDataIPS_4_16(0xD522,1,0x02);
		WriteCmdDataIPS_4_16(0xD523,1,0xA4);
		WriteCmdDataIPS_4_16(0xD524,1,0x02);
		WriteCmdDataIPS_4_16(0xD525,1,0xB7);
		WriteCmdDataIPS_4_16(0xD526,1,0x02);
		WriteCmdDataIPS_4_16(0xD527,1,0xCF);
		WriteCmdDataIPS_4_16(0xD528,1,0x02);
		WriteCmdDataIPS_4_16(0xD529,1,0xDE);
		WriteCmdDataIPS_4_16(0xD52A,1,0x02);
		WriteCmdDataIPS_4_16(0xD52B,1,0xF2);
		WriteCmdDataIPS_4_16(0xD52C,1,0x02);
		WriteCmdDataIPS_4_16(0xD52D,1,0xFE);
		WriteCmdDataIPS_4_16(0xD52E,1,0x03);
		WriteCmdDataIPS_4_16(0xD52F,1,0x10);
		WriteCmdDataIPS_4_16(0xD530,1,0x03);
		WriteCmdDataIPS_4_16(0xD531,1,0x33);
		WriteCmdDataIPS_4_16(0xD532,1,0x03);
		WriteCmdDataIPS_4_16(0xD533,1,0x6D);
		WriteCmdDataIPS_4_16(0xD600,1,0x00);
		WriteCmdDataIPS_4_16(0xD601,1,0x33);
		WriteCmdDataIPS_4_16(0xD602,1,0x00);
		WriteCmdDataIPS_4_16(0xD603,1,0x34);
		WriteCmdDataIPS_4_16(0xD604,1,0x00);
		WriteCmdDataIPS_4_16(0xD605,1,0x3A);
		WriteCmdDataIPS_4_16(0xD606,1,0x00);
		WriteCmdDataIPS_4_16(0xD607,1,0x4A);
		WriteCmdDataIPS_4_16(0xD608,1,0x00);
		WriteCmdDataIPS_4_16(0xD609,1,0x5C);
		WriteCmdDataIPS_4_16(0xD60A,1,0x00);
		WriteCmdDataIPS_4_16(0xD60B,1,0x81);
		WriteCmdDataIPS_4_16(0xD60C,1,0x00);
		WriteCmdDataIPS_4_16(0xD60D,1,0xA6);
		WriteCmdDataIPS_4_16(0xD60E,1,0x00);
		WriteCmdDataIPS_4_16(0xD60F,1,0xE5);
		WriteCmdDataIPS_4_16(0xD610,1,0x01);
		WriteCmdDataIPS_4_16(0xD611,1,0x13);
		WriteCmdDataIPS_4_16(0xD612,1,0x01);
		WriteCmdDataIPS_4_16(0xD613,1,0x54);
		WriteCmdDataIPS_4_16(0xD614,1,0x01);
		WriteCmdDataIPS_4_16(0xD615,1,0x82);
		WriteCmdDataIPS_4_16(0xD616,1,0x01);
		WriteCmdDataIPS_4_16(0xD617,1,0xCA);
		WriteCmdDataIPS_4_16(0xD618,1,0x02);
		WriteCmdDataIPS_4_16(0xD619,1,0x00);
		WriteCmdDataIPS_4_16(0xD61A,1,0x02);
		WriteCmdDataIPS_4_16(0xD61B,1,0x01);
		WriteCmdDataIPS_4_16(0xD61C,1,0x02);
		WriteCmdDataIPS_4_16(0xD61D,1,0x34);
		WriteCmdDataIPS_4_16(0xD61E,1,0x02);
		WriteCmdDataIPS_4_16(0xD61F,1,0x67);
		WriteCmdDataIPS_4_16(0xD620,1,0x02);
		WriteCmdDataIPS_4_16(0xD621,1,0x84);
		WriteCmdDataIPS_4_16(0xD622,1,0x02);
		WriteCmdDataIPS_4_16(0xD623,1,0xA4);
		WriteCmdDataIPS_4_16(0xD624,1,0x02);
		WriteCmdDataIPS_4_16(0xD625,1,0xB7);
		WriteCmdDataIPS_4_16(0xD626,1,0x02);
		WriteCmdDataIPS_4_16(0xD627,1,0xCF);
		WriteCmdDataIPS_4_16(0xD628,1,0x02);
		WriteCmdDataIPS_4_16(0xD629,1,0xDE);
		WriteCmdDataIPS_4_16(0xD62A,1,0x02);
		WriteCmdDataIPS_4_16(0xD62B,1,0xF2);
		WriteCmdDataIPS_4_16(0xD62C,1,0x02);
		WriteCmdDataIPS_4_16(0xD62D,1,0xFE);
		WriteCmdDataIPS_4_16(0xD62E,1,0x03);
		WriteCmdDataIPS_4_16(0xD62F,1,0x10);
		WriteCmdDataIPS_4_16(0xD630,1,0x03);
		WriteCmdDataIPS_4_16(0xD631,1,0x33);
		WriteCmdDataIPS_4_16(0xD632,1,0x03);
		WriteCmdDataIPS_4_16(0xD633,1,0x6D);
		//LV2 Page 0 enable
		WriteCmdDataIPS_4_16(0xF000,1,0x55);
		WriteCmdDataIPS_4_16(0xF001,1,0xAA);
		WriteCmdDataIPS_4_16(0xF002,1,0x52);
		WriteCmdDataIPS_4_16(0xF003,1,0x08);
		WriteCmdDataIPS_4_16(0xF004,1,0x00);
		//Display control
		WriteCmdDataIPS_4_16(0xB100,1, 0xCC);
		WriteCmdDataIPS_4_16(0xB101,1, 0x00);
		//Source hold time
		WriteCmdDataIPS_4_16(0xB600,1,0x05);
		//Gate EQ control
		WriteCmdDataIPS_4_16(0xB700,1,0x70);
		WriteCmdDataIPS_4_16(0xB701,1,0x70);
		//Source EQ control (Mode 2)
		WriteCmdDataIPS_4_16(0xB800,1,0x01);
		WriteCmdDataIPS_4_16(0xB801,1,0x03);
		WriteCmdDataIPS_4_16(0xB802,1,0x03);
		WriteCmdDataIPS_4_16(0xB803,1,0x03);
		//Inversion mode (2-dot)
		WriteCmdDataIPS_4_16(0xBC00,1,0x02);
		WriteCmdDataIPS_4_16(0xBC01,1,0x00);
		WriteCmdDataIPS_4_16(0xBC02,1,0x00);
		//Timing control 4H w/ 4-delay
		WriteCmdDataIPS_4_16(0xC900,1,0xD0);
		WriteCmdDataIPS_4_16(0xC901,1,0x02);
		WriteCmdDataIPS_4_16(0xC902,1,0x50);
		WriteCmdDataIPS_4_16(0xC903,1,0x50);
		WriteCmdDataIPS_4_16(0xC904,1,0x50);
		//WriteCmdDataIPS_4_16(0x3500,1,0x00);
		WriteCmdDataIPS_4_16(0x3A00,1,0x66);  //16-bit/pixel RGB666
#endif

        if ((t & 0x7f) == 0x55){
        	// NT35510 IPS Display detected. Identified in code by (Option.SSDspeed==1)
            Option.SSDspeed = 1;
            SaveOptions();
			WriteCmdDataIPS_4_16(0xF000,1,0x55);
			WriteCmdDataIPS_4_16(0xF001,1,0xAA);
			WriteCmdDataIPS_4_16(0xF002,1,0x52);
			WriteCmdDataIPS_4_16(0xF003,1,0x08);
			WriteCmdDataIPS_4_16(0xF004,1,0x01);
			//#AVDD:manual,1,
			WriteCmdDataIPS_4_16(0xB600,3,0x34);
			//WriteCmdDataIPS_4_16(0xB601,1,0x34);
			//WriteCmdDataIPS_4_16(0xB602,1,0x34);
			WriteCmdDataIPS_4_16(0xB000,3,0x0D);//09
			//WriteCmdDataIPS_4_16(0xB001,1,0x0D);
			//WriteCmdDataIPS_4_16(0xB002,1,0x0D);
			//#AVEE:manual,1,-6V
			WriteCmdDataIPS_4_16(0xB700,3,0x24);
			//WriteCmdDataIPS_4_16(0xB701,1,0x24);
			//WriteCmdDataIPS_4_16(0xB702,1,0x24);
			WriteCmdDataIPS_4_16(0xB100,3,0x0D);
			//WriteCmdDataIPS_4_16(0xB101,1,0x0D);
			//WriteCmdDataIPS_4_16(0xB102,1,0x0D);
			//#PowerControlfor
			//VCL
			WriteCmdDataIPS_4_16(0xB800,3,0x24);
			//WriteCmdDataIPS_4_16(0xB801,1,0x24);
			//WriteCmdDataIPS_4_16(0xB802,1,0x24);
			WriteCmdDataIPS_4_16(0xB200,1,0x00);
			//#VGH:ClampEnable,1,
			WriteCmdDataIPS_4_16(0xB900,3,0x24);
			//WriteCmdDataIPS_4_16(0xB901,1,0x24);
			//WriteCmdDataIPS_4_16(0xB902,1,0x24);
			WriteCmdDataIPS_4_16(0xB300,3,0x05);
			//WriteCmdDataIPS_4_16(0xB301,1,0x05);
			//WriteCmdDataIPS_4_16(0xB302,1,0x05);
			WriteCmdDataIPS_4_16(0xBF00,1,0x01);
			//#VGL(LVGL):
			WriteCmdDataIPS_4_16(0xBA00,3,0x34);
			//WriteCmdDataIPS_4_16(0xBA01,1,0x34);
			//WriteCmdDataIPS_4_16(0xBA02,1,0x34);
			//#VGL_REG(VGLO)
			WriteCmdDataIPS_4_16(0xB500,3,0x0B);
			//WriteCmdDataIPS_4_16(0xB501,1,0x0B);
			//WriteCmdDataIPS_4_16(0xB502,1,0x0B);
			//#VGMP/VGSP:
			WriteCmdDataIPS_4_16(0xBC00,1,0x00);
			WriteCmdDataIPS_4_16(0xBC01,1,0xA3);
			WriteCmdDataIPS_4_16(0xBC02,1,0x00);
			//#VGMN/VGSN
			WriteCmdDataIPS_4_16(0xBD00,1,0x00);
			WriteCmdDataIPS_4_16(0xBD01,1,0xA3);
			WriteCmdDataIPS_4_16(0xBD02,1,0x00);
			//#VCOM=-0.1
			WriteCmdDataIPS_4_16(0xBE00,1,0x00);
			WriteCmdDataIPS_4_16(0xBE01,1,0x63);//4f
				//VCOMH+0x01;
			//#R+
			WriteCmdDataIPS_4_16(0xD100,1,0x00);
			WriteCmdDataIPS_4_16(0xD101,1,0x37);
			WriteCmdDataIPS_4_16(0xD102,1,0x00);
			WriteCmdDataIPS_4_16(0xD103,1,0x52);
			WriteCmdDataIPS_4_16(0xD104,1,0x00);
			WriteCmdDataIPS_4_16(0xD105,1,0x7B);
			WriteCmdDataIPS_4_16(0xD106,1,0x00);
			WriteCmdDataIPS_4_16(0xD107,1,0x99);
			WriteCmdDataIPS_4_16(0xD108,1,0x00);
			WriteCmdDataIPS_4_16(0xD109,1,0xB1);
			WriteCmdDataIPS_4_16(0xD10A,1,0x00);
			WriteCmdDataIPS_4_16(0xD10B,1,0xD2);
			WriteCmdDataIPS_4_16(0xD10C,1,0x00);
			WriteCmdDataIPS_4_16(0xD10D,1,0xF6);
			WriteCmdDataIPS_4_16(0xD10E,1,0x01);
			WriteCmdDataIPS_4_16(0xD10F,1,0x27);
			WriteCmdDataIPS_4_16(0xD110,1,0x01);
			WriteCmdDataIPS_4_16(0xD111,1,0x4E);
			WriteCmdDataIPS_4_16(0xD112,1,0x01);
			WriteCmdDataIPS_4_16(0xD113,1,0x8C);
			WriteCmdDataIPS_4_16(0xD114,1,0x01);
			WriteCmdDataIPS_4_16(0xD115,1,0xBE);
			WriteCmdDataIPS_4_16(0xD116,1,0x02);
			WriteCmdDataIPS_4_16(0xD117,1,0x0B);
			WriteCmdDataIPS_4_16(0xD118,1,0x02);
			WriteCmdDataIPS_4_16(0xD119,1,0x48);
			WriteCmdDataIPS_4_16(0xD11A,1,0x02);
			WriteCmdDataIPS_4_16(0xD11B,1,0x4A);
			WriteCmdDataIPS_4_16(0xD11C,1,0x02);
			WriteCmdDataIPS_4_16(0xD11D,1,0x7E);
			WriteCmdDataIPS_4_16(0xD11E,1,0x02);
			WriteCmdDataIPS_4_16(0xD11F,1,0xBC);
			WriteCmdDataIPS_4_16(0xD120,1,0x02);
			WriteCmdDataIPS_4_16(0xD121,1,0xE1);
			WriteCmdDataIPS_4_16(0xD122,1,0x03);
			WriteCmdDataIPS_4_16(0xD123,1,0x10);
			WriteCmdDataIPS_4_16(0xD124,1,0x03);
			WriteCmdDataIPS_4_16(0xD125,1,0x31);
			WriteCmdDataIPS_4_16(0xD126,1,0x03);
			WriteCmdDataIPS_4_16(0xD127,1,0x5A);
			WriteCmdDataIPS_4_16(0xD128,1,0x03);
			WriteCmdDataIPS_4_16(0xD129,1,0x73);
			WriteCmdDataIPS_4_16(0xD12A,1,0x03);
			WriteCmdDataIPS_4_16(0xD12B,1,0x94);
			WriteCmdDataIPS_4_16(0xD12C,1,0x03);
			WriteCmdDataIPS_4_16(0xD12D,1,0x9F);
			WriteCmdDataIPS_4_16(0xD12E,1,0x03);
			WriteCmdDataIPS_4_16(0xD12F,1,0xB3);
			WriteCmdDataIPS_4_16(0xD130,1,0x03);
			WriteCmdDataIPS_4_16(0xD131,1,0xB9);
			WriteCmdDataIPS_4_16(0xD132,1,0x03);
			WriteCmdDataIPS_4_16(0xD133,1,0xC1);
			//#G+
			WriteCmdDataIPS_4_16(0xD200,1,0x00);
			WriteCmdDataIPS_4_16(0xD201,1,0x37);
			WriteCmdDataIPS_4_16(0xD202,1,0x00);
			WriteCmdDataIPS_4_16(0xD203,1,0x52);
			WriteCmdDataIPS_4_16(0xD204,1,0x00);
			WriteCmdDataIPS_4_16(0xD205,1,0x7B);
			WriteCmdDataIPS_4_16(0xD206,1,0x00);
			WriteCmdDataIPS_4_16(0xD207,1,0x99);
			WriteCmdDataIPS_4_16(0xD208,1,0x00);
			WriteCmdDataIPS_4_16(0xD209,1,0xB1);
			WriteCmdDataIPS_4_16(0xD20A,1,0x00);
			WriteCmdDataIPS_4_16(0xD20B,1,0xD2);
			WriteCmdDataIPS_4_16(0xD20C,1,0x00);
			WriteCmdDataIPS_4_16(0xD20D,1,0xF6);
			WriteCmdDataIPS_4_16(0xD20E,1,0x01);
			WriteCmdDataIPS_4_16(0xD20F,1,0x27);
			WriteCmdDataIPS_4_16(0xD210,1,0x01);
			WriteCmdDataIPS_4_16(0xD211,1,0x4E);
			WriteCmdDataIPS_4_16(0xD212,1,0x01);
			WriteCmdDataIPS_4_16(0xD213,1,0x8C);
			WriteCmdDataIPS_4_16(0xD214,1,0x01);
			WriteCmdDataIPS_4_16(0xD215,1,0xBE);
			WriteCmdDataIPS_4_16(0xD216,1,0x02);
			WriteCmdDataIPS_4_16(0xD217,1,0x0B);
			WriteCmdDataIPS_4_16(0xD218,1,0x02);
			WriteCmdDataIPS_4_16(0xD219,1,0x48);
			WriteCmdDataIPS_4_16(0xD21A,1,0x02);
			WriteCmdDataIPS_4_16(0xD21B,1,0x4A);
			WriteCmdDataIPS_4_16(0xD21C,1,0x02);
			WriteCmdDataIPS_4_16(0xD21D,1,0x7E);
			WriteCmdDataIPS_4_16(0xD21E,1,0x02);
			WriteCmdDataIPS_4_16(0xD21F,1,0xBC);
			WriteCmdDataIPS_4_16(0xD220,1,0x02);
			WriteCmdDataIPS_4_16(0xD221,1,0xE1);
			WriteCmdDataIPS_4_16(0xD222,1,0x03);
			WriteCmdDataIPS_4_16(0xD223,1,0x10);
			WriteCmdDataIPS_4_16(0xD224,1,0x03);
			WriteCmdDataIPS_4_16(0xD225,1,0x31);
			WriteCmdDataIPS_4_16(0xD226,1,0x03);
			WriteCmdDataIPS_4_16(0xD227,1,0x5A);
			WriteCmdDataIPS_4_16(0xD228,1,0x03);
			WriteCmdDataIPS_4_16(0xD229,1,0x73);
			WriteCmdDataIPS_4_16(0xD22A,1,0x03);
			WriteCmdDataIPS_4_16(0xD22B,1,0x94);
			WriteCmdDataIPS_4_16(0xD22C,1,0x03);
			WriteCmdDataIPS_4_16(0xD22D,1,0x9F);
			WriteCmdDataIPS_4_16(0xD22E,1,0x03);
			WriteCmdDataIPS_4_16(0xD22F,1,0xB3);
			WriteCmdDataIPS_4_16(0xD230,1,0x03);
			WriteCmdDataIPS_4_16(0xD231,1,0xB9);
			WriteCmdDataIPS_4_16(0xD232,1,0x03);
			WriteCmdDataIPS_4_16(0xD233,1,0xC1);
			//#B+
			WriteCmdDataIPS_4_16(0xD300,1,0x00);
			WriteCmdDataIPS_4_16(0xD301,1,0x37);
			WriteCmdDataIPS_4_16(0xD302,1,0x00);
			WriteCmdDataIPS_4_16(0xD303,1,0x52);
			WriteCmdDataIPS_4_16(0xD304,1,0x00);
			WriteCmdDataIPS_4_16(0xD305,1,0x7B);
			WriteCmdDataIPS_4_16(0xD306,1,0x00);
			WriteCmdDataIPS_4_16(0xD307,1,0x99);
			WriteCmdDataIPS_4_16(0xD308,1,0x00);
			WriteCmdDataIPS_4_16(0xD309,1,0xB1);
			WriteCmdDataIPS_4_16(0xD30A,1,0x00);
			WriteCmdDataIPS_4_16(0xD30B,1,0xD2);
			WriteCmdDataIPS_4_16(0xD30C,1,0x00);
			WriteCmdDataIPS_4_16(0xD30D,1,0xF6);
			WriteCmdDataIPS_4_16(0xD30E,1,0x01);
			WriteCmdDataIPS_4_16(0xD30F,1,0x27);
			WriteCmdDataIPS_4_16(0xD310,1,0x01);
			WriteCmdDataIPS_4_16(0xD311,1,0x4E);
			WriteCmdDataIPS_4_16(0xD312,1,0x01);
			WriteCmdDataIPS_4_16(0xD313,1,0x8C);
			WriteCmdDataIPS_4_16(0xD314,1,0x01);
			WriteCmdDataIPS_4_16(0xD315,1,0xBE);
			WriteCmdDataIPS_4_16(0xD316,1,0x02);
			WriteCmdDataIPS_4_16(0xD317,1,0x0B);
			WriteCmdDataIPS_4_16(0xD318,1,0x02);
			WriteCmdDataIPS_4_16(0xD319,1,0x48);
			WriteCmdDataIPS_4_16(0xD31A,1,0x02);
			WriteCmdDataIPS_4_16(0xD31B,1,0x4A);
			WriteCmdDataIPS_4_16(0xD31C,1,0x02);
			WriteCmdDataIPS_4_16(0xD31D,1,0x7E);
			WriteCmdDataIPS_4_16(0xD31E,1,0x02);
			WriteCmdDataIPS_4_16(0xD31F,1,0xBC);
			WriteCmdDataIPS_4_16(0xD320,1,0x02);
			WriteCmdDataIPS_4_16(0xD321,1,0xE1);
			WriteCmdDataIPS_4_16(0xD322,1,0x03);
			WriteCmdDataIPS_4_16(0xD323,1,0x10);
			WriteCmdDataIPS_4_16(0xD324,1,0x03);
			WriteCmdDataIPS_4_16(0xD325,1,0x31);
			WriteCmdDataIPS_4_16(0xD326,1,0x03);
			WriteCmdDataIPS_4_16(0xD327,1,0x5A);
			WriteCmdDataIPS_4_16(0xD328,1,0x03);
			WriteCmdDataIPS_4_16(0xD329,1,0x73);
			WriteCmdDataIPS_4_16(0xD32A,1,0x03);
			WriteCmdDataIPS_4_16(0xD32B,1,0x94);
			WriteCmdDataIPS_4_16(0xD32C,1,0x03);
			WriteCmdDataIPS_4_16(0xD32D,1,0x9F);
			WriteCmdDataIPS_4_16(0xD32E,1,0x03);
			WriteCmdDataIPS_4_16(0xD32F,1,0xB3);
			WriteCmdDataIPS_4_16(0xD330,1,0x03);
			WriteCmdDataIPS_4_16(0xD331,1,0xB9);
			WriteCmdDataIPS_4_16(0xD332,1,0x03);
			WriteCmdDataIPS_4_16(0xD333,1,0xC1);
			//#R-///////////////////////////////////////////
			WriteCmdDataIPS_4_16(0xD400,1,0x00);
			WriteCmdDataIPS_4_16(0xD401,1,0x37);
			WriteCmdDataIPS_4_16(0xD402,1,0x00);
			WriteCmdDataIPS_4_16(0xD403,1,0x52);
			WriteCmdDataIPS_4_16(0xD404,1,0x00);
			WriteCmdDataIPS_4_16(0xD405,1,0x7B);
			WriteCmdDataIPS_4_16(0xD406,1,0x00);
			WriteCmdDataIPS_4_16(0xD407,1,0x99);
			WriteCmdDataIPS_4_16(0xD408,1,0x00);
			WriteCmdDataIPS_4_16(0xD409,1,0xB1);
			WriteCmdDataIPS_4_16(0xD40A,1,0x00);
			WriteCmdDataIPS_4_16(0xD40B,1,0xD2);
			WriteCmdDataIPS_4_16(0xD40C,1,0x00);
			WriteCmdDataIPS_4_16(0xD40D,1,0xF6);
			WriteCmdDataIPS_4_16(0xD40E,1,0x01);
			WriteCmdDataIPS_4_16(0xD40F,1,0x27);
			WriteCmdDataIPS_4_16(0xD410,1,0x01);
			WriteCmdDataIPS_4_16(0xD411,1,0x4E);
			WriteCmdDataIPS_4_16(0xD412,1,0x01);
			WriteCmdDataIPS_4_16(0xD413,1,0x8C);
			WriteCmdDataIPS_4_16(0xD414,1,0x01);
			WriteCmdDataIPS_4_16(0xD415,1,0xBE);
			WriteCmdDataIPS_4_16(0xD416,1,0x02);
			WriteCmdDataIPS_4_16(0xD417,1,0x0B);
			WriteCmdDataIPS_4_16(0xD418,1,0x02);
			WriteCmdDataIPS_4_16(0xD419,1,0x48);
			WriteCmdDataIPS_4_16(0xD41A,1,0x02);
			WriteCmdDataIPS_4_16(0xD41B,1,0x4A);
			WriteCmdDataIPS_4_16(0xD41C,1,0x02);
			WriteCmdDataIPS_4_16(0xD41D,1,0x7E);
			WriteCmdDataIPS_4_16(0xD41E,1,0x02);
			WriteCmdDataIPS_4_16(0xD41F,1,0xBC);
			WriteCmdDataIPS_4_16(0xD420,1,0x02);
			WriteCmdDataIPS_4_16(0xD421,1,0xE1);
			WriteCmdDataIPS_4_16(0xD422,1,0x03);
			WriteCmdDataIPS_4_16(0xD423,1,0x10);
			WriteCmdDataIPS_4_16(0xD424,1,0x03);
			WriteCmdDataIPS_4_16(0xD425,1,0x31);
			WriteCmdDataIPS_4_16(0xD426,1,0x03);
			WriteCmdDataIPS_4_16(0xD427,1,0x5A);
			WriteCmdDataIPS_4_16(0xD428,1,0x03);
			WriteCmdDataIPS_4_16(0xD429,1,0x73);
			WriteCmdDataIPS_4_16(0xD42A,1,0x03);
			WriteCmdDataIPS_4_16(0xD42B,1,0x94);
			WriteCmdDataIPS_4_16(0xD42C,1,0x03);
			WriteCmdDataIPS_4_16(0xD42D,1,0x9F);
			WriteCmdDataIPS_4_16(0xD42E,1,0x03);
			WriteCmdDataIPS_4_16(0xD42F,1,0xB3);
			WriteCmdDataIPS_4_16(0xD430,1,0x03);
			WriteCmdDataIPS_4_16(0xD431,1,0xB9);
			WriteCmdDataIPS_4_16(0xD432,1,0x03);
			WriteCmdDataIPS_4_16(0xD433,1,0xC1);
			//#G-//////////////////////////////////////////////
			WriteCmdDataIPS_4_16(0xD500,1,0x00);
			WriteCmdDataIPS_4_16(0xD501,1,0x37);
			WriteCmdDataIPS_4_16(0xD502,1,0x00);
			WriteCmdDataIPS_4_16(0xD503,1,0x52);
			WriteCmdDataIPS_4_16(0xD504,1,0x00);
			WriteCmdDataIPS_4_16(0xD505,1,0x7B);
			WriteCmdDataIPS_4_16(0xD506,1,0x00);
			WriteCmdDataIPS_4_16(0xD507,1,0x99);
			WriteCmdDataIPS_4_16(0xD508,1,0x00);
			WriteCmdDataIPS_4_16(0xD509,1,0xB1);
			WriteCmdDataIPS_4_16(0xD50A,1,0x00);
			WriteCmdDataIPS_4_16(0xD50B,1,0xD2);
			WriteCmdDataIPS_4_16(0xD50C,1,0x00);
			WriteCmdDataIPS_4_16(0xD50D,1,0xF6);
			WriteCmdDataIPS_4_16(0xD50E,1,0x01);
			WriteCmdDataIPS_4_16(0xD50F,1,0x27);
			WriteCmdDataIPS_4_16(0xD510,1,0x01);
			WriteCmdDataIPS_4_16(0xD511,1,0x4E);
			WriteCmdDataIPS_4_16(0xD512,1,0x01);
			WriteCmdDataIPS_4_16(0xD513,1,0x8C);
			WriteCmdDataIPS_4_16(0xD514,1,0x01);
			WriteCmdDataIPS_4_16(0xD515,1,0xBE);
			WriteCmdDataIPS_4_16(0xD516,1,0x02);
			WriteCmdDataIPS_4_16(0xD517,1,0x0B);
			WriteCmdDataIPS_4_16(0xD518,1,0x02);
			WriteCmdDataIPS_4_16(0xD519,1,0x48);
			WriteCmdDataIPS_4_16(0xD51A,1,0x02);
			WriteCmdDataIPS_4_16(0xD51B,1,0x4A);
			WriteCmdDataIPS_4_16(0xD51C,1,0x02);
			WriteCmdDataIPS_4_16(0xD51D,1,0x7E);
			WriteCmdDataIPS_4_16(0xD51E,1,0x02);
			WriteCmdDataIPS_4_16(0xD51F,1,0xBC);
			WriteCmdDataIPS_4_16(0xD520,1,0x02);
			WriteCmdDataIPS_4_16(0xD521,1,0xE1);
			WriteCmdDataIPS_4_16(0xD522,1,0x03);
			WriteCmdDataIPS_4_16(0xD523,1,0x10);
			WriteCmdDataIPS_4_16(0xD524,1,0x03);
			WriteCmdDataIPS_4_16(0xD525,1,0x31);
			WriteCmdDataIPS_4_16(0xD526,1,0x03);
			WriteCmdDataIPS_4_16(0xD527,1,0x5A);
			WriteCmdDataIPS_4_16(0xD528,1,0x03);
			WriteCmdDataIPS_4_16(0xD529,1,0x73);
			WriteCmdDataIPS_4_16(0xD52A,1,0x03);
			WriteCmdDataIPS_4_16(0xD52B,1,0x94);
			WriteCmdDataIPS_4_16(0xD52C,1,0x03);
			WriteCmdDataIPS_4_16(0xD52D,1,0x9F);
			WriteCmdDataIPS_4_16(0xD52E,1,0x03);
			WriteCmdDataIPS_4_16(0xD52F,1,0xB3);
			WriteCmdDataIPS_4_16(0xD530,1,0x03);
			WriteCmdDataIPS_4_16(0xD531,1,0xB9);
			WriteCmdDataIPS_4_16(0xD532,1,0x03);
			WriteCmdDataIPS_4_16(0xD533,1,0xC1);
			//#B-///////////////////////////////
			WriteCmdDataIPS_4_16(0xD600,1,0x00);
			WriteCmdDataIPS_4_16(0xD601,1,0x37);
			WriteCmdDataIPS_4_16(0xD602,1,0x00);
			WriteCmdDataIPS_4_16(0xD603,1,0x52);
			WriteCmdDataIPS_4_16(0xD604,1,0x00);
			WriteCmdDataIPS_4_16(0xD605,1,0x7B);
			WriteCmdDataIPS_4_16(0xD606,1,0x00);
			WriteCmdDataIPS_4_16(0xD607,1,0x99);
			WriteCmdDataIPS_4_16(0xD608,1,0x00);
			WriteCmdDataIPS_4_16(0xD609,1,0xB1);
			WriteCmdDataIPS_4_16(0xD60A,1,0x00);
			WriteCmdDataIPS_4_16(0xD60B,1,0xD2);
			WriteCmdDataIPS_4_16(0xD60C,1,0x00);
			WriteCmdDataIPS_4_16(0xD60D,1,0xF6);
			WriteCmdDataIPS_4_16(0xD60E,1,0x01);
			WriteCmdDataIPS_4_16(0xD60F,1,0x27);
			WriteCmdDataIPS_4_16(0xD610,1,0x01);
			WriteCmdDataIPS_4_16(0xD611,1,0x4E);
			WriteCmdDataIPS_4_16(0xD612,1,0x01);
			WriteCmdDataIPS_4_16(0xD613,1,0x8C);
			WriteCmdDataIPS_4_16(0xD614,1,0x01);
			WriteCmdDataIPS_4_16(0xD615,1,0xBE);
			WriteCmdDataIPS_4_16(0xD616,1,0x02);
			WriteCmdDataIPS_4_16(0xD617,1,0x0B);
			WriteCmdDataIPS_4_16(0xD618,1,0x02);
			WriteCmdDataIPS_4_16(0xD619,1,0x48);
			WriteCmdDataIPS_4_16(0xD61A,1,0x02);
			WriteCmdDataIPS_4_16(0xD61B,1,0x4A);
			WriteCmdDataIPS_4_16(0xD61C,1,0x02);
			WriteCmdDataIPS_4_16(0xD61D,1,0x7E);
			WriteCmdDataIPS_4_16(0xD61E,1,0x02);
			WriteCmdDataIPS_4_16(0xD61F,1,0xBC);
			WriteCmdDataIPS_4_16(0xD620,1,0x02);
			WriteCmdDataIPS_4_16(0xD621,1,0xE1);
			WriteCmdDataIPS_4_16(0xD622,1,0x03);
			WriteCmdDataIPS_4_16(0xD623,1,0x10);
			WriteCmdDataIPS_4_16(0xD624,1,0x03);
			WriteCmdDataIPS_4_16(0xD625,1,0x31);
			WriteCmdDataIPS_4_16(0xD626,1,0x03);
			WriteCmdDataIPS_4_16(0xD627,1,0x5A);
			WriteCmdDataIPS_4_16(0xD628,1,0x03);
			WriteCmdDataIPS_4_16(0xD629,1,0x73);
			WriteCmdDataIPS_4_16(0xD62A,1,0x03);
			WriteCmdDataIPS_4_16(0xD62B,1,0x94);
			WriteCmdDataIPS_4_16(0xD62C,1,0x03);
			WriteCmdDataIPS_4_16(0xD62D,1,0x9F);
			WriteCmdDataIPS_4_16(0xD62E,1,0x03);
			WriteCmdDataIPS_4_16(0xD62F,1,0xB3);
			WriteCmdDataIPS_4_16(0xD630,1,0x03);
			WriteCmdDataIPS_4_16(0xD631,1,0xB9);
			WriteCmdDataIPS_4_16(0xD632,1,0x03);
			WriteCmdDataIPS_4_16(0xD633,1,0xC1);
			//#EnablePage0
			WriteCmdDataIPS_4_16(0xF000,1,0x55);
			WriteCmdDataIPS_4_16(0xF001,1,0xAA);
			WriteCmdDataIPS_4_16(0xF002,1,0x52);
			WriteCmdDataIPS_4_16(0xF003,1,0x08);
			WriteCmdDataIPS_4_16(0xF004,1,0x00);
			//#RGBI/FSetting
			WriteCmdDataIPS_4_16(0xB000,1,0x08);
			WriteCmdDataIPS_4_16(0xB001,1,0x05);
			WriteCmdDataIPS_4_16(0xB002,1,0x02);
			WriteCmdDataIPS_4_16(0xB003,1,0x05);
			WriteCmdDataIPS_4_16(0xB004,1,0x02);
			//##SDT:
			WriteCmdDataIPS_4_16(0xB600,1,0x08);
			WriteCmdDataIPS_4_16(0xB500,1,0x50);//480x800
			//##GateEQ:
			WriteCmdDataIPS_4_16(0xB700,2,0x00);
			//WriteCmdDataIPS_4_16(0xB701,1,0x00);
			//##SourceEQ:
			WriteCmdDataIPS_4_16(0xB800,1,0x01);
			WriteCmdDataIPS_4_16(0xB801,3,0x05);
			//WriteCmdDataIPS_4_16(0xB802,1,0x05);
			//WriteCmdDataIPS_4_16(0xB803,1,0x05);
			//#Inversion:Columninversion(NVT)
			WriteCmdDataIPS_4_16(0xBC00,3,0x00);
			//WriteCmdDataIPS_4_16(0xBC01,1,0x00);
			//WriteCmdDataIPS_4_16(0xBC02,1,0x00);
			//#BOE'sSetting(default)
			WriteCmdDataIPS_4_16(0xCC00,1,0x03);
			WriteCmdDataIPS_4_16(0xCC01,2,0x00);
			//WriteCmdDataIPS_4_16(0xCC02,1,0x00);
			//#DisplayTiming:
			WriteCmdDataIPS_4_16(0xBD00,1,0x01);
			WriteCmdDataIPS_4_16(0xBD01,1,0x84);
			WriteCmdDataIPS_4_16(0xBD02,1,0x07);
			WriteCmdDataIPS_4_16(0xBD03,1,0x31);
			WriteCmdDataIPS_4_16(0xBD04,1,0x00);
			WriteCmdDataIPS_4_16(0xBA00,1,0x01);
			WriteCmdDataIPS_4_16(0xFF00,1,0xAA);
			WriteCmdDataIPS_4_16(0xFF01,1,0x55);
			WriteCmdDataIPS_4_16(0xFF02,1,0x25);
			WriteCmdDataIPS_4_16(0xFF03,1,0x01);
			WriteCmdDataIPS_4_16(0x3500,1,0x00);
			WriteCmdDataIPS_4_16(0x3A00,1,0x66); // 55=Colour 565 66=Colour 666 77=Colour 888
		}else{
		//============ OTM8009A+HSD3.97 20140613 ===============================================//
		Option.SSDspeed = 0;
		WriteCmdDataIPS_4_16(0xff00,1,0x80); //enable access command2
		WriteCmdDataIPS_4_16(0xff01,1,0x09); //enable access command2
		WriteCmdDataIPS_4_16(0xff02,1,0x01); //enable access command2
		WriteCmdDataIPS_4_16(0xff80,1,0x80); //enable access command2
		WriteCmdDataIPS_4_16(0xff81,1,0x09); //enable access command2


		WriteCmdDataIPS_4_16(0xff03,1,0x01); //DON?T KNOW ???
		WriteCmdDataIPS_4_16(0xc5b1,1,0xA9); //power control
		WriteCmdDataIPS_4_16(0xc591,1,0x0F); //power control
		WriteCmdDataIPS_4_16(0xc0B4,1,0x50);

		//panel driving mode : column inversion

		/* Gamma Correction 2.2+ Setting */
		WriteCmdDataIPS_4_16(0xE100,1,0x00);
		WriteCmdDataIPS_4_16(0xE101,1,0x09);
		WriteCmdDataIPS_4_16(0xE102,1,0x0F);
		WriteCmdDataIPS_4_16(0xE103,1,0x0E);
		WriteCmdDataIPS_4_16(0xE104,1,0x07);
		WriteCmdDataIPS_4_16(0xE105,1,0x10);
		WriteCmdDataIPS_4_16(0xE106,1,0x0B);
		WriteCmdDataIPS_4_16(0xE107,1,0x0A);
		WriteCmdDataIPS_4_16(0xE108,1,0x04);
		WriteCmdDataIPS_4_16(0xE109,1,0x07);
		WriteCmdDataIPS_4_16(0xE10A,1,0x0B);
		WriteCmdDataIPS_4_16(0xE10B,1,0x08);
		WriteCmdDataIPS_4_16(0xE10C,1,0x0F);
		WriteCmdDataIPS_4_16(0xE10D,1,0x10);
		WriteCmdDataIPS_4_16(0xE10E,1,0x0A);
		WriteCmdDataIPS_4_16(0xE10F,1,0x01);
		/* Gamma Correction 2.2- Setting */
		WriteCmdDataIPS_4_16(0xE200,1,0x00);
		WriteCmdDataIPS_4_16(0xE201,1,0x09);
		WriteCmdDataIPS_4_16(0xE202,1,0x0F);
		WriteCmdDataIPS_4_16(0xE203,1,0x0E);
		WriteCmdDataIPS_4_16(0xE204,1,0x07);
		WriteCmdDataIPS_4_16(0xE205,1,0x10);
		WriteCmdDataIPS_4_16(0xE206,1,0x0B);
		WriteCmdDataIPS_4_16(0xE207,1,0x0A);
		WriteCmdDataIPS_4_16(0xE208,1,0x04);
		WriteCmdDataIPS_4_16(0xE209,1,0x07);
		WriteCmdDataIPS_4_16(0xE20A,1,0x0B);
		WriteCmdDataIPS_4_16(0xE20B,1,0x08);
		WriteCmdDataIPS_4_16(0xE20C,1,0x0F);
		WriteCmdDataIPS_4_16(0xE20D,1,0x10);
		WriteCmdDataIPS_4_16(0xE20E,1,0x0A);
		WriteCmdDataIPS_4_16(0xE20F,1,0x01);

		WriteCmdDataIPS_4_16(0xD900,1,0x4E); /* VCOM Voltage Setting */
		WriteCmdDataIPS_4_16(0xc181,1,0x66); //osc=65HZ
		WriteCmdDataIPS_4_16(0xc1a1,1,0x08); //RGB Video Mode Setting
		WriteCmdDataIPS_4_16(0xc592,1,0x01); //power control
		WriteCmdDataIPS_4_16(0xc595,1,0x34); //power control
		WriteCmdDataIPS_4_16(0xd800,2,0x79); //GVDD / NGVDD setting

		WriteCmdDataIPS_4_16(0xc594,1,0x33); //power control

		WriteCmdDataIPS_4_16(0xc0a3,1,0x1B); //panel timing setting
		WriteCmdDataIPS_4_16(0xc582,1,0x83); //power control
		WriteCmdDataIPS_4_16(0xc481,1,0x83);  //source driver setting
		WriteCmdDataIPS_4_16(0xc1a1,1,0x0E);
		WriteCmdDataIPS_4_16(0xb3a6,1,0x20);
		WriteCmdDataIPS_4_16(0xb3a7,1,0x01);

		WriteCmdDataIPS_4_16(0xce80,1,0x85); // GOA VST
		WriteCmdDataIPS_4_16(0xce81,1,0x01); // GOA VST
		WriteCmdDataIPS_4_16(0xce82,1,0x00); // GOA VST
		WriteCmdDataIPS_4_16(0xce83,1,0x84); // GOA VST
		WriteCmdDataIPS_4_16(0xce84,1,0x01); // GOA VST
		WriteCmdDataIPS_4_16(0xce85,1,0x00); // GOA VST
		WriteCmdDataIPS_4_16(0xcea0,1,0x18); // GOA CLK
		WriteCmdDataIPS_4_16(0xcea1,1,0x04); // GOA CLK
		WriteCmdDataIPS_4_16(0xcea2,1,0x03); // GOA CLK
		WriteCmdDataIPS_4_16(0xcea3,1,0x39); // GOA CLK
		WriteCmdDataIPS_4_16(0xcea4,3,0x00); // GOA CLK

		WriteCmdDataIPS_4_16(0xcea7,1,0x18); // GOA CLK
		WriteCmdDataIPS_4_16(0xcea8,1,0x03); // GOA CLK
		WriteCmdDataIPS_4_16(0xcea9,1,0x03); // GOA CLK
		WriteCmdDataIPS_4_16(0xceaa,1,0x3a); // GOA CLK
		WriteCmdDataIPS_4_16(0xceab,3,0x00); // GOA CLK

		WriteCmdDataIPS_4_16(0xceb0,1,0x18); // GOA CLK
		WriteCmdDataIPS_4_16(0xceb1,1,0x02); // GOA CLK
		WriteCmdDataIPS_4_16(0xceb2,1,0x03); // GOA CLK
		WriteCmdDataIPS_4_16(0xceb3,1,0x3b); // GOA CLK
		WriteCmdDataIPS_4_16(0xceb4,3,0x00); // GOA CLK

		WriteCmdDataIPS_4_16(0xceb7,1,0x18); // GOA CLK
		WriteCmdDataIPS_4_16(0xceb8,1,0x01); // GOA CLK
		WriteCmdDataIPS_4_16(0xceb9,1,0x03); // GOA CLK
		WriteCmdDataIPS_4_16(0xceba,1,0x3c); // GOA CLK
		WriteCmdDataIPS_4_16(0xcebb,3,0x00); // GOA CLK

		WriteCmdDataIPS_4_16(0xcfc0,1,0x01); // GOA ECLK
		WriteCmdDataIPS_4_16(0xcfc1,1,0x01); // GOA ECLK
		WriteCmdDataIPS_4_16(0xcfc2,2,0x20); // GOA ECLK

		WriteCmdDataIPS_4_16(0xcfc4,2,0x00); // GOA ECLK

		WriteCmdDataIPS_4_16(0xcfc6,1,0x01); // GOA other options
		WriteCmdDataIPS_4_16(0xcfc7,1,0x00);

		// GOA signal toggle option setting
		WriteCmdDataIPS_4_16(0xcfc8,2,0x00); //GOA signal toggle option setting

		//GOA signal toggle option setting
		WriteCmdDataIPS_4_16(0xcfd0,1,0x00);
		WriteCmdDataIPS_4_16(0xcb80,10,0x00);

		WriteCmdDataIPS_4_16(0xcb90,15,0x00);
		WriteCmdDataIPS_4_16(0xcba0,15,0x00);
		WriteCmdDataIPS_4_16(0xcbb0,10,0x00);

		WriteCmdDataIPS_4_16(0xcbc0,1,0x00);
		WriteCmdDataIPS_4_16(0xcbc1,5,0x04);

		WriteCmdDataIPS_4_16(0xcbc6,9,0x00);

		WriteCmdDataIPS_4_16(0xcbd0,6,0x00);

		WriteCmdDataIPS_4_16(0xcbd6,5,0x04);

		WriteCmdDataIPS_4_16(0xcbdb,4,0x00);

		WriteCmdDataIPS_4_16(0xcbe0,10,0x00);

		WriteCmdDataIPS_4_16(0xcbf0,10,0xFF);

		WriteCmdDataIPS_4_16(0xcc80,1,0x00);
		WriteCmdDataIPS_4_16(0xcc81,1,0x26);
		WriteCmdDataIPS_4_16(0xcc82,1,0x09);
		WriteCmdDataIPS_4_16(0xcc83,1,0x0B);
		WriteCmdDataIPS_4_16(0xcc84,1,0x01);
		WriteCmdDataIPS_4_16(0xcc85,1,0x25);
		WriteCmdDataIPS_4_16(0xcc86,4,0x00);

		WriteCmdDataIPS_4_16(0xcc90,11,0x00);

		WriteCmdDataIPS_4_16(0xcc9b,1,0x26);
		WriteCmdDataIPS_4_16(0xcc9c,1,0x0A);
		WriteCmdDataIPS_4_16(0xcc9d,1,0x0C);
		WriteCmdDataIPS_4_16(0xcc9e,1,0x02);
		WriteCmdDataIPS_4_16(0xcca0,1,0x25);
		WriteCmdDataIPS_4_16(0xcca1,14,0x00);

		WriteCmdDataIPS_4_16(0xccb0,1,0x00);
		WriteCmdDataIPS_4_16(0xccb1,1,0x25);
		WriteCmdDataIPS_4_16(0xccb2,1,0x0C);
		WriteCmdDataIPS_4_16(0xccb3,1,0x0A);
		WriteCmdDataIPS_4_16(0xccb4,1,0x02);
		WriteCmdDataIPS_4_16(0xccb5,1,0x26);
		WriteCmdDataIPS_4_16(0xccb6,4,0x00);

		WriteCmdDataIPS_4_16(0xccc0,11,0x00);

		WriteCmdDataIPS_4_16(0xcccb,1,0x25);
		WriteCmdDataIPS_4_16(0xcccc,1,0x0B);
		WriteCmdDataIPS_4_16(0xcccd,1,0x09);
		WriteCmdDataIPS_4_16(0xccce,1,0x01);
		WriteCmdDataIPS_4_16(0xccd0,1,0x26);
		WriteCmdDataIPS_4_16(0xccd1,14,0x00);
		WriteCmdDataIPS_4_16(0x3A00,1,0x55); // Colour 565
		}


		WriteSSD1963Command(0x1100); uSec( 100000);//delay(100);
		WriteSSD1963Command(0x2900); uSec( 100000);//delay(50);

		//It is a natural PORTrait LCD, but we will fudge to make it
		//work for 1=landscape,2=portrait 3=RLandscape,4=RPortrait

        #define OTM8009A_MADCTL_MY  0x80
        #define OTM8009A_MADCTL_MX  0x40
        #define OTM8009A_MADCTL_MV  0x20

		int i=0;
		switch(Option.DISPLAY_ORIENTATION) {
		   	case LANDSCAPE:     i=OTM8009A_MADCTL_MX | OTM8009A_MADCTL_MV; break;
		   	case PORTRAIT:      i=0x0 ; break;
		   	case RLANDSCAPE:    i=OTM8009A_MADCTL_MY | OTM8009A_MADCTL_MV; break;
		   	case RPORTRAIT:     i=OTM8009A_MADCTL_MX | OTM8009A_MADCTL_MY; break;
		}


		WriteCmdDataIPS_4_16(0x3600,1,i);   //set Memory Access Control
		ClearScreen(Option.DefaultBC);
		//LCD_BL_Period=2;

}
void InitSSD1963(void) {
	int myDisplayHRes=DisplayHRes;
	//LCD_BL_Period=100-LCD_BL_Period;
    // IMPORTANT: At this stage the SSD1963 is running at a slow speed and cannot respond to high speed commands
    //            So we use slow speed versions of WriteSSD1963Command/WriteDataSSD1963 with a 3 uS delay between each control signal change
    
	// Set MN(multipliers) of PLL, VCO = crystal freq * (N+1)
	// PLL freq = VCO/M with 250MHz < VCO < 800MHz
	// The max PLL freq is around 120MHz. To obtain 120MHz as the PLL freq
    
	WriteSSD1963Command(CMD_SET_PLL_MN);	                            // Set PLL with OSC = 10MHz (hardware), command is 0xE3
	WriteDataSSD1963(0x23);				                            // Multiplier N = 35, VCO (>250MHz)= OSC*(N+1), VCO = 360MHz
	WriteDataSSD1963(0x02);				                            // Divider M = 2, PLL = 360/(M+1) = 120MHz
	WriteDataSSD1963(0x54);				                            // Validate M and N values

	WriteSSD1963Command(CMD_PLL_START);	                            // Start PLL command
	WriteDataSSD1963(0x01);				                            // enable PLL

	uSec(10000);						                                // wait stablize
	WriteSSD1963Command(CMD_PLL_START);	                            // Start PLL command again
	WriteDataSSD1963(0x03);				                            // now, use PLL output as system clock

	WriteSSD1963Command(CMD_SOFT_RESET);	                            // Soft reset
	uSec(10000);

#define parallel_write_data WriteDataSSD1963
#define TFT_Write_Data WriteDataSSD1963

	// Configure for the TFT panel, varies from individual manufacturer
	WriteSSD1963Command(CMD_SET_PCLK);		                            // set pixel clock (LSHIFT signal) frequency
	WriteDataSSD1963(SSD1963PClock1);                                  // paramaters set by DISPLAY INIT
	WriteDataSSD1963(SSD1963PClock2);
	WriteDataSSD1963(SSD1963PClock3);
    //	uSec(10000);

    
	// Set panel mode, varies from individual manufacturer
	WriteSSD1963Command(CMD_SET_PANEL_MODE);
	WriteDataSSD1963(SSD1963Mode1);                                        // parameters set by DISPLAY INIT
	WriteDataSSD1963(SSD1963Mode2);
	WriteDataSSD1963((myDisplayHRes - 1) >> 8);                              //Set panel size
	WriteDataSSD1963(myDisplayHRes - 1);
	WriteDataSSD1963((DisplayVRes - 1) >> 8);
	WriteDataSSD1963(DisplayVRes - 1);
	WriteDataSSD1963(0x00);				                                //RGB sequence


	// Set horizontal period
	WriteSSD1963Command(CMD_SET_HOR_PERIOD);
	#define HT (myDisplayHRes + SSD1963HorizPulseWidth + SSD1963HorizBackPorch + SSD1963HorizFrontPorch)
	WriteDataSSD1963((HT - 1) >> 8);
	WriteDataSSD1963(HT - 1);
	#define HPS (SSD1963HorizPulseWidth + SSD1963HorizBackPorch)
	WriteDataSSD1963((HPS - 1) >> 8);
	WriteDataSSD1963(HPS - 1);
	WriteDataSSD1963(SSD1963HorizPulseWidth - 1);
	WriteDataSSD1963(0x00);
	WriteDataSSD1963(0x00);
	WriteDataSSD1963(0x00);
	
	// Set vertical period
	WriteSSD1963Command(CMD_SET_VER_PERIOD);
	#define VT (SSD1963VertPulseWidth + SSD1963VertBackPorch + SSD1963VertFrontPorch + DisplayVRes)
	WriteDataSSD1963((VT - 1) >> 8);
	WriteDataSSD1963(VT - 1);
	#define VSP (SSD1963VertPulseWidth + SSD1963VertBackPorch)
	WriteDataSSD1963((VSP - 1) >> 8);
	WriteDataSSD1963(VSP - 1);
	WriteDataSSD1963(SSD1963VertPulseWidth - 1);
	WriteDataSSD1963(0x00);
	WriteDataSSD1963(0x00);
	
	// Set pixel data interface
	WriteSSD1963Command(CMD_SET_DATA_INTERFACE);
    WriteDataSSD1963(SSD1963PixelInterface);                                                // 8-bit colour format
	WriteSSD1963Command(CMD_SET_PIXEL_FORMAT);
    WriteDataSSD1963(SSD1963PixelFormat);                                                // 8-bit colour format

    // initialise the GPIOs
	WriteSSD1963Command(CMD_SET_GPIO_CONF);                                // Set all GPIOs to output, controlled by host
	WriteDataSSD1963(0x0f);				                                // Set GPIO0 as output
	WriteDataSSD1963(0x01);				                                // GPIO[3:0] used as normal GPIOs

	// LL Reset to LCD!!!
	GPIO_WR(LCD_SPENA, 1);
	GPIO_WR(LCD_SPCLK, 1);
	GPIO_WR(LCD_SPDAT,1);
	GPIO_WR(LCD_RESET,1);
	GPIO_WR(LCD_RESET,0);
	uSec(1000);
	GPIO_WR(LCD_RESET,1);

    // setup the pixel write order
    WriteSSD1963Command(CMD_SET_ADDR_MODE);
    switch(Option.DISPLAY_ORIENTATION) {
        case LANDSCAPE:     WriteDataSSD1963(SSD1963_LANDSCAPE); break;
        case PORTRAIT:      WriteDataSSD1963(SSD1963_PORTRAIT); break;
        case RLANDSCAPE:    WriteDataSSD1963(SSD1963_RLANDSCAPE); break;
        case RPORTRAIT:     WriteDataSSD1963(SSD1963_RPORTRAIT); break;
    }

    // Set the scrolling area
	WriteSSD1963Command(CMD_SET_SCROLL_AREA);
	WriteDataSSD1963(0);
	WriteDataSSD1963(0);
	WriteDataSSD1963(DisplayVRes >> 8);
	WriteDataSSD1963(DisplayVRes);
	WriteDataSSD1963(0);
	WriteDataSSD1963(0);
    ScrollStart = 0;
 
	ClearScreen(Option.DefaultBC);
    SetBacklightSSD1963(Option.DefaultBrightness);
	WriteSSD1963Command(CMD_ON_DISPLAY);	                                // Turn on display; show the image on display	
}


/**********************************************************************************************
Draw a filled rectangle on the video output with physical frame buffer coordinates
     x1, y1 - the start physical frame buffer coordinate
     x2, y2 - the end physical frame buffer coordinate
     c - the colour to use for both the fill
 This is only called by DrawRectangleSSD1963() below
***********************************************************************************************/
void PhysicalDrawRect_16(int x1, int y1, int x2, int y2, int c) {
    int i;
    if(Option.DISPLAY_TYPE==ILI9341_16) {
    	WriteSSD1963Command(ILI9341_PIXELFORMAT);
    	WriteDataSSD1963(0x55); //change to RGB565 for write rectangle
    	SetAreaILI9341(x1, y1 , x2, y2, 1);
    }else if(Option.DISPLAY_TYPE==IPS_4_16) {
    	if(Option.SSDspeed==1)WriteCmdDataIPS_4_16(0x3A00,1,0x55);
    	SetAreaIPS_4_16(x1, y1 , x2, y2, 1);
    } else {
    	SetAreaSSD1963(x1, y1 , x2, y2);                                // setup the area to be filled
    	WriteSSD1963Command(CMD_WR_MEMSTART);
    }
    uint16_t j=((c>>8) & 0xf800) | ((c>>5) & 0x07e0) | ((c>>3) & 0x001f);
    for(i = (x2 - x1 + 1) * (y2 - y1 + 1); i > 0; i--){
    	  FMC_RBANK1->RAM = j;
    	  __DSB();
    }
    if(Option.SSDspeed==1)WriteCmdDataIPS_4_16(0x3A00,1,0x66);
    if(Option.DISPLAY_TYPE==ILI9341_16){
    	WriteSSD1963Command(ILI9341_PIXELFORMAT);
    	WriteDataSSD1963(0x66);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Drawing primitives used by the functions in GUI.c and Draw.c
//
////////////////////////////////////////////////////////////////////////////////////////////////////////


/**********************************************************************************************
Draw a filled rectangle on the video output with logical (MMBasic) coordinates
     x1, y1 - the start coordinate
     x2, y2 - the end coordinate
     c - the colour to use for both the fill
***********************************************************************************************/
void DrawRectangleSSD1963(int x1, int y1, int x2, int y2, int c) {
    int t;
    // make sure the coordinates are kept within the display area
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x1 < 0) x1 = 0;
    if(x1 >= HRes) x1 = HRes - 1;
    if(x2 < 0) x2 = 0;
    if(x2 >= HRes) x2 = HRes - 1;
    if(y1 < 0) y1 = 0;
    if(y1 >= VRes) y1 = VRes - 1;
    if(y2 < 0) y2 = 0;
    if(y2 >= VRes) y2 = VRes - 1;

    t = y2 - y1;                                                    // get the distance between the top and bottom

    // set y1 to the physical location in the frame buffer (only really has an effect when scrolling is in action)
    if(Option.DISPLAY_ORIENTATION == RLANDSCAPE)
        y1 = (y1 + (VRes - ScrollStart)) % VRes;
    else
        y1 = (y1 + ScrollStart) % VRes;
    y2 = y1 + t;
    if(Option.DISPLAY_TYPE > SSD_PANEL_8){
        if(y2 >= VRes) {                                                // if the box splits over the frame buffer boundary
            PhysicalDrawRect_16(x1, y1, x2, VRes - 1, c);                  // draw the top part
            PhysicalDrawRect_16(x1, 0, x2, y2 - VRes , c);                 // and the bottom part
        } else
            PhysicalDrawRect_16(x1, y1, x2, y2, c);                        // the whole box is within the frame buffer - much easier
    }
}
void DrawBufferSSD1963_16(int x1, int y1, int x2, int y2, char* p) {
   // int i,t;
	int t,toggle=0;
	unsigned int bl=0;
    int xx1=x1, yy1=y1, xx2=x2, yy2=y2, x, y;
    union colourmap
    {
    char rgbbytes[4];
    unsigned int rgb;
    } c;
    // make sure the coordinates are kept within the display area
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x1 < 0) xx1 = 0;
    if(x1 >= HRes) xx1 = HRes - 1;
    if(x2 < 0) xx2 = 0;
    if(x2 >= HRes) xx2 = HRes - 1;
    if(y1 < 0) yy1 = 0;
    if(y1 >= VRes) yy1 = VRes - 1;
    if(y2 < 0) yy2 = 0;
    if(y2 >= VRes) yy2 = VRes - 1;

    t = yy2 - yy1;                                                    // get the distance between the top and bottom

    if(Option.DISPLAY_ORIENTATION == RLANDSCAPE)
        yy1 = (yy1 + (VRes - ScrollStart)) % VRes;
    else
        yy1 = (yy1 + ScrollStart) % VRes;
    yy2 = yy1 + t;                                                    // and set y2 to the same
#ifdef notrequired
    	if(yy2 >= VRes) {
        	if(Option.DISPLAY_TYPE==ILI9341_16) {
        		SetAreaILI9341(xx1, yy1, xx2, VRes - 1, 1);
        	}else if(Option.DISPLAY_TYPE==) {
        		SetAreaIPS_4_16(xx1, yy1, xx2,IPS_4_16 VRes - 1, 1);
        	} else {
        		SetAreaSSD1963(xx1, yy1, xx2, VRes - 1); // if the box splits over the frame buffer boundary
        		WriteSSD1963Command(CMD_WR_MEMSTART);
        	}
    		for(i = (xx2 - xx1 + 1) * ((VRes - 1) - yy1 + 1); i > 0; i--){
    			c.rgbbytes[0]=*p++; //this order swaps the bytes to match the .BMP file
    			c.rgbbytes[1]=*p++;
    			c.rgbbytes[2]=*p++;
    			FMC_RBANK1->RAM = ((c.rgb>>8) & 0xf800) | ((c.rgb>>5) & 0x07e0) | ((c.rgb>>3) & 0x001f);
    			__DSB();
				}
        	if(Option.DISPLAY_TYPE==ILI9341_16) {
        		SetAreaILI9341(xx1, 0, xx2, yy2 - VRes, 1);
        	}else if(Option.DISPLAY_TYPE==IPS_4_16) {
        		SetAreaIPS_4_16(xx1, 0, xx2, yy2 - VRes, 1);
        	} else {
        		SetAreaSSD1963(xx1, 0, xx2, yy2 - VRes );
        		WriteSSD1963Command(CMD_WR_MEMSTART);
        	}
    		for(i = (xx2 - xx1 + 1) * (yy2 - VRes + 1); i > 0; i--){
    			c.rgbbytes[0]=*p++; //this order swaps the bytes to match the .BMP file
    			c.rgbbytes[1]=*p++;
    			c.rgbbytes[2]=*p++;
    			FMC_RBANK1->RAM = ((c.rgb>>8) & 0xf800) | ((c.rgb>>5) & 0x07e0) | ((c.rgb>>3) & 0x001f);
    			__DSB();
				}
    	} else {
#endif
    		// the whole box is within the frame buffer - much easier
        	if(Option.DISPLAY_TYPE==ILI9341_16) {
        		Option.SSDspeed=2;
        		SetAreaILI9341(xx1, yy1 , xx2, yy2, 1);
        	}else if(Option.DISPLAY_TYPE==IPS_4_16) {
        		SetAreaIPS_4_16(xx1, yy1 , xx2, yy2, 1);
        	} else {
        		SetAreaSSD1963(xx1, yy1 , xx2, yy2);                                // setup the area to be filled
        		WriteSSD1963Command(CMD_WR_MEMSTART);
        	}
   		for(y=y1;y<=y2;y++){
    			for(x=x1;x<=x2;x++){

    				if(x>=0 && x<HRes && y>=0 && y<VRes){

    					  c.rgbbytes[0]=*p++; //this order swaps the bytes to match the .BMP file
    					  c.rgbbytes[1]=*p++;
    					  c.rgbbytes[2]=*p++;

    					 if(Option.SSDspeed==0){
    					    FMC_RBANK1->RAM = ((c.rgb>>8) & 0xf800) | ((c.rgb>>5) & 0x07e0) | ((c.rgb>>3) & 0x001f);
    					   __DSB();
    					 }else{ //NT35510 and ILI9341_16 in RGB666
						    if(toggle==0){
						    	FMC_RBANK1->RAM = ((c.rgb>>8) & 0xf800) | ((c.rgb>>8) & 0x00fc);__DSB(); // RG
						    	bl=(c.rgb & 0x00f8); //save blue
						    	toggle=1;
						    }else{
						    	FMC_RBANK1->RAM = (bl<<8) | ((c.rgb>>16) & 0x00f8) ;__DSB();  // BR
						    	FMC_RBANK1->RAM = ((c.rgb) & 0xfc00) | ((c.rgb) & 0x00f8) ;__DSB();
						    	toggle=0;
						    }

    					 }
    				} else {
    					p+=3;
    				}
    			}
    		}
   	 if(Option.SSDspeed==2)Option.SSDspeed=0;
   	 if((Option.SSDspeed==1) && (toggle==1)){
   			// extra packet needed
   			FMC_RBANK1->RAM = (bl<<8) | ((c.rgb>>8) & 0x00f8) ; __DSB();  //
   	 }
}
void ReadBufferSSD1963_16(int x1, int y1, int x2, int y2, char* p) {
     int x, y, t, i,nr=0 ;
	int toggle=0,t1=0;
    // make sure the coordinates are kept within the display area
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    int xx1=x1, yy1=y1, xx2=x2, yy2=y2;
    if(x1 < 0) xx1 = 0;
    if(x1 >= HRes) xx1 = HRes - 1;
    if(x2 < 0) xx2 = 0;
    if(x2 >= HRes) xx2 = HRes - 1;
    if(y1 < 0) yy1 = 0;
    if(y1 >= VRes) yy1 = VRes - 1;
    if(y2 < 0) yy2 = 0;
    if(y2 >= VRes) yy2 = VRes - 1;

     t = yy2 - yy1;                                                    // get the distance between the top and bottom
    if(Option.DISPLAY_ORIENTATION == RLANDSCAPE)
        yy1 = (yy1 + (VRes - ScrollStart)) % VRes;
    else
        yy1 = (yy1 + ScrollStart) % VRes;
    yy2 = yy1 + t;                                                    // and set y2 to the same

    if(yy2 >= VRes) {      ////////////////////////////////////////////////////////////
    	if(Option.DISPLAY_TYPE==ILI9341_16) {
    		SetAreaILI9341(xx1, yy1, xx2, VRes - 1, 0);
        	t=FMC_RBANK1->RAM ; __DSB();// dummy read
    	}else if(Option.DISPLAY_TYPE==IPS_4_16) {
        	SetAreaIPS_4_16(xx1, yy1, xx2, VRes - 1, 0);
        	t=FMC_RBANK1->RAM ; __DSB();// dummy read
    	} else {
    		SetAreaSSD1963(xx1, yy1, xx2, VRes - 1); // if the box splits over the frame buffer boundary
    		WriteSSD1963Command(CMD_RD_MEMSTART);
    	}
    	t=FMC_RBANK1->RAM ; __DSB();
        *p++=(t & 0x1F)<<3; *p++=(t & 0x7e0)>>3; *p++=(t & 0xf800)>>8;
        i=(xx2 - xx1 + 1) * ((VRes - 1) - yy1 + 1);
        for( ; i > 1; i--){
        	t=FMC_RBANK1->RAM ; __DSB();
            *p++=(t & 0x1F)<<3; *p++=(t & 0x7e0)>>3; *p++=(t & 0xf800)>>8;
        }
    	if(Option.DISPLAY_TYPE==ILI9341_16) {
    		SetAreaILI9341(xx1, 0, xx2, yy2 - VRes ,0 );
    		t=FMC_RBANK1->RAM ; // dummy read
    	}else if(Option.DISPLAY_TYPE==IPS_4_16) {
    		SetAreaIPS_4_16(xx1, 0, xx2, yy2 - VRes ,0 );
    		t=FMC_RBANK1->RAM ; // dummy read
    	} else {
    		SetAreaSSD1963(xx1, 0, xx2, yy2 - VRes );
    		WriteSSD1963Command(CMD_RD_MEMSTART);
    	}
    	t=FMC_RBANK1->RAM ; __DSB();
        *p++=(t & 0x1F)<<3; *p++=(t & 0x7e0)>>3; *p++=(t & 0xf800)>>8;
         for(i = (xx2 - xx1 + 1) * (yy2 - VRes + 1); i > 1; i--){
        	t=FMC_RBANK1->RAM ; __DSB();
            *p++=(t & 0x1F)<<3; *p++=(t & 0x7e0)>>3; *p++=(t & 0xf800)>>8;
        }
    } else {
    	// the whole box is within the frame buffer - much easier
    	if(Option.DISPLAY_TYPE==ILI9341_16) {
    		//WriteSSD1963Command(ILI9341_PIXELFORMAT);
    		//WriteDataSSD1963(0x66); //change to RGB666 for read
    		SetAreaILI9341(xx1, yy1 , xx2, yy2, 0);
    		t=FMC_RBANK1->RAM ; // dummy read
    	}else if(Option.DISPLAY_TYPE==IPS_4_16) {
    		SetAreaIPS_4_16(xx1, yy1 , xx2, yy2, 0);
    		t=FMC_RBANK1->RAM ; // dummy read
    	} else {
    		SetAreaSSD1963(xx1, yy1 , xx2, yy2);                                // setup the area to be filled
    		WriteSSD1963Command(CMD_RD_MEMSTART);
    	}
        for(y=y1;y<=y2;y++){
            for(x=x1;x<=x2;x++){
                if(x>=0 && x<HRes && y>=0 && y<VRes){
                	 if(Option.DISPLAY_TYPE==ILI9341_16) {
                	      if(toggle==0){
                	         t=(FMC_RBANK1->RAM); __DSB();
                	         t<<=8 ;
                	         t1=FMC_RBANK1->RAM ; __DSB();
                	         t|=(t1>>8);
                	         t1 &=0xFF;
                	         toggle=1;
                	      } else {
                	         t=FMC_RBANK1->RAM; __DSB();
                	       	t|= (t1<<16);
                	        toggle=0;
                	      }
                	      *p++=(t & 0xf8); *p++=(t & 0xfc00)>>8; *p++=(t & 0xf80000)>>16;

                	}else if(Option.DISPLAY_TYPE==IPS_4_16) {
                		if(toggle==0){     //RGBR 8bit each
                		  	t=(FMC_RBANK1->RAM); __DSB(); // RG
                		   	t1=FMC_RBANK1->RAM ; __DSB(); // BR

                		    *p++=(t1 & 0xF800)>>8;  //BLUE
                		    *p++=(t & 0xFC);      //GREEN
                			*p++=(t & 0xF800)>>8;   //RED

                		    nr=(t1 & 0xF8);       //save next red

                		    if(Option.SSDspeed==1){  //NT35510 does not need toggle=1
                		      toggle=0;
                		    }else{
                		      toggle=1;
                		    }
                		} else {

                		 	t=FMC_RBANK1->RAM; __DSB(); //get the second  GB
                		 	*p++=(t & 0xFB);       //Blue
                		 	*p++=(t & 0xFC00)>>8;  //Green   FIX  HERE
                		 	*p++=nr ;              //add the red

                		  	toggle=0;
                		}


                	} else {
                    	t=FMC_RBANK1->RAM ; __DSB();
                        *p++=(t & 0x1F)<<3; *p++=(t & 0x7e0)>>3; *p++=(t & 0xf800)>>8;
                	}
                } else p+=3; //don't read data for pixels outside the screen
            }
        }
       // if(Option.DISPLAY_TYPE==ILI9341_16){
             	//WriteSSD1963Command(ILI9341_PIXELFORMAT);
              	//WriteDataSSD1963(0x55); //change to RDB565 for write
       // }
    } ////////////////////////////////////////////////////////////////////////////////
}void fun_getscanline(void){
	if(Option.DISPLAY_TYPE == ILI9341 || Option.DISPLAY_TYPE == ILI9481) error("Invalid on this display");
	int t;
	int x=0,c;
		getargs(&ep, 3,",");
		if(argc == 0){
			    if(Option.DISPLAY_TYPE == IPS_4_16){
					WriteSSD1963Command(0x4500 );
					t=FMC_RBANK1->RAM ; // dummy read
					t=(FMC_RBANK1->RAM <<8);
					if(Option.SSDspeed==1 && x==0){
						WriteSSD1963Command(0x4501 );
						x=FMC_RBANK1->RAM ; // dummy read
					}
					t|=FMC_RBANK1->RAM;
				}else if(Option.DISPLAY_TYPE == ILI9341_16){
					WriteSSD1963Command(CMD_GET_SCANLINE);
					t=FMC_RBANK1->RAM ; // dummy read
					t=(FMC_RBANK1->RAM <<8);
					t|=FMC_RBANK1->RAM;
				}else{
					WriteSSD1963Command(CMD_GET_SCANLINE);
					t=(FMC_RBANK1->RAM <<8);
					t|=FMC_RBANK1->RAM;
			   }

		}else{
			t=0;
			x = getinteger(argv[0]);
			if (x==1)c=0xDA;	//Read ID1
			if (x==2)c=0xDB;	//Read ID2
			if (x==3)c=0xDC;	//Read ID3
			if(Option.DISPLAY_TYPE == IPS_4_16){
					WriteSSD1963Command(c<<8 );
					x=FMC_RBANK1->RAM ; // dummy read
					t=(FMC_RBANK1->RAM);

			}else if(Option.DISPLAY_TYPE == ILI9341_16){
					WriteSSD1963Command(c);
					t=FMC_RBANK1->RAM ; // dummy read
					t=(FMC_RBANK1->RAM );
			}

		}

	iret=t;
	targ = T_INT;
}

/***********************************************************************************************
Print the bitmap of a char on the video output
	x1, y1 - the top left of the char
    width, height - size of the char's bitmap
    scale - how much to scale the bitmap
	fg, bg - foreground and background colour
    bitmap - pointer to the bitmap
***********************************************************************************************/
void DrawBitmapSSD1963_16(int x1, int y1, int width, int height, int scale, int fg, int bg, unsigned char *bitmap ){
    int i, j, k, m, n, y, yt;
    int vertCoord, horizCoord, XStart, XEnd;//, fg16, bg16,ig16;
    int toggle=0;
    unsigned int p1=0,bl=0,fg16, bg16,ig16;
    char *buff;
    union car
    {
    char rgbbytes[4];
    unsigned int rgb;
    } c;
    buff=NULL;
    if(x1>=HRes || y1>=VRes || x1+width*scale<0 || y1+height*scale<0)return;
    // adjust when part of the bitmap is outside the displayable coordinates
    vertCoord = y1; if(y1 < 0) y1 = 0;                                 // the y coord is above the top of the screen
    XStart = x1; if(XStart < 0) XStart = 0;                            // the x coord is to the left of the left marginn
    XEnd = x1 + (width * scale) - 1; if(XEnd >= HRes) XEnd = HRes - 1; // the width of the bitmap will extend beyond the right margin
    if(bg==-1){
        buff=GetMemory(width * height * scale * scale * 3 );
        ReadBuffer(XStart, y1, XEnd, (y1 + (height * scale) - 1), buff);
        n=0;
    }
    fg16=((fg>>8) & 0xf800) | ((fg>>5) & 0x07e0) | ((fg>>3) & 0x001f);
    bg16=((bg>>8) & 0xf800) | ((bg>>5) & 0x07e0) | ((bg>>3) & 0x001f);
    // set y and yt to the physical location in the frame buffer (only is important when scrolling is in action)
    if(Option.DISPLAY_ORIENTATION == RLANDSCAPE) yt = y = (y1 + (VRes - ScrollStart)) % VRes;
    else yt = y = (y1 + ScrollStart) % VRes;
    
	if(Option.DISPLAY_TYPE==ILI9341_16) {
		Option.SSDspeed=2;
		SetAreaILI9341(XStart, y, XEnd, (y + (height * scale) - 1)  % VRes, 1);
	}else if(Option.DISPLAY_TYPE==IPS_4_16 ) {
		SetAreaIPS_4_16(XStart, y, XEnd, (y + (height * scale) - 1)  % VRes, 1);
	} else {
		SetAreaSSD1963(XStart, y, XEnd, (y + (height * scale) - 1)  % VRes);
		WriteSSD1963Command(CMD_WR_MEMSTART);
	}
#ifdef SSDspeed
	if(Option.SSDspeed){
		for(i = 0; i < height; i++) {                                   // step thru the font scan line by line
			for(j = 0; j < scale; j++) {                                // repeat lines to scale the font
				if(vertCoord++ < 0) continue;                           // we are above the top of the screen
				if(vertCoord > VRes){
					if(buff!=NULL)FreeMemory(buff);
					return;                            // we have extended beyond the bottom of the screen
				}
				// if we have scrolling in action we could run over the end of the frame buffer
				// if so, terminate this area and start a new one at the top of the frame buffer
				if(y++ == VRes) {
					//if(Option.DISPLAY_TYPE==ILI9341_16) {
					//	SetAreaILI9341(XStart, 0, XEnd, ((yt + (height * scale) - 1)  % VRes) - y, 1);
					//}else if(Option.DISPLAY_TYPE==IPS_4_16 ) {
					//	SetAreaIPS_4_16(XStart, 0, XEnd, ((yt + (height * scale) - 1)  % VRes) - y, 1);
					//} else {
						SetAreaSSD1963(XStart, 0, XEnd, ((yt + (height * scale) - 1)  % VRes) - y);
						WriteSSD1963Command(CMD_WR_MEMSTART);
					//}
				}
				horizCoord = x1;
				for(k = 0; k < width; k++) {                            // step through each bit in a scan line
					for(m = 0; m < scale; m++) {                        // repeat pixels to scale in the x axis
						if(horizCoord++ < 0) continue;                  // we have not reached the left margin
						if(horizCoord > HRes) continue;                 // we are beyond the right margin
						if((bitmap[((i * width) + k)/8] >> (((height * width) - ((i * width) + k) - 1) %8)) & 1) {
							} else {
							if(buff!=NULL){
								c.rgbbytes[0]=buff[n];
								c.rgbbytes[1]=buff[n+1];
								c.rgbbytes[2]=buff[n+2];
								ig16=((c.rgb>>8) & 0xf800) | ((c.rgb>>5) & 0x07e0) | ((c.rgb>>3) & 0x001f);
								FMC_RBANK1->RAM = (uint16_t)ig16;
								__DSB();
								} else {
								FMC_RBANK1->RAM = (uint16_t)bg16;
						  	  __DSB();
								}
						}
						n+=3;
					}
				}
			}
		}
	} else {  //ONLY NEED THIS//////////////////////////////////////////////////////
#endif

		for(i = 0; i < height; i++) {                                   // step thru the font scan line by line
			for(j = 0; j < scale; j++) {                                // repeat lines to scale the font
				if(vertCoord++ < 0) continue;                           // we are above the top of the screen
				if(vertCoord > VRes){
					if(buff!=NULL)FreeMemory(buff);
					return;                            // we have extended beyond the bottom of the screen
				}
				// if we have scrolling in action we could run over the end of the frame buffer
				// if so, terminate this area and start a new one at the top of the frame buffer
				if(y++ == VRes) {
					if(Option.DISPLAY_TYPE==ILI9341_16) {
						Option.SSDspeed=2;
						SetAreaILI9341(XStart, 0, XEnd, ((yt + (height * scale) - 1)  % VRes) - y, 1);
					}else if(Option.DISPLAY_TYPE==IPS_4_16) {
						SetAreaIPS_4_16(XStart, 0, XEnd, ((yt + (height * scale) - 1)  % VRes) - y, 1);
					} else {
						SetAreaSSD1963(XStart, 0, XEnd, ((yt + (height * scale) - 1)  % VRes) - y);
						WriteSSD1963Command(CMD_WR_MEMSTART);
					}
				}
				horizCoord = x1;
				for(k = 0; k < width; k++) {                            // step through each bit in a scan line
					for(m = 0; m < scale; m++) {                        // repeat pixels to scale in the x axis
						if(horizCoord++ < 0) continue;                  // we have not reached the left margin
						if(horizCoord > HRes) continue;                 // we are beyond the right margin
						if((bitmap[((i * width) + k)/8] >> (((height * width) - ((i * width) + k) - 1) %8)) & 1) {
                                p1=fg16;


						} else {
							 if(buff!=NULL){
								c.rgbbytes[0]=buff[n];
								c.rgbbytes[1]=buff[n+1];
								c.rgbbytes[2]=buff[n+2];
								ig16=((c.rgb>>8) & 0xf800) | ((c.rgb>>5) & 0x07e0) | ((c.rgb>>3) & 0x001f);
								p1=ig16;

							 } else {
								p1=bg16;
							 }
						}
						if(Option.SSDspeed==0){
							FMC_RBANK1->RAM = p1; __DSB();
						}else{
						     if(toggle==0){

                                  FMC_RBANK1->RAM = (p1  & 0xf800) | ((p1>>3) & 0xfc);
                                  __DSB(); // RG
								  bl=((p1<<3) & 0xf8); //save blue
								  toggle=1;
						      }else{

                                  FMC_RBANK1->RAM =((bl<<8 ) & 0xf800) | ((p1>>8) & 0xf8) ; // BR
                                  __DSB();
								  FMC_RBANK1->RAM =((p1<<5 ) & 0xfc00) | ((p1<<3) & 0xf8) ; // GB
								  __DSB();
								 toggle=0;
							}
						}
						n+=3;
					}

				}
			}
		}
       if(Option.SSDspeed==2)Option.SSDspeed=0;
	   if((Option.SSDspeed==1) && (toggle==1)){
		// extra packet needed
		//p2= (((bl<<8 ) & 0xf800) | ((p1>>8) & 0xf8));
		FMC_RBANK1->RAM = ((bl<<8 ) & 0xf800) | ((p1>>8) & 0xf8);  // BR
		__DSB();
	   }
	   if(buff!=NULL)FreeMemory(buff);
}

/**********************************************************************************************
 Scroll the image by a number of scan lines
 Will only work in landscape or reverse landscape
 lines - the number of scan lines to scroll
        if positive the display will scroll up
        if negative it will scroll down
***********************************************************************************************/
void ScrollSSD1963(int lines) {
    int t;
    t = ScrollStart;

    if(lines >= 0) {
        DrawRectangle(0, 0, HRes - 1, lines - 1, gui_bcolour); // erase the line to be scrolled off
        while(lines--) {
            if(Option.DISPLAY_ORIENTATION == LANDSCAPE) {
                if(++t >= VRes) t = 0;
            } else if(Option.DISPLAY_ORIENTATION == RLANDSCAPE) {
                if(--t < 0) t = VRes - 1;
            }
        }
    } else {
        while(lines++) {
            if(Option.DISPLAY_ORIENTATION == LANDSCAPE) {
                if(--t < 0) t = VRes - 1;
            } else if(Option.DISPLAY_ORIENTATION == RLANDSCAPE) {
                if(++t >= VRes) t = 0;
            }
        }
        DrawRectangle(0, 0, HRes - 1, lines - 1, gui_bcolour); // erase the line introduced at the top
    }
	WriteSSD1963Command(CMD_SET_SCROLL_START);
	WriteDataSSD1963(t >> 8);
	WriteDataSSD1963(t);

    ScrollStart = t;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions used to support the use of a SSD1963 display as the console
//
////////////////////////////////////////////////////////////////////////////////////////////////////////




/**********************************************************************************************
 Control the cursor
 This does all the work of displaying, flashing and removing the cursor.

 It is called by FullScreenEditor() and MMgetchar()
 Both of these functions keep calling ShowCursor() with the argument true while they are in a
 loop waiting for a character - this function will then display and flash the cursor.   When
 a character is received these functions will call ShowCursor() with the argument false which
 is the signal to remove the cursor from the screen.

 Note that CursorTimer is incremented by the millisecond interrupt
***********************************************************************************************/
void ShowCursor(int show) {
	static int visible = false;
    int newstate;

    if(!Option.DISPLAY_CONSOLE) return;
	newstate = ((CursorTimer <= CURSOR_ON) && show);                // what should be the state of the cursor?
	if(visible == newstate) return;									// we can skip the rest if the cursor is already in the correct state
	visible = newstate;                                             // draw the cursor BELOW the font
    if(gui_font_height!=10)DrawLine(CurrentX, CurrentY + gui_font_height, CurrentX + gui_font_width, CurrentY + gui_font_height, 2, visible ? gui_fcolour : gui_bcolour);
    else DrawLine(CurrentX, CurrentY + gui_font_height-2, CurrentX + gui_font_width, CurrentY + gui_font_height-2, 2, visible ? gui_fcolour : gui_bcolour);
}



/******************************************************************************************
 Print a char on the LCD display (SSD1963 and in landscape only).  It handles control chars
 such as newline and will wrap at the end of the line and scroll the display if necessary.

 The char is printed at the current location defined by CurrentX and CurrentY
 *****************************************************************************************/
void DisplayPutC(char c) {

    if(!BasicRunning || !Option.DISPLAY_CONSOLE) return;

    // if it is printable and it is going to take us off the right hand end of the screen do a CRLF
    if(c >= FontTable[gui_font >> 4][2] && c < FontTable[gui_font >> 4][2] + FontTable[gui_font >> 4][3]) {
        if(CurrentX + gui_font_width > HRes) {
            DisplayPutC('\r');
            DisplayPutC('\n');
        }
    }

    // handle the standard control chars
    switch(c) {
        case '\b':  CurrentX -= gui_font_width;
                    if(CurrentX < 0) CurrentX = 0;
                    return;
        case '\r':  CurrentX = 0;
                    return;
        case '\n':  CurrentY += gui_font_height;
                    if(CurrentY + gui_font_height >= VRes) {
                        ScrollLCD(CurrentY + gui_font_height - VRes);
                        CurrentY -= (CurrentY + gui_font_height - VRes);
                    }
                    return;
        case '\t':  do {
                        DisplayPutC(' ');
                    } while((CurrentX/gui_font_width) % Option.Tab);
                    return;
    }
    GUIPrintChar(gui_font, gui_fcolour, gui_bcolour, c, ORIENT_NORMAL);            // print it
}



/******************************************************************************************
 Print a string on the LCD display (SSD1963 and in landscape only).   It handles control
 chars such as newline and will wrap at the end of the line and scroll the display if
 necessary.

 The string is printed at the current location defined by CurrentX and CurrentY
*****************************************************************************************/
void DisplayPutS(char *s) {
    while(*s) DisplayPutC(*s++);
}

void ScrollILI9341(int lines){
	if(lines>0){
		DoBlit(0, lines, 0, 0, HRes, VRes-lines);
		DrawRectangle(0, VRes-lines, HRes - 1, VRes - 1, gui_bcolour); // erase the line to be scrolled off
    } else if(lines<0){
    	lines=-lines;
		DoBlit(0, 0, 0, lines, HRes, VRes-lines);
        DrawRectangle(0, 0, HRes - 1, lines - 1, gui_bcolour); // erase the line to be scrolled off
    }
}
