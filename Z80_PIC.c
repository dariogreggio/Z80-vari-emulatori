// Local Header Files
#include <stdlib.h>
#include <string.h>
#include <xc.h>
#include "Z80_PIC.h"

#include <sys/attribs.h>
#include <sys/kmem.h>

#ifdef ST7735
#include "Adafruit_ST77xx.h"
#include "Adafruit_ST7735.h"
#include "adafruit_gfx.h"
#endif
#ifdef ILI9341
#include "Adafruit_ILI9341.h"
#include "adafruit_gfx.h"
#endif



// PIC32MZ1024EFE064 Configuration Bit Settings

// 'C' source line config statements

// DEVCFG3
// USERID = No Setting
#pragma config FMIIEN = OFF             // Ethernet RMII/MII Enable (RMII Enabled)
#pragma config FETHIO = OFF             // Ethernet I/O Pin Select (Alternate Ethernet I/O)
#pragma config PGL1WAY = ON             // Permission Group Lock One Way Configuration (Allow only one reconfiguration)
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = ON            // USB USBID Selection (Controlled by the USB Module)

// DEVCFG2
/* Default SYSCLK = 200 MHz (8MHz FRC / FPLLIDIV * FPLLMUL / FPLLODIV) */
//#pragma config FPLLIDIV = DIV_1, FPLLMULT = MUL_50, FPLLODIV = DIV_2
#pragma config FPLLIDIV = DIV_1         // System PLL Input Divider (1x Divider)
#pragma config FPLLRNG = RANGE_5_10_MHZ// System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_FRC       // System PLL Input Clock Selection (FRC is input to the System PLL)
#pragma config FPLLMULT = MUL_51       // System PLL Multiplier (PLL Multiply by 50)
#pragma config FPLLODIV = DIV_2        // System PLL Output Clock Divider (2x Divider)
#pragma config UPLLFSEL = FREQ_24MHZ    // USB PLL Input Frequency Selection (USB PLL input is 24 MHz)

// DEVCFG1
#pragma config FNOSC = FRCDIV           // Oscillator Selection Bits (Fast RC Osc w/Div-by-N (FRCDIV))
#pragma config DMTINTV = WIN_127_128    // DMT Count Window Interval (Window/Interval value is 127/128 counter value)
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enable SOSC)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FCKSM = CSECME           // Clock Switching and Monitor Selection (Clock Switch Enabled, FSCM Enabled)
#pragma config WDTPS = PS16384          // Watchdog Timer Postscaler (1:16384)
  // circa 6-7 secondi, 24.7.19
#pragma config WDTSPGM = STOP           // Watchdog Timer Stop During Flash Programming (WDT stops during Flash programming)
#pragma config WINDIS = NORMAL          // Watchdog Timer Window Mode (Watchdog Timer is in non-Window mode)
#pragma config FWDTEN = ON             // Watchdog Timer Enable (WDT Enabled)
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window size is 25%)
#pragma config DMTCNT = DMT31           // Deadman Timer Count Selection (2^31 (2147483648))
#pragma config FDMTEN = OFF             // Deadman Timer Enable (Deadman Timer is disabled)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#ifdef ILI9341      // pcb arduino forgetIvrea32
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (Communicate on PGEC2/PGED2)
#else   // pcb radio 2019
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#endif
#pragma config TRCEN = OFF              // Trace Enable (Trace features in the CPU are disabled)
#pragma config BOOTISA = MIPS32         // Boot ISA Selection (Boot code and Exception code is MIPS32)
#pragma config FECCCON = OFF_UNLOCKED   // Dynamic Flash ECC Configuration (ECC and Dynamic ECC are disabled (ECCCON bits are writable))
#pragma config FSLEEP = OFF             // Flash Sleep Mode (Flash is powered down when the device is in Sleep mode)
#pragma config DBGPER = PG_ALL          // Debug Mode CPU Access Permission (Allow CPU access to all permission regions)
#pragma config SMCLR = MCLR_NORM        // Soft Master Clear Enable bit (MCLR pin generates a normal system Reset)
#pragma config SOSCGAIN = GAIN_2X       // Secondary Oscillator Gain Control bits (2x gain setting)
#pragma config SOSCBOOST = ON           // Secondary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCGAIN = GAIN_2X       // Primary Oscillator Gain Control bits (2x gain setting)
#pragma config POSCBOOST = ON           // Primary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config EJTAGBEN = NORMAL        // EJTAG Boot (Normal EJTAG functionality)

// DEVCP0
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// SEQ3

// DEVADC0

// DEVADC1

// DEVADC2

// DEVADC3

// DEVADC4

// DEVADC7



const char CopyrightString[]= {'Z','8','0',' ','E','m','u','l','a','t','o','r',' ','v',
	VERNUMH+'0','.',VERNUML/10+'0',(VERNUML % 10)+'0',' ','-',' ', '0','7','/','0','9','/','2','5', 0 };

const char Copyr1[]="(C) Dario's Automation 2019-2025 - G.Dar\xd\xa\x0";



// Global Variables:
BOOL fExit,debug;
extern BYTE ColdReset;
extern BYTE ram_seg[];
extern BYTE rom_seg[],rom_seg2[];
#ifdef SKYNET
extern BYTE Keyboard[1];
extern volatile BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
#endif
#ifdef NEZ80
extern BYTE DisplayRAM[8];
extern BYTE Keyboard[1];
extern volatile BYTE TIMIRQ;
extern volatile BYTE MUXcnt;
#endif
#ifdef SKYNET
extern BYTE LCDram[256 /* 4*40 per la geometria, vale così */],LCDfunction,LCDentry,LCDdisplay,LCDcursor;
extern signed char LCDptr;
extern BYTE IOExtPortI[4],IOExtPortO[4];
extern BYTE IOPortI,IOPortO,ClIRQPort,ClWDPort;
extern BYTE i146818RegR[2],i146818RegW[2],i146818RAM[64];
extern BYTE KBDataI,KBDataO,KBControl /*,KBStatus*/, KBRAM[32];
#define KBStatus KBRAM[0]   // pare...
extern const unsigned char fontLCD_eu[],fontLCD_jp[];
#endif
#ifdef ZX80
extern volatile BYTE TIMIRQ;
extern BYTE Keyboard[8];
#endif
#ifdef ZX81
extern volatile BYTE TIMIRQ;
extern BYTE Keyboard[8];
#endif
#ifdef GALAKSIJA
extern volatile BYTE TIMIRQ;
extern BYTE Keyboard[8];
extern BYTE Latch;
#endif
#ifdef MSX
extern volatile BYTE TIMIRQ,VIDIRQ;
extern BYTE TMS9918Reg[8],TMS9918RegS,TMS9918Sel,TMS9918WriteStage;
extern BYTE AY38910RegR[16],AY38910RegW[16],AY38910RegSel;
extern BYTE i8255RegR[4],i8255RegW[4];
extern BYTE i8251RegR[4],i8251RegW[4];
extern BYTE i8253RegR[4],i8253RegW[4];
extern BYTE iPrinter[2];
extern BYTE VideoRAM[VIDEORAM_SIZE];
extern BYTE Keyboard[11];
extern WORD TIMEr;
#endif
volatile PIC32_RTCC_DATE currentDate={1,1,0};
volatile PIC32_RTCC_TIME currentTime={0,0,0};
const BYTE dayOfMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};


#ifdef SKYNET
#if 0
extern const unsigned char CHIESA_BIN[];
#endif
  
extern const unsigned char Z803_BIN[];

#if 0
extern const unsigned char CASANET_BIN[];
#endif
#ifdef CASANET_SENZA_MAINLOOP
extern const unsigned char CASANET3_BIN[];
#endif
extern const unsigned char CASANET3_BIN[];
#if 0
extern const unsigned char SKYBASIC_BIN[];
#endif
#endif
#if NEZ80
extern const unsigned char Z80NE_BIN[];
#endif
#ifdef GALAKSIJA
extern const unsigned char GALAKSIJA_BIN[];
extern const unsigned char GALAKSIJA_BIN2[];
#endif
#ifdef ZX80
extern const unsigned char ZX80_BIN[];
#endif
#ifdef ZX81
extern const unsigned char ZX81_BIN[];
#endif

#ifdef MSX
extern const unsigned char MSX_BIN[];
#endif



WORD displayColor[3]={BLACK,BRIGHTRED,RED};

#ifdef NEZ80
#ifdef ILI9341
#error LCD NON SUPPORTATO! fare
#endif
int PlotDisplay(WORD pos,BYTE ch,BYTE c) {
	register int i;
  int x,y;
  SWORD color;

#define DIGIT_X_SIZE 14
#define DIGIT_Y_SIZE 30
#define DIGIT_OBLIQ 2
  
  x=8;
  y=40;
  x+=(DIGIT_X_SIZE+4)*pos;
//	fillRect(x,y,DIGIT_X_SIZE+3,DIGIT_Y_SIZE+1,BLACK);
  
  if(c)
    color=BRIGHTRED;
  else
    color=RED;
  if(!(ch & 1)) {
    drawLine(x+1+DIGIT_OBLIQ,y+1,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y+1,color);
    }
  else
    drawLine(x+1+DIGIT_OBLIQ,y+1,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y+1,BLACK);
  if(!(ch & 2)) {
    drawLine(x+DIGIT_X_SIZE+DIGIT_OBLIQ,y+1,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,color);
    }
  else
    drawLine(x+DIGIT_X_SIZE+DIGIT_OBLIQ,y+1,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
  if(!(ch & 4)) {
    drawLine(x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,color);
    }
  else
    drawLine(x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,BLACK);
  if(!(ch & 8)) {
    drawLine(x+1,y+DIGIT_Y_SIZE,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,color);
    }
  else
    drawLine(x+1,y+DIGIT_Y_SIZE,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,BLACK);
  if(!(ch & 16)) {
    drawLine(x+1+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+1,y+DIGIT_Y_SIZE,color);
    }
  else
    drawLine(x+1+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+1,y+DIGIT_Y_SIZE,BLACK);
  if(!(ch & 32)) {
    drawLine(x+1+DIGIT_OBLIQ,y+1,x+1+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,color);
    }
  else
    drawLine(x+1+DIGIT_OBLIQ,y+1,x+1+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
  if(!(ch & 64)) {
    drawLine(x+1+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,color);
    }
  else
    drawLine(x+1+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
// più bello..?    drawLine(x+2+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x-1+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
  if(!(ch & 128)) {
    drawCircle(x+DIGIT_X_SIZE+2,y+DIGIT_Y_SIZE+1,1,color);    // 
    }
  else
    drawCircle(x+DIGIT_X_SIZE+2,y+DIGIT_Y_SIZE+1,1,BLACK);
  
	}
#endif

#ifdef SKYNET
#ifdef ILI9341
#error LCD NON SUPPORTATO! fare
#endif
int UpdateScreen(WORD c) {
  int x,y,x1,y1;
  SWORD color;
	UINT8 i,j,lcdMax;
	BYTE *fontPtr,*lcdPtr;
  static BYTE cursorState=0,cursorDivider=0;

#define LCD_MAX_X 20
#define LCD_MAX_Y 4
#define DIGIT_X_SIZE 6
#define DIGIT_Y_SIZE 8
  
  
//  i8255RegR[0] |= 0x80; mah... fare?? v. di là

  y=(_TFTHEIGHT-(LCD_MAX_Y*DIGIT_Y_SIZE))/2 +20;
  
//	fillRect(x,y,DIGIT_X_SIZE+3,DIGIT_Y_SIZE+1,BLACK);
	gfx_drawRect((_TFTWIDTH-(LCD_MAX_X*DIGIT_X_SIZE))/2-1,(_TFTHEIGHT-(LCD_MAX_Y*DIGIT_Y_SIZE))/2 +16,
          DIGIT_X_SIZE*20+4,DIGIT_Y_SIZE*4+7,LIGHTGRAY);
  
  if(c)
    color=WHITE;
  else
    color=LIGHTGRAY;

  
//        LCDdisplay=7; //test cursore

  cursorDivider++;
  if(cursorDivider>=11) {
    cursorDivider=0;
    cursorState=!cursorState;
    }
          
        
  if(LCDdisplay & 4) {

  lcdMax=LCDfunction & 8 ? LCD_MAX_Y : LCD_MAX_Y/2;
  for(y1=0; y1<LCD_MAX_Y; y1++) {
    x=(_TFTWIDTH-(LCD_MAX_X*DIGIT_X_SIZE))/2;
    
//    LCDram[0]='A';LCDram[1]='1';LCDram[2]=1;LCDram[3]=40;
//    LCDram[21]='Z';LCDram[23]='8';LCDram[25]='0';LCDram[27]=64;LCDram[39]='.';
//    LCDram[84+4]='C';LCDram[84+5]='4';
    
    
    switch(y1) {    // 4x20 
      case 0:
        lcdPtr=&LCDram[0];
        break;
      case 1:
        lcdPtr=&LCDram[0x40];
        break;
      case 2:
        lcdPtr=&LCDram[20];
        break;
      case 3:
        lcdPtr=&LCDram[0x40+20];
        break;
      }

    for(x1=0; x1<LCD_MAX_X; x1++) {
//      UINT8 ch;
  
//      ch=*lcdPtr;
//      if(LCDdisplay & 2) {
//	      if(!(LCDdisplay & 1) || cursorState) { questo era per avere il bloccone, fisso o lampeggiante, ma in effetti sui LCD veri è diverso!
      if((lcdPtr-&LCDram[0]) == LCDptr) {
        if(LCDdisplay & 2) {
          for(j=6; j>1; j--) {    //lineetta bassa E FONT TYPE QUA??
            drawPixel(x+x1*6+j, y+7, color);
            }
          }
        if((LCDdisplay & 1) && cursorState) {
          int k=LCDdisplay & 2 ? 7 : 8;

          for(i=0; i<k; i++) {    //

            if(LCDfunction & 4)   // font type...
              ;

            for(j=6; j>1; j--) {    //+ piccolo..
              drawPixel(x+x1*6+j, y+i, color);
              }
            }
          }
        goto skippa;
        }
      
      fontPtr=fontLCD_eu+((UINT16)*lcdPtr)*10;
      for(i=0; i<8; i++) {
        UINT8 line;

        line = pgm_read_byte(fontPtr+i);

        if(LCDfunction & 4)   // font type...
          ;
        
        for(j=6; j>0; j--, line >>= 1) {
          if(line & 0x1)
            drawPixel(x+x1*6+j, y+i, color);
          else
            drawPixel(x+x1*6+j, y+i, BLACK);
          }
        }
      
skippa:
      lcdPtr++;
      }
      
    y+=DIGIT_Y_SIZE;
    }
    }
  
//  i8255RegR[0] &= ~0x7f;
  
//  LuceLCD=i8255RegW[1] &= 0x80; fare??

  
	gfx_drawRect(4,41,_TFTWIDTH-9,13,ORANGE);
  for(i=0,j=1; i<8; i++,j<<=1) {
    if(IOExtPortO[0] & j)
      fillCircle(10+i*9,47,3,RED);
    else
      fillCircle(10+i*9,47,3,DARKGRAY);
    }
  for(i=8,j=1; i<16; i++,j<<=1) {
    if(IOExtPortO[1] & j)
      fillCircle(12+i*9,47,3,RED);
    else
      fillCircle(12+i*9,47,3,DARKGRAY);
    }
  for(i=0,j=1; i<8; i++,j<<=1) {
    if(IOPortO /*ram_seg[0] /*MemPort1 = 0x8000 */ & j)
      fillCircle(10+i*7,36,2,RED);
    else
      fillCircle(10+i*7,36,2,DARKGRAY);
    }
          
	}
#endif

#ifdef ZX80
//https://problemkaputt.de/zxdocs.htm#zx80zx81videomodetextandblockgraphics
/*
Overview
This is the ZX standard video mode. The display area consists of 32x24 characters of 8x8 pixels each. The user cannot set single pixels though, only 64 predefined characters can be used. However, some of these characters are split into 2x2 blocks (4x4 pixel each), allowing to display 64x48 block low resolution graphics.

Video Memory
Video memory is addressed by the D_FILE pointer (400Ch) in ZX80/81 system area. The first byte in VRAM is a HALT opcode (76h),
 followed by the data (one byte per character) for each of the 24 lines, each line is terminated by a HALT opcode also. 
 In case that a line contains less than 32 characters, the HALT opcode blanks (white) the rest of the line up to 
 the right screen border. (Thus left-aligned text will take up less memory than centered or right-aligned text.)

Character Data, VRAM Size
Character data in range 00h..3Fh displays the 64 characters, normally black on white. 
Characters may be inverted by setting Bit 7, ie. C0h..FFh represents the same as above displayed white on black.
The fully expanded VRAM size is 793 bytes (32x24 + 25 HALTs, almost occupying the whole 1Kbyte of internal RAM), 
an empty fully collapsed screen occupies only 25 bytes (HALTs).

Character Set
The character set is addressed by the I register multiplied by 100h. 
In the ZX81 this is 1Eh for 1E00h..1FFFh, in ZX80 0Eh for 0E00h..0FFFh. 
Setting I=40h..7Fh in attempt to define a custom charset in RAM rather than ROM does not work.

Display procedure Tech Details
The display data is more or less 'executed' by the CPU. When displaying a line, the BIOS takes the address 
of the first character, eg. 4123h, sets Bit 15, ie. C123h, and then jumps to that address.

The hardware now senses A15=HIGH and /M1=LOW (signalizing opcode read), upon this condition memory is mirrored 
from C000-FFFF to 4000-7FFF. The 'opcode' is presented to the databus as usually, the display circuit interpretes 
it as character data, and (if Bit 6 is zero) forces the databus to zero before the CPU realizes what is going on, 
causing a NOP opcode (00h) to be executed. Bit 7 of the stolen opcode is used as invert attribute, 
Bit 0-5 address one of the 64 characters in ROM at (I*100h+char*8+linecntr), the byte at that address is loaded 
into a shift register, and bits are shifted to the display at a rate of 6.5MHz (ie. 8 pixels within 4 CPU cycles).

However, when encountering an opcode with Bit 6 set, then the video circuit rejects the opcode (displays white, 
regardless of Bit 7 and 0-5), and the CPU executes the opcode as normal. Usually this would be the HALT opcode 
- before displaying a line, BIOS enables INTs and initializes the R register, which will produce an interrupt 
when Bit 6 of R becomes zero.
In this special case R is incremented at a fixed rate of 4 CPU cycles (video data executed as NOPs, 
followed by repeated HALT), so that line display is suspended at a fixed time, regardless of the collapsed or 
expanded length of the line.

As mentioned above, an additional register called linecntr is used to address the vertical position (0..7) 
whithin a character line. This register is reset during vertical retrace, and then incremented once per scanline. 
The BIOS thus needs to 'execute' each character line eight times, before starting to 'execute' the next character line.
 
 http://searle.x10host.com/zx80/zx80ScopePics.html
*/

#ifdef ST7735
#define HORIZ_SIZE 256
#define VERT_SIZE 192
#define REAL_SIZE    1      // diciamo :)
int UpdateScreen(SWORD rowIni, SWORD rowFin, BYTE _i) {
	register int i,j;
	int k,y1,y2,x2,row1,row2,curvideopos,oldvideopos,usedpos;
	register BYTE *p,*p1;
  SWORD D_FILE;
  BYTE ch,invert;

  // ci mette circa 25mS ogni passata... [[dovrebbe essere la metà... (viene chiamata circa 25-30 volte al secondo e ora siamo a 40mS invece di 20, 16/10/22)
  
	// per SPI DMA https://www.microchip.com/forums/m1110777.aspx#1110777

  LED3 = 1;

  row1=rowIni;
  row2=rowFin;
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
#ifdef REAL_SIZE    
  x2=160/8;
//  y2=128/8;    // 
  y2=192/8  +1  ;    // voglio/devo visualizzare le righe inferiori...
  if(rowIni>=128)
    rowIni=128;
  if(rowFin>=128)
    rowFin=128;
#else
  x2=HORIZ_SIZE/8;
  
  y2=25; /// CAPIRE PERCHé ce n'è uno in +... e si parte da 1 quindi (v.sotto
#endif
  START_WRITE();
  setAddrWindow(0,rowIni,_width,rowFin-rowIni);
  D_FILE=MAKEWORD(ram_seg[0x000c],ram_seg[0x000d]);     // 400Ch
  if(D_FILE<0x4000)
    return;
  curvideopos=oldvideopos=0;
  for(i=y1; i<y2; i++) {
    curvideopos=oldvideopos;
#ifdef REAL_SIZE    
    for(k=0; k<8; k++) {      // scanline del carattere da plottare
#else
    for(k=0; k<8; k++) {      // scanline del carattere da plottare
#endif
      usedpos=0;
      p1=((BYTE*)&ram_seg[D_FILE+curvideopos-0x4000]);    // carattere a inizio riga corrente
      for(j=0; j<HORIZ_SIZE/8; j++) {
        ch=*p1;
        if(ch != 0x76) {
          invert=ch & 0x80;
          ch &= 0x3f;
          // si potrebbe usare I << 8
          p=((BYTE*)&rom_seg)+(((DWORD)_i) << 8) /*0x0e00*/+((DWORD)ch)*8+k;    // pattern del carattere da disegnare
          ch=*p;
          p1++;
          curvideopos++;
          usedpos++;
          }
        else {
          invert=ch=0;
          }

#ifdef REAL_SIZE    
        if(i>=9 && j<x2) {    // una riga sotto...
          if(invert) {
            writedata16(ch & 0x80 ? WHITE : BLACK);     // 
            writedata16(ch & 0x40 ? WHITE : BLACK);     // 
            writedata16(ch & 0x20 ? WHITE : BLACK);     // 
            writedata16(ch & 0x10 ? WHITE : BLACK);     // 
            writedata16(ch & 0x8 ? WHITE : BLACK);     // 
            writedata16(ch & 0x4 ? WHITE : BLACK);     // 
            writedata16(ch & 0x2 ? WHITE : BLACK);     // 
            writedata16(ch & 0x1 ? WHITE : BLACK);     // 
            }
          else {
            writedata16(ch & 0x80 ? BLACK : WHITE);     // 
            writedata16(ch & 0x40 ? BLACK : WHITE);     // 
            writedata16(ch & 0x20 ? BLACK : WHITE);     // 
            writedata16(ch & 0x10 ? BLACK : WHITE);     // 
            writedata16(ch & 0x8 ? BLACK : WHITE);     // 
            writedata16(ch & 0x4 ? BLACK : WHITE);     // 
            writedata16(ch & 0x2 ? BLACK : WHITE);     // 
            writedata16(ch & 0x1 ? BLACK : WHITE);     // 
            }
          }
#else

        if(invert) {
          writedata16(ch & 0x80 ? WHITE : BLACK);     // 
          writedata16(ch & 0x40 ? WHITE : BLACK);     // 
          writedata16(ch & 0x10 ? WHITE : BLACK);     // 
          writedata16(ch & 0x8 ? WHITE : BLACK);     // 
          writedata16(ch & 0x4 ? WHITE : BLACK);     // 5:8 -> 256:160
          }
        else {
          writedata16(ch & 0x80 ? BLACK : WHITE);     // 
          writedata16(ch & 0x40 ? BLACK : WHITE);     // 
          writedata16(ch & 0x10 ? BLACK : WHITE);     // 
          writedata16(ch & 0x8 ? BLACK : WHITE);     // 
          writedata16(ch & 0x4 ? BLACK : WHITE);     // 5:8 -> 256:160
          }
        
  //	writecommand(CMD_NOP);
#endif
    
        }
#ifndef REAL_SIZE    
      if(k==1 || k==4 || k==6)
        k++;
#endif

      curvideopos=oldvideopos;
      
#ifdef USA_SPI_HW
      ClrWdt();
#endif

      }
    oldvideopos=curvideopos+usedpos+1;

    }
  
  END_WRITE();
    
  LED3 = 0;
  
	}
#endif
#ifdef ILI9341
#define HORIZ_SIZE 256
#define VERT_SIZE 192
#define DO_STRETCH 1
int UpdateScreen(SWORD rowIni, SWORD rowFin, BYTE _i) {
	register int i,j;
	int k,y1,y2,x2,row1,row2,curvideopos,oldvideopos,usedpos;
	register BYTE *p,*p1;
  SWORD D_FILE;
  BYTE ch,invert;

  // ci mette circa 25mS ogni passata... [[dovrebbe essere la metà... (viene chiamata circa 25-30 volte al secondo e ora siamo a 40mS invece di 20, 16/10/22)
  
	// per SPI DMA https://www.microchip.com/forums/m1110777.aspx#1110777

  LED3 = 1;

  row1=rowIni;
  row2=rowFin;
  START_WRITE();
#ifdef DO_STRETCH
  y1=row1/8;
  y2=row2/8;
  x2=HORIZ_SIZE/8;
  y2=VERT_SIZE/8   +1;



  setAddrWindow(0,0,_width,_height);
#else
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
  x2=160/8;
//  y2=128/8;    // 
  y2=192/8  +1  ;    // voglio/devo visualizzare le righe inferiori...
  if(rowIni>=128)
    rowIni=128;
  if(rowFin>=128)
    rowFin=128;

  setAddrWindow(0,rowIni,HORIZ_SIZE,rowFin-rowIni);
#endif
  D_FILE=MAKEWORD(ram_seg[0x000c],ram_seg[0x000d]);     // 400Ch
  if(D_FILE<0x4000)
    return;
  curvideopos=oldvideopos=0;
  for(i=y1; i<y2; i++) {
    curvideopos=oldvideopos;

#ifdef DO_STRETCH
    for(k=0; k<10; k++) {      // scanline del carattere da plottare
#else
    for(k=0; k<8; k++) {      // scanline del carattere da plottare
#endif
      usedpos=0;
      p1=((BYTE*)&ram_seg[D_FILE+curvideopos-0x4000]);    // carattere a inizio riga corrente
      for(j=0; j<HORIZ_SIZE/8; j++) {
        ch=*p1;
        if(ch != 0x76) {
          invert=ch & 0x80;
          ch &= 0x3f;
          // si potrebbe usare I << 8
#ifdef DO_STRETCH
          if(k>=9)
            p=((BYTE*)&rom_seg)+(((DWORD)_i) << 8) /*0x0e00*/+((DWORD)ch)*8+k-2;    
          else if(k>=4)
            p=((BYTE*)&rom_seg)+(((DWORD)_i) << 8) /*0x0e00*/+((DWORD)ch)*8+k-1;    
          else
            p=((BYTE*)&rom_seg)+(((DWORD)_i) << 8) /*0x0e00*/+((DWORD)ch)*8+k;    // pattern del carattere da disegnare
#else
          p=((BYTE*)&rom_seg)+(((DWORD)_i) << 8) /*0x0e00*/+((DWORD)ch)*8+k;    // pattern del carattere da disegnare
#endif
          ch=*p;
          p1++;
          curvideopos++;
          usedpos++;
          }
        else {
          invert=ch=0;
          }

        if(i>=1 && j<x2) {
          if(invert) {
            writedata16(ch & 0x80 ? WHITE : BLACK);     // 
            writedata16(ch & 0x40 ? WHITE : BLACK);     // 
            writedata16(ch & 0x20 ? WHITE : BLACK);     // 
            writedata16(ch & 0x10 ? WHITE : BLACK);     // 
#ifdef DO_STRETCH
            writedata16(ch & 0x10 ? WHITE : BLACK);     // 
#endif
            writedata16(ch & 0x8 ? WHITE : BLACK);     // 
            writedata16(ch & 0x4 ? WHITE : BLACK);     // 
            writedata16(ch & 0x2 ? WHITE : BLACK);     // 
            writedata16(ch & 0x1 ? WHITE : BLACK);     // 
#ifdef DO_STRETCH
            writedata16(ch & 0x1 ? WHITE : BLACK);     // 
#endif
            }
          else {
            writedata16(ch & 0x80 ? BLACK : WHITE);     // 
            writedata16(ch & 0x40 ? BLACK : WHITE);     // 
            writedata16(ch & 0x20 ? BLACK : WHITE);     // 
            writedata16(ch & 0x10 ? BLACK : WHITE);     // 
#ifdef DO_STRETCH
            writedata16(ch & 0x10 ? BLACK : WHITE);     // 
#endif
            writedata16(ch & 0x8 ? BLACK : WHITE);     // 
            writedata16(ch & 0x4 ? BLACK : WHITE);     // 
            writedata16(ch & 0x2 ? BLACK : WHITE);     // 
            writedata16(ch & 0x1 ? BLACK : WHITE);     // 
#ifdef DO_STRETCH
            writedata16(ch & 0x1 ? BLACK : WHITE);     // 
#endif
            }
         }
    
        }

      curvideopos=oldvideopos;
      }
    oldvideopos=curvideopos+usedpos+1;
    ClrWdt();
    }
  
  END_WRITE();
    
  LED3 = 0;
	}
#endif  
#endif
#ifdef ZX81
//https://problemkaputt.de/zxdocs.htm#zx80zx81videomodetextandblockgraphics
#define HORIZ_SIZE 256
#define VERT_SIZE 192
#define REAL_SIZE    1      // diciamo :)
#ifdef ILI9341
#error LCD NON SUPPORTATO! fare
#endif
int UpdateScreen(SWORD rowIni, SWORD rowFin, BYTE _i) {
	register int i,j;
	int k,y1,y2,x2,row1,row2,curvideopos,oldvideopos,usedpos;
	register BYTE *p,*p1;
  SWORD D_FILE;
  BYTE ch,invert;

  // ci mette circa 25mS ogni passata... [[dovrebbe essere la metà... (viene chiamata circa 25-30 volte al secondo e ora siamo a 40mS invece di 20, 16/10/22)
  
	// per SPI DMA https://www.microchip.com/forums/m1110777.aspx#1110777

  LED3 = 1;

  row1=rowIni;
  row2=rowFin;
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
#ifdef REAL_SIZE    
  x2=160/8;
//  y2=128/8;    // 
  y2=192/8  +1  ;    // voglio/devo visualizzare le righe inferiori...
  if(rowIni>=128)
    rowIni=128;
  if(rowFin>=128)
    rowFin=128;
#else
  x2=HORIZ_SIZE/8;
  
  y2=25; /// CAPIRE PERCHé ce n'è uno in +...
#endif
  START_WRITE();
  setAddrWindow(0,rowIni,HORIZ_SIZE,rowFin-rowIni);
  D_FILE=MAKEWORD(ram_seg[0x000c],ram_seg[0x000d]);     // 400Ch
  if(D_FILE<0x4000)
    return;
  curvideopos=oldvideopos=0;
  for(i=y1; i<y2; i++) {
    curvideopos=oldvideopos;
#ifdef REAL_SIZE    
    for(k=0; k<8; k++) {      // scanline del carattere da plottare
#else
    for(k=0; k<8; k++) {      // scanline del carattere da plottare
#endif
      usedpos=0;
      p1=((BYTE*)&ram_seg[D_FILE+curvideopos-0x4000]);    // carattere a inizio riga corrente
      for(j=0; j<HORIZ_SIZE/8; j++) {
        ch=*p1;
        if(ch != 0x76) {
          invert=ch & 0x80;
          ch &= 0x3f;
          // si potrebbe usare I << 8
          p=((BYTE*)&rom_seg)+(((DWORD)_i) << 8) /*0x0e00*/+((DWORD)ch)*8+k;    // pattern del carattere da disegnare
          ch=*p;
          p1++;
          curvideopos++;
          usedpos++;
          }
        else {
          invert=ch=0;
          }

#ifdef REAL_SIZE    
        if(i>=9 && j<x2) {
          if(invert) {
            writedata16(ch & 0x80 ? WHITE : BLACK);     // 
            writedata16(ch & 0x40 ? WHITE : BLACK);     // 
            writedata16(ch & 0x20 ? WHITE : BLACK);     // 
            writedata16(ch & 0x10 ? WHITE : BLACK);     // 
            writedata16(ch & 0x8 ? WHITE : BLACK);     // 
            writedata16(ch & 0x4 ? WHITE : BLACK);     // 
            writedata16(ch & 0x2 ? WHITE : BLACK);     // 
            writedata16(ch & 0x1 ? WHITE : BLACK);     // 
            }
          else {
            writedata16(ch & 0x80 ? BLACK : WHITE);     // 
            writedata16(ch & 0x40 ? BLACK : WHITE);     // 
            writedata16(ch & 0x20 ? BLACK : WHITE);     // 
            writedata16(ch & 0x10 ? BLACK : WHITE);     // 
            writedata16(ch & 0x8 ? BLACK : WHITE);     // 
            writedata16(ch & 0x4 ? BLACK : WHITE);     // 
            writedata16(ch & 0x2 ? BLACK : WHITE);     // 
            writedata16(ch & 0x1 ? BLACK : WHITE);     // 
            }
          }
#else

        if(invert) {
          writedata16(ch & 0x80 ? WHITE : BLACK);     // 
          writedata16(ch & 0x40 ? WHITE : BLACK);     // 
          writedata16(ch & 0x10 ? WHITE : BLACK);     // 
          writedata16(ch & 0x8 ? WHITE : BLACK);     // 
          writedata16(ch & 0x4 ? WHITE : BLACK);     // 5:8 -> 256:160
          }
        else {
          writedata16(ch & 0x80 ? BLACK : WHITE);     // 
          writedata16(ch & 0x40 ? BLACK : WHITE);     // 
          writedata16(ch & 0x10 ? BLACK : WHITE);     // 
          writedata16(ch & 0x8 ? BLACK : WHITE);     // 
          writedata16(ch & 0x4 ? BLACK : WHITE);     // 5:8 -> 256:160
          }
        
#ifdef USA_SPI_HW
        ClrWdt();
#endif
  //	writecommand(CMD_NOP);
#endif
    
        }
#ifndef REAL_SIZE    
      if(k==1 || k==4 || k==6)
        k++;
#endif

      curvideopos=oldvideopos;

      }
    oldvideopos=curvideopos+usedpos+1;

    }
  
  END_WRITE();
    
  LED3 = 0;
  
	}
#endif
#ifdef ZXSPECTRUM
//    6144 bytes worth of bitmap data, starting at memory address &4000
//    768 byte colour attribute data, immediately after the bitmap data at address &5800
//http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout
#endif

#ifdef GALAKSIJA
extern const unsigned char GALAKSIJA_CHARROM[0x800];
#ifdef ST7735
#define HORIZ_SIZE 256
#define VERT_SIZE 128
//#define REAL_SIZE    1      // diciamo :)
int UpdateScreen(SWORD rowIni, SWORD rowFin) {
	register int i,j;
	int k,y1,y2,x1,x2,row1,row2;
	register BYTE *p,*p1;
  BYTE ch;

  // ci mette circa 25mS ogni passata... [[dovrebbe essere la metà... (viene chiamata circa 25-30 volte al secondo e ora siamo a 40mS invece di 20, 16/10/22)
  
	// per SPI DMA https://www.microchip.com/forums/m1110777.aspx#1110777

  LED3 = 1;

  row1=rowIni;
  row2=rowFin;
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
  x1=0;
#ifdef REAL_SIZE    
  x2=160/8;
  y2=10;    // fan 130 vs. 128...
#else
  x2=HORIZ_SIZE/8;
#endif
  START_WRITE();
  setAddrWindow(0,rowIni,_width,rowFin-rowIni);
  for(i=y1; i<y2; i++) {
#ifdef REAL_SIZE    
    for(k=0; k<13; k++) {      // scanline del carattere da plottare
#else
    for(k=2; k<12; k++) {      // scanline del carattere da plottare
#endif
      p1=((BYTE*)&ram_seg[0x0000])+i*(HORIZ_SIZE/8);    // carattere a inizio riga
      for(j=x1; j<x2; j++) {
        ch=*p1 & 0x3f;      // v. pdf slide 2250_presentacija
        if(*p1 & 0x80)
          ch |= 0x40;
        p=((BYTE*)&GALAKSIJA_CHARROM)+(ch)+k*128;    // pattern del carattere da disegnare
        ch=*p;

#ifdef REAL_SIZE    
        writedata16(ch & 0x1 ? BLACK : WHITE);     // 
        writedata16(ch & 0x2 ? BLACK : WHITE);     // 
        writedata16(ch & 0x4 ? BLACK : WHITE);     // 
        writedata16(ch & 0x8 ? BLACK : WHITE);     // 
        writedata16(ch & 0x10 ? BLACK : WHITE);     // 
        writedata16(ch & 0x20 ? BLACK : WHITE);     // 
        writedata16(ch & 0x40 ? BLACK : WHITE);     // 
        writedata16(ch & 0x80 ? BLACK : WHITE);     // 

#else

        writedata16(ch & 0x1 ? BLACK : WHITE);     // 
        writedata16(ch & 0x4 ? BLACK : WHITE);     // 
        writedata16(ch & 0x10 ? BLACK : WHITE);     // 
        writedata16(ch & 0x40 ? BLACK : WHITE);     // 
        writedata16(ch & 0x80 ? BLACK : WHITE);     // 5:8 -> 256:160
        
#ifdef USA_SPI_HW
        ClrWdt();
#endif
  //	writecommand(CMD_NOP);
#endif
    
        p1++;
        }
#ifndef REAL_SIZE    
      if(k==4 || k==7)
        k++;
#endif
      }

    }
  
  END_WRITE();
    
  LED3 = 0;
  
	}
#endif
#ifdef ILI9341
#define HORIZ_SIZE 256
#define VERT_SIZE 128
//#define DO_STRETCH 1
int UpdateScreen(SWORD rowIni, SWORD rowFin) {
	register int i,j;
	int k,y1,y2,x1,x2,row1,row2;
	register BYTE *p,*p1;
  BYTE ch;

  // ci mette circa 25mS ogni passata... [[dovrebbe essere la metà... (viene chiamata circa 25-30 volte al secondo e ora siamo a 40mS invece di 20, 16/10/22)
  
	// per SPI DMA https://www.microchip.com/forums/m1110777.aspx#1110777

  LED3 = 1;

  row1=rowIni;
  row2=rowFin;
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
  x1=0;
  x2=160/8;
  y2=10;    // fan 130 vs. 128...
  START_WRITE();
#ifdef DO_STRETCH
  setAddrWindow(0,0,_width,_height);
#else
  setAddrWindow(0,rowIni,HORIZ_SIZE,rowFin-rowIni);
#endif
  for(i=y1; i<y2; i++) {
#ifdef DO_STRETCH
    for(k=0; k<13; k++) {      // scanline del carattere da plottare
#else
    for(k=0; k<13; k++) {      // scanline del carattere da plottare
#endif
      p1=((BYTE*)&ram_seg[0x0000])+i*(HORIZ_SIZE/8);    // carattere a inizio riga
      for(j=x1; j<x2; j++) {
        ch=*p1 & 0x3f;      // v. pdf slide 2250_presentacija
        if(*p1 & 0x80)
          ch |= 0x40;
        p=((BYTE*)&GALAKSIJA_CHARROM)+(ch)+k*128;    // pattern del carattere da disegnare
        ch=*p;

        writedata16(ch & 0x1 ? BLACK : WHITE);     // 
        writedata16(ch & 0x2 ? BLACK : WHITE);     // 
        writedata16(ch & 0x4 ? BLACK : WHITE);     // 
        writedata16(ch & 0x8 ? BLACK : WHITE);     // 
#ifdef DO_STRETCH
        writedata16(ch & 0x8 ? BLACK : WHITE);     // 
#endif
        writedata16(ch & 0x10 ? BLACK : WHITE);     // 
        writedata16(ch & 0x20 ? BLACK : WHITE);     // 
        writedata16(ch & 0x40 ? BLACK : WHITE);     // 
        writedata16(ch & 0x80 ? BLACK : WHITE);     // 
#ifdef DO_STRETCH
        writedata16(ch & 0x80 ? BLACK : WHITE);     // 
#endif

        p1++;
        }
      ClrWdt();
      }
    }
  END_WRITE();
    
  LED3 = 0;
	}
#endif
#endif
#ifdef MSX
//https://www.angelfire.com/art2/unicorndreams/msx/RR-VDP.html#VDP-StatusReg
const WORD graphColors[16]={BLACK/*transparent*/,BLACK,LIGHTGREEN,BRIGHTGREEN, BLUE,BRIGHTBLUE,RED,BRIGHTCYAN,
  LIGHTRED,BRIGHTRED,YELLOW,LIGHTYELLOW, GREEN,MAGENTA,LIGHTGRAY,WHITE
	};
#ifdef ST7735
//#define REAL_SIZE 1
#define HORIZ_SIZE 256
#define VERT_SIZE 192
int UpdateScreen(SWORD rowIni, SWORD rowFin) {
	register int i;
	UINT16 px,py;
	int row1;
	register BYTE *p1,*p2;
  BYTE ch1,ch2,color,color1,color0;

  // ci mette circa 1mS ogni passata... 4/1/23  @256*256 (erano 1.5 @512x256)
  
	// per SPI DMA https://www.microchip.com/forums/m1110777.aspx#1110777

  BYTE videoMode=((TMS9918Reg[1] & 0x18) >> 2) | ((TMS9918Reg[0] & 2) >> 1);
  WORD charAddress=((WORD)(TMS9918Reg[4] & 7)) << 11;
  WORD videoAddress=((WORD)(TMS9918Reg[2] & 0xf)) << 10;
  WORD colorAddress=((WORD)(TMS9918Reg[3])) << 6;
  WORD spriteAttrAddress=((WORD)(TMS9918Reg[5] & 0x7f)) << 7;
  WORD spritePatternAddress=((WORD)(TMS9918Reg[5] & 0x7)) << 11;


  START_WRITE();

  
  if(!(TMS9918Reg[1] & 0b01000000)) {    // blanked
#ifdef REAL_SIZE
    setAddrWindow(0,rowIni,_width,_height /*rowFin-rowIni*/);
    for(py=0; py<_height; py++)    // bordo/sfondo
      for(px=0; px<_width; px++)
        writedata16(graphColors[TMS9918Reg[7] & 0xf]);
#else
    setAddrWindow(0,rowIni/2,_width,_height /*(rowFin-rowIni)/2*/);
    for(py=0; py<_height; py++)    // bordo/sfondo
      for(px=0; px<_width; px++)
        writedata16(graphColors[TMS9918Reg[7] & 0xf]);
#endif
    }
  else if(rowIni>=0 && rowIni<=192) {
    
//  LED3 = 1;
  
#ifdef REAL_SIZE
  setAddrWindow(0,rowIni,_width,rowFin-rowIni);
  switch(videoMode) {    
    case 0:     // graphics 1
      for(py=rowIni/8; py<rowFin/2/8; py++) {    // 192 
        p2=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
          p1=p2;
					for(px=0; px<20 /*32*/ /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=VideoRAM[charAddress + (ch1*8) + (row1)];
						color=VideoRAM[colorAddress+ch1/8];
						color1=color >> 4; color0=color & 0xf;

						writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
										graphColors[ch2 & 0b01000000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
										graphColors[ch2 & 0b00010000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
										graphColors[ch2 & 0b00000100 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
										graphColors[ch2 & 0b00000001 ? color1 : color0]);
	          }
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      break;
    case 1:     // graphics 2
      for(py=rowIni; py<rowFin/3; py++) {    // 192 linee 
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
          ch1=*p1++;
          ch2=VideoRAM[charAddress + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b01000000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                  graphColors[ch2 & 0b00010000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000100 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                  graphColors[ch2 & 0b00000001 ? color1 : color0]);
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      for(py=rowFin/3; py<(rowFin*2)/3; py++) {    //
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
          ch1=*p1++;
          ch2=VideoRAM[charAddress +2048 + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+2048+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b01000000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                  graphColors[ch2 & 0b00010000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000100 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                  graphColors[ch2 & 0b00000001 ? color1 : color0]);
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      for(py=(rowFin*2)/3; py<rowFin; py++) {    // 
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
          ch1=*p1++;
          ch2=VideoRAM[charAddress +4096 + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+4096+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b01000000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                  graphColors[ch2 & 0b00010000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000100 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                  graphColors[ch2 & 0b00000001 ? color1 : color0]);
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      break;
    case 2:     // multicolor
      for(py=rowIni; py<rowFin; py+=4) {    // 48 linee diventano 96
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*16);
        ch1=*p1++;
        p2=((BYTE*)&VideoRAM[charAddress+ch1]);
        for(px=0; px<HORIZ_SIZE; px+=2) {    // 64 pixel diventano 128
          ch2=*p2++;
          color=VideoRAM[colorAddress+ch2];
          color1=color >> 4; color0=color & 0xf;
          
          // finire!!

          writedata16x2(graphColors[color1],graphColors[color0]);
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      break;
    case 4:     // text 32x24, ~18mS, O0 opp O2, 2/7/24
      color1=TMS9918Reg[7] >> 4; color0=TMS9918Reg[7] & 0xf;
      for(py=rowIni/8; py<128/8 /*rowFin/8 /2*/ ; py++) {    // 192 linee(32 righe char) 
        p2=((BYTE*)&VideoRAM[videoAddress]) + (py*40);
        // mettere bordo
        for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
          p1=p2;
          for(px=0; px<40 /2; px++) {    // 240 pixel 
            ch1=*p1++;
            ch2=VideoRAM[charAddress + (ch1*8) + (row1)];

            writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                    graphColors[ch2 & 0b01000000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                    graphColors[ch2 & 0b00010000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                    graphColors[ch2 & 0b00000100 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                    graphColors[ch2 & 0b00000001 ? color1 : color0]);

            }
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      break;
    }
#else
  setAddrWindow(0,0/*rowIni/2*/,_width,_height/*(rowFin-rowIni)/2*/);
  
  for(py=(_height-(rowFin-rowIni)/2)/2; py; py--)    // bordo/sfondo
    for(px=0; px<_width; px++)
      writedata16(graphColors[TMS9918Reg[7] & 0xf]);
  
  switch(videoMode) {    
    case 0:     // graphics 1
      for(py=rowIni/8; py<rowFin/8; py++) {    // 192 linee diventa 96
        p2=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        // mettere bordo?
        for(row1=0; row1<8; row1+=2) {    // 192 linee diventa 96
          p1=p2;
          for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel diventano 128...
            ch1=*p1++;
            ch2=VideoRAM[charAddress + (ch1*8) + (row1)];
            color=VideoRAM[colorAddress+ch1/8];
            color1=color >> 4; color0=color & 0xf;

            writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                    graphColors[ch2 & 0b00100000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                    graphColors[ch2 & 0b00000010 ? color1 : color0]);
            writedata16(graphColors[ch2 & 0b00000001 ? color1 : color0]); // 128->160
            }
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
handle_sprites:
      { // OVVIAMENTE sarebbe meglio gestirli riga per riga...!
      struct SPRITE_ATTR *sa;
      BYTE ssize=TMS9918Reg[1] & 2 ? 32 : 8,smag=TMS9918Reg[1] & 1 ? 16 : 8;
      BYTE j;
      
      sa=((struct SPRITE_ATTR *)&VideoRAM[spriteAttrAddress]);
      for(i=0; i<32; i++) {
        struct SPRITE_ATTR *sa2;
        
        if(sa->ypos>=LAST_SPRITE_YPOS)
          continue;
        
        j=smag*(ssize==32 ? 2 : 1);
        sa2=sa+1;
        for(j=i+1; j<32; j++) {
          if(sa2->ypos < LAST_SPRITE_YPOS) {
            
            if((sa2->ypos>=sa->ypos && sa2->ypos<=sa->ypos+j) &&
              (sa2->xpos>=sa->xpos && sa2->xpos<=sa->xpos+j)) {
              // controllare solo i pixel accesi, a 1!
              TMS9918RegS |= 0b00100000;
              }
            // poi ci sarebbe il flag 5 sprite per riga!
            }
          sa2++;
          }
        
        p1=((BYTE*)&VideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize);
        j=ssize;
        if(sa->ypos > 0xe1)     // Y diventa negativo..
          ;
        if(sa->eclock)     // X diventa negativo..
          ;
        setAddrWindow(sa->xpos/2,sa->ypos/2,8/2,8/2);
        color1=sa->color; color0=TMS9918Reg[7] & 0xf;
        
        for(py=0; py<ssize; py++) {
          ch1=*p1++;
          if(smag==16) {
            writedata16x2(graphColors[ch1 & 0b10000000 ? color1 : color0],graphColors[ch1 & 0b10000000 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b1000000 ? color1 : color0],graphColors[ch1 & 0b1000000 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b100000 ? color1 : color0],graphColors[ch1 & 0b100000 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b10000 ? color1 : color0],graphColors[ch1 & 0b10000 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b1000 ? color1 : color0],graphColors[ch1 & 0b1000 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b100 ? color1 : color0],graphColors[ch1 & 0b100 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b10 ? color1 : color0],graphColors[ch1 & 0b10 ? color1 : color0]);
            writedata16x2(graphColors[ch1 & 0b1 ? color1 : color0],graphColors[ch1 & 0b1 ? color1 : color0]);
            }
          else {
            writedata16(graphColors[ch1 & 0b10000000 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b1000000 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b100000 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b10000 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b1000 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b100 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b10 ? color1 : color0]); 
            writedata16(graphColors[ch1 & 0b1 ? color1 : color0]); 
            }
          j--;
          switch(j) {   // gestisco i "quadranti" sprite messi a cazzo...
            case 23:
              setAddrWindow(sa->xpos/2,sa->ypos/2+8/2,8/2,8/2);
              break;
            case 15:
              p1=((BYTE*)&VideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize) + 16;
              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2,8/2,8/2);
              break;
            case 7:
              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2+8/2,8/2,8/2);
              break;
            default:
              break;
            }
          }
          
        sa++;
        }
      }
      break;
    case 1:     // graphics 2
      for(py=rowIni; py<rowFin/3; py+=2) {    // 192 linee diventa 96
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        // mettere bordo?
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px+=2) {    // 256 pixel diventano 128...
          ch1=*p1++;
          ch2=VideoRAM[charAddress + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b00100000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000010 ? color1 : color0]);
          writedata16(graphColors[ch2 & 0b00000001 ? color1 : color0]); // 128->160
          
          ch1=*p1++;
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      for(py=rowFin/3; py<(rowFin*2)/3; py+=2) {    //
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        // mettere bordo?
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px+=2) {    // 256 pixel diventano 128...
          ch1=*p1++;
          ch2=VideoRAM[charAddress +2048 + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+2048+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b00100000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000010 ? color1 : color0]);
          writedata16(graphColors[ch2 & 0b00000001 ? color1 : color0]); // 128->160
          
          ch1=*p1++;
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      for(py=(rowFin*2)/3; py<rowFin; py+=2) {    // 
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        // mettere bordo?
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px+=2) {    // 256 pixel diventano 128...
          ch1=*p1++;
          ch2=VideoRAM[charAddress +4096 + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+4096+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b00100000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000010 ? color1 : color0]);
          writedata16(graphColors[ch2 & 0b00000001 ? color1 : color0]); // 128->160
          
          ch1=*p1++;
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      goto handle_sprites;
      break;
    case 2:     // multicolor
      for(py=rowIni; py<rowFin; py+=4) {    // 48 linee diventano 96
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*16);
        ch1=*p1++;
        p2=((BYTE*)&VideoRAM[charAddress+ch1]);
        // mettere bordo
        for(px=0; px<HORIZ_SIZE; px+=2) {    // 64 pixel diventano 128
          ch2=*p2++;
          color=VideoRAM[colorAddress+ch2];
          color1=color >> 4; color0=color & 0xf;
          
          // finire!!

          writedata16x2(graphColors[color1],graphColors[color0]);
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      goto handle_sprites;
      break;
    case 4:     // text 32x24
      color1=TMS9918Reg[7] >> 4; color0=TMS9918Reg[7] & 0xf;
      for(py=rowIni/8; py<rowFin/8; py++) {    // 192 linee(32 righe char) diventa 96
        p2=((BYTE*)&VideoRAM[videoAddress]) + (py*40);
        // mettere bordo
        for(row1=0; row1<8; row1+=2) {    // 192 linee(32 righe char) diventa 96
          p1=p2;
          for(px=0; px<40; px++) {    // 240 pixel diventano 120...
            ch1=*p1++;
            ch2=VideoRAM[charAddress + (ch1*8) + (row1)];

            writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                    graphColors[ch2 & 0b00100000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b00010000 ? color1 : color0],
                    graphColors[ch2 & 0b00001000 ? color1 : color0]);

            }
          }
    #ifdef USA_SPI_HW
        ClrWdt();
    #endif
        }
      break;
    }
  setAddrWindow(0,((rowFin-rowIni)/2)+(_height-(rowFin-rowIni)/2)/2,
    _width,(_height-(rowFin-rowIni)/2)/2);
  for(py=(_height-(rowFin-rowIni)/2)/2; py; py--)    // bordo/sfondo
    for(px=0; px<_width; px++)
      writedata16(graphColors[TMS9918Reg[7] & 0xf]);
  
#endif

  END_WRITE();

  //	writecommand(CMD_NOP);
#endif
#ifdef ILI9341
#define HORIZ_SIZE 256
#define VERT_SIZE 192
#define DO_STRETCH 1
int UpdateScreen(SWORD rowIni, SWORD rowFin) {
	register int i;
	UINT16 px,py;
	int row1;
	register BYTE *p1,*p2;
  BYTE ch1,ch2,color,color1,color0;

  BYTE videoMode=((TMS9918Reg[1] & 0x18) >> 2) | ((TMS9918Reg[0] & 2) >> 1);
  WORD charAddress=((WORD)(TMS9918Reg[4] & 7)) << 11;
  WORD videoAddress=((WORD)(TMS9918Reg[2] & 0xf)) << 10;
  WORD colorAddress=((WORD)(TMS9918Reg[3])) << 6;
  WORD spriteAttrAddress=((WORD)(TMS9918Reg[5] & 0x7f)) << 7;
  WORD spritePatternAddress=((WORD)(TMS9918Reg[5] & 0x7)) << 11;

  START_WRITE();
  
  if(!(TMS9918Reg[1] & 0b01000000)) {    // blanked
#ifdef DO_STRETCH
    setAddrWindow(0,0,_width,_height);
    for(py=0; py<_height; py++)    // bordo/sfondo
      for(px=0; px<_width; px++)
        writedata16(graphColors[TMS9918Reg[7] & 0xf]);
#else
    setAddrWindow((_width-HORIZ_SIZE)/2,(_height-VERT_SIZE)/2,HORIZ_SIZE,VERT_SIZE);
    for(py=0; py<VERT_SIZE; py++)    // bordo/sfondo
      for(px=0; px<HORIZ_SIZE; px++)
        writedata16(graphColors[TMS9918Reg[7] & 0xf]);
#endif
    }
  else if(rowIni>=0 && rowIni<=192) {
    
//  LED3 = 1;
  
#ifdef DO_STRETCH
  setAddrWindow(0,0,_width,_height);
#else
  setAddrWindow((_width-HORIZ_SIZE)/2,(_height-VERT_SIZE)/2,HORIZ_SIZE,VERT_SIZE);
#endif
  switch(videoMode) {    
    case 0:     // graphics 1
      for(py=rowIni/8; py<rowFin/8; py++) {    // 192 
        p2=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
#ifdef DO_STRETCH
        for(row1=0; row1<10; row1++) {    // 192 linee(32 righe char) 
#else
        for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
#endif
          p1=p2;
					for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
						ch1=*p1++;
#ifdef DO_STRETCH
            if(row1>=9)
              ch2=VideoRAM[charAddress + (ch1*8) + (row1-2)]; 
            else if(row1>=4)
              ch2=VideoRAM[charAddress + (ch1*8) + (row1-1)]; 
            else
              ch2=VideoRAM[charAddress + (ch1*8) + (row1)]; 
#else
            ch2=VideoRAM[charAddress + (ch1*8) + (row1)];
#endif
						color=VideoRAM[colorAddress+ch1/8];
						color1=color >> 4; color0=color & 0xf;

						writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
										graphColors[ch2 & 0b01000000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
										graphColors[ch2 & 0b00010000 ? color1 : color0]);
#ifdef DO_STRETCH
						writedata16(graphColors[ch2 & 0b00010000 ? color1 : color0]);
#endif
						writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
										graphColors[ch2 & 0b00000100 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
										graphColors[ch2 & 0b00000001 ? color1 : color0]);
#ifdef DO_STRETCH
						writedata16(graphColors[ch2 & 0b00000001 ? color1 : color0]);
#endif
	          }
          }
        ClrWdt();
        }
handle_sprites:
      { // OVVIAMENTE sarebbe meglio gestirli riga per riga...!
      struct SPRITE_ATTR *sa;
      BYTE ssize=TMS9918Reg[1] & 2 ? 32 : 8,smag=TMS9918Reg[1] & 1 ? 16 : 8;
      BYTE j;
      
      sa=((struct SPRITE_ATTR *)&VideoRAM[spriteAttrAddress]);
      for(i=0; i<32; i++) {
        struct SPRITE_ATTR *sa2;
        
        if(sa->ypos>=LAST_SPRITE_YPOS)
          continue;
        
        j=smag*(ssize==32 ? 2 : 1);
        sa2=sa+1;
        for(j=i+1; j<32; j++) {
          if(sa2->ypos < LAST_SPRITE_YPOS) {
            
            if((sa2->ypos>=sa->ypos && sa2->ypos<=sa->ypos+j) &&
              (sa2->xpos>=sa->xpos && sa2->xpos<=sa->xpos+j)) {
              // controllare solo i pixel accesi, a 1!
              TMS9918RegS |= 0b00100000;
              }
            // poi ci sarebbe il flag 5 sprite per riga!
            }
          sa2++;
          }
        
        p1=((BYTE*)&VideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize);
        j=ssize;
        if(sa->ypos > 0xe1)     // Y diventa negativo..
          ;
        if(sa->eclock)     // X diventa negativo..
          ;
        setAddrWindow(sa->xpos/2,sa->ypos/2,8/2,8/2);
        color1=sa->color; color0=TMS9918Reg[7] & 0xf;
        
        for(py=0; py<ssize; py++) {
          ch1=*p1++;
          if(smag==16) {
            writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],graphColors[ch2 & 0b10000000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b1000000 ? color1 : color0],graphColors[ch2 & 0b1000000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b100000 ? color1 : color0],graphColors[ch2 & 0b100000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b10000 ? color1 : color0],graphColors[ch2 & 0b10000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b1000 ? color1 : color0],graphColors[ch2 & 0b1000 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b100 ? color1 : color0],graphColors[ch2 & 0b100 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b10 ? color1 : color0],graphColors[ch2 & 0b10 ? color1 : color0]);
            writedata16x2(graphColors[ch2 & 0b1 ? color1 : color0],graphColors[ch2 & 0b1 ? color1 : color0]);
            }
          else {
            writedata16(graphColors[ch2 & 0b10000000 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b1000000 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b100000 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b10000 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b1000 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b100 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b10 ? color1 : color0]); 
            writedata16(graphColors[ch2 & 0b1 ? color1 : color0]); 
            }
          j--;
          switch(j) {   // gestisco i "quadranti" sprite messi a cazzo...
            case 23:
              setAddrWindow(sa->xpos/2,sa->ypos/2+8/2,8/2,8/2);
              break;
            case 15:
              p1=((BYTE*)&VideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize) + 16;
              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2,8/2,8/2);
              break;
            case 7:
              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2+8/2,8/2,8/2);
              break;
            default:
              break;
            }
          }
          
        sa++;
        }
      }
      break;
    case 1:     // graphics 2
      for(py=rowIni; py<rowFin/3; py++) {    // 192 linee 
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
          ch1=*p1++;
          ch2=VideoRAM[charAddress + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b01000000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                  graphColors[ch2 & 0b00010000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000100 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                  graphColors[ch2 & 0b00000001 ? color1 : color0]);
          }
        ClrWdt();
        }
      for(py=rowFin/3; py<(rowFin*2)/3; py++) {    //
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
          ch1=*p1++;
          ch2=VideoRAM[charAddress +2048 + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+2048+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b01000000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                  graphColors[ch2 & 0b00010000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000100 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                  graphColors[ch2 & 0b00000001 ? color1 : color0]);
          }
        ClrWdt();
        }
      for(py=(rowFin*2)/3; py<rowFin; py++) {    // 
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
        for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
          ch1=*p1++;
          ch2=VideoRAM[charAddress +4096 + (ch1*8) + (py & 7)];
          color=VideoRAM[colorAddress+4096+py];
          color1=color >> 4; color0=color & 0xf;

          writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                  graphColors[ch2 & 0b01000000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                  graphColors[ch2 & 0b00010000 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                  graphColors[ch2 & 0b00000100 ? color1 : color0]);
          writedata16x2(graphColors[ch2 & 0b00000010 ? color1 : color0],
                  graphColors[ch2 & 0b00000001 ? color1 : color0]);
          }
        ClrWdt();
        }
      goto handle_sprites;
      break;
    case 2:     // multicolor
      for(py=rowIni; py<rowFin; py+=4) {    // 48 linee diventano 96
        p1=((BYTE*)&VideoRAM[videoAddress]) + (py*16);
        ch1=*p1++;
        p2=((BYTE*)&VideoRAM[charAddress+ch1]);
        for(px=0; px<HORIZ_SIZE; px+=2) {    // 64 pixel diventano 128
          ch2=*p2++;
          color=VideoRAM[colorAddress+ch2];
          color1=color >> 4; color0=color & 0xf;
          
          // finire!!

          writedata16x2(graphColors[color1],graphColors[color0]);
          }
        ClrWdt();
        }
      goto handle_sprites;
      break;
    case 4:     // text 32x24, ~120mS, O1 opp O2, 2/7/24
      color1=TMS9918Reg[7] >> 4; color0=TMS9918Reg[7] & 0xf;
      for(py=rowIni/8; py<rowFin/8; py++) {    // 192 linee(32 righe char) 
        p2=((BYTE*)&VideoRAM[videoAddress]) + (py*40);
        // mettere bordo
#ifdef DO_STRETCH
        for(row1=0; row1<10; row1++) {    // 192 linee(32 righe char) 
#else
        for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
#endif
          p1=p2;
          for(px=0; px<40; px++) {    // 240 pixel 
            ch1=*p1++;
#ifdef DO_STRETCH
            if(row1>=9)
              ch2=VideoRAM[charAddress + (ch1*8) + (row1-2)]; 
            else if(row1>=4)
              ch2=VideoRAM[charAddress + (ch1*8) + (row1-1)]; 
            else
              ch2=VideoRAM[charAddress + (ch1*8) + (row1)]; 
#else
            ch2=VideoRAM[charAddress + (ch1*8) + (row1)];
#endif

            writedata16x2(graphColors[ch2 & 0b10000000 ? color1 : color0],
                    graphColors[ch2 & 0b01000000 ? color1 : color0]);
#ifdef DO_STRETCH
						writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
              graphColors[ch2 & 0b00100000 ? color1 : color0]);
						writedata16(graphColors[ch2 & 0b00010000 ? color1 : color0]);
#else
            writedata16x2(graphColors[ch2 & 0b00100000 ? color1 : color0],
                    graphColors[ch2 & 0b00010000 ? color1 : color0]);
#endif
            writedata16x2(graphColors[ch2 & 0b00001000 ? color1 : color0],
                    graphColors[ch2 & 0b00000100 ? color1 : color0]);
#ifdef DO_STRETCH
						writedata16(graphColors[ch2 & 0b00000100 ? color1 : color0]);
#endif

            }
//          for(px=0; px<8; px++)     // 240 -> 256
//            writedata16x2(0,0);
          }
        ClrWdt();
        }
      break;
    }

  END_WRITE();    // 
  
#endif  

   
    
//    LED3 = 0;

    }
  }  
#endif


                

int main(void) {
  int i;

  // disable JTAG port
//  DDPCONbits.JTAGEN = 0;
  
  SYSKEY = 0x00000000;
  SYSKEY = 0xAA996655;    //qua non dovrebbe servire essendo 1° giro (v. IOLWAY
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 0;      // PPS Unlock
  SYSKEY = 0x00000000;
  
  RPB9Rbits.RPB9R = 1;        // Assign RPB9 as U3TX, pin 22 (ok arduino)
// NON SI PUO'... sistemare se serve  U3RXRbits.U3RXR = 2;      // Assign RPB8 as U3RX, pin 21 (ok arduino)
#ifdef ST7735
#ifdef USA_SPI_HW
  RPG8Rbits.RPG8R = 6;        // Assign RPG8 as SDO2, pin 6
//  SDI2Rbits.SDI2R = 1;        // Assign RPG7 as SDI2, pin 5
#endif
  RPD5Rbits.RPD5R = 12;        // Assign RPD5 as OC1, pin 53; anche vaga uscita audio :)
  RPD1Rbits.RPD1R = 12;        // Assign RPD1 as OC1, pin 49; buzzer
#endif
#ifdef ILI9341
  RPB1Rbits.RPB1R = 12;        // Assign RPB1 as OC7, pin 15; buzzer
#endif

//#define DEBUG_TESTREFCLK  
#ifdef DEBUG_TESTREFCLK
// test REFCLK
#ifdef ST7735
  PPSOutput(4,RPC4,REFCLKO2);   // RefClk su pin 1 (RG15, buzzer)
	REFOCONbits.ROSSLP=1;
	REFOCONbits.ROSEL=1;
	REFOCONbits.RODIV=0;
	REFOCONbits.ROON=1;
	TRISFbits.TRISF3=1;
#endif
#ifdef ILI9341
//  RPD9Rbits.RPD9R = 15;        // Assign RPD9 (SDA) as RefClk3, pin 43
  RPB8Rbits.RPB8R = 15;        // Assign RPB8 as RefClk3, pin 21 + comodo
	REFO3CONbits.SIDL=0;
	REFO3CONbits.RSLP=1;
	REFO3CONbits.ROSEL=1;   // PBclk
	REFO3CONbits.RODIV=1;   // :2
	REFO3CONbits.OE=1;
	REFO3CONbits.ON=1;
//	TRISDbits.TRISD9=1;
	TRISBbits.TRISB8=1;
#endif
#endif
  
  
#ifdef ILI9341
//  while(PB4DIVbits.PBDIVRDY == 0);    // velocizzo clock porte! bah cambia pochissimo... v. WRITE_LATB
//  PB4DIVbits.PBDIV = 0;   //200MHz va un pizzico meglio/più veloce/coerente SEMBRA un 30-50%! 9/1/23
//  PB4DIVbits.ON = 1;
#endif

  SYSKEY = 0x00000000;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 1;      // PPS Lock
  SYSKEY = 0x00000000;

   // Disable all Interrupts
  __builtin_disable_interrupts();
  
//  SPLLCONbits.PLLMULT=10;
  
  OSCTUN=0;
  OSCCONbits.FRCDIV=0;
  
  // Switch to FRCDIV, SYSCLK=8MHz
  SYSKEY=0xAA996655;
  SYSKEY=0x556699AA;
  OSCCONbits.NOSC=0x00; // FRC
  OSCCONbits.OSWEN=1;
  SYSKEY=0x33333333;
  while(OSCCONbits.OSWEN) {
    Nop();
    }
    // At this point, SYSCLK is ~8MHz derived directly from FRC
 //http://www.microchip.com/forums/m840347.aspx
  // Switch back to FRCPLL, SYSCLK=200MHz
  SYSKEY=0xAA996655;
  SYSKEY=0x556699AA;
  OSCCONbits.NOSC=0x01; // SPLL
  OSCCONbits.OSWEN=1;
  SYSKEY=0x33333333;
  while(OSCCONbits.OSWEN) {
    Nop();
    }
  // At this point, SYSCLK is ~200MHz derived from FRC+PLL
//***
  mySYSTEMConfigPerformance();

    
#ifdef ST7735
	TRISB=0b0000000000110000;			// AN4,5 (rb4..5)
	TRISC=0b0000000000000000;
	TRISD=0b0000000000001100;			// 2 pulsanti
	TRISE=0b0000000000000000;			// 3 led
	TRISF=0b0000000000000000;			// 
	TRISG=0b0000000000000000;			// SPI2 (rg6..8)

  ANSELB=0;
  ANSELE=0;
  ANSELG=0;

  CNPUDbits.CNPUD2=1;   // switch/pulsanti
  CNPUDbits.CNPUD3=1;
  CNPUGbits.CNPUG6=1;   // I2C tanto per
  CNPUGbits.CNPUG8=1;  
#endif

#ifdef ILI9341
	TRISB=0b0000000000000001;			// pulsante; [ AN ?? ]
	TRISC=0b0000000000000000;
	TRISD=0b0000000000000000;			// 2led
	TRISE=0b0000000000000000;			// led
	TRISF=0b0000000000000001;			// pulsante
	TRISG=0b0000000000000000;			// SPI2 (rg6..8)

  ANSELB=0;
  ANSELE=0;
  ANSELG=0;

  CNPUFbits.CNPUF0=1;   // switch/pulsanti
  CNPUBbits.CNPUB0=1;
  CNPUDbits.CNPUD9=1;   // I2C tanto per
  CNPUDbits.CNPUD10=1;  
#endif
      
  
  Timer_Init();
  PWM_Init();
  UART_Init(/*230400L*/ 115200L);

  myINTEnableSystemMultiVectoredInt();
  ShortDelay(50000); 

  
/*for(;;) {// test timing ok 2024
  __delay_us(5);
  ClrWdt();
  LATB ^= 0xffff;
  }*/

  
//    	ColdReset=0;    Emulate(0);

#ifndef USING_SIMULATOR
//#ifndef __DEBUG
#ifdef ST7735
  Adafruit_ST7735_1(0,0,0,0,-1);
  Adafruit_ST7735_initR(INITR_BLACKTAB);
#endif
#ifdef ILI9341
  Adafruit_ILI9341_8(8, 9, 10, 11, 12, 13, 14);
	begin(0);
  __delay_ms(200);
#endif
  
//  displayInit(NULL);
  
#ifdef m_LCDBLBit
  m_LCDBLBit=1;
#endif
  
//	begin();
	clearScreen();

// init done
	setTextWrap(1);
//	setTextColor2(WHITE, BLACK);

	drawBG();
  
  __delay_ms(200);
  
	gfx_fillRect(3,_TFTHEIGHT-20,_TFTWIDTH-6,16,BLACK);
#ifdef SKYNET
 	setTextColor(BLUE);
	LCDXY(3,14);
	gfx_print("(emulating 4x20 LCD)");
#endif
#ifdef NEZ80
 	setTextColor(BLUE);
	LCDXY(1,13);
	gfx_print("(emulating 8 digit display)");
#endif
#ifdef GALAKSIJA
 	setTextColor(BRIGHTRED);
	LCDXY(2,13);
	gfx_print("(emulating GALAKSIJA)");
  __delay_ms(1000);
#endif
#ifdef ZX80
 	setTextColor(BRIGHTCYAN);
#ifdef ILI9341
	LCDXY(18,23);
#else
	LCDXY(5,13);
#endif
	gfx_print("(emulating ZX80)");
  __delay_ms(1000);
#endif
#ifdef MSX
 	setTextColor(BRIGHTBLUE);
#ifdef ILI9341
	LCDXY(18,23);
#else
	LCDXY(5,14);
#endif
	gfx_print("(emulating MSX)");
  __delay_ms(1000);
#endif
  
  LCDCls();

//#endif
#endif
  

#ifdef SKYNET 
//  memcpy(rom_seg,CHIESA_BIN,0xa08);
  memcpy(rom_seg,Z803_BIN,0x16be);
//  memcpy(rom_seg2,CASANET_BIN,0x997);
  memcpy(rom_seg2,CASANET3_BIN,0x280);
//  memcpy(rom_seg2,SKYBASIC_BIN,0x31fa);
#endif
#ifdef NEZ80 
  memcpy(rom_seg,Z80NE_BIN,0x400);
#endif
#ifdef GALAKSIJA
  memcpy(rom_seg,GALAKSIJA_BIN,0x1000);
  memcpy(rom_seg2,GALAKSIJA_BIN2,0x1000);
#endif
#ifdef ZX80
  memcpy(rom_seg,ZX80_BIN,0x1000);
#endif
#ifdef ZX81
  memcpy(rom_seg,ZX81_BIN,0x2000);
#endif
#ifdef MSX
  memcpy(rom_seg,MSX_BIN,0x8000);
#endif
  
  initHW();

        
	ColdReset=0;

  Emulate(0);

  }


enum CACHE_MODE {
  UNCACHED=0x02,
  WB_WA=0x03,
  WT_WA=0x01,
  WT_NWA=0x00,
/* Cache Coherency Attributes */
//#define _CACHE_WRITEBACK_WRITEALLOCATE      3
//#define _CACHE_WRITETHROUGH_WRITEALLOCATE   1
//#define _CACHE_WRITETHROUGH_NOWRITEALLOCATE 0
//#define _CACHE_DISABLE                      2
  };
void mySYSTEMConfigPerformance(void) {
  unsigned int PLLIDIV;
  unsigned int PLLMUL;
  unsigned int PLLODIV;
  float CLK2USEC;
  unsigned int SYSCLK;
  static unsigned char PLLODIVVAL[]={
    2,2,4,8,16,32,32,32
    };
	unsigned int cp0;

  PLLIDIV=SPLLCONbits.PLLIDIV+1;
  PLLMUL=SPLLCONbits.PLLMULT+1;
  PLLODIV=PLLODIVVAL[SPLLCONbits.PLLODIV];

  SYSCLK=(FOSC*PLLMUL)/(PLLIDIV*PLLODIV);
  CLK2USEC=SYSCLK/1000000.0f;

  SYSKEY = 0x0;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;

  if(SYSCLK<=60000000)
    PRECONbits.PFMWS=0;
  else if(SYSCLK<=120000000)
    PRECONbits.PFMWS=1;
  else if(SYSCLK<=200000000)
    PRECONbits.PFMWS=2;
  else if(SYSCLK<=252000000)
    PRECONbits.PFMWS=4;
  else
    PRECONbits.PFMWS=7;

  PRECONbits.PFMSECEN=0;    // non c'è nella versione "2019" ...
  PRECONbits.PREFEN=0x1;

  SYSKEY = 0x0;

  // Set up caching
  cp0 = _mfc0(16, 0);
  cp0 &= ~0x07;
  cp0 |= WB_WA /*0b011*/; // K0 = Cacheable, non-coherent, write-back, write allocate
   _mtc0(16, 0, cp0);  
  }

void myINTEnableSystemMultiVectoredInt(void) {

  PRISS = 0x76543210;
  INTCONSET = _INTCON_MVEC_MASK /*0x1000*/;    //MVEC
  asm volatile ("ei");
  //__builtin_enable_interrupts();
  }

/* CP0.Count counts at half the CPU rate */
#define TICK_HZ (CPU_HZ / 2)

/* wait at least usec microseconds */
#if 0
void delay_usec(unsigned long usec) {
unsigned long start, stop;

  /* get start ticks */
  start = readCP0Count();

  /* calculate number of ticks for the given number of microseconds */
  stop = (usec * 1000000) / TICK_HZ;

  /* add start value */
  stop += start;

  /* wait till Count reaches the stop value */
  while (readCP0Count() < stop)
    ;
  }
#endif

void xdelay_us(uint32_t us) {
  
  if(us == 0) {
    return;
    }
  unsigned long start_count = ReadCoreTimer /*_CP0_GET_COUNT*/();
  unsigned long now_count;
  long cycles = ((GetSystemClock() + 1000000U) / 2000000U) * us;
  do {
    now_count = ReadCoreTimer /*_CP0_GET_COUNT*/();
    } while ((unsigned long)(now_count-start_count) < cycles);
  }

void __attribute__((used)) __delay_us(unsigned int usec) {
  unsigned int tWait, tStart;

  tWait=(GetSystemClock()/2000000)*usec;
  tStart=_mfc0(9,0);
  while((_mfc0(9,0)-tStart)<tWait)
    ClrWdt();        // wait for the time to pass
  }

void __attribute__((used)) __delay_ms(unsigned int ms) {
  
  for(;ms;ms--)
    __delay_us(1000);
  }

// ===========================================================================
// ShortDelay - Delays (blocking) for a very short period (in CoreTimer Ticks)
// ---------------------------------------------------------------------------
// The DelayCount is specified in Core-Timer Ticks.
// This function uses the CoreTimer to determine the length of the delay.
// The CoreTimer runs at half the system clock. 100MHz
// If CPU_CLOCK_HZ is defined as 80000000UL, 80MHz/2 = 40MHz or 1LSB = 25nS).
// Use US_TO_CT_TICKS to convert from uS to CoreTimer Ticks.
// ---------------------------------------------------------------------------

void ShortDelay(                       // Short Delay
  DWORD DelayCount)                   // Delay Time (CoreTimer Ticks)
{
  DWORD StartTime;                    // Start Time
  StartTime = ReadCoreTimer();         // Get CoreTimer value for StartTime
  while( (DWORD)(ReadCoreTimer() - StartTime) < DelayCount)
    ClrWdt();
  }
 

void Timer_Init(void) {

  T2CON=0;
  T2CONbits.TCS = 0;                  // clock from peripheral clock
  T2CONbits.TCKPS = 7;                // 1:256 prescaler (pwm clock=390625Hz)
  T2CONbits.T32 = 0;                  // 16bit
//  PR2 = 2000;                         // rollover every n clocks; 2000 = 50KHz
  PR2 = 65535;                         // per ora faccio solo onda quadra
  T2CONbits.TON = 1;                  // start timer per PWM
  
  // TIMER 3 INITIALIZATION (TIMER IS USED AS A TRIGGER SOURCE FOR ALL CHANNELS).
  T3CON=0;
  T3CONbits.TCS = 0;                  // clock from peripheral clock
  T3CONbits.TCKPS = 4;                // 1:16 prescaler
  PR3 = (GetPeripheralClock()/16)/1600;         // 1600Hz
  T3CONbits.TON = 1;                  // start timer 

  IPC3bits.T3IP=4;            // set IPL 4, sub-priority 2??
  IPC3bits.T3IS=0;
  IEC0bits.T3IE=1;             // enable Timer 3 interrupt se si vuole

	}

void PWM_Init(void) {

  SYSKEY = 0x00000000;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;
  CFGCONbits.OCACLK=0;      // sceglie timer per PWM
  SYSKEY = 0x00000000;
  
#ifdef ILI9341
	OC7CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC7R    = 500;		 // su PIC32 è read-only!
//  OC7RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC7R    = 32768;		 // su PIC32 è read-only!
  OC7RS   = 0;        // per ora faccio solo onda quadra [v. SID reg. 0-1]
  OC7CONbits.ON = 1;   // on
#endif
#ifdef ST7735
	OC1CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC1R    = 500;		 // su PIC32 è read-only!
//  OC1RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC1R    = 32768;		 // su PIC32 è read-only!
  OC1RS   = 0;        // per ora faccio solo onda quadra [v. SID reg. 0-1]
  OC1CONbits.ON = 1;   // on
#endif

  }

void UART_Init(DWORD baudRate) {
  
  U3MODE=0b0000000000001000;    // BRGH=1
  U3STA= 0b0000010000000000;    // TXEN
  DWORD baudRateDivider = ((GetPeripheralClock()/(4*baudRate))-1);
  U3BRG=baudRateDivider;
  U3MODEbits.ON=1;
  
#if 0
  ANSELDCLR = 0xFFFF;
  CFGCONbits.IOLOCK = 0;      // PPS Unlock
  RPD11Rbits.RPD11R = 3;        // Assign RPD11 as U1TX
  U1RXRbits.U1RXR = 3;      // Assign RPD10 as U1RX
  CFGCONbits.IOLOCK = 1;      // PPS Lock

  // Baud related stuffs.
  U1MODEbits.BRGH = 1;      // Setup High baud rates.
  unsigned long int baudRateDivider = ((GetSystemClock()/(4*baudRate))-1);
  U1BRG = baudRateDivider;  // set BRG

  // UART Configuration
  U1MODEbits.ON = 1;    // UART1 module is Enabled
  U1STAbits.UTXEN = 1;  // TX is enabled
  U1STAbits.URXEN = 1;  // RX is enabled

  // UART Rx interrupt configuration.
  IFS1bits.U1RXIF = 0;  // Clear the interrupt flag
  IFS1bits.U1TXIF = 0;  // Clear the interrupt flag

  INTCONbits.MVEC = 1;  // Multi vector interrupts.

  IEC1bits.U1RXIE = 1;  // Rx interrupt enable
  IEC1bits.U1EIE = 1;
  IPC7bits.U1IP = 7;    // Rx Interrupt priority level
  IPC7bits.U1IS = 3;    // Rx Interrupt sub priority level
#endif
  }

char BusyUART1(void) {
  
  return(!U3STAbits.TRMT);
  }

void putsUART1(unsigned int *buffer) {
  char *temp_ptr = (char *)buffer;

    // transmit till NULL character is encountered 

  if(U3MODEbits.PDSEL == 3)        /* check if TX is 8bits or 9bits */
    {
        while(*buffer) {
            while(U3STAbits.UTXBF); /* wait if the buffer is full */
            U3TXREG = *buffer++;    /* transfer data word to TX reg */
        }
    }
  else {
        while(*temp_ptr) {
            while(U3STAbits.UTXBF);  /* wait if the buffer is full */
            U3TXREG = *temp_ptr++;   /* transfer data byte to TX reg */
        }
    }
  }

unsigned int ReadUART1(void) {
  
  if(U3MODEbits.PDSEL == 3)
    return (U3RXREG);
  else
    return (U3RXREG & 0xFF);
  }

void WriteUART1(unsigned int data) {
  
  if(U3MODEbits.PDSEL == 3)
    U3TXREG = data;
  else
    U3TXREG = data & 0xFF;
  }

void __ISR(_UART3_RX_VECTOR) UART3_ISR(void) {
  
  LED3 ^= 1;    // LED to indicate the ISR.
  char curChar = U3RXREG;
  IFS4bits.U3RXIF = 0;  // Clear the interrupt flag!
  }


int emulateKBD(BYTE ch) {
  int i;

#ifdef SKYNET
#elif ZX80
  
	switch(toupper(ch)) {   // solo maiuscole qua..
    case 0:
      for(i=0; i<8; i++)
        Keyboard[i]=0xff;
      // FORSE non dovremmo rilasciare i modifier, qua???
      break;
		case '£':
			Keyboard[0] &= ~0b00000001;
		case ' ':
			Keyboard[7] &= ~0b00000001;
			break;
      // minuscole/maiuscole?
		case 'A':
			Keyboard[1] &= ~0b00000001;
			break;
		case '|':    // OR ...
			Keyboard[0] &= ~0b00000001;
		case 'B':
			Keyboard[7] &= ~0b00010000;
			break;
		case '?':
			Keyboard[0] &= ~0b00000001;
		case 'C':
			Keyboard[0] &= ~0b00001000;
			break;
		case 'D':
			Keyboard[1] &= ~0b00000100;
			break;
		case 'E':
			Keyboard[2] &= ~0b00000100;
			break;
		case 'F':
			Keyboard[1] &= ~0b00001000;
			break;
		case 'G':
			Keyboard[1] &= ~0b00010000;
			break;
		case '^':   // ** doppio asterisco in effetti ...
			Keyboard[0] &= ~0b00000001;
		case 'H':
			Keyboard[6] &= ~0b00010000;
			break;
		case '(':
			Keyboard[0] &= ~0b00000001;
		case 'I':
			Keyboard[5] &= ~0b00000100;
			break;
		case '-':
			Keyboard[0] &= ~0b00000001;
		case 'J':
			Keyboard[6] &= ~0b00001000;
			break;
		case '+':
			Keyboard[0] &= ~0b00000001;
		case 'K':
			Keyboard[6] &= ~0b00000100;
			break;
		case '=':
			Keyboard[0] &= ~0b00000001;
		case 'L':
			Keyboard[6] &= ~0b00000010;
			break;
		case '>':
			Keyboard[0] &= ~0b00000001;
		case 'M':
			Keyboard[7] &= ~0b00000100;
			break;
		case '<':
			Keyboard[0] &= ~0b00000001;
		case 'N':
			Keyboard[7] &= ~0b00001000;
			break;
		case ')':
			Keyboard[0] &= ~0b00000001;
		case 'O':
			Keyboard[5] &= ~0b00000010;
			break;
		case '*':
			Keyboard[0] &= ~0b00000001;
		case 'P':
			Keyboard[5] &= ~0b00000001;
			break;
		case 'Q':
			Keyboard[2] &= ~0b00000001;
			break;
		case 'R':
			Keyboard[2] &= ~0b00001000;
			break;
		case 'S':
			Keyboard[1] &= ~0b00000010;
			break;
		case 'T':
			Keyboard[2] &= ~0b00010000;
			break;
		case '$':
			Keyboard[0] &= ~0b00000001;
		case 'U':
			Keyboard[5] &= ~0b00001000;
			break;
		case '/':
			Keyboard[0] &= ~0b00000001;
		case 'V':
			Keyboard[0] &= ~0b00010000;
			break;
		case 'W':
			Keyboard[2] &= ~0b00000010;
			break;
		case ';':
			Keyboard[0] &= ~0b00000001;
		case 'X':
			Keyboard[0] &= ~0b00000100;
			break;
		case '\"':
			Keyboard[0] &= ~0b00000001;
		case 'Y':
			Keyboard[5] &= ~0b00010000;
			break;
		case ':':
			Keyboard[0] &= ~0b00000001;
		case 'Z':
			Keyboard[0] &= ~0b00000010;
			break;
      
		case '0':
			Keyboard[4] &= ~0b00000001;
			break;
		case '1':
			Keyboard[3] &= ~0b00000001;
			break;
		case '2':
			Keyboard[3] &= ~0b00000010;
			break;
		case '3':
			Keyboard[3] &= ~0b00000100;
			break;
		case '4':
			Keyboard[3] &= ~0b00001000;
			break;
		case '5':
			Keyboard[3] &= ~0b00010000;
			break;
		case '6':
			Keyboard[4] &= ~0b00010000;
			break;
		case '7':
			Keyboard[4] &= ~0b00001000;
			break;
		case '8':
			Keyboard[4] &= ~0b00000100;
			break;
		case '9':
			Keyboard[4] &= ~0b00000010;
			break;
		case ',':
			Keyboard[0] &= ~0b00000001;
		case '.':
			Keyboard[7] &= ~0b00000010;
			break;
		case '\r':
			Keyboard[6] &= ~0b00000001;
			break;
      
		case 0x1f:    // Shift 
			Keyboard[0] &= ~0b00000001;
			break;

		}
  
#elif ZX81
 
	switch(toupper(ch)) {   // solo maiuscole qua..
    case 0:
      for(i=0; i<8; i++)
        Keyboard[i]=0xff;
      // FORSE non dovremmo rilasciare i modifier, qua???
      break;
		case '£':
			Keyboard[0] &= ~0b00000001;
		case ' ':
			Keyboard[7] &= ~0b00000001;
			break;
      // minuscole/maiuscole?
		case 'A':
			Keyboard[1] &= ~0b00000001;
			break;
		case '|':    // OR ...
			Keyboard[0] &= ~0b00000001;
		case 'B':
			Keyboard[7] &= ~0b00010000;
			break;
		case '?':
			Keyboard[0] &= ~0b00000001;
		case 'C':
			Keyboard[0] &= ~0b00001000;
			break;
		case 'D':
			Keyboard[1] &= ~0b00000100;
			break;
		case 'E':
			Keyboard[2] &= ~0b00000100;
			break;
		case 'F':
			Keyboard[1] &= ~0b00001000;
			break;
		case 'G':
			Keyboard[1] &= ~0b00010000;
			break;
		case '^':   // ** doppio asterisco in effetti ...
			Keyboard[0] &= ~0b00000001;
		case 'H':
			Keyboard[6] &= ~0b00010000;
			break;
		case '(':
			Keyboard[0] &= ~0b00000001;
		case 'I':
			Keyboard[5] &= ~0b00000100;
			break;
		case '-':
			Keyboard[0] &= ~0b00000001;
		case 'J':
			Keyboard[6] &= ~0b00001000;
			break;
		case '+':
			Keyboard[0] &= ~0b00000001;
		case 'K':
			Keyboard[6] &= ~0b00000100;
			break;
		case '=':
			Keyboard[0] &= ~0b00000001;
		case 'L':
			Keyboard[6] &= ~0b00000010;
			break;
		case '>':
			Keyboard[0] &= ~0b00000001;
		case 'M':
			Keyboard[7] &= ~0b00000100;
			break;
		case '<':
			Keyboard[0] &= ~0b00000001;
		case 'N':
			Keyboard[7] &= ~0b00001000;
			break;
		case ')':
			Keyboard[0] &= ~0b00000001;
		case 'O':
			Keyboard[5] &= ~0b00000010;
			break;
		case '*':
			Keyboard[0] &= ~0b00000001;
		case 'P':
			Keyboard[5] &= ~0b00000001;
			break;
		case 'Q':
			Keyboard[2] &= ~0b00000001;
			break;
		case 'R':
			Keyboard[2] &= ~0b00001000;
			break;
		case 'S':
			Keyboard[1] &= ~0b00000010;
			break;
		case 'T':
			Keyboard[2] &= ~0b00010000;
			break;
		case '$':
			Keyboard[0] &= ~0b00000001;
		case 'U':
			Keyboard[5] &= ~0b00001000;
			break;
		case '/':
			Keyboard[0] &= ~0b00000001;
		case 'V':
			Keyboard[0] &= ~0b00010000;
			break;
		case 'W':
			Keyboard[2] &= ~0b00000010;
			break;
		case ';':
			Keyboard[0] &= ~0b00000001;
		case 'X':
			Keyboard[0] &= ~0b00000100;
			break;
		case '\"':
			Keyboard[0] &= ~0b00000001;
		case 'Y':
			Keyboard[5] &= ~0b00010000;
			break;
		case ':':
			Keyboard[0] &= ~0b00000001;
		case 'Z':
			Keyboard[0] &= ~0b00000010;
			break;
      
		case '0':
			Keyboard[4] &= ~0b00000001;
			break;
		case '1':
			Keyboard[3] &= ~0b00000001;
			break;
		case '2':
			Keyboard[3] &= ~0b00000010;
			break;
		case '3':
			Keyboard[3] &= ~0b00000100;
			break;
		case '4':
			Keyboard[3] &= ~0b00001000;
			break;
		case '5':
			Keyboard[3] &= ~0b00010000;
			break;
		case '6':
			Keyboard[4] &= ~0b00010000;
			break;
		case '7':
			Keyboard[4] &= ~0b00001000;
			break;
		case '8':
			Keyboard[4] &= ~0b00000100;
			break;
		case '9':
			Keyboard[4] &= ~0b00000010;
			break;
		case ',':
			Keyboard[0] &= ~0b00000001;
		case '.':
			Keyboard[7] &= ~0b00000010;
			break;
		case '\r':
			Keyboard[6] &= ~0b00000001;
			break;
      
		case 0x1f:    // Shift 
			Keyboard[0] &= ~0b00000001;
			break;
      
		}

#elif NEZ80
	switch(ch) {
    case 0:
      Keyboard[0]=0;
      break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			Keyboard[0] = ch - '0';
			Keyboard[0] |= 0x80;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			Keyboard[0] = ch - 'A' +10;
			Keyboard[0] |= 0x80;
			break;
		case '\r':      // CTRL-0 ossia mostra contenuto memoria e/o registri
			Keyboard[0] = 0x10;
			Keyboard[0] |= 0x80;
			break;
		case '!':       // CTRL-1 ossia torna a inizio inserimento
			Keyboard[0] = 0x11;
			Keyboard[0] |= 0x80;
			break;
		case '+':       // CTRL-2 ossia mostra registri interni
			Keyboard[0] = 0x12;
			Keyboard[0] |= 0x80;
			break;
		case '/':       // CTRL-3 ossia avanti step/step
			Keyboard[0] = 0x13;
			Keyboard[0] |= 0x80;
			break;
		case ' ':       // CTRL-4 = RUN (esegui)
			Keyboard[0] = 0x14;
			Keyboard[0] |= 0x80;
			break;
		case '*':       // esegui Ext
			Keyboard[0] = 0x17;
			Keyboard[0] |= 0x80;
			break;
		case '.':       // 
			Keyboard[0] = 0x1f;
			Keyboard[0] |= 0x80;
			break;
		// CTRL-5 e CTRL-6 sono usati per il registratore a cassette...
		}
  
  
#elif GALAKSIJA

	switch(ch) {
    case 0:
      for(i=0; i<8; i++)
        Keyboard[i]=0xff;
      // FORSE non dovremmo rilasciare i modifier, qua???
      break;
		case ' ':
			Keyboard[3] &= ~0b10000000;
			break;
      // minuscole/maiuscole?
		case 'A':
			Keyboard[0] &= ~0b00000010;
			break;
		case 'B':
			Keyboard[0] &= ~0b00000100;
			break;
		case 'C':
			Keyboard[0] &= ~0b00001000;
			break;
		case 'D':
			Keyboard[0] &= ~0b00010000;
			break;
		case 'E':
			Keyboard[0] &= ~0b00100000;
			break;
		case 'F':
			Keyboard[0] &= ~0b01000000;
			break;
		case 'G':
			Keyboard[0] &= ~0b10000000;
			break;
		case 'H':
			Keyboard[1] &= ~0b00000001;
			break;
		case 'I':
			Keyboard[1] &= ~0b00000010;
			break;
		case 'J':
			Keyboard[1] &= ~0b00000100;
			break;
		case 'K':
			Keyboard[1] &= ~0b00001000;
			break;
		case 'L':
			Keyboard[1] &= ~0b00010000;
			break;
		case 'M':
			Keyboard[1] &= ~0b00100000;
			break;
		case 'N':
			Keyboard[1] &= ~0b01000000;
			break;
		case 'O':
			Keyboard[1] &= ~0b10000000;
			break;
		case 'P':
			Keyboard[2] &= ~0b00000001;
			break;
		case 'Q':
			Keyboard[2] &= ~0b00000010;
			break;
		case 'R':
			Keyboard[2] &= ~0b00000100;
			break;
		case 'S':
			Keyboard[2] &= ~0b00001000;
			break;
		case 'T':
			Keyboard[2] &= ~0b00010000;
			break;
		case 'U':
			Keyboard[2] &= ~0b00100000;
			break;
		case 'V':
			Keyboard[2] &= ~0b01000000;
			break;
		case 'W':
			Keyboard[2] &= ~0b10000000;
			break;
		case 'X':
			Keyboard[3] &= ~0b00000001;
			break;
		case 'Y':
			Keyboard[3] &= ~0b00000010;
			break;
		case 'Z':
			Keyboard[3] &= ~0b00000100;
			break;
      
		case '0':
			Keyboard[4] &= ~0b00000001;
			break;
		case '!':
			Keyboard[6] &= ~0b00100000;
		case '1':
			Keyboard[4] &= ~0b00000010;
			break;
		case '"':
			Keyboard[6] &= ~0b00100000;
		case '2':
			Keyboard[4] &= ~0b00000100;
			break;
		case '#':
			Keyboard[6] &= ~0b00100000;
		case '3':
			Keyboard[4] &= ~0b00001000;
			break;
		case '$':
			Keyboard[6] &= ~0b00100000;
		case '4':
			Keyboard[4] &= ~0b00010000;
			break;
		case '%':
			Keyboard[6] &= ~0b00100000;
		case '5':
			Keyboard[4] &= ~0b00100000;
			break;
		case '&':
			Keyboard[6] &= ~0b00100000;
		case '6':
			Keyboard[4] &= ~0b01000000;
			break;
		case '\'':
			Keyboard[6] &= ~0b00100000;
		case '7':
			Keyboard[4] &= ~0b10000000;
			break;
		case '(':
			Keyboard[6] &= ~0b00100000;
		case '8':
			Keyboard[5] &= ~0b00000001;
			break;
		case ')':
			Keyboard[6] &= ~0b00100000;
		case '9':
			Keyboard[5] &= ~0b00000010;
			break;
		case '+':
			Keyboard[6] &= ~0b00100000;
		case ';':
			Keyboard[5] &= ~0b00000100;		 // 
			break;
		case '*':
			Keyboard[6] &= ~0b00100000;
		case ':':
			Keyboard[5] &= ~0b00001000;		 // 
			break;
		case '<':
			Keyboard[6] &= ~0b00100000;
		case ',':		// ,
			Keyboard[5] &= ~0b00010000;		 // ,
			break;
		case '-':
			Keyboard[6] &= ~0b00100000;
		case '=':
			Keyboard[5] &= ~0b00100000;
			break;
		case '>':
			Keyboard[6] &= ~0b00100000;
		case '.':
			Keyboard[5] &= ~0b01000000;
			break;
		case '?':
			Keyboard[6] &= ~0b00100000;
		case '/':
			Keyboard[5] &= ~0b10000000;
			break;
		case '\r':
			Keyboard[6] &= ~0b00000001;
			break;
      
		case 0x1:     // CRSR up
			Keyboard[3] &= ~0b00001000;
			break;
		case 0x2:     // CRSR down
			Keyboard[3] &= ~0b00010000;
			break;
		case 0x3:     // CRSR left
			Keyboard[3] &= ~0b00100000;
			break;
		case 0x4:     // CRSR right
			Keyboard[3] &= ~0b01000000;
			break;
		case 0x5:     // DEL
			Keyboard[6] &= ~0b00001000;
			break;
		case 0x1f:    // Shift 
			Keyboard[6] &= ~0b00100000;
			break;
		case 0x1e:    // List
			Keyboard[6] &= ~0b00010000;
			break;
		case 0x1d:    // Break
			Keyboard[6] &= ~0b00000010;
			break;
		case 0x1c:    // Repeat
			Keyboard[6] &= ~0b00000100;
			break;
		}

#elif MSX
//  https://map.grauw.nl/articles/keymatrix.php

	switch(ch) {
    case 0:
      for(i=0; i<11; i++)
        Keyboard[i]=0xff;
      // FORSE non dovremmo rilasciare i modifier, qua???
      break;
		case ' ':
			Keyboard[8] &= ~0b00000001;
			break;
      // minuscole/maiuscole?
		case 'A':
			Keyboard[2] &= ~0b01000000;
			break;
		case 'B':
			Keyboard[2] &= ~0b10000000;
			break;
		case 'C':
			Keyboard[3] &= ~0b00000001;
			break;
		case 'D':
			Keyboard[3] &= ~0b00000010;
			break;
		case 'E':
			Keyboard[3] &= ~0b00000100;
			break;
		case 'F':
			Keyboard[3] &= ~0b00001000;
			break;
		case 'G':
			Keyboard[3] &= ~0b00010000;
			break;
		case 'H':
			Keyboard[3] &= ~0b00100000;
			break;
		case 'I':
			Keyboard[3] &= ~0b01000000;
			break;
		case 'J':
			Keyboard[3] &= ~0b10000000;
			break;
		case 'K':
			Keyboard[4] &= ~0b00000001;
			break;
		case 'L':
			Keyboard[4] &= ~0b00000010;
			break;
		case 'M':
			Keyboard[4] &= ~0b00000100;
			break;
		case 'N':
			Keyboard[4] &= ~0b00001000;
			break;
		case 'O':
			Keyboard[4] &= ~0b00010000;
			break;
		case 'P':
			Keyboard[4] &= ~0b00100000;
			break;
		case 'Q':
			Keyboard[4] &= ~0b01000000;
			break;
		case 'R':
			Keyboard[4] &= ~0b10000000;
			break;
		case 'S':
			Keyboard[5] &= ~0b00000001;
			break;
		case 'T':
			Keyboard[5] &= ~0b00000010;
			break;
		case 'U':
			Keyboard[5] &= ~0b00000100;
			break;
		case 'V':
			Keyboard[5] &= ~0b00001000;
			break;
		case 'W':
			Keyboard[5] &= ~0b00010000;
			break;
		case 'X':
			Keyboard[5] &= ~0b00100000;
			break;
		case 'Y':
			Keyboard[5] &= ~0b01000000;
			break;
		case 'Z':
			Keyboard[5] &= ~0b10000000;
			break;
      
		case ')':
			Keyboard[6] &= ~0b00000001;
		case '0':
			Keyboard[0] &= ~0b00000001;
			break;
		case '!':
			Keyboard[6] &= ~0b00000001;
		case '1':
			Keyboard[0] &= ~0b00000010;
			break;
		case '@':
			Keyboard[6] &= ~0b00000001;
		case '2':
			Keyboard[0] &= ~0b00000100;
			break;
		case '#':
			Keyboard[6] &= ~0b00000001;
		case '3':
			Keyboard[0] &= ~0b00001000;
			break;
		case '$':
			Keyboard[6] &= ~0b00000001;
		case '4':
			Keyboard[0] &= ~0b00010000;
			break;
		case '%':
			Keyboard[6] &= ~0b00000001;
		case '5':
			Keyboard[0] &= ~0b00100000;
			break;
		case '^':
			Keyboard[6] &= ~0b00000001;
		case '6':
			Keyboard[0] &= ~0b01000000;
			break;
		case '&':
			Keyboard[6] &= ~0b00000001;
		case '7':
			Keyboard[0] &= ~0b10000000;
			break;
		case '*':
			Keyboard[6] &= ~0b00000001;
		case '8':
			Keyboard[1] &= ~0b00000001;
			break;
		case '(':
			Keyboard[6] &= ~0b00000001;
		case '9':
			Keyboard[1] &= ~0b00000010;
			break;
		case ':':
			Keyboard[6] &= ~0b00000001;
		case ';':
			Keyboard[1] &= ~0b10000000;		 // 
			break;
		case '\"':
			Keyboard[6] &= ~0b00000001;
		case '\'':
			Keyboard[2] &= ~0b00000001;		 // 
			break;
		case '<':
			Keyboard[6] &= ~0b00000001;
		case ',':		// ,
			Keyboard[2] &= ~0b00000100;		 // ,
			break;
		case '-':
			Keyboard[1] &= ~0b00000100;
			break;
		case '+':
			Keyboard[6] &= ~0b00000001;
		case '=':
			Keyboard[5] &= ~0b00100000;
			break;
		case '>':
			Keyboard[6] &= ~0b00000001;
		case '.':
			Keyboard[2] &= ~0b00001000;
			break;
		case '?':
			Keyboard[6] &= ~0b00000001;
		case '/':
			Keyboard[2] &= ~0b00010000;
			break;
		case 0x8:     // BS
			Keyboard[7] &= ~0b00100000;
			break;
		case '\t':
			Keyboard[7] &= ~0b00001000;
			break;
		case '\r':
			Keyboard[7] &= ~0b10000000;
			break;
		case '\x1b':
			Keyboard[7] &= ~0b00000100;
			break;
//		case VK_PAUSE:
//			Keyboard[7] &= ~0b00010000;
//			break;
      
		case 0xa6:    // F6
			Keyboard[6] &= ~0b00000001;
		case 0xa1:    // F1
			Keyboard[6] &= ~0b00100000;
			break;
		case 0xa7:    // F7
			Keyboard[6] &= ~0b00000001;
		case 0xa2:    // F2
			Keyboard[6] &= ~0b01000000;
			break;
		case 0xa8:    // F8
			Keyboard[6] &= ~0b00000001;
		case 0xa3:    // F3
			Keyboard[6] &= ~0b10000000;
			break;
		case 0xa9:    // F9
			Keyboard[6] &= ~0b00000001;
		case 0xa4:    // F4
			Keyboard[6] &= ~0b00000001;
			break;
		case 0xaa:    // F10
			Keyboard[6] &= ~0b00000001;
		case 0xa5:    // F5
			Keyboard[6] &= ~0b00000010;
			break;
      
		case 0x1:     // CRSR up
			Keyboard[8] &= ~0b00100000;
			break;
		case 0x2:     // CRSR down
			Keyboard[8] &= ~0b01000000;
			break;
		case 0x3:     // CRSR left
			Keyboard[8] &= ~0b00010000;
			break;
		case 0x4:     // CRSR right
			Keyboard[8] &= ~0b10000000;
			break;
		case 0x5:     // DEL
			Keyboard[8] &= ~0b00001000;
			break;
		case 0x6:			// INS
			Keyboard[8] &= ~0b00000100;
			break;
//		case VK_HOME:    // 
//			Keyboard[8] &= ~0b00000010;
//			break;
		case 0x1f:    // Shift 
			Keyboard[6] &= ~0b00000001;
			break;
		case 0x1e:    // Graph
			Keyboard[6] &= ~0b00000100;
			break;
		case 0x1d:    // Stop
			Keyboard[7] &= ~0b00010000;
			break;
		case 0x1c:    // Select
			Keyboard[7] &= ~0b01000000;
			break;
		case 0x1a:    // Control
			Keyboard[6] &= ~0b00000010;
			break;
		case 0x19:    // CAPS
			Keyboard[6] &= ~0b00001000;
			break;
		}

#endif

  
no_irq:
    ;
  }

BYTE whichKeysFeed=0;
char keysFeed[64]={0};
volatile BYTE keysFeedPtr=255;
#ifdef NEZ80
const char *keysFeed1="02E1\r";
const char *keysFeed2="03E5\r";
#elif ZX80
//const char *keysFeed1="O\"ZX\",80\r";   // print
const char *keysFeed1="4Y\r";   // 
//const char *keysFeed2="1Fi=1 410\r";   // 1 for I=1 to 10  \x1F= SHIFT no non va così..
const char *keysFeed2="1O17,\r";   // 1 
const char *keysFeed3="2Oi,\r";   // 2 PRINT I,
const char *keysFeed4="3G1\r";   // 
const char *keysFeed5="R\r";   // RUN
const char *keysFeed6="A\r";   // LIST
#elif ZX81
const char *keysFeed1="O\"ZX\",80\r";   // print
//const char *keysFeed2="1Fi=1 419\r";   // 1 for I=1 to 19  \x1F= SHIFT no non va così..
const char *keysFeed2="1\r";   // 1 
const char *keysFeed3="2Oi,\r";   // 2 PRINT I,
const char *keysFeed4="3G1\r";   // 
const char *keysFeed5="R\r";   // RUN
const char *keysFeed6="A\r";   // LIST
#elif GALAKSIJA
const char *keysFeed2="PRINT \"CIAO\"\r";   // no minuscole
const char *keysFeed1="PRINT 512,476    \r";   // 
const char *keysFeed3="PRINT 25003000 : P. \"b\"\r";   // 
const char *keysFeed4="PRINT MEM,15,3*7 \r";   // 
const char *keysFeed5="PRONT\r";   // 
#elif MSX
const char *keysFeed2="PRINT \"CIAO\"\r";   // 
const char *keysFeed1="PRINT 512,476 :COLOR 3\r";   // 11=giallo
const char *keysFeed3="PRINT 25003000 : PRINT VDP(1)\r";   // 
const char *keysFeed4="PRINT FRE(0),15,7*7 \r";   // 
const char *keysFeed5="SOUND 0,172:SOUND 1,1:SOUND 8,12:SOUND 7,190\r";   // C 4° ottava=261Hz  https://www.msx.org/wiki/SOUND
//const char *keysFeed6="SCREEN ,,0:PRONT \x19 \r";   // CAPS-lock non va... 4/7/24
const char *keysFeed6="\x19";   // CAPS-lock non va... 4/7/24
#endif

void __ISR(_TIMER_3_VECTOR,ipl4SRS) TMR_ISR(void) {
// https://www.microchip.com/forums/m842396.aspx per IRQ priority ecc
  static BYTE divider,divider2;
  static WORD dividerTim;
  static WORD dividerEmulKbd;
  static BYTE keysFeedPhase=0;
  int i;

#ifdef SKYNET
#define TIMIRQ_DIVIDER 32   // 
#elif NEZ80
#define TIMIRQ_DIVIDER 32   // si potrebbe modificare e rallentare qua, v.sotto
#elif ZX80
#define TIMIRQ_DIVIDER 32    // non usato (v. Refresh _r)
#elif ZX81
#define TIMIRQ_DIVIDER 1    // HSync...
#elif GALAKSIJA
#define TIMIRQ_DIVIDER 32   // 50Hz
#elif MSX
#define TIMIRQ_DIVIDER 32   // 3.911uS
#endif
  
  //LED2 ^= 1;      // check timing: 1600Hz, 9/11/19 (fuck berlin day)) 2022 ok fuck UK ;) & anyone
  
  divider++;
#ifdef USING_SIMULATOR
  if(divider>=1) {   // 
#else
  if(divider>=TIMIRQ_DIVIDER) {   //
#endif
    divider=0;
//    CIA1IRQ=1;
#ifdef NEZ80
    TIMIRQ=1;
#endif
#ifdef ZX80
    TIMIRQ=1;     // 50Hz, come VSync NON USATO!
#endif
#ifdef ZX81
    TIMIRQ=1;     // 15625Hz, come HSync
#endif
#ifdef GALAKSIJA
    TIMIRQ=1;     // 50Hz, come VSync
#endif
#ifdef MSXTURBO
    TIMIRQ=1;     // ??Hz
#endif
#ifdef MSX
    i8253RegR[0]++;
    if(!i8253RegR[0] && !(i8251RegW[2] & 0b00001000))
      //VERIFICARE frequenza e se c'è!
      TIMIRQ=1;     // ??Hz
#endif
    }


  dividerTim++;
  if(dividerTim>=1600) {   // 1Hz RTC
#ifdef SKYNET
    // vedere registro 0A, che ha i divisori...
    // i146818RAM[10] & 15
    dividerTim=0;
    if(!(i146818RAM[11] & 0x80)) {    // SET
      i146818RAM[10] |= 0x80;
      currentTime.sec++;
      if(currentTime.sec >= 60) {
        currentTime.sec=0;
        currentTime.min++;
        if(currentTime.min >= 60) {
          currentTime.min=0;
          currentTime.hour++;
          if( ((i146818RAM[11] & 2) && currentTime.hour >= 24) || 
            (!(i146818RAM[11] & 2) && currentTime.hour >= 12) ) {
            currentTime.hour=0;
            currentDate.mday++;
            i=dayOfMonth[currentDate.mon-1];
            if((i==28) && !(currentDate.year % 4))
              i++;
            if(currentDate.mday > i) {		// (rimangono i secoli...)
              currentDate.mday=0;
              currentDate.mon++;
              if(currentDate.mon > 12) {		// 
                currentDate.mon=1;
                currentDate.year++;
                }
              }
            }
          }
        } 
      i146818RAM[12] |= 0x90;
      i146818RAM[10] &= ~0x80;
      } 
    else
      i146818RAM[10] &= ~0x80;
    // inserire Alarm... :)
    i146818RAM[12] |= 0x40;     // in effetti dice che deve fare a 1024Hz! o forse è l'altro flag, bit3 ecc
    if(i146818RAM[12] & 0x40 && i146818RAM[11] & 0x40 ||
       i146818RAM[12] & 0x20 && i146818RAM[11] & 0x20 ||
       i146818RAM[12] & 0x10 && i146818RAM[11] & 0x10)     
      i146818RAM[12] |= 0x80;
    if(i146818RAM[12] & 0x80)     
      RTCIRQ=1;
#endif
		} 
  

#ifdef NEZ80
  divider2++;
  if(divider2>7) {   // 1=800 Hz (ma rallento se no non va la tastiera...)
          LED3^=1;    // confermato 10/7/21
          ram_seg[0x2e1]=0x55;    // test visualizza ram

    divider2=0;
		MUXcnt++;		// sarebbe 1KHz in effetti...
		MUXcnt &= 0xf;
		}
#endif
#ifdef MSXTURBO
//This timer is present only on the MSX turbo R      
  TIMEr++;     // 3.911uS
#endif
  
#if defined(NEZ80) || defined(ZX80) || defined(ZX81) || defined(GALAKSIJA) || defined(MSX)
  if(keysFeedPtr==255)      // EOL
    goto fine;
  if(keysFeedPtr==254) {    // NEW string
    keysFeedPtr=0;
    keysFeedPhase=0;
#ifdef NEZ80
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
			case 1:
				strcpy(keysFeed,keysFeed2);
				break;
      }
		whichKeysFeed++;
		if(whichKeysFeed>=2)
			whichKeysFeed=0;
#elif ZX80
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
			case 1:
				strcpy(keysFeed,keysFeed2);
				break;
			case 2:
				strcpy(keysFeed,keysFeed3);
				break;
			case 3:
				strcpy(keysFeed,keysFeed4);
				break;
			case 4:
				strcpy(keysFeed,keysFeed5);
				break;
			case 5:
				strcpy(keysFeed,keysFeed6);
				break;
			}
		whichKeysFeed++;
		if(whichKeysFeed>5)
			whichKeysFeed=0;
#elif ZX81
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
			case 1:
				strcpy(keysFeed,keysFeed2);
				break;
			case 2:
				strcpy(keysFeed,keysFeed3);
				break;
			case 3:
				strcpy(keysFeed,keysFeed4);
				break;
			case 4:
				strcpy(keysFeed,keysFeed5);
				break;
			case 5:
				strcpy(keysFeed,keysFeed6);
				break;
			}
		whichKeysFeed++;
		if(whichKeysFeed>5)
			whichKeysFeed=0;
#elif GALAKSIJA
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
			case 1:
				strcpy(keysFeed,keysFeed2);
				break;
			case 2:
				strcpy(keysFeed,keysFeed3);
				break;
			case 3:
				strcpy(keysFeed,keysFeed4);
				break;
			case 4:
				strcpy(keysFeed,keysFeed5);
				break;
			}
		whichKeysFeed++;
		if(whichKeysFeed>4)
			whichKeysFeed=0;
#elif MSX
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
			case 1:
				strcpy(keysFeed,keysFeed2);
				break;
			case 2:
				strcpy(keysFeed,keysFeed3);
				break;
			case 3:
				strcpy(keysFeed,keysFeed4);
				break;
			case 4:
				strcpy(keysFeed,keysFeed5);
				break;
			case 5:
				strcpy(keysFeed,keysFeed6);
				break;
			}
		whichKeysFeed++;
		if(whichKeysFeed>5)
			whichKeysFeed=0;
#endif
//    goto fine;
		}
  if(keysFeed[keysFeedPtr]) {
    dividerEmulKbd++;
    if(dividerEmulKbd>=400 /*300*/) {   // ~.2Hz per emulazione tastiera! (più veloce di tot non va...))
      dividerEmulKbd=0;
      if(!keysFeedPhase) {
#ifdef NEZ80
        if(!(MUXcnt & 8))
          goto wait_kbd;
#endif
        keysFeedPhase=1;
        emulateKBD(keysFeed[keysFeedPtr]);
        }
      else {
#ifdef NEZ80
        if(Keyboard[0] & 0x80)
          goto wait_kbd;
#endif
        keysFeedPhase=0;
#ifdef ZX80
        emulateKBD(NULL);
#endif
#ifdef ZX81
        emulateKBD(NULL);
#endif
#ifdef GALAKSIJA
        emulateKBD(NULL);
#endif
#ifdef MSX
        emulateKBD(NULL);
#endif
        keysFeedPtr++;
wait_kbd: ;
        }
      }
    }
  else
    keysFeedPtr=255;
#endif
    
fine:
  IFS0CLR = _IFS0_T3IF_MASK;
  }

// ---------------------------------------------------------------------------------------
// declared static in case exception condition would prevent
// auto variable being created
static enum {
	EXCEP_IRQ = 0,			// interrupt
	EXCEP_AdEL = 4,			// address error exception (load or ifetch)
	EXCEP_AdES,				// address error exception (store)
	EXCEP_IBE,				// bus error (ifetch)
	EXCEP_DBE,				// bus error (load/store)
	EXCEP_Sys,				// syscall
	EXCEP_Bp,				// breakpoint
	EXCEP_RI,				// reserved instruction
	EXCEP_CpU,				// coprocessor unusable
	EXCEP_Overflow,			// arithmetic overflow
	EXCEP_Trap,				// trap (possible divide by zero)
	EXCEP_IS1 = 16,			// implementation specfic 1
	EXCEP_CEU,				// CorExtend Unuseable
	EXCEP_C2E				// coprocessor 2
  } _excep_code;

static unsigned int _epc_code;
static unsigned int _excep_addr;

void __attribute__((weak)) _general_exception_handler(uint32_t __attribute__((unused)) code, uint32_t __attribute__((unused)) address) {
  }

void __attribute__((nomips16,used)) _general_exception_handler_entry(void) {
  
	asm volatile("mfc0 %0,$13" : "=r" (_epc_code));
	asm volatile("mfc0 %0,$14" : "=r" (_excep_addr));

	_excep_code = (_epc_code & 0x0000007C) >> 2;

  _general_exception_handler(_excep_code, _excep_addr);

	while (1)	{
		// Examine _excep_code to identify the type of exception
		// Examine _excep_addr to find the address that caused the exception
    }
  }


