/*-*****************************************************************************

ArmmiteF4 MMBasic

Draw.c

Does the basic LCD display commands and drawing in MMBasic.


Copyright 2011-2023 Geoff Graham and  Peter Mather.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <float.h>

#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"


/***************************************************************************/
// define the fonts


    #include "X_8x13_LE.h"
    #include "Misc_12x20_LE.h"
    #include "Hom_16x24_LE.h"
    #include "Hom_16x24_BLD.h"
    #include "Fnt_10x16.h"
    #include "ArialNumFontPlus.h"
    #include "Font_8x6.h"

    unsigned char *FontTable[FONT_TABLE_SIZE] = {   (unsigned char *)X_8x13_LE,
                                                    (unsigned char *)Misc_12x20_LE,
                                                    (unsigned char *)Hom_16x24_LE,
													(unsigned char *)Hom_16x24_BLD,

//                                                    (unsigned char *)Inconsola,
    												(unsigned char *)Fnt_10x16,         //FONT 5 now 10x16
                                                    (unsigned char *)ArialNumFontPlus,
													(unsigned char *)F_6x8_LE,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                };


/***************************************************************************/

extern int gui_font;
extern int gui_fcolour;
extern int gui_bcolour;
int gui_reverse;
extern TIM_HandleTypeDef htim1;                                 //G.A.
void SetBacklight(int intensity);
int CurrentX, CurrentY;                                             // the current default position for the next char to be written
int DisplayHRes, DisplayVRes;                                       // the physical characteristics of the display
char * blitbuffptr[MAXBLITBUF]={NULL};											//Buffer pointers for the BLIT command
// the MMBasic programming characteristics of the display
// note that HRes == 0 is an indication that a display is not configured
int HRes = 0, VRes = 0;

// pointers to the drawing primitives
void (*DrawRectangle)(int x1, int y1, int x2, int y2, int c) = (void (*)(int , int , int , int , int ))DisplayNotSet;
void (*DrawBitmap)(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap) = (void (*)(int , int , int , int , int , int , int , unsigned char *))DisplayNotSet;
void (*ScrollLCD) (int lines) = (void (*)(int ))DisplayNotSet;
void (*DrawBuffer)(int x1, int y1, int x2, int y2, char *c) = (void (*)(int , int , int , int , char * ))DisplayNotSet;
void (*ReadBuffer)(int x1, int y1, int x2, int y2, char *c) = (void (*)(int , int , int , int , char * ))DisplayNotSet;
void DrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill);
extern void Display_Refresh(void);
void ClearTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int ints_per_line, uint32_t *br);
void DrawFilledCircle(int x, int y, int radius, int r, int fill, int ints_per_line, uint32_t *br, MMFLOAT aspect, MMFLOAT aspect2);
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
MMFLOAT pidiv2=PI_VALUE/2.0, kpar = 0.5522847498 ;


/****************************************************************************************************

 MMBasic commands and functions

****************************************************************************************************/
void Display_Refresh(void){}
void polygon(char *p, int close);


// these are the GUI commands that are common to the MX170 and MX470 versions
// in the case of the MX170 this function is called directly by MMBasic when the GUI command is used
// in the case of the MX470 it is called by MX470GUI in GUI.c
void cmd_guiMX170(void) {
    char *p;

	if(Option.DISPLAY_TYPE == 0) error("Display not configured");

    // display a bitmap stored in an integer or string
    if((p = checkstring(cmdline, "BITMAP"))) {
        int x, y, fc, bc, h, w, scale, t, bytes;
        char *s;
        MMFLOAT f;
        long long int i64;

        getargs(&p, 15, ",");
        if(!(argc & 1) || argc < 5) error("Argument count");

        // set the defaults
        h = 8; w = 8; scale = 1; bytes = 8; fc = gui_fcolour; bc = gui_bcolour;

        x = getinteger(argv[0]);
        y = getinteger(argv[2]);

        // get the type of argument 3 (the bitmap) and its value (integer or string)
        t = T_NOTYPE;
        evaluate(argv[4], &f, &i64, &s, &t, true);
        if(t & T_NBR)
            error("Invalid argument");
        else if(t & T_INT)
            s = (char *)&i64;
        else if(t & T_STR)
            bytes = *s++;

        if(argc > 5 && *argv[6]) w = getint(argv[6], 1, HRes);
        if(argc > 7 && *argv[8]) h = getint(argv[8], 1, VRes);
        if(argc > 9 && *argv[10]) scale = getint(argv[10], 1, 15);
        if(argc > 11 && *argv[12]) fc = getint(argv[12], 0, WHITE);
        if(argc == 15) bc = getint(argv[14], -1, WHITE);
        if(h * w > bytes * 8) error("Not enough data");
        DrawBitmap(x, y, w, h, scale, fc, bc, (unsigned char *)s);
        if(Option.Refresh)Display_Refresh();
        return;
    }

    if((p = checkstring(cmdline, "CALIBRATE"))) {
        int tlx, tly, trx, try, blx, bly, brx, bry, midy;
        char *s;
        if(Option.TOUCH_CS == 0) error("Touch not configured");

        if(*p && *p != '\'') {                                      // if the calibration is provided on the command line
            getargs(&p, 9, ",");
            if(argc != 9) error("Argument count");
            Option.TOUCH_SWAPXY = getinteger(argv[0]);
            Option.TOUCH_XZERO = getinteger(argv[2]);
            Option.TOUCH_YZERO = getinteger(argv[4]);
            Option.TOUCH_XSCALE = getinteger(argv[6]) / 10000.0;
            Option.TOUCH_YSCALE = getinteger(argv[8]) / 10000.0;
            SaveOptions();
            return;
        } else {
            if(CurrentLinePtr) error("Invalid in a program");
        }

        GetCalibration(TARGET_OFFSET, TARGET_OFFSET, &tlx, &tly);
        GetCalibration(HRes - TARGET_OFFSET, TARGET_OFFSET, &trx, &try);
        if(abs(trx - tlx) < CAL_ERROR_MARGIN && abs(tly - try) < CAL_ERROR_MARGIN) error("Touch hardware failure");

        GetCalibration(TARGET_OFFSET, VRes - TARGET_OFFSET, &blx, &bly);
        GetCalibration(HRes - TARGET_OFFSET, VRes - TARGET_OFFSET, &brx, &bry);
        midy = max(max(tly, try), max(bly, bry)) / 2;
        Option.TOUCH_SWAPXY = ((tly < midy && try > midy) || (tly > midy && try < midy));

        if(Option.TOUCH_SWAPXY) {
            swap(tlx, tly);
            swap(trx, try);
            swap(blx, bly);
            swap(brx, bry);
        }

        Option.TOUCH_XSCALE = (MMFLOAT)(HRes - TARGET_OFFSET * 2) / (MMFLOAT)(trx - tlx);
        Option.TOUCH_YSCALE = (MMFLOAT)(VRes - TARGET_OFFSET * 2) / (MMFLOAT)(bly - tly);
        Option.TOUCH_XZERO = ((MMFLOAT)tlx - ((MMFLOAT)TARGET_OFFSET / Option.TOUCH_XSCALE));
        Option.TOUCH_YZERO = ((MMFLOAT)tly - ((MMFLOAT)TARGET_OFFSET / Option.TOUCH_YSCALE));
        SaveOptions();
        brx = (HRes - TARGET_OFFSET) - ((brx - Option.TOUCH_XZERO) * Option.TOUCH_XSCALE);
        bry = (VRes - TARGET_OFFSET) - ((bry - Option.TOUCH_YZERO)*Option.TOUCH_YSCALE);
        if(abs(brx) > CAL_ERROR_MARGIN || abs(bry) > CAL_ERROR_MARGIN) {
            s = "Warning: Inaccurate calibration\r\n";
        } else
//        	PFlt(Option.TOUCH_XSCALE);PFltComma(Option.TOUCH_YSCALE);
//    		PIntComma(Option.TOUCH_XZERO);PIntComma(Option.TOUCH_YZERO);
//    		PIntComma(Option.TOUCH_SWAPXY);
            s = "Done. No errors\r\n";
        CurrentX = CurrentY = 0;
        MMPrintString(s);
        strcpy(inpbuf, "Deviation X = "); IntToStr(inpbuf + strlen(inpbuf), brx, 10);
        strcat(inpbuf, ", Y = "); IntToStr(inpbuf + strlen(inpbuf), bry, 10); strcat(inpbuf, " (pixels)\r\n");
        MMPrintString(inpbuf);
        if(!Option.DISPLAY_CONSOLE) {
            GUIPrintString(0, 0, 0x11, JUSTIFY_LEFT, JUSTIFY_TOP, ORIENT_NORMAL, WHITE, BLACK, s);
            GUIPrintString(0, 36, 0x11, JUSTIFY_LEFT, JUSTIFY_TOP, ORIENT_NORMAL, WHITE, BLACK, inpbuf);
        }
        return;
    }

    if((p = checkstring(cmdline, "TEST"))) {
        if((checkstring(p, "LCDPANEL"))) {
            int t, c=0;
            t = ((HRes > VRes) ? HRes : VRes) / 7;
            Timer3=0;
            while(getConsole() < '\r') {
                DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t/5, 1, 1, rgb((rand() % 8)*256/8, (rand() % 8)*256/8, (rand() % 8)*256/8), 1);
                if(Option.Refresh)Display_Refresh();
                c++;
            }
            c*=1000;
            c/=Timer3;
            ClearScreen(gui_bcolour);
            strcpy(inpbuf, "Circles per second = "); IntToStr(inpbuf + strlen(inpbuf), c, 10);
            if(!Option.DISPLAY_CONSOLE) {
                GUIPrintString(0, 36, 0x11, JUSTIFY_LEFT, JUSTIFY_TOP, ORIENT_NORMAL, WHITE, BLACK, inpbuf);
            }
            return;
        }
        if((checkstring(p, "TOUCH"))) {
            int x, y;
            ClearScreen(gui_bcolour);
            while(getConsole() < '\r') {
                x = GetTouch(GET_X_AXIS);
                y = GetTouch(GET_Y_AXIS);
                if(x != TOUCH_ERROR && y != TOUCH_ERROR){
                	DrawBox(x - 1, y - 1, x + 1, y + 1, 0, WHITE, WHITE);
                    if(Option.Refresh)Display_Refresh();
                }
            }
            ClearScreen(gui_bcolour);
            return;
        }
    }


    if((p = checkstring(cmdline, "RESET"))) {
        if((checkstring(p, "LCDPANEL"))) {
            InitDisplaySPI(0);
	if((Option.DISPLAY_TYPE >= SSD_PANEL_START && Option.DISPLAY_TYPE <= SSD_PANEL_END) || (Option.DISPLAY_TYPE >= P16_PANEL_START && Option.DISPLAY_TYPE<= P16_PANEL_END ))  InitDisplaySSD(0);
            if(Option.TOUCH_CS) {
                GetTouchValue(CMD_PENIRQ_ON);                                      // send the controller the command to turn on PenIRQ
                GetTouchAxis(CMD_MEASURE_X);
            }
            return;
        }
    }


    error("Unknown command");
}
/* This used by the BLIT commands. It uses temporary memory*/

char *blitmemory;
#define blitmemorysize 2560+RAMPAGESIZE
void DoBlit(int x1, int y1, int x2, int y2, int w, int h){
    int max_x;
    char *buff;
    int multiplier=3;
    blitmemory=GetTempMemory(blitmemorysize);
    if((w*h*multiplier)>blitmemorysize-RAMPAGESIZE){ //need to use alternative copy
        if(x1 >=x2) {
            max_x=(blitmemorysize-RAMPAGESIZE)/h/multiplier;
            buff=blitmemory;
            while(w > max_x){
                ReadBuffer(x1,y1,x1+max_x-1,y1+h-1,buff);
                DrawBuffer(x2,y2,x2+max_x-1,y2+h-1,buff);
                x1+=max_x;
                x2+=max_x;
                w-=max_x;
            }
            ReadBuffer(x1,y1,x1+w-1,y1+h-1,buff);
            DrawBuffer(x2,y2,x2+w-1,y2+h-1,buff);
            FreeMemory(buff);
            return;
        } else {
            int start_x1,start_x2;
            max_x=(blitmemorysize-RAMPAGESIZE)/h/multiplier;
            buff=blitmemory;
            start_x1=x1+w-max_x;
            start_x2=x2+w-max_x;
            while(w > max_x){
                ReadBuffer(start_x1,y1,start_x1+max_x-1,y1+h-1,buff);
                DrawBuffer(start_x2,y2,start_x2+max_x-1,y2+h-1,buff);
                w-=max_x;
                start_x1-=max_x;
                start_x2-=max_x;
            }
            ReadBuffer(x1,y1,x1+w-1,y1+h-1,buff);
            DrawBuffer(x2,y2,x2+w-1,y2+h-1,buff);
            FreeMemory(buff);
            return;
        }
    } else {
        buff=blitmemory;
        ReadBuffer(x1,y1,x1+w-1,y1+h-1,buff);
        DrawBuffer(x2,y2,x2+w-1,y2+h-1,buff);
        FreeMemory(buff);
    }
}

/* This used when OPTION LCDPANEL CONSOLE is set and scrolling LCDPanels that don't
 * support hardware scrolling. Its 1K buffer is allocated in CCRAM so that it still
 * has memory when the Edit command uses all available temporary ram for editting.
*/
/*
char blitmemory[1536*1];
void DoBlit(int x1, int y1, int x2, int y2, int w, int h){
    int max_x;
    char *buff;
    int multiplier=3;
    int memory=sizeof(blitmemory);
    if((w*h*multiplier)>memory-RAMPAGESIZE){ //need to use alternative copy
        if(x1 >=x2) {
            max_x=(memory-RAMPAGESIZE)/h/multiplier;
            buff=blitmemory;
            while(w > max_x){
                ReadBuffer(x1,y1,x1+max_x-1,y1+h-1,buff);
                DrawBuffer(x2,y2,x2+max_x-1,y2+h-1,buff);
                x1+=max_x;
                x2+=max_x;
                w-=max_x;
            }
            ReadBuffer(x1,y1,x1+w-1,y1+h-1,buff);
            DrawBuffer(x2,y2,x2+w-1,y2+h-1,buff);
            FreeMemory(buff);
            return;
        } else {
            int start_x1,start_x2;
            max_x=(memory-RAMPAGESIZE)/h/multiplier;
            buff=blitmemory;
            start_x1=x1+w-max_x;
            start_x2=x2+w-max_x;
            while(w > max_x){
                ReadBuffer(start_x1,y1,start_x1+max_x-1,y1+h-1,buff);
                DrawBuffer(start_x2,y2,start_x2+max_x-1,y2+h-1,buff);
                w-=max_x;
                start_x1-=max_x;
                start_x2-=max_x;
            }
            ReadBuffer(x1,y1,x1+w-1,y1+h-1,buff);
            DrawBuffer(x2,y2,x2+w-1,y2+h-1,buff);
            FreeMemory(buff);
            return;
        }
    } else {
        buff=blitmemory;
        ReadBuffer(x1,y1,x1+w-1,y1+h-1,buff);
        DrawBuffer(x2,y2,x2+w-1,y2+h-1,buff);
        FreeMemory(buff);
    }
}
*/
//BLIT WRITE [#]b, x, y [,mode] now in picomite passing w,h not used in H7 and picomites.
void cmd_blit(void){
    int x1, y1, x2, y2, w, h, bnbr;
    char *p;
    if((void *)ReadBuffer == (void *)DisplayNotSet) error("Invalid on this display");
    if((p = checkstring(cmdline, "READ"))) {
        getargs(&p, 9, ",");
        if(argc !=9) error("Syntax");
        if(*argv[0] == '#') argv[0]++;                              // check if the first arg is prefixed with a #
        bnbr = getint(argv[0], 1, MAXBLITBUF) - 1;                  // get the buffer number
        x1 = getinteger(argv[2]);
        y1 = getinteger(argv[4]);
        w = getinteger(argv[6]);
        h = getinteger(argv[8]);
        if(w < 1 || h < 1) return;
        if(x1 < 0) { x2 -= x1; w += x1; x1 = 0; }
        if(y1 < 0) { y2 -= y1; h += y1; y1 = 0; }
        if(x1 + w > HRes) w = HRes - x1;
        if(y1 + h > VRes) h = VRes - y1;
        if(w < 1 || h < 1 || x1 < 0 || x1 + w > HRes || y1 < 0 || y1 + h > VRes ) return;
        if(blitbuffptr[bnbr] == NULL){
            blitbuffptr[bnbr] = GetMemory(w * h * 3);
            ReadBuffer(x1, y1, x1 + w - 1, y1 + h - 1, blitbuffptr[bnbr]);
        } else error("Buffer in use");
    } else if((p = checkstring(cmdline, "WRITE"))) {
        getargs(&p, 9, ",");
        if(argc != 9) error("Syntax");
        if(*argv[0] == '#') argv[0]++;                              // check if the first arg is prefixed with a #
        bnbr = getint(argv[0], 1, MAXBLITBUF) - 1;                  // get the buffer number
        x1 = getinteger(argv[2]);
        y1 = getinteger(argv[4]);
        w = getinteger(argv[6]);
        h = getinteger(argv[8]);
        if(w < 1 || h < 1) return;
        if(x1 < 0) { x2 -= x1; w += x1; x1 = 0; }
        if(y1 < 0) { y2 -= y1; h += y1; y1 = 0; }
        if(x1 + w > HRes) w = HRes - x1;
        if(y1 + h > VRes) h = VRes - y1;
        if(w < 1 || h < 1 || x1 < 0 || x1 + w > HRes || y1 < 0 || y1 + h > VRes ) return;
        if(blitbuffptr[bnbr] != NULL){
            DrawBuffer(x1, y1, x1 + w - 1, y1 + h - 1, blitbuffptr[bnbr]);
        } else error("Buffer not in use");
    } else if((p = checkstring(cmdline, "CLOSE"))) {
        getargs(&p, 1, ",");
        if(*argv[0] == '#') argv[0]++;                              // check if the first arg is prefixed with a #
        bnbr = getint(argv[0], 1, MAXBLITBUF) - 1;                  // get the buffer number
        if(blitbuffptr[bnbr] != NULL){
            FreeMemory(blitbuffptr[bnbr]);
            blitbuffptr[bnbr] = NULL;
        } else error("Buffer not in use");
        // get the number
     } else {
    	 getargs(&cmdline, 11,",");
    	 if(argc !=11) error("Syntax");
    	 x1 = getinteger(argv[0]);
    	 y1 = getinteger(argv[2]);
    	 x2 = getinteger(argv[4]);
    	 y2 = getinteger(argv[6]);
    	 w = getinteger(argv[8]);
    	 h = getinteger(argv[10]);
    	 if(w < 1 || h < 1) return;
    	 if(x1 < 0) { x2 -= x1; w += x1; x1 = 0; }
    	 if(x2 < 0) { x1 -= x2; w += x2; x2 = 0; }
    	 if(y1 < 0) { y2 -= y1; h += y1; y1 = 0; }
    	 if(y2 < 0) { y1 -= y2; h += y2; y2 = 0; }
    	 if(x1 + w > HRes) w = HRes - x1;
    	 if(x2 + w > HRes) w = HRes - x2;
    	 if(y1 + h > VRes) h = VRes - y1;
    	 if(y2 + h > VRes) h = VRes - y2;
    	 if(w < 1 || h < 1 || x1 < 0 || x1 + w > HRes || x2 < 0 || x2 + w > HRes || y1 < 0 || y1 + h > VRes || y2 < 0 || y2 + h > VRes) return;
    	 DoBlit( x1, y1, x2, y2, w, h);
     }
}

// get and decode the justify$ string used in TEXT and GUI CAPTION
// the values are returned via pointers
int GetJustification(char *p, int *jh, int *jv, int *jo) {
    switch(toupper(*p++)) {
        case 'L':   *jh = JUSTIFY_LEFT; break;
        case 'C':   *jh = JUSTIFY_CENTER; break;
        case 'R':   *jh = JUSTIFY_RIGHT; break;
        case  0 :   return true;
        default:    p--;
    }
    skipspace(p);
    switch(toupper(*p++)) {
        case 'T':   *jv = JUSTIFY_TOP; break;
        case 'M':   *jv = JUSTIFY_MIDDLE; break;
        case 'B':   *jv = JUSTIFY_BOTTOM; break;
        case  0 :   return true;
        default:    p--;
    }
    skipspace(p);
    switch(toupper(*p++)) {
        case 'N':   *jo = ORIENT_NORMAL; break;                     // normal
        case 'V':   *jo = ORIENT_VERT; break;                       // vertical text (top to bottom)
        case 'I':   *jo = ORIENT_INVERTED; break;                   // inverted
        case 'U':   *jo = ORIENT_CCW90DEG; break;                   // rotated CCW 90 degrees
        case 'D':   *jo = ORIENT_CW90DEG; break;                    // rotated CW 90 degrees
        case  0 :   return true;
        default:    return false;
    }
    return *p == 0;
}

void cmd_text(void) {
    int x, y, font, scale, fc, bc;
    char *s;
    int jh = 0, jv = 0, jo = 0;

    getargs(&cmdline, 17, ",");                                     // this is a macro and must be the first executable stmt
    if(!(argc & 1) || argc < 5) error("Argument count");
    x = getinteger(argv[0]);
    y = getinteger(argv[2]);
    s = getCstring(argv[4]);

    if(argc > 5 && *argv[6])
        if(!GetJustification(argv[6], &jh, &jv, &jo))
            if(!GetJustification(getCstring(argv[6]), &jh, &jv, &jo))
                error("Justification");;

    font = (gui_font >> 4) + 1; scale = (gui_font & 0b1111); fc = gui_fcolour; bc = gui_bcolour;        // the defaults
    if(argc > 7 && *argv[8]) {
        if(*argv[8] == '#') argv[8]++;
        font = getint(argv[8], 1, FONT_TABLE_SIZE);
    }
    if(FontTable[font - 1] == NULL) error("Invalid font #%", font);
	if(argc > 9 && *argv[10]) scale = getint(argv[10], 1, 15);
	if(argc > 11 && *argv[12]) fc = getint(argv[12], 0, WHITE);
	if(argc ==15) bc = getint(argv[14], -1, WHITE);
    GUIPrintString(x, y, ((font - 1) << 4) | scale, jh, jv, jo, fc, bc, s);
}

/*
void  getargaddress (char *p, long long int **ip, MMFLOAT **fp, int *n){
    char *ptr=NULL;
    *fp=NULL;
    *ip=NULL;
    char pp[STRINGSIZE]={0};
    strcpy(pp,(char *)p);
    if(!isnamestart(pp[0])){ //found a literal
        *n=1;
        return;
    }
    ptr = findvar(pp, V_FIND | V_EMPTY_OK | V_NOFIND_NULL);
    if(ptr && vartbl[VarIndex].type & T_NBR) {
        if(vartbl[VarIndex].dims[0] <= 0){ //simple variable
            *n=1;
            return;
        } else { // array or array element
            if(*n == 0)*n=vartbl[VarIndex].dims[0] + 1 - OptionBase;
            else *n = (vartbl[VarIndex].dims[0] + 1 - OptionBase)< *n ? (vartbl[VarIndex].dims[0] + 1 - OptionBase) : *n;
            skipspace(p);
            do {
                p++;
            } while(isnamechar(*p));
            if(*p == '!') p++;
            if(*p == '(') {
                p++;
                skipspace(p);
                if(*p != ')') { //array element
                    *n=1;
                    return;
                }
            }
        }
        if(vartbl[VarIndex].dims[1] != 0) error("Invalid variable");
        *fp = (MMFLOAT*)ptr;
    } else if(ptr && vartbl[VarIndex].type & T_INT) {
        if(vartbl[VarIndex].dims[0] <= 0){
            *n=1;
            return;
        } else {
            if(*n == 0)*n=vartbl[VarIndex].dims[0] + 1 - OptionBase;
            else *n = (vartbl[VarIndex].dims[0] + 1 - OptionBase)< *n ? (vartbl[VarIndex].dims[0] + 1 - OptionBase) : *n;
            skipspace(p);
            do {
                p++;
            } while(isnamechar(*p));
            if(*p == '%') p++;
            if(*p == '(') {
                p++;
                skipspace(p);
                if(*p != ')') { //array element
                    *n=1;
                    return;
                }
            }
        }
        if(vartbl[VarIndex].dims[1] != 0) error("Invalid variable");
        *ip = (long long int *)ptr;
    } else {
    	*n=1; //may be a function call
    }
}
*/
// new one from Picomite with TOMs fix for LINE
void  getargaddress (char *p, long long int **ip, MMFLOAT **fp, int *n){
    char *ptr=NULL;
    *fp=NULL;
    *ip=NULL;
    char pp[STRINGSIZE]={0};
    strcpy(pp,(char *)p);
    if(!isnamestart(pp[0])){ //found a literal
        *n=1;
        return;
    }
    ptr = findvar((char *)pp, V_FIND | V_EMPTY_OK | V_NOFIND_NULL);
    if(ptr && vartbl[VarIndex].type & (T_NBR | T_INT)) {
        if(vartbl[VarIndex].dims[0] <= 0){ //simple variable
            *n=1;
            return;
        } else { // array or array element
            if(*n == 0)*n=vartbl[VarIndex].dims[0] + 1 - OptionBase;
            else *n = (vartbl[VarIndex].dims[0] + 1 - OptionBase)< *n ? (vartbl[VarIndex].dims[0] + 1 - OptionBase) : *n;
            skipspace(p);
            do {
                p++;
            } while(isnamechar(*p));
            //if(*p == '!') p++;               // <--------- changed this line
            if (*p == '!' || *p == '%') p++;  // <--------- change by TOM to fix LINE issue
            if(*p == '(') {
                p++;
                skipspace(p);
                if(*p != ')') { //array element
                    *n=1;
                    return;
                }
            }
        }
        if(vartbl[VarIndex].dims[1] != 0) error("Invalid variable");
        if(vartbl[VarIndex].type & T_NBR)*fp = (MMFLOAT*)ptr;
        else *ip = (long long int *)ptr;
    } else {
    	*n=1; //may be a function call
    }
}



void cmd_pixel(void) {
    int x1, y1, c=0, n=0 ,i, nc=0;
    long long int *x1ptr, *y1ptr, *cptr;
    MMFLOAT *x1fptr, *y1fptr, *cfptr;
    getargs(&cmdline, 5,",");
    if(!(argc == 3 || argc == 5)) error("Argument count");
    getargaddress(argv[0], &x1ptr, &x1fptr, &n);
    if(n != 1) getargaddress(argv[2], &y1ptr, &y1fptr, &n);
    if(n==1){ //just a single point
        c = gui_fcolour;                                    // setup the defaults
        x1 = getinteger(argv[0]);
        y1 = getinteger(argv[2]);
        if(argc == 5)
            c = getint(argv[4], 0, WHITE);
        else
            c = gui_fcolour;
        DrawPixel(x1, y1, c);
    } else {
        c = gui_fcolour;                                        // setup the defaults
        if(argc == 5){
            getargaddress(argv[4], &cptr, &cfptr, &nc); 
            if(nc == 1) c = getint(argv[4], 0, WHITE);
            else if(nc>1) {
                if(nc < n) n=nc; //adjust the dimensionality
                for(i=0;i<nc;i++){
                    c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                    if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        for(i=0;i<n;i++){
            x1 = (x1fptr == NULL ? x1ptr[i] : (int)x1fptr[i]);
            y1 = (y1fptr == NULL ? y1ptr[i] : (int)y1fptr[i]);
            if(nc > 1) c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
            DrawPixel(x1, y1, c);
        }
    }
    if(Option.Refresh)Display_Refresh();
}
void DrawBuffered(int xti, int yti, int c, int complete){
	static unsigned char pos=0;
	static unsigned char movex, movey, movec;
	static short xtilast[8];
	static short ytilast[8];
	static int clast[8];
	xtilast[pos]=xti;
	ytilast[pos]=yti;
	clast[pos]=c;
	if(complete==1){
		if(pos==1){
			DrawPixel(xtilast[0],ytilast[0],clast[0]);
		} else {
			DrawLine(xtilast[0],ytilast[0],xtilast[pos-1],ytilast[pos-1],1,clast[0]);
		}
		pos=0;
	} else {
		if(pos==0){
			movex = movey = movec = 1;
			pos+=1;
		} else {
			if(xti==xtilast[0] && abs(yti-ytilast[pos-1])==1)movex=0;else movex=1;
			if(yti==ytilast[0] && abs(xti-xtilast[pos-1])==1)movey=0;else movey=1;
			if(c==clast[0])movec=0;else movec=1;
			if(movec==0 && (movex==0 || movey==0) && pos<6) pos+=1;
			else {
				if(pos==1){
					DrawPixel(xtilast[0],ytilast[0],clast[0]);
				} else {
					DrawLine(xtilast[0],ytilast[0],xtilast[pos-1],ytilast[pos-1],1,clast[0]);
				}
				movex = movey = movec = 1;
				xtilast[0]=xti;
				ytilast[0]=yti;
				clast[0]=c;
				pos=1;
			}
		}
	}
}

void bezier(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, int c){
    float tmp,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7,tmp8,t=0.0,xt=x0,yt=y0;
    int i, xti,yti,xtlast=-1, ytlast=-1;
    for (i=0; i<500; i++)
    {
    	tmp = 1.0 - t;
    	tmp3 = t * t;
    	tmp4 = tmp * tmp;
    	tmp1 = tmp3 * t;
    	tmp2 = tmp4 * tmp;
    	tmp5 = 3.0 * t;
    	tmp6 = 3.0 *  tmp3;
    	tmp7 = tmp5 * tmp4;
    	tmp8 = tmp6 * tmp;
    	xti=(int)xt;
    	yti=(int)yt;
    	xt =    ((((tmp2 * x0) + (tmp7 * x1)) + (tmp8 * x2)) + (tmp1 * x3));
    	yt =    ((((tmp2 * y0) + (tmp7 * y1)) +	(tmp8 * y2)) +(tmp1 * y3));
    	if((xti!=xtlast) || (yti!=ytlast)) {
    		DrawBuffered(xti, yti, c, 0);
    		xtlast=xti;
    		ytlast=yti;
    	}
    	t+=0.002;
    }
	DrawBuffered(0, 0, 0, 1);
}


void pointcalc(int angle, int x, int y, int r2, int *x0, int * y0){
	float c1,s1;
	int quad;
	angle %=360;
	switch(angle){
	case 0:
		*x0=x;
		*y0=y-r2;
		break;
	case 45:
		*x0=x+r2+1;
		*y0=y-r2;
		break;
	case 90:
		*x0=x+r2+1;
		*y0=y;
		break;
	case 135:
		*x0=x+r2+1;
		*y0=y+r2;
		break;
	case 180:
		*x0=x;
		*y0=y+r2;
		break;
	case 225:
		*x0=x-r2;
		*y0=y+r2;
		break;
	case 270:
		*x0=x-r2;
		*y0=y;
		break;
	case 315:
		*x0=x-r2;
		*y0=y-r2;
		break;
	default:
		c1=cosf(Rad(angle));
		s1=sinf(Rad(angle));
		quad = (angle / 45) % 8;
		switch(quad){
		case 0:
			*y0=y-r2;
			*x0=x+s1*r2/c1;
			break;
		case 1:
 		  *x0=x+r2+1;
 		  *y0=y-c1*r2/s1;
 		  break;
		case 2:
 		  *x0=x+r2+1;
 		  *y0=y-c1*r2/s1;
 		  break;
		case 3:
 		  *y0=y+r2;
 		  *x0=x-s1*r2/c1;
 		  break;
		case 4:
 		  *y0=y+r2;
 		  *x0=x-s1*r2/c1;
 		  break;
		case 5:
 		  *x0=x-r2;
 		  *y0=y+c1*r2/s1;
 		  break;
		case 6:
			*x0=x-r2;
			*y0=y+c1*r2/s1;
			break;
		case 7:
			*y0=y-r2;
			*x0=x+s1*r2/c1;
			break;
		}
	}
}
void cmd_arc(void){
	// Parameters are:
	// X coordinate of centre of arc
	// Y coordinate of centre of arc
	// inner radius of arc
	// outer radius of arc - omit it 1 pixel wide
	// start radial of arc in degrees
	// end radial of arc in degrees
	// Colour of arc
	int x, y, r1, r2, c ,i ,j, k, xs=-1, xi=0, m;
	int rad1, rad2, rad3, rstart, quadr;
	int x0, y0, x1, y1, x2, y2, xr, yr;
	getargs(&cmdline, 13,",");
    if(!(argc == 11 || argc == 13)) error("Argument count");
    if(Option.DISPLAY_TYPE == 0) error("Display not configured");
    x = getinteger(argv[0]);
    y = getinteger(argv[2]);
    r1 = getinteger(argv[4]);
    if(*argv[6])r2 = getinteger(argv[6]);
    else {
    	r2=r1;
    	r1--;
    }
    if(r2 < r1)error("Inner radius < outer");
    rad1 = getnumber(argv[8]);
    rad2 = getnumber(argv[10]);
    while(rad1<0.0)rad1+=360.0;
    while(rad2<0.0)rad2+=360.0;
	if(rad1==rad2)error("Radials");
     if(argc == 13)
        c = getint(argv[12], 0, WHITE);
    else
        c = gui_fcolour;
    while(rad2<rad1)rad2+=360;
    rad3=rad1+360;
    rstart=rad2;
    int quad1 = (rad1 / 45) % 8;
    x2=x;y2=y;
    int ints_per_line=RoundUptoInt((r2*2)+1)/32;
    uint32_t *br=(uint32_t *)GetTempMemory(((ints_per_line+1)*((r2*2)+1))*4);
    DrawFilledCircle(x, y, r2, r2, 1, ints_per_line, br, 1.0, 1.0);
    DrawFilledCircle(x, y, r1, r2, 0, ints_per_line, br, 1.0, 1.0);
    while(rstart<rad3){
		pointcalc(rstart, x, y, r2, &x0, &y0);
   		quadr = (rstart / 45) % 8;
    	if(quadr==quad1 && rad3-rstart<45){
    		pointcalc(rad3, x, y, r2, &x1, &y1);
    		ClearTriangle(x0-x+r2, y0-y+r2, x1-x+r2, y1-y+r2, x2-x+r2, y2-y+r2, ints_per_line, br);
    		rstart=rad3;
    	} else {
    			rstart +=45;
    			rstart -= (rstart % 45);
    			pointcalc(rstart, x, y, r2, &xr, &yr);
    			ClearTriangle(x0-x+r2, y0-y+r2, xr-x+r2, yr-y+r2, x2-x+r2, y2-y+r2, ints_per_line, br);
    	}
    }
    int save_refresh=Option.Refresh;
    Option.Refresh=0;
 	for(j=0;j<r2*2+1;j++){
 		for(i=0;i<ints_per_line;i++){
 			k=br[i+j*ints_per_line];
 			for(m=0;m<32;m++){
 				if(xs==-1 && (k & 0x80000000)){
 					xs=m;
 					xi=i;
 				}
 				if(xs!=-1 && !(k & 0x80000000)){
					DrawRectangle(x-r2+xs+xi*32, y-r2+j, x-r2+m+i*32, y-r2+j, c);
 					xs=-1;
 				}
 				k<<=1;
 			}
 		}
		if(xs!=-1){
			DrawRectangle(x-r2+xs+xi*32, y-r2+j, x-r2+m+i*32, y-r2+j, c);
			xs=-1;
		}
	}
	Option.Refresh=save_refresh;
    if(Option.Refresh)Display_Refresh();
}
void cmd_bezier(void) {
	// X coordinate of start point
	// Y coordinate of start point
	// X coordinate of first control point
	// Y coordinate of first control point
	// X coordinate of second control point
	// Y coordinate of second control point
	// X coordinate of end point
	// Y coordinate of end point
	// Colour of curve
	int xx0, yy0, xx1, yy1, xx2, yy2, xx3, yy3, c;
    getargs(&cmdline, 17,",");
    if(!(argc==15 || argc==17))error("Argument count");
    if(Option.DISPLAY_TYPE == 0) error("Display not configured");
    xx0 = getint(argv[0],0,HRes-1);
    yy0 = getint(argv[2],0,VRes-1);
    xx1 = getinteger(argv[4]);
    yy1 = getinteger(argv[6]);
    xx2 = getinteger(argv[8]);
    yy2 = getinteger(argv[10]);
    xx3 = getint(argv[12],0,HRes-1);
    yy3 = getint(argv[14],0,VRes-1);
    c = gui_fcolour;                                    // setup the defaults
    if(argc > 15 && *argv[16]) c = getint(argv[16], 0, WHITE);
    int save_refresh=Option.Refresh;
    Option.Refresh=0;
    bezier(xx0, yy0, xx1, yy1, xx2, yy2, xx3, yy3, c);
	Option.Refresh=save_refresh;
    if(Option.Refresh)Display_Refresh();
}
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} rgb_t;
#define TFLOAT float
typedef struct {
    TFLOAT  xpos;       // current position and heading
    TFLOAT  ypos;       // (uses floating-point numbers for
    TFLOAT  heading;    //  increased accuracy)

    rgb_t  pen_color;   // current pen color
    rgb_t  fill_color;  // current fill color
    bool   pendown;     // currently drawing?
    bool   filled;      // currently filling?
} fill_t;
fill_t main_fill;
fill_t backup_fill;
int    main_fill_poly_vertex_count = 0;       // polygon vertex count
TFLOAT *main_fill_polyX=NULL; // polygon vertex x-coords
TFLOAT *main_fill_polyY=NULL; // polygon vertex y-coords

void fill_set_pen_color(int red, int green, int blue)
{
    main_fill.pen_color.red = red;
    main_fill.pen_color.green = green;
    main_fill.pen_color.blue = blue;
}

void fill_set_fill_color(int red, int green, int blue)
{
    main_fill.fill_color.red = red;
    main_fill.fill_color.green = green;
    main_fill.fill_color.blue = blue;
}
static void fill_begin_fill()
{
    main_fill.filled = true;
    main_fill_poly_vertex_count = 0;
}

static void fill_end_fill(int count, int ystart, int yend)
{
    // based on public-domain fill algorithm in C by Darel Rex Finley, 2007
    //   from http://alienryderflex.com/polygon_fill/

    TFLOAT *nodeX=GetMemory(count * sizeof(TFLOAT));     // x-coords of polygon intercepts
    int nodes;                              // size of nodeX
    int y, i, j;                         // current pixel and loop indices
    TFLOAT temp;                            // temporary variable for sorting
    int f=(main_fill.fill_color.red<<16) | (main_fill.fill_color.green<<8) | main_fill.fill_color.blue;
    int c= (main_fill.pen_color.red<<16) | (main_fill.pen_color.green<<8) | main_fill.pen_color.blue;
    int xstart, xend;
    //  loop through the rows of the image

    for (y = ystart; y < yend; y++) {

        //  build a list of polygon intercepts on the current line
        nodes = 0;
        j = main_fill_poly_vertex_count-1;
        for (i = 0; i < main_fill_poly_vertex_count; i++) {
            if ((main_fill_polyY[i] <  (TFLOAT)y &&
                 main_fill_polyY[j] >= (TFLOAT)y) ||
                (main_fill_polyY[j] <  (TFLOAT)y &&
                 main_fill_polyY[i] >= (TFLOAT)y)) {

                // intercept found; record it
                nodeX[nodes++] = (main_fill_polyX[i] +
                        ((TFLOAT)y - main_fill_polyY[i]) /
                        (main_fill_polyY[j] - main_fill_polyY[i]) *
                        (main_fill_polyX[j] - main_fill_polyX[i]));
            }
            j = i;
        }

        //  sort the nodes via simple insertion sort
        for (i = 1; i < nodes; i++) {
            temp = nodeX[i];
            for (j = i; j > 0 && temp < nodeX[j-1]; j--) {
                nodeX[j] = nodeX[j-1];
            }
            nodeX[j] = temp;
        }

        //  fill the pixels between node pairs
        for (i = 0; i < nodes; i += 2) {
        	xstart=(int)floorf(nodeX[i])+1;
        	xend=(int)ceilf(nodeX[i+1])-1;
        	DrawLine(xstart,y,xend,y,1, f);
        }
    }

    main_fill.filled = false;

    // redraw polygon (filling is imperfect and can occasionally occlude sides)
    for (i = 0; i < main_fill_poly_vertex_count; i++) {
        int x0 = (int)roundf(main_fill_polyX[i]);
        int y0 = (int)roundf(main_fill_polyY[i]);
        int x1 = (int)roundf(main_fill_polyX[(i+1) %
            main_fill_poly_vertex_count]);
        int y1 = (int)roundf(main_fill_polyY[(i+1) %
            main_fill_poly_vertex_count]);
        DrawLine(x0, y0, x1, y1, 1, c);
    }
    FreeMemory(nodeX);
}
/**** New from Picomite to allow LINEGRAPH ********************/
void polygon(char *p, int close){
	int xcount=0;
	long long int *xptr=NULL, *yptr=NULL,xptr2=0, yptr2=0, *polycount=NULL, *cptr=NULL, *fptr=NULL;
	MMFLOAT *polycountf=NULL, *cfptr=NULL, *ffptr=NULL, *xfptr=NULL, *yfptr=NULL, xfptr2=0, yfptr2=0;
	int i, f=0, c, xtot=0, ymax=0, ymin=1000000;
    int n=0, nx=0, ny=0, nc=0, nf=0;
    getargs(&p, 9,",");
    if(Option.DISPLAY_TYPE == 0) error("Display not configured");
    getargaddress(argv[0], &polycount, &polycountf, &n);
    if(n==1){
    	xcount = xtot = getinteger(argv[0]);
    	if((xcount<3 || xcount>9999) && xcount!=0)error("Invalid number of vertices");
        getargaddress(argv[2], &xptr, &xfptr, &nx);
        if(xcount==0){
            xcount = xtot = nx;
        }
        if(nx<xtot)error("X Dimensions %", nx);
        getargaddress(argv[4], &yptr, &yfptr, &ny);
        if(ny<xtot)error("Y Dimensions %", ny);
        if(xptr)xptr2=*xptr;
        else xfptr2=*xfptr;
        if(yptr)yptr2=*yptr;
        else yfptr2=*yfptr;
        c = gui_fcolour;                                    // setup the defaults
        if(argc > 5 && *argv[6]) c = getint(argv[6], 0, WHITE);
        if(argc > 7 && *argv[8]){
        	main_fill_polyX=(TFLOAT  *)GetTempMemory(xtot * sizeof(TFLOAT));
        	main_fill_polyY=(TFLOAT  *)GetTempMemory(xtot * sizeof(TFLOAT));
        	f = getint(argv[8], 0, WHITE);
    		fill_set_fill_color((f>>16) & 0xFF, (f>>8) & 0xFF , f & 0xFF);
        	fill_set_pen_color((c>>16) & 0xFF, (c>>8) & 0xFF , c & 0xFF);
        	fill_begin_fill();
        }
       for(i=0;i<xcount-1;i++){
          	if(argc > 7){
                  main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
                  main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
                  if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
                  if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
                  main_fill_poly_vertex_count++;
          	} else {
          		int x1=(xfptr==NULL ? *xptr++ : (int)*xfptr++);
          		int x2=(xfptr==NULL ? *xptr : (int)*xfptr);
          		int y1=(yfptr==NULL ? *yptr++ : (int)*yfptr++);
          		int y2=(yfptr==NULL ? *yptr : (int)*yfptr);
           		DrawLine(x1,y1,x2,y2, 1, c);
           	}
        }
        if(argc > 7){
            main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
            main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
            if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
            if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
            if(main_fill_polyY[main_fill_poly_vertex_count]!=main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]){
                	main_fill_poly_vertex_count++;
                	main_fill_polyX[main_fill_poly_vertex_count]=main_fill_polyX[0];
                	main_fill_polyY[main_fill_poly_vertex_count]=main_fill_polyY[0];
            }
            main_fill_poly_vertex_count++;
        	if(main_fill_poly_vertex_count>5){
        		fill_end_fill(xcount,ymin,ymax);
        	} else if(main_fill_poly_vertex_count==5){
				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],f,f);
				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],f,f);
				if(f!=c){
					DrawLine(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],1,c);
					DrawLine(main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],1,c);
					DrawLine(main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],1,c);
					DrawLine(main_fill_polyX[3],main_fill_polyY[3],main_fill_polyX[4],main_fill_polyY[4],1,c);
				}
			} else {
				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],c,f);
        	}
       } else if(close){
    		int x1=(xfptr==NULL ? *xptr : (int)*xfptr);
    		int x2=(xfptr==NULL ? xptr2 : (int)xfptr2);
    		int y1=(yfptr==NULL ? *yptr : (int)*yfptr);
    		int y2=(yfptr==NULL ? yptr2 : (int)yfptr2);
    		DrawLine(x1,y1,x2,y2, 1, c);
        }
    } else {
    	int *cc=GetTempMemory(n*sizeof(int)); //array for foreground colours
    	int *ff=GetTempMemory(n*sizeof(int)); //array for background colours
    	int xstart ,j, xmax=0;
    	for(i=0;i<n;i++){
    		if((polycountf == NULL ? polycount[i] : (int)polycountf[i])>xmax)xmax=(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
    		if(!(polycountf == NULL ? polycount[i] : (int)polycountf[i]))break;
    		xtot+=(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
    		if((polycountf == NULL ? polycount[i] : (int)polycountf[i])<3 || (polycountf == NULL ? polycount[i] : (int)polycountf[i])>9999)error("Invalid number of vertices, polygon %",i);
    	}
    	n=i;
        getargaddress(argv[2], &xptr, &xfptr, &nx);
        if(nx<xtot)error("X Dimensions %", nx);
        getargaddress(argv[4], &yptr, &yfptr, &ny);
        if(ny<xtot)error("Y Dimensions %", ny);
    	main_fill_polyX=(TFLOAT  *)GetTempMemory(xmax * sizeof(TFLOAT));
    	main_fill_polyY=(TFLOAT  *)GetTempMemory(xmax * sizeof(TFLOAT));
		if(argc > 5 && *argv[6]){ //foreground colour specified
			getargaddress(argv[6], &cptr, &cfptr, &nc);
			if(nc == 1) for(i=0;i<n;i++)cc[i] = getint(argv[6], 0, WHITE);
			else {
				if(nc < n) error("Foreground colour Dimensions");
				for(i=0;i<n;i++){
					cc[i] = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
					if(cc[i] < 0 || cc[i] > 0xFFFFFF) error("% is invalid (valid is % to %)", (int)cc[i], 0, 0xFFFFFF);
				}
			}
		} else for(i=0;i<n;i++)cc[i] = gui_fcolour;
		if(argc > 7){ //background colour specified
			getargaddress(argv[8], &fptr, &ffptr, &nf);
			if(nf == 1) for(i=0;i<n;i++) ff[i] = getint(argv[8], 0, WHITE);
			else {
				if(nf < n) error("Background colour Dimensions");
				for(i=0;i<n;i++){
					ff[i] = (ffptr == NULL ? fptr[i] : (int)ffptr[i]);
					if(ff[i] < 0 || ff[i] > 0xFFFFFF) error("% is invalid (valid is % to %)", (int)ff[i], 0, 0xFFFFFF);
				}
			}
		}
    	xstart=0;
    	for(i=0;i<n;i++){
            if(xptr)xptr2=*xptr;
            else xfptr2=*xfptr;
            if(yptr)yptr2=*yptr;
            else yfptr2=*yfptr;
    		ymax=0;
    		ymin=1000000;
    		main_fill_poly_vertex_count=0;
        	xcount = (int)(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
            if(argc > 7 && *argv[8]){
            	fill_set_pen_color((cc[i]>>16) & 0xFF, (cc[i]>>8) & 0xFF , cc[i] & 0xFF);
        		fill_set_fill_color((ff[i]>>16) & 0xFF, (ff[i]>>8) & 0xFF , ff[i] & 0xFF);
            	fill_begin_fill();
            }
           for(j=xstart;j<xstart+xcount-1;j++){
            	if(argc > 7){
                    main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
                    main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
                    if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
                    if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
                    main_fill_poly_vertex_count++;
            	} else {
            		int x1=(xfptr==NULL ? *xptr++ : (int)*xfptr++);
            		int x2=(xfptr==NULL ? *xptr : (int)*xfptr);
            		int y1=(yfptr==NULL ? *yptr++ : (int)*yfptr++);
            		int y2=(yfptr==NULL ? *yptr : (int)*yfptr);
            		DrawLine(x1,y1,x2,y2, 1, cc[i]);
            	}
            }
            if(argc > 7){
                main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
                main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
                if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
                if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
                if(main_fill_polyY[main_fill_poly_vertex_count]!=main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]){
                    	main_fill_poly_vertex_count++;
                    	main_fill_polyX[main_fill_poly_vertex_count]=main_fill_polyX[0];
                    	main_fill_polyY[main_fill_poly_vertex_count]=main_fill_polyY[0];
                }
                main_fill_poly_vertex_count++;
            	if(main_fill_poly_vertex_count>5){
            		fill_end_fill(xcount,ymin,ymax);
            	} else if(main_fill_poly_vertex_count==5){
    				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],ff[i],ff[i]);
    				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],ff[i],ff[i]);
    				if(ff[i]!=cc[i]){
    					DrawLine(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],1,cc[i]);
    					DrawLine(main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],1,cc[i]);
    					DrawLine(main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],1,cc[i]);
    					DrawLine(main_fill_polyX[3],main_fill_polyY[3],main_fill_polyX[4],main_fill_polyY[4],1,cc[i]);
    				}
    			} else {
    				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],cc[i],ff[i]);
            	}
            } else {
        		int x1=(xfptr==NULL ? *xptr : (int)*xfptr);
        		int x2=(xfptr==NULL ? xptr2 : (int)xfptr2);
        		int y1=(yfptr==NULL ? *yptr : (int)*yfptr);
        		int y2=(yfptr==NULL ? yptr2 : (int)yfptr2);
        		DrawLine(x1,y1,x2,y2, 1, cc[i]);
            	if(xfptr!=NULL)xfptr++;
            	else xptr++;
            	if(yfptr!=NULL)yfptr++;
            	else yptr++;
            }

            xstart+=xcount;
    	}
    }
}

void cmd_polygon(void){
    polygon(cmdline,1);
}




/* Orginal pre
void cmd_polygon(void){
	int xcount=0;
	long long int *xptr=NULL, *yptr=NULL,xptr2=0, yptr2=0, *polycount=NULL, *cptr=NULL, *fptr=NULL;
	MMFLOAT *polycountf=NULL, *cfptr=NULL, *ffptr=NULL, *xfptr=NULL, *yfptr=NULL, xfptr2=0, yfptr2=0;
	int i, f=0, c, xtot=0, ymax=0, ymin=1000000;
    int n=0, nx=0, ny=0, nc=0, nf=0;
    getargs(&cmdline, 9,",");
    getargaddress(argv[0], &polycount, &polycountf, &n);
    if(n==1){
    	xcount = xtot = getinteger(argv[0]);
    	if(xcount<3 || xcount>9999)error("Invalid number of vertices");
        getargaddress(argv[2], &xptr, &xfptr, &nx);
        if(nx<xtot)error("X Dimensions %", nx);
        getargaddress(argv[4], &yptr, &yfptr, &ny);
        if(ny<xtot)error("Y Dimensions %", ny);
        if(xptr)xptr2=*xptr;
        else xfptr2=*xfptr;
        if(yptr)yptr2=*yptr;
        else yfptr2=*yfptr;
        c = gui_fcolour;                                    // setup the defaults
        if(argc > 5 && *argv[6]) c = getint(argv[6], 0, WHITE);
        if(argc > 7 && *argv[8]){
        	main_fill_polyX=(TFLOAT  *)GetTempMemory(xtot * sizeof(TFLOAT));
        	main_fill_polyY=(TFLOAT  *)GetTempMemory(xtot * sizeof(TFLOAT));
        	f = getint(argv[8], 0, WHITE);
    		fill_set_fill_color((f>>16) & 0xFF, (f>>8) & 0xFF , f & 0xFF);
        	fill_set_pen_color((c>>16) & 0xFF, (c>>8) & 0xFF , c & 0xFF);
        	fill_begin_fill();
        }
       for(i=0;i<xcount-1;i++){
          	if(argc > 7){
                  main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
                  main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
                  if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
                  if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
                  main_fill_poly_vertex_count++;
          	} else {
          		int x1=(xfptr==NULL ? *xptr++ : (int)*xfptr++);
          		int x2=(xfptr==NULL ? *xptr : (int)*xfptr);
          		int y1=(yfptr==NULL ? *yptr++ : (int)*yfptr++);
          		int y2=(yfptr==NULL ? *yptr : (int)*yfptr);
           		DrawLine(x1,y1,x2,y2, 1, c);
           	}
        }
        if(argc > 7){
            main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
            main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
            if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
            if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
            if(main_fill_polyY[main_fill_poly_vertex_count]!=main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]){
                	main_fill_poly_vertex_count++;
                	main_fill_polyX[main_fill_poly_vertex_count]=main_fill_polyX[0];
                	main_fill_polyY[main_fill_poly_vertex_count]=main_fill_polyY[0];
            }
            main_fill_poly_vertex_count++;
        	if(main_fill_poly_vertex_count>5){
        		fill_end_fill(xcount,ymin,ymax);
        	} else if(main_fill_poly_vertex_count==5){
				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],f,f);
				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],f,f);
				if(f!=c){
					DrawLine(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],1,c);
					DrawLine(main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],1,c);
					DrawLine(main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],1,c);
					DrawLine(main_fill_polyX[3],main_fill_polyY[3],main_fill_polyX[4],main_fill_polyY[4],1,c);
				}
			} else {
				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],c,f);
        	}
       } else {
    		int x1=(xfptr==NULL ? *xptr : (int)*xfptr);
    		int x2=(xfptr==NULL ? xptr2 : (int)xfptr2);
    		int y1=(yfptr==NULL ? *yptr : (int)*yfptr);
    		int y2=(yfptr==NULL ? yptr2 : (int)yfptr2);
    		DrawLine(x1,y1,x2,y2, 1, c);
        }
    } else {
    	int *cc=GetTempMemory(n*sizeof(int)); //array for foreground colours
    	int *ff=GetTempMemory(n*sizeof(int)); //array for background colours
    	int xstart ,j, xmax=0;
    	for(i=0;i<n;i++){
    		if((polycountf == NULL ? polycount[i] : (int)polycountf[i])>xmax)xmax=(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
    		if(!(polycountf == NULL ? polycount[i] : (int)polycountf[i]))break;
    		xtot+=(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
    		if((polycountf == NULL ? polycount[i] : (int)polycountf[i])<3 || (polycountf == NULL ? polycount[i] : (int)polycountf[i])>9999)error("Invalid number of vertices, polygon %",i);
    	}
    	n=i;
        getargaddress(argv[2], &xptr, &xfptr, &nx);
        if(nx<xtot)error("X Dimensions %", nx);
        getargaddress(argv[4], &yptr, &yfptr, &ny);
        if(ny<xtot)error("Y Dimensions %", ny);
    	main_fill_polyX=(TFLOAT  *)GetTempMemory(xmax * sizeof(TFLOAT));
    	main_fill_polyY=(TFLOAT  *)GetTempMemory(xmax * sizeof(TFLOAT));
		if(argc > 5 && *argv[6]){ //foreground colour specified
			getargaddress(argv[6], &cptr, &cfptr, &nc);
			if(nc == 1) for(i=0;i<n;i++)cc[i] = getint(argv[6], 0, WHITE);
			else {
				if(nc < n) error("Foreground colour Dimensions");
				for(i=0;i<n;i++){
					cc[i] = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
					if(cc[i] < 0 || cc[i] > 0xFFFFFF) error("% is invalid (valid is % to %)", (int)cc[i], 0, 0xFFFFFF);
				}
			}
		} else for(i=0;i<n;i++)cc[i] = gui_fcolour;
		if(argc > 7){ //background colour specified
			getargaddress(argv[8], &fptr, &ffptr, &nf);
			if(nf == 1) for(i=0;i<n;i++) ff[i] = getint(argv[8], 0, WHITE);
			else {
				if(nf < n) error("Background colour Dimensions");
				for(i=0;i<n;i++){
					ff[i] = (ffptr == NULL ? fptr[i] : (int)ffptr[i]);
					if(ff[i] < 0 || ff[i] > 0xFFFFFF) error("% is invalid (valid is % to %)", (int)ff[i], 0, 0xFFFFFF);
				}
			}
		}
    	xstart=0;
    	for(i=0;i<n;i++){
            if(xptr)xptr2=*xptr;
            else xfptr2=*xfptr;
            if(yptr)yptr2=*yptr;
            else yfptr2=*yfptr;
    		ymax=0;
    		ymin=1000000;
    		main_fill_poly_vertex_count=0;
        	xcount = (int)(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
            if(argc > 7 && *argv[8]){
            	fill_set_pen_color((cc[i]>>16) & 0xFF, (cc[i]>>8) & 0xFF , cc[i] & 0xFF);
        		fill_set_fill_color((ff[i]>>16) & 0xFF, (ff[i]>>8) & 0xFF , ff[i] & 0xFF);
            	fill_begin_fill();
            }
           for(j=xstart;j<xstart+xcount-1;j++){
            	if(argc > 7){
                    main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
                    main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
                    if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
                    if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
                    main_fill_poly_vertex_count++;
            	} else {
            		int x1=(xfptr==NULL ? *xptr++ : (int)*xfptr++);
            		int x2=(xfptr==NULL ? *xptr : (int)*xfptr);
            		int y1=(yfptr==NULL ? *yptr++ : (int)*yfptr++);
            		int y2=(yfptr==NULL ? *yptr : (int)*yfptr);
            		DrawLine(x1,y1,x2,y2, 1, cc[i]);
            	}
            }
            if(argc > 7){
                main_fill_polyX[main_fill_poly_vertex_count] = (xfptr==NULL ? (TFLOAT )*xptr++ : (TFLOAT )*xfptr++) ;
                main_fill_polyY[main_fill_poly_vertex_count] = (yfptr==NULL ? (TFLOAT )*yptr++ : (TFLOAT )*yfptr++) ;
                if(main_fill_polyY[main_fill_poly_vertex_count]>ymax)ymax=main_fill_polyY[main_fill_poly_vertex_count];
                if(main_fill_polyY[main_fill_poly_vertex_count]<ymin)ymin=main_fill_polyY[main_fill_poly_vertex_count];
                if(main_fill_polyY[main_fill_poly_vertex_count]!=main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]){
                    	main_fill_poly_vertex_count++;
                    	main_fill_polyX[main_fill_poly_vertex_count]=main_fill_polyX[0];
                    	main_fill_polyY[main_fill_poly_vertex_count]=main_fill_polyY[0];
                }
                main_fill_poly_vertex_count++;
            	if(main_fill_poly_vertex_count>5){
            		fill_end_fill(xcount,ymin,ymax);
            	} else if(main_fill_poly_vertex_count==5){
    				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],ff[i],ff[i]);
    				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],ff[i],ff[i]);
    				if(ff[i]!=cc[i]){
    					DrawLine(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],1,cc[i]);
    					DrawLine(main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],1,cc[i]);
    					DrawLine(main_fill_polyX[2],main_fill_polyY[2],main_fill_polyX[3],main_fill_polyY[3],1,cc[i]);
    					DrawLine(main_fill_polyX[3],main_fill_polyY[3],main_fill_polyX[4],main_fill_polyY[4],1,cc[i]);
    				}
    			} else {
    				DrawTriangle(main_fill_polyX[0],main_fill_polyY[0],main_fill_polyX[1],main_fill_polyY[1],main_fill_polyX[2],main_fill_polyY[2],cc[i],ff[i]);
            	}
            } else {
        		int x1=(xfptr==NULL ? *xptr : (int)*xfptr);
        		int x2=(xfptr==NULL ? xptr2 : (int)xfptr2);
        		int y1=(yfptr==NULL ? *yptr : (int)*yfptr);
        		int y2=(yfptr==NULL ? yptr2 : (int)yfptr2);
        		DrawLine(x1,y1,x2,y2, 1, cc[i]);
            	if(xfptr!=NULL)xfptr++;
            	else xptr++;
            	if(yfptr!=NULL)yfptr++;
            	else yptr++;
            }

            xstart+=xcount;
    	}
    }
}

*/
void cmd_circle(void) {
    int x, y, r, w=0, c=0, f=0, n=0 ,i, nc=0, nw=0, nf=0, na=0;
    MMFLOAT a;
    long long int *xptr, *yptr, *rptr, *fptr, *wptr, *cptr, *aptr;
    MMFLOAT *xfptr, *yfptr, *rfptr, *ffptr, *wfptr, *cfptr, *afptr;
    getargs(&cmdline, 13,",");
    if(!(argc & 1) || argc < 5) error("Argument count");
    getargaddress(argv[0], &xptr, &xfptr, &n);
    if(n != 1) {
        getargaddress(argv[2], &yptr, &yfptr, &n);
        getargaddress(argv[4], &rptr, &rfptr, &n);
    }
    if(n==1){
        w = 1; c = gui_fcolour; f = -1; a = 1;                          // setup the defaults
        x = getinteger(argv[0]);
        y = getinteger(argv[2]);
        r = getinteger(argv[4]);
        if(argc > 5 && *argv[6]) w = getint(argv[6], 0, 100);
        if(argc > 7 && *argv[8]) a = getnumber(argv[8]);
        if(argc > 9 && *argv[10]) c = getint(argv[10], 0, WHITE);
        if(argc > 11) f = getint(argv[12], -1, WHITE);
        int save_refresh=Option.Refresh;
        Option.Refresh=0;
        DrawCircle(x, y, r, w, c, f, a);
        Option.Refresh=save_refresh;
    } else {
        w = 1; c = gui_fcolour; f = -1; a = 1;                          // setup the defaults
        if(argc > 5 && *argv[6]) {
            getargaddress(argv[6], &wptr, &wfptr, &nw); 
            if(nw == 1) w = getint(argv[6], 0, 100);
            else if(nw>1) {
                if(nw > 1 && nw < n) n=nw; //adjust the dimensionality
                for(i=0;i<nw;i++){
                    w = (wfptr == NULL ? wptr[i] : (int)wfptr[i]);
                    if(w < 0 || w > 100) error("% is invalid (valid is % to %)", (int)w, 0, 100);
                }
            }
        }
        if(argc > 7 && *argv[8]){
            getargaddress(argv[8], &aptr, &afptr, &na); 
            if(na == 1) a = getnumber(argv[8]);
            if(na > 1 && na < n) n=na; //adjust the dimensionality
        }
        if(argc > 9 && *argv[10]){
            getargaddress(argv[10], &cptr, &cfptr, &nc); 
            if(nc == 1) c = getint(argv[10], 0, WHITE);
            else if(nc>1) {
                if(nc > 1 && nc < n) n=nc; //adjust the dimensionality
                for(i=0;i<nc;i++){
                    c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                    if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        if(argc > 11){
            getargaddress(argv[12], &fptr, &ffptr, &nf); 
            if(nf == 1) f = getint(argv[12], -1, WHITE);
            else if(nf>1) {
                if(nf > 1 && nf < n) n=nf; //adjust the dimensionality
                for(i=0;i<nf;i++){
                    f = (ffptr == NULL ? fptr[i] : (int)ffptr[i]);
                    if(f < 0 || f > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        int save_refresh=Option.Refresh;
        Option.Refresh=0;
        for(i=0;i<n;i++){
            x = (xfptr==NULL ? xptr[i] : (int)xfptr[i]);
            y = (yfptr==NULL ? yptr[i] : (int)yfptr[i]);
            r = (rfptr==NULL ? rptr[i] : (int)rfptr[i])-1;
            if(nw > 1) w = (wfptr==NULL ? wptr[i] : (int)wfptr[i]);
            if(nc > 1) c = (cfptr==NULL ? cptr[i] : (int)cfptr[i]);
            if(nf > 1) f = (ffptr==NULL ? fptr[i] : (int)ffptr[i]);
			if(na > 1) a = (afptr==NULL ? (MMFLOAT)aptr[i] : afptr[i]);
            DrawCircle(x, y, r, w, c, f, a);
        }
        Option.Refresh=save_refresh;
    }
    if(Option.Refresh)Display_Refresh();
}
/*
static short xb0,xb1,yb0,yb1;
void drawAAPixel( int x , int y , MMFLOAT brightness, uint32_t c){
    union colourmap
    {
        unsigned char rgbbytes[4];
        unsigned int rgb;
    } col;
	col.rgb=c;
	col.rgbbytes[0]= (unsigned char)((MMFLOAT)col.rgbbytes[0]*brightness);
	col.rgbbytes[1]= (unsigned char)((MMFLOAT)col.rgbbytes[1]*brightness);
	col.rgbbytes[2]= (unsigned char)((MMFLOAT)col.rgbbytes[2]*brightness);
 	//if(((x>=xb0 && x<=xb1) && (y>=yb0 && y<=yb1)))DrawPixel(x,y,col.rgb);
 	DrawPixel(x,y,col.rgb);  //fix for hor and vert not correct LW
//    else PInt(x),PIntComma(y);PRet();
}

void drawAALine(MMFLOAT x0 , MMFLOAT y0 , MMFLOAT x1 , MMFLOAT y1, uint32_t c, int w)
{
// Ensure positive integer values for width
	if(w < 1) w = 1;

// If drawing a dot, the call drawDot function
//if Math.abs(y1 - y0) < 1.0 && Math.abs(x1 - x0) < 1.0
//  #drawDot (x0 + x1) / 2, (y0 + y1) / 2
//  return
    xb0=x0;xb1=x1;yb0=y0;yb1=y1;
    if(xb1<xb0)swap(xb1,xb0);
    if(yb1<yb0)swap(yb1,yb0);

// steep means that m > 1
	int steep = abs(y1 - y0) > abs(x1 - x0) ;
// swap the co-ordinates if slope > 1 or we
// draw backwards
	if (steep)
	{
		swap(x0 , y0);
		swap(x1 , y1);
	}
	if (x0 > x1)
	{
		swap(x0 ,x1);
		swap(y0 ,y1);
	}
	//compute the slope
	MMFLOAT dx = x1-x0;
	MMFLOAT dy = y1-y0;

	MMFLOAT gradient;
	if (dx <= 0.0) gradient = 1;
    else gradient=dy/dx;


//rotate w
	w = w * sqrt(1 + (gradient * gradient));

// Handle first endpoint
	MMFLOAT xend = round(x0);
	MMFLOAT yend = y0 - (w - 1) * 0.5 + gradient * (xend - x0);
	MMFLOAT xgap = 1 - (x0 + 0.5 - xend);
	MMFLOAT xpxl1 = xend; //this will be used in the main loop
	MMFLOAT ypxl1 = floor(yend);
	MMFLOAT fpart = yend - floor(yend);
	MMFLOAT rfpart = 1.0 - fpart;

	if(steep){
	  drawAAPixel(ypxl1    , xpxl1, rfpart * xgap, c);
	  for(int i=1;i<=w;i++) drawAAPixel(ypxl1 + i, xpxl1, 1, c);
	  drawAAPixel(ypxl1 + w, xpxl1,  fpart * xgap, c);
	} else {
	  drawAAPixel(xpxl1, ypxl1    , rfpart * xgap, c);
	  for(int i=1; i<=w; i++) drawAAPixel(xpxl1, ypxl1 + i, 1, c);
	  drawAAPixel(xpxl1, ypxl1 + w,  fpart * xgap, c);
	}
	MMFLOAT intery = yend + gradient; // first y-intersection for the main loop

// Handle second endpoint
	xend = round(x1);
	yend = y1 - (w - 1) * 0.5 + gradient * (xend - x1);
	xgap = 1 - (x1 + 0.5 - xend);
	MMFLOAT xpxl2 = xend; // this will be used in the main loop
	MMFLOAT ypxl2 = floor(yend);
	fpart = yend - floor(yend);
	rfpart = 1 - fpart;

	if(steep){
		drawAAPixel(ypxl2    , xpxl2, rfpart * xgap, c);
		for(int i=1;i<=w;i++) drawAAPixel(ypxl2 + i, xpxl2, 1, c);
		drawAAPixel(ypxl2 + w, xpxl2,  fpart * xgap, c);
	} else {
		drawAAPixel(xpxl2, ypxl2    , rfpart * xgap, c);
		for(int i=1; i<=w; i++) drawAAPixel(xpxl2, ypxl2 + i, 1, c);
		drawAAPixel(xpxl2, ypxl2 + w,  fpart * xgap, c);
	}
// main loop
	if(steep){
		for(int x=xpxl1 + 1; x<=xpxl2; x++){
			fpart = intery - floor(intery);
			rfpart = 1 - fpart;
			MMFLOAT y = floor(intery);
			drawAAPixel(y    , x, rfpart, c);
			for(int i=1;i<=w;i++) drawAAPixel(y + i, x, 1, c);
			drawAAPixel(y + w, x,  fpart, c);
			intery = intery + gradient;
		}
	} else {
		for(int x=xpxl1 + 1; x<=xpxl2; x++){
			fpart = intery - floor(intery);
			rfpart = 1 - fpart;
			MMFLOAT y = floor(intery);
			drawAAPixel(x, y    , rfpart, c);
			for(int i=1;i<=w;i++) drawAAPixel(x, y + i, 1, c);
			drawAAPixel(x, y + w,  fpart, c);
			intery = intery + gradient;
		}
	}
}
*/
void cmd_line(void) {
    if(Option.DISPLAY_TYPE == 0) error("Display not configured");
    char *p;
    int x1, y1, x2, y2, w=0, c=0, n=0 ,i, nc=0, nw=0;

         if((p=checkstring(cmdline,"PLOT"))){
           long long int *y1ptr;
           MMFLOAT *y1fptr;
           int xs=0,xinc=1;
           int ys=0,yinc=1;
           getargs(&p, 13,",");
           getargaddress(argv[0], &y1ptr, &y1fptr, &n);
           if(n==1)error("Argument 1 is not an array");
           nc=n;
           if(argc>=3 && *argv[2])nc=getint(argv[2],1,HRes-1);
           if(nc>n)nc=n;
           if(argc>=5 && *argv[4])xs=getint(argv[4],0,HRes-1);
           if(argc>=7 && *argv[6])xinc=getint(argv[6],1,HRes-1);
           if(argc>=9 && *argv[8])ys=getint(argv[8],OptionBase,n-2+OptionBase);
           if(argc>=11 && *argv[10])yinc=getint(argv[10],1,n-1);
           c = gui_fcolour;  w = 1;                                        // setup the defaults
           if(argc == 13) c = getint(argv[12], 0, WHITE);
           int y=ys-OptionBase;
           for(i=0;i<(nc-1);i++){
              if(y>=nc)break;
              if(y+yinc>=nc)break;
              x1 = xs+i*xinc;
              y1 = (y1fptr==NULL ? y1ptr[y] : (int)y1fptr[y]);
              if(y1<0)y1=0;
              if(y1>=VRes)y1=VRes-1;
              x2 = xs+(i+1)*xinc;
              y2 = (y1fptr==NULL ? y1ptr[y+yinc] : (int)y1fptr[y+yinc]);
              if(x1>=HRes)break; //can only get worse so stop now
              if(x2>=HRes)x2=HRes-1;
              if(y2<0)y2=0;
              if(y2>=VRes)y2=VRes-1;
              DrawLine(x1, y1, x2, y2, w, c);
              y+=yinc;
           }
		} else if((p=checkstring(cmdline,"GRAPH"))){
            char *pp=GetTempMemory(STRINGSIZE);
            strcpy((char *)pp,(char *)p);
            memmove(&pp[2],pp,strlen((char *)p)+1);
            pp[0]='0';
            pp[1]=',';
            polygon(pp,0);
            return;
/*		} else if((p=checkstring(cmdline,"AA"))){
			MMFLOAT x1, y1, x2, y2;
			getargs(&p, 11,",");
			c = gui_fcolour;  ;  w = 1;                                         // setup the defaults
			x1 = getnumber(argv[0]);
			y1 = getnumber(argv[2]);
			x2 = getnumber(argv[4]);
			y2 = getnumber(argv[6]);
			if(argc > 7 && *argv[8]){
				w = getint(argv[8], 1, 100);
			}
            if(argc == 11) c = getint(argv[10], 0, WHITE);
			drawAALine(x1, y1, x2, y2, c, w);
			return;
*/
		} else {
            long long int *x1ptr, *y1ptr, *x2ptr, *y2ptr, *wptr, *cptr;
            MMFLOAT *x1fptr, *y1fptr, *x2fptr, *y2fptr, *wfptr, *cfptr;
            getargs(&cmdline, 11,",");
            if(!(argc & 1) || argc < 3) error("Argument count");
            getargaddress(argv[0], &x1ptr, &x1fptr, &n);
            if(n != 1) {
                if(argc<7)error("Argument count");
                getargaddress(argv[2], &y1ptr, &y1fptr, &n);
                getargaddress(argv[4], &x2ptr, &x2fptr, &n);
                getargaddress(argv[6], &y2ptr, &y2fptr, &n);
            }
            if(n==1){
                c = gui_fcolour;  w = 1;                                        // setup the defaults
                x1 = getinteger(argv[0]);
                y1 = getinteger(argv[2]);
                if(argc>=5 && *argv[4])x2 = getinteger(argv[4]);
                else {
                    x2=CurrentX;CurrentX=x1;
                }
                if(argc>=7 && *argv[6])y2 = getinteger(argv[6]);
                else {
                    y2=CurrentY;CurrentY=y1;
                }
                /* start of fix from Picomite RC9 */
                if(x1==CurrentX && y1==CurrentY){
                    CurrentX=x2;
                    CurrentY=y2;
                }
                /***** End of fix ****************/
                if(argc > 7 && *argv[8]){
                    w = getint(argv[8], 1, 100);
                }
                if(argc == 11) c = getint(argv[10], 0, WHITE);
                DrawLine(x1, y1, x2, y2, w, c);
            } else {
                c = gui_fcolour;  w = 1;                                        // setup the defaults
                if(argc > 7 && *argv[8]){
                    getargaddress(argv[8], &wptr, &wfptr, &nw);
                    if(nw == 1) w = getint(argv[8], 0, 100);
                    else if(nw>1) {
                        if(nw > 1 && nw < n) n=nw; //adjust the dimensionality
                        for(i=0;i<nw;i++){
                            w = (wfptr == NULL ? wptr[i] : (int)wfptr[i]);
                            if(w < 0 || w > 100) error("% is invalid (valid is % to %)", (int)w, 0, 100);
                        }
                    }
                }

                if(argc == 11){
                    getargaddress(argv[10], &cptr, &cfptr, &nc);
                    if(nc == 1) c = getint(argv[10], 0, WHITE);
                    else if(nc>1) {
                        if(nc > 1 && nc < n) n=nc; //adjust the dimensionality
                        for(i=0;i<nc;i++){
                            c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                            if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                        }
                    }
                }
                for(i=0;i<n;i++){
                    x1 = (x1fptr==NULL ? x1ptr[i] : (int)x1fptr[i]);
                    y1 = (y1fptr==NULL ? y1ptr[i] : (int)y1fptr[i]);
                    x2 = (x2fptr==NULL ? x2ptr[i] : (int)x2fptr[i]);
                    y2 = (y2fptr==NULL ? y2ptr[i] : (int)y2fptr[i]);
                    if(nw > 1) w = (wfptr==NULL ? wptr[i] : (int)wfptr[i]);
                    if(nc > 1) c = (cfptr==NULL ? cptr[i] : (int)cfptr[i]);
                    DrawLine(x1, y1, x2, y2, w, c);
                }
            }
        }
}


/*
void cmd_line(void) {
    int x1, y1, x2, y2, w=0, c=0, n=0 ,i, nc=0, nw=0;
    long long int *x1ptr, *y1ptr, *x2ptr, *y2ptr, *wptr, *cptr;
    MMFLOAT *x1fptr, *y1fptr, *x2fptr, *y2fptr, *wfptr, *cfptr;
    getargs(&cmdline, 11,",");
    if(!(argc & 1) || argc < 7) error("Argument count");
    getargaddress(argv[0], &x1ptr, &x1fptr, &n);
    if(n != 1) {
        getargaddress(argv[2], &y1ptr, &y1fptr, &n);
        getargaddress(argv[4], &x2ptr, &x2fptr, &n);
        getargaddress(argv[6], &y2ptr, &y2fptr, &n);
    }
    if(n==1){
        c = gui_fcolour;  w = 1;                                        // setup the defaults
        x1 = getinteger(argv[0]);
        y1 = getinteger(argv[2]);
        x2 = getinteger(argv[4]);
        y2 = getinteger(argv[6]);
        if(argc > 7 && *argv[8]){
            w = getint(argv[8], 1, 100);
        }
        if(argc == 11) c = getint(argv[10], 0, WHITE);
        DrawLine(x1, y1, x2, y2, w, c);        
    } else {
        c = gui_fcolour;  w = 1;                                        // setup the defaults
        if(argc > 7 && *argv[8]){
            getargaddress(argv[8], &wptr, &wfptr, &nw); 
            if(nw == 1) w = getint(argv[8], 0, 100);
            else if(nw>1) {
                if(nw > 1 && nw < n) n=nw; //adjust the dimensionality
                for(i=0;i<nw;i++){
                    w = (wfptr == NULL ? wptr[i] : (int)wfptr[i]);
                    if(w < 0 || w > 100) error("% is invalid (valid is % to %)", (int)w, 0, 100);
                }
            }
        }
        if(argc == 11){
            getargaddress(argv[10], &cptr, &cfptr, &nc); 
            if(nc == 1) c = getint(argv[10], 0, WHITE);
            else if(nc>1) {
                if(nc > 1 && nc < n) n=nc; //adjust the dimensionality
                for(i=0;i<nc;i++){
                    c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                    if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        for(i=0;i<n;i++){
            x1 = (x1fptr==NULL ? x1ptr[i] : (int)x1fptr[i]);
            y1 = (y1fptr==NULL ? y1ptr[i] : (int)y1fptr[i]);
            x2 = (x2fptr==NULL ? x2ptr[i] : (int)x2fptr[i]);
            y2 = (y2fptr==NULL ? y2ptr[i] : (int)y2fptr[i]);
            if(nw > 1) w = (wfptr==NULL ? wptr[i] : (int)wfptr[i]);
            if(nc > 1) c = (cfptr==NULL ? cptr[i] : (int)cfptr[i]);
            DrawLine(x1, y1, x2, y2, w, c);
        }
    }
}
*/

void cmd_box(void) {
    int x1, y1, wi, h, w=0, c=0, f=0,  n=0 ,i, nc=0, nw=0, nf=0,hmod,wmod;
    long long int *x1ptr, *y1ptr, *wiptr, *hptr, *wptr, *cptr, *fptr;
    MMFLOAT *x1fptr, *y1fptr, *wifptr, *hfptr, *wfptr, *cfptr, *ffptr;
    getargs(&cmdline, 13,",");
    if(!(argc & 1) || argc < 7) error("Argument count");
    getargaddress(argv[0], &x1ptr, &x1fptr, &n);
    if(n != 1) {
        getargaddress(argv[2], &y1ptr, &y1fptr, &n);
        getargaddress(argv[4], &wiptr, &wifptr, &n);
        getargaddress(argv[6], &hptr, &hfptr, &n);
    }
    if(n == 1){
        c = gui_fcolour; w = 1; f = -1;                                 // setup the defaults
        x1 = getinteger(argv[0]);
        y1 = getinteger(argv[2]);
        wi = getinteger(argv[4]) ;
        h = getinteger(argv[6]) ;
        wmod=(wi > 0 ? -1 : 1);
        hmod=(h > 0 ? -1 : 1);
        if(argc > 7 && *argv[8]) w = getint(argv[8], 0, 100);
        if(argc > 9 && *argv[10]) c = getint(argv[10], 0, WHITE);
        if(argc == 13) f = getint(argv[12], -1, WHITE);
        if(wi != 0 && h != 0) DrawBox(x1, y1, x1 + wi + wmod, y1 + h + hmod, w, c, f);
    } else {
        c = gui_fcolour;  w = 1;                                        // setup the defaults
        if(argc > 7 && *argv[8]){
            getargaddress(argv[8], &wptr, &wfptr, &nw); 
            if(nw == 1) w = getint(argv[8], 0, 100);
            else if(nw>1) {
                if(nw > 1 && nw < n) n=nw; //adjust the dimensionality
                for(i=0;i<nw;i++){
                    w = (wfptr == NULL ? wptr[i] : (int)wfptr[i]);
                    if(w < 0 || w > 100) error("% is invalid (valid is % to %)", (int)w, 0, 100);
                }
            }
        }
        if(argc > 9 && *argv[10]) {
            getargaddress(argv[10], &cptr, &cfptr, &nc); 
            if(nc == 1) c = getint(argv[10], 0, WHITE);
            else if(nc>1) {
                if(nc > 1 && nc < n) n=nc; //adjust the dimensionality
                for(i=0;i<nc;i++){
                    c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                    if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        if(argc == 13){
            getargaddress(argv[12], &fptr, &ffptr, &nf); 
            if(nf == 1) c = getint(argv[12], 0, WHITE);
            else if(nf>1) {
                if(nf > 1 && nf < n) n=nf; //adjust the dimensionality
                for(i=0;i<nf;i++){
                    f = (ffptr == NULL ? fptr[i] : (int)ffptr[i]);
                    if(f < -1 || f > WHITE) error("% is invalid (valid is % to %)", (int)c, -1, WHITE);
                }
            }
        }
        for(i=0;i<n;i++){
            x1 = (x1fptr==NULL ? x1ptr[i] : (int)x1fptr[i]);
            y1 = (y1fptr==NULL ? y1ptr[i] : (int)y1fptr[i]);
            wi = (wifptr==NULL ? wiptr[i] : (int)wifptr[i]);
            h =  (hfptr==NULL ? hptr[i] : (int)hfptr[i]);
            wmod=(wi > 0 ? -1 : 1);
            hmod=(h > 0 ? -1 : 1);
            if(nw > 1) w = (wfptr==NULL ? wptr[i] : (int)wfptr[i]);
            if(nc > 1) c = (cfptr==NULL ? cptr[i] : (int)cfptr[i]);
            if(nf > 1) f = (ffptr==NULL ? fptr[i] : (int)ffptr[i]);
            if(wi != 0 && h != 0) DrawBox(x1, y1, x1 + wi + wmod, y1 + h + hmod, w, c, f);

        }
    }
}



void cmd_rbox(void) {
    int x1, y1, wi, h, w=0, c=0, f=0,  r=0, n=0 ,i, nc=0, nw=0, nf=0,hmod,wmod;
    long long int *x1ptr, *y1ptr, *wiptr, *hptr, *wptr, *cptr, *fptr;
    MMFLOAT *x1fptr, *y1fptr, *wifptr, *hfptr, *wfptr, *cfptr, *ffptr;
    getargs(&cmdline, 13,",");
    if(!(argc & 1) || argc < 7) error("Argument count");
    getargaddress(argv[0], &x1ptr, &x1fptr, &n);
    if(n != 1) {
        getargaddress(argv[2], &y1ptr, &y1fptr, &n);
        getargaddress(argv[4], &wiptr, &wifptr, &n);
        getargaddress(argv[6], &hptr, &hfptr, &n);
    }
    if(n == 1){
        c = gui_fcolour; w = 1; f = -1; r = 10;                         // setup the defaults
        x1 = getinteger(argv[0]);
        y1 = getinteger(argv[2]);
        w = getinteger(argv[4]);
        h = getinteger(argv[6]);
        wmod=(w > 0 ? -1 : 1);
        hmod=(h > 0 ? -1 : 1);
        if(argc > 7 && *argv[8]) r = getint(argv[8], 0, 100);
        if(argc > 9 && *argv[10]) c = getint(argv[10], 0, WHITE);
        if(argc == 13) f = getint(argv[12], -1, WHITE);
        if(w != 0 && h != 0) DrawRBox(x1, y1, x1 + w + wmod, y1 + h + hmod, r, c, f);
    } else {
        c = gui_fcolour;  w = 1;                                        // setup the defaults
        if(argc > 7 && *argv[8]){
            getargaddress(argv[8], &wptr, &wfptr, &nw); 
            if(nw == 1) w = getint(argv[8], 0, 100);
            else if(nw>1) {
                if(nw > 1 && nw < n) n=nw; //adjust the dimensionality
                for(i=0;i<nw;i++){
                    w = (wfptr == NULL ? wptr[i] : (int)wfptr[i]);
                    if(w < 0 || w > 100) error("% is invalid (valid is % to %)", (int)w, 0, 100);
                }
            }
        }
        if(argc > 9 && *argv[10]) {
            getargaddress(argv[10], &cptr, &cfptr, &nc); 
            if(nc == 1) c = getint(argv[10], 0, WHITE);
            else if(nc>1) {
                if(nc > 1 && nc < n) n=nc; //adjust the dimensionality
                for(i=0;i<nc;i++){
                    c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                    if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        if(argc == 13){
            getargaddress(argv[12], &fptr, &ffptr, &nf); 
            if(nf == 1) c = getint(argv[12], 0, WHITE);
            else if(nf>1) {
                if(nf > 1 && nf < n) n=nf; //adjust the dimensionality
                for(i=0;i<nf;i++){
                    f = (ffptr == NULL ? fptr[i] : (int)ffptr[i]);
                    if(f < -1 || f > WHITE) error("% is invalid (valid is % to %)", (int)c, -1, WHITE);
                }
            }
        }
        for(i=0;i<n;i++){
            x1 = (x1fptr==NULL ? x1ptr[i] : (int)x1fptr[i]);
            y1 = (y1fptr==NULL ? y1ptr[i] : (int)y1fptr[i]);
            wi = (wifptr==NULL ? wiptr[i] : (int)wifptr[i]);
            h =  (hfptr==NULL ? hptr[i] : (int)hfptr[i]);
            wmod=(wi > 0 ? -1 : 1);
            hmod=(h > 0 ? -1 : 1);
            if(nw > 1) w = (wfptr==NULL ? wptr[i] : (int)wfptr[i]);
            if(nc > 1) c = (cfptr==NULL ? cptr[i] : (int)cfptr[i]);
            if(nf > 1) f = (ffptr==NULL ? fptr[i] : (int)ffptr[i]);
            if(wi != 0 && h != 0) DrawRBox(x1, y1, x1 + wi + wmod, y1 + h + hmod, w, c, f);
        }
    }
}



// these three functions were written by Peter Mather (matherp on the Back Shed forum)
// read the contents of a pIXEL out of screen memory
void fun_pixel(void) {
    if((void *)ReadBuffer == (void *)DisplayNotSet) error("function PIXEL not available on this display");
    union colourmap
    {
        char rgbbytes[4];
        unsigned int rgb;
    } c;
    int x,y;
	getargs(&ep, 3,",");
	if(argc != 3) error("Argument count");
    x = getinteger(argv[0]);
    y = getinteger(argv[2]);
    ReadBuffer(x,y,x,y,(char *)&c.rgb);
    iret = c.rgb & 0xFFFFFF;
    targ = T_INT;
}

void cmd_triangle(void) {                                           // thanks to Peter Mather (matherp on the Back Shed forum)
    int x1, y1, x2, y2, x3, y3, c=0, f=0,  n=0,i, nc=0, nf=0;
    long long int *x3ptr, *y3ptr, *x1ptr, *y1ptr, *x2ptr, *y2ptr, *fptr, *cptr;
    MMFLOAT *x3fptr, *y3fptr, *x1fptr, *y1fptr, *x2fptr, *y2fptr, *ffptr, *cfptr;
    getargs(&cmdline, 15,",");
    if(!(argc & 1) || argc < 11) error("Argument count");
    getargaddress(argv[0], &x1ptr, &x1fptr, &n);
    if(n != 1) {
    	int cn=n;
        getargaddress(argv[2], &y1ptr, &y1fptr, &n);
        if(n<cn)cn=n;
        getargaddress(argv[4], &x2ptr, &x2fptr, &n);
        if(n<cn)cn=n;
        getargaddress(argv[6], &y2ptr, &y2fptr, &n);
        if(n<cn)cn=n;
        getargaddress(argv[8], &x3ptr, &x3fptr, &n);
        if(n<cn)cn=n;
        getargaddress(argv[10], &y3ptr, &y3fptr, &n);
        if(n<cn)cn=n;
        n=cn;
    }
    if(n == 1){
        c = gui_fcolour; f = -1;
        x1 = getinteger(argv[0]);
        y1 = getinteger(argv[2]);
        x2 = getinteger(argv[4]);
        y2 = getinteger(argv[6]);
        x3 = getinteger(argv[8]);
        y3 = getinteger(argv[10]);
        if(argc >= 13 && *argv[12]) c = getint(argv[12], BLACK, WHITE);
        if(argc == 15) f = getint(argv[14], -1, WHITE);
        DrawTriangle(x1, y1, x2, y2, x3, y3, c, f);
    } else {
        c = gui_fcolour; f = -1;
        if(argc >= 13 && *argv[12]) {
            getargaddress(argv[12], &cptr, &cfptr, &nc); 
            if(nc == 1) c = getint(argv[10], 0, WHITE);
            else if(nc>1) {
                if(nc > 1 && nc < n) n=nc; //adjust the dimensionality
                for(i=0;i<nc;i++){
                    c = (cfptr == NULL ? cptr[i] : (int)cfptr[i]);
                    if(c < 0 || c > WHITE) error("% is invalid (valid is % to %)", (int)c, 0, WHITE);
                }
            }
        }
        if(argc == 15){
            getargaddress(argv[14], &fptr, &ffptr, &nf); 
            if(nf == 1) c = getint(argv[14], -1, WHITE);
            else if(nf>1) {
                if(nf > 1 && nf < n) n=nf; //adjust the dimensionality
                for(i=0;i<nf;i++){
                    f = (ffptr == NULL ? fptr[i] : (int)ffptr[i]);
                    if(f < -1 || f > WHITE) error("% is invalid (valid is % to %)", (int)c, -1, WHITE);
                }
            }
        }
        for(i=0;i<n;i++){
            x1 = (x1fptr==NULL ? x1ptr[i] : (int)x1fptr[i]);
            y1 = (y1fptr==NULL ? y1ptr[i] : (int)y1fptr[i]);
            x2 = (x2fptr==NULL ? x2ptr[i] : (int)x2fptr[i]);
            y2 = (y2fptr==NULL ? y2ptr[i] : (int)y2fptr[i]);
            x3 = (x3fptr==NULL ? x3ptr[i] : (int)x3fptr[i]);
            y3 = (y3fptr==NULL ? y3ptr[i] : (int)y3fptr[i]);
            if(x1==x2 && x1==x3 && y1==y2 && y1==y3 && x1==-1 && y1==-1)return;
            if(nc > 1) c = (cfptr==NULL ? cptr[i] : (int)cfptr[i]);
            if(nf > 1) f = (ffptr==NULL ? fptr[i] : (int)ffptr[i]);
            DrawTriangle(x1, y1, x2, y2, x3, y3, c, f);
        }
    }
}

void cmd_cls(void) {
    HideAllControls();
    skipspace(cmdline);
    if(!(*cmdline == 0 || *cmdline == '\''))
        ClearScreen(getint(cmdline, 0, WHITE));
    else
        ClearScreen(gui_bcolour);
    CurrentX = CurrentY = 0;
}



void fun_rgb(void) {
    getargs(&ep, 5, ",");
    if(argc == 5)
        iret = rgb(getint(argv[0], 0, 255), getint(argv[2], 0, 255), getint(argv[4], 0, 255));
    else if(argc == 1) {
            if(checkstring(argv[0], "WHITE"))        iret = WHITE;
            else if(checkstring(argv[0], "BLACK"))   iret = BLACK;
            else if(checkstring(argv[0], "BLUE"))    iret = BLUE;
            else if(checkstring(argv[0], "GREEN"))   iret = GREEN;
            else if(checkstring(argv[0], "CYAN"))    iret = CYAN;
            else if(checkstring(argv[0], "RED"))     iret = RED;
            else if(checkstring(argv[0], "MAGENTA")) iret = MAGENTA;
            else if(checkstring(argv[0], "YELLOW"))  iret = YELLOW;
            else if(checkstring(argv[0], "BROWN"))   iret = BROWN;
            else if(checkstring(argv[0], "GRAY"))    iret = GRAY;
            else if(checkstring(argv[0], "GREY"))    iret = GRAY;
            else error("Invalid colour: $", argv[0]);
    } else
        error("Syntax");
    targ = T_INT;
}



void fun_mmhres(void) {
    iret = HRes;
    targ = T_INT;
}



void fun_mmvres(void) {
    iret = VRes;
    targ = T_INT;
}

/*************************************************************************
 The FONT command will no longer change the PromptFont. In commandline mode
 with Option.DISPLAY_CONSOLE this means the font will revert to the PrompFont
 when a command completes.
 *************************************************************************/

void cmd_font(void) {
    getargs(&cmdline, 3, ",");
    if(argc < 1) error("Argument count");
    if(*argv[0] == '#') ++argv[0];
    if(argc == 3)
        SetFont(((getint(argv[0], 1, FONT_TABLE_SIZE) - 1) << 4) | getint(argv[2], 1, 15));
    else
        SetFont(((getint(argv[0], 1, FONT_TABLE_SIZE) - 1) << 4) | 1);

    //if(Option.DISPLAY_CONSOLE && !CurrentLinePtr) {                 // if we are at the command prompt on the LCD
    //    PromptFont = gui_font;
    //    if(CurrentY + gui_font_height >= VRes) {
    //        ScrollLCD(CurrentY + gui_font_height - VRes);           // scroll up if the font change split the line over the bottom
    //        CurrentY -= (CurrentY + gui_font_height - VRes);
    //    }
    //}
}



void cmd_colour(void) {
    getargs(&cmdline, 3, ",");
    if(argc < 1) error("Argument count");
    gui_fcolour = getint(argv[0], 0, WHITE);
    if(argc == 3)
        gui_bcolour = getint(argv[2], -1, WHITE);
    last_fcolour = gui_fcolour;
    last_bcolour = gui_bcolour;
    if(!CurrentLinePtr) {
        PromptFC = gui_fcolour;
        PromptBC = gui_bcolour;
    }
}


void fun_mmcharwidth(void) {
    if(Option.DISPLAY_TYPE == 0) error("Display not configured");
    iret = FontTable[gui_font >> 4][0] * (gui_font & 0b1111);
    targ = T_INT;
}


void fun_mmcharheight(void) {
    if(Option.DISPLAY_TYPE == 0) error("Display not configured");
    iret = FontTable[gui_font >> 4][1] * (gui_font & 0b1111);
    targ = T_INT;
}






/****************************************************************************************************

 General purpose drawing routines

****************************************************************************************************/


void DrawPixel(int x, int y, int c) {
    DrawRectangle(x, y, x, y, c);
}


void ClearScreen(int c) {
    DrawRectangle(0, 0, HRes - 1, VRes - 1, c);
    if(Option.Refresh)Display_Refresh();
}


/**************************************************************************************************
Draw a line on a the video output
	x1, y1 - the start coordinate
	x2, y2 - the end coordinate
    w - the width of the line (ignored for diagional lines)
	c - the colour to use
***************************************************************************************************/
#define abs( a)     (((a)> 0) ? (a) : -(a))

void DrawLine(int x1, int y1, int x2, int y2, int w, int c) {


    if(y1 == y2) {
        DrawRectangle(x1, y1, x2, y2 + w - 1, c);                   // horiz line
        if(Option.Refresh)Display_Refresh();
        return;
    }
    if(x1 == x2) {
        DrawRectangle(x1, y1, x2 + w - 1, y2, c);                   // vert line
        if(Option.Refresh)Display_Refresh();
        return;
    }
    int  dx, dy, sx, sy, err, e2;
    dx = abs(x2 - x1); sx = x1 < x2 ? 1 : -1;
    dy = -abs(y2 - y1); sy = y1 < y2 ? 1 : -1;
    err = dx + dy;
    while(1) {
        DrawBuffered(x1, y1, c,0);
        e2 = 2 * err;
        if (e2 >= dy) {
            if (x1 == x2) break;
            err += dy; x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2) break;
            err += dx; y1 += sy;
        }
    }
    DrawBuffered(0, 0, 0, 1);
    if(Option.Refresh)Display_Refresh();
}



/**********************************************************************************************
Draw a box
     x1, y1 - the start coordinate
     x2, y2 - the end coordinate
     w      - the width of the sides of the box (can be zero)
     c      - the colour to use for sides of the box
     fill   - the colour to fill the box (-1 for no fill)
***********************************************************************************************/
void DrawBox(int x1, int y1, int x2, int y2, int w, int c, int fill) {
    int t;

    // make sure the coordinates are in the right sequence
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(w > x2 - x1) w = x2 - x1;
    if(w > y2 - y1) w = y2 - y1;

    if(w > 0 && fill!=c) {
        w--;
        DrawRectangle(x1, y1, x2, y1 + w, c);                       // Draw the top horiz line
        DrawRectangle(x1, y2 - w, x2, y2, c);                       // Draw the bottom horiz line
        DrawRectangle(x1, y1, x1 + w, y2, c);                       // Draw the left vert line
        DrawRectangle(x2 - w, y1, x2, y2, c);                       // Draw the right vert line
        w++;
    }
    if(fill==c)w=0;
    if(fill >= 0) DrawRectangle(x1 + w, y1 + w, x2 - w, y2 - w, fill);
    if(Option.Refresh)Display_Refresh();
}



/**********************************************************************************************
Draw a box with rounded corners
     x1, y1 - the start coordinate
     x2, y2 - the end coordinate
     radius - the radius (in pixels) of the arc forming the corners
     c      - the colour to use for sides
     fill   - the colour to fill the box (-1 for no fill)
***********************************************************************************************/
void DrawRBox(int x1, int y1, int x2, int y2, int radius, int c, int fill) {
    int f, ddF_x, ddF_y, xx, yy;
    int t;

    // make sure the coordinates are in the right sequence
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }

    f = 1 - radius;
    ddF_x = 1;
    ddF_y = -2 * radius;
    xx = 0;
    yy = radius;

    while(xx < yy) {
        if(f >= 0) {
            yy-=1;
            ddF_y += 2;
            f += ddF_y;
        }
        xx+=1;
        ddF_x += 2;
        f += ddF_x  ;
        DrawPixel(x2 + xx - radius, y2 + yy - radius, c);           // Bottom Right Corner
        DrawPixel(x2 + yy - radius, y2 + xx - radius, c);           // ^^^
        DrawPixel(x1 - xx + radius, y2 + yy - radius, c);           // Bottom Left Corner
        DrawPixel(x1 - yy + radius, y2 + xx - radius, c);           // ^^^

        DrawPixel(x2 + xx - radius, y1 - yy + radius, c);           // Top Right Corner
        DrawPixel(x2 + yy - radius, y1 - xx + radius, c);           // ^^^
        DrawPixel(x1 - xx + radius, y1 - yy + radius, c);           // Top Left Corner
        DrawPixel(x1 - yy + radius, y1 - xx + radius, c);           // ^^^
        if(fill >= 0) {
            DrawLine(x2 + xx - radius - 1, y2 + yy - radius, x1 - xx + radius + 1, y2 + yy - radius, 1, fill);
            DrawLine(x2 + yy - radius - 1, y2 + xx - radius, x1 - yy + radius + 1, y2 + xx - radius, 1, fill);
            DrawLine(x2 + xx - radius - 1, y1 - yy + radius, x1 - xx + radius + 1, y1 - yy + radius, 1, fill);
            DrawLine(x2 + yy - radius - 1, y1 - xx + radius, x1 - yy + radius + 1, y1 - xx + radius, 1, fill);
        }
    }
    if(fill >= 0) DrawRectangle(x1 + 1, y1 + radius, x2 - 1, y2 - radius, fill);
    DrawRectangle(x1 + radius - 1, y1, x2 - radius + 1, y1, c);     // top side
    DrawRectangle(x1 + radius - 1, y2,  x2 - radius + 1, y2, c);    // botom side
    DrawRectangle(x1, y1 + radius, x1, y2 - radius, c);             // left side
    DrawRectangle(x2, y1 + radius, x2, y2 - radius, c);             // right side
    if(Option.Refresh)Display_Refresh();
}




/***********************************************************************************************
Draw a circle on the video output
	x, y - the center of the circle
	radius - the radius of the circle
    w - width of the line drawing the circle
	c - the colour to use for the circle
	fill - the colour to use for the fill or -1 if no fill
	aspect - the ration of the x and y axis (a MMFLOAT).  1.0 gives a prefect circle
***********************************************************************************************/
void DrawCircle(int x, int y, int radius, int w, int c, int fill, MMFLOAT aspect) {
   int a, b, P;
   int A, B;
   int asp;
   MMFLOAT aspect2;
   if(w>1){
	   if(fill>=0){ // thick border with filled centre
		   DrawCircle(x,y,radius,0,c,c,aspect);
		    aspect2=((aspect*(MMFLOAT)radius)-(MMFLOAT)w)/((MMFLOAT)(radius-w));
		   DrawCircle(x,y,radius-w,0,fill,fill,aspect2);
	   } else { //thick border with empty centre
		   	int r1=radius-w,r2=radius, xs=-1,xi=0, i,j,k,m, ll=radius;
		    if(aspect>1.0)ll=(int)((MMFLOAT)radius*aspect);
		    int ints_per_line=RoundUptoInt((ll*2)+1)/32;
		    uint32_t *br=(uint32_t *)GetTempMemory(((ints_per_line+1)*((r2*2)+1))*4);
		    DrawFilledCircle(x, y, r2, r2, 1, ints_per_line, br, aspect, aspect);
		    aspect2=((aspect*(MMFLOAT)r2)-(MMFLOAT)w)/((MMFLOAT)r1);
		    DrawFilledCircle(x, y, r1, r2, 0, ints_per_line, br, aspect, aspect2);
		    x=(int)((MMFLOAT)x+(MMFLOAT)r2*(1.0-aspect));
		 	for(j=0;j<r2*2+1;j++){
		 		for(i=0;i<ints_per_line;i++){
		 			k=br[i+j*ints_per_line];
		 			for(m=0;m<32;m++){
		 				if(xs==-1 && (k & 0x80000000)){
		 					xs=m;
		 					xi=i;
		 				}
		 				if(xs!=-1 && !(k & 0x80000000)){
							DrawRectangle(x-r2+xs+xi*32, y-r2+j, x-r2+m+i*32, y-r2+j, c);
		 					xs=-1;
		 				}
		 				k<<=1;
		 			}
		 		}
				if(xs!=-1){
					DrawRectangle(x-r2+xs+xi*32, y-r2+j, x-r2+m+i*32, y-r2+j, c);
					xs=-1;
				}
			}
	   }

   } else { //single thickness outline
	   int w1=w,r1=radius;
	   if(fill>=0){
		   while(w >= 0 && radius > 0) {
		       a = 0;
		       b = radius;
		       P = 1 - radius;
		       asp = aspect * (MMFLOAT)(1 << 10);

		       do {
		         A = (a * asp) >> 10;
		         B = (b * asp) >> 10;
		         if(fill >= 0 && w >= 0) {
		             DrawRectangle(x-A, y+b, x+A, y+b, fill);
		             DrawRectangle(x-A, y-b, x+A, y-b, fill);
		             DrawRectangle(x-B, y+a, x+B, y+a, fill);
		             DrawRectangle(x-B, y-a, x+B, y-a, fill);
		         }
		          if(P < 0)
		             P+= 3 + 2*a++;
		          else
		             P+= 5 + 2*(a++ - b--);

		        } while(a <= b);
		        w--;
		        radius--;
		   }
	   }
	   if(c!=fill){
		   w=w1; radius=r1;
		   while(w >= 0 && radius > 0) {
		       a = 0;
		       b = radius;
		       P = 1 - radius;
		       asp = aspect * (MMFLOAT)(1 << 10);
		       do {
		         A = (a * asp) >> 10;
		         B = (b * asp) >> 10;
		         if(w) {
		             DrawPixel(A+x, b+y, c);
		             DrawPixel(B+x, a+y, c);
		             DrawPixel(x-A, b+y, c);
		             DrawPixel(x-B, a+y, c);
		             DrawPixel(B+x, y-a, c);
		             DrawPixel(A+x, y-b, c);
		             DrawPixel(x-A, y-b, c);
		             DrawPixel(x-B, y-a, c);
		         }
		          if(P < 0)
		             P+= 3 + 2*a++;
		          else
		             P+= 5 + 2*(a++ - b--);

		        } while(a <= b);
		        w--;
		        radius--;
		   }
	   }
   }

    if(Option.Refresh)Display_Refresh();
}
#define RoundUptoInt(a)     (((a) + (32 - 1)) & (~(32 - 1)))// round up to the nearest whole integer

void hline(int x0, int x1, int y, int f, int ints_per_line, uint32_t *br) { //draw a horizontal line
    uint32_t w1, xx1, w0, xx0, x, xn, i;
    const uint32_t a[]={0xFFFFFFFF,0x7FFFFFFF,0x3FFFFFFF,0x1FFFFFFF,0xFFFFFFF,0x7FFFFFF,0x3FFFFFF,0x1FFFFFF,
                        0xFFFFFF,0x7FFFFF,0x3FFFFF,0x1FFFFF,0xFFFFF,0x7FFFF,0x3FFFF,0x1FFFF,
                        0xFFFF,0x7FFF,0x3FFF,0x1FFF,0xFFF,0x7FF,0x3FF,0x1FF,
                        0xFF,0x7F,0x3F,0x1F,0x0F,0x07,0x03,0x01};
    const uint32_t b[]={0x80000000,0xC0000000,0xe0000000,0xf0000000,0xf8000000,0xfc000000,0xfe000000,0xff000000,
                        0xff800000,0xffC00000,0xffe00000,0xfff00000,0xfff80000,0xfffc0000,0xfffe0000,0xffff0000,
                        0xffff8000,0xffffC000,0xffffe000,0xfffff000,0xfffff800,0xfffffc00,0xfffffe00,0xffffff00,
                        0xffffff80,0xffffffC0,0xffffffe0,0xfffffff0,0xfffffff8,0xfffffffc,0xfffffffe,0xffffffff};
    w0 = y * (ints_per_line);
    xx0 = 0;
    w1 = y * (ints_per_line) + x1/32;
    xx1 = (x1 & 0x1F);
    w0 = y * (ints_per_line) + x0/32;
    xx0 = (x0 & 0x1F);
    w1 = y * (ints_per_line) + x1/32;
    xx1 = (x1 & 0x1F);
    if(w1==w0){ //special case both inside same word
        x=(a[xx0] & b[xx1]);
        xn=~x;
        if(f)br[w0] |= x; else  br[w0] &= xn;                   // turn on the pixel
    } else {
        if(w1-w0>1){ //first deal with full words
            for(i=w0+1;i<w1;i++){
            // draw the pixel
            	br[i]=0;
                if(f)br[i] = 0xFFFFFFFF;          // turn on the pixels
            }
        }
        x=~a[xx0];
        br[w0] &= x;
        x=~x;
        if(f)br[w0] |= x;                         // turn on the pixel
        x=~b[xx1];
        br[w1] &= x;
        x=~x;
        if(f)br[w1] |= x;                         // turn on the pixel
    }
}

void DrawFilledCircle(int x, int y, int radius, int r, int fill, int ints_per_line, uint32_t *br, MMFLOAT aspect, MMFLOAT aspect2) {
   int a, b, P;
   int A, B, asp;
   	   x=(int)((MMFLOAT)r*aspect)+radius;
   	   y=r+radius;
       a = 0;
       b = radius;
       P = 1 - radius;
       asp = aspect2 * (MMFLOAT)(1 << 10);
       do {
	         A = (a * asp) >> 10;
	         B = (b * asp) >> 10;
	         hline(x-A-radius, x+A-radius,  y+b-radius, fill, ints_per_line, br);
	         hline(x-A-radius, x+A-radius,  y-b-radius, fill, ints_per_line, br);
	         hline(x-B-radius, x+B-radius,  y+a-radius, fill, ints_per_line, br);
	         hline(x-B-radius, x+B-radius,  y-a-radius, fill, ints_per_line, br);
	         if(P < 0)
	            P+= 3 + 2*a++;
	          else
	            P+= 5 + 2*(a++ - b--);

        } while(a <= b);
}
void ClearTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int ints_per_line, uint32_t *br) {

        long a, b, y, last;
        long  dx01,  dy01,  dx02,  dy02, dx12,  dy12,  sa, sb;

        if (y0 > y1) {
            swap(y0, y1);
            swap(x0, x1);
        }
        if (y1 > y2) {
            swap(y2, y1);
            swap(x2, x1);
        }
        if (y0 > y1) {
            swap(y0, y1);
            swap(x0, x1);
        }

            dx01 = x1 - x0;  dy01 = y1 - y0;  dx02 = x2 - x0;
            dy02 = y2 - y0; dx12 = x2 - x1;  dy12 = y2 - y1;
            sa = 0; sb = 0;
            if(y1 == y2) {
                last = y1;                                          //Include y1 scanline
            } else {
                last = y1 - 1;                                      // Skip it
            }
            for (y = y0; y <= last; y++){
                a = x0 + sa / dy01;
                b = x0 + sb / dy02;
                sa = sa + dx01;
                sb = sb + dx02;
                a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
                b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                if(a > b)swap(a, b);
                hline(a, b,  y, 0, ints_per_line, br);
            }
            sa = dx12 * (y - y1);
            sb = dx02 * ( y- y0);
            while (y <= y2){
                a = x1 + sa / dy12;
                b = x0 + sb / dy02;
                sa = sa + dx12;
                sb = sb + dx02;
                a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                if(a > b) swap(a, b);
                hline(a, b,  y, 0, ints_per_line, br);
                y = y + 1;
            }
}
/**********************************************************************************************
Draw a triangle
    Thanks to Peter Mather (matherp on the Back Shed forum)
     x0, y0 - the first corner
     x1, y1 - the second corner
     x2, y2 - the third corner
     c      - the colour to use for sides of the triangle
     fill   - the colour to fill the triangle (-1 for no fill)
***********************************************************************************************/
void DrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill) {
	if(x0 * (y1 - y2) +  x1 * (y2 - y0) +  x2 * (y0 - y1)==0){ // points are co-linear i.e zero area
		if (y0 > y1) {
			swap(y0, y1);
			swap(x0, x1);
		}
		if (y1 > y2) {
			swap(y2, y1);
			swap(x2, x1);
		}
		if (y0 > y1) {
			swap(y0, y1);
			swap(x0, x1);
		}
		DrawLine(x0,y0,x2,y2,1,c);
	} else {
    if(fill == -1){
        // draw only the outline
        DrawLine(x0, y0, x1, y1, 1, c);
        DrawLine(x1, y1, x2, y2, 1, c);
        DrawLine(x2, y2, x0, y0, 1, c);
    } else {
        //we are drawing a filled triangle which may also have an outline
        int a, b, y, last;

        if (y0 > y1) {
            swap(y0, y1);
            swap(x0, x1);
        }
        if (y1 > y2) {
            swap(y2, y1);
            swap(x2, x1);
        }
        if (y0 > y1) {
            swap(y0, y1);
            swap(x0, x1);
        }

            if(y1 == y2) {
                last = y1;                                          //Include y1 scanline
            } else {
                last = y1 - 1;                                      // Skip it
            }
            for (y = y0; y <= last; y++){
                a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
                b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                if(a > b)swap(a, b);
                DrawRectangle(a, y, b, y, fill);
            }
            while (y <= y2){
                a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                if(a > b) swap(a, b);
                DrawRectangle(a, y, b, y, fill);
                y = y + 1;
            }
            // we also need an outline but we do this last to overwrite the edge of the fill area
            if(c!=fill){
	            DrawLine(x0, y0, x1, y1, 1, c);
	            DrawLine(x1, y1, x2, y2, 1, c);
	            DrawLine(x2, y2, x0, y0, 1, c);
	        }
	    }
    if(Option.Refresh)Display_Refresh();
	}
}


/******************************************************************************************
 Print a char on the LCD display
 Any characters not in the font will print as a space.
 The char is printed at the current location defined by CurrentX and CurrentY
*****************************************************************************************/
void GUIPrintChar(int fnt, int fc, int bc, char c, int orientation) {
    unsigned char *p, *fp, *np = NULL;
    int BitNumber, BitPos, x, y, newx, newy, modx, mody, scale = fnt & 0b1111;
    int height, width;

    // to get the +, - and = chars for font 6 we fudge them by scaling up font 1
    if((fnt & 0xf0) == 0x50 && (c == '-' || c == '+' || c == '=')) {
        fp = (unsigned char *)FontTable[0];
        scale = scale * 4;
    } else
        fp = (unsigned char *)FontTable[fnt >> 4];

    height = fp[1];
    width = fp[0];
    modx = mody = 0;
    if(orientation > ORIENT_VERT){
        np = GetTempMemory(width * height);
        if (orientation == ORIENT_INVERTED) {
            modx -= width * scale -1;
            mody -= height * scale -1;
        }
        else if (orientation == ORIENT_CCW90DEG) {
            mody -= width * scale;
        }
        else if (orientation == ORIENT_CW90DEG){
            modx -= height * scale -1;
        }
    }

    if(c >= fp[2] && c < fp[2] + fp[3]) {
        p = fp + 4 + (int)(((c - fp[2]) * height * width) / 8);

        if(orientation > ORIENT_VERT) {                             // non-standard orientation
            if (orientation == ORIENT_INVERTED) {
                for(y = 0; y < height; y++) {
                    newy = height - y - 1;
                    for(x=0; x < width; x++) {
                        newx = width - x - 1;
                        if((p[((y * width) + x)/8] >> (((height * width) - ((y * width) + x) - 1) %8)) & 1) {
                            BitNumber=((newy * width) + newx);
                            BitPos = 128 >> (BitNumber % 8);
                            np[BitNumber / 8] |= BitPos;
                        }
                    }
                }
            } else if (orientation == ORIENT_CCW90DEG) {
                for(y = 0; y < height; y++) {
                    newx = y;
                    for(x = 0; x < width; x++) {
                        newy = width - x - 1;
                        if((p[((y * width) + x)/8] >> (((height * width) - ((y * width) + x) - 1) %8)) & 1) {
                            BitNumber=((newy * height) + newx);
                            BitPos = 128 >> (BitNumber % 8);
                            np[BitNumber / 8] |= BitPos;
                        }
                    }
                }
            } else if (orientation == ORIENT_CW90DEG) {
                for(y = 0; y < height; y++) {
                    newx = height - y - 1;
                    for(x=0; x < width; x++) {
                        newy = x;
                        if((p[((y * width) + x)/8] >> (((height * width) - ((y * width) + x) - 1) %8)) & 1) {
                            BitNumber=((newy * height) + newx);
                            BitPos = 128 >> (BitNumber % 8);
                            np[BitNumber / 8] |= BitPos;
                        }
                    }
                }
            }
        }  else np = p;

        if(orientation < ORIENT_CCW90DEG) DrawBitmap(CurrentX + modx, CurrentY + mody, width, height, scale, fc, bc, np);
        else DrawBitmap(CurrentX + modx, CurrentY + mody, height, width, scale, fc, bc, np);
    } else {
        if(orientation < ORIENT_CCW90DEG) DrawRectangle(CurrentX + modx, CurrentY + mody, CurrentX + modx + (width * scale), CurrentY + mody + (height * scale), bc);
        else DrawRectangle(CurrentX + modx, CurrentY + mody, CurrentX + modx + (height * scale), CurrentY + mody + (width * scale), bc);
    }

    // to get the . and degree symbols for font 6 we draw a small circle
    if((fnt & 0xf0) == 0x50) {
        if(orientation > ORIENT_VERT) {
            if(orientation == ORIENT_INVERTED) {
                if(c == '.') DrawCircle(CurrentX + modx + (width * scale)/2, CurrentY + mody + 7 * scale, 4 * scale, 0, fc, fc, 1.0);
                if(c == 0x60) DrawCircle(CurrentX + modx + (width * scale)/2, CurrentY + mody + (height * scale)- 9 * scale, 6 * scale, 2 * scale, fc, -1, 1.0);
            } else if(orientation == ORIENT_CCW90DEG) {
                if(c == '.') DrawCircle(CurrentX + modx + (height * scale) - 7 * scale, CurrentY + mody + (width * scale)/2, 4 * scale, 0, fc, fc, 1.0);
                if(c == 0x60) DrawCircle(CurrentX + modx + 9 * scale, CurrentY + mody + (width * scale)/2, 6 * scale, 2 * scale, fc, -1, 1.0);
            } else if(orientation == ORIENT_CW90DEG) {
                if(c == '.') DrawCircle(CurrentX + modx + 7 * scale, CurrentY + mody + (width * scale)/2, 4 * scale, 0, fc, fc, 1.0);
                if(c == 0x60) DrawCircle(CurrentX + modx + (height * scale)- 9 * scale, CurrentY + mody + (width * scale)/2, 6 * scale, 2 * scale, fc, -1, 1.0);
            }

        } else {
            if(c == '.') DrawCircle(CurrentX + modx + (width * scale)/2, CurrentY + mody + (height * scale) - 7 * scale, 4 * scale, 0, fc, fc, 1.0);
            if(c == 0x60) DrawCircle(CurrentX + modx + (width * scale)/2, CurrentY + mody + 9 * scale, 6 * scale, 2 * scale, fc, -1, 1.0);
        }
    }

    if(orientation == ORIENT_NORMAL) CurrentX += width * scale;
    else if (orientation == ORIENT_VERT) CurrentY += height * scale;
    else if (orientation == ORIENT_INVERTED) CurrentX -= width * scale;
    else if (orientation == ORIENT_CCW90DEG) CurrentY -= width * scale;
    else if (orientation == ORIENT_CW90DEG ) CurrentY += width * scale;
}


/******************************************************************************************
 Print a string on the LCD display
 The string must be a C string (not an MMBasic string)
 Any characters not in the font will print as a space.
*****************************************************************************************/
void GUIPrintString(int x, int y, int fnt, int jh, int jv, int jo, int fc, int bc, char *str) {
    CurrentX = x;  CurrentY = y;
    if(jo == ORIENT_NORMAL) {
        if(jh == JUSTIFY_CENTER) CurrentX -= (strlen(str) * GetFontWidth(fnt)) / 2;
        if(jh == JUSTIFY_RIGHT)  CurrentX -= (strlen(str) * GetFontWidth(fnt));
        if(jv == JUSTIFY_MIDDLE) CurrentY -= GetFontHeight(fnt) / 2;
        if(jv == JUSTIFY_BOTTOM) CurrentY -= GetFontHeight(fnt);
    }
    else if(jo == ORIENT_VERT) {
        if(jh == JUSTIFY_CENTER) CurrentX -= GetFontWidth(fnt)/ 2;
        if(jh == JUSTIFY_RIGHT)  CurrentX -= GetFontWidth(fnt);
        if(jv == JUSTIFY_MIDDLE) CurrentY -= (strlen(str) * GetFontHeight(fnt)) / 2;
        if(jv == JUSTIFY_BOTTOM) CurrentY -= (strlen(str) * GetFontHeight(fnt));
    }
    else if(jo == ORIENT_INVERTED) {
        if(jh == JUSTIFY_CENTER) CurrentX += (strlen(str) * GetFontWidth(fnt)) / 2;
        if(jh == JUSTIFY_RIGHT)  CurrentX += (strlen(str) * GetFontWidth(fnt));
        if(jv == JUSTIFY_MIDDLE) CurrentY += GetFontHeight(fnt) / 2;
        if(jv == JUSTIFY_BOTTOM) CurrentY += GetFontHeight(fnt);
    }
    else if(jo == ORIENT_CCW90DEG) {
        if(jh == JUSTIFY_CENTER) CurrentX -=  GetFontHeight(fnt) / 2;
        if(jh == JUSTIFY_RIGHT)  CurrentX -=  GetFontHeight(fnt);
        if(jv == JUSTIFY_MIDDLE) CurrentY += (strlen(str) * GetFontWidth(fnt)) / 2;
        if(jv == JUSTIFY_BOTTOM) CurrentY += (strlen(str) * GetFontWidth(fnt));
    }
    else if(jo == ORIENT_CW90DEG) {
        if(jh == JUSTIFY_CENTER) CurrentX += GetFontHeight(fnt) / 2;
        if(jh == JUSTIFY_RIGHT)  CurrentX += GetFontHeight(fnt);
        if(jv == JUSTIFY_MIDDLE) CurrentY -= (strlen(str) * GetFontWidth(fnt)) / 2;
        if(jv == JUSTIFY_BOTTOM) CurrentY -= (strlen(str) * GetFontWidth(fnt));
    }
    while(*str) GUIPrintChar(fnt, fc, bc, *str++, jo);
    if(Option.Refresh)Display_Refresh();
}




/****************************************************************************************************
 ****************************************************************************************************

 Basic drawing primitives for a user defined LCD display driver (ie, OPTION LCDPANEL USER)
 all drawing is done using either DrawRectangleUser() or DrawBitmapUser()

 ****************************************************************************************************
****************************************************************************************************/


// Draw a filled rectangle
// this is the basic drawing primitive used by most drawing routines
//    x1, y1, x2, y2 - the coordinates
//    c - the colour
void DrawRectangleUser(int x1, int y1, int x2, int y2, int c){
    char callstr[256];
    char *nextstmtSaved = nextstmt;
    if(FindSubFun("MM.USER_RECTANGLE", 0) >= 0) {
        strcpy(callstr, "MM.USER_RECTANGLE");
        strcat(callstr, " "); IntToStr(callstr + strlen(callstr), x1, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), y1, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), x2, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), y2, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), c, 10);
        callstr[strlen(callstr)+1] = 0;                             // two NULL chars required to terminate the call
        LocalIndex++;
        ExecuteProgram(callstr);
        nextstmt = nextstmtSaved;
        LocalIndex--;
        TempMemoryIsChanged = true;                                 // signal that temporary memory should be checked
    } else
        error("MM.USER_RECTANGLE not defined");
}


//Print the bitmap of a char on the video output
//    x, y - the top left of the char
//    width, height - size of the char's bitmap
//    scale - how much to scale the bitmap
//	  fc, bc - foreground and background colour
//    bitmap - pointer to the bitmap
void DrawBitmapUser(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap){
    char callstr[256];
    char *nextstmtSaved = nextstmt;
    if(FindSubFun("MM.USER_BITMAP", 0) >= 0) {
        strcpy(callstr, "MM.USER_BITMAP");
        strcat(callstr, " "); IntToStr(callstr + strlen(callstr), x1, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), y1, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), width, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), height, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), scale, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), fc, 10);
        strcat(callstr, ","); IntToStr(callstr + strlen(callstr), bc, 10);
        strcat(callstr, ",&H"); IntToStr(callstr + strlen(callstr), (unsigned int)bitmap, 16);
        callstr[strlen(callstr)+1] = 0;                             // two NULL chars required to terminate the call
        LocalIndex++;
        ExecuteProgram(callstr);
        LocalIndex--;
        TempMemoryIsChanged = true;                                 // signal that temporary memory should be checked
        nextstmt = nextstmtSaved;
    } else
        error("MM.USER_BITMAP not defined");
}

void ScrollLCDSPI(int lines){
	if(lines==0)return;
     char *buff=GetMemory(3*HRes);
     if(lines >= 0) {
        for(int i=0;i<VRes-lines;i++) {
        	ReadBuffer(0, i+lines, HRes -1, i+lines, buff);
			DrawBuffer(0, i, HRes - 1, i, buff);
			while(MMInkey() != -1);                                         // consume any keystrokes accumulated while redrawing the screen
        }
        DrawRectangle(0, VRes-lines, HRes - 1, VRes - 1, gui_bcolour); // erase the lines to be scrolled off
    } else {
    	lines=-lines;
        for(int i=VRes-1;i>=lines;i--) {
        	ReadBuffer(0, i-lines, HRes -1, i-lines, buff);
			DrawBuffer(0, i, HRes - 1, i, buff);
			while(MMInkey() != -1);                                         // consume any keystrokes accumulated while redrawing the screen
        }
        DrawRectangle(0, 0, HRes - 1, lines - 1, gui_bcolour); // erase the lines introduced at the top
    }
    FreeMemory(buff);
}



/****************************************************************************************************

 General purpose routines

****************************************************************************************************/



int rgb(int r, int g, int b) {
    return RGB(r, g, b);
}


int GetFontWidth(int fnt) {
    return FontTable[fnt >> 4][0] * (fnt & 0b1111);
}


int GetFontHeight(int fnt) {
    return FontTable[fnt >> 4][1] * (fnt & 0b1111);
}


void SetFont(int fnt) {
    if(FontTable[fnt >> 4] == NULL) error("Invalid font number #%", (fnt >> 4)+1);
    gui_font_width = FontTable[fnt >> 4][0] * (fnt & 0b1111);
    gui_font_height = FontTable[fnt >> 4][1] * (fnt & 0b1111);
//    if(((fnt >> 4)+1)==7)gui_font_height=10;
    if(Option.DISPLAY_CONSOLE) {
        Option.Height = VRes/gui_font_height;
        Option.Width = HRes/gui_font_width;
    }
    gui_font = fnt;
}

void SetBacklight(int intensity){
	 if (intensity>100)intensity-=101;
	 if(Option.DISPLAY_TYPE >= SSD_PANEL_START && Option.DISPLAY_TYPE <= SSD_PANEL_END){
	      SetBacklightSSD1963(intensity);         //Send the command to the SSD1963 incase it strapped that way
	      htim1.Instance->CCR3=1000-intensity*10; //Also set the PWM pin in case its strapped that way
	 }else if(Option.DISPLAY_TYPE==IPS_4_16 ){
		   if(Option.DefaultBrightness>100){
			   htim1.Instance->CCR3=intensity*10;
		   }else{
			   htim1.Instance->CCR3=1000-intensity*10;
		   }
	 } else {
	   if(Option.DefaultBrightness>100){
		   htim1.Instance->CCR3=1000-intensity*10;
	   }else{
		   htim1.Instance->CCR3=intensity*10;
	   }
	 }
}

void ResetDisplay(void) {
    if(!Option.DISPLAY_CONSOLE) {
        SetFont(Option.DefaultFont);
        gui_fcolour = Option.DefaultFC;
        gui_bcolour = Option.DefaultBC;
       // SetBacklight(Option.DefaultBrightness);
    }
    ResetGUI();
}

