/***********************************************************************************************************************
MMBasic

LCDDriver.c

This is the driver for SPI LCDs in MMBasic.
The core SPI LCD driver was written and developed by Peter Mather of the Back Shed Forum (http://www.thebackshed.com/forum/forum_topics.asp?FID=16)

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

#include <stdarg.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#define GenSPI hspi2


void DefineRegionSPI(int xstart, int ystart, int xend, int yend, int rw);
void DrawBitmapSPI(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap);
int CurrentSPISpeed=NONE_SPI_SPEED;
extern SPI_HandleTypeDef GenSPI;
#define SPIsend(a) {uint8_t b=a;HAL_SPI_Transmit(&GenSPI,&b,1,500);}
#define SPIqueue(a) {HAL_SPI_Transmit(&GenSPI,a,2,500);}
#define SPIsend2(a) {SPIsend(0);SPIsend(a);}


// utility function for routines that want to reserve a pin for special I/O
// this ignores any previous settings and forces the pin to its new state
// pin is the pin number
// inp is true if an input or false if an output
// init is the value used to initialise the pin if it is an output (hi or lo)
// type is the final tag for the pin in ExtCurrentConfig[]
void MIPS16 SetAndReserve(int pin, int inp, int init, int type) {
    if(pin == 0) return;                                            // do nothing if not set
    GPIO_InitTypeDef GPIO_InitDef;
    if(inp) {
		GPIO_InitDef.Mode = GPIO_MODE_INPUT;
    } else {
        PinSetBit(pin, init ? LATSET : LATCLR);                     // set LAT
    	GPIO_InitDef.Mode = GPIO_MODE_OUTPUT_PP;
    }
	GPIO_InitDef.Pull = GPIO_NOPULL; //set as input with no pullup or down
	GPIO_InitDef.Pin = PinDef[pin].bitnbr;
	GPIO_InitDef.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(PinDef[pin].sfr, &GPIO_InitDef);
    ExtCurrentConfig[pin] = type;
}


void MIPS16 ConfigDisplaySPI(char *p) {
	int p1, p2, p3;
	char code;
    getargs(&p, 9, ",");
    if(!(argc == 7 || argc == 9)) error("Argument count or display type");

    if(checkstring(argv[0], "ILI9341")) {
        Option.DISPLAY_TYPE = ILI9341;
    } else if(checkstring(argv[0], "ILI9481")) {
        Option.DISPLAY_TYPE = ILI9481;
	} else
        error("Invalid display type");

    if(checkstring(argv[2], "L") || checkstring(argv[2], "LANDSCAPE"))
        Option.DISPLAY_ORIENTATION = LANDSCAPE;
    else if(checkstring(argv[2], "P") || checkstring(argv[2], "PORTRAIT"))
        Option.DISPLAY_ORIENTATION = PORTRAIT;
    else if(checkstring(argv[2], "RL") || checkstring(argv[2], "RLANDSCAPE"))
        Option.DISPLAY_ORIENTATION = RLANDSCAPE;
    else if(checkstring(argv[2], "RP") || checkstring(argv[2], "RPORTRAIT"))
        Option.DISPLAY_ORIENTATION = RPORTRAIT;
    else error("Orientation");
	if((code=codecheck(argv[4])))argv[4]+=2;
	p1 = getinteger(argv[4]);
	if(code)p1=codemap(code, p1);
	if((code=codecheck(argv[6])))argv[6]+=2;
	p2 = getinteger(argv[6]);
	if(code)p2=codemap(code, p2);
    CheckPin(p1, CP_IGNORE_INUSE);
    CheckPin(p2, CP_IGNORE_INUSE);
    if(argc == 9) {
    	if((code=codecheck(argv[8])))argv[8]+=2;
    	p3 = getinteger(argv[8]);
    	if(code)p3=codemap(code, p3);
        CheckPin(p3, CP_IGNORE_INUSE);
        Option.LCD_CS = p3;
    } else
        Option.LCD_CS = 0;

    Option.LCD_CD = p1;
    Option.LCD_Reset = p2;

    Option.TOUCH_XZERO = TOUCH_NOT_CALIBRATED;                      // record the touch feature as not calibrated
    InitDisplaySPI(1);
}



// initialise the display controller
// this is used in the initial boot sequence of the Micromite
void MIPS16 InitDisplaySPI(int fullinit) {

	if(!(Option.DISPLAY_TYPE == ILI9341 || Option.DISPLAY_TYPE == ILI9481 || Option.DISPLAY_TYPE == ILI9163 || Option.DISPLAY_TYPE == ST7735)) return;
    if(fullinit) {
        SetAndReserve(Option.LCD_CD, P_OUTPUT, 1, EXT_BOOT_RESERVED);                            // config data/command as an output
        SetAndReserve(Option.LCD_Reset, P_OUTPUT, 1, EXT_BOOT_RESERVED);                         // config reset as an output
        if(Option.LCD_CS) SetAndReserve(Option.LCD_CS, P_OUTPUT, 1, EXT_BOOT_RESERVED);          // config chip select as an output

        // open the SPI port and reserve the I/O pins
        OpenSpiChannel();

        // setup the pointers to the drawing primitives
        DrawRectangle = DrawRectangleSPI;
        DrawBitmap = DrawBitmapSPI;
        DrawBuffer = DrawBufferSPI;
        if(Option.DISPLAY_TYPE == ILI9341)ReadBuffer = ReadBufferSPI;
    }

    // the parameters for the display panel are set here
    // the initialisation sequences and the SPI driver code was written by Peter Mather (matherp on The Back Shed forum)
    switch(Option.DISPLAY_TYPE) {

    case ILI9481:
        DisplayHRes = 480;
        DisplayVRes = 320;
        ResetController();
        spi_write_command(0x11);
        uSec(20000);
        spi_write_cd(0xD0,3,0x07,0x42,0x18);
        spi_write_cd(0xD1,3,0x00,0x07,0x10);
        spi_write_cd(0xD2,2,0x01,0x02);
        spi_write_cd(0xC0,5,0x10,0x3B,0x00,0x02,0x11);
//            spi_write_cd(0xC1, 3,0x10, 0x12, 0xC8);
//            spi_write_cd(0xC5,1,0x01);
        spi_write_cd(0xB3,4,0x00,0x00,0x00,0x10);
        spi_write_cd(0xC8,12,0x00,0x32,0x36,0x45,0x06,0x16,0x37,0x75,0x77,0x54,0x0C,0x00);
        spi_write_cd(0xE0,15,0x0f,0x24,0x1c,0x0a,0x0f,0x08,0x43,0x88,0x03,0x0f,0x10,0x06,0x0f,0x07,0x00);
        spi_write_cd(0xE1,15,0x0F,0x38,0x30,0x09,0x0f,0x0f,0x4e,0x77,0x3c,0x07,0x10,0x05,0x23,0x1b,0x00);
        spi_write_cd(0x36,0x0A);
        spi_write_cd(0x3A,1,0x55);
        spi_write_cd(0x2A,4,0x00,0x00,0x01,0x3F);
        spi_write_cd(0x2B,4,0x00,0x00,0x01,0xE0);

        uSec(120000);
        spi_write_command(0x29);
        switch(Option.DISPLAY_ORIENTATION) {
            case LANDSCAPE:     spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Landscape); break;
            case PORTRAIT:      spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Portrait); break;
            case RLANDSCAPE:    spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Landscape180); break;
            case RPORTRAIT:     spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Portrait180); break;
        }
        break;
        case ILI9341:
		DisplayHRes = 320;
		DisplayVRes = 240;
		ResetController();
		spi_write_command(ILI9341_DISPLAYOFF);
		spi_write_cd(ILI9341_POWERCONTROL1,1,0x23);
		spi_write_cd(ILI9341_POWERCONTROL2,1,0x10);
		spi_write_cd(ILI9341_VCOMCONTROL1,2,0x2B,0x2B);
		spi_write_cd(ILI9341_VCOMCONTROL2,1,0xC0);
		spi_write_cd(ILI9341_PIXELFORMAT,1,0x55);
		spi_write_cd(ILI9341_FRAMECONTROL,2,0x00,0x1B);
		spi_write_cd(ILI9341_ENTRYMODE,1,0x07);
		spi_write_cd(ILI9341_SLEEPOUT,1,0);
		uSec(50000);
		spi_write_command(ILI9341_NORMALDISP);
		spi_write_command(ILI9341_DISPLAYON);
		uSec(100000);
		switch(Option.DISPLAY_ORIENTATION) {
        	case LANDSCAPE:     spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Landscape); break;
        	case PORTRAIT:      spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Portrait); break;
        	case RLANDSCAPE:    spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Landscape180); break;
        	case RPORTRAIT:     spi_write_cd(ILI9341_MEMCONTROL,1,ILI9341_Portrait180); break;
		}
		break;
    }

    if(Option.DISPLAY_ORIENTATION & 1) {
        VRes=DisplayVRes;
        HRes=DisplayHRes;
    } else {
        VRes=DisplayHRes;
        HRes=DisplayVRes;
    }

    ResetDisplay();
    ClearScreen(gui_bcolour);
}


// set Chip Select for the LCD low
// this also checks the configuration of the SPI channel and if required reconfigures it to suit the LCD controller
void set_cs(void) {
    SpiCsLow(Option.LCD_CS, LCD_SPI_SPEED);
}



void spi_write_data(unsigned char data){
    PinSetBit(Option.LCD_CD, LATSET);
    set_cs();
    if(Option.DISPLAY_TYPE == ILI9481)	{SPIsend2(data);}
    else {SPIsend(data);}
    SpiCsHigh(Option.LCD_CS);
}


void spi_write_command(unsigned char data){
    PinSetBit(Option.LCD_CD, LATCLR);
    set_cs();
    if(Option.DISPLAY_TYPE == ILI9481)	{SPIsend2(data);}
    else {SPIsend(data);}
    SpiCsHigh(Option.LCD_CS);
}


void spi_write_cd(unsigned char command, int data, ...){
   int i;
   va_list ap;
   va_start(ap, data);
   spi_write_command(command);
   for(i = 0; i < data; i++) spi_write_data((char)va_arg(ap, int));
   va_end(ap);
}


void MIPS16 ResetController(void){
//    PinSetBit(Option.LCD_Reset, LATSET);
    uSec(100000);
    PinSetBit(Option.LCD_Reset, LATCLR);
    uSec(100000);
    PinSetBit(Option.LCD_Reset, LATSET);
    uSec(200000);
    spi_write_command(ILI9341_SOFTRESET);                           //software reset
    uSec(200000);
}


void DefineRegionSPI(int xstart, int ystart, int xend, int yend, int rw) {
    if(HRes == 0) error("Display not configured");
    if(Option.DISPLAY_TYPE == ILI9481){
    	if(rw) set_cs();
    	PinSetBit(Option.LCD_CD, LATCLR);
    	SPIsend2(ILI9341_COLADDRSET);
    	PinSetBit(Option.LCD_CD, LATSET);
    	SPIsend2(xstart >> 8);
    	SPIsend2(xstart);
    	SPIsend2(xend >> 8);
    	SPIsend2(xend);
    	PinSetBit(Option.LCD_CD, LATCLR);
    	SPIsend2(ILI9341_PAGEADDRSET);
    	PinSetBit(Option.LCD_CD, LATSET);
    	SPIsend2(ystart >> 8);
    	SPIsend2(ystart);
    	SPIsend2(yend >> 8);
    	SPIsend2(yend);
    	PinSetBit(Option.LCD_CD, LATCLR);
    	if(rw) {
    		SPIsend2(ILI9341_MEMORYWRITE);
    	} else {
    		SPIsend2(ILI9341_RAMRD);
    	}
    	PinSetBit(Option.LCD_CD, LATSET);                               //set CD high
    } else {
    	if(rw) set_cs();
    	PinSetBit(Option.LCD_CD, LATCLR);
    	SPIsend(ILI9341_COLADDRSET);
    	PinSetBit(Option.LCD_CD, LATSET);
    	SPIsend(xstart >> 8);
    	SPIsend(xstart);
    	SPIsend(xend >> 8);
    	SPIsend(xend);
    	PinSetBit(Option.LCD_CD, LATCLR);
    	SPIsend(ILI9341_PAGEADDRSET);
    	PinSetBit(Option.LCD_CD, LATSET);
    	SPIsend(ystart >> 8);
    	SPIsend(ystart);
    	SPIsend(yend >> 8);
    	SPIsend(yend);
    	PinSetBit(Option.LCD_CD, LATCLR);
    	if(rw) {
    		SPIsend(ILI9341_MEMORYWRITE);
    	} else {
    		SPIsend(ILI9341_RAMRD);
    	}
    	PinSetBit(Option.LCD_CD, LATSET);                               //set CD high
    }
}

/****************************************************************************************************
 ****************************************************************************************************

 Basic drawing primitives
 all drawing on the LCD is done using either one of these two functions

 ****************************************************************************************************
****************************************************************************************************/


// Draw a filled rectangle
// this is the basic drawing promitive used by most drawing routines
//    x1, y1, x2, y2 - the coordinates
//    c - the colour
void DrawRectangleSPI(int x1, int y1, int x2, int y2, int c){
	int i,t;
    unsigned char col[2], *p;
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

    // convert the colours to 565 format
    col[0]= ((c >> 16) & 0b11111000) | ((c >> 13) & 0b00000111);
    col[1] = ((c >>  5) & 0b11100000) | ((c >>  3) & 0b00011111);

    DefineRegionSPI(x1, y1, x2, y2, 1);
    PinSetBit(Option.LCD_CD, LATSET);                               //set CD high
    set_cs();
	i = x2 - x1 + 1;
	i*=2;
	p=GetMemory(i);
	for(t=0;t<i;t+=2){p[t]=col[0];p[t+1]=col[1];}
	for(t=y1;t<=y2;t++)	HAL_SPI_Transmit(&GenSPI,p,i,500);
	FreeMemory(p);
    SpiCsHigh(Option.LCD_CS);                                       //set CS high
}


void ReadBufferSPI(int x1, int y1, int x2, int y2, char* p) {
    int r, N, t;
    unsigned char h,l;
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
    N=(x2- x1+1) * (y2- y1+1) * 3;
    spi_write_cd(ILI9341_PIXELFORMAT,1,0x66); //change to RDB666 for read
    PinSetBit(Option.LCD_CS, LATCLR);
    DefineRegionSPI(x1, y1, x2, y2, 0);
    HAL_SPI_TransmitReceive(&GenSPI,&h,&l,1,500);
    r=0;
	HAL_SPI_Receive(&GenSPI,(uint8_t *)p,N,500);

    PinSetBit(Option.LCD_CD, LATCLR);
    SpiCsHigh(Option.LCD_CS);                  //set CS high
    // revert to non enhanced SPI mode
    spi_write_cd(ILI9341_PIXELFORMAT,1,0x55); //change back to rdb565
    // getting RGB666 need to convert to BGR565
    r=0;
    while(N) {
        h=(uint8_t)p[r+2];
        l=(uint8_t)p[r];
        p[r]=(int8_t)(h & 0xF8);
        p[r+2]=(int8_t)(l & 0xF8);
        p[r+1]=p[r+1] & 0xFC;
        r+=3;
        N-=3;
    }
}

void DrawBufferSPI(int x1, int y1, int x2, int y2, char* p) {
	volatile int i, j, k, t;
    int memory;
    volatile char *q;
    union colourmap
    {
    char rgbbytes[4];
    unsigned int rgb;
    } c;
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


    DefineRegionSPI(x1, y1, x2, y2, 1);
    PinSetBit(Option.LCD_CD, LATSET);                               //set CD high
    set_cs();
	i = x2 - x1 + 1;
	i *= (y2 - y1 + 1);
	i*=2;
	// switch to SPI enhanced mode for the bulk transfer
	memory=FreeSpaceOnHeap()/2;
	if((i) < memory){
		q=GetMemory(i);
		k=0;
	    for(j = (x2 - x1 + 1) * (y2 - y1 + 1); j > 0; j--){
	        c.rgbbytes[0]=*p++; //this order swaps the bytes to match the .BMP file
	        c.rgbbytes[1]=*p++;
	        c.rgbbytes[2]=*p++;
	    // convert the colours to 565 format
	        // convert the colours to 565 format
	        q[k++]= ((c.rgb >> 16) & 0b11111000) | ((c.rgb >> 13) & 0b00000111);
	        q[k++] = ((c.rgb >>  5) & 0b11100000) | ((c.rgb >>  3) & 0b00011111);
	    }
		HAL_SPI_Transmit(&GenSPI,(uint8_t *)q,i,500);
		FreeMemory((void *)q);
	} else {
		int n;
		q=GetMemory(memory);
		n=memory;
		while(i){
			if(i>=n){
				k=0;
			    for(j = n/2; j > 0; j--){
			        c.rgbbytes[0]=*p++; //this order swaps the bytes to match the .BMP file
			        c.rgbbytes[1]=*p++;
			        c.rgbbytes[2]=*p++;
			    // convert the colours to 565 format
			        // convert the colours to 565 format
			        q[k++]= ((c.rgb >> 16) & 0b11111000) | ((c.rgb >> 13) & 0b00000111);
			        q[k++] = ((c.rgb >>  5) & 0b11100000) | ((c.rgb >>  3) & 0b00011111);
			    }
				HAL_SPI_Transmit(&GenSPI,(uint8_t *)q,n,500);
				i-=n;
			} else {
				k=0;
			    for(j = i/2; j > 0; j--){
			        c.rgbbytes[0]=*p++; //this order swaps the bytes to match the .BMP file
			        c.rgbbytes[1]=*p++;
			        c.rgbbytes[2]=*p++;
			    // convert the colours to 565 format
			        // convert the colours to 565 format
			        q[k++]= ((c.rgb >> 16) & 0b11111000) | ((c.rgb >> 13) & 0b00000111);
			        q[k++] = ((c.rgb >>  5) & 0b11100000) | ((c.rgb >>  3) & 0b00011111);
			    }
				HAL_SPI_Transmit(&GenSPI,(uint8_t *)q,i,500);
				i=0;
			}

		}
		FreeMemory((void *)q);
		MMPrintString("\r\n");

	}

    SpiCsHigh(Option.LCD_CS);                  //set CS high

    // revert to non enhanced SPI mode
}


//Print the bitmap of a char on the video output
//    x, y - the top left of the char
//    width, height - size of the char's bitmap
//    scale - how much to scale the bitmap
//	  fc, bc - foreground and background colour
//    bitmap - pointer to the bitmap
void DrawBitmapSPI(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap){
    int i, j, k, m, n;
    char f[2],b[2];
      int vertCoord, horizCoord, XStart, XEnd, YEnd;
    char *p=0;
    union colourmap {
    char rgbbytes[4];
    unsigned int rgb;
    } c;
    if(bc == -1 && Option.DISPLAY_TYPE != ILI9341) bc = 0xFFFFFF;
    if(x1>=HRes || y1>=VRes || x1+width*scale<0 || y1+height*scale<0)return;
    // adjust when part of the bitmap is outside the displayable coordinates
    vertCoord = y1; if(y1 < 0) y1 = 0;                                 // the y coord is above the top of the screen
    XStart = x1; if(XStart < 0) XStart = 0;                            // the x coord is to the left of the left marginn
    XEnd = x1 + (width * scale) - 1; if(XEnd >= HRes) XEnd = HRes - 1; // the width of the bitmap will extend beyond the right margin
    YEnd = y1 + (height * scale) - 1; if(YEnd >= VRes) YEnd = VRes - 1;// the height of the bitmap will extend beyond the bottom margin
    if(bc == -1) {                                                     //special case of overlay text
        i = 0;
        j = width * height * scale * scale * 3;
        p = GetMemory(j);                                              //allocate some temporary memory
        ReadBuffer(XStart, y1, XEnd, YEnd, p);
    }
    // convert the colours to 565 format
    f[0]= ((fc >> 16) & 0b11111000) | ((fc >> 13) & 0b00000111);
    f[1] = ((fc >>  5) & 0b11100000) | ((fc >>  3) & 0b00011111);
    b[0] = ((bc >> 16) & 0b11111000) | ((bc >> 13) & 0b00000111);
    b[1] = ((bc >>  5) & 0b11100000) | ((bc >>  3) & 0b00011111);
    DefineRegionSPI(XStart, y1, XEnd, YEnd, 1);


    PinSetBit(Option.LCD_CD, LATSET);                               //set CD high
    set_cs();
    n = 0;
    for(i = 0; i < height; i++) {                                   // step thru the font scan line by line
        for(j = 0; j < scale; j++) {                                // repeat lines to scale the font
            if(vertCoord++ < 0) continue;                           // we are above the top of the screen
            if(vertCoord > VRes) {                                  // we have extended beyond the bottom of the screen
                if(p != NULL) FreeMemory(p);
                return;
            }
            horizCoord = x1;
            for(k = 0; k < width; k++) {                            // step through each bit in a scan line
                for(m = 0; m < scale; m++) {                        // repeat pixels to scale in the x axis
                    if(horizCoord++ < 0) continue;                  // we have not reached the left margin
                    if(horizCoord > HRes) continue;                 // we are beyond the right margin
                    if((bitmap[((i * width) + k)/8] >> (((height * width) - ((i * width) + k) - 1) %8)) & 1) {
                        SPIqueue((uint8_t *)&f);
                    } else {
                        if(bc == -1){
                            c.rgbbytes[0] = p[n];
                            c.rgbbytes[1] = p[n+1];
                            c.rgbbytes[2] = p[n+2];
                            b[0] = ((c.rgb >> 16) & 0b11111000) | ((c.rgb >> 13) & 0b00000111);
                            b[1] = ((c.rgb >>  5) & 0b11100000) | ((c.rgb >>  3) & 0b00011111);
                        } 
                        SPIqueue((uint8_t *)&b);

                    }
                    n += 3;
                }
            }
        }
    }

    SpiCsHigh(Option.LCD_CS);                                       //set CS high

    // revert to non enhanced SPI mode
    if(p != NULL) FreeMemory(p);

}


// the default function for DrawRectangle() and DrawBitmap()
void DisplayNotSet(void) {
    error("Display not configured");
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// These three functions allow the SPI port to be used by multiple drivers (LCD/touch/SD card)
// The BASIC use of the SPI port does NOT use these functions
// The MX170 uses SPI channel 1 which is shared by the BASIC program
// The MX470 uses SPI channel 2 which it has exclusive control of (needed because touch can be used at any time)
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// config the SPI port for output
// it will not touch the port if it has already been opened
void MIPS16 OpenSpiChannel(void) {
    if(ExtCurrentConfig[SPI2_OUT_PIN] != EXT_BOOT_RESERVED) {
        ExtCfg(SPI2_OUT_PIN, EXT_BOOT_RESERVED, 0);
        ExtCfg(SPI2_INP_PIN, EXT_BOOT_RESERVED, 0);
        ExtCfg(SPI2_CLK_PIN, EXT_BOOT_RESERVED, 0);
        CurrentSPISpeed=NONE_SPI_SPEED;
    }
}

void SPISpeedSet(int speed){
    if(CurrentSPISpeed != speed){
        HAL_SPI_DeInit(&GenSPI);
    	if(speed==LCD_SPI_SPEED){
    		CurrentSPISpeed=LCD_SPI_SPEED;
    		GenSPI.Init.CLKPolarity = SPI_POLARITY_LOW;
    		GenSPI.Init.CLKPhase = SPI_PHASE_1EDGE;
    		GenSPI.Init.BaudRatePrescaler = (Option.DISPLAY_TYPE==ILI9481 ? SPI_BAUDRATEPRESCALER_4 : SPI_BAUDRATEPRESCALER_2);
    	} else if(speed==SD_SLOW_SPI_SPEED){
    		CurrentSPISpeed=SD_SLOW_SPI_SPEED;
    		GenSPI.Init.CLKPolarity = SPI_POLARITY_LOW;
    		GenSPI.Init.CLKPhase = SPI_PHASE_1EDGE;
    		GenSPI.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_256;
    	} else if(speed==SD_FAST_SPI_SPEED){
    		GenSPI.Init.CLKPolarity = SPI_POLARITY_LOW;
    		GenSPI.Init.CLKPhase = SPI_PHASE_1EDGE;
    		CurrentSPISpeed=SD_FAST_SPI_SPEED;
    		GenSPI.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_4;
    	} else if(speed==TOUCH_SPI_SPEED){
    		if(Option.DISPLAY_TYPE!=ILI9481){
    			GenSPI.Init.CLKPolarity = SPI_POLARITY_HIGH;
    			GenSPI.Init.CLKPhase = SPI_PHASE_2EDGE;
    			CurrentSPISpeed=TOUCH_SPI_SPEED;
    			GenSPI.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_256;
    		} else {
    			GenSPI.Init.CLKPolarity = SPI_POLARITY_LOW;
    			GenSPI.Init.CLKPhase = SPI_PHASE_1EDGE;
    			CurrentSPISpeed=TOUCH_SPI_SPEED;
    			GenSPI.Init.BaudRatePrescaler =SPI_BAUDRATEPRESCALER_64;
    		}
    	}
   	  HAL_SPI_Init(&GenSPI);
    }

}
// set the chip select for the SPI to low (enabled)
// if the SPI is currently set to a different mode or baudrate this will change it accordingly
// also, it checks if the chip select pin needs to be changed
void SpiCsLow(int pin, int speed) {
	SPISpeedSet(speed);
    if(pin) PinSetBit(pin, LATCLR);                                 // set CS low
}


// set the chip select for SPI to high (disabled)
void SpiCsHigh(int pin) {
    if(pin) PinSetBit(pin, LATSET);                                 // set CS high
}

