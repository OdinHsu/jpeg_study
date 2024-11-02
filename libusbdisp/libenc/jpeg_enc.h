/***************************************************************************/
/*  File: encoder.h                                                        */
/*  Date: 20-Jun-2014                                                      */
/*                                                                         */
/***************************************************************************/
/*
C source of a JPEG encoder.

The principles of designing JPEG encoder were mainly clarity and portability.
Kept as simple as possibly, using basic c in most places for portability.

The program should give the same results on any C compiler which provides at least
256 kb of free memory. I needed that for the precalculated bitcode (3*64k) and
category (64k) arrays.

*/
/***************************************************************************/

#ifndef JPEG_ENC_H
#define JPEG_ENC_H

//#define _SSE2_
//#define _X86_64_

//#define _NEON_

#ifdef _X86_64_
#define _64_BIT_
#endif

//#define _NO_HEADER_

// for writebits
#ifdef _64_BIT_
#define BUF_BITS 64
#else
#define BUF_BITS 32
#endif

#define BUF_BYTES (BUF_BITS/8)

#include "mm_type.h"
#include "debug_trace.h"

#ifndef _NO_HEADER_

struct APP0infotype {
		WORD marker;// = 0xFFE0
		WORD length; // = 16 for usual JPEG, no thumbnail
		BYTE JFIFsignature[5]; // = "JFIF",'\0'
		BYTE versionhi; // 1
		BYTE versionlo; // 1
		BYTE xyunits;   // 0 = no units, normal density
		WORD xdensity;  // 1
		WORD ydensity;  // 1
		BYTE thumbnwidth; // 0
		BYTE thumbnheight; // 0
};

struct  SOF0infotype {
		WORD marker; // = 0xFFC0
		WORD length; // = 17 for a truecolor YCbCr JPG
		BYTE precision ;// Should be 8: 8 bits/sample
		WORD height ;
		WORD width;
		BYTE nrofcomponents;//Should be 3: We encode a truecolor JPG
		BYTE IdY;  // = 1
		BYTE HVY; // sampling factors for Y (bit 0-3 vert., 4-7 hor.)
		BYTE QTY;  // Quantization Table number for Y = 0
		BYTE IdCb; // = 2
		BYTE HVCb;
		BYTE QTCb; // 1
		BYTE IdCr; // = 3
		BYTE HVCr;
		BYTE QTCr; // Normally equal to QTCb = 1
};

struct SOSinfotype {
		 WORD marker;  // = 0xFFDA
		 WORD length; // = 12
		 BYTE nrofcomponents; // Should be 3: truecolor JPG
		 BYTE IdY; //1
		 BYTE HTY; //0 // bits 0..3: AC table (0..3)
				   // bits 4..7: DC table (0..3)
		 BYTE IdCb; //2
		 BYTE HTCb; //0x11
		 BYTE IdCr; //3
		 BYTE HTCr; //0x11
		 BYTE Ss,Se,Bf; // not interesting, they should be 0,63,0
};
#endif

// Ytable from DQTinfo should be equal to a scaled and zizag reordered version
// of the table which can be found in "tables.h": std_luminance_qt
// Cbtable , similar = std_chrominance_qt
// We'll init them in the program using set_DQTinfo function

struct DQTinfotype {
		 WORD marker;  // = 0xFFDB
		 WORD length;  // = 132
		 BYTE QTYinfo;// = 0:  bit 0..3: number of QT = 0 (table for Y)
				  //       bit 4..7: precision of QT, 0 = 8 bit
		 BYTE Ytable[64];
		 BYTE QTCbinfo; // = 1 (quantization table for Cb,Cr}
		 BYTE Cbtable[64];
};

struct DHTinfotype {
		 WORD marker;  // = 0xFFC4
		 WORD length;  //0x01A2
		 BYTE HTYDCinfo; // bit 0..3: number of HT (0..3), for Y =0
				//bit 4  :type of HT, 0 = DC table,1 = AC table
				//bit 5..7: not used, must be 0
		 BYTE YDC_nrcodes[16]; //at index i = nr of codes with length i
		 BYTE YDC_values[12];
		 BYTE HTYACinfo; // = 0x10
		 BYTE YAC_nrcodes[16];
		 BYTE YAC_values[162];//we'll use the standard Huffman tables
		 BYTE HTCbDCinfo; // = 1
		 BYTE CbDC_nrcodes[16];
		 BYTE CbDC_values[12];
		 BYTE HTCbACinfo; //  = 0x11
		 BYTE CbAC_nrcodes[16];
		 BYTE CbAC_values[162];
};

typedef struct { BYTE B,G,R,A; } colorRGB;

typedef DWORD bitstring;

#define writebyte(jpeg_stream,b) (jpeg_stream)->pbitstream[(jpeg_stream)->pos++]=(b)
#define writeword(jpeg_stream,w) writebyte(jpeg_stream,(unsigned char)((w)/256));writebyte(jpeg_stream,(unsigned char)((w)%256));

#ifdef _SSE2_
//#define jsimd_extbgrx_ycc_convert jsimd_extbgrx_ycc_convert_sse2
#define jsimd_extbgrx_ycc_convert jsimd_extrgbx_ycc_convert_sse2
#define jsimd_fdct_ifast          jsimd_fdct_ifast_sse2
#define jsimd_quantize            jsimd_quantize_sse2
#endif

#ifdef _NEON_
#define jsimd_extbgrx_ycc_convert jsimd_extrgbx_ycc_convert_neon
//#define jsimd_extbgrx_ycc_convert jsimd_extbgrx_ycc_convert_neon
#define jsimd_fdct_ifast          jsimd_fdct_ifast_neon
#define jsimd_quantize            jsimd_quantize_neon
#endif

#define DCTSIZE2 64

typedef unsigned int JDIMENSION;
typedef unsigned char JSAMPLE;
typedef JSAMPLE  *JSAMPROW;	    /* ptr to one image row of pixel samples. */
typedef JSAMPROW *JSAMPARRAY;	/* ptr to some rows (a 2-D sample array) */
typedef JSAMPARRAY *JSAMPIMAGE;	/* a 3-D sample array: top index is color */

typedef short DCTELEM;  /* prefer 16 bit with SIMD for parellelism */
typedef unsigned short UDCTELEM;
typedef unsigned int UDCTELEM2;

#define CONST_BITS 14
#define ONE	((INT32) 1)
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)

typedef struct {
    bitstring DC[12];
    bitstring AC[256];
} HuffmanTable_t;

typedef struct {
	unsigned char *pbitstream;
	unsigned int  pos;
} jpeg_stream_t;


#if (defined WIN32) || (defined WIN64)
typedef struct __declspec(align(16)) {
#else
    typedef struct __attribute__((aligned(16))) {
#endif
        
        INT16 divisor_Y[DCTSIZE2*4];  // 64*4*2 = 512
        INT16 divisor_Cb[DCTSIZE2*4]; // 64*4*2 = 512
        
        INT16 DU_DCTY[64];   // Current DU (after DCT and quantization) which we'll zigzag, 64*2=128
        INT16 DU_DCTCb[64];  // Current DU (after DCT and quantization) which we'll zigzag, 64*2=128
        INT16 DU_DCTCr[64];  // Current DU (after DCT and quantization) which we'll zigzag, 64*2=128
        INT16 ZDU[64];       // Zigzag ordered DU, 64*2 = 128
        DWORD RLE[128];      // 128*4 = 512
        
        jpeg_stream_t jpeg_stream; // offset = 0x800
        
#ifdef _64_BIT_
        INT64 wordnew;
#else
        DWORD wordnew; // The word that will be written in the JPG file
#endif
        
        int   wordpos; // bit position in the word we write (bytenew),should be<=31 and >=0

        int   Ximage,Yimage; // image dimensions divisible by 8

        SBYTE *Y_buffer;
        SBYTE *Cb_buffer;
        SBYTE *Cr_buffer;

        int  QualityFactor;

        struct DQTinfotype DQTinfo;
        

    } cinfo_t;

#define DBG_MAIN 0x01
#define DBG_DUMP_QTABLE 0x02

#define DEBUG_LEVEL (0)
//#define DEBUG_LEVEL (DBG_DUMP_QTABLE)

#ifdef __cplusplus
extern "C" {
#endif

void Initialize(cinfo_t* cinfo,unsigned char scalefactor);
void DeInitialize();
void WriteHeader(cinfo_t* cinfo,int width,int height);
int  GetDQTinfo(cinfo_t *cinfo,jpeg_stream_t* stream);
int  write_DHTinfo(jpeg_stream_t* stream );
int  encode(cinfo_t* cinfo);

#if !((defined WIN64) || (defined _NEON_) || (defined MACHO-64))
void __fastcall writebits(bitstring bs,cinfo_t *cinfo);
#endif
    
void jsimd_extbgrx_ycc_convert
(
	JDIMENSION img_width,
	JSAMPARRAY input_buf,
	JSAMPIMAGE output_buf,
	JDIMENSION output_row,
	int num_rows
);

void jsimd_extbgr_ycc_convert
(
	JDIMENSION img_width,
	JSAMPARRAY input_buf,
	JSAMPIMAGE output_buf,
	JDIMENSION output_row,
	int num_rows
);

void jsimd_quantize
(
	INT16 * coef_block, 
	INT16 * divisors,
    INT16 * workspace
);

void jsimd_fdct_ifast(INT16 * data);
    
#if (defined WIN32 || defined WIN64)
    __inline int clz(int x)
    {
        int res;
        BitScanReverse(&res,x);
        return 31 - res;
    };
    
    __inline int ctz(int x)
    {
        int res;
        BitScanForward(&res,x);
        return res;
    }
#else
#define clz(x) __builtin_clz(x)
#define ctz(x) __builtin_ctz(x)
#endif

#ifdef __cplusplus
}
#endif
#endif //SAVEJPG_H
