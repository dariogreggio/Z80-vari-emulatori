//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#define STRICT

#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#ifndef WIN32
#include <shell.h>
#endif

#include "afxres.h"
#include "resource.h"



//#define Z80_EXTENDED 1

//#define DEBUG_TESTSUITE 1


//#define ZX80 1
//#define ZX81 1      // 
//#define SKYNET 1
//#define NEZ80 1
#define GALAKSIJA 1     // emulatore online https://galaksija.net/?i=1
                        // https://www.kernelcrash.com/blog/making-a-galaksija/2020/03/02/
											// CANCRO a antonio caradonna MORTE alla puglia!!!
//#define MSX 1
#ifdef MSX
#define TMS99xx_BASE 0x98
#define TMSVIDEORAM_SIZE 16384
#define TMS_R0_MODE_GRAPHICS_I    0x00
#define TMS_R0_MODE_GRAPHICS_II   0x02
#define TMS_R0_MODE_MULTICOLOR    0x00
#define TMS_R0_MODE_TEXT          0x00
#define TMS_R0_EXT_VDP_ENABLE     0x01
#define TMS_R0_EXT_VDP_DISABLE    0x00

#define TMS_R1_RAM_16K            0x80
#define TMS_R1_RAM_4K             0x00
#define TMS_R1_DISP_BLANK         0x00
#define TMS_R1_DISP_ACTIVE        0x40
#define TMS_R1_INT_ENABLE         0x20
#define TMS_R1_INT_DISABLE        0x00
#define TMS_R1_MODE_GRAPHICS_I    0x00
#define TMS_R1_MODE_GRAPHICS_II   0x00
#define TMS_R1_MODE_MULTICOLOR    0x08
#define TMS_R1_MODE_TEXT          0x10
#define TMS_R1_SPRITE_8           0x00
#define TMS_R1_SPRITE_16          0x02
#define TMS_R1_SPRITE_MAG1        0x00
#define TMS_R1_SPRITE_MAG2        0x01
#define LAST_SPRITE_YPOS        0xD0

#define TMS_DEFAULT_VRAM_NAME_ADDRESS          0x3800
#define TMS_DEFAULT_VRAM_COLOR_ADDRESS         0x0000
#define TMS_DEFAULT_VRAM_PATT_ADDRESS          0x2000
#define TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS   0x3B00
#define TMS_DEFAULT_VRAM_SPRITE_PATT_ADDRESS   0x1800
extern BYTE TMSVideoRAM[TMSVIDEORAM_SIZE];
#pragma pack( push, before_structs )
#pragma pack(1)
struct SPRITE_ATTR {
  BYTE  ypos,xpos;    // v. sotto, a volte usato come signed
  BYTE  name;
  union {
    struct {
      unsigned char color:4;
      unsigned char unused:3;
      unsigned char eclock:1;
      };
    BYTE  tag;
    };
  };
#pragma pack( pop, before_structs )
#endif
//#define MSX2 1


#ifdef DEBUG_TESTSUITE
#define RAM_SIZE 65536
#define ROM_SIZE 65536		// non usata cmq
#define CPU_CLOCK_DIVIDER 1000000L			// 
#define HORIZ_SIZE 256
#define VERT_SIZE (200)
#define HORIZ_OFFSCREEN 8
#define VERT_OFFSCREEN 8      // 
#define MIN_RASTER 0        // 
#define MAX_RASTER ((VERT_SIZE+VERT_OFFSCREEN*2)-1)
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2

#else

#ifdef ZX80
#define CPU_CLOCK_DIVIDER 1000000L			// così va più o meno come il PIC, su PC greggiod
#define RAM_START 0x4000
#define RAM_SIZE 1024
#define ROM_SIZE 4096				// 4K, ZX80; 8K, ZX81
#define HORIZ_SIZE 256
#define VERT_SIZE (192+8)		// non è troppo chiaro... c'è una riga in più ma se la tolgo non va
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MIN_RASTER 0        // 
#define MAX_RASTER (VERT_SIZE-1)
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2
#elif ZX81
#define CPU_CLOCK_DIVIDER 1000000L			// 
#define RAM_START 0x4000
#define RAM_SIZE 1024
#define ROM_SIZE 8192				// 4K, ZX80; 8K, ZX81
#define HORIZ_SIZE 256
#define VERT_SIZE (192+8)
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MIN_RASTER 0        // 
#define MAX_RASTER (VERT_SIZE-1)
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2
#elif SKYNET
#define CPU_CLOCK_DIVIDER 1000000L			// 
#define RAM_START 0x8000
#define RAM_SIZE 2048
#define ROM_SIZE 0x4000
#define ROM_SIZE2 0x4000
#define HORIZ_SIZE (20*8)
#define VERT_SIZE (4*8)
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define VIDEORAM_SIZE 4096
#elif NEZ80
#define CPU_CLOCK_DIVIDER 1000000L			// 
#define RAM_START 0x0000
#define RAM_SIZE 2048
#define ROM_START 0x8000
#define ROM_SIZE 0x400
//#define ROM_SIZE2 0x4000
#define HORIZ_SIZE (32*8)
#define VERT_SIZE 8
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MIN_RASTER 0        // 
#define MAX_RASTER (VERT_SIZE-1)
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2
#elif GALAKSIJA
#define CPU_CLOCK_DIVIDER 1000000L			// 
#define RAM_START 0x2800
#define RAM_SIZE (2048*3)   // max 3x6116 = 0x1800
#define ROM_START 0x0000
#define ROM_SIZE 0x1000
#define ROM_SIZE2 0x1000
#define HORIZ_SIZE 256
#define VERT_SIZE (16*13)
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define MIN_RASTER 0        // 
#define MAX_RASTER (VERT_SIZE-1)
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2
#elif MSX
#define CPU_CLOCK_DIVIDER 1000000L			// 		// va male la tastiera quando rallenti... sistemare
#define RAM_START 0x8000
#define RAM_SIZE 32768
//#define RAM_START 0xc000
//#define RAM_SIZE 16384 ma DEVE partire dal Top!!
//#define RAM_START 0xf000  // per VELOCIZZAER debug!
//#define RAM_SIZE 4096 // ma DEVE partire dal Top!!
#define RAM_SIZE2 65536
#define RAM_START2 0x0000   // in slot 2, v.sotto
#define ROM_START 0x0000
#define ROM_SIZE 32768
// OCCHIO slot!! v.sotto
#define HORIZ_SIZE 256
#define VERT_SIZE 192
#define HORIZ_OFFSCREEN 8
#define VERT_OFFSCREEN 0      // 
#define MIN_RASTER 0        // 
#define MAX_RASTER ((VERT_SIZE+VERT_OFFSCREEN*2)-1)
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2
#endif
#endif

#pragma warning FINIRE VERT_OFF e fare passate + piccole, v.


typedef unsigned short int SWORD;

#pragma pack( push, before_structs )
#pragma pack(1)
union PIPE {
	SWORD x;
	BYTE bb[4];
	struct {
		BYTE l;
		BYTE h;
//		BYTE u;		 bah no, sposto la pipe quando ci sono le istruzioni lunghe 4...
		} b;
	union {
		unsigned int reg2:3;
		unsigned int reg:3;
		};
	};

union Z_REG {
  SWORD x;
  struct { 
    BYTE l;
    BYTE h;
    } b;
//    } _bc1,_de1,_hl1,_af1,_af2,_bc2,_de2,_hl2;
  };
#define ID_CARRY 0x1
#define ID_ADDSUB 0x2
#define ID_PV 0x4
#define ID_HALFCARRY 0x10
#define ID_ZERO 0x40
#define ID_SIGN 0x80
// https://stackoverflow.com/questions/8119577/z80-daa-instruction e Undocumented: i bit non usati diventano copie dei bit del risultato ultima istruzione
union REGISTRO_F {
  BYTE b;
  struct {
    unsigned char Carry: 1;
    unsigned char AddSub: 1;
    unsigned char PV: 1;   // 1=pari 0=dispari
    unsigned char unused: 1;
    unsigned char HalfCarry: 1;
    unsigned char unused2: 1;
    unsigned char Zero: 1;
    unsigned char Sign: 1;
    };
  };
union Z_REG_FLAG {
  SWORD x;
  struct { 
    BYTE l;
		union REGISTRO_F f;
    } b;
  };
union Z_REGISTERS {
  BYTE  b[8];
	struct {
		union Z_REG r[3];
		union Z_REG_FLAG r3;
		};
  };
union OPERAND {
  BYTE *reg8;
  WORD *reg16;
  WORD mem;
  };
union RESULT {
  struct {
    BYTE l;
    BYTE h;
    } b;
  WORD x;
  DWORD d;
  };
#pragma pack( pop, before_structs )

enum {
  DoReset=1,
  DoNMI=2,
  DoIRQ=4,
  DoWait=8,
  DoHalt=16
  };


extern HWND ghWnd,hStatusWnd;
extern HINSTANCE g_hinst;
extern HANDLE hAccelTable;

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Dump0(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Dump1(HWND, UINT, WPARAM, LPARAM);
int decodeKBD(int, long, BOOL);
#ifdef NEZ80
int PlotDisplay(WORD pos,BYTE ch,BYTE c);
int UpdateScreen(HDC,WORD,WORD);
#elif SKYNET
int UpdateScreen(HDC hDC,WORD);
#endif
#ifdef ZX80
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin, BYTE _i);
#endif
#ifdef GALAKSIJA
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin);
#endif
#ifdef MSX
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin);
#endif
BYTE _fastcall GetValue(SWORD);
SWORD _fastcall GetIntValue(SWORD);
void _fastcall PutValue(SWORD, BYTE);
void _fastcall PutIntValue(SWORD, SWORD);
BYTE _fastcall InValue(SWORD);
void _fastcall OutValue(SWORD, BYTE);
BYTE _fastcall GetPipe(SWORD);
int Emulate(int);
void initHW(void);
BOOL CenterWindow(HWND, HWND);
int WritePrivateProfileInt(char *,char *,int,char *);
int Disassemble(const BYTE *src,HFILE f,char *dest,DWORD len,WORD pcaddr,BYTE opzioni);





/* turn a numeric literal into a hex constant
(avoids problems with leading zeroes)
8-bit constants max value 0b11111111, always fits in unsigned long
Tom Torfs
https://post.bytes.com/forum/topic/c/176274-binary-constant-macros
*/
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
| ((x&0x000000F0LU)?2:0) \
| ((x&0x00000F00LU)?4:0) \
| ((x&0x0000F000LU)?8:0) \
| ((x&0x000F0000LU)?16:0) \
| ((x&0x00F00000LU)?32:0) \
| ((x&0x0F000000LU)?64:0) \
| ((x&0xF0000000LU)?128:0)

/* *** user macros *** /

/* for upto 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

/* for upto 16-bit binary constants, MSB first */
#define B16(dmsb,dlsb) (((unsigned short)B8(dmsb)<<8) \
| B8(dlsb))

/* for upto 32-bit binary constants, MSB first */
#define B32(dmsb,db2,db3,dlsb) (((unsigned long)B8(dmsb)<< 24) \
| ((unsigned long)B8(db2)<<16) \
| ((unsigned long)B8(db3)<<8) \
| B8(dlsb))

