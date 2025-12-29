#include <windows.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include "z80win.h"

#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )


#define UNIMPLEMENTED_MEMORY_VALUE 0xFF

BYTE ram_seg[RAM_SIZE];
#ifdef RAM_SIZE2 
BYTE ram_seg2[RAM_SIZE2];
#endif
BYTE rom_seg[ROM_SIZE];			
#ifdef ROM_SIZE2 
BYTE rom_seg2[ROM_SIZE2];
#endif
#ifdef SKYNET
BYTE i8255RegR[4],i8255RegW[4];
BYTE LCDram[256 /*4*40*/],LCDCGDARAM=0,LCDCGram[64],LCDCGptr=0,LCDfunction,LCDdisplay,
	LCDentry=2 /* I/D */,LCDcursor;			// emulo LCD text come Z80net
signed char LCDptr=0;
BYTE IOExtPortI[4],IOExtPortO[4];
BYTE IOPortI,IOPortO,ClIRQPort,ClWDPort;
/*Led1Bit        equ 7
Led2Bit        equ 6
SpeakerBit     equ 5
WDEnBit        equ 3
ComOutBit2     equ 2
NMIEnBit       equ 1
ComOutBit      equ 0
Puls1Bit       equ 7
ComInBit2      equ 5
DipSwitchBit   equ 1
ComInBit       equ 0*/
BYTE i146818RegR[2],i146818RegW[2],i146818RAM[64];
BYTE i8042RegR[2],i8042RegW[2];
BYTE KBDataI,KBDataO,KBControl,/*KBStatus,*/ KBRAM[32];   // https://wiki.osdev.org/%228042%22_PS/2_Controller
#define KBStatus KBRAM[0]   // pare...
BYTE Keyboard[1]={0};
volatile BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
#endif
#ifdef NEZ80
BYTE DisplayRAM[8];
BYTE Keyboard[2]={0,0};		// tast hex, tast LX388 video
volatile BYTE TIMIRQ;
BYTE sense50Hz;
volatile BYTE MUXcnt;
BYTE isKeyPressed;
#endif
#ifdef ZX80
volatile BYTE TIMIRQ,lineCntr;
//http://searle.x10host.com/zx80/zx80.html
BYTE Keyboard[8]={255,255,255,255,255,255,255,255};
/* There are forty ($28) system variables followed by Program area
; These are located at the start of RAM.
;
; +---------+---------+-----------+---+-----------+-----------+-------+-------+
; |         |         |           |   |           |           |       |       |
; | SYSVARS | Program | Variables |80h| WKG Space | Disp File | Spare | Stack |
; |         |         |           |   |           |           |       |       |
; +---------+---------+-----------+---+-----------+-----------+-------+-------+
;           ^         ^               ^           ^     ^     ^       ^
;         $4024      VARS            E_LINE    D_FILE       DF_END   SP
;                                                     DF_EA
*/
#endif
#ifdef ZX81
volatile BYTE TIMIRQ,NMIGenerator=0;
BYTE Keyboard[8]={255,255,255,255,255,255,255,255};
#endif
#ifdef GALAKSIJA
volatile BYTE TIMIRQ;
BYTE Keyboard[8]={255,255,255,255,255,255,255,255};
BYTE Latch=0;
/* About the latch
; ***************

; A 6-bit register (called "latch" in the original documentation) can be 
; accessed on all memory addresses that can be written in binary as

; 0 0 1 0  0 x x x  x x 1 1  1 x x x  
; (for example 207fh as used in VIDEO_INT)

; The content is write-only. A read from these addresses will return an 
; unspecified value.

; Individual bits have the following meaning:
;
;  7	Clamp RAM A7 to one (1 disabled, 0 enabled)
;  ---
;  6    Cassette port output bit 0
;  ---
;  5    Character generator row bit 3
;  ---
;  4	Character generator row bit 2
;  ---
;  3	Character generator row bit 1
;  ---
;  2	Character generator row bit 0
;	Cassette port output bit 1
;  ---
;  1	Unused
;  ---
;  0	Unused

;  Character generator row bits hold the current row of the character being
;  drawn to the screen.

;  Cassette port is high if both output bits are 1, low if both are 0 and
;  zero if one bit is 1 and one is 0.

;  Bit 7 forces RAM address line A7 to one. This is required because the top
;  bit of the R register never changes (only bottom 7 bits are incremented
;  during each opcode).
*/
#endif
#ifdef MSX
volatile BYTE TIMIRQ,VIDIRQ;
volatile WORD TIMEr;
BYTE TMS9918Reg[8],TMS9918RegS,TMS9918Sel,TMS9918WriteStage,TMS9918Buffer;
WORD TMS9918RAMPtr;
BYTE AY38910RegR[16],AY38910RegW[16],AY38910RegSel;
BYTE i8255RegR[4],i8255RegW[4];
BYTE i8251RegR[4],i8251RegW[4];
BYTE i8253RegR[4],i8253RegW[4];
BYTE iPrinter[2];
BYTE TMSVideoRAM[TMSVIDEORAM_SIZE];
BYTE Keyboard[11]={255,255,255,255,255,255,255,255,255,255,255};
#endif


extern BYTE CPUPins;
extern BYTE Pipe1;
extern union PIPE Pipe2;

#define MAX_WATCHDOG 100      // v. di là
extern WORD WDCnt;


//extern BYTE KBDataI,KBDataO,KBControl,/*KBStatus,*/ KBRAM[32];   // https://wiki.osdev.org/%228042%22_PS/2_Controller
//#define KBStatus KBRAM[0]   // pare...
extern BYTE Keyboard[];
extern volatile DWORD millis;




BYTE _fastcall GetValue(SWORD t) {
	register BYTE i;

#ifdef DEBUG_TESTSUITE
  i=ram_seg[t];
#else

#ifdef MSX
  switch(t & B16(11000000,00000000)) {
    case 0<<14:
      switch(i8255RegW[0] & B8(00000011)) {      // gestire slot, 
        case 0:
          i=rom_seg[t];
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
            i=ram_seg2[t-RAM_START2];
          else
            i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
#endif
        default:
          i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 1<<14:
      switch(i8255RegW[0] & B8(00001100)) {      // gestire slot, 
        case 0:
          i=rom_seg[t];
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
            i=ram_seg2[t-RAM_START2];
          else
            i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
#endif
        default:
          i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 2<<14:
      switch(i8255RegW[0] & B8(00110000)) {      // gestire slot, 
        case 0<<4:
          if(t>=RAM_START)
            i=ram_seg[t-RAM_START];
          else
            i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
            i=ram_seg2[t-RAM_START2];
          else
            i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
#endif
        default:
          i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 3<<14:
      switch(i8255RegW[0] & B8(11000000)) {      // gestire slot, 
        case 0<<6:
          if(t>=RAM_START)
            i=ram_seg[t-RAM_START];
          else
            i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
#ifdef RAM_SIZE2 
        case 2<<6:
          if(t>=RAM_START2)
            i=ram_seg2[t-RAM_START2];
          else
            i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
#endif
        default:
          i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    }
#ifdef MSX2
gestire 0xffff per subslot...
#endif
#else
#ifdef NEZ80
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		t-=RAM_START;
		i=ram_seg[t];
		}
	else if(t >= ROM_START && t < (ROM_START+ROM_SIZE)) {
		t-=ROM_START;
		i=rom_seg[t];
		}
#else
#ifdef ZX80
//xché??	t &= 0x7fff;
#endif
#ifdef ZX81
//xché??	t &= 0x7fff;
#endif
	if(t < ROM_SIZE) {			// ZX80, 81, Galaksija
		i=rom_seg[t];
		}
#ifdef SKYNET
	else if((t-0x4000) < ROM_SIZE2) {			// SKYNET
		i=rom_seg2[t-0x4000];
		}
#elif GALAKSIJA
	else if((t-ROM_SIZE2) < ROM_SIZE2) {			// 
		i=rom_seg2[t-ROM_SIZE2];
		}
#endif
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		i=ram_seg[t-RAM_START];
		}
#ifdef SKYNET
  else if(t==0xe000) {        //
//    IOPortI = B8(00011100);      // dip-switch=0001; v. main
//    IOPortI |= B8(00000001);      // ComIn
/*    if(PORTDbits.RD2)
      IOPortI |= 0x80;      // pulsante
    else
      IOPortI &= ~0x80;*/
    i=IOPortI;
    }
  else if(t>=0xe002 && t<=0xe003) {        //   CMOS RAM/RTC (Real Time Clock  MC146818)
    t &= 0x1;
    switch(t) {
      case 0:
        i=i146818RegR[0];
        break;
      case 1:     // il bit 7 attiva/disattiva NMI boh??
        switch(i146818RegW[0] & 0x3f) {
          case 0:
//            i146818RegR[1]=currentTime.sec;
            break;
            // in mezzo c'è Alarm...
          case 2:
//            i146818RegR[1]=currentTime.min;
            break;
          case 4:
//            i146818RegR[1]=currentTime.hour;
            break;
            // 6 è day of week...
          case 7:
//            i146818RegR[1]=currentDate.mday;
            break;
          case 8:
//            i146818RegR[1]=currentDate.mon;
            break;
          case 9:
//            i146818RegR[1]=currentDate.year;
            break;
          case 12:
						i146818RegR[1]=i146818RAM[12] = 0;   // flag IRQ
            break;
          default:      // qua ci sono i 4 registri e poi la RAM
            i146818RegR[1]=i146818RAM[i146818RegW[0] & 0x3f];
            break;
          }
        i=i146818RegR[1];
        break;
      }
    }
  else if(t==0xe004) {
    ClIRQPort;
    }
  else if(t==0xe005) {
    ClWDPort;
    }
#endif
#ifdef GALAKSIJA
  else if((t >= 0x2000) && (t <= 0x27ff)) {
//    printf("Leggo a %04x: %02x\n",t,CIA1Reg[t & 0xf]);
    t &= 0x3f;
    if(t<=0x37)
      i = Keyboard[t >> 3] & (1 << (t & 0x7)) ? 1 : 0;   // i 3 bit bassi pilotano il mux row/lettura, i 3 alti il mux col/scrittura, out è su D0
    // in effetti sono 7, ma anche se il sw sforasse si becca cmq latch :)
    else
      i = Latch;   // (in effetti non è leggibile...)
    }
#endif
#ifdef MSX
#endif
#endif
#endif
#ifndef MSX
	else
    i=UNIMPLEMENTED_MEMORY_VALUE;
#endif
#endif

	return i;
	}

#ifdef SKYNET
#define LCD_BOARD_ADDRESS 0x80
// emulo display LCD testo (4x20, 4x20 o altro) come su scheda Z80:
//  all'indirizzo+0 c'è la porta dati (in/out), a +2 i fili C/D, RW e E (E2)
#define IO_BOARD_ADDRESS 0x00
#endif
BYTE _fastcall InValue(SWORD t) {    // OCCHIO pare che siano 16bit anche I/O!
	register BYTE i,j;

#ifdef DEBUG_TESTSUITE
	i=0xff;
#else

#ifdef ZX80
//  https://electronics.stackexchange.com/questions/51460/how-does-the-zx80-keyboard-avoid-ghosting-and-masking
/*Input from Port FEh (or any other port with A0 zero)
Reading from this port initiates the Vertical Retrace period (and accordingly,
Cassette Output becomes Low), and resets the LINECNTR register to zero,
LINECNTR remains stopped/zero until user terminates retrace - In the ZX81, all
of the above happens only if NMIs are disabled.
Bit Expl.
0-4 Keyboard column bits (0=Pressed)
5 Not used (1)
6 Display Refresh Rate (0=60Hz, 1=50Hz)
7 Cassette input (0=Normal, 1=Pulse)*/
  switch(t) {     // OCCHIO byte alto! qua è così
/*          A8   A9  A10  A11  A12  A13  A14  A15  Keyboard layout
            |    |    |    |    |    |    |    |
    K4 -    V    G    T    5    6    Y    H    B
    K3 -    C    F    R    4    7    U    J    N
    K2 -    X    D    E    3    8    I    K    M
    K1 -    Z    S    W    2    9    O    L    .
    K0 -  Shift  A    Q    1    0    P   NL  Space

(I'm using K0-K5 to be the inputs to IC 10)
Lines K0,K1,K2,K3,K4 are pulled to 5V by R13,R14,R15,R16,R17*/
/*    Port____Line____Bit__0____1____2____3____4__
FEFEh 0 (A8) SHIFT Z X C V
FDFEh 1 (A9) A S D F G
FBFEh 2 (A10) Q W E R T
F7FEh 3 (A11) 1 2 3 4 5
EFFEh 4 (A12) 0 9 8 7 6
DFFEh 5 (A13) P O I U Y
BFFEh 6 (A14) ENTER L K J H
7FFEh 7 (A15) SPC . M N B*/
    case 0xFEFE:
      i = Keyboard[0];
      break;
    case 0xFDFE:
      i = Keyboard[1];
      break;
    case 0xFBFE:
      i = Keyboard[2];
      break;
    case 0xF7FE:
      i = Keyboard[3];
      break;
    case 0xEFFE:
      i = Keyboard[4];
      break;
    case 0xDFFE:
      i = Keyboard[5];
      break;
    case 0xBFFE:
      i = Keyboard[6];
      break;
    case 0x7FFE:
      i = Keyboard[7];
      break;
    }
#elif ZX81
  switch(t) {     // OCCHIO byte alto! qua è così
    case 0xFEFE:
      i = Keyboard[0];
      break;
    case 0xFDFE:
      i = Keyboard[1];
      break;
    case 0xFBFE:
      i = Keyboard[2];
      break;
    case 0xF7FE:
      i = Keyboard[3];
      break;
    case 0xEFFE:
      i = Keyboard[4];
      break;
    case 0xDFFE:
      i = Keyboard[5];
      break;
    case 0xBFFE:
      i = Keyboard[6];
      break;
    case 0x7FFE:
      i = Keyboard[7];
      break;
    }
#elif SKYNET
  switch(t & 0xff) {
    case IO_BOARD_ADDRESS:
    case IO_BOARD_ADDRESS+1:
    case IO_BOARD_ADDRESS+2:
    case IO_BOARD_ADDRESS+3:
      i=IOExtPortI[t-IO_BOARD_ADDRESS];
      break;
//    case 0x0e:      // board signature...
//#warning TOGLIERE QUA!
//      return 0x68;      // LCD
//      break;

    case LCD_BOARD_ADDRESS:
      // per motivi che ora non ricordo, il BIOS indirizza 0..3 mentre la 2°EEprom (casanet) aveva indirizzi doppi o meglio bit 0..1 messi a 2-0
      // potrei ricompilare casanet per andare "dritto" 0..3, per ora lascio così (unico problema è conflitto a +6 con la tastiera... amen! tanto scrive solo all'inizio)
      // if(i8255RegR[2] & B8(01000000)) fare...?
      // else 			i=i8255RegW[0];
      if(!(i8255RegR[1] & 1))			// se status...
        i8255RegR[0]=0 | (LCDptr & 0x7f); //sempre ready!
      else {
        if(!LCDCGDARAM) {
          i8255RegR[0]=LCDram[LCDptr]; //
          if(LCDentry & 2) {
            LCDptr++;
            if(LCDptr >= 0x7f)			// parametrizzare! o forse no, fisso max 127
              LCDptr=0;
            }
          else {
            LCDptr--;
            if(LCDptr < 0)			// parametrizzare!
              LCDptr=0x7f;    // CONTROLLARE
            }
          }
        else {
          i8255RegR[0]=LCDCGram[LCDptr++]; //
          LCDCGptr &= 0x3f;
          }
        }
			i=i8255RegR[0];
      break;
    case LCD_BOARD_ADDRESS+2:
			i=i8255RegR[1];
      break;
    case LCD_BOARD_ADDRESS+4:
      // qua c'è la 3° porta del 8255
      // il b6 mette a in o out la porta dati A (1=input)
			i=i8255RegR[2];
      break;
    case LCD_BOARD_ADDRESS+5 /* sarebbe 6 ma ovviamente non si può! v. sopra*/:
      // 8255 settings
			i=i8255RegR[3];
      break;
      
    case LCD_BOARD_ADDRESS+6:   // 
      if(i8042RegW[1]==0xAA) {      // self test
        i8042RegR[0]=0x55;
        }
      else if(i8042RegW[1]==0xAB) {      // diagnostics
        i8042RegR[0]=B8(00000000);
        }
      else if(i8042RegW[1]>=0x20 && i8042RegW[1]<0x40) {
        i8042RegR[0]=KBRAM[i8042RegW[1] & 0x1f];
        }
      else if(i8042RegW[1]>=0x60 && i8042RegW[1]<0x80) {
        //KBRAM[i8042RegW[1] & 0x1f]
        }
      else if(i8042RegW[1]==0xC0) {
        i8042RegR[0]=KBStatus;
        }
      else if(i8042RegW[1]==0xD0) {
        i8042RegR[0]=KBControl;
        }
      else if(i8042RegW[1]==0xD1) {
        }
      else if(i8042RegW[1]==0xD2) {
        }
      else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
        }
      i=i8042RegR[0];
      break;
    case LCD_BOARD_ADDRESS+7:
      i=i8042RegR[1];
      i=0; // non-busy
      break;
    case LCD_BOARD_ADDRESS+0xe:
      i=0x68; // LCD signature
      break;
		}
#elif NEZ80

/*
LX.382 	scheda CPU 	ram: 0x0000 - 0x03FF eprom: 0x8000 - 0x83FF 	-

LX.383 	interfaccia tastiera esadecimale 	- 	
display (out): 	0xF0 - 0xF7
tastiera (in): 	0xF0
attiva la linea l'NMI dopo l'istruzione successiva.
E' utilizzato per gestire il single-step (out): 	0xF8

LX.385 	interfaccia cassette 	- 	0xEE - 0xEF

LX.386 	espansione di memoria da 8 KBytes 	
indirizzabile a piacimento a blocchi di 1K in base ai ponticelli ed alle ram inserite (oltre l'indirizzo 0x7FFF oppure se utilizzata insieme all'espansione da 32 KBytes occorre effettuare delle modifiche)
	-

LX.392 	espansione di memoria da 32 KBytes 	
indirizzabile a piacimento a blocchi di 16K in base ai ponticelli ed alle ram inserite
	-
LX.389 	interfaccia stampante 	- 	indirizzabile mediante ponticelli sui seguenti indirizzi a scelta:
0x02 - 0x03
0x06 - 0x07
0x0A - 0x0b
0x0E - 0x0F
0x12 - 0x13
0x16 - 0x17
0x1A - 0x1B
0x1E - 0x1F

LX.548 	basic da 16 KBytes su eprom 	eprom: 0x0000 - 0x3FFF 	-

LX.388 	interfaccia video 	ram: 0xEC00 - 0xEDFF 	
tastiera: 	0xEA
ritraccia video: 	0xEB

LX.529 	interfaccia video grafica e stampante 	- 	
PIO 0 porta A - dati (ram 0): 	0x80
PIO 0 porta A - controllo (ram 0): 	0x82
PIO 0 porta B - dati (stampante): 	0x81
PIO 0 porta B - controllo (stampante): 	0x83
PIO 1 porta A - dati (ram 1): 	0x84
PIO 1 porta A - controllo (ram 1): 	0x86
PIO 1 porta B - dati (tastiera): 	0x85
PIO 1 porta B - controllo (tastiera): 	0x87
PIO 2 porta A - dati (ram 2): 	0x88
PIO 2 porta A - controllo (ram 2): 	0x8A
PIO 2 porta B - dati (busy stampante + 40/80 caratteri video): 	0x89
PIO 2 porta B - controllo (busy stampante + 40/80 caratteri video): 	0x8B
SY6545 registro di indirizzo e di stato: 	0x8C
SY6545 registro dati: 	0x8D
RAM 3 attributi dei caratteri: 	0x8E
beeper: 	0x8F

LX.390 	interfaccia floppy 	eprom: 0xF000 - 0xF3FF 	
registro di comando (o di stato se ci si accede in lettura): 	0xD0
registro di traccia: 	0xD1
registro di settore: 	0xD2
data register (scrive solo se il controller è libero): 	0xD3
drive select e side one select: 	0xD6
data register (scrive sempre): 	0xD7

LX.394-395 	programmatore di eprom 	eprom da programmare: 0x9000 - 0x9FFF
eprom con firmware: 0x8400 - 0x87FF 	abilitazione programmazione eprom: 0x7F

LX.683 	interfaccia hard-disk 	eprom su scheda floppy LX.390: 0xF000 - 0xF7FF 	0xB8 - 0xB9 - 0xBA - 0xBB

*/
 
  switch(t & 0xff) {
    case 0xee:			// interfaccia cassette
    case 0xef:
      break;
    case 0xf0:    
      //finire
//      j=0;
//      Keyboard[0] = 0x15 | 0x80;
//			i=(MUXcnt & 8) << 4;
      if(MUXcnt & 8) {
        i = Keyboard[0];
      if(i & 128) {
        static int T;
        T=0;
        }
        }
      else {
//				if(!isKeyPressed) {
					Keyboard[0] &= 0x7f;
//					}
				i = Keyboard[0];
        }
//      i=sense50Hz;
      break;
		}
#elif GALAKSIJA
#elif MSX
//https://www.msx.org/wiki/I/O_Ports_List
//http://map.grauw.nl/resources/msx_io_ports.php
  switch(t & 0xff) {
    case 0x80: // 8251, RS232 data
//      i=ReadUART1();
      i8251RegR[0]=i;
      break;
    case 0x80+1: // RS232 control
      i=i8251RegR[1];
      break;
    case 0x80+2: // RS232 IRQ
      i=i8251RegR[2];
      break;
    case 0x80+3: // n/a
//      i=i8251RegR[3];
      break;
    case 0x84: // counter 0, timer 8253
      i=i8253RegR[0];
      break;
    case 0x84+1: // counter 1
      i=i8253RegR[1];
      break;
    case 0x84+2: // counter 2
      i=i8253RegR[2];
      break;
    case 0x84+3: // 8253 mode
      i=i8253RegR[3];
      break;
    case 0x90: // printer b1=busy read, b0=strobe output
      i=iPrinter[1];
      break;
    case 0x91: // printer data
      i=iPrinter[0];
      break;
    case TMS99xx_BASE:
      TMS9918WriteStage=0;
      i=TMS9918Buffer;
      TMS9918Buffer=TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
      break;
    case TMS99xx_BASE+1:
      i=TMS9918RegS;
      TMS9918RegS &= 0x7f;
      TMS9918WriteStage=0;
      break;
    case TMS99xx_BASE+2:    // Palette access port (only v9938/v9958)
        TMS9918WriteStage=0;
      break;
    case TMS99xx_BASE+3:    // Indirect register access port (only v9938/v9958)
        TMS9918WriteStage=0;
      break;
    case 0xa0:      // sound PSG AY3
      i=AY38910RegSel;
      break;
    case 0xa0+1:      // sound write
      break;
    case 0xa0+2:      // sound read
      switch(AY38910RegSel) {
        case 14:    // joystick
					if(AY38910RegR[7] & B8(01000000))		// if output BUT ONLY INPUT!
						i=AY38910RegW[14];
					else
						i=AY38910RegR[14];
					break;
        case 15:
					if(AY38910RegR[7] & B8(10000000))		// if output ALWAYS OUTPUT!!
						i=AY38910RegW[15];
					else
						i=AY38910RegR[15];
					break;
        default:
          i=AY38910RegR[AY38910RegSel];
          break;
        }
      break;
#ifdef MSXTURBO
    case 0xa7:      // wait
      break;
#endif
    case 0xa8:      // slot-select, 8255
			if(i8255RegW[3] & B8(00010000))
        i=i8255RegR[0];
      else
        i=i8255RegW[0];
      break;
    case 0xa8+1:      // keyboard, 8255
			if(i8255RegW[3] & B8(00000010)) {
  			i8255RegR[1] = Keyboard[i8255RegW[2] & 0xf /* OCCHIO 11!*/];
    		i=i8255RegR[1];
        }
      else
        i=i8255RegW[1];
      break;
    case 0xa8+2:      // keyboard scan & cassette ecc, 8255
			if(i8255RegW[3] & B8(00000001))
        i=i8255RegR[2] & 0xf;
      else
        i=i8255RegW[2] & 0xf;
			if(i8255RegW[3] & B8(00001000))
        i |= i8255RegR[2] & 0xf0;
      else
        i |= i8255RegW[2] & 0xf0;
      break;
    case 0xa8+3:      // 8255
			i=i8255RegR[3];
      break;
    case 0xb4:      // calendar clock
      break;
    case 0xb8:      // light pen
      break;
    case 0xc0:      // music
//      i=AY38910Reg[t & 0xf];
      break;
    case 0xc1:      // music
//      i=AY38910Reg[t & 0xf];
      break;
#ifdef MSXTURBO
//This timer is present only on the MSX turbo R      
    case 0xe6:      // timer (S1990) LSB
      i=LOBYTE(TIMEr);
      break;
    case 0xe7:      // timer (S1990) MSB
      i=HIBYTE(TIMEr);
      break;
#endif
    case 0xf5:      // system control
      break;
    case 0xf7:      // A/V control
      break;
    }
#endif
#ifdef MSX2
    case 0xfc:      // Memory Mapper
      // (not reliable to read back...)
      break;
    case 0xfd:      // Memory Mapper
      break;
    case 0xfe:      // Memory Mapper
      break;
    case 0xff:      // Memory Mapper
      break;
#endif
#endif

	return i;
	}

SWORD _fastcall GetIntValue(SWORD t) {
	register SWORD i;

#ifdef DEBUG_TESTSUITE
	i=MAKEWORD(ram_seg[t],ram_seg[t+1]);
#else

#ifdef MSX
  switch(t & B16(11000000,00000000)) {
    case 0<<14:
      switch(i8255RegW[0] & B8(00000011)) {      // gestire slot, 
        case 0:
      		i=MAKEWORD(rom_seg[t],rom_seg[t+1]);
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
        		i=MAKEWORD(ram_seg2[t-RAM_START2],ram_seg2[t-RAM_START2+1]);
          else
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
#endif
        default:
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
        }
      break;
    case 1<<14:
      switch(i8255RegW[0] & B8(00001100)) {      // gestire slot, 
        case 0:
      		i=MAKEWORD(rom_seg[t],rom_seg[t+1]);
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
        		i=MAKEWORD(ram_seg2[t-RAM_START2],ram_seg2[t-RAM_START2+1]);
          else
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
#endif
        default:
          i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
        }
      break;
    case 2<<14:
      switch(i8255RegW[0] & B8(00110000)) {      // gestire slot, 
        case 0<<4:
          if(t>=RAM_START)
        		i=MAKEWORD(ram_seg[t-RAM_START],ram_seg[t-RAM_START+1]);
          else
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
        		i=MAKEWORD(ram_seg2[t-RAM_START2],ram_seg2[t-RAM_START2+1]);
          else
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
#endif
        default:
          i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
        }
      break;
    case 3<<14:
      switch(i8255RegW[0] & B8(11000000)) {      // gestire slot, 
        case 0<<6:
          if(t>=RAM_START)
        		i=MAKEWORD(ram_seg[t-RAM_START],ram_seg[t-RAM_START+1]);
          else
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
#ifdef RAM_SIZE2 
        case 2<<6:
          if(t>=RAM_START2)
        		i=MAKEWORD(ram_seg2[t-RAM_START2],ram_seg2[t-RAM_START2+1]);
          else
            i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
#endif
        default:
          i=MAKEWORD(UNIMPLEMENTED_MEMORY_VALUE,UNIMPLEMENTED_MEMORY_VALUE);
          break;
        }
      break;
    }
#ifdef MSX2
gestire 0xffff per subslot... ?? qua
#endif
#else
#ifdef NEZ80
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		t-=RAM_START;
		i=MAKEWORD(ram_seg[t],ram_seg[t+1]);
		}
	else if(t >= ROM_START && t < (ROM_START+ROM_SIZE)) {
		t-=ROM_START;
		i=MAKEWORD(rom_seg[t],rom_seg[t+1]);
		}
#else
#ifdef ZX80
//	t &= 0x7fff;
//  ??
#endif
#ifdef ZX81
	t &= 0x7fff;
//  ??
#endif
	if(t < ROM_SIZE) {			// ZX80, 81, Galaksija, 
		i=MAKEWORD(rom_seg[t],rom_seg[t+1]);
		}
#ifdef SKYNET
	else if((t-0x4000) < ROM_SIZE2) {			// SKYNET
		t-=0x4000;
		i=MAKEWORD(rom_seg2[t],rom_seg2[t+1]);
		}
#elif GALAKSIJA
	else if((t-ROM_SIZE) < ROM_SIZE2) {			// GALAKSIJA
		t-=ROM_SIZE;
		i=MAKEWORD(rom_seg2[t],rom_seg2[t+1]);
		}
#endif
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		t-=RAM_START;
		i=MAKEWORD(ram_seg[t],ram_seg[t+1]);
		}
#ifdef GALAKSIJA
  // serve??
//    if(t<=0x37)
//      i = Keyboard[t & 0x7] & (t >> 3) ? 1 : 0;   // i 3 bit bassi pilotano il mux lettura, i 3 alti il mux scrittura, out è su D0
//    else
//      i = Latch;   // 
#endif
#endif
#endif
#ifndef MSX
	else
    i=UNIMPLEMENTED_MEMORY_VALUE;
#endif
#endif

	return i;
	}

BYTE _fastcall GetPipe(SWORD t) {

#ifdef DEBUG_TESTSUITE
  Pipe1=ram_seg[t++];
  Pipe2.b.l=ram_seg[t++];
//		Pipe2.b.h=ram_seg[t++];
//		Pipe2.b.u=ram_seg[t];
  Pipe2.b.h=ram_seg[t];

#else

#ifdef MSX
  switch(t & B16(11000000,00000000)) {
    case 0<<14:
      switch(i8255RegW[0] & B8(00000011)) {      // gestire slot, 
        case 0:
          Pipe1=rom_seg[t++];
          Pipe2.b.l=rom_seg[t++];
      //		Pipe2.b.h=rom_seg[t++];
      //		Pipe2.b.u=rom_seg[t];
          Pipe2.b.h=rom_seg[t];
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            Pipe1=ram_seg2[t++];
            Pipe2.b.l=ram_seg2[t++];
        //		Pipe2.b.h=ram_seg[t++];
        //		Pipe2.b.u=ram_seg[t];
            Pipe2.b.h=ram_seg2[t];
            }
          else {
            Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
            }
          break;
#endif
        default:
          Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 1<<14:
      switch(i8255RegW[0] & B8(00001100)) {      // gestire slot, 
        case 0:
          Pipe1=rom_seg[t++];
          Pipe2.b.l=rom_seg[t++];
      //		Pipe2.b.h=rom_seg[t++];
      //		Pipe2.b.u=rom_seg[t];
          Pipe2.b.h=rom_seg[t];
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            Pipe1=ram_seg2[t++];
            Pipe2.b.l=ram_seg2[t++];
        //		Pipe2.b.h=ram_seg2[t++];
        //		Pipe2.b.u=ram_seg2[t];
            Pipe2.b.h=ram_seg2[t];
            }
          else {
            Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
            }
          break;
#endif
        default:
          Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 2<<14:
      switch(i8255RegW[0] & B8(00110000)) {      // gestire slot, 
        case 0<<4:
          if(t>=RAM_START) {
            t-=RAM_START;
            Pipe1=ram_seg[t++];
            Pipe2.b.l=ram_seg[t++];
        //		Pipe2.b.h=ram_seg[t++];
        //		Pipe2.b.u=ram_seg[t];
            Pipe2.b.h=ram_seg[t];
            }
          else {
            Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
            }
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            Pipe1=ram_seg2[t++];
            Pipe2.b.l=ram_seg2[t++];
        //		Pipe2.b.h=ram_seg2[t++];
        //		Pipe2.b.u=ram_seg2[t];
            Pipe2.b.h=ram_seg2[t];
            }
          else {
            Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
            }
          break;
#endif
        default:
          Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 3<<14:
      switch(i8255RegW[0] & B8(11000000)) {      // gestire slot, 
        case 0<<6:
          if(t>=RAM_START) {
            t-=RAM_START;
            Pipe1=ram_seg[t++];
            Pipe2.b.l=ram_seg[t++];
        //		Pipe2.b.h=ram_seg[t++];
        //		Pipe2.b.u=ram_seg[t];
            Pipe2.b.h=ram_seg[t];
            }
          else {
            Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
            }
          break;
#ifdef RAM_SIZE2 
        case 2<<6:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            Pipe1=ram_seg2[t++];
            Pipe2.b.l=ram_seg2[t++];
        //		Pipe2.b.h=ram_seg2[t++];
        //		Pipe2.b.u=ram_seg2[t];
            Pipe2.b.h=ram_seg2[t];
            }
          else {
            Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
        //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
            Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
            }
          break;
#endif
        default:
          Pipe1=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.l=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
      //		Pipe2.b.u=UNIMPLEMENTED_MEMORY_VALUE;
          Pipe2.b.h=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    }
#else
#ifdef NEZ80
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		t-=RAM_START;
	  Pipe1=ram_seg[t++];
		Pipe2.b.l=ram_seg[t++];
//		Pipe2.b.h=ram_seg[t++];
//		Pipe2.b.u=ram_seg[t];
		Pipe2.b.h=ram_seg[t];
		}
	else if(t >= ROM_START && t < (ROM_START+ROM_SIZE)) {
		t-=ROM_START;
	  Pipe1=rom_seg[t++];
		Pipe2.b.l=rom_seg[t++];
//		Pipe2.b.h=rom_seg[t++];
//		Pipe2.b.u=rom_seg[t];
		Pipe2.b.h=rom_seg[t];
		}
#else
#ifdef ZX80
//	t &= 0x7fff;
//  sicuro??
#endif
#ifdef ZX81
//	t &= 0x7fff;
//  sicuro??
#endif
	if(t < ROM_SIZE) {			// ZX80, 81, Galaksija
	  Pipe1=rom_seg[t++];
		Pipe2.b.l=rom_seg[t++];
//		Pipe2.b.h=rom_seg[t++];
//		Pipe2.b.u=rom_seg[t];
		Pipe2.b.h=rom_seg[t];
		}
#ifdef SKYNET
	else if((t-0x4000) < ROM_SIZE2) {			// SKYNET
		t-=0x4000;
	  Pipe1=rom_seg2[t++];
		Pipe2.b.l=rom_seg2[t++];
//		Pipe2.b.h=rom_seg2[t++];
//		Pipe2.b.u=rom_seg2[t];
		Pipe2.b.h=rom_seg2[t];
		}
#endif
#ifdef GALAKSIJA
	else if((t-0x1000) < ROM_SIZE2) {			// GALAKSIJA
		t-=0x1000;
	  Pipe1=rom_seg2[t++];
		Pipe2.b.l=rom_seg2[t++];
//		Pipe2.b.h=rom_seg2[t++];
//		Pipe2.b.u=rom_seg2[t];
		Pipe2.b.h=rom_seg2[t];
		}
#endif
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		t-=RAM_START;
	  Pipe1=ram_seg[t++];
		Pipe2.b.l=ram_seg[t++];
//		Pipe2.b.h=ram_seg[t++];
//		Pipe2.b.u=ram_seg[t];
		Pipe2.b.h=ram_seg[t];
		}
#endif
#endif
#endif

	return Pipe1;
	}

void _fastcall PutValue(SWORD t,BYTE t1) {
	register SWORD i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);
#ifdef DEBUG_TESTSUITE

	ram_seg[t]=t1;
#else

#ifdef MSX
  switch(t & B16(11000000,00000000)) {
    case 0<<14:
      switch(i8255RegW[0] & B8(00000011)) {      // gestire slot, 
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
            ram_seg2[t-RAM_START2]=t1;
          else
            ;
          break;
#endif
        default:
          ;
          break;
        }
      break;
    case 1<<14:
      switch(i8255RegW[0] & B8(00001100)) {      // gestire slot, 
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
            ram_seg2[t-RAM_START2]=t1;
          else
            ;
          break;
#endif
        default:
          ;
          break;
        }
      break;
    case 2<<14:
      switch(i8255RegW[0] & B8(00110000)) {      // gestire slot, 
        case 0<<4:
          if(t>=RAM_START)
            ram_seg[t-RAM_START]=t1;
          else
            ;
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2)
            ram_seg2[t-RAM_START2]=t1;
          else
            ;
          break;
#endif
        default:
          i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 3<<14:
      switch(i8255RegW[0] & B8(11000000)) {      // gestire slot, 
        case 0<<6:
          if(t>=RAM_START)
            ram_seg[t-RAM_START]=t1;
          else
            ;
          break;
#ifdef RAM_SIZE2 
        case 2<<6:
          if(t>=RAM_START2)
            ram_seg2[t-RAM_START2]=t1;
          else
            ;
          break;
#endif
        default:
          ;
          break;
        }
      break;
    }
#ifdef MSX2
gestire 0xffff per subslot...
#endif
#else
#ifdef NEZ80
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
	  ram_seg[t-RAM_START]=t1;
		}
#else
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {		// ZX80,81,Galaksija
	  ram_seg[t-RAM_START]=t1;
		}
#ifdef ZX80
	else {
		}
#elif ZX81
	else {
		}
#elif SKYNET
  else if(t==0xe000) {        //
    IOPortO=t1;      // b5 è speaker...
//    LATDbits.LATD1= t1 & B8(00100000) ? 1 : 0;
//    LATEbits.LATE2= t1 & B8(10000000) ? 1 : 0;  // led, fare se si vuole
//    LATEbits.LATE3= t1 & B8(01000000) ? 1 : 0;
// finire volendo...    if(IOPortO & 0x1)
//      IOPortI &= ~0x5;
    }
  else if(t>=0xe002 && t<=0xe003) {        //   CMOS RAM/RTC (Real Time Clock  MC146818)
    t &= 0x1;
    switch(t) {
      case 0:
        i146818RegR[0]=i146818RegW[0]=t1;
//        time_t;
        break;
      case 1:     // il bit 7 attiva/disattiva NMI
        i146818RegW[t]=t1;
        switch(i146818RegW[0] & 0x3f) {
          case 0:
//            currentTime.sec=t1;
            break;
            // in mezzo c'è Alarm...
          case 2:
//            currentTime.min=t1;;
            break;
          case 4:
//            currentTime.hour=t1;
            break;
          // 6 è day of week...
          case 7:
//            currentDate.mday=t1;
            break;
          case 8:
//            currentDate.mon=t1;
            break;
          case 9:
//            currentDate.year=t1;
            break;
          case 10:
            t1 &= 0x7f;     // vero hardware!
            t1 |= i146818RAM[10] & 0x80;
            goto writeRegRTC;
            break;
          case 11:
            if(t1 & 0x80)
              i146818RAM[10] &= 0x7f;
            goto writeRegRTC;
            break;
          case 12:
            t1 &= 0xf0;     // vero hardware!
            goto writeRegRTC;
            break;
          case 13:
            t1 &= 0x80;     // vero hardware!
            goto writeRegRTC;
            break;
          default:      // in effetti ci sono altri 4 registri... RAM da 14 in poi 
writeRegRTC:            
            i146818RAM[i146818RegW[0] & 0x3f] = t1;
            break;
          }
        break;
      }
		}
  else if(t==0xe004) {
    ClIRQPort;
    }
  else if(t==0xe005) {
		WDCnt=MAX_WATCHDOG;
    ClWDPort;
    }
#elif GALAKSIJA
  else if((t >= 0x2000) && (t <= 0x27ff)) {
    t &= 0x3f;
    if(t<=0x37)
      ; //non dovrebbe servirmi...
    else
      Latch=t1;   // 
    }
#endif
#endif
#endif
#endif

	}

void _fastcall PutIntValue(SWORD t,SWORD t1) {
	register SWORD i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);
#ifdef DEBUG_TESTSUITE

	ram_seg[t++]=LOBYTE(t1);
	ram_seg[t]=HIBYTE(t1);
#else

#ifdef MSX
  switch(t & B16(11000000,00000000)) {
    case 0<<14:
      switch(i8255RegW[0] & B8(00000011)) {      // gestire slot, 
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START) {
            t-=RAM_START2;
            ram_seg2[t++]=LOBYTE(t1);
            ram_seg2[t]=HIBYTE(t1);
            }
          else
            ;
          break;
#endif
        default:
          ;
          break;
        }
      break;
    case 1<<14:
      switch(i8255RegW[0] & B8(00001100)) {      // gestire slot, 
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            ram_seg2[t++]=LOBYTE(t1);
            ram_seg2[t]=HIBYTE(t1);
            }
          else
            ;
          break;
#endif
        default:
          ;
          break;
        }
      break;
    case 2<<14:
      switch(i8255RegW[0] & B8(00110000)) {      // gestire slot, 
        case 0<<4:
          if(t>=RAM_START) {
            t-=RAM_START;
            ram_seg[t++]=LOBYTE(t1);
            ram_seg[t]=HIBYTE(t1);
            }
          else
            ;
          break;
#ifdef RAM_SIZE2 
        case 2<<4:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            ram_seg2[t++]=LOBYTE(t1);
            ram_seg2[t]=HIBYTE(t1);
            }
          else
            ;
          break;
#endif
        default:
          i=UNIMPLEMENTED_MEMORY_VALUE;
          break;
        }
      break;
    case 3<<14:
      switch(i8255RegW[0] & B8(11000000)) {      // gestire slot, 
        case 0<<6:
          if(t>=RAM_START) {
            t-=RAM_START;
            ram_seg[t++]=LOBYTE(t1);
            ram_seg[t]=HIBYTE(t1);
            }
          else
            ;
          break;
#ifdef RAM_SIZE2 
        case 2<<6:
          if(t>=RAM_START2) {
            t-=RAM_START2;
            ram_seg2[t++]=LOBYTE(t1);
            ram_seg2[t]=HIBYTE(t1);
            }
          else
            ;
          break;
#endif
        default:
          ;
          break;
        }
      break;
    }
#ifdef MSX2
gestire 0xffff per subslot... qua?
#endif
#else
#ifdef NEZ80
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {
		t-=RAM_START;
	  ram_seg[t++]=LOBYTE(t1);
	  ram_seg[t]=HIBYTE(t1);
		}
#else
	if(t >= RAM_START && t < (RAM_START+RAM_SIZE)) {		// ZX80,81,Galaksija
		t-=RAM_START;
	  ram_seg[t++]=LOBYTE(t1);
	  ram_seg[t]=HIBYTE(t1);
		}
#endif
#endif
#endif
	}

void _fastcall OutValue(SWORD t,BYTE t1) {   // 
	register SWORD i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

#ifdef DEBUG_TESTSUITE

#else

#ifdef ZX80
/*  Output to Port FFh (or ANY other port)
Writing any data to any port terminates the Vertical Retrace period, and
restarts the LINECNTR counter. The retrace signal is also output to the
cassette (ie. the Cassette Output becomes High)*/
  switch(t) {     // OCCHIO byte alto! qua è così VERIFICARE out
    default:
      lineCntr=0;
      break;
    }
#elif ZX81
/*Port FDh Write (ZX81 only)
Writing any data to this port disables the NMI generator.
Port FEh Write (ZX81 only)
Writing any data to this port enables the NMI generator.
NMIs (Non maskable interrupts) are used during SLOW mode vertical blanking
periods to count the number of drawn blank scanlines.
   */
  switch(t) {     // OCCHIO byte alto! qua è così VERIFICARE out
    case 0xfd:
      NMIGenerator=0;
      break;
    case 0xfe:
      NMIGenerator=1;
      break;
    }
#elif SKYNET
  switch(t & 0xff) {
    case IO_BOARD_ADDRESS:
    case IO_BOARD_ADDRESS+1:
    case IO_BOARD_ADDRESS+2:
    case IO_BOARD_ADDRESS+3:
      IOExtPortO[t-IO_BOARD_ADDRESS]=t1;
      break;

    case LCD_BOARD_ADDRESS:
      // per motivi che ora non ricordo, il BIOS indirizza 0..3 mentre la 2°EEprom (casanet) aveva indirizzi doppi o meglio bit 0..1 messi a 2-0
      // potrei ricompilare casanet per andare "dritto" 0..3, per ora lascio così (unico problema è conflitto a +6 con la tastiera... amen! tanto scrive solo all'inizio)
      i8255RegW[0]=t1;
      break;
    case LCD_BOARD_ADDRESS+2:
      if(i8255RegW[1] & 4 && !(t1 & 4)) {   // quando E scende
        // in teoria dovremmo salvare in R[0] solo in questo istante, la lettura... ma ok! (v. sopra)
        if(i8255RegW[1] & 1)	{		// se dati
          if(!LCDCGDARAM) {
            LCDram[LCDptr]=i8255RegW[0];
            if(LCDentry & 2) {
              LCDptr++;
              if(LCDptr >= 0x7f)			// parametrizzare! o forse no, fisso max 127
                LCDptr=0;
              }
            else {
              LCDptr--;
              if(LCDptr < 0)			// parametrizzare!
                LCDptr=0x7f;    // CONTROLLARE
              }
            }
          else {
            LCDCGram[LCDCGptr++]=i8255RegW[0];
            LCDCGptr &= 0x3f;
            }
          }
        else {									// se comandi https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
          if(i8255RegW[0] <= 1) {			// CLS/home
            LCDptr=0;
						LCDentry |= 2;		// ripristino INCremento, dice
            memset(LCDram,' ',sizeof(LCDram) /* 4*40 */);
            }
          else if(i8255RegW[0] <= 3) {			// home
            LCDptr=0;
            }
          else if(i8255RegW[0] <= 7) {			// entry mode ecc
            LCDentry=i8255RegW[0] & 3;
            }
          else if(i8255RegW[0] <= 15) {			// display on-off ecc
            LCDdisplay=i8255RegW[0] & 7;
            }
          else if(i8255RegW[0] <= 31) {			// cursor set, increment & shift off
            LCDcursor=i8255RegW[0] & 15;
            }
          else if(i8255RegW[0] <= 63) {			// function set, 2-4 linee
            LCDfunction=i8255RegW[0] & 31;
            }
          else if(i8255RegW[0] <= 127) {			// CG RAM addr set
            LCDCGptr=i8255RegW[0] & 0x3f;			// 
            LCDCGDARAM=1;// user defined char da 0x40
            }
          else {				// >0x80 = cursor set
            LCDptr=i8255RegW[0] & 0x7f;			// PARAMETRIZZARE e gestire 2 Enable x 4x40!
            LCDCGDARAM=0;
            }
          }
        }
      i8255RegW[1]=i8255RegR[1]=t1;
      break;
    case LCD_BOARD_ADDRESS+4:
      // qua c'è la 3° porta del 8255
      // il b6 mette a in o out la porta dati A (1=input)
      i8255RegW[2]=i8255RegR[2]=t1;
      break;
    case LCD_BOARD_ADDRESS+5 /* sarebbe 6 ma ovviamente non si può! v. sopra*/:
      // 8255 settings
      i8255RegW[3]=i8255RegR[3]=t1;
      if(i8255RegW[3] & 0x80) {   //2024 verificare, v.MSX
        i8255RegR[3]=i8255RegW[3];
        }
      else {
        if(i8255RegW[3] & 1)
          i8255RegW[3] |= (1 << ((i8255RegW[3] >> 1) & 7));
        else
          i8255RegW[3] &= ~(1 << ((i8255RegW[3] >> 1) & 7));
        }
      break;

    case LCD_BOARD_ADDRESS+6:   
      i8042RegR[0]=i8042RegW[0]=t1;
      if(i8042RegW[1]==0xAA) {
        }
      else if(i8042RegW[1]>=0x20 && i8042RegW[1]<0x40) {
        //KBRAM[i8042RegW[1] & 0x1f]
        }
      else if(i8042RegW[1]>=0x60 && i8042RegW[1]<0x80) {
        KBRAM[i8042RegW[1] & 0x1f]=t1;
        KBRAM[0] &= 0x7f;     // dice...
        }
      else if(i8042RegW[1]==0xC0) {
        }
      else if(i8042RegW[1]==0xD0) {
        }
      else if(i8042RegW[1]==0xD1) {
        KBControl=t1;
        }
      else if(i8042RegW[1]==0xD2) {
        Keyboard[0]=t1;
        }
      else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
        }
      break;
//    case 7:     // keyboard...
//#warning togliere!
    case LCD_BOARD_ADDRESS+7:
      i8042RegR[1]=i8042RegW[1]=t1;
      if(i8042RegW[1]==0xAA) {
        }
      else if(i8042RegW[1]>=0x20 && i8042RegW[1]<0x40) {
        //KBRAM[i8042RegW[1] & 0x1f]
        }
      else if(i8042RegW[1]>=0x60 && i8042RegW[1]<0x80) {
        //KBRAM[i8042RegW[1] & 0x1f]
        }
      else if(i8042RegW[1]==0xC0) {
        }
      else if(i8042RegW[1]==0xD0) {
        }
      else if(i8042RegW[1]==0xD1) {
        }
      else if(i8042RegW[1]==0xD2) {
        }
      else if(i8042RegW[1]>=0xF0 && i8042RegW[1]<=0xFF) {
        }
      break;
		}
#elif NEZ80
  switch(t & 0xff) {
    case 0xee:			// interfaccia cassette
    case 0xef:
      break;
    case 0xf0:
    case 0xf1:
    case 0xf2:
    case 0xf3:
    case 0xf4:
    case 0xf5:
    case 0xf6:
    case 0xf7:
      i = t & 0x7;
      DisplayRAM[i]=t1;
      PlotDisplay(7-i,t1,1);
//	InvalidateRect(ghWnd,NULL,TRUE);
      break;
		}
#elif GALAKSIJA
  CPUPins |= DoWait;
#elif MSX
//https://www.msx.org/wiki/I/O_Ports_List
  switch(t & 0xff) {
    case 0x59: // usato nel sorgente... boh??
      break;
    case 0x80: // 8251, RS232 data
//      WriteUART1(t1);
      i8251RegW[0]=t1;
      break;
    case 0x80+1: // RS232 control
      i8251RegW[1]=t1;
      break;
    case 0x80+2: // RS232, IRQ
      i8251RegW[2]=t1;
      break;
    case 0x80+3: // n/a
//      i8251RegW[3]=t1;
      break;
    case 0x80+4: // counter 0, timer 8253
      i8253RegW[0]=i8253RegR[0]=t1;
//UART_Init() rx
      break;
    case 0x80+5: // counter 1
      i8253RegW[1]=i8253RegR[1]=t1;
//UART_Init() tx
      break;
    case 0x80+6: // counter 2
      i8253RegW[2]=i8253RegR[2]=t1;
      break;
    case 0x80+7: // 8253 mode
      i8253RegW[3]=t1;
      break;
    case 0x90: // printer b1=busy read, b0=strobe output
      iPrinter[1] &= B8(00000010);
      iPrinter[1] |= t1 & 1;
      break;
    case 0x91: // printer data
      iPrinter[0]=t1;
      break;
    case TMS99xx_BASE:
      TMS9918WriteStage=0;
      TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)]=t1;
//      TMS9918Buffer = t1;
      break;
    case TMS99xx_BASE+1:
      if(!TMS9918WriteStage) {   /* first stage byte - either an address LSB or a register value */
        TMS9918Sel = t1;
        TMS9918WriteStage = 1;
        }
      else {    /* second byte - either a register number or an address MSB */
        if(t1 & 0x80) { /* register */
//          if((t1 & 0x7f) < 8)
            TMS9918Reg[t1 & 0x07] = TMS9918Sel;
          }
        else {  /* address */
          TMS9918RAMPtr = TMS9918Sel | ((t1 & 0x3f) << 8);
          if(!(t1 & 0x40)) {
            TMS9918Buffer = TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
            }
          }
        TMS9918WriteStage = 0;
        } 
      break;
    case TMS99xx_BASE+2:    // Palette access port (only v9938/v9958)
        TMS9918WriteStage=0;
      break;
    case TMS99xx_BASE+3:    // Indirect register access port (only v9938/v9958)
        TMS9918WriteStage=0;
      break;
    case 0xa0:      // sound PSG AY3
      AY38910RegSel=t1 & 0xf;
      break;
    case 0xa0+1:      // sound write
      AY38910RegW[AY38910RegSel]=t1;
      switch(AY38910RegSel) {
        case 0:
        case 1:
          i=MAKEWORD(AY38910RegW[0],AY38910RegW[1]);      // ???Hz 6/24  https://www.msx.org/wiki/SOUND
          i *= 3.5; //fatt correzione 4/7/24
//          PR2 = i;		 // 
//          OC1RS = i/2;		 // 
        case 7:
set_ch0:
          if(!(AY38910RegW[7] & 1) && (AY38910RegW[8] & 15)) {
//            OC1CONbits.ON = 1;   // on
            }
          else {
//            OC1CONbits.ON = 0;   // off
            }
					AY38910RegR[AY38910RegSel]=AY38910RegW[AY38910RegSel];
          break;
        case 8:    // amplitude
          goto set_ch0;
          break;
        case 9:
        case 10:
					AY38910RegR[AY38910RegSel]=AY38910RegW[AY38910RegSel];
          break;
        case 14:    // joystick
					if(AY38910RegR[7] & B8(01000000))		// if output BUT ONLY INPUT!
						AY38910RegR[14]=AY38910RegW[14];
					break;
        case 15:
					if(AY38910RegR[7] & B8(10000000))		// if output ALWAYS OUTPUT!!
						AY38910RegR[15]=AY38910RegW[15];
					break;
        default:
					AY38910RegR[AY38910RegSel]=AY38910RegW[AY38910RegSel];
          break;
        }
      break;
    case 0xa0+2:      // sound read
      break;
#ifdef MSXTURBO
    case 0xa7:      // wait
      break;
#endif
    case 0xa8:      // 8255 slot-select
      i8255RegW[0]=i8255RegR[0]=t1;
      break;
    case 0xa8+1:      // keyboard data, 8255
      i8255RegW[1]=/*i8255RegR[1]=*/t1;
      break;
    case 0xa8+2:      // keyboard scan & cassette ecc, 8255
      i8255RegW[2]=t1;
      i8255RegR[2]=i8255RegW[2];
      if(i8255RegW[2] & 0x80) {   // key click (SW BUZZER)!! NON va ma boh...
        int pwm1,pwm2,pwmon;
#ifdef ST7735
        pwm1 = PR2;
        pwm2 = OC1RS;
        pwmon=OC1CONbits.ON;
        PR2 = 60;		 // 2KHz
        OC1RS = 60/2;		 // 
        OC1CONbits.ON = 1;   // on
        __delay_ms(20);
        OC1CONbits.ON = 0;   // off
        PR2 = pwm1;		 // 
        OC1RS = pwm2;		 // 
        OC1CONbits.ON=pwmon;
#endif
#ifdef ILI9341
        pwm1 = PR2;
        pwm2 = OC7RS;
        pwmon=OC7CONbits.ON;
        PR2 = 60;		 // 2KHz
        OC7RS = 60/2;		 // 
        OC7CONbits.ON = 1;   // on
        __delay_ms(20);
        OC7CONbits.ON = 0;   // off
        PR2 = pwm1;		 // 
        OC7RS = pwm2;		 // 
        OC7CONbits.ON=pwmon;
#endif
        }
      else {
#ifdef ST7735
//        OC1CONbits.ON = 0;   // off
#endif
#ifdef ILI9341
//        OC7CONbits.ON = 0;   // off
#endif
        }
      // opp accendere se 1 e spegnere se 0...
      break;
    case 0xa8+3:      // 8255
      i8255RegW[3]=t1;
      if(i8255RegW[3] & 0x80) {
        i8255RegR[3]=i8255RegW[3];
        }
      else {
        if(i8255RegW[3] & 1)
          i8255RegW[3] |= (1 << ((i8255RegW[3] >> 1) & 7));
        else
          i8255RegW[3] &= ~(1 << ((i8255RegW[3] >> 1) & 7));
        }
      break;
    case 0xb4:      // calendar clock
      break;
    case 0xb8:      // light pen
      break;
    case 0xc0:      // music
//      AY38910Reg[t & 0xf]=t1;
      break;
    case 0xc1:      // music
//      AY38910Reg[t & 0xf]=t1;
      break;
#ifdef MSXTURBO
//This timer is present only on the MSX turbo R      
    case 0xe6:      // timer (S1990) LSB
//      LOBYTE(TIMEr);
      break;
    case 0xe7:      // timer (S1990) MSB
//      HIBYTE(TIMEr);
      break;
#endif
    case 0xf5:      // system control
      break;
    case 0xf7:      // A/V control
      break;
    }
#endif
#ifdef MSX2
    case 0xfc:      // Memory Mapper
      break;
    case 0xfd:      // Memory Mapper
      break;
    case 0xfe:      // Memory Mapper
      break;
    case 0xff:      // Memory Mapper
      break;
#endif
#endif
	}


extern const unsigned char charset_international[2048],tmsFont[(128-32)*8];
void initHW(void) {
  struct SPRITE_ATTR *sa;
  int i;
  
#ifdef SKYNET 
  IOPortI=B8(10111111);   // dip=1111; puls=1; Comx=1
  memset(i8255RegR,0,sizeof(i8255RegR));
  memset(i8255RegW,0,sizeof(i8255RegW));
  memset(LCDram,0,sizeof(LCDram));
  memset(LCDCGram,0,sizeof(LCDCGram));
	LCDCGDARAM=0;
	LCDCGptr=0;
	LCDfunction=0;
	LCDdisplay=0;
	LCDentry=2;
	LCDcursor=0;
#endif
#ifdef NEZ80 
#endif
#ifdef GALAKSIJA
	Latch=0;
#endif
#ifdef ZX80
#endif
#ifdef ZX81
NMIGenerator=0;  
#endif
#ifdef MSX
  i8251RegW[2]=B8(00001111);    // IRQ mask stato iniziale
  i8255RegW[0]=i8255RegR[0]=0;
  i8255RegW[1]=i8255RegR[1]=0;
  i8255RegW[2]=i8255RegR[2]=0;
  i8255RegW[3]=i8255RegR[3]=0;
  iPrinter[1]=0;
	AY38910RegR[14]=AY38910RegR[15]=B8(11111111);
  AY38910RegSel=0;
  memset(TMSVideoRAM,0,TMSVIDEORAM_SIZE);    // mah...
// Ti99 dice:	The real 9918A will set all VRs to 0, which basically makes the screen black, blank, and off, 4K VRAM selected, and no interrupts. 
  TMS9918Reg[0]=TMS_R0_EXT_VDP_DISABLE | TMS_R0_MODE_GRAPHICS_I;
  TMS9918Reg[1]=TMS_R1_RAM_16K | TMS_R1_MODE_GRAPHICS_I /* bah   | TMS_R1_DISP_ACTIVE | TMS_R1_INT_ENABLE*/;
  TMS9918Reg[2]=TMS_DEFAULT_VRAM_NAME_ADDRESS >> 10;
  TMS9918Reg[3]=TMS_DEFAULT_VRAM_COLOR_ADDRESS >> 6;
  TMS9918Reg[4]=TMS_DEFAULT_VRAM_PATT_ADDRESS >> 11;
  TMS9918Reg[5]=TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS >> 7;
  TMS9918Reg[6]=TMS_DEFAULT_VRAM_SPRITE_PATT_ADDRESS >> 11;
  TMS9918Reg[7]=(1 /*black*/ << 4) | 7 /*cyan*/;
  TMS9918RegS=0;
  TMS9918Sel=TMS9918WriteStage=0;
  memcpy(TMSVideoRAM+TMS_DEFAULT_VRAM_PATT_ADDRESS,charset_international,2048);// mah...
//  memcpy(TMSVideoRAM+TMS_DEFAULT_VRAM_PATT_ADDRESS,tmsFont,(128-32)*8);
  sa=(struct SPRITE_ATTR *)&TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS];
  for(i=0; i<32; i++) {
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4]=LAST_SPRITE_YPOS;
    sa->ypos=LAST_SPRITE_YPOS;
    sa->xpos=sa->tag=sa->name=0;
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4+1]=0;
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4+2]=0;
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4+3]=0;
    sa++;
    }

  for(i=0; i<768; i++)
    TMSVideoRAM[TMS_DEFAULT_VRAM_NAME_ADDRESS+i]=i & 0xff;
#endif

  Keyboard[0]=Keyboard[1]=Keyboard[2]=Keyboard[3]=Keyboard[4]=Keyboard[5]=Keyboard[6]=Keyboard[7]=255;
  
	TIMIRQ=0;

//  OC1CONbits.ON = 0;   // spengo buzzer/audio

  }

