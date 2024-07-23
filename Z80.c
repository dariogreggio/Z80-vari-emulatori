//http://z80-heaven.wikidot.com/instructions-set
//https://gist.github.com/seanjensengrey/f971c20d05d4d0efc0781f2f3c0353da
//#warning CFR Z80HW per modifiche a gestione flag... 2022

//v. metodo usato su 68000 senza byte alto... provare
// la cosa di inEI... chiss�... e anche in 8086

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include <graph.h>
//#include <dos.h>
//#include <malloc.h>
//#include <memory.h>
//#include <fcntl.h>
//#include <io.h>
#include <xc.h>


#include "Z80_PIC.h"
#include "Adafruit_ST77xx.h"      // per LOBYTE ecc...


#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )

#undef Z80_EXTENDED

extern BYTE fExit;
extern BYTE debug;

extern volatile BYTE keysFeedPtr;
extern volatile BYTE TIMIRQ,VIDIRQ;

BYTE CPUPins=DoReset;
#define MAX_WATCHDOG 100      // x30mS v. sotto
WORD WDCnt=MAX_WATCHDOG;
BYTE ColdReset=1;
BYTE Pipe1;
union __attribute__((__packed__)) {
	SWORD x;
	BYTE bb[4];
	struct __attribute__((__packed__)) {
		BYTE l;
		BYTE h;
//		BYTE u;		 bah no, sposto la pipe quando ci sono le istruzioni lunghe 4...
		} b;
	} Pipe2;

union __attribute__((__packed__)) Z_REG {
  SWORD x;
  struct __attribute__((__packed__)) { 
    BYTE l;
    BYTE h;
    } b;
//    } _bc1,_de1,_hl1,_af1,_af2,_bc2,_de2,_hl2;
  };
union __attribute__((__packed__)) Z_REGISTERS {
  BYTE  b[8];
  union Z_REG r[4];
  };

#define ID_CARRY 0x1
#define ID_ADDSUB 0x2
#define ID_PV 0x4
#define ID_HALFCARRY 0x10
#define ID_ZERO 0x40
#define ID_SIGN 0x80
union __attribute__((__packed__)) REGISTRO_F {
  BYTE b;
  struct __attribute__((__packed__)) {
    unsigned int Carry: 1;
    unsigned int AddSub: 1;
    unsigned int PV: 1;   // 1=pari 0=dispari
    unsigned int unused: 1;
    unsigned int HalfCarry: 1;
    unsigned int unused2: 1;
    unsigned int Zero: 1;
    unsigned int Sign: 1;
    };
  };
union __attribute__((__packed__)) OPERAND {
  BYTE *reg8;
  WORD *reg16;
  WORD mem;
  };
union __attribute__((__packed__)) RESULT {
  struct __attribute__((__packed__)) {
    BYTE l;
    BYTE h;
    } b;
  WORD x;
  DWORD d;
  };
    
// http://clrhome.org/table/
// https://wikiti.brandonw.net/index.php?title=Z80_Instruction_Set
int Emulate(int mode) {
/*Registers (http://www.z80.info/z80syntx.htm#LD)
--------------
 A = 111
 B = 000
 C = 001
 D = 010
 E = 011
 H = 100
 L = 101*/
#define _a regs1.r[3].b.l //
	// in TEORIA, REGISTRO_F dovrebbe appartenere qua... ho patchato pop af push ecc, SISTEMARE!! _f.b
  // n.b. anche che A/F sono invertiti rispetto agli altri, v. push/pop
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
#define WORKING_REG regs1.b[((Pipe1 & 0x38) ^ 8) >> 3]      // la parte bassa/alta � invertita...
#define WORKING_REG2 regs1.b[(Pipe1 ^ 1) & 7]
//#define WORKING_REG_CB regs1.b[((Pipe2.b.l & 0x38) ^ 8) >> 3]
#define WORKING_REG_CB regs1.b[(Pipe2.b.l ^ 1) & 7]
#define WORKING_BITPOS (1 << ((Pipe2.b.l & 0x38) >> 3))
#define WORKING_BITPOS2 (1 << ((Pipe2.b.h & 0x38) >> 3))
    
	SWORD _pc=0;
	SWORD _ix=0;
#ifdef Z80_EXTENDED
  BYTE _ixh,_ixl;     // UNIRE ovviamente! o usare LO/HIBYTE
#endif
	SWORD _iy=0;
#ifdef Z80_EXTENDED
  BYTE _iyh,_iyl;     // UNIRE ovviamente!
#endif
	SWORD _sp=0;
	BYTE _i,_r;
	BYTE IRQ_Mode=1;
	BYTE IRQ_Enable1=0,IRQ_Enable2=0,inEI=0;
  union Z_REGISTERS regs1,regs2;
  union RESULT res1,res2,res3;
//  union OPERAND op1,op2;
	union REGISTRO_F _f;
	union REGISTRO_F _f1;
	/*register*/ SWORD i;
  int c=0;


#if NEZ80
#define ROM_START 0x8000
	_pc=ROM_START;    // truschino sull'hw originale... al boot va qua
#else
	_pc=0;
#endif
  
  
//  _pc=0x0935;
//  _sp=0x8700;
  
  
	do {

		c++;
#ifdef MSX
		if(!(c & 0xffff)) {
#else
		if(!(c & 0x3ffff)) {
#endif
      ClrWdt();
// yield()
#ifndef USING_SIMULATOR      
#ifdef SKYNET
			UpdateScreen(1);
#endif
#ifdef GALAKSIJA
			UpdateScreen(0,128);    // fare passate pi� piccole!
#endif
#ifdef ZX80
			UpdateScreen(0,192,_i);    // fare passate pi� piccole!
      // lineCntr usare?
#endif
#ifdef ZX81
			UpdateScreen(0,192,_i);    // fare passate pi� piccole!
#endif
#ifdef MSX
			UpdateScreen(0,192);    // fare passate pi� piccole!
extern BYTE TMS9918Reg[8],TMS9918RegS;
      TMS9918RegS |= 0b10000000;
      if(TMS9918Reg[1] & 0b00100000) {
        VIDIRQ=1;
        }
      
extern BYTE i8255RegW[4];
      LED3= i8255RegW[2] & 0b01000000 ? 0 : 1;    // CAPS-LOCK!
#endif
#endif
      LED1^=1;    // 42mS~ con SKYNET 7/6/20; 10~mS con Z80NE 10/7/21; 35mS GALAKSIJA 16/10/22; 30mS ZX80 27/10/22
      // raddoppio 2024 (QUADRUPLICO/ecc! 27/10/22
      
#ifdef SKYNET    
      WDCnt--;
      if(!WDCnt) {
        WDCnt=MAX_WATCHDOG;
        if(IOPortO & 0b00001000) {     // WDEn
          CPUPins=DoReset;
          }
        }
#endif
      
      }

		if(ColdReset) {
			ColdReset=0;
      CPUPins=DoReset;
			continue;
      }


#ifdef SKYNET
    if(RTCIRQ) {
      CPUPins |= DoIRQ;
//      ExtIRQNum=0x70;      // IRQ RTC
      LCDram[0x40+20+19]++;
      RTCIRQ=0;
      }
    if(!(IOPortI & 1)) {
 			CPUPins |= DoNMI;
      }
#endif
#ifdef NEZ80
    if(TIMIRQ) {
extern BYTE sense50Hz;
      sense50Hz = !sense50Hz;
      TIMIRQ=0;
// forse... verificare      CPUPins |= DoNMI;     
      }
#endif
#ifdef ZX80
    { static oldR=0;
      if(!(_r & 0x40) && (oldR & 0x40)) {
	      CPUPins |= DoIRQ;
        }
      oldR=_r;
      }
#endif
#ifdef ZX81
    if(TIMIRQ) {
      TIMIRQ=0;
      if(NMIGenerator)
	 			CPUPins |= DoNMI;     // verificare... horiz. retrace dice...
      }
#endif
#ifdef GALAKSIJA
    if(TIMIRQ) {
      CPUPins |= DoIRQ;
      TIMIRQ=0;
      }
#endif
#ifdef MSX
    if(TIMIRQ) {
      CPUPins |= DoIRQ;
      TIMIRQ=0;
      }
    if(VIDIRQ) {
      CPUPins |= DoIRQ;
      VIDIRQ=0;
      }
#endif

    
		/*
		if((_pc >= 0xa000) && (_pc <= 0xbfff)) {
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
			*/
		if(debug) {
//			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
		/*if(kbhit()) {
			getch();
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			printf("281-284: %02x %02x %02x %02x\n",*(p1+0x281),*(p1+0x282),*(p1+0x283),*(p1+0x284));
			printf("2b-2c: %02x %02x\n",*(p1+0x2b),*(p1+0x2c));
			printf("33-34: %02x %02x\n",*(p1+0x33),*(p1+0x34));
			printf("37-38: %02x %02x\n",*(p1+0x37),*(p1+0x38));
			}*/
		if(CPUPins & DoReset) {
#if NEZ80
    	_pc=ROM_START;    // truschino sull'hw originale... al boot va qua
#else
			_pc=0;
#endif
      _i=_r=0;
			IRQ_Enable1=0;IRQ_Enable2=0;inEI=0;
     	IRQ_Mode=1;
      
			CPUPins = 0;//DoWait;
      keysFeedPtr=255; //meglio ;)
      
      initHW();// bah... boh? solo ColdReset?

      continue;
			}
		if((CPUPins & DoNMI) && !inEI) {
			CPUPins &= ~(DoNMI | DoHalt);
			IRQ_Enable2=IRQ_Enable1; IRQ_Enable1=0;
			PutValue(--_sp,HIBYTE(_pc));
			PutValue(--_sp,LOBYTE(_pc));
			_pc=0x0066;
			}
		if((CPUPins & DoIRQ) && !inEI) {
      
      // LED2^=1;    // 
			CPUPins &= ~DoHalt;
      
			if(IRQ_Enable1) {
				IRQ_Enable1=IRQ_Enable2=0;    // When the CPU accepts a maskable interrupt, both IFF1 and IFF2 are automatically reset, 
        //inhibiting further interrupts until the programmer issues a new EI instruction. [was: ma non sono troppo sicuro... boh?]
				CPUPins &= ~DoIRQ;
				PutValue(--_sp,HIBYTE(_pc));
				PutValue(--_sp,LOBYTE(_pc));
				switch(IRQ_Mode) {
				  case 0:
						i=0 /*bus_dati*/;
						// DEVE ESEGUIRE i come istruzione!!
            				  	_pc=  i  ;

				  	break;
				  case 1:
				  	_pc=0x0038;
				  	break;
				  case 2:
				  	_pc=(((SWORD)_i) << 8) | (0 /*bus_dati*/ << 1) | 0;
				  	break;
				  }
				}
			}

		// buttare fuori il valore di _r per il refresh... :)
    _r++;
  
		if(CPUPins & DoHalt) {
      //mettere ritardino per analogia con le istruzioni?
//      __delay_ns(500); non va pi� nulla... boh...
			continue;		// esegue cmq IRQ e refresh
      }
		if(CPUPins & DoWait) {
      //mettere ritardino per analogia con le istruzioni?
//      __delay_ns(100);
			continue;		// esegue cmq IRQ?? penso di no... sistemare
      }

//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
    
#ifdef SKYNET
/*    
      LCDram[0x40+20+3]=(_pc & 0xf) + 0x30;
      if(LCDram[0x40+20+3] >= 0x3a)
        LCDram[0x40+20+3]+=7;
      LCDram[0x40+20+2]=((_pc/16) & 0xf) + 0x30;
      if(LCDram[0x40+20+2] >= 0x3a)
        LCDram[0x40+20+2]+=7;
      LCDram[0x40+20+1]=((_pc/256) & 0xf) + 0x30;
      if(LCDram[0x40+20+1] >= 0x3a)
        LCDram[0x40+20+1]+=7;
      LCDram[0x40+20+0]=((_pc/4096) & 0xf) + 0x30;
      if(LCDram[0x40+20+0] >= 0x3a)
        LCDram[0x40+20+0]+=7;
  */    
#endif
#ifdef GALAKSIJA
/*      ram_seg[96+3]=(_pc & 0xf) + 0x30;
      if(ram_seg[96+3] >= 0x3a)
        ram_seg[96+3]-=0x39;   // qui 'A' = 1 ecc
      ram_seg[96+2]=((_pc/16) & 0xf) + 0x30;
      if(ram_seg[96+2] >= 0x3a)
        ram_seg[96+2]-=0x39;
      ram_seg[96+1]=((_pc/256) & 0xf) + 0x30;
      if(ram_seg[96+1] >= 0x3a)
        ram_seg[96+1]-=0x39;
      ram_seg[96+0]=((_pc/4096) & 0xf) + 0x30;
      if(ram_seg[96+0] >= 0x3a)
        ram_seg[96+0]-=0x39;
*/
#endif    
    
      if(!SW2) {        // test tastiera, me ne frego del repeat/rientro :)
       // continue;
        __delay_ms(100); ClrWdt();
#ifdef NEZ80
        CPUPins = DoReset;
#endif
#ifdef ZX80
        CPUPins = DoReset;
#endif
#ifdef ZX81
        CPUPins = DoReset;
#endif
#ifdef GALAKSIJA
        CPUPins = DoReset;
  
//        __delay_ms(100);
//  			UpdateScreen(0,128);    // 
#endif
#ifdef MSX
        CPUPins = DoReset;
#endif
        }
      if(!SW1) {        // test tastiera
#if defined(NEZ80) || defined(ZX80) || defined(ZX81) || defined(GALAKSIJA) || defined(MSX)
        if(keysFeedPtr==255)      // debounce...
          keysFeedPtr=254;
#endif
        }

      LED2^=1;    // ~700nS 7/6/20, ~600 con 32bit 10/7/21 MA NON FUNZIONA/visualizza!! verificare; 5-700nS 27/10/22

    
/*      if(_pc == 0x069d ab5 43c Cd3) {
        ClrWdt();
        }*/
    if(inEI)
      inEI=0;
  
		switch(GetPipe(_pc++)) {
			case 0:   // NOP
				break;

			case 1:   // LD BC,nn ecc
			case 0x11:
			case 0x21:
#define WORKING_REG16 regs1.r[(Pipe1 & 0x30) >> 4].x
			  WORKING_REG16=Pipe2.x;
			  _pc+=2;
				break;

			case 2:   // LD (BC),a
			  PutValue(_bc,_a);
				break;

			case 3:   // INC BC ecc
			case 0x13:
			case 0x23:
				WORKING_REG16 ++;
				break;

			case 4:   // INC B ecc
			case 0xc:
			case 0x14:
			case 0x1c:
			case 0x24:
			case 0x2c:
			case 0x3c:
				WORKING_REG ++;
        res3.b.l=WORKING_REG;
        
aggInc:
      	_f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
        _f.AddSub=0;
      	_f.PV= res3.b.l == 0x80 ? 1 : 0; //P = 1 if x=7FH before, else 0
      	_f.HalfCarry= (res3.b.l & 0xf) == 0 ? 1 : 0; // INC x      1 if carry from bit 3 else 0 
        break;

			case 5:   // DEC B ecc
			case 0xd:
			case 0x15:
			case 0x1d:
			case 0x25:
			case 0x2d:
			case 0x3d:
				WORKING_REG --;
        res3.b.l=WORKING_REG;
        
aggDec:
      	_f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
        _f.AddSub=1;
      	_f.PV= res3.b.l == 0x7f ? 1 : 0; //P = 1    // DEC x         P = 1 if x=80H before, else 0
      	_f.HalfCarry= (res3.b.l & 0xf) == 0xf ? 1 : 0; // DEC x      1 if borrow from bit 4 else 0 
				break;

			case 6:   // LD B,n ecc
			case 0xe:
			case 0x16:
			case 0x1e:
			case 0x26:
			case 0x2e:
			case 0x3e:
			  WORKING_REG=Pipe2.b.l;
			  _pc++;
				break;

			case 7:   // RLCA
				_f.Carry=_a & 0x80 ? 1 : 0;
				_a <<= 1;
				_a |= _f.Carry;
        
aggRotate:
        _f.HalfCarry=_f.AddSub=0;
				break;
                                         
			case 8:   // EX AF,AF'
        _f_af=_f.b;
        
			  res3.x=_af;
				_af=regs2.r[3].x;
				regs2.r[3].x=res3.x;

        _f.b=_f_af;
				break;

			case 9:   // ADD HL,BC ecc
			case 0x19:
			case 0x29:
        res1.x=_hl;
        res2.x=WORKING_REG16;
			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
			  _hl = res3.x;
        _f.AddSub=0;
        _f.HalfCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
        goto aggFlagWC;
        break;

			case 0xa:   // LD A,(BC)
			  _a=GetValue(_bc);
				break;

      case 0xb:   // DEC BC ecc
      case 0x1b:
      case 0x2b:
				WORKING_REG16 --;
				break;

			case 0xf:   // RRCA
				_f.Carry=_a & 1;
				_a >>= 1;
				if(_f.Carry)
					_a |= 0x80;
        goto aggRotate;
				break;

			case 0x10:   // DJNZ
				_pc++;
			  _b--;
				if(_b)
					_pc+=(signed char)Pipe2.b.l;
				break;

			case 0x12:    // LD (DE),A
			  PutValue(_de,_a);
				break;

      case 0x17:    // RLA
				_f1=_f;
				_f.Carry=_a & 0x80 ? 1 : 0;
				_a <<= 1;
				_a |= _f1.Carry;
        goto aggRotate;
				break;

      case 0x18:    // JR
				_pc++;
				_pc+=(signed char)Pipe2.b.l;
				break;
				
			case 0x1a:    // LD A,(DE)
			  _a=GetValue(_de);
				break;

			case 0x1f:    // RRA
				_f1=_f;
				_f.Carry=_a & 1;
				_a >>= 1;
				if(_f1.Carry)
					_a |= 0x80;
        goto aggRotate;
				break;

			case 0x20:    // JR nz
				_pc++;
				if(!_f.Zero)
					_pc+=(signed char)Pipe2.b.l;
				break;

			case 0x22:    // LD(nn),HL
			  PutIntValue(Pipe2.x,_hl);
			  _pc+=2;
				break;

			case 0x27:		// DAA
        res3.x=res1.x=_a;
        i=_f.Carry;
        _f.Carry=0;
        if((_a & 0xf) > 9 || _f.HalfCarry) {
          res3.x+=6;
          _a=res3.b.l;
          _f.Carry= i || HIBYTE(res3.x);
          _f.HalfCarry=1;
          }
        else
          _f.HalfCarry=0;
        if((res1.b.l>0x99) || i) {
          _a+=0x60;  
          _f.Carry=1;
          }
        else
          _f.Carry=0;
        goto calcParity;
				break;

      case 0x28:    // JR z
				_pc++;
        if(_f.Zero)
				  _pc+=(signed char)Pipe2.b.l;
				break;
				
			case 0x2a:    // LD HL,(nn)
			  _hl=GetIntValue(Pipe2.x);
			  _pc+=2;
				break;

  		case 0x2f:    // CPL
        _a=~_a;
        _f.HalfCarry=_f.AddSub=1;
				break;
        
			case 0x30:    // JR nc
				_pc++;
				if(!_f.Carry)
					_pc+=(signed char)Pipe2.b.l;
				break;

			case 0x31:    // LD SP,nn (v. anche HL ecc)
			  _sp=Pipe2.x;
			  _pc+=2;
				break;

			case 0x32:    // LD (nn),A
			  PutValue(Pipe2.x,_a);
			  _pc+=2;
				break;

			case 0x33:    // INC SP (v. anche INC BC ecc)
			  _sp++;
				break;

			case 0x34:    // INC (HL)
        res3.b.l=GetValue(_hl)+1;
			  PutValue(_hl,res3.b.l);
        goto aggInc;
				break;

			case 0x35:    // DEC (HL)
        res3.b.l=GetValue(_hl)-1;
			  PutValue(_hl,res3.b.l);
        goto aggDec;
				break;

			case 0x36:    // LD (HL),n
			  PutValue(_hl,Pipe2.b.l);
			  _pc++;
				break;
              
      case 0x37:    // SCF
        _f.Carry=1;
        _f.AddSub=_f.HalfCarry=0;
        break;
                                   
      case 0x38:    // JR c
				_pc++;
        if(_f.Carry)
				  _pc+=(signed char)Pipe2.b.l;
				break;
				
			case 0x39:    // ADD HL,SP
        res1.x=_hl;
        res2.x=_sp;
			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
        _hl = res3.x;
        _f.AddSub=0;
        _f.HalfCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
        goto aggFlagWC;
				break;

			case 0x3a:    // LD A,(nn)
			  _a=GetValue(Pipe2.x);
			  _pc+=2;
				break;

      case 0x3b:    // DEC SP (v. anche DEC BC ecc)
			  _sp--;
				break;

      case 0x3f:    // CCF
        _f.HalfCarry=_f.Carry;
        _f.Carry=!_f.Carry;
        _f.AddSub=0;
        break;
                                   
			case 0x40:    // LD r,r
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x47:
			case 0x48:                             // 
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4f:
			case 0x50:                             // 
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x57:
			case 0x58:                             // 
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5f:
			case 0x60:                             // 
			case 0x61:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x65:
			case 0x67:
			case 0x68:                             // 
			case 0x69:
			case 0x6a:
			case 0x6b:
			case 0x6c:
			case 0x6d:
			case 0x6f:
			case 0x78:                             // 
			case 0x79:
			case 0x7a:
			case 0x7b:
			case 0x7c:
			case 0x7d:
			case 0x7f:
				WORKING_REG=WORKING_REG2;
				break;

			case 0x46:    // LD r,(HL)
			case 0x4e:
			case 0x56:
			case 0x5e:
			case 0x66:
			case 0x6e:
			case 0x7e:
				WORKING_REG=GetValue(_hl);
				break;

			case 0x70:                             // 
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x77:
				PutValue(_hl,/* regs1.b[((Pipe1 & 7) +1) & 7]*/ WORKING_REG2);
				break;
        
			case 0x76:    // HALT
			  CPUPins |= DoHalt;
				break;

			case 0x80:    // ADD A,r
			case 0x81:
			case 0x82:
			case 0x83:
			case 0x84:
			case 0x85:
			case 0x87:
        res2.b.l=WORKING_REG2;
        
aggSomma:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x+res2.x;
        _a=res3.b.l;
        _f.AddSub=0;
        _f.HalfCarry = ((res1.b.l & 0xf) + (res2.b.l & 0xf)) >= 0x10 ? 1 : 0;   // 

aggFlagB:
//        _f.PV = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x40) != !!(((res1.b.l & 0x80) + (res2.b.l & 0x80)) & 0x80);
//        _f.PV = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
  //(M^result)&(N^result)&0x80 is nonzero. That is, if the sign of both inputs is different from the sign of the result. (Anding with 0x80 extracts just the sign bit from the result.) 
  //Another C++ formula is !((M^N) & 0x80) && ((M^result) & 0x80)
//        _f.PV = !!((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80);
//        _f.PV = !!(!((res1.b.l ^ res2.b.l) & 0x80) && ((res1.b.l ^ res3.b.l) & 0x80));
//**        _f.PV = ((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80) ? 1 : 0;
  // Calculate the overflow by sign comparison.
/*  carryIns = ((a ^ b) ^ 0x80) & 0x80;
  if (carryIns) // if addend signs are the same
  {
    // overflow if the sum sign differs from the sign of either of addends
    carryIns = ((*acc ^ a) & 0x80) != 0;
  }*/
	// per overflow e halfcarry https://stackoverflow.com/questions/8034566/overflow-and-carry-flags-on-z80
/*The overflow checks the most significant bit of the 8 bit result. This is the sign bit. If we add two negative numbers (MSBs=1) then the result should be negative (MSB=1), whereas if we add two positive numbers (MSBs=0) then the result should be positive (MSBs=0), so the MSB of the result must be consistent with the MSBs of the summands if the operation was successful, otherwise the overflow bit is set.*/        
/*        if(!_f.Sign) {
          _f.PV=(res1.b.l & 0x80 || res2.b.l & 0x80) ? 1 : 0;
          }
        else {
          _f.PV=(res1.b.l & 0x80 && res2.b.l & 0x80) ? 1 : 0;
          }*/
/*        if(res1.b.l & 0x80 && res2.b.l & 0x80)
          _f.PV=res3.b.l & 0x80 ? 0 : 1;
        else if(!(res1.b.l & 0x80) && !(res2.b.l & 0x80))
          _f.PV=res3.b.l & 0x80 ? 1 : 0;
        else
          _f.PV=0;*/
        _f.PV = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
        
aggFlagBC:    // http://www.z80.info/z80sflag.htm
				_f.Carry=!!res3.b.h;
        
        _f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
				break;

			case 0x86:    // ADD A,(HL)
        res2.b.l=GetValue(_hl);
        goto aggSomma;
				break;

			case 0x88:    // ADC A,r
			case 0x89:
			case 0x8a:
			case 0x8b:
			case 0x8c:
			case 0x8d:
			case 0x8f:
        res2.b.l=WORKING_REG2;
        
aggSommaC:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x+res2.x+_f.Carry;
        _a=res3.b.l;
        _f.AddSub=0;
        _f.HalfCarry = ((res1.b.l & 0xf) + (res2.b.l & 0xf)) >= 0x10 ? 1 : 0;   // 
//#warning CONTARE IL CARRY NELL overflow?? no, pare di no (v. emulatore ma io credo di s�
//        _f.PV = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
/*        if(res1.b.l & 0x80 && res2.b.l & 0x80)
          _f.PV=res3.b.l & 0x80 ? 0 : 1;
        else if(!(res1.b.l & 0x80) && !(res2.b.l & 0x80))
          _f.PV=res3.b.l & 0x80 ? 1 : 0;
        else
          _f.PV=0;*/
        goto aggFlagB;
				break;

			case 0x8e:    // ADC A,(HL)
        res2.b.l=GetValue(_hl);
        goto aggSommaC;
				break;

			case 0x90:    // SUB A,r
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x97:
        res2.b.l=WORKING_REG2;
        
aggSottr:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x-res2.x;
        _a=res3.b.l;
        _f.AddSub=1;
        _f.HalfCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
//        _f.PV = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _f.PV = ((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80) ? 1 : 0;
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.PV=1;
          else
            _f.PV=0;
          }
        else
          _f.PV=0;*/
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.PV=1;
          else
            _f.PV=0;
          }
        else
          _f.PV=0;*/
        _f.PV = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
  			goto aggFlagBC;
				break;

			case 0x96:    // SUB A,(HL)
        res2.b.l=GetValue(_hl);
				goto aggSottr;
				break;

			case 0x98:    // SBC A,r
			case 0x99:
			case 0x9a:
			case 0x9b:
			case 0x9c:
			case 0x9d:
			case 0x9f:
        res2.b.l=WORKING_REG2;
        
aggSottrC:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x-res2.x-_f.Carry;
        _a=res3.b.l;
        _f.AddSub=1;
        _f.HalfCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0  ? 1 : 0;   // 
//#warning CONTARE IL CARRY NELL overflow?? no, pare di no (v. emulatore ma io credo di s�..
//        _f.PV = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.PV=1;
          else
            _f.PV=0;
          }
        else
          _f.PV=0;*/
        goto aggFlagB;
				break;

			case 0x9e:    // SBC A,(HL)
        res2.b.l=GetValue(_hl);
				goto aggSottrC;
				break;

			case 0xa0:    // AND A,r
			case 0xa1:
			case 0xa2:
			case 0xa3:
			case 0xa4:
			case 0xa5:
			case 0xa7:
				_a &= WORKING_REG2;
        _f.HalfCarry=1;
        
aggAnd:
        res3.b.l=_a;
aggAnd2:
        _f.Carry=0;
aggAnd3:      // usato da IN 
        _f.AddSub=0;
        _f.Zero=_a ? 0 : 1;
        _f.Sign=_a & 0x80 ? 1 : 0;
        // halfcarry � 1 fisso se AND e 0 se OR/XOR
        
calcParity:
          {
          BYTE par;
          par= res3.b.l >> 1;			// Microchip AN774
          par ^= res3.b.l;
          res3.b.l= par >> 2;
          par ^= res3.b.l;
          res3.b.l= par >> 4;
          par ^= res3.b.l;
          _f.PV=par & 1 ? 1 : 0;		// EVEN
          }
				break;

			case 0xa6:    // AND A,(HL)
				_a &= GetValue(_hl);
        _f.HalfCarry=1;
        goto aggAnd;
				break;

			case 0xa8:    // XOR A,r
			case 0xa9:
			case 0xaa:
			case 0xab:
			case 0xac:
			case 0xad:
			case 0xaf:
				_a ^= WORKING_REG2;
        _f.HalfCarry=0;
        goto aggAnd;
				break;

			case 0xae:    // XOR A,(HL)
				_a ^= GetValue(_hl);
        _f.HalfCarry=0;
        goto aggAnd;
				break;

			case 0xb0:    // OR A,r
			case 0xb1:
			case 0xb2:
			case 0xb3:
			case 0xb4:
			case 0xb5:
			case 0xb7:
				_a |= WORKING_REG2;
        _f.HalfCarry=0;
        goto aggAnd;
				break;

			case 0xb6:    // OR A,(HL)
				_a |= GetValue(_hl);
        _f.HalfCarry=0;
        goto aggAnd;
				break;

			case 0xb8:    // CP A,r
			case 0xb9:
			case 0xba:
			case 0xbb:
			case 0xbc:
			case 0xbd:
			case 0xbf:
				res2.b.l=WORKING_REG2;
				goto compare;
				break;

			case 0xbe:    // CP A,(HL)
				res2.b.l=GetValue(_hl);
  			goto compare;
				break;

			case 0xc0:    // RET NZ
			  if(!_f.Zero)
			    goto Return;
				break;

			case 0xc1:    // POP BC ecc
			case 0xd1:    
			case 0xe1:    
#define WORKING_REG16B regs1.r[(Pipe1 & 0x30) >> 4].b
				WORKING_REG16B.l=GetValue(_sp++);
				WORKING_REG16B.h=GetValue(_sp++);
				break;
			case 0xf1:    
				_f.b=_f_af=GetValue(_sp++);
				_a=GetValue(_sp++);
				break;

			case 0xc2:    // JP nz
			  if(!_f.Zero)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xc3:    // JP
Jump:
				_pc=Pipe2.x;
				break;

			case 0xc4:    // CALL nz
			  if(!_f.Zero)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xc5:    // PUSH BC ecc
			case 0xd5:    // 
			case 0xe5:    // 
				PutValue(--_sp,WORKING_REG16B.h);
				PutValue(--_sp,WORKING_REG16B.l);
				break;
			case 0xf5:    // push af..
        _f_af=_f.b;
				PutValue(--_sp,_a);
				PutValue(--_sp,_f_af);
				break;

			case 0xc6:    // ADD A,n
			  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSomma;
				break;

			case 0xc7:    // RST
			case 0xcf:
			case 0xd7:
			case 0xdf:
			case 0xe7:
			case 0xef:
			case 0xf7:
			case 0xff:
			  i=Pipe1 & 0x38;
RST:
				PutValue(--_sp,HIBYTE(_pc));
				PutValue(--_sp,LOBYTE(_pc));
				_pc=i;
				break;
				
			case 0xc8:    // RET z
			  if(_f.Zero)
			    goto Return;
				break;

			case 0xc9:    // RET
Return:
        _pc=GetValue(_sp++);
        _pc |= ((SWORD)GetValue(_sp++)) << 8;
				break;

			case 0xca:    // JP z
			  if(_f.Zero)
			    goto Jump;
			  else
			    _pc+=2;
				break;


			case 0xcb:
        _pc++;
				switch(Pipe2.b.l) {
					case 0x00:   // RLC r
					case 0x01:
					case 0x02:
					case 0x03:
					case 0x04:
					case 0x05:
					case 0x07:
						_f.Carry=WORKING_REG_CB & 0x80 ? 1 : 0;
						WORKING_REG_CB <<= 1;
						WORKING_REG_CB |= _f.Carry;
            res3.b.l=WORKING_REG_CB;
            
aggRotate2:
            _f.HalfCarry=_f.AddSub=0;
            _f.Zero=res3.b.l ? 0 : 1;
          	_f.Sign=res3.b.l & 0x80 ? 1 : 0;
            goto calcParity;
						break;
					case 0x06:   // RLC (HL)
						res3.b.l=GetValue(_hl);
						_f.Carry=res3.b.l & 0x80 ? 1 : 0;
						res3.b.l <<= 1;
						res3.b.l |= _f.Carry;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;

					case 0x08:   // RRC r
					case 0x09:
					case 0x0a:
					case 0x0b:
					case 0x0c:
					case 0x0d:
					case 0x0f:
						_f.Carry=WORKING_REG_CB & 0x1;
						WORKING_REG_CB >>= 1;
						if(_f.Carry)
							WORKING_REG_CB |= 0x80;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x0e:   // RRC (HL)
						res3.b.l=GetValue(_hl);
						_f.Carry=res3.b.l & 0x1;
						res3.b.l >>= 1;
						if(_f.Carry)
							res3.b.l |= 0x80;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;

					case 0x10:   // RL r
					case 0x11:
					case 0x12:
					case 0x13:
					case 0x14:
					case 0x15:
					case 0x17:
						_f1=_f;
						_f.Carry=WORKING_REG_CB & 0x80 ? 1 : 0;
						WORKING_REG_CB <<= 1;
						WORKING_REG_CB |= _f1.Carry;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x16:   // RL (HL)
						_f1=_f;
						res3.b.l=GetValue(_hl);
						_f.Carry=res3.b.l & 0x80 ? 1 : 0;
						res3.b.l <<= 1;
						res3.b.l |= _f1.Carry;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;
            
					case 0x18:   // RR r
					case 0x19:
					case 0x1a:
					case 0x1b:
					case 0x1c:
					case 0x1d:
					case 0x1f:
						_f1=_f;
						_f.Carry=WORKING_REG_CB & 0x1;
						WORKING_REG_CB >>= 1;
						if(_f1.Carry)
							WORKING_REG_CB |= 0x80;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x1e:   // RR (HL)
						_f1=_f;
						res3.b.l=GetValue(_hl);
						_f.Carry=res3.b.l & 0x1;
						res3.b.l >>= 1;
						if(_f1.Carry)
							res3.b.l |= 0x80;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;

					case 0x20:   // SLA r
					case 0x21:
					case 0x22:
					case 0x23:
					case 0x24:
					case 0x25:
					case 0x27:
						_f.Carry= WORKING_REG_CB & 0x80 ? 1 : 0;
						WORKING_REG_CB <<= 1;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x26:   // SLA (HL)
						res3.b.l=GetValue(_hl);
						_f.Carry= res3.b.l & 0x80 ? 1 : 0;
						res3.b.l <<= 1;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;

					case 0x28:   // SRA r
					case 0x29:
					case 0x2a:
					case 0x2b:
					case 0x2c:
					case 0x2d:
					case 0x2f:
						_f.Carry=WORKING_REG_CB & 0x1;
						WORKING_REG_CB >>= 1;
						if(WORKING_REG_CB & 0x40)
							WORKING_REG_CB |= 0x80;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x2e:   // SRA (HL)
						res3.b.l=GetValue(_hl);
						_f.Carry=res3.b.l & 0x1;
						res3.b.l >>= 1;
						if(res3.b.l & 0x40)
							res3.b.l |= 0x80;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;

#ifdef Z80_EXTENDED
					case 0x30:   // SLL r
					case 0x31:
					case 0x32:
					case 0x33:
					case 0x34:
					case 0x35:
					case 0x37:
						_f.Carry= WORKING_REG_CB & 0x80 ? 1 : 0;
						WORKING_REG_CB <<= 1;
						WORKING_REG_CB |= 1;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x36:   // SLL (HL)
						res3.b.l=GetValue(_hl);
						_f.Carry= res3.b.l & 0x80 ? 1 : 0;
						res3.b.l <<= 1;
						res3.b.l |= 1;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;
#endif

					case 0x38:   // SRL r
					case 0x39:
					case 0x3a:
					case 0x3b:
					case 0x3c:
					case 0x3d:
					case 0x3f:
						_f.Carry=WORKING_REG_CB & 0x1;
						WORKING_REG_CB >>= 1;
            res3.b.l=WORKING_REG_CB;
            goto aggRotate2;
						break;
					case 0x3e:   // SRL (HL)
						res3.b.l=GetValue(_hl);
						_f.Carry=res3.b.l & 0x1;
						res3.b.l >>= 1;
						PutValue(_hl,res3.b.l);
            goto aggRotate2;
						break;

					case 0x40:   // BIT 
					case 0x41:
					case 0x42:
					case 0x43:
					case 0x44:
					case 0x45:
					case 0x47:
					case 0x48:
					case 0x49:
					case 0x4a:
					case 0x4b:
					case 0x4c:
					case 0x4d:
					case 0x4f:
					case 0x50:
					case 0x51:
					case 0x52:
					case 0x53:
					case 0x54:
					case 0x55:
					case 0x57:
					case 0x58:
					case 0x59:
					case 0x5a:
					case 0x5b:
					case 0x5c:
					case 0x5d:
					case 0x5f:
					case 0x60:
					case 0x61:
					case 0x62:
					case 0x63:
					case 0x64:
					case 0x65:
					case 0x67:
					case 0x68:
					case 0x69:
					case 0x6a:
					case 0x6b:
					case 0x6c:
					case 0x6d:
					case 0x6f:
					case 0x70:
					case 0x71:
					case 0x72:
					case 0x73:
					case 0x74:
					case 0x75:
					case 0x77:
					case 0x78:
					case 0x79:
					case 0x7a:
					case 0x7b:
					case 0x7c:
					case 0x7d:
					case 0x7f:
            res3.b.l=WORKING_REG_CB & WORKING_BITPOS;
aggBit:
            _f.Zero=res3.b.l ? 0 : 1;
            _f.HalfCarry=1;
            _f.AddSub=0;
						break;

					case 0x46:   // BIT (HL)
					case 0x4e:
					case 0x56:
					case 0x5e:
					case 0x66:
					case 0x6e:
					case 0x76:
					case 0x7e:
						res3.b.l=GetValue(_hl) & WORKING_BITPOS;
						goto aggBit;
						break;

					case 0x80:   // RES 
					case 0x81:
					case 0x89:
					case 0x91:
					case 0x99:
					case 0xa1:
					case 0xa9:
					case 0xb1:
					case 0xb9:
					case 0x88:
					case 0x90:
					case 0x98:
					case 0xa0:
					case 0xa8:
					case 0xb0:
					case 0xb8:
					case 0x82:
					case 0x8a:
					case 0x92:
					case 0x9a:
					case 0xa2:
					case 0xaa:
					case 0xb2:
					case 0xba:
					case 0x83:
					case 0x8b:
					case 0x93:
					case 0x9b:
					case 0xa3:
					case 0xab:
					case 0xb3:
					case 0xbb:
					case 0x84:
					case 0x8c:
					case 0x94:
					case 0x9c:
					case 0xa4:
					case 0xac:
					case 0xb4:
					case 0xbc:
					case 0x85:
					case 0x8d:
					case 0x95:
					case 0x9d:
					case 0xa5:
					case 0xad:
					case 0xb5:
					case 0xbd:
					case 0x87:
					case 0x8f:
					case 0x97:
					case 0x9f:
					case 0xa7:
					case 0xaf:
					case 0xb7:
					case 0xbf:
						WORKING_REG_CB &= ~ WORKING_BITPOS;
						break;

					case 0x86:   // RES (HL)
					case 0x8e:
					case 0x96:
					case 0x9e:
					case 0xa6:
					case 0xae:
					case 0xb6:
					case 0xbe:
						res3.b.l=GetValue(_hl);
            res3.b.l &= ~ WORKING_BITPOS;
						PutValue(_hl,res3.b.l);
						break;

					case 0xc0:   // SET
					case 0xc8:
					case 0xd0:
					case 0xd8:
					case 0xe0:
					case 0xe8:
					case 0xf0:
					case 0xf8:
					case 0xc1:
					case 0xc9:
					case 0xd1:
					case 0xd9:
					case 0xe1:
					case 0xe9:
					case 0xf1:
					case 0xf9:
					case 0xc2:
					case 0xca:
					case 0xd2:
					case 0xda:
					case 0xe2:
					case 0xea:
					case 0xf2:
					case 0xfa:
					case 0xc3:
					case 0xcb:
					case 0xd3:
					case 0xdb:
					case 0xe3:
					case 0xeb:
					case 0xf3:
					case 0xfb:
					case 0xc4:
					case 0xcc:
					case 0xd4:
					case 0xdc:
					case 0xe4:
					case 0xec:
					case 0xf4:
					case 0xfc:
					case 0xc5:
					case 0xcd:
					case 0xd5:
					case 0xdd:
					case 0xe5:
					case 0xed:
					case 0xf5:
					case 0xfd:
					case 0xc7:
					case 0xcf:
					case 0xd7:
					case 0xdf:
					case 0xe7:
					case 0xef:
					case 0xf7:
					case 0xff:
						WORKING_REG_CB |= WORKING_BITPOS;
						break;

					case 0xc6:   // SET (HL)
					case 0xce:
					case 0xd6:
					case 0xde:
					case 0xe6:
					case 0xee:
					case 0xf6:
					case 0xfe:
						res3.b.l=GetValue(_hl);
						res3.b.l |= WORKING_BITPOS;
						PutValue(_hl,res3.b.l);
						break;

					default:
//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
            Nop();
						break;
					}
				break;

			case 0xcc:    // CALL z
			  if(_f.Zero)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xcd:		// CALL
Call:
				i=Pipe2.x;
		    _pc+=2;
				goto RST;
				break;

			case 0xce:    // ADC A,n
			  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSommaC;
				break;

			case 0xd0:    // RET nc
			  if(!_f.Carry)
			    goto Return;
				break;

			case 0xd2:    // JP nc
			  if(!_f.Carry)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xd3:    // OUT
				OutValue(MAKEWORD(Pipe2.b.l,_a),_a);
				_pc++;
				break;

			case 0xd4:    // CALL nc
			  if(!_f.Carry)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xd6:    // SUB n
  		  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSottr;
				break;

			case 0xd8:    // RET c
			  if(_f.Carry)
			    goto Return;
				break;

			case 0xd9:    // EXX
        {
        BYTE n;
        for(n=0; n<3; n++) {
          res3.x=regs2.r[n].x;
          regs2.r[n].x=regs1.r[n].x;
          regs1.r[n].x=res3.x;
          }
        }
				break;

			case 0xda:    // JP c
			  if(_f.Carry)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xdb:    // IN a,  NON tocca flag
				_pc++;
				_a=InValue(MAKEWORD(Pipe2.b.l,_a));
				break;

			case 0xdc:    // CALL c
			  if(_f.Carry)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xdd:
				switch(GetPipe(_pc++)) {
					case 0xcb:   //
#define IX_OFFSET (_ix+((signed char)Pipe2.b.l))
#define WORKING_REG_DD_CB regs1.b[Pipe2.b.h & 7]
						switch(Pipe2.b.h) {		// il 4� byte!
#ifdef Z80_EXTENDED
							case 0x00:   // RLC (IX+
							case 0x01:
							case 0x02:
							case 0x03:
							case 0x04:
							case 0x05:
							case 0x07:
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
    						_f.Carry= WORKING_REG_DD_CB & 0x80 ? 1 : 0;
								WORKING_REG_DD_CB <<= 1;
								WORKING_REG_DD_CB |= _f.Carry;
                res3.b.l=WORKING_REG_DD_CB;
    						_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x06:   // RLC (IX+
								res3.b.l = GetValue(IX_OFFSET);
    						_f.Carry= res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								res3.b.l |= _f.Carry;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x08:   // RRC
							case 0x09:
							case 0x0a:
							case 0x0b:
							case 0x0c:
							case 0x0d:
							case 0x0f:
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
								_f.Carry=WORKING_REG_DD_CB & 0x1;
								WORKING_REG_DD_CB >>= 1;
								if(_f.Carry)
									WORKING_REG_DD_CB |= 0x80;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x0e:   // RRC (IX+
								res3.b.l = GetValue(IX_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								if(_f.Carry)
									res3.b.l |= 0x80;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x10:   // RL (IX+
							case 0x11:
							case 0x12:
							case 0x13:
							case 0x14:
							case 0x15:
							case 0x17:
								_f1=_f;
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
    						_f.Carry=WORKING_REG_DD_CB & 0x80 ? 1 : 0;
								WORKING_REG_DD_CB <<= 1;
								WORKING_REG_DD_CB |= _f1.Carry;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x16:   // RL (IX+
								_f1=_f;
								res3.b.l = GetValue(IX_OFFSET);
    						_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								res3.b.l |= _f1.Carry;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x18:   // RR (IX+
							case 0x19:
							case 0x1a:
							case 0x1b:
							case 0x1c:
							case 0x1d:
							case 0x1f:
								_f1=_f;
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
								_f.Carry=WORKING_REG_DD_CB & 0x1;
								WORKING_REG_DD_CB >>= 1;
								if(_f1.Carry)
									WORKING_REG_DD_CB |= 0x80;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x1e:   // RR (IX+
								_f1=_f;
								res3.b.l = GetValue(IX_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								if(_f1.Carry)
									res3.b.l |= 0x80;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x20:   // SLA
							case 0x21:
							case 0x22:
							case 0x23:
							case 0x24:
							case 0x25:
							case 0x27:
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
								_f.Carry=WORKING_REG_DD_CB & 0x80 ? 1 : 0;
								WORKING_REG_DD_CB <<= 1;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x26:   // SLA (IX+
								res3.b.l = GetValue(IX_OFFSET);
								_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x28:   // SRA (IX+
							case 0x29:
							case 0x2a:
							case 0x2b:
							case 0x2c:
							case 0x2d:
							case 0x2f:
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
								_f.Carry=WORKING_REG_DD_CB & 0x1;
								WORKING_REG_DD_CB >>= 1;
								if(WORKING_REG_DD_CB & 0x40)
									WORKING_REG_DD_CB |= 0x80;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x2e:   // SRA (IX+
								res3.b.l = GetValue(IX_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								if(res3.b.l & 0x40)
									res3.b.l |= 0x80;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x30:   // SLL (IX+
							case 0x31:
							case 0x32:
							case 0x33:
							case 0x34:
							case 0x35:
							case 0x37:
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
								_f.Carry=WORKING_REG_DD_CB & 0x80 ? 1 : 0;
								WORKING_REG_DD_CB <<= 1;
								WORKING_REG_DD_CB |= 1;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
							case 0x36:   // SLL
								res3.b.l = GetValue(IX_OFFSET);
								_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								res3.b.l |= 1;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;
#endif

#ifdef Z80_EXTENDED
							case 0x38:   // SRL (IX+
							case 0x39:
							case 0x3a:
							case 0x3b:
							case 0x3c:
							case 0x3d:
							case 0x3f:
								WORKING_REG_DD_CB = GetValue(IX_OFFSET);
								_f.Carry=WORKING_REG_DD_CB & 0x1;
								WORKING_REG_DD_CB >>= 1;
                res3.b.l=WORKING_REG_DD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x3e:   // SRL (IX+
								res3.b.l = GetValue(IX_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								PutValue(IX_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x40:   // BIT IX
							case 0x41:
							case 0x42:
							case 0x43:
							case 0x44:
							case 0x45:
							case 0x47:
							case 0x48:
							case 0x49:
							case 0x4a:
							case 0x4b:
							case 0x4c:
							case 0x4d:
							case 0x4f:
							case 0x50:
							case 0x51:
							case 0x52:
							case 0x53:
							case 0x54:
							case 0x55:
							case 0x57:
							case 0x58:
							case 0x59:
							case 0x5a:
							case 0x5b:
							case 0x5c:
							case 0x5d:
							case 0x5f:
							case 0x60:
							case 0x61:
							case 0x62:
							case 0x63:
							case 0x64:
							case 0x65:
							case 0x67:
							case 0x68:
							case 0x69:
							case 0x6a:
							case 0x6b:
							case 0x6c:
							case 0x6d:
							case 0x6f:
							case 0x70:
							case 0x71:
							case 0x72:
							case 0x73:
							case 0x74:
							case 0x75:
							case 0x77:
							case 0x78:
							case 0x79:
							case 0x7a:
							case 0x7b:
							case 0x7c:
							case 0x7d:
							case 0x7f:
								res3.b.l=GetValue(IX_OFFSET) & WORKING_BITPOS2;
        				_pc+=2;
                goto aggBit;
								break;
#endif

							case 0x46:   // BIT (IX+
							case 0x4e:
							case 0x56:
							case 0x5e:
							case 0x66:
							case 0x6e:
							case 0x76:
							case 0x7e:
								res3.b.l=GetValue(IX_OFFSET) & WORKING_BITPOS2;
        				_pc+=2;
                goto aggBit;
								break;


#ifdef Z80_EXTENDED
							case 0x80:   // RES (IX+
							case 0x81:
							case 0x89:
							case 0x91:
							case 0x99:
							case 0xa1:
							case 0xa9:
							case 0xb1:
							case 0xb9:
							case 0x88:
							case 0x90:
							case 0x98:
							case 0xa0:
							case 0xa8:
							case 0xb0:
							case 0xb8:
							case 0x82:
							case 0x8a:
							case 0x92:
							case 0x9a:
							case 0xa2:
							case 0xaa:
							case 0xb2:
							case 0xba:
							case 0x83:
							case 0x8b:
							case 0x93:
							case 0x9b:
							case 0xa3:
							case 0xab:
							case 0xb3:
							case 0xbb:
							case 0x84:
							case 0x8c:
							case 0x94:
							case 0x9c:
							case 0xa4:
							case 0xac:
							case 0xb4:
							case 0xbc:
							case 0x85:
							case 0x8d:
							case 0x95:
							case 0x9d:
							case 0xa5:
							case 0xad:
							case 0xb5:
							case 0xbd:
							case 0x87:
							case 0x8f:
							case 0x97:
							case 0x9f:
							case 0xa7:
							case 0xaf:
							case 0xb7:
							case 0xbf:
								res3.b.l=GetValue(IX_OFFSET);
								res3.b.l &= ~ WORKING_BITPOS2;
								WORKING_REG_DD_CB = res3.b.l;
								break;
#endif

							case 0x86:   // RES (IX+
							case 0x8e:
							case 0x96:
							case 0x9e:
							case 0xa6:
							case 0xae:
							case 0xb6:
							case 0xbe:
								res3.b.l=GetValue(IX_OFFSET);
								res3.b.l &= ~ WORKING_BITPOS2;
								PutValue(IX_OFFSET,res3.b.l);
								break;

#ifdef Z80_EXTENDED
							case 0xc0:   // SET (IX+
							case 0xc8:
							case 0xd0:
							case 0xd8:
							case 0xe0:
							case 0xe8:
							case 0xf0:
							case 0xf8:
							case 0xc1:
							case 0xc9:
							case 0xd1:
							case 0xd9:
							case 0xe1:
							case 0xe9:
							case 0xf1:
							case 0xf9:
							case 0xc2:
							case 0xca:
							case 0xd2:
							case 0xda:
							case 0xe2:
							case 0xea:
							case 0xf2:
							case 0xfa:
							case 0xc3:
							case 0xcb:
							case 0xd3:
							case 0xdb:
							case 0xe3:
							case 0xeb:
							case 0xf3:
							case 0xfb:
							case 0xc4:
							case 0xcc:
							case 0xd4:
							case 0xdc:
							case 0xe4:
							case 0xec:
							case 0xf4:
							case 0xfc:
							case 0xc5:
							case 0xcd:
							case 0xd5:
							case 0xdd:
							case 0xe5:
							case 0xed:
							case 0xf5:
							case 0xfd:
							case 0xc7:
							case 0xcf:
							case 0xd7:
							case 0xdf:
							case 0xe7:
							case 0xef:
							case 0xf7:
							case 0xff:
								res3.b.l=GetValue(IX_OFFSET);
								res3.b.l |= WORKING_BITPOS2;
								WORKING_REG_DD_CB = res3.b.l;
								break;
#endif

							case 0xc6:   // SET (IX+
							case 0xce:
							case 0xd6:
							case 0xde:
							case 0xe6:
							case 0xee:
							case 0xf6:
							case 0xfe:
								res3.b.l=GetValue(IX_OFFSET);
								res3.b.l |= WORKING_BITPOS2;
								PutValue(IX_OFFSET,res3.b.l);
								break;

							default:
		//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
                Nop();
								break;
							}
						_pc+=2;
						break;
            
//#define WORKING_REG regs1.b[((Pipe1 & 0x38) >> 3) & 3]
					case 0x09:   // ADD IX,BC
					case 0x19:   // ADD IX,DE
            res2.x=WORKING_REG16;
            
aggSommaIX:
            res1.x=_ix;
    			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
    			  _ix = res3.x;
            _f.HalfCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
            goto aggFlagWC;
						break;
					case 0x21:    // LD IX,nn
						_ix = Pipe2.x;
						_pc+=2;
						break;
					case 0x22:    // LD (nn),IX
						PutIntValue(Pipe2.x,_ix);
            _pc+=2;
						break;
					case 0x23:   // INC IX
						_ix++;
						break;
#ifdef Z80_EXTENDED
					case 0x24:    // INC IXh
						_ixh++;
            res3.b.l=_ixh;
						goto aggInc;
						break;
					case 0x25:    // DEC IXh
						_ixh--;
            res3.b.l=_ixh;
						goto aggDec;
						break;
					case 0x26:    // LD IXh,n
						_ixh=Pipe2.b.l;
						_pc++;
						break;
#endif
					case 0x29:    // ADD IX,IX
            res2.x=_ix;
            goto aggSommaIX;
						break;
					case 0x2a:    // LD IX,(nn)
						_ix = GetIntValue(Pipe2.x);
						_pc+=2;
						break;
					case 0x2b:    // DEC IX
						_ix--;
						break;
#ifdef Z80_EXTENDED
					case 0x2c:    // INC IXl
						_ixl++;
            res3.b.l=_ixl;
						goto aggInc;
						break;
					case 0x2d:    // DEC IXl
						_ixl--;
            res3.b.l=_ixl;
						goto aggDec;
						break;
					case 0x2e:    // LD IXl,n
						_ixl=Pipe2.b.l;
						_pc++;
						break;
#endif
					case 0x34:    // INC (IX+n)
						res3.b.l = GetValue(IX_OFFSET);
						res3.b.l++;
						PutValue(IX_OFFSET,res3.b.l);
						_pc++;
            goto aggInc;
						break;
					case 0x35:    // DEC (IX+n)
						res3.b.l = GetValue(IX_OFFSET);
						res3.b.l--;
						PutValue(IX_OFFSET,res3.b.l);
						_pc++;
            goto aggDec;
						break;
					case 0x36:    // LD (IX+n),n
						PutValue(IX_OFFSET,Pipe2.b.h);
						_pc+=2;
						break;
					case 0x39:    // ADD IX,SP
            res2.x=_sp;
            goto aggSommaIX;
						break;
#ifdef Z80_EXTENDED
					case 0x44:    // LD r,IXh
					case 0x4c:
					case 0x54:
					case 0x5c:
					case 0x7c:
						WORKING_REG = _ixh;
						break;
					case 0x45:    // LD r,IXl
					case 0x4d:
					case 0x55:
					case 0x5d:
					case 0x7d:
						WORKING_REG = _ixl;
						break;
#endif
					case 0x46:    // LD r,(IX+n)
					case 0x4e:
					case 0x56:
					case 0x5e:
					case 0x66:
					case 0x6e:
					case 0x7e:
						WORKING_REG = GetValue(IX_OFFSET);
						_pc++;
						break;

#ifdef Z80_EXTENDED
					case 0x60:    // LD IXh,r
					case 0x61:
					case 0x62:
					case 0x63:
					case 0x67:
						_ixh=WORKING_REG2;
						break;
					case 0x64:
						_ixh=_ixh;
						break;
					case 0x65:
						_ixh=_ixl;
						break;

					case 0x68:    // LD IXl,r
					case 0x69:
					case 0x6a:
					case 0x6b:
					case 0x6f:
						_ixh=WORKING_REG2;
						break;
					case 0x6c:
						_ixl=_ixh;
						break;
					case 0x6d:
						_ixl=_ixl;
						break;
#endif

					case 0x70:    // LD (IX+n),r
					case 0x71:
					case 0x72:
					case 0x73:
					case 0x74:
					case 0x75:
					case 0x77:
						PutValue(IX_OFFSET,WORKING_REG2);
						_pc++;
						break;

#ifdef Z80_EXTENDED
					case 0x84:    // ADD A,IXh
            res2.b.l=_ixh;
            goto aggSomma;
						break;
					case 0x85:    // ADD A,IXl
            res2.b.l=_ixl;
            goto aggSomma;
						break;
#endif
					case 0x86:    // ADD A,(IX+n)
            res2.b.l=GetValue(IX_OFFSET);
						_pc++;
            goto aggSomma;
						break;
#ifdef Z80_EXTENDED
					case 0x8c:    // ADC A,IXh
            res2.b.l=_ixh;
            goto aggSommaC;
						break;
					case 0x8d:    // ADC A,IXl
            res2.b.l=_ixl;
            goto aggSommaC;
						break;
#endif
					case 0x8e:    // ADC A,(IX+n)
						res2.b.l=GetValue(IX_OFFSET);
						_pc++;
            goto aggSommaC;
						break;

#ifdef Z80_EXTENDED
					case 0x94:    // SUB IXh
						res2.b.l=_ixh;
            goto aggSottr;
						break;
					case 0x95:    // SUB IXl
						res2.b.l=_ixl;
            goto aggSottr;
						break;
#endif
					case 0x96:    // SUB A,(IX+n)
						res2.b.l=GetValue(IX_OFFSET);
						_pc++;
            goto aggSottr;
						break;
#ifdef Z80_EXTENDED
					case 0x9c:    // SBC A,IXh
						res2.b.l=_ixh;
            goto aggSottrC;
						break;
					case 0x9d:    // SBC A,IXl
						res2.b.l=_ixl;
            goto aggSottrC;
						break;
#endif
					case 0x9e:    // SBC A,(IX+n)
						res2.b.l=GetValue(IX_OFFSET);
						_pc++;
            goto aggSottrC;
						break;

#ifdef Z80_EXTENDED
					case 0xa4:    // AND A,IXh
						_a &= _ixh;
            _f.HalfCarry=1;
            goto aggAnd;
						break;
					case 0xa5:    // AND A,IXl
						_a &= _ixl;
            _f.HalfCarry=1;
            goto aggAnd;
						break;
#endif
					case 0xa6:    // AND A,(IX+n)
						_a &= GetValue(IX_OFFSET);
            _f.HalfCarry=1;
						_pc++;
            goto aggAnd;
						break;
#ifdef Z80_EXTENDED
					case 0xac:    // XOR A,IXh
						_a ^= _ixh;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
					case 0xad:    // XOR A,IXl
						_a ^= _ixl;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
#endif
					case 0xae:    // XOR A,(IX+n)
						_a ^= GetValue(IX_OFFSET);
            _f.HalfCarry=0;
						_pc++;
            goto aggAnd;
						break;

#ifdef Z80_EXTENDED
					case 0xb4:    // OR A,IXh
						_a |= _ixh;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
					case 0xb5:    // OR A,IXl
						_a |= _ixl;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
#endif
					case 0xb6:    // OR A,(IX+n)
						_a |= GetValue(IX_OFFSET);
            _f.HalfCarry=0;
						_pc++;
            goto aggAnd;
						break;

#ifdef Z80_EXTENDED
					case 0xbc:    // CP IXh
						res2.b.l=_ixh;
						goto compare;
						break;
					case 0xbd:    // CP IXl
						res2.b.l=_ixl;
						goto compare;
						break;
#endif
					case 0xbe:    // CP (IX+n)
						res2.b.l=GetValue(IX_OFFSET);
						_pc++;
						goto compare;
						break;

					case 0xe1:    // POP IX
						_ix=GetValue(_sp++);
						_ix |= ((SWORD)GetValue(_sp++)) << 8;
						break;
					case 0xe3:    // EX (SP),IX
						res3.x=GetIntValue(_sp);
						PutIntValue(_sp,_ix);
						_ix=res3.x;
						break;
					case 0xe5:    // PUSH IX
						PutValue(--_sp,HIBYTE(_ix));
						PutValue(--_sp,LOBYTE(_ix));
						break;
					case 0xe9:    // JP (IX)
					  _pc=_ix;
						break;
					case 0xf9:    // LD SP,IX
					  _sp=_ix;
						break;

					default:
//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
            Nop();
						break;
					}
				break;

			case 0xde:    // SBC A,n
				_pc++;
				res2.b.l=Pipe2.b.l;
        goto aggSottrC;
				break;

			case 0xe0:    // RET po
			  if(!_f.PV)
			    goto Return;
				break;

			case 0xe2:    // JP po
			  if(!_f.PV)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xe3:    // EX (SP),HL
				res3.x=GetIntValue(_sp);
				PutIntValue(_sp,_hl);
				_hl=res3.x;
				break;

			case 0xe4:    // CALL po
			  if(!_f.PV)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xe6:    // AND A,n
				_a &= Pipe2.b.l;
        _f.HalfCarry=1;
				_pc++;
        goto aggAnd;
				break;

			case 0xe8:    // RET pe
			  if(_f.PV)
			    goto Return;
				break;

			case 0xe9:    // JP (HL)
			  _pc=_hl;
				break;

			case 0xea:    // JP pe
			  if(_f.PV)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xeb:    // EX DE,HL
				res3.x=_de;
				_de=_hl;
				_hl=res3.x;
				break;

			case 0xec:    // CALL pe
			  if(_f.PV)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xed:      // ED instructions
				switch(GetPipe(_pc++)) {
#ifdef Z80_EXTENDED
					case 0x00:    // IN0
					case 0x08:
					case 0x10:
					case 0x18:
					case 0x20:
					case 0x28:
					case 0x38:
						_c=InValue(Pipe2.b.l);    // 
            _pc++;
						break;
					case 0x01:    // OUT0
					case 0x09:
					case 0x11:
					case 0x19:
					case 0x21:
					case 0x29:
					case 0x39:
						OutValue(Pipe2.b.l,_c);   // 
            _pc++;
						break;
#endif
#ifdef Z80_EXTENDED
					case 0x04:    // TST
					case 0x0C:
					case 0x14:
					case 0x1C:
					case 0x24:
					case 0x2C:
					case 0x3C:
            res3.b.l=_a & WORKING_REG;
            goto aggBit;
						break;
					case 0x34:
            res3.b.l=_a & GetValue(_hl);
            goto aggBit;
						break;
					case 0x64:
            res3.b.l=_a & Pipe2.b.l;
            _pc++;
            goto aggBit;
						break;
					case 0x74:// boh?!
            _pc++;
						break;
#endif

					case 0x40:    // IN r,(C)
					case 0x48:
					case 0x50:
					case 0x58:
					case 0x60:
					case 0x68:
					case 0x78:
						WORKING_REG=InValue(_bc);    // 
            res3.b.l=WORKING_REG;
            //_f.HalfCarry= ???
            goto aggAnd3;
						break;
#ifdef Z80_EXTENDED
					case 0x70:    // IN (C)
            res3.b.l=InValue(_bc);    // verificare...
            goto aggAnd3;// verificare flag qua!
						break;
#endif

					case 0x41:    // OUT (C),r
					case 0x49:
					case 0x51:
					case 0x59:
					case 0x61:
					case 0x69:
					case 0x79:
						OutValue(_bc,WORKING_REG);   // 
						break;
#ifdef Z80_EXTENDED
					case 0x71:    // OUT (C),0
						OutValue(_bc,0);      // verificare...
						break;
#endif

					case 0x42:    // SBC HL,BC ecc
					case 0x52:
					case 0x62:
            res1.x=_hl;
            res2.x=WORKING_REG16;
            res3.d=(DWORD)res1.x-(DWORD)res2.x-_f.Carry;
    			  _hl = res3.x;
            
aggSottr16:
            _f.HalfCarry = ((res1.x & 0xfff) - (res2.x & 0xfff)) & 0xf000 ? 1 : 0;   // 
            _f.AddSub=1;
//        _f.PV = !!(((res1.b.h & 0x40) + (res2.b.h & 0x40)) & 0x80) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
/*            if((res1.b.h & 0x80) != (res2.b.h & 0x80)) {
              if(((res1.b.h & 0x80) && !(res3.b.h & 0x80)) || (!(res1.b.h & 0x80) && (res3.b.h & 0x80)))
                _f.PV=1;
              else
                _f.PV=0;
              }
            else
              _f.PV=0;*/
            
aggFlagW:
//            _f.PV = !!(((res1.b.h & 0x40) + (res2.b.h & 0x40)) & 0x40) != !!(((res1.b.h & 0x80) + (res2.b.h & 0x80)) & 0x80);
    //        _f.PV = !!((res1.b.h ^ res3.b.h) & (res2.b.h ^ res3.b.h) & 0x80);
//            _f.PV = !!(!((res1.b.h ^ res2.b.h) & 0x80) && ((res1.b.h ^ res3.b.h) & 0x80));
//**            _f.PV = ((res1.b.h ^ res3.b.h) & (res2.b.h ^ res3.b.h) & 0x80) ? 1 : 0;
    //#warning flag per word o byte?? boh?!
//            _f.PV = !!(((res1.b.h & 0x40) + (res2.b.h & 0x40)) & 0x80) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
            _f.PV = !!HIWORD(res3.x) != !!((res3.b.h & 0x80) ^ (res1.b.h & 0x80) ^ (res2.b.h & 0x80));

            _f.Zero=res3.x ? 0 : 1;
            _f.Sign=res3.b.h & 0x80 ? 1 : 0;

aggFlagWC:
     				_f.Carry=!!HIWORD(res3.d);
//i flag tutti solo per ADC/SBC se no solo carry/halfcarry
						break;
					case 0x4a:    // ADC HL,BC ecc
					case 0x5a:
					case 0x6a:
            res1.x=_hl;
            res2.x=WORKING_REG16;
            res3.d=(DWORD)res1.x+(DWORD)res2.x+_f.Carry;
            _hl = res3.x;
            
aggSomma16:            
            _f.HalfCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
//        _f.PV = !!(((res1.b.h & 0x40) + (res2.b.h & 0x40)) & 0x80) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
/*            if(res1.b.h & 0x80 && res2.b.h & 0x80)
              _f.PV=res3.b.h & 0x80 ? 0 : 1;
            else if(!(res1.b.h & 0x80) && !(res2.b.h & 0x80))
              _f.PV=res3.b.h & 0x80 ? 1 : 0;
            else
              _f.PV=0;*/
            _f.AddSub=0;
						goto aggFlagW;
						break;
					case 0x72:      // SBC HL,SP
            res1.x=_hl;
            res2.x=_sp;
            res3.d=(DWORD)res1.x-(DWORD)res2.x-_f.Carry;
    			  _hl = res3.x;
						goto aggSottr16;
						break;
					case 0x7a:      // ADC HL,SP
            res1.x=_hl;
            res2.x=_sp;
            res3.d=(DWORD)res1.x+(DWORD)res2.x+_f.Carry;
            _hl = res3.x;
						goto aggSomma16;
						break;

					case 0x43:    // LD (nn),BC ecc
					case 0x53:
					case 0x63:	  // per HL � undocumented...
						PutIntValue(Pipe2.x,WORKING_REG16);
						_pc+=2;
						break;
					case 0x73:    // LD (nn),SP
						PutIntValue(Pipe2.x,_sp);		// 
						_pc+=2;
						break;
            
					case 0x4b:    // LD BC,(nn) ecc
					case 0x5b:
					case 0x6b:		// per HL � undocumented...
						WORKING_REG16=GetIntValue(Pipe2.x);
						_pc+=2;
						break;
					case 0x7b:    // LD SP,(nn)
						_sp=GetIntValue(Pipe2.x);		// 
						_pc+=2;
						break;

					case 0x44:    // NEG
						res1.b.l=0;
						res2.b.l=_a;
						res1.b.h=res2.b.h=0;
						res3.x=res1.x-res2.x;
						_f.Carry= _a ? 1 : 0;
		        _f.PV = _a == 0x80 ? 1 : 0;
//#warning            P = 1 if A = 80H before, else 0 E/MA altrove dice overflow...
						_a = res3.b.l;
            _f.AddSub=1;
            _f.HalfCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
		        _f.Zero=res3.b.l ? 0 : 1;
				    _f.Sign=res3.b.l & 0x80 ? 1 : 0;
						break;

					case 0x45:    // RETN
/*					case 0x55:
					case 0x65:
					case 0x75:
					case 0x5d:
					case 0x6d:
					case 0x7d:*/
//						_fIRQ=_f1;			//VERIFICARE! bah fondamentalmente non mi sembra serva...
						_pc=GetValue(_sp++);
						_pc |= ((SWORD)GetValue(_sp++)) << 8;
						IRQ_Enable1=IRQ_Enable2;
						break;
					case 0x4d:			// RETI
						_pc=GetValue(_sp++);
						_pc |= ((SWORD)GetValue(_sp++)) << 8;
						break;

					case 0x46:    // IM
					case 0x56:
//					case 0x66:
//					case 0x76:
						IRQ_Mode=Pipe1 & 0x10 ? 1: 0;
						break;
#ifdef Z80_EXTENDED
					case 0x4c:    // MLT
            res3.x=regs1.r[0].b.l*regs1.r[0].b.h;
            _bc=res3.x;
						break;    // no flags
					case 0x5c:
            res3.x=regs1.r[1].b.l*regs1.r[1].b.h;
            _de=res3.x;
						break;
					case 0x6c:
            res3.x=regs1.r[2].b.l*regs1.r[2].b.h;
            _hl=res3.x;
						break;
					case 0x7c:
            res3.x=LOBYTE(_sp)*HIBYTE(_sp);
            _sp=res3.x;
						break;
#endif
					case 0x5e:    // IM 2
						IRQ_Mode=2;
#ifdef Z80_EXTENDED
//					case 0x7e:
//						IRQ_Mode=Pipe2.b.l & 0x10 ? 2 : -1 /*boh! ma cmq solo se extended ??? */;
#endif
						break;

					case 0x47:    // LD i
						_i=_a;
						break;
					case 0x57:
						_a=_i;
						_f.PV=IRQ_Enable2;
						break;

					case 0x4f:    // LD r
						_r=_a;
						break;
					case 0x5f:
						_a=_r;
						_f.PV=IRQ_Enable2;
						break;

					case 0x67:    // RRD
						res3.b.l=GetValue(_hl);     // verificare...
						PutValue(_hl,(res3.b.l & 0xf) | ((_a & 0xf) << 4));
						_a = (_a & 0xf0) | (res3.b.l & 0xf);
            res3.b.l=_a;
            goto aggRotate2;
						break;
					case 0x6f:    // RLD
						res3.b.l=GetValue(_hl);
						PutValue(_hl,((res3.b.l & 0xf) << 4) | (_a & 0xf));
						_a = (_a & 0xf0) | ((res3.b.l & 0xf0) >> 4);
            res3.b.l=_a;
            goto aggRotate2;
						break;

#ifdef Z80_EXTENDED
					case 0x76:      // SLP
						break;
#endif
            
#ifdef Z80_EXTENDED
					case 0x83:      // OTIM
						break;
					case 0x8B:      // OTDM
						break;
					case 0x93:      // OTIMR
						break;
					case 0x9B:      // OTDMR
						break;
#endif
            
					case 0xa0:    // LDI
						PutValue(_de++,GetValue(_hl++));
aggLDI:
						_bc--;
            _f.AddSub=_f.HalfCarry=0;
						_f.PV=!!_bc;
						break;
					case 0xa1:    // CPI
						_bc--;
						res1.b.l=_a;
						res2.b.l=GetValue(_hl++);
            
aggCPI:
						res1.b.h=res2.b.h=0;
						res3.x=res1.x-res2.x;
            _f.AddSub=1;
            _f.HalfCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
            _f.Zero=res3.b.l ? 0 : 1;
            _f.Sign=res3.b.l & 0x80 ? 1 : 0;
						_f.PV=!!_bc;
						break;
					case 0xa2:    // INI
						PutValue(_hl++,InValue(_c));
						_b--;
						_f.Zero=!_b;
            _f.AddSub=1;
						break;
					case 0xa3:    // OUTI
						OutValue(_c,GetValue(_hl++));
						_b--;
						_f.Zero=!_b;
            _f.AddSub=0;
						break;

					case 0xb0:      // LDIR
						PutValue(_de++,GetValue(_hl++));
						_bc--;
						if(_bc)
							_pc-=2;			// cos� ripeto e consento IRQ...
            _f.AddSub=_f.HalfCarry=0;
            _f.PV=0;
						break;
					case 0xb1:    // CPIR
						res1.b.l=_a;
						res2.b.l=GetValue(_hl++);
            _bc--;
						if(res1.b.l != res2.b.l) {
              if(_bc) 
                _pc-=2;			// cos� ripeto e consento IRQ...
              }
            goto aggCPI;    // 
						break;
					case 0xb2:    // INIR
						PutValue(_hl++,InValue(_c));
						_b--;
						if(_b)
							_pc-=2;			// cos� ripeto e consento IRQ...
						_f.Zero=!_b;
						_f.AddSub=1;  // in teoria solo alla fine... ma ok
						break;
					case 0xb3:    // OTIR
						OutValue(_c,GetValue(_hl++));
						_b--;
						if(_b)
							_pc-=2;			// cos� ripeto e consento IRQ...
						_f.Zero=!_b;
						_f.AddSub=1;  // in teoria solo alla fine... ma ok
						break;

					case 0xa8:    // LDD
						PutValue(_de--,GetValue(_hl--));
						goto aggLDI;
						break;
					case 0xa9:    // CPD
						_bc--;
						res1.b.l=_a;
						res2.b.l=GetValue(_hl--);
            goto aggCPI;    // 
						break;
					case 0xaa:    // IND
						PutValue(_hl--,InValue(_bc));
						_b--;
						_f.Zero=!_b;
            _f.AddSub=1;
						break;
					case 0xab:    // OUTD
						OutValue(_bc,GetValue(_hl--));
						_b--;
						_f.Zero=!_b;
            _f.AddSub=1;
						break;

					case 0xb8:    // LDDR
						PutValue(_de--,GetValue(_hl--));
						_bc--;
						if(_bc)
							_pc-=2;			// cos� ripeto e consento IRQ...
            _f.PV=0;
						break;
					case 0xb9:    // CPDR
						res1.b.l=_a;
						res2.b.l=GetValue(_hl--);
            _bc--;
						if(res1.b.l != res2.b.l) {
              if(_bc) 
                _pc-=2;			// cos� ripeto e consento IRQ...
              }
            goto aggCPI;    // 
						break;
					case 0xba:    // INDR
						PutValue(_hl--,InValue(_c));
						_b--;
						if(_b)
							_pc-=2;			// cos� ripeto e consento IRQ...
						_f.Zero=!_b;
            _f.AddSub=1;
						break;
					case 0xbb:    // OTDR
						OutValue(_c,GetValue(_hl--));
						_b--;
						if(_b)
							_pc-=2;			// cos� ripeto e consento IRQ...
						_f.Zero=!_b;
            _f.AddSub=1;
						break;

					default:
//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
            Nop();
						break;
					}
				break;

			case 0xee:    // XOR A,n
				_a ^= Pipe2.b.l;
        _f.HalfCarry=0;
				_pc++;
        goto aggAnd;
				break;

			case 0xf0:    // RET p
			  if(!_f.Sign)
			    goto Return;
				break;

			case 0xf2:    // JP p
			  if(!_f.Sign)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xf3:    // DI
			  IRQ_Enable1=IRQ_Enable2=0;
				break;

			case 0xf4:    // CALL p
			  if(!_f.Sign)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xf6:    // OR A,n
				_a |= Pipe2.b.l;
        _f.HalfCarry=0;
				_pc++;
        goto aggAnd;
				break;

			case 0xf8:    // RET m
			  if(_f.Sign)
			    goto Return;
				break;

			case 0xf9:    // LD SP,HL
			  _sp=_hl;
				break;

			case 0xfa:    // JP m
			  if(_f.Sign)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xfb:    // EI
//When an EI instruction is executed, any pending interrupt request is not accepted until after the
//instruction following EI is executed. This single instruction delay is necessary when the
//next instruction is a return instruction
			  IRQ_Enable1=IRQ_Enable2=1;
        inEI=1;
				break;

			case 0xfc:    // CALL m
			  if(_f.Sign)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xfd:
				switch(GetPipe(_pc++)) {
					case 0xcb:
#define IY_OFFSET (_iy+((signed char)Pipe2.b.l))
#define WORKING_REG_FD_CB regs1.b[Pipe2.b.h & 7]
						switch(Pipe2.b.h) {			// il 4� byte!
#ifdef Z80_EXTENDED
							case 0x00:    // RLC (IY+
							case 0x01:
							case 0x02:
							case 0x03:
							case 0x04:
							case 0x05:
							case 0x07:
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x80 ? 1 : 0;
								WORKING_REG_FD_CB <<= 1;
								WORKING_REG_FD_CB |= _f.Carry;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x06:    // RLC (IY+)
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								res3.b.l |= _f.Carry;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x08:    // RRC (IY+
							case 0x09:
							case 0x0a:
							case 0x0b:
							case 0x0c:
							case 0x0d:
							case 0x0f:
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x1;
								WORKING_REG_FD_CB >>= 1;
								if(_f.Carry)
									WORKING_REG_FD_CB |= 0x80;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x0e:    // RRC (IY+)
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								if(_f.Carry)
									res3.b.l |= 0x80;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x10:    // RL
							case 0x11:
							case 0x12:
							case 0x13:
							case 0x14:
							case 0x15:
							case 0x17:
								_f1=_f;
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x80 ? 1 : 0;
								WORKING_REG_FD_CB <<= 1;
								WORKING_REG_FD_CB |= _f1.Carry;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x16:    // RL (IY+)
								_f1=_f;
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								res3.b.l |= _f1.Carry;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x18:    // RR
							case 0x19:
							case 0x1a:
							case 0x1b:
							case 0x1c:
							case 0x1d:
							case 0x1f:
								_f1=_f;
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x1;
								WORKING_REG_FD_CB >>= 1;
								if(_f1.Carry)
									WORKING_REG_FD_CB |= 0x80;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x1e:    // RR (IY+)
								_f1=_f;
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								if(_f1.Carry)
									res3.b.l |= 0x80;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x20:    // SLA (IY+
							case 0x21:
							case 0x22:
							case 0x23:
							case 0x24:
							case 0x25:
							case 0x27:
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x80 ? 1 : 0;
								WORKING_REG_FD_CB <<= 1;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x26:    // SLA (IY+
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x28:    // SRA (IY+
							case 0x29:
							case 0x2a:
							case 0x2b:
							case 0x2c:
							case 0x2d:
							case 0x2f:
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x1;
								WORKING_REG_FD_CB >>= 1;
								if(WORKING_REG_FD_CB & 0x40)
									WORKING_REG_FD_CB |= 0x80;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x2e:    // SRA (IY+)
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								if(res3.b.l & 0x40)
									res3.b.l |= 0x80;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x30:    // SLL (IY+
							case 0x31:
							case 0x32:
							case 0x33:
							case 0x34:
							case 0x35:
							case 0x37:
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x80 ? 1 : 0;
								WORKING_REG_FD_CB <<= 1;
								WORKING_REG_FD_CB |= 1;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
							case 0x36:    // SLL (IY)
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 0x80 ? 1 : 0;
								res3.b.l <<= 1;
								res3.b.l |= 1;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;
#endif

#ifdef Z80_EXTENDED
							case 0x38:    // SRL (IY+
							case 0x39:
							case 0x3a:
							case 0x3b:
							case 0x3c:
							case 0x3d:
							case 0x3f:
								WORKING_REG_FD_CB = GetValue(IY_OFFSET);
								_f.Carry=WORKING_REG_FD_CB & 0x1;
								WORKING_REG_FD_CB >>= 1;
                res3.b.l=WORKING_REG_FD_CB;
        				_pc+=2;
                goto aggRotate2;
								break;
#endif
							case 0x3e:    // SRL (IY+)
								res3.b.l = GetValue(IY_OFFSET);
								_f.Carry=res3.b.l & 1;
								res3.b.l >>= 1;
								PutValue(IY_OFFSET,res3.b.l);
        				_pc+=2;
                goto aggRotate2;
								break;

#ifdef Z80_EXTENDED
							case 0x40:    // BIT (IY+
							case 0x41:
							case 0x42:
							case 0x43:
							case 0x44:
							case 0x45:
							case 0x47:
							case 0x48:
							case 0x49:
							case 0x4a:
							case 0x4b:
							case 0x4c:
							case 0x4d:
							case 0x4f:
							case 0x50:
							case 0x51:
							case 0x52:
							case 0x53:
							case 0x54:
							case 0x55:
							case 0x57:
							case 0x58:
							case 0x59:
							case 0x5a:
							case 0x5b:
							case 0x5c:
							case 0x5d:
							case 0x5f:
							case 0x60:
							case 0x61:
							case 0x62:
							case 0x63:
							case 0x64:
							case 0x65:
							case 0x67:
							case 0x68:
							case 0x69:
							case 0x6a:
							case 0x6b:
							case 0x6c:
							case 0x6d:
							case 0x6f:
							case 0x70:
							case 0x71:
							case 0x72:
							case 0x73:
							case 0x74:
							case 0x75:
							case 0x77:
							case 0x78:
							case 0x79:
							case 0x7a:
							case 0x7b:
							case 0x7c:
							case 0x7d:
							case 0x7f:
                res3.b.l=GetValue(IY_OFFSET) & WORKING_BITPOS2;
        				_pc+=2;
                goto aggBit;
								break;
#endif

							case 0x46:    // BIT (IY+)
							case 0x4e:
							case 0x56:
							case 0x5e:
							case 0x66:
							case 0x6e:
							case 0x76:
							case 0x7e:
                res3.b.l=GetValue(IY_OFFSET) & WORKING_BITPOS2;
        				_pc+=2;
                goto aggBit;
								break;


#ifdef Z80_EXTENDED
							case 0x80:    // RES
							case 0x81:
							case 0x89:
							case 0x91:
							case 0x99:
							case 0xa1:
							case 0xa9:
							case 0xb1:
							case 0xb9:
							case 0x88:
							case 0x90:
							case 0x98:
							case 0xa0:
							case 0xa8:
							case 0xb0:
							case 0xb8:
							case 0x82:
							case 0x8a:
							case 0x92:
							case 0x9a:
							case 0xa2:
							case 0xaa:
							case 0xb2:
							case 0xba:
							case 0x83:
							case 0x8b:
							case 0x93:
							case 0x9b:
							case 0xa3:
							case 0xab:
							case 0xb3:
							case 0xbb:
							case 0x84:
							case 0x8c:
							case 0x94:
							case 0x9c:
							case 0xa4:
							case 0xac:
							case 0xb4:
							case 0xbc:
							case 0x85:
							case 0x8d:
							case 0x95:
							case 0x9d:
							case 0xa5:
							case 0xad:
							case 0xb5:
							case 0xbd:
							case 0x87:
							case 0x8f:
							case 0x97:
							case 0x9f:
							case 0xa7:
							case 0xaf:
							case 0xb7:
							case 0xbf:
								res3.b.l=GetValue(IY_OFFSET);
								res3.b.l &= ~ WORKING_BITPOS2;
								WORKING_REG_FD_CB = res3.b.l;
								break;
#endif

							case 0x86:    // RES (IY+)
							case 0x8e:
							case 0x96:
							case 0x9e:
							case 0xa6:
							case 0xae:
							case 0xb6:
							case 0xbe:
								res3.b.l=GetValue(IY_OFFSET);
								res3.b.l &= ~ WORKING_BITPOS2;
								PutValue(IY_OFFSET,res3.b.l);
								break;

#ifdef Z80_EXTENDED
							case 0xc0:    // SET
							case 0xc8:
							case 0xd0:
							case 0xd8:
							case 0xe0:
							case 0xe8:
							case 0xf0:
							case 0xf8:
							case 0xc1:
							case 0xc9:
							case 0xd1:
							case 0xd9:
							case 0xe1:
							case 0xe9:
							case 0xf1:
							case 0xf9:
							case 0xc2:
							case 0xca:
							case 0xd2:
							case 0xda:
							case 0xe2:
							case 0xea:
							case 0xf2:
							case 0xfa:
							case 0xc3:
							case 0xcb:
							case 0xd3:
							case 0xdb:
							case 0xe3:
							case 0xeb:
							case 0xf3:
							case 0xfb:
							case 0xc4:
							case 0xcc:
							case 0xd4:
							case 0xdc:
							case 0xe4:
							case 0xec:
							case 0xf4:
							case 0xfc:
							case 0xc5:
							case 0xcd:
							case 0xd5:
							case 0xdd:
							case 0xe5:
							case 0xed:
							case 0xf5:
							case 0xfd:
							case 0xc7:
							case 0xcf:
							case 0xd7:
							case 0xdf:
							case 0xe7:
							case 0xef:
							case 0xf7:
							case 0xff:
								res3.b.l=GetValue(IY_OFFSET);
								res3.b.l |= WORKING_BITPOS2;
								WORKING_REG_FD_CB = res3.b.l;
								break;
#endif

							case 0xc6:    // SET (IY+)
							case 0xce:
							case 0xd6:
							case 0xde:
							case 0xe6:
							case 0xee:
							case 0xf6:
							case 0xfe:
								res3.b.l=GetValue(IY_OFFSET);
								res3.b.l |= WORKING_BITPOS2;
								PutValue(IY_OFFSET,res3.b.l);
								break;

							default:
		//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
                Nop();
								break;
							}
						_pc+=2;
						break;
            
					case 0x09:    // ADD IY,BC ecc
					case 0x19:
            res2.x=WORKING_REG16;
            
aggSommaIY:            
            res1.x=_iy;
            res3.d=(DWORD)res1.x+(DWORD)res2.x;
    			  _iy = res3.x;
            _f.HalfCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
            _f.AddSub=0;
            goto aggFlagWC;
						break;
					case 0x21:    // LD IY,nn
						_iy = Pipe2.x;
						_pc+=2;
						break;
					case 0x22:    // LD (nn),IY
						PutIntValue(Pipe2.x,_iy);
            _pc+=2;
						break;
					case 0x23:    // INC IY
						_iy++;
						break;
#ifdef Z80_EXTENDED
					case 0x24:    // INC IYh
						_iyh++;
            res3.b.l=_iyh;
						goto aggInc;
						break;
					case 0x25:    // DEC IYh
						_iyh--;
            res3.b.l=_iyh;
						goto aggDec;
						break;
					case 0x26:    // LD IYh,n
						_iyh=Pipe2.b.l;
						_pc++;
						break;
#endif
					case 0x29:    // ADD IY,IY
            res2.x=_iy;
            goto aggSommaIY;
						break;
					case 0x2a:    // LD IY,(nn)
						_iy = GetIntValue(Pipe2.x);
						_pc+=2;
						break;
					case 0x2b:    // DEC IY
						_iy--;
						break;
#ifdef Z80_EXTENDED
					case 0x2c:    // INC IYl
						_iyl++;
            res3.b.l=_iyl;
						goto aggInc;
						break;
					case 0x2d:    // DEC IYl
						_iyl--;
            res3.b.l=_iyl;
						goto aggDec;
						break;
					case 0x2e:    // LD IYl,n
						_iyl=Pipe2.b.l;
						_pc++;
						break;
#endif
					case 0x34:    // INC (IY+)
						res3.b.l = GetValue(IY_OFFSET);
						res3.b.l++;
						PutValue(IY_OFFSET,res3.b.l);
						_pc++;
						goto aggInc;
						break;
					case 0x35:    // DEC (IY+)
						res3.b.l = GetValue(IY_OFFSET);
						res3.b.l--;
						PutValue(IY_OFFSET,res3.b.l);
						_pc++;
						goto aggDec;
						break;
					case 0x36:    // LD (IY),n
						PutValue(IY_OFFSET,Pipe2.b.h);
						_pc+=2;
						break;
					case 0x39:    // ADD IY,SP
            res2.x=_sp;
            goto aggSommaIY;
						break;
#ifdef Z80_EXTENDED
					case 0x44:    // LD r,IYh
					case 0x4c:
					case 0x54:
					case 0x5c:
					case 0x7c:
						WORKING_REG = _iyh;
						break;
					case 0x45:    // LD r,IYl
					case 0x4d:
					case 0x55:
					case 0x5d:
					case 0x7d:
						WORKING_REG = _iyl;
						break;
#endif
					case 0x46:    // LD r,(IY+n)
					case 0x4e:
					case 0x56:
					case 0x5e:
					case 0x66:
					case 0x6e:
					case 0x7e:    // LD A,(IY+n)
						WORKING_REG = GetValue(IY_OFFSET);
						_pc++;
						break;

#ifdef Z80_EXTENDED
					case 0x60:    // LD IYh,r
					case 0x61:
					case 0x62:
					case 0x63:
					case 0x67:
						_iyh=WORKING_REG2;
						break;
					case 0x64:    // 
						_iyh=_iyh;
						break;
					case 0x65:    // 
						_iyh=_iyl;
						break;
#endif
#ifdef Z80_EXTENDED
					case 0x68:    // LD IYl,r
					case 0x69:
					case 0x6a:
					case 0x6b:
					case 0x6f:
						_iyl=WORKING_REG2;
						break;
					case 0x6c:    // 
						_iyl=_iyh;
						break;
					case 0x6d:    // 
						_iyl=_iyl;
						break;
#endif

					case 0x70:    // LD (IY+n),r
					case 0x71:
					case 0x72:
					case 0x73:
					case 0x74:
					case 0x75:
					case 0x77:
						PutValue(IY_OFFSET,WORKING_REG2);
						_pc++;
						break;

#ifdef Z80_EXTENDED
					case 0x84:    // ADD A,IYh
            res2.b.l=_iyh;
						goto aggSomma;
						break;
					case 0x85:    // ADD A,IYl
            res2.b.l=_iyl;
						goto aggSomma;
						break;
#endif
					case 0x86:    // ADD A,(IY+n)
						_pc++;
            res2.b.l=GetValue(IY_OFFSET);
						goto aggSomma;
						break;
#ifdef Z80_EXTENDED
					case 0x8c:    // ADD A,IYh
						res2.b.l = _iyh;
						goto aggSommaC;
						break;
					case 0x8d:    // ADD A,IYl
						res2.b.l = _iyl;
						goto aggSommaC;
						break;
#endif
					case 0x8e:    // ADC A,(IY+n)
						res2.b.l = GetValue(IY_OFFSET);
						_pc++;
						goto aggSommaC;
						break;
#ifdef Z80_EXTENDED
					case 0x94:    // SUB A,IYh
						res2.b.l = _iyh;
						goto aggSottr;
						break;
					case 0x95:    // SUB A,IYl
						res2.b.l = _iyl;
						goto aggSottr;
						break;
#endif
					case 0x96:    // SUB A,(IY+n)
						res2.b.l = GetValue(IY_OFFSET);
						_pc++;
						goto aggSottr;
						break;
#ifdef Z80_EXTENDED
					case 0x9c:    // SBC A,IYh
						res2.b.l = _iyh;
						goto aggSottrC;
						break;
					case 0x9d:    // SBC A,IYl
						res2.b.l = _iyl;
						goto aggSottrC;
						break;
#endif
					case 0x9e:    // SBC A,(IY+n)
						res2.b.l = GetValue(IY_OFFSET);
						_pc++;
						goto aggSottrC;
						break;
#ifdef Z80_EXTENDED
					case 0xa4:    // AND IYh
						_a &= _iyh;
            _f.HalfCarry=1;
            goto aggAnd;
						break;
					case 0xa5:    // AND IYl
						_a &= _iyl;
            _f.HalfCarry=1;
            goto aggAnd;
						break;
#endif
					case 0xa6:    // AND (IY+n)
						_a &= GetValue(IY_OFFSET);
            _f.HalfCarry=1;
						_pc++;
            goto aggAnd;
						break;
#ifdef Z80_EXTENDED
					case 0xac:    // XOR IYh
						_a ^= _iyh;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
					case 0xad:    // XOR IYl
						_a ^= _iyl;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
#endif
					case 0xae:    // XOR (IY+n)
						_a ^= GetValue(IY_OFFSET);
            _f.HalfCarry=0;
						_pc++;
            goto aggAnd;
						break;
#ifdef Z80_EXTENDED
					case 0xb4:    // OR IYh
						_a |= _iyh;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
					case 0xb5:    // OR IYl
						_a |= _iyl;
            _f.HalfCarry=0;
            goto aggAnd;
						break;
#endif
					case 0xb6:    // OR (IY+n)
						_a |= GetValue(IY_OFFSET);
            _f.HalfCarry=0;
						_pc++;
            goto aggAnd;
						break;
#ifdef Z80_EXTENDED
					case 0xbc:    // CP IYh
						res2.b.l=_iyh;
						goto compare;
						break;
					case 0xbd:    // CP IYl
						res2.b.l=_iyl;
						goto compare;
            break;
#endif
					case 0xbe:    // CP (IY+n)
						res2.b.l=GetValue(IY_OFFSET);
						_pc++;
						goto compare;
						break;

					case 0xe1:    // POP IY
						_iy=GetValue(_sp++);
						_iy |= ((SWORD)GetValue(_sp++)) << 8;
						break;
					case 0xe3:    // EX (SP),IY
						res3.x=GetIntValue(_sp);
						PutIntValue(_sp,_iy);
						_iy=res3.x;
						break;
					case 0xe5:    // PUSH IY
						PutValue(--_sp,HIBYTE(_iy));
						PutValue(--_sp,LOBYTE(_iy));
						break;
					case 0xe9:    // JP (IY)
					  _pc=_iy;
						break;
					case 0xf9:    // LD SP,IY
					  _sp=_iy;
						break;

					default:
//				wsprintf(myBuf,"Istruzione sconosciuta a %04x: %02x",_pc-1,GetValue(_pc-1));
            Nop();
						break;
					}
				break;

			case 0xfe:    // CP n
				res2.b.l=Pipe2.b.l;
				_pc++;
        
compare:        
				res1.b.l=_a;
				res1.b.h=res2.b.h=0;
				res3.x=res1.x-res2.x;
        _f.HalfCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.PV=1;
          else
            _f.PV=0;
          }
        else
          _f.PV=0;*/
        _f.PV = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
        _f.AddSub=1;
  			goto aggFlagBC;
				break;
			
			}
		} while(!fExit);
	}


