/***************************************************************************/
/*  File: jpeg_enc.cpp                                                        */
/*  Date: 20-Jun-2014                                                      */
/*                                                                         */
/***************************************************************************/
/*
C source of a JPEG encoder.

The principles of designing JPEG encoder were mainly clarity and portability.
Kept as simple as possibly, using basic c in most places for portability.

*/
/***************************************************************************/

//#include "stdafx.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#if (defined WIN32 || defined WIN64)

#include <intrin.h>
#pragma intrinsic(_BitScanReverse)

#endif

#include "jpeg_enc.h"

/***************************************************************************/

#ifndef _NO_HEADER_

static struct APP0infotype APP0info = { 0xFFE0,16,{'J','F','I','F',0},1,1,0,1,1,0,0 };
static struct SOF0infotype SOF0info = { 0xFFC0,17,8,0,0,3,1,0x11,0,2,0x11,1,3,0x11,1};
static struct SOSinfotype  SOSinfo  = { 0xFFDA,12,3,1,0,2,0x11,3,0x11,0,0x3F,0 };

#endif
// DHT for all encoder
static struct DHTinfotype DHTinfo;

static WORD zigzag[64]={
	      0, 1, 5, 6,14,15,27,28,
		  2, 4, 7,13,16,26,29,42,
		  3, 8,12,17,25,30,41,43,
		  9,11,18,24,31,40,44,53,
		 10,19,23,32,39,45,52,54,
		 20,22,33,38,46,51,55,60,
		 21,34,37,47,50,56,59,61,
		 35,36,48,49,57,58,62,63 
};

//These are the sample quantization tables given in JPEG spec section K.1.
//	The spec says that the values given produce "good" quality, and
//	when divided by 2, "very good" quality
static BYTE std_luminance_qt[64] = {
	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55,
	14,  13,  16,  24,  40,  57,  69,  56,
	14,  17,  22,  29,  51,  87,  80,  62,
	18,  22,  37,  56,  68, 109, 103,  77,
	24,  35,  55,  64,  81, 104, 113,  92,
	49,  64,  78,  87, 103, 121, 120, 101,
	72,  92,  95,  98, 112, 100, 103,  99
};

static BYTE std_chrominance_qt[64] = {
	17,  18,  24,  47,  99,  99,  99,  99,
	18,  21,  26,  66,  99,  99,  99,  99,
	24,  26,  56,  99,  99,  99,  99,  99,
	47,  66,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99
};

// Standard Huffman tables (cf. JPEG standard section K.3)
static BYTE std_dc_luminance_nrcodes[17] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
static BYTE std_dc_luminance_values[12]  = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static BYTE std_dc_chrominance_nrcodes[17] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
static BYTE std_dc_chrominance_values[12]  = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static BYTE std_ac_luminance_nrcodes[17] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static BYTE std_ac_luminance_values[162] = {
	  0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	  0xf9, 0xfa };

static BYTE std_ac_chrominance_nrcodes[17] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
static BYTE std_ac_chrominance_values[162] = {
	  0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	  0xf9, 0xfa 
};

// The Huffman tables we'll use:
static HuffmanTable_t Y_HT;
static HuffmanTable_t Cb_HT;


#include "Optimize_SSE2.h"
#include "Optimize_NEON.h"

/***************************************************************************/

#ifndef _NO_HEADER_
//Nothing to overwrite for APP0info
void write_APP0info(jpeg_stream_t* jpeg_stream)
{
	writeword(jpeg_stream,APP0info.marker);
	writeword(jpeg_stream,APP0info.length);
	writebyte(jpeg_stream,'J');
	writebyte(jpeg_stream,'F');
	writebyte(jpeg_stream,'I');
	writebyte(jpeg_stream,'F');
	writebyte(jpeg_stream,0);

	writebyte(jpeg_stream,APP0info.versionhi);
	writebyte(jpeg_stream,APP0info.versionlo);
	writebyte(jpeg_stream,APP0info.xyunits);
	writeword(jpeg_stream,APP0info.xdensity);
	writeword(jpeg_stream,APP0info.ydensity);
	writebyte(jpeg_stream,APP0info.thumbnwidth);
	writebyte(jpeg_stream,APP0info.thumbnheight);
}

// We should overwrite width and height
void write_SOF0info(cinfo_t* cinfo)
{
	writeword(&cinfo->jpeg_stream,SOF0info.marker);
	writeword(&cinfo->jpeg_stream,SOF0info.length);
	writebyte(&cinfo->jpeg_stream,SOF0info.precision);
	writeword(&cinfo->jpeg_stream,SOF0info.height);
	writeword(&cinfo->jpeg_stream,SOF0info.width);
	writebyte(&cinfo->jpeg_stream,SOF0info.nrofcomponents);
	writebyte(&cinfo->jpeg_stream,SOF0info.IdY);
	writebyte(&cinfo->jpeg_stream,SOF0info.HVY);
	writebyte(&cinfo->jpeg_stream,SOF0info.QTY);
	writebyte(&cinfo->jpeg_stream,SOF0info.IdCb);
	writebyte(&cinfo->jpeg_stream,SOF0info.HVCb);
	writebyte(&cinfo->jpeg_stream,SOF0info.QTCb);
	writebyte(&cinfo->jpeg_stream,SOF0info.IdCr);
	writebyte(&cinfo->jpeg_stream,SOF0info.HVCr);
	writebyte(&cinfo->jpeg_stream,SOF0info.QTCr);
}

//Nothing to overwrite for SOSinfo
void write_SOSinfo(cinfo_t* cinfo)
{
	writeword(&cinfo->jpeg_stream,SOSinfo.marker);
	writeword(&cinfo->jpeg_stream,SOSinfo.length);
	writebyte(&cinfo->jpeg_stream,SOSinfo.nrofcomponents);
	writebyte(&cinfo->jpeg_stream,SOSinfo.IdY);
	writebyte(&cinfo->jpeg_stream,SOSinfo.HTY);
	writebyte(&cinfo->jpeg_stream,SOSinfo.IdCb);
	writebyte(&cinfo->jpeg_stream,SOSinfo.HTCb);
	writebyte(&cinfo->jpeg_stream,SOSinfo.IdCr);
	writebyte(&cinfo->jpeg_stream,SOSinfo.HTCr);
	writebyte(&cinfo->jpeg_stream,SOSinfo.Ss);
	writebyte(&cinfo->jpeg_stream,SOSinfo.Se);
	writebyte(&cinfo->jpeg_stream,SOSinfo.Bf);
}

void write_comment(jpeg_stream_t* jpeg_stream,BYTE *comment)
{
	WORD i,length;
	writeword(jpeg_stream,0xFFFE); //The COM marker
    
	//length=(WORD)strlen((const char *)comment);
	for(length = 0;comment[length]!=0;) length++;
    
	writeword(jpeg_stream,length+2);
	for (i=0;i<length;i++) writebyte(jpeg_stream,comment[i]);
}

#endif

int write_DQTinfo(cinfo_t *cinfo,jpeg_stream_t* stream)
{
	int  i;
	int  len = stream->pos;

	writeword(stream,cinfo->DQTinfo.marker);
	writeword(stream,67);
	writebyte(stream,cinfo->DQTinfo.QTYinfo); 
	for (i=0;i<64;i++) writebyte(stream,cinfo->DQTinfo.Ytable[i]);
	writeword(stream,cinfo->DQTinfo.marker);
	writeword(stream,67);
	writebyte(stream,cinfo->DQTinfo.QTCbinfo);
	for (i=0;i<64;i++) writebyte(stream,cinfo->DQTinfo.Cbtable[i]);

	return stream->pos - len;
}

// SPD615 DQT format
int GetDQTinfo(cinfo_t *cinfo,jpeg_stream_t* stream)
{
	int  i;
	int  len = stream->pos;

	writeword(stream,0xFFDB); // 2
	writeword(stream,67);     // 2
	writebyte(stream,00);     // 1
	for (i=0;i<64;i++) writebyte(stream,cinfo->DQTinfo.Ytable[zigzag[i]]); // 64
	writeword(stream,0xFFDB); // 2
	writeword(stream,67);     // 2
	writebyte(stream,01);     // 1
	for (i=0;i<64;i++) writebyte(stream,cinfo->DQTinfo.Cbtable[zigzag[i]]); // 64

	return stream->pos - len;
}

int write_DHTinfo(jpeg_stream_t* stream )
{
	int  i;
	int  len = stream->pos;

	writeword(stream,DHTinfo.marker);
	writeword(stream,DHTinfo.length);
	writebyte(stream,DHTinfo.HTYDCinfo);
	for (i=0;i<16;i++)  writebyte(stream,DHTinfo.YDC_nrcodes[i]);
	for (i=0;i<=11;i++) writebyte(stream,DHTinfo.YDC_values[i]);
	writebyte(stream,DHTinfo.HTYACinfo);
	for (i=0;i<16;i++)   writebyte(stream,DHTinfo.YAC_nrcodes[i]);
	for (i=0;i<=161;i++) writebyte(stream,DHTinfo.YAC_values[i]);
	writebyte(stream,DHTinfo.HTCbDCinfo);
	for (i=0;i<16;i++)   writebyte(stream,DHTinfo.CbDC_nrcodes[i]);
	for (i=0;i<=11;i++)  writebyte(stream,DHTinfo.CbDC_values[i]);
	writebyte(stream,DHTinfo.HTCbACinfo);
	for (i=0;i<16;i++)   writebyte(stream,DHTinfo.CbAC_nrcodes[i]);
	for (i=0;i<=161;i++) writebyte(stream,DHTinfo.CbAC_values[i]);

	return stream->pos - len;
}

// Set quantization table and zigzag reorder it
void set_quant_table(BYTE *basic_table,BYTE scale_factor,BYTE *newtable)
{
	BYTE i;
	long temp;

	for (i = 0; i < 64; i++) {
		temp = ((long) basic_table[i] * scale_factor + 50L) / 100L;
		//limit the values to the valid range
		if (temp <= 0L) temp = 1L;
		if (temp > 255L) temp = 255L; //limit to baseline range if requested
		newtable[zigzag[i]] = (BYTE) temp;
	}
}

void set_DQTinfo(cinfo_t *cinfo,BYTE quality)
{
	BYTE scalefactor=(100-quality)*2;	// scalefactor controls the visual quality of the image
										// the smaller is, the better image we'll get, and the smaller
										// compression we'll achieve
	cinfo->DQTinfo.marker=0xFFDB;
	cinfo->DQTinfo.length=132;
	cinfo->DQTinfo.QTYinfo=0;
	cinfo->DQTinfo.QTCbinfo=1;
	set_quant_table(std_luminance_qt,scalefactor,cinfo->DQTinfo.Ytable);
	set_quant_table(std_chrominance_qt,scalefactor,cinfo->DQTinfo.Cbtable);
}

void set_DHTinfo()
{
	BYTE i;
	DHTinfo.marker=0xFFC4;
	DHTinfo.length=0x01A2;
	DHTinfo.HTYDCinfo=0;
	for (i=0;i<16;i++)  DHTinfo.YDC_nrcodes[i]=std_dc_luminance_nrcodes[i+1];
	for (i=0;i<=11;i++)  DHTinfo.YDC_values[i]=std_dc_luminance_values[i];
	DHTinfo.HTYACinfo=0x10;
	for (i=0;i<16;i++)  DHTinfo.YAC_nrcodes[i]=std_ac_luminance_nrcodes[i+1];
	for (i=0;i<=161;i++) DHTinfo.YAC_values[i]=std_ac_luminance_values[i];
	DHTinfo.HTCbDCinfo=1;
	for (i=0;i<16;i++)  DHTinfo.CbDC_nrcodes[i]=std_dc_chrominance_nrcodes[i+1];
	for (i=0;i<=11;i++)  DHTinfo.CbDC_values[i]=std_dc_chrominance_values[i];
	DHTinfo.HTCbACinfo=0x11;
	for (i=0;i<16;i++)  DHTinfo.CbAC_nrcodes[i]=std_ac_chrominance_nrcodes[i+1];
	for (i=0;i<=161;i++) DHTinfo.CbAC_values[i]=std_ac_chrominance_values[i];
}

#ifndef _WRITE_BITS_
void writebits(bitstring bs,cinfo_t *cinfo)
{
	WORD value;
	int  i,len,shift;
#ifdef _X86_64_
    INT64  data;
#else
    DWORD  data;
#endif
	BYTE* p;

	len   = bs & 0xFF;       // length;
	value = (WORD)(bs>>len); // left alignment to WORD

	if( (cinfo->wordpos + len) < BUF_BITS ) { // 32/64
		cinfo->wordnew = (cinfo->wordnew << len) | (value >> (16-len));
		cinfo->wordpos += len; 
	} else { // >= 32/64
		shift = BUF_BITS - cinfo->wordpos;
		data = (cinfo->wordnew << shift) | (value >> (16-shift));

		// output 4/8 bytes
		p = ((BYTE*)&data)+(BUF_BYTES-1);
		for(i=0;i<BUF_BYTES;i++) {
			writebyte(&cinfo->jpeg_stream,*p);
		    if( *p-- == 0xFF ) writebyte(&cinfo->jpeg_stream,0x00);
		}

		value <<= shift; 
		len    -= shift;

		cinfo->wordnew = value >> (16-len);
		cinfo->wordpos = len;
	} 
};

#endif

#ifndef _FLASH_BITS_

#if (defined WIN32)
static void __fastcall flashbits(cinfo_t *cinfo)
#else
static void flashbits(cinfo_t *cinfo)
#endif
{
#ifdef _X86_64_
    INT64  data  = cinfo->wordnew;
#else
    int  data  = cinfo->wordnew;
#endif
    BYTE*  p;
    if( cinfo->wordpos ){
        int   len = BUF_BITS - cinfo->wordpos;
        data = (data << len);
        p = ((BYTE*)&data)+(BUF_BYTES-1);
        while(cinfo->wordpos > 0) {
            writebyte(&cinfo->jpeg_stream,*p);
            if( *p-- == 0xFF ) writebyte(&cinfo->jpeg_stream,0x00);
            cinfo->wordpos -= 8;
        }
    }
    cinfo->wordpos = 0;
    cinfo->wordnew = 0;
}

#endif

void compute_Huffman_table(BYTE *nrcodes,BYTE *std_table,bitstring *HT)
{
	BYTE k,j;
	BYTE pos_in_table;
	WORD codevalue;
	codevalue=0; pos_in_table=0;
	for (k=1;k<=16;k++)	{
		for (j=1;j<=nrcodes[k];j++) {
			HT[std_table[pos_in_table]] = (codevalue<<16) | k;
			pos_in_table++;
			codevalue++;
		}
		codevalue*=2;
	}
}

void init_Huffman_tables()
{
	compute_Huffman_table(std_dc_luminance_nrcodes,  std_dc_luminance_values,  Y_HT.DC );
	compute_Huffman_table(std_dc_chrominance_nrcodes,std_dc_chrominance_values,Cb_HT.DC);
	compute_Huffman_table(std_ac_luminance_nrcodes,  std_ac_luminance_values,  Y_HT.AC );
	compute_Huffman_table(std_ac_chrominance_nrcodes,std_ac_chrominance_values,Cb_HT.AC);
}

void exitmessage(char *error_message)
{
	printf("%s\n",error_message);exit(EXIT_FAILURE);
}

static int flss (UINT16 val)
{
  int bit;

  if (!val) return 0;

  bit = (32 - clz(val));
  return bit;
}

static int compute_reciprocal (UINT16 divisor, DCTELEM * dtbl)
{
  UDCTELEM2 fq, fr;
  UDCTELEM c;
  int b, r;

  b = flss(divisor) - 1;
  r  = sizeof(DCTELEM) * 8 + b;

  fq = ((UDCTELEM2)1 << r) / divisor;
  fr = ((UDCTELEM2)1 << r) % divisor;

  c = divisor / 2; /* for rounding */

  if (fr == 0) { /* divisor is power of two */
    /* fq will be one bit too large to fit in DCTELEM, so adjust */
    fq >>= 1;
    r--;
  } else if (fr <= (divisor / 2U)) { /* fractional part is < 0.5 */
    c++;
  } else { /* fractional part is > 0.5 */
    fq++;
  }

  dtbl[DCTSIZE2 * 0] = (DCTELEM) fq;      /* reciprocal */
  dtbl[DCTSIZE2 * 1] = (DCTELEM) c;       /* correction + roundfactor */
  dtbl[DCTSIZE2 * 2] = (DCTELEM) (1 << (sizeof(DCTELEM)*8*2 - r));  /* scale */
  dtbl[DCTSIZE2 * 3] = (DCTELEM) r - sizeof(DCTELEM)*8; /* shift */

  if(r <= 16) return 0;
  else return 1;
}


// Using a bit modified form of the FDCT routine from IJG's C source:
// Forward DCT routine idea taken from Independent JPEG Group's C source for
// JPEG encoders/decoders

// For float AA&N IDCT method, divisors are equal to quantization
//   coefficients scaled by scalefactor[row]*scalefactor[col], where
//   scalefactor[0] = 1
//   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
//   We apply a further scale factor of 8.
//   What's actually stored is 1/divisor so that the inner loop can
//   use a multiplication rather than a division.
void prepare_quant_tables(cinfo_t *cinfo)
{
	int i;
	static const INT16 aanscales[DCTSIZE2] = {
	  /* precomputed values scaled up by 14 bits */
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	for ( i = 0; i<64; i++) {
		compute_reciprocal(
			DESCALE(((INT32) cinfo->DQTinfo.Ytable[zigzag[i]]*(INT32) aanscales[i]),CONST_BITS-3), 
			&cinfo->divisor_Y[i]
		);
		compute_reciprocal(
			DESCALE(((INT32) cinfo->DQTinfo.Cbtable[zigzag[i]]*(INT32) aanscales[i]),CONST_BITS-3), 
			&cinfo->divisor_Cb[i]
		);
	}
}

#ifndef _FDCT_AND_QUAN_
void fdct_and_quantization(INT16* divisors,SWORD *outdata)
{
    jsimd_fdct_ifast(outdata);
    jsimd_quantize(outdata, divisors,outdata );
}
#endif

#ifndef _PROCESS_DU_
void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
{
    int   i;
    int   startpos;
    int   end0pos;
    int   nrzeroes;
    int   Diff;
    int   mask;
    int   value;
    int   nrmarker;
    bitstring *HTDC=HT->DC;
    bitstring *HTAC=HTDC+12;

    DWORD cat;
    bitstring data;
    
    bitstring EOB=HTAC[0x00];
    bitstring M16zeroes=HTAC[0xF0];
    
    INT16 ZDU[64];
    
    //zigzag reorder
    for (i=0;i<=63;i++) ZDU[zigzag[i]]=DU[i];
    Diff=DU[0]-*DC;
    *DC=DU[0];
    
    //Encode DC
    
    if (Diff==0) {
        writebits(HTDC[0],cinfo); //Diff might be 0
    } else {
        
        mask = (Diff>>31);  // -1 or 0
        value = Diff + mask;
        
        cat = (BYTE)(32 - clz(value^mask));
        writebits(HTDC[cat],cinfo);
        writebits((value << 16) | (cat),cinfo);
        
    }
    
    //Encode ACs
    for (end0pos=63;(end0pos>0)&&(ZDU[end0pos]==0);end0pos--) ;
    //end0pos = first element in reverse order !=0
    if (end0pos==0) {
        writebits(EOB,cinfo);
        return;
    }
    
    i=1;
    while (i<=end0pos) {
        startpos=i;
        for (; (ZDU[i]==0)&&(i<=end0pos);i++) ;
        nrzeroes=i-startpos;
        if (nrzeroes>=16) {
            for (nrmarker=0;nrmarker<nrzeroes/16;nrmarker++) writebits(M16zeroes,cinfo);
            nrzeroes=nrzeroes%16;
        };
        mask  = ((int)ZDU[i])>>31;  // -1 or 0
        value = ZDU[i] + mask;
        cat = (BYTE)(32 - clz(value^mask));
        data = (value<<16)|cat;
        writebits(HTAC[nrzeroes*16+cat],cinfo);
        writebits(data,cinfo);
        
        i++;
    }
    
    if (end0pos!=63) writebits(EOB,cinfo);
    
}
#endif

#ifndef _LOAD_DATA_UNIT_
void load_data_units_from_buffer(cinfo_t *cinfo,WORD xpos,WORD ypos)
{
    DWORD location = (ypos*cinfo->Ximage) + xpos;;
    int   pos = 0;
    int   i;
    SBYTE YDU[64];
    SBYTE CbDU[64];
    SBYTE CrDU[64];
    
    for (i=0;i<8;i++) {
        *(INT64*) &YDU[pos]  = *(INT64*) &cinfo->Y_buffer[location]; // move 8 bytes to DU
        *(INT64*) &CbDU[pos] = *(INT64*) &cinfo->Cb_buffer[location];
        *(INT64*) &CrDU[pos] = *(INT64*) &cinfo->Cr_buffer[location];
        location += cinfo->Ximage; // jump to next line
        pos += 8;
    }
    
    for (i=0;i<64;i++) {
        cinfo->DU_DCTY[i]=(SWORD)((SBYTE)(YDU[i]+128));
        cinfo->DU_DCTCb[i]=(SWORD)((SBYTE)(CbDU[i]+128));
        cinfo->DU_DCTCr[i]=(SWORD)((SBYTE)(CrDU[i]+128));
    }
}
#endif

#ifndef _MAIN_ENCODER_
void main_encoder(cinfo_t* cinfo)
{
	SWORD DCY=0,DCCb=0,DCCr=0; //DC coefficients used for differential encoding
	WORD xpos,ypos,i;
	for (ypos=0;ypos<cinfo->Yimage;ypos+=8) {
		for (xpos=0;xpos<cinfo->Ximage;xpos+=8)	{
            load_data_units_from_buffer(cinfo,xpos,ypos);

            fdct_and_quantization(cinfo->divisor_Y, cinfo->DU_DCTY);
            fdct_and_quantization(cinfo->divisor_Cb,cinfo->DU_DCTCb);
            fdct_and_quantization(cinfo->divisor_Cb,cinfo->DU_DCTCr);
            
            process_DU(cinfo,cinfo->DU_DCTY,  &DCY ,&Y_HT);
            process_DU(cinfo,cinfo->DU_DCTCb, &DCCb,&Cb_HT);
            process_DU(cinfo,cinfo->DU_DCTCr, &DCCr,&Cb_HT);
		}
	}
}
#endif

static int initialized = 0;

void Initialize(cinfo_t* cinfo,BYTE scalefactor)
{
	if( initialized == 0 ) { // for global variables
		set_DHTinfo();
		init_Huffman_tables();
		initialized = 1;
	}
	cinfo->QualityFactor = scalefactor;
	set_DQTinfo(cinfo,scalefactor);
	prepare_quant_tables(cinfo);
}

void DeInitialize()
{
}

#ifndef _NO_HEADER_
void WriteHeader(cinfo_t* cinfo,int width,int height)
{
	SOF0info.width=width;
	SOF0info.height=height;

	// write header
	writeword(&cinfo->jpeg_stream,0xFFD8); //SOI
	write_APP0info(&cinfo->jpeg_stream);

	write_DQTinfo(cinfo,&cinfo->jpeg_stream);
	write_SOF0info(cinfo);
	write_DHTinfo(&cinfo->jpeg_stream);
	write_SOSinfo(cinfo);
}
#endif

int encode(cinfo_t* cinfo)
{
	cinfo->wordpos=0;
	cinfo->wordnew=0;

	main_encoder(cinfo);
	flashbits(cinfo); //Do the bit alignment of the EOI marker
	writeword(&cinfo->jpeg_stream,0xFFD9); //EOI

	return cinfo->jpeg_stream.pos;
}
