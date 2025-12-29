#define APPNAME "Z80win"


// Windows Header Files:
#include <windows.h>
#include <stdint.h>

// Local Header Files
#include "z80win.h"
#include "resource.h"
#include <stdio.h>
#include "cjson.h"

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
	#define IS_WIN32 TRUE
#else
	#define IS_WIN32 FALSE
#endif
#define IS_NT      IS_WIN32 && (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN32S  IS_WIN32 && (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95 (BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

// Global Variables:
HINSTANCE g_hinst;
HANDLE hAccelTable;
char szAppName[] = APPNAME; // The name of this application
char INIFile[] = APPNAME".ini";
char szTitle[]   = APPNAME; // The title bar text
int AppXSize=534,AppYSize=358,AppYSizeR,YXRatio=1;		// così il rapporto è ok, ev. ingandire o rimpicciolire
LARGE_INTEGER pcFrequency;
BOOL fExit,debug=0,doppiaDim;
HWND ghWnd,hStatusWnd;
HBRUSH hBrush;
HPEN hPen1;
HFONT hFont,hFont2;
#ifdef SKYNET 
COLORREF Colori[2]={		// 
	RGB(0,0,0),						 // nero 
	RGB(0xff,0xff,0xff),	 // bianco
	};
#elif NEZ80 
COLORREF Colori[2]={		// 
	RGB(0,0,0),						 // nero 
	RGB(0xff,0x00,0x00),	 // rosso
	};
#elif GALAKSIJA
COLORREF Colori[2]={		// 
	RGB(0,0,0),						 // nero 
	RGB(0xff,0xff,0xff),	 // bianco
	};
#elif ZX80
COLORREF Colori[2]={		// 
	RGB(0,0,0),						 // nero 
	RGB(0xff,0xff,0xff),	 // bianco
	};
#elif ZX81
COLORREF Colori[2]={		// 
	RGB(0,0,0),						 // nero 
	RGB(0xff,0xff,0xff),	 // bianco
	};
#elif MSX
COLORREF Colori[16]={		// questi per MSX/TMS...
	RGB(0,0,0),						 // nero (trasparente
	RGB(0,0,0),						 // nero
	RGB(0x00,0xc0,0x00),	 // verde medio
	RGB(0x80,0xff,0x80),	 // verde chiaro

	RGB(0x00,0x00,0x80),	 // blu scuro
	RGB(0x00,0x00,0xff),	 // blu
	RGB(0x80,0x00,0x00),	 // rosso scuro
	RGB(0x00,0xff,0xff),	 // azzurro/cyan

	RGB(0xc0,0x00,0x00),	 // rosso medio
	RGB(0xff,0x00,0x00),	 // rosso chiaro
	RGB(0x80,0x80,0x00),	 // giallo scuro
	RGB(0xff,0xff,0x00),	 // giallo
	
	RGB(0x00,0x80,0x00),	 // verde
	RGB(0xff,0x00,0xff),	 // porpora/magenta
	RGB(0x80,0x80,0x80),	 // grigio medio
	RGB(0xff,0xff,0xff),	 // bianco
	};
#endif
UINT hTimer;
extern BOOL ColdReset;
extern BYTE CPUPins;
extern WORD CPUDivider;
extern BYTE rom_seg[ROM_SIZE],ram_seg[RAM_SIZE];
#ifdef ROM_SIZE2
extern BYTE rom_seg2[ROM_SIZE2];
#endif
extern SWORD VICRaster;
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
extern BYTE Keyboard[11];
extern WORD TIMEr;
BYTE TMSVideoRAM[TMSVIDEORAM_SIZE];
#endif
BYTE VideoRAM[VIDEORAM_SIZE];
extern HFILE spoolFile;
BITMAPINFO *bmI;


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg;

	if(!hPrevInstance) {
		if(!InitApplication(hInstance)) {
			return (FALSE);
		  }
	  }

	if (!InitInstance(hInstance, nCmdShow)) {
		return (FALSE);
  	}

	if(*lpCmdLine) {
		PostMessage(ghWnd,WM_USER+1,0,(LPARAM)lpCmdLine);
		}

	hAccelTable = LoadAccelerators (hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
	Emulate(0);

  return msg.wParam;
	}


#ifdef NEZ80
#define DIGIT_X_SIZE (rc.right/12)
#define DIGIT_X_SPACE (DIGIT_X_SIZE/3)
#define DIGIT_Y_SIZE (rc.bottom/4)
#define DIGIT_OBLIQ (rc.right/50)
#define BLACK RGB(0,0,0)
//#define SEGMENT_BLANK (rc.right/60)
#define SEGMENT_BLANK (8)

int drawLine(HDC hDC,WORD x1,WORD y1,WORD x2,WORD y2,COLORREF color) {
	HPEN Pen;
	HBRUSH Brush;
	int segmentBlank;

	Pen=CreatePen(PS_SOLID,6,color);
	SelectObject(hDC,Pen);
	if(y1==y2) {
		segmentBlank=((x2-x1)/SEGMENT_BLANK);
		MoveToEx(hDC,x1+segmentBlank,y1,NULL);
		LineTo(hDC,x2-segmentBlank,y2);
		}
	else {
		segmentBlank=((y2-y1)/SEGMENT_BLANK);
		MoveToEx(hDC,x1,y1+segmentBlank,NULL);
		LineTo(hDC,x2,y2-segmentBlank);
		}
	DeleteObject(Pen);
	}
int drawCircle(HDC hDC,WORD x,WORD y,WORD d,COLORREF color) {
	HPEN Pen;

	Pen=CreatePen(PS_SOLID,6,color);
	SelectObject(hDC,Pen);
	Ellipse(hDC,x-d,y-d,x+d,y+d);
	DeleteObject(Pen);
	}
int PlotDisplay(WORD pos,BYTE ch,BYTE c) {
	register int i;
  int x,y;
  COLORREF color;
	HDC hDC=GetDC(ghWnd);
	RECT rc;

	GetClientRect(ghWnd,&rc);

  x=rc.right/14;
  y=rc.bottom/4;
  x+=(DIGIT_X_SIZE+DIGIT_X_SPACE)*pos;
//	fillRect(x,y,DIGIT_X_SIZE+3,DIGIT_Y_SIZE+1,BLACK);
  
  if(c)
    color=RGB(255,0,0);
  else
    color=RGB(128,0,0);
  if(!(ch & 1)) {		//A
    drawLine(hDC,x+DIGIT_OBLIQ,y,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y,color);
    }
  else {
    drawLine(hDC,x+DIGIT_OBLIQ,y,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y,BLACK);
    }
  if(!(ch & 2)) {
    drawLine(hDC,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,color);
    }
  else {
    drawLine(hDC,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
    }
  if(!(ch & 4)) {
    drawLine(hDC,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,color);
    }
  else {
    drawLine(hDC,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,BLACK);
    }
  if(!(ch & 8)) {
    drawLine(hDC,x,y+DIGIT_Y_SIZE,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,color);
    }
  else {
    drawLine(hDC,x,y+DIGIT_Y_SIZE,x+DIGIT_X_SIZE,y+DIGIT_Y_SIZE,BLACK);
    }
  if(!(ch & 16)) {
    drawLine(hDC,x+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x,y+DIGIT_Y_SIZE,color);
    }
  else {
    drawLine(hDC,x+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x,y+DIGIT_Y_SIZE,BLACK);
    }
  if(!(ch & 32)) {
    drawLine(hDC,x+DIGIT_OBLIQ,y,x+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,color);
    }
  else {
    drawLine(hDC,x+DIGIT_OBLIQ,y,x+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
    }
  if(!(ch & 64)) {		//G
    drawLine(hDC,x+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,color);
    }
  else {
    drawLine(hDC,x+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
    }
// più bello..?    drawLine(x+2+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,x-1+DIGIT_X_SIZE+DIGIT_OBLIQ/2,y+DIGIT_Y_SIZE/2,BLACK);
  if(!(ch & 128)) {
    drawCircle(hDC,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y+DIGIT_Y_SIZE,1,color);    // 
    }
  else {
    drawCircle(hDC,x+DIGIT_X_SIZE+DIGIT_OBLIQ,y+DIGIT_Y_SIZE,1,BLACK);
		}
  ReleaseDC(ghWnd,hDC);
	}

int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin) {
	int i;

	for(i=0; i<8; i++)
		PlotDisplay(7-i,DisplayRAM[i],1);
	}

#endif

#ifdef SKYNET
int drawPixel(HDC hDC,WORD x,WORD y,COLORREF color) {
	HPEN Pen;
	HBRUSH Brush;

	Pen=CreatePen(PS_SOLID,4,color);
	SelectObject(hDC,Pen);
	SetPixel(hDC,x,y,color);
	DeleteObject(Pen);
	}
int fillCircle(HDC hDC,WORD x,WORD y,WORD d,COLORREF color) {
	HPEN Pen;
	HBRUSH Brush;

	Pen=CreatePen(PS_SOLID,4,color);
	Brush=CreateSolidBrush(color);
	SelectObject(hDC,Pen);
	Ellipse(hDC,x-d,y-d,x+d,y+d);
	DeleteObject(Brush);
	DeleteObject(Pen);
	}
int drawRect(HDC hDC,WORD x1,WORD y1,WORD x2,WORD y2,COLORREF color) {
	HPEN Pen;
	HBRUSH Brush;

	Pen=CreatePen(PS_SOLID,4,color);
	Brush=CreateSolidBrush(color);
	SelectObject(hDC,Pen);
	Rectangle(hDC,x1,y1,x2,y2);
	DeleteObject(Brush);
	DeleteObject(Pen);
	}
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin, WORD c) {
	int k,i1,y1,y2,x1,x2;
	register BYTE *p,*p1;
  BYTE chl,chh;
	char myBuf[128];
	UINT16 px,py;
  int x,y;
  COLORREF color;
	UINT8 i,j,lcdMax;
	BYTE *fontPtr,*lcdPtr;
  static BYTE cursorState=0,cursorDivider=0;
	RECT rc;

#define LCD_MAX_X 20
#define LCD_MAX_Y 4
#define DIGIT_X_SIZE (rc.right/12)
#define DIGIT_X_SPACE (DIGIT_X_SIZE/3)
#define DIGIT_Y_SIZE (rc.bottom/4)
#define BLACK RGB(0,0,0)
  
  
//  i8255RegR[0] |= 0x80; mah... fare?? v. di là
	GetClientRect(ghWnd,&rc);

  y=(rc.bottom-(LCD_MAX_Y*DIGIT_Y_SIZE))/2 +20;
  
//	fillRect(x,y,DIGIT_X_SIZE+3,DIGIT_Y_SIZE+1,BLACK);
	drawRect(hDC,(rc.right-(LCD_MAX_X*DIGIT_X_SIZE))/2-1,(rc.bottom-(LCD_MAX_Y*DIGIT_Y_SIZE))/2 +16,
          DIGIT_X_SIZE*20+4,DIGIT_Y_SIZE*4+7,RGB(192,192,192));
  
  if(c)
    color=RGB(0xff,0xff,0xff);
  else
    color=RGB(192,192,192);

  
//        LCDdisplay=7; //test cursore

  cursorDivider++;
  if(cursorDivider>=11) {
    cursorDivider=0;
    cursorState=!cursorState;
    }
          
        
  if(LCDdisplay & 4) {

  lcdMax=LCDfunction & 8 ? LCD_MAX_Y : LCD_MAX_Y/2;
  for(y1=0; y1<LCD_MAX_Y; y1++) {
    x=(rc.right-(LCD_MAX_X*DIGIT_X_SIZE))/2;
    
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
            drawPixel(hDC,x+x1*6+j, y+7, color);
            }
          }
        if((LCDdisplay & 1) && cursorState) {
          int k=LCDdisplay & 2 ? 7 : 8;

          for(i=0; i<k; i++) {    //

            if(LCDfunction & 4)   // font type...
              ;

            for(j=6; j>1; j--) {    //+ piccolo..
              drawPixel(hDC,x+x1*6+j, y+i, color);
              }
            }
          }
        goto skippa;
        }
      
      fontPtr=fontLCD_eu+((UINT16)*lcdPtr)*10;
      for(i=0; i<8; i++) {
        UINT8 line;

        line = *(fontPtr+i);

        if(LCDfunction & 4)   // font type...
          ;
        
        for(j=6; j>0; j--, line >>= 1) {
          if(line & 0x1)
            drawPixel(hDC,x+x1*6+j, y+i, color);
          else
            drawPixel(hDC,x+x1*6+j, y+i, BLACK );
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

  
	drawRect(hDC,4,41,rc.right-9,13,RGB(255,128,0));
  for(i=0,j=1; i<8; i++,j<<=1) {
    if(IOExtPortO[0] & j)
      fillCircle(hDC,(rc.right/5)+i*(rc.right/24),rc.bottom/4,3,RGB(255,0,0));
    else
      fillCircle(hDC,(rc.right/5)+i*(rc.right/24),rc.bottom/4,3,RGB(112,96,96));
    }
  for(i=8,j=1; i<16; i++,j<<=1) {
    if(IOExtPortO[1] & j)
      fillCircle(hDC,(rc.right/5)+i*(rc.right/24),rc.bottom/4,3,RGB(255,0,0));
    else
      fillCircle(hDC,(rc.right/5)+i*(rc.right/24),rc.bottom/4,3,RGB(112,96,96));
    }
  for(i=0,j=1; i<8; i++,j<<=1) {
    if(IOPortO /*ram_seg[0] /*MemPort1 = 0x8000 */ & j)
      fillCircle(hDC,(rc.right/5)+i*(rc.right/24),rc.bottom/5,2,RGB(255,0,0));
    else
      fillCircle(hDC,(rc.right/5)+i*(rc.right/24),rc.bottom/5,2,RGB(112,96,96));
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

int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin, BYTE _i) {
	register int i,j;
	int k,y1,y2,x2,row1,row2,curvideopos,oldvideopos,usedpos;
	register BYTE *p,*p1;
  SWORD D_FILE;
  BYTE ch,invert;
	HBITMAP hBitmap,hOldBitmap;
	HDC hCompDC;
	UINT16 px,py;
	BYTE *pVideoRAM;
  int x,y,x1;

  row1=rowIni;
  row2=rowFin;
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
  x2=HORIZ_SIZE/8;
//  y2=192/8  +1  ;    // voglio/devo visualizzare le righe inferiori...
  if(rowIni>=VERT_SIZE)
    rowIni=VERT_SIZE;
  if(rowFin>=VERT_SIZE)
    rowFin=VERT_SIZE;

  D_FILE=MAKEWORD(ram_seg[0x000c],ram_seg[0x000d]);     // 400Ch
  if(D_FILE<0x4000)
    return;
  curvideopos=oldvideopos=0;
  for(i=y1; i<y2; i++) {
    curvideopos=oldvideopos;

    for(k=0; k<8; k++) {      // scanline del carattere da plottare
			pVideoRAM=(BYTE*)&VideoRAM[0]+((i*8)+k)*(((HORIZ_SIZE/8)+(HORIZ_OFFSCREEN*2)));

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

        if(i>=1 && j<x2) {
          if(invert) {
						*pVideoRAM++=ch;
            }
          else {
						*pVideoRAM++=~ch;
            }
         }
    
        }

      curvideopos=oldvideopos;
      }
    oldvideopos=curvideopos+usedpos+1;
    }
  
	i=StretchDIBits(hDC,0,(rowIni*AppYSizeR)/(VERT_SIZE),doppiaDim ? AppXSize*2 : AppXSize,
		doppiaDim ? ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE/2) : ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE),
		0,rowFin,HORIZ_SIZE+HORIZ_OFFSCREEN*2,rowIni-rowFin,
		VideoRAM,bmI,DIB_RGB_COLORS,SRCCOPY);
    
	}
#endif
#ifdef ZX81
//https://problemkaputt.de/zxdocs.htm#zx80zx81videomodetextandblockgraphics
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin, BYTE _i) {
	register int i,j;
	int k,y1,y2,x2,row1,row2,curvideopos,oldvideopos,usedpos;
	register BYTE *p,*p1;
  SWORD D_FILE;
  BYTE ch,invert;
	char myBuf[128];
	HBITMAP hBitmap,hOldBitmap;
	HDC hCompDC;
	UINT16 px,py;
	BYTE *pVideoRAM;
  int x,y,x1;


  row1=rowIni;
  row2=rowFin;
  y1=row1/8;
  y2=row2/8;
  y2=min(y2,VERT_SIZE/8);
  x2=HORIZ_SIZE/8;
  y2=192/8  +1  ;    // voglio/devo visualizzare le righe inferiori...
  if(rowIni>=VERT_SIZE)
    rowIni=VERT_SIZE;
  if(rowFin>=VERT_SIZE)
    rowFin=VERT_SIZE;
  D_FILE=MAKEWORD(ram_seg[0x000c],ram_seg[0x000d]);     // 400Ch
  if(D_FILE<0x4000)
    return;
  curvideopos=oldvideopos=0;
  for(i=y1; i<y2; i++) {
    curvideopos=oldvideopos;
    for(k=0; k<8; k++) {      // scanline del carattere da plottare
			pVideoRAM=(BYTE*)&VideoRAM[0]+((i*8)+k)*(((HORIZ_SIZE/8)+(HORIZ_OFFSCREEN*2)));
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

        if(i>=9 && j<x2) {
          if(invert) {
						*pVideoRAM++=ch;
            }
          else {
						*pVideoRAM++=~ch;
            }
          }
        }

      curvideopos=oldvideopos;
      }
    oldvideopos=curvideopos+usedpos+1;
    }
  
	i=StretchDIBits(hDC,0,(rowIni*AppYSizeR)/(VERT_SIZE),doppiaDim ? AppXSize*2 : AppXSize,
		doppiaDim ? ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE/2) : ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE),
		0,rowFin,HORIZ_SIZE+HORIZ_OFFSCREEN*2,rowIni-rowFin,
		VideoRAM,bmI,DIB_RGB_COLORS,SRCCOPY);
  
	}
#endif
#ifdef ZXSPECTRUM
//    6144 bytes worth of bitmap data, starting at memory address &4000
//    768 byte colour attribute data, immediately after the bitmap data at address &5800
//http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout
#endif

#ifdef GALAKSIJA
extern const unsigned char GALAKSIJA_CHARROM[0x800];
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin) {
	register int i,j;
	int k,y1,y2,x1,x2,row1,row2;
	register BYTE *p,*p1;
  BYTE ch;
	HBITMAP hBitmap,hOldBitmap;
	HDC hCompDC;
	UINT16 px,py;
	BYTE *pVideoRAM;
  int x,y;
  BYTE ch2;

  // ci mette circa 25mS ogni passata... [[dovrebbe essere la metà... (viene chiamata circa 25-30 volte al secondo e ora siamo a 40mS invece di 20, 16/10/22)

  row1=rowIni;
  row2=rowFin;
  y1=row1/13;
  y2=row2/13;
  y2=min(y2,VERT_SIZE/13);
  x1=0;
  x2=HORIZ_SIZE/8;
  for(i=y1; i<y2; i++) {
    for(k=0; k<13; k++) {      // scanline del carattere da plottare
			pVideoRAM=(BYTE*)&VideoRAM[0]+((i*13)+k)*(((HORIZ_SIZE/8)+(HORIZ_OFFSCREEN*2)));
      p1=((BYTE*)&ram_seg[0x0000])+i*(HORIZ_SIZE/8);    // carattere a inizio riga
      for(j=x1; j<x2; j++) {
        ch=*p1 & 0x3f;      // v. pdf slide 2250_presentacija
        if(*p1 & 0x80)
          ch |= 0x40;
        p=((BYTE*)&GALAKSIJA_CHARROM)+(ch)+k*128;    // pattern del carattere da disegnare
        ch=*p;

				ch2 = ch & 1 ? 0x80 : 0;
				ch2 |= ch & 2 ? 0x40 : 0;
				ch2 |= ch & 4 ? 0x20 : 0;
				ch2 |= ch & 8 ? 0x10 : 0;
				ch2 |= ch & 16 ? 0x08 : 0;
				ch2 |= ch & 32 ? 0x04 : 0;
				ch2 |= ch & 64 ? 0x02 : 0;
				ch2 |= ch & 128 ? 0x01 : 0;

				*pVideoRAM++=~ch2;

        p1++;
        }
      }
    }

	i=StretchDIBits(hDC,0,(rowIni*AppYSizeR)/(VERT_SIZE),doppiaDim ? AppXSize*2 : AppXSize,
		doppiaDim ? ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE/2) : ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE),
		0,rowFin,HORIZ_SIZE+HORIZ_OFFSCREEN*2,rowIni-rowFin,
		VideoRAM,bmI,DIB_RGB_COLORS,SRCCOPY);
	}
#endif
#ifdef MSX
//https://www.angelfire.com/art2/unicorndreams/msx/RR-VDP.html#VDP-StatusReg
const WORD graphColors[16]={0/*transparent*/,1,2,3, 4,5,6,7,
  8,9,10,11, 12,13,14,15
	};
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin) {
	register int i;
	UINT16 px,py;
	int row1;
	int x;
	register BYTE *p1,*p2;
  BYTE ch1,ch2,color,color1,color0;
	BYTE *pVideoRAM;

  BYTE videoMode=((TMS9918Reg[1] & 0x18) >> 2) | ((TMS9918Reg[0] & 2) >> 1);
  WORD charAddress=((WORD)(TMS9918Reg[4] & 7)) << 11;
  WORD videoAddress=((WORD)(TMS9918Reg[2] & 0xf)) << 10;
  WORD colorAddress=((WORD)(TMS9918Reg[3])) << 6;
  WORD spriteAttrAddress=((WORD)(TMS9918Reg[5] & 0x7f)) << 7;
  WORD spritePatternAddress=((WORD)(TMS9918Reg[6] & 0x7)) << 11;

  if(!rowIni) {   // 
//    setAddrWindow(0,0,_width,bordery/2);
    for(py=0; py<VERT_OFFSCREEN; py++) {
			pVideoRAM=(BYTE*)&VideoRAM[0]+py*(((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2));
      for(px=0; px<((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2); px++)
				*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
      }
    }
  
  if(rowIni>=VERT_OFFSCREEN && rowIni<VERT_SIZE+VERT_OFFSCREEN) {
		if(!(TMS9918Reg[1] & B8(01000000))) {    // blanked
			for(py=rowIni; py<rowFin; py++) {   // 
				pVideoRAM=(BYTE*)&VideoRAM[0]+py*(((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2));
				for(px=0; px<((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2); px++)
	//        writedata16(graphColors[TMS9918Reg[7] & 0xf]);
					*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
				}
			}
		else switch(videoMode) {
			case 0:     // graphics 1
				for(py=rowIni/8; py<rowFin/8; py++) {    // 192 
					p2=((BYTE*)&TMSVideoRAM[videoAddress]) + ((py-VERT_OFFSCREEN/8)*HORIZ_SIZE/8);
					for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
						pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*((((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2)));
						for(px=0; px<HORIZ_OFFSCREEN/2; px++)
							*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
						p1=p2;
						for(px=0; px<HORIZ_SIZE/8; px++) {    // 256 pixel 
							ch1=*p1++;
							ch2=TMSVideoRAM[charAddress + (ch1*8) + (row1)];
							color=TMSVideoRAM[colorAddress+ch1/8];
							color1=color >> 4; color0=color & 0xf;
							if(!color0)
								color0=TMS9918Reg[7] & 0xf;
							if(!color1)
								color1=TMS9918Reg[7] & 0xf;

							*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
							}
						for(px=0; px<HORIZ_OFFSCREEN/2; px++)
							*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
						}
					}

handle_sprites:
				{ // (OVVIAMENTE sarebbe meglio gestirli riga per riga...!
				struct SPRITE_ATTR *sa;
				BYTE ssize=TMS9918Reg[1] & 2 ? 32 : 8,smag=TMS9918Reg[1] & 1 ? 16 : 8;
				BYTE j2;
				int8_t j,smax;
				uint16_t sofs;

				for(smax=0; smax<32; smax++) {			// screen order inverso per cui mi serve sapere da dove iniziare
					sa=((struct SPRITE_ATTR *)&TMSVideoRAM[spriteAttrAddress+smax*sizeof(struct SPRITE_ATTR)]);
					if(sa->ypos>=LAST_SPRITE_YPOS)
						break;
					}
				smax--;

				for(i=smax; i>=0; i--) {			// screen order inverso!
					struct SPRITE_ATTR *sa2;
        
					sa=((struct SPRITE_ATTR *)&TMSVideoRAM[spriteAttrAddress+i*sizeof(struct SPRITE_ATTR)]);
					j2=smag*(ssize==32 ? 2 : 1);
					for(j=smax-1; j>=0; j--) {
						sa2=((struct SPRITE_ATTR *)&TMSVideoRAM[spriteAttrAddress+j*sizeof(struct SPRITE_ATTR)]);
						if((sa2->ypos>=sa->ypos && sa2->ypos<=sa->ypos+j2) &&
							(sa2->xpos>=sa->xpos && sa2->xpos<=sa->xpos+j2)) {
							// controllare solo i pixel accesi, a 1!
							TMS9918RegS |= B8(00100000);
							// poi ci sarebbe il flag 5 sprite per riga!
							}
						sa2++;
						}
        
#if 0
					TMSVideoRAM[spritePatternAddress+i*8]=rand();TMSVideoRAM[spritePatternAddress+i*8+3]=58;
//					TMSVideoRAM[spritePatternAddress+8]=16;TMSVideoRAM[spritePatternAddress+12]=40;
//					TMSVideoRAM[spritePatternAddress+16]=25;TMSVideoRAM[spritePatternAddress+21]=40;
					sa->color=i;
sa->name=i;//TEST!
sa->xpos=timeGetTime()/8 /*40+i*4*/; sa->ypos=5+i*10;
#endif

					if(ssize==8)
						p1=((BYTE*)&TMSVideoRAM[spritePatternAddress]) + (((WORD)sa->name)*8); // (((WORD)sa->name)*ssize);
					else
						p1=((BYTE*)&TMSVideoRAM[spritePatternAddress]) + ((((WORD)sa->name) & 0xfc)*8);
					j=ssize;
					if(sa->ypos > 0xe1)     // Y diventa negativo..
						;
	//        setAddrWindow(sa->xpos/2,sa->ypos/2,8/2,8/2);
					color1=sa->color; color0=TMS9918Reg[7] & 0xf;

					
						if(sa->name==0x88 && sa->ypos>rowIni && sa->ypos<rowFin)			// debug
							x=1;

					if(sa->color==0)
						goto skippa_sprite;

					x=sa->eclock ? -32 : 0;     // X diventa negativo..
					x+=sa->xpos;
					sofs=0;
					for(py=0; py<ssize; py++,j--) {
						BYTE oldpixel;
						int y;

						if(ssize>8)
							y=(py & 15)+sa->ypos;
						else
							y=py+sa->ypos;
//						if(y+VERT_OFFSCREEN<rowIni || y+VERT_OFFSCREEN>rowFin)		non va così.. sistemare per velocizzare!
//							continue;
						pVideoRAM=(BYTE*)&VideoRAM[VERT_OFFSCREEN*(HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2+HORIZ_OFFSCREEN/2]+
							(y)*(((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2))+sofs+x/2;
						ch1=*p1++;

						if(x & 1)// FORSE bisognerebbe gestire posizione dispari...
							;
						if(smag==16) {
	/*            writedata16x2(graphColors[ch2 & B8(10000000 ? color1 : color0],graphColors[ch2 & B8(10000000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(1000000 ? color1 : color0],graphColors[ch2 & B8(1000000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(100000 ? color1 : color0],graphColors[ch2 & B8(100000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(10000 ? color1 : color0],graphColors[ch2 & B8(10000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(1000 ? color1 : color0],graphColors[ch2 & B8(1000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(100 ? color1 : color0],graphColors[ch2 & B8(100 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(10 ? color1 : color0],graphColors[ch2 & B8(10 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(1 ? color1 : color0],graphColors[ch2 & B8(1 ? color1 : color0]);*/
							oldpixel=*pVideoRAM;
							if(ch1 & B8(10000000))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(01000000))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00100000))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00010000))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00001000))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00000100))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00000010))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00000001))
								oldpixel = (color1) | (color1 << 4);
							*pVideoRAM=oldpixel;
							}
						else {
	/*            writedata16(graphColors[ch2 & B8(10000000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(1000000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(100000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(10000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(1000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(100 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(10 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(1 ? color1 : color0]); */
							oldpixel=*pVideoRAM;
							if(ch1 & B8(10000000))
								oldpixel = (oldpixel & 0x0f) | (color1 << 4);
							if(ch1 & B8(01000000))
								oldpixel = (oldpixel & 0xf0) | (color1);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00100000))
								oldpixel = (oldpixel & 0x0f) | (color1 << 4);
							if(ch1 & B8(00010000))
								oldpixel = (oldpixel & 0xf0) | (color1);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00001000))
								oldpixel = (oldpixel & 0x0f) | (color1 << 4);
							if(ch1 & B8(00000100))
								oldpixel = (oldpixel & 0xf0) | (color1);
							*pVideoRAM++=oldpixel;
							oldpixel=*pVideoRAM;
							if(ch1 & B8(00000010))
								oldpixel = (oldpixel & 0x0f) | (color1 << 4);
							if(ch1 & B8(00000001))
								oldpixel = (oldpixel & 0xf0) | (color1);
							*pVideoRAM=oldpixel;
							}

						switch(j) {   // gestisco i "quadranti" sprite messi a cazzo...
							case 23:
	//              setAddrWindow(sa->xpos/2,sa->ypos/2+8/2,8/2,8/2);
								sofs=0;
								break;
							case 15:
//								p1=((BYTE*)&TMSVideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize) + (16*ssize)/2;
	//              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2,8/2,8/2);
								sofs=0+smag/2;
								if(x>239)		// migliorare bordo dx
									goto skippa_sprite;
								break;
							case 7:
	//              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2+8/2,8/2,8/2);
//								pVideoRAM=(BYTE*)&VideoRAM[0]+(py+sa->ypos+8)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)))+sa->xpos+(8*ssize)/2;
								sofs=0+smag/2;
								break;
							default:
								break;
							}
						}

skippa_sprite:
						;
					}
				}
				break;
			case 1:     // graphics 2
				for(py=rowIni; py<rowFin/3; py++) {    // 192 linee 
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + ((py-VERT_OFFSCREEN/8)*HORIZ_SIZE/8);
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8))*((((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2)));
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					for(px=0; px<HORIZ_SIZE/8; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=TMSVideoRAM[charAddress + (ch1*8) + (py & 7)];
						color=TMSVideoRAM[colorAddress+py];
						color1=color >> 4; color0=color & 0xf;
						if(!color0)
							color0=TMS9918Reg[7] & 0xf;
						if(!color1)
							color1=TMS9918Reg[7] & 0xf;

	/*          writedata16x2(graphColors[ch2 & B8(10000000 ? color1 : color0],
										graphColors[ch2 & B8(01000000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & B8(00100000 ? color1 : color0],
										graphColors[ch2 & B8(00010000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & B8(00001000 ? color1 : color0],
										graphColors[ch2 & B8(00000100 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & B8(00000010 ? color1 : color0],
										graphColors[ch2 & B8(00000001 ? color1 : color0]);*/
						*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
						}
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					}
				for(py=rowFin/3; py<(rowFin*2)/3; py++) {    //
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8))*((((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2)));
						for(px=0; px<HORIZ_OFFSCREEN/2; px++)
							*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=TMSVideoRAM[charAddress +2048 + (ch1*8) + (py & 7)];
						color=TMSVideoRAM[colorAddress+2048+py];
						color1=color >> 4; color0=color & 0xf;

						*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
						}
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					}
				for(py=(rowFin*2)/3; py<rowFin; py++) {    // 
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8))*((((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2)));
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					for(px=0; px<HORIZ_SIZE/8; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=TMSVideoRAM[charAddress +4096 + (ch1*8) + (py & 7)];
						color=TMSVideoRAM[colorAddress+4096+py];
						color1=color >> 4; color0=color & 0xf;

						*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
						}
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					}
				goto handle_sprites;
				break;
			case 2:     // multicolor
				for(py=rowIni; py<rowFin; py+=4) {    // 48 linee diventano 96
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8))*((((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2)));
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*16);
					ch1=*p1++;
					p2=((BYTE*)&TMSVideoRAM[charAddress+ch1]);
					for(px=0; px<HORIZ_SIZE; px+=2) {    // 64 pixel diventano 128
						ch2=*p2++;
						color=TMSVideoRAM[colorAddress+ch2];
						color1=color >> 4; color0=color & 0xf;
						if(!color0)
							color0=TMS9918Reg[7] & 0xf;
						if(!color1)
							color1=TMS9918Reg[7] & 0xf;
          
						// finire!!

	//          writedata16x2(graphColors[color1],graphColors[color0]);
						}
					for(px=0; px<HORIZ_OFFSCREEN/2; px++)
						*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
					}
				goto handle_sprites;
				break;
			case 4:     // text 32x24, ~120mS, O1 opp O2, 2/7/24
				color1=TMS9918Reg[7] >> 4; color0=TMS9918Reg[7] & 0xf;
				for(py=rowIni/8; py<rowFin/8; py++) {    // 192 linee(32 righe char) 
					p2=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*40);
					// mettere bordo
					for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
						pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*((((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2)));
						for(px=0; px<HORIZ_OFFSCREEN/2; px++)
							*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
						p1=p2;
						for(px=0; px<40; px++) {    // 240 pixel 
							ch1=*p1++;
							ch2=TMSVideoRAM[charAddress + (ch1*8) + (row1)];

							*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
							}
						for(px=0; px<HORIZ_OFFSCREEN/2; px++)
							*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
						}
					}
				break;
			}

		if(rowFin>=MAX_RASTER-VERT_OFFSCREEN) {
			for(py=VERT_SIZE+VERT_OFFSCREEN; py<VERT_SIZE+VERT_OFFSCREEN*2; py++) {
				pVideoRAM=(BYTE*)&VideoRAM[0]+py*(((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2));
				for(px=0; px<((HORIZ_SIZE+HORIZ_OFFSCREEN*2)/2); px++)
					*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
				}
			/*{
			HFILE file=_lcreat("videoram.bin",0);
			_lwrite(file,VideoRAM,sizeof(VideoRAM));
			_lclose(file);
			}*/
			}
		}

	i=StretchDIBits(hDC,0,(rowIni*AppYSizeR)/(VERT_SIZE+VERT_OFFSCREEN*2),doppiaDim ? AppXSize*2 : AppXSize,
		doppiaDim ? ((rowFin-rowIni)*AppYSizeR)/((VERT_SIZE+VERT_OFFSCREEN*2)/2) : ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE+VERT_OFFSCREEN*2),
		0,rowFin,HORIZ_SIZE+HORIZ_OFFSCREEN*2,rowIni-rowFin,
		VideoRAM,bmI,DIB_RGB_COLORS,SRCCOPY);

  }
#endif





VOID CALLBACK myTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime) {
	int i;
	HDC hDC;
  static BYTE divider,divider2;
  static BYTE dividerTim=0;

  divider++;
//  if(divider>=32) {   // 50 Hz per TOD  qua siamo a 50 Hz :)
//    divider=0;
#ifdef SKYNET
      TIMIRQ=1;
#endif
#ifdef NEZ80
      TIMIRQ=1;
#endif
#ifdef ZX80
      TIMIRQ=1;
#endif
#ifdef ZX81
      TIMIRQ=1;
#endif
#ifdef GALAKSIJA
      TIMIRQ=1;
#endif
#ifdef MSX
    i8253RegR[0]++;
    if(!i8253RegR[0] && !(i8251RegW[2] & B8(00001000)))
      //VERIFICARE frequenza e se c'è!
      TIMIRQ=1;     // ??Hz
#endif
//    }

#ifdef NEZ80
// metto in loop		MUXcnt++;		// sarebbe 1KHz in effetti...
//		MUXcnt &= 0xf;
#endif

  if(divider>=50) {   // ergo 1Hz
    divider=0;
#ifdef SKYNET
    // vedere registro 0A, che ha i divisori...
    // i146818RAM[10] & 15
    dividerTim=0;
    if(!(i146818RAM[11] & 0x80)) {    // SET
      i146818RAM[10] |= 0x80;
/*      currentTime.sec++;
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
        } */
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
#ifdef NEZ80
#endif
#ifdef ZX80
#endif
#ifdef ZX81
#endif
#ifdef GALAKSIJA
#endif
#ifdef MSX
#endif
#ifdef MSXTURBO
//This timer is present only on the MSX turbo R      
		TIMEr++;     // 3.911uS
#endif
  
		}

	}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId,wmEvent;
	PAINTSTRUCT ps;
	HDC hDC,hCompDC;
 	POINT pnt;
	HMENU hMenu;
	HRSRC hrsrc;
	HFILE myFile;
 	BOOL bGotHelp;
	int i,j,k,i1,j1,k1;
	long l;
	char myBuf[128];
	char *s,*s1;
	LOGBRUSH br;
	RECT rc;
	SIZE mySize;
	HFONT hOldFont;
	HPEN myPen,hOldPen;
	HBRUSH myBrush,hOldBrush;
	static int TimerCnt;
	HANDLE hROM;

	switch(message) { 
		case WM_COMMAND:
			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId) {
				case ID_APP_ABOUT:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC)About);
					break;

				case ID_OPZIONI_DUMP:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_DUMP), hWnd, (DLGPROC)Dump0);
					break;

				case ID_OPZIONI_DUMPDISP:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_DUMP1), hWnd, (DLGPROC)Dump1);
					break;

				case ID_OPZIONI_RESET:
   				CPUPins |= DoReset;
					InvalidateRect(hWnd,NULL,TRUE);
  				SetWindowText(hStatusWnd,"<reset>");
					break;

				case ID_EMULATORE_CARTRIDGE:
					break;

				case ID_APP_EXIT:
					PostMessage(hWnd,WM_CLOSE,0,0l);
					break;

				case ID_FILE_NEW:
					break;

				case ID_FILE_OPEN:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binari\0*.BIN\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if(GetOpenFileName(&ofn) == TRUE) {
						myFile=_lopen(ofn.lpstrFile /*"z80memory.bin"*/,OF_READ);
						if(myFile != -1) {
							_lread(myFile,ram_seg,RAM_SIZE);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}
					}
					break;

				case ID_FILE_SAVE_AS:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binari\0*.BIN\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST;

					if(GetSaveFileName(&ofn) == TRUE) {
							// use ofn.lpstrFile
						myFile=_lcreat(ofn.lpstrFile /*"z80memory.bin"*/,0);
						if(myFile != -1) {
							_lwrite(myFile,ram_seg,RAM_SIZE);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}

					}
					break;

				case ID_OPZIONI_DEBUG:
					debug=!debug;
					break;

				case ID_OPZIONI_TRACE:
//					_f.SR.Trace =!_f.SR.Trace ;
					break;

				case ID_OPZIONI_DIMENSIONEDOPPIA:
					doppiaDim=!doppiaDim;
					break;

				case ID_OPZIONI_AGGIORNA:
					InvalidateRect(hWnd,NULL,TRUE);
					break;

				case ID_OPZIONI_VELOCIT_LENTO:
					CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER/2);
					break;
				case ID_OPZIONI_VELOCIT_NORMALE:
					CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER);
					break;
				case ID_OPZIONI_VELOCIT_VELOCE:
					CPUDivider=pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER*2);
					break;

				case ID_EDIT_PASTE:
					break;

        case ID_HELP: // Only called in Windows 95
          bGotHelp = WinHelp(hWnd, APPNAME".HLP", HELP_FINDER,(DWORD)0);
          if(!bGotHelp) {
            MessageBox(GetFocus(),"Unable to activate help",
              szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_INDEX: // Not called in Windows 95
          bGotHelp = WinHelp(hWnd, APPNAME".HLP", HELP_CONTENTS,(DWORD)0);
		      if(!bGotHelp) {
            MessageBox(GetFocus(),"Unable to activate help",
              szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_FINDER: // Not called in Windows 95
          if(!WinHelp(hWnd, APPNAME".HLP", HELP_PARTIALKEY,
				 		(DWORD)(LPSTR)"")) {
						MessageBox(GetFocus(),"Unable to activate help",
							szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_USING: // Not called in Windows 95
					if(!WinHelp(hWnd, (LPSTR)NULL, HELP_HELPONHELP, 0)) {
						MessageBox(GetFocus(),"Unable to activate help",
							szAppName, MB_OK|MB_ICONHAND);
					  }
					break;

				default:
					return (DefWindowProc(hWnd, message, wParam, lParam));
			}
			break;

		case WM_NCRBUTTONUP: // RightClick on windows non-client area...
			if (IS_WIN95 && SendMessage(hWnd, WM_NCHITTEST, 0, lParam) == HTSYSMENU) {
				// The user has clicked the right button on the applications
				// 'System Menu'. Here is where you would alter the default
				// system menu to reflect your application. Notice how the
				// explorer deals with this. For this app, we aren't doing
				// anything
				return (DefWindowProc(hWnd, message, wParam, lParam));
			  }
			else {
				// Nothing we are interested in, allow default handling...
				return (DefWindowProc(hWnd, message, wParam, lParam));
			  }
      break;

      case WM_RBUTTONDOWN: // RightClick in windows client area...
        pnt.x = LOWORD(lParam);
        pnt.y = HIWORD(lParam);
        ClientToScreen(hWnd, (LPPOINT)&pnt);
        hMenu = GetSubMenu(GetMenu(hWnd),2);
        if(hMenu) {
          TrackPopupMenu(hMenu, 0, pnt.x, pnt.y, 0, hWnd, NULL);
          }
        break;

		case WM_PAINT:
			hDC=BeginPaint(hWnd,&ps);
			myPen=CreatePen(PS_SOLID,16,Colori[0]);
			br.lbStyle=BS_SOLID;
			br.lbColor=Colori[0];

			br.lbHatch=0;
			myBrush=CreateBrushIndirect(&br);
			SelectObject(hDC,myPen);
			SelectObject(hDC,myBrush);
//			SelectObject(hDC,hFont);
//			Rectangle(hDC,0,0,200,200);
			Rectangle(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);

			UpdateScreen(hDC,0/*VERT_OFFSCREEN*/,VERT_SIZE+VERT_OFFSCREEN*2,0);
			DeleteObject(myPen);
			DeleteObject(myBrush);
			EndPaint(hWnd,&ps);
			break;        

		case WM_TIMER:
			TimerCnt++;
			break;

		case WM_SIZE:
			GetClientRect(hWnd,&rc);
			AppXSize=rc.right-rc.left;
			AppYSize=rc.bottom-rc.top;
			AppYSizeR=rc.bottom-rc.top-(GetSystemMetrics(SM_CYSIZEFRAME)*2);
			MoveWindow(hStatusWnd,0,rc.bottom-16,rc.right,16,TRUE);
			break;        

		case WM_KEYDOWN:
			decodeKBD(wParam,lParam,0);
			break;        

		case WM_KEYUP:
			decodeKBD(wParam,lParam,1);
			break;        

		case WM_LBUTTONDOWN:
#ifdef QL
#elif MACINTOSH
      mouseState &= ~B8(00010000);  // 
#endif
			break;        

		case WM_LBUTTONUP:
#ifdef QL
#elif MACINTOSH
      mouseState |= B8(00010000);
#endif
			break;        

		case WM_MOUSEMOVE:
#ifdef QL
#elif MACINTOSH
			{
			mousePos.x=(int)LOWORD(lParam);
			mousePos.y=(int)HIWORD(lParam);
			ScreenToClient(hWnd,&mousePos);

			mouseTimer=4*max(mousePos.x-oldMousePos.x,mousePos.y-oldMousePos.y);
			mousePinState=0;

			}
#endif
			break;        

		case WM_CREATE:

			QueryPerformanceFrequency(&pcFrequency);		// in KHz => 2800000 su PC
			CPUDivider=pcFrequency.QuadPart/CPU_CLOCK_DIVIDER;	// su PC greggiod con 6 ho un clock 68000 ragionevolmente simile all'originale
			

//			bInFront=GetPrivateProfileInt(APPNAME,"SempreInPrimoPiano",0,INIFile);

			bmI=GlobalAlloc(GPTR,sizeof(BITMAPINFO)+(sizeof(COLORREF)*16));
			bmI->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
#ifdef MSX
			bmI->bmiHeader.biBitCount=4;
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=16;
			bmI->bmiHeader.biCompression=BI_RGB;
			bmI->bmiHeader.biWidth=HORIZ_SIZE+HORIZ_OFFSCREEN*2;
			bmI->bmiHeader.biHeight=VERT_SIZE+VERT_OFFSCREEN*2;
			bmI->bmiHeader.biPlanes=1;
			bmI->bmiHeader.biXPelsPerMeter=bmI->bmiHeader.biYPelsPerMeter=0;
			for(i=0; i<bmI->bmiHeader.biClrUsed; i++) {
				bmI->bmiColors[i].rgbRed=GetRValue(Colori[i]);
				bmI->bmiColors[i].rgbGreen=GetGValue(Colori[i]);
				bmI->bmiColors[i].rgbBlue=GetBValue(Colori[i]);
				}
#elif GALAKSIJA
			bmI->bmiHeader.biBitCount=1;
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=2;
			bmI->bmiHeader.biCompression=BI_RGB;
			bmI->bmiHeader.biWidth=HORIZ_SIZE+HORIZ_OFFSCREEN*2;
			bmI->bmiHeader.biHeight=VERT_SIZE+VERT_OFFSCREEN*2;
			bmI->bmiHeader.biPlanes=1;
			bmI->bmiHeader.biXPelsPerMeter=bmI->bmiHeader.biYPelsPerMeter=0;
			for(i=0; i<bmI->bmiHeader.biClrUsed; i++) {
				bmI->bmiColors[i].rgbRed=GetRValue(Colori[i]);
				bmI->bmiColors[i].rgbGreen=GetGValue(Colori[i]);
				bmI->bmiColors[i].rgbBlue=GetBValue(Colori[i]);
				}
#elif defined(ZX80) || defined(ZX81)
			bmI->bmiHeader.biBitCount=1;
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=2;
			bmI->bmiHeader.biCompression=BI_RGB;
			bmI->bmiHeader.biWidth=HORIZ_SIZE+HORIZ_OFFSCREEN*2;
			bmI->bmiHeader.biHeight=VERT_SIZE+VERT_OFFSCREEN*2;
			bmI->bmiHeader.biPlanes=1;
			bmI->bmiHeader.biXPelsPerMeter=bmI->bmiHeader.biYPelsPerMeter=0;
			for(i=0; i<bmI->bmiHeader.biClrUsed; i++) {
				bmI->bmiColors[i].rgbRed=GetRValue(Colori[i]);
				bmI->bmiColors[i].rgbGreen=GetGValue(Colori[i]);
				bmI->bmiColors[i].rgbBlue=GetBValue(Colori[i]);
				}
#endif
			hFont=CreateFont(12,6,0,0,FW_LIGHT,0,0,0,
				ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_MODERN, (LPSTR)"Courier New");
			hFont2=CreateFont(14,7,0,0,FW_LIGHT,0,0,0,
				ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS, (LPSTR)"Arial");
			GetClientRect(hWnd,&rc);
			hStatusWnd = CreateWindow("static","",
				WS_BORDER | SS_LEFT | WS_CHILD,
				0,rc.bottom-16,AppXSize-GetSystemMetrics(SM_CXVSCROLL)-2*GetSystemMetrics(SM_CXSIZEFRAME),16,
				hWnd,(HMENU)1001,g_hinst,NULL);
			ShowWindow(hStatusWnd, SW_SHOW);
			GetClientRect(hWnd,&rc);
			AppYSize=rc.bottom-rc.top;
			AppYSizeR=AppYSize-16;
			SendMessage(hStatusWnd,WM_SETFONT,(WPARAM)hFont2,0);
			hPen1=CreatePen(PS_SOLID,1,RGB(255,255,255));
			br.lbStyle=BS_SOLID;
			br.lbColor=0x000000;
			br.lbHatch=0;
			hBrush=CreateBrushIndirect(&br);
#ifdef ZX80
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_ZX80),"BINARY"))) {
#elif ZX81
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_ZX81),"BINARY"))) {
#elif SKYNET
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_Z803),"BINARY"))) {
				HRSRC hrsrc2;
				if((hrsrc2=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_CASANET3),"BINARY"))) {
					hROM=LoadResource(NULL,hrsrc2);
					memcpy(rom_seg2,hROM,ROM_SIZE2);
					FreeResource(hrsrc2);
					}
#elif NEZ80
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_Z80NE_382),"BINARY"))) {
#elif GALAKSIJA
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GALAKSIJA_A),"BINARY"))) {
				HRSRC hrsrc2;
				if((hrsrc2=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_GALAKSIJA_B),"BINARY"))) {
					hROM=LoadResource(NULL,hrsrc2);
					memcpy(rom_seg2,hROM,ROM_SIZE2);
					FreeResource(hrsrc2);
					}
#elif MSX
			if((hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_BINARY_MSX_SPECTRAVIDEO_SVI738),"BINARY"))) {
#endif
				hROM=LoadResource(NULL,hrsrc);
				memcpy(rom_seg,hROM,ROM_SIZE);
				FreeResource(hrsrc);
				}
			else
				MessageBox(NULL,"Impossibile caricare ROM",NULL,MB_OK);



			spoolFile=_lcreat("spoolfile.txt",0);
/*			{extern const unsigned char IBMBASIC[32768];
			spoolFile=_lcreat("ibmbasic.bin",0);
			_lwrite(spoolFile,IBMBASIC,32768);
			}*/
/*			{
				char *myBuf=malloc(100);
//				memset(myBuf,0,100000);
			Disassemble(rom_seg,spoolFile,myBuf,0x2000,0x0000,7);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			free(myBuf);
			}*/

/*			_outp(0x37a,8);
			_outp(0x37a,0);
			*/



#ifdef DEBUG_TESTSUITE
			{char *buf;
			FILE *f;
			struct _stat fs;
			cJSON *theJSON,*j2,*jtop;
			char myBuf[256];
		  extern union Z_REGISTERS regs1,regs2;
			extern uint16_t _pc,_sp;
			extern SWORD _ix;
			extern SWORD _iy;
			extern BYTE IRQ_Mode;
			extern BYTE IRQ_Enable1,IRQ_Enable2,inEI;
			extern union REGISTRO_F _f,_f1;
	extern BYTE _i,_r;
#define _a regs1.r[3].b.l //
#define _f_af regs1.r[3].b.h //
#define _b regs1.r[0].b.h
#define _c regs1.r[0].b.l
#define _d regs1.r[1].b.h
#define _e regs1.r[1].b.l
#define _h regs1.r[2].b.h
#define _l regs1.r[2].b.l
#define _af regs1.r[3].x
#define _bc regs1.r[0].x
#define _de regs1.r[1].x
#define _hl regs1.r[2].x

			WORD opcode=6; WORD opcode2;


			for(opcode=0x0; opcode<=0xff; opcode++) {
//			for(opcode2=0; opcode2<=0xff; opcode2++) {

//			wsprintf(myBuf,"%02x cb __ %02x.json",opcode,opcode2);
//			wsprintf(myBuf,"%02x %02x.json",opcode,opcode2);
			wsprintf(myBuf,"%02x.json",opcode,opcode2);
			if(f=fopen(myBuf,"r")) {
				_fstat(f->_file,&fs);
				buf=malloc(fs.st_size);
				fread(buf,fs.st_size,1,f);
				theJSON=cJSON_Parse(buf);


				jtop=theJSON->child;
				j2=jtop;
				while(j2) {		// ne farà 10000!
					cJSON *j3,*j4;
					j3=j2->child;
					_lwrite(spoolFile,j3->valuestring,strlen(j3->valuestring));		// qua il mnemonico istruzione QUA NO PORCAMADONNA #cancrobambini
	// qua i byte istr.
					_lwrite(spoolFile,"\r\n",2);

					j3=j3->next;							// qua istr.

//				j4=FindItem(theJSON,NULL,"initial");	// prova
				j4=j3;			//"initial"
				j4=j4->child;			// qua i valori registri/ram iniziali

					while(j4) {

						if(stricmp(j4->string,"ram")) {
							sprintf(myBuf,"%s: %d %04X\r\n",j4->string,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
							_lwrite(spoolFile,myBuf,strlen(myBuf));
							}
						if(!stricmp(j4->string,"a"))
							_a=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"b"))
							_b=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"c"))
							_c=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"d"))
							_d=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"e"))
							_e=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"f"))
							_f.b =(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"h"))
							_h=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"l"))
							_l=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"pc"))
							_pc=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"sp"))
							_sp=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"i"))
							_i=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"r"))
							_r=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"ei"))
							inEI=(BYTE)j4->valuedouble;
/* boh che sono??						else if(!stricmp(j4->string,"wz"))
							_wz=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"p"))
							_p=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"q"))
							_q=(BYTE)j4->valuedouble;*/
						else if(!stricmp(j4->string,"af_"))
							regs2.r[3].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"bc_"))
							regs2.r[0].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"de_"))
							regs2.r[1].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"hl_"))
							regs2.r[2].x=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"ix"))
							_ix=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"iy"))
							_iy=(WORD)j4->valuedouble;
						else if(!stricmp(j4->string,"im"))
							IRQ_Mode=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"iff1"))
							IRQ_Enable1=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"iff2"))
							IRQ_Enable2=(BYTE)j4->valuedouble;
						else if(!stricmp(j4->string,"ram")) {
							cJSON *j5;
							j5=j4->child;			// qua i valori ram iniziali
							while(j5) {

								sprintf(myBuf,"%u (%04X): %d (%02X)\r\n",(WORD)j5->child->valuedouble,(WORD)j5->child->valuedouble,
									(BYTE)j5->child->next->valuedouble,(BYTE)j5->child->next->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
		//						PutValue((DWORD)j4->child->valuedouble,(BYTE)j4->child->next->valuedouble);
								ram_seg[(WORD)j5->child->valuedouble]=(BYTE)j5->child->next->valuedouble;
								j5=j5->next;
								}

							}
						j4=j4->next;
						}


				singleStep();

					j3=j3->next;
					j4=j3->child;			// qua i valori registri/ram finali
					while(j4) {

						if(stricmp(j4->string,"ram")) {
							sprintf(myBuf,"%s: %d %04X\r\n",j4->string,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
							_lwrite(spoolFile,myBuf,strlen(myBuf));
							}
						if(!stricmp(j4->string,"a")) {
							if(_a != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_a,_a,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"b")) {
							if(_b != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_b,_b,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"c")) {
							if(_c != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_c,_c,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"d")) {
							if(_d != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_d,_d,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"e")) {
							if(_e != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_e,_e,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"f")) {
							if((_f.b & ~0x28) != ((BYTE)j4->valuedouble & ~0x28)) {		// patch per cazzata  https://jnz.dk/z80/flags.html
								sprintf(myBuf,"DIFFf %s: %u (%02X) != %u (%02X)\r\n",j4->string,_f.b,_f.b,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"h")) {
							if(_h != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_h,_h,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"l")) {
							if(_l != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_l,_l,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"pc")) {
							if(_pc != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,_pc,_pc,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"sp")) {
							if(_sp != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,_sp,_sp,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"i")) {
							if(_i != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_i,_i,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"r")) {
							if(_r != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_r,_r,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"ei")) {
							if(inEI != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,inEI,inEI,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
/* boh che sono??						else if(!stricmp(j4->string,"p")) {
							if(_p != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_p,_p,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"q")) {
							if(_q != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,_q,_q,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"wz")) {
							if(_wz != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFFf %s: %u (%02X) != %u (%02X)\r\n",j4->string,_wz,_wz,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}*/
						else if(!stricmp(j4->string,"af_")) {
							if(regs2.r[3].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs2.r[3].x,regs2.r[3].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"bc_")) {
							if(regs2.r[0].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs2.r[0].x,regs2.r[0].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"de_")) {
							if(regs2.r[1].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs2.r[1].x,regs2.r[1].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"hl_")) {
							if(regs2.r[2].x != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,regs2.r[2].x,regs2.r[2].x,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"ix")) {
							if(_ix != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,_ix,_ix,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"iy")) {
							if(_iy != (WORD)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%04X) != %u (%04X)\r\n",j4->string,_iy,_iy,(WORD)j4->valuedouble,(WORD)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"im")) {
							if(IRQ_Mode != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,IRQ_Mode,IRQ_Mode,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"iff1")) {
							if(IRQ_Enable1 != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,IRQ_Enable1,IRQ_Enable1,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"iff2")) {
							if(IRQ_Enable2 != (BYTE)j4->valuedouble) {
								sprintf(myBuf,"DIFF %s: %u (%02X) != %u (%02X)\r\n",j4->string,IRQ_Enable2,IRQ_Enable2,(BYTE)j4->valuedouble,(BYTE)j4->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								}
							}
						else if(!stricmp(j4->string,"ram")) {
							cJSON *j5;
							j5=j4->child;			// qua i valori ram finali
							while(j5) {

								sprintf(myBuf,"%u (%04X): %d (%02X)\r\n",(WORD)j5->child->valuedouble,(WORD)j5->child->valuedouble,
									(BYTE)j5->child->next->valuedouble,(BYTE)j5->child->next->valuedouble);
								_lwrite(spoolFile,myBuf,strlen(myBuf));
								if(ram_seg[(WORD)j5->child->valuedouble] != (BYTE)j5->child->next->valuedouble) {
									sprintf(myBuf,"DIFFRAM %u (%04X): %u (%02X) != %u (%02X)\r\n",(WORD)j5->child->valuedouble,(WORD)j5->child->valuedouble,
										ram_seg[(WORD)j5->child->valuedouble],ram_seg[(WORD)j5->child->valuedouble],
										(BYTE)j5->child->next->valuedouble,(BYTE)j5->child->next->valuedouble);
									_lwrite(spoolFile,myBuf,strlen(myBuf));
									}
								j5=j5->next;
								}
							}

						j4=j4->next;
						}


					_lwrite(spoolFile,"\r\n",2);
					j2=j2->next;
					}


				cJSON_Delete(theJSON);
				free(buf);
				fclose(f);
				}
					MessageBeep(0);
//						PlayResource(MAKEINTRESOURCE(IDR_WAVE_SPEAKER),FALSE);
//				}
				}
			}
#endif



//			hTimer=SetTimer(NULL,0,1000/32,myTimerProc);  // (basato su Raster
			// non usato... fa schifo! era per refresh...
#ifdef NEZ80
			hTimer=SetTimer(NULL,0,1000/50,myTimerProc);  // era per mux ma metto di là
#else
			hTimer=SetTimer(NULL,0,1000/50,myTimerProc);  // 
#endif
			initHW();
			ColdReset=1;
			break;

		case WM_QUERYENDSESSION:
#ifndef _DEBUG
			if(MessageBox(hWnd,"La chiusura del programma cancellerà la memoria dello Z80. Continuare?",APPNAME,MB_OKCANCEL | MB_ICONSTOP | MB_DEFBUTTON2) == IDOK)
				return 1l;
			else 
				return 0l;
#else
			return 1l;
#endif
			break;

		case WM_CLOSE:
esciprg:          
#ifndef _DEBUG
			if(MessageBox(hWnd,"La chiusura del programma cancellerà la memoria dello Z80. Continuare?",APPNAME,MB_OKCANCEL | MB_ICONSTOP | MB_DEFBUTTON2) == IDOK) {
			  DestroyWindow(hWnd);
			  }
#else
		  DestroyWindow(hWnd);
#endif
			return 0l;
			break;

		case WM_DESTROY:
//			WritePrivateProfileInt(APPNAME,"SempreInPrimoPiano",bInFront,INIFile);
			// Tell WinHelp we don't need it any more...
			KillTimer(hWnd,hTimer);
			_lclose(spoolFile);
	    WinHelp(hWnd,APPNAME".HLP",HELP_QUIT,(DWORD)0);
			GlobalFree(bmI);
			DeleteObject(hBrush);
			DeleteObject(hPen1);
			DeleteObject(hFont);
			DeleteObject(hFont2);
			PostQuitMessage(0);
			break;

   	case WM_INITMENU:
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_DEBUG,MF_BYCOMMAND | (debug ? MF_CHECKED : MF_UNCHECKED));
//   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_TRACE,MF_BYCOMMAND | (_f.SR.Trace ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_DIMENSIONEDOPPIA,MF_BYCOMMAND | (doppiaDim ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_VELOCIT_LENTO,MF_BYCOMMAND | (CPUDivider==pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER/2) ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_VELOCIT_NORMALE,MF_BYCOMMAND | (CPUDivider==pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER) ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_VELOCIT_VELOCE,MF_BYCOMMAND | (CPUDivider==pcFrequency.QuadPart/(CPU_CLOCK_DIVIDER*2) ? MF_CHECKED : MF_UNCHECKED));
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}
	return (0);
	}



ATOM MyRegisterClass(CONST WNDCLASS *lpwc) {
	HANDLE  hMod;
	FARPROC proc;
	WNDCLASSEX wcex;

	hMod=GetModuleHandle("USER32");
	if(hMod != NULL) {

#if defined (UNICODE)
		proc = GetProcAddress (hMod, "RegisterClassExW");
#else
		proc = GetProcAddress (hMod, "RegisterClassExA");
#endif

		if(proc != NULL) {
			wcex.style         = lpwc->style;
			wcex.lpfnWndProc   = lpwc->lpfnWndProc;
			wcex.cbClsExtra    = lpwc->cbClsExtra;
			wcex.cbWndExtra    = lpwc->cbWndExtra;
			wcex.hInstance     = lpwc->hInstance;
			wcex.hIcon         = lpwc->hIcon;
			wcex.hCursor       = lpwc->hCursor;
			wcex.hbrBackground = lpwc->hbrBackground;
    	wcex.lpszMenuName  = lpwc->lpszMenuName;
			wcex.lpszClassName = lpwc->lpszClassName;

			// Added elements for Windows 95:
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.hIconSm = LoadIcon(wcex.hInstance, "SMALL");
			
			return (*proc)(&wcex);//return RegisterClassEx(&wcex);
		}
	}
return (RegisterClass(lpwc));
}


BOOL InitApplication(HINSTANCE hInstance) {
  WNDCLASS  wc;
  HWND      hwnd;

  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_APP32));
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH));

        // Since Windows95 has a slightly different recommended
        // format for the 'Help' menu, lets put this in the alternate menu like this:
  if(IS_WIN95) {
		wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    } else {
	  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    }
  wc.lpszClassName = szAppName;

  if(IS_WIN95) {
	  if(!MyRegisterClass(&wc))
			return 0;
    }
	else {
	  if(!RegisterClass(&wc))
	  	return 0;
    }

	return 1;
  }

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	
	g_hinst=hInstance;

	ghWnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_THICKFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		AppXSize+GetSystemMetrics(SM_CXSIZEFRAME)*2,AppYSize+GetSystemMetrics(SM_CYSIZEFRAME)*2+GetSystemMetrics(SM_CYMENUSIZE)+GetSystemMetrics(SM_CYSIZE)+18,
		NULL, NULL, hInstance, NULL);

	if(!ghWnd) {
		return (FALSE);
	  }

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return (TRUE);
  }

//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
// 		This version allows greater flexibility over the contents of the 'About' box,
// 		by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
//	WM_INITDIALOG - initialize dialog box
//	WM_COMMAND    - Input received
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static  HFONT hfontDlg;		// Font for dialog text
	static	HFONT hFinePrint;	// Font for 'fine print' in dialog
	DWORD   dwVerInfoSize;		// Size of version information block
	LPSTR   lpVersion;			// String pointer to 'version' text
	DWORD   dwVerHnd=0;			// An 'ignored' parameter, always '0'
	UINT    uVersionLen;
	WORD    wRootLen;
	BOOL    bRetCode;
	int     i;
	char    szFullPath[256];
	char    szResult[256];
	char    szGetName[256];
	DWORD	dwVersion;
	char	szVersion[40];
	DWORD	dwResult;

	switch (message) {
    case WM_INITDIALOG:
//			ShowWindow(hDlg, SW_HIDE);
			hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");
			hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");
//			CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
			GetModuleFileName(g_hinst, szFullPath, sizeof(szFullPath));

			// Now lets dive in and pull out the version information:
			dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
			if(dwVerInfoSize) {
				LPSTR   lpstrVffInfo;
				HANDLE  hMem;
				hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
				lpstrVffInfo  = GlobalLock(hMem);
				GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
				// The below 'hex' value looks a little confusing, but
				// essentially what it is, is the hexidecimal representation
				// of a couple different values that represent the language
				// and character set that we are wanting string values for.
				// 040904E4 is a very common one, because it means:
				//   US English, Windows MultiLingual characterset
				// Or to pull it all apart:
				// 04------        = SUBLANG_ENGLISH_USA
				// --09----        = LANG_ENGLISH
				// ----04E4 = 1252 = Codepage for Windows:Multilingual
				lstrcpy(szGetName, "\\StringFileInfo\\040904E4\\");	 
				wRootLen = lstrlen(szGetName); // Save this position
			
				// Set the title of the dialog:
				lstrcat (szGetName, "ProductName");
				bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
					(LPSTR)szGetName,
					(LPVOID)&lpVersion,
					(UINT *)&uVersionLen);
//				lstrcpy(szResult, "About ");
//				lstrcat(szResult, lpVersion);
//				SetWindowText (hDlg, szResult);

				// Walk through the dialog items that we want to replace:
				for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
					GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
					szGetName[wRootLen] = (char)0;
					lstrcat (szGetName, szResult);
					uVersionLen   = 0;
					lpVersion     = NULL;
					bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
						(LPSTR)szGetName,
						(LPVOID)&lpVersion,
						(UINT *)&uVersionLen);

					if(bRetCode && uVersionLen && lpVersion) {
					// Replace dialog item text with version info
						lstrcpy(szResult, lpVersion);
						SetDlgItemText(hDlg, i, szResult);
					  }
					else {
						dwResult = GetLastError();
						wsprintf (szResult, "Error %lu", dwResult);
						SetDlgItemText (hDlg, i, szResult);
					  }
					SendMessage (GetDlgItem (hDlg, i), WM_SETFONT, 
						(UINT)((i==DLG_VERLAST)?hFinePrint:hfontDlg),TRUE);
				  } // for


				GlobalUnlock(hMem);
				GlobalFree(hMem);

			}
		else {
				// No version information available.
			} // if (dwVerInfoSize)

    SendMessage(GetDlgItem (hDlg, IDC_LABEL), WM_SETFONT,
			(WPARAM)hfontDlg,(LPARAM)TRUE);

			// We are  using GetVersion rather then GetVersionEx
			// because earlier versions of Windows NT and Win32s
			// didn't include GetVersionEx:
			dwVersion = GetVersion();

			if (dwVersion < 0x80000000) {
				// Windows NT
				wsprintf (szVersion, "Microsoft Windows NT %u.%u (Build: %u)",
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
          (DWORD)(HIWORD(dwVersion)) );
				}
			else
				if (LOBYTE(LOWORD(dwVersion))<4) {
					// Win32s
				wsprintf (szVersion, "Microsoft Win32s %u.%u (Build: %u)",
  				(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
					(DWORD)(HIWORD(dwVersion) & ~0x8000) );
				}
			else {
					// Windows 95
				wsprintf(szVersion,"Microsoft Windows 95 %u.%u",
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))) );
				}

			SetWindowText(GetDlgItem(hDlg, IDC_OSVERSION), szVersion);
//			SetWindowPos(hDlg,NULL,GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2,0,0,SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
//			ShowWindow(hDlg, SW_SHOW);
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK || wParam==IDCANCEL) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			else if(wParam==3) {
				MessageBox(hDlg,"Se trovate utile questo programma, mandate un contributo!!\nVia Rivalta 39 - 10141 Torino (Italia)\n[Dario Greggio]","ADPM Synthesis sas",MB_OK);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}


LRESULT CALLBACK Dump0(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char myBuf[256],myBuf1[32];
	int i,j;

	switch (message) {
    case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),WM_SETFONT,(WPARAM)hFont,0);
			for(i=0; i<256; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",ram_seg[i+j]);
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}

LRESULT CALLBACK Dump1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char myBuf[256],myBuf1[32];
	int i,j;

	switch (message) {
    case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST2),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST2),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),WM_SETFONT,(WPARAM)hFont,0);
			for(i=0xd000; i<0xd000+47; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",GetValue(i+j));
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			for(i=0xd400; i<=0xd41C; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",GetValue(i+j));
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST2),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			i=0xdc00;
			wsprintf(myBuf,"%04X: ",i);
			for(j=0; j<16; j++) {
				wsprintf(myBuf1,"%02X ",GetValue(i+j));
				strcat(myBuf,myBuf1);
			  }
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),LB_ADDSTRING,0,(LPARAM)myBuf);
			i=0xdd00;
			wsprintf(myBuf,"%04X: ",i);
			for(j=0; j<16; j++) {
				wsprintf(myBuf1,"%02X ",GetValue(i+j));
				strcat(myBuf,myBuf1);
			  }
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),LB_ADDSTRING,0,(LPARAM)myBuf);
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}



int WritePrivateProfileInt(char *s, char *s1, int n, char *f) {
  int i;
  char myBuf[16];
  
  wsprintf(myBuf,"%d",n);
  WritePrivateProfileString(s,s1,myBuf,f);
  }

int ShowMe() {
	int i;
	char buffer[16];

	buffer[0]='A'^ 0x17;
	buffer[1]='D'^ 0x17;
	buffer[2]='P'^ 0x17;
	buffer[3]='M'^ 0x17;
	buffer[4]='-'^ 0x17;
	buffer[5]='G'^ 0x17;
	buffer[6]='.'^ 0x17;
	buffer[7]='D'^ 0x17;
	buffer[8]='a'^ 0x17;
	buffer[9]='r'^ 0x17;
	buffer[10]=' '^ 0x17;
	buffer[11]='2'^ 0x17;
	buffer[12]='2' ^ 0x17;
	buffer[13]=0;
	for(i=0; i<13; i++) buffer[i]^=0x17;
	MessageBox(GetDesktopWindow(),buffer,APPNAME,MB_OK | MB_ICONEXCLAMATION);
	}


