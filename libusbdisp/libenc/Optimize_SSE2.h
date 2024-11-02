//
//  Optimize_SSE2.h
//  jpegenc
//
//  Created by Jumplux MacBook Pro 15 on 2015/3/5.
//  Copyright (c) 2015年 Jumplux MacBook Pro 15. All rights reserved.
//

#ifndef jpegenc_Optimize_SSE2_h
#define jpegenc_Optimize_SSE2_h

#include "jpeg_enc.h"
#ifdef _SSE2_

#include <emmintrin.h>
//#define _WRITEBITS_ASM_
#define _SSE2_LOAD_ADD_128_
//#define _SSE2_WRITEBLOCK_  // best
#define _SSE2_PDU_


#ifdef _WRITEBITS_ASM_
#ifdef _X86_64_
#define writebits writebits64
#endif
void writebits(bitstring bs,cinfo_t *cinfo);
#define _WRITE_BITS_
#else
#if (defined WIN32)
void __fastcall writebits(bitstring bs,cinfo_t *cinfo)
#else
void writebits(bitstring bs,cinfo_t *cinfo)
#endif
{
    int    i,len,shift;
    BYTE*  p;
    WORD   value;
#ifdef _X86_64_
    INT64  data;
#else
    DWORD  data;
#endif
    
    len   = bs & 0xFF;       // length;
    value = (WORD)(bs>>len); // left alignment to WORD
    
    if( (cinfo->wordpos + len) < BUF_BITS ) { // 32 or 64
        cinfo->wordnew = (cinfo->wordnew << len) | (value >> (16-len));
        cinfo->wordpos += len;
    } else { // >= 32 or 64
        shift = BUF_BITS - cinfo->wordpos;
        data = (cinfo->wordnew << shift) | (value >> (16-shift));
        
        // output 4 bytes
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

#define _WRITE_BITS_
#endif


//-------------------------------------------------------------------

#ifdef _SSE2_LOAD_ADD_128_
void load_data_units_from_buffer(cinfo_t *cinfo,WORD xpos,WORD ypos)
{   // Optimize by sse2 intrinsics
    DWORD location;
    int   i;
    __m128i r1,r2,r3;
    __m128i *poutY  = (__m128i*) cinfo->DU_DCTY;
    __m128i *poutCb = (__m128i*) cinfo->DU_DCTCb;
    __m128i *poutCr = (__m128i*) cinfo->DU_DCTCr;
    __m128i *pin;
    
    location = (ypos*cinfo->Ximage) + xpos;
    r2 = _mm_set1_epi16(0x8000);
    
    for( i=0;i<8;i++) {
        pin  = (__m128i*) &cinfo->Y_buffer[location];
        
        r3 = _mm_loadu_si128(pin);
        r1 = _mm_unpacklo_epi8(r2,r3);
        r1 = _mm_add_epi16(r1, r2);
        r1 = _mm_srai_epi16(r1, 8);
        _mm_store_si128(poutY++, r1);
        
        pin  = (__m128i*) &cinfo->Cb_buffer[location];
        
        r3 = _mm_loadu_si128(pin);
        r1 = _mm_unpacklo_epi8(r2,r3);
        r1 = _mm_add_epi16(r1, r2);
        r1 = _mm_srai_epi16(r1, 8);
        _mm_store_si128(poutCb++, r1);
        
        pin  = (__m128i*) &cinfo->Cr_buffer[location];
        
        r3 = _mm_loadu_si128(pin);
        r1 = _mm_unpacklo_epi8(r2,r3);
        r1 = _mm_add_epi16(r1, r2);
        r1 = _mm_srai_epi16(r1, 8);
        _mm_store_si128(poutCr++, r1);
        
        location += cinfo->Ximage;
    }
    
}
#define _LOAD_DATA_UNIT_
#endif

//-------------------------------------------------------------------

#ifdef  _SSE2_WRITEBLOCK_

#define write_rle(x) cinfo->RLE[nRLE++] = (x)
void writeblock(cinfo_t *cinfo,int nRLE);

void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
{
    int   i,nrzeroes,mask;
    int   k,n,value;
    unsigned int result;
    
    DWORD       cat;
    bitstring  data;

    bitstring *HTDC=HT->DC;
    bitstring *HTAC=HT->AC;
    
    const bitstring EOB=HTAC[0x00];
    const bitstring M16zeroes=HTAC[0xF0];
    
    __m128i *p;
    __m128i r1,r2,r3;
    INT16*  pZDU;
    int     nRLE=0;
    
    // ZDU : zigzag order of DU
    for(i=0;i<64;i+=4) {
        cinfo->ZDU[zigzag[i]]   = DU[i];   cinfo->ZDU[zigzag[i+1]] = DU[i+1];
        cinfo->ZDU[zigzag[i+2]] = DU[i+2]; cinfo->ZDU[zigzag[i+3]] = DU[i+3];
    }
    
    cinfo->ZDU[0] = DU[0] - *DC;
    *DC           = DU[0];
    
    r2 = _mm_set1_epi16(0x0000);
    p = (__m128i*) cinfo->ZDU;
    
    r1 = _mm_load_si128(p++);    // load data 0~7
    r3 = _mm_load_si128(p++);    // load data 8~15
    
    r1 = _mm_cmpeq_epi16(r1, r2);
    result = _mm_movemask_epi8(r1)&0xFFFF;
    
    r3 = _mm_cmpeq_epi16(r3, r2);
    result |= _mm_movemask_epi8(r3)<<16;
    
    result = ~result;
    
    //Encode DC
    if(cinfo->ZDU[0]==0) {
        write_rle(HTDC[0]); // Diff might be 0
    } else {
        mask  = ((int)cinfo->ZDU[0])>>31;  // -1 or 0
        value = cinfo->ZDU[0] + mask;
        cat   = (BYTE)(32 - clz(value^mask));
        data  = (value<<16)|cat;
        
        write_rle(HTDC[cat]);
        write_rle(data);
    }
    result >>= 2;
    
    // Encode AC
    i=1;
    while(result) { // 1~15
        n = ctz(result)/2;
        
        mask  = ((int)cinfo->ZDU[i+n])>>31;  // -1 or 0
        value = cinfo->ZDU[i+n] + mask;
        cat   = (BYTE)(32 - clz(value^mask));
        write_rle(HTAC[n*16+cat]);
        write_rle((value<<16)|cat);
        result >>= (n+1)*2;
        i += n+1;
    };
    nrzeroes = (16-i);
    
    for(k=1;k<4;k++) { // 16~63
        pZDU = (INT16*) p;
        i  = 0;
        r1 = _mm_load_si128(p++);
        r3 = _mm_load_si128(p++);
        
        r1 = _mm_cmpeq_epi16(r1, r2);
        result = _mm_movemask_epi8(r1)&0xFFFF;
        
        r3 = _mm_cmpeq_epi16(r3, r2);
        result |= _mm_movemask_epi8(r3)<<16;
        
        result = ~result;
        
        while(result!=0 && i<16) {
            n = ctz(result)/2;
            nrzeroes += n;
            if (nrzeroes>=16) {
                switch(nrzeroes/16) {
                    case  3:write_rle(M16zeroes);
                    case  2:write_rle(M16zeroes);
                    default:write_rle(M16zeroes);
                }
                nrzeroes=nrzeroes%16;
            };
            
            mask  = ((int)(pZDU[i+n]))>>31;  // -1 or 0
            value = pZDU[i+n] + mask;
            cat   = (BYTE)(32 - clz(value^mask));
            write_rle(HTAC[nrzeroes*16+cat]);
            write_rle((value<<16)|cat);
            nrzeroes = 0;
            result >>= (n+1)*2;
            i += (n+1);
        }
        nrzeroes += (16-i);
    };
    if (cinfo->ZDU[63] == 0) write_rle(EOB); // last DU is zero
        
#if 1
    for(i = 0; i< nRLE ;i++) writebits(cinfo->RLE[i],cinfo);
#else
    writeblock(cinfo,nRLE);
#endif
#define _PROCESS_DU_
    
}

#endif

#ifdef _SSE2_PDU_

void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
{
    int   i,nrzeroes,mask;
    int   k,n,value;
    unsigned int result;
    
    DWORD       cat;
    bitstring  data;
    
    bitstring *HTDC=HT->DC;
    bitstring *HTAC=HT->AC;

    const bitstring EOB=HTAC[0x00];
    const bitstring M16zeroes=HTAC[0xF0];
    
    __m128i *p;
    __m128i r1,r2;
    
    INT16 *pZDU;
    
    // ZDU : zigzag order of DU
    for(i=0;i<64;i+=4) {
        cinfo->ZDU[zigzag[i]]   = DU[i];   cinfo->ZDU[zigzag[i+1]] = DU[i+1];
        cinfo->ZDU[zigzag[i+2]] = DU[i+2]; cinfo->ZDU[zigzag[i+3]] = DU[i+3];
    }
    
    cinfo->ZDU[0] = DU[0] - *DC;
    *DC           = DU[0];
    
    r2 = _mm_set1_epi16(0x0000);
    p = (__m128i*) cinfo->ZDU;
    
    r1 = _mm_load_si128(p++);    // load data 0~7
    r1 = _mm_cmpeq_epi16(r1, r2);
    
    result = _mm_movemask_epi8(r1)&0xFFFF;
    
    r1 = _mm_load_si128(p++);    // load data 8~15
    r1 = _mm_cmpeq_epi16(r1, r2);
    
    result |= _mm_movemask_epi8(r1)<<16;
    
    result = ~result;
    
    //Encode DC
    if(cinfo->ZDU[0]==0) {
        writebits(HTDC[0],cinfo); // Diff might be 0
    } else {
        mask  = ((int)cinfo->ZDU[0])>>31;  // -1 or 0
        value = cinfo->ZDU[0] + mask;
        cat   = (BYTE)(32 - clz(value^mask));
        data  = (value<<16)|cat;
        
        writebits(HTDC[cat],cinfo);
        writebits(data,cinfo);
    }
    result >>= 2;
    
    // Encode AC
    i=1;
    while(result) { // 1~15
        n = ctz(result)/2;
        
        mask  = ((int)cinfo->ZDU[i+n])>>31;  // -1 or 0
        value = cinfo->ZDU[i+n] + mask;
        cat   = (BYTE)(32 - clz(value^mask));
        data  = (value<<16)|cat;
        writebits(HTAC[n*16+cat],cinfo);
        writebits(data,cinfo);
        i += n+1;
        result >>= (n+1)*2;
    }
    nrzeroes = (16-i);
    
    for(k=1;k<4;k++) { // 16~63
        pZDU = (INT16*) p;
        r1 = _mm_load_si128(p++);
        r1 = _mm_cmpeq_epi16(r1, r2);
        
        result = _mm_movemask_epi8(r1)&0xFFFF;
        
        r1 = _mm_load_si128(p++);
        r1 = _mm_cmpeq_epi16(r1, r2);
        
        result |= _mm_movemask_epi8(r1)<<16;
        
        result = ~result;
        
        i=0;
        while(result&&(i<16)) {
            n = ctz(result)/2;
            nrzeroes += n;
            if (nrzeroes>=16) {
                switch(nrzeroes/16) {
                    case  3:writebits(M16zeroes,cinfo);
                    case  2:writebits(M16zeroes,cinfo);
                    default:writebits(M16zeroes,cinfo);
                }
                nrzeroes=nrzeroes%16;
            };
            pZDU += n; // Skip n data;
            
            mask  = ((int)(*pZDU))>>31;  // -1 or 0
            value = (*pZDU) + mask;
            cat   = (BYTE)(32 - clz(value^mask));
            data  = (value<<16)|cat;
            writebits(HTAC[nrzeroes*16+cat],cinfo);
            writebits(data,cinfo);
            i += (n+1);
            pZDU++; // next data
            result >>= (n+1)*2;
            
            nrzeroes = 0;
        }
        nrzeroes += (16-i);
    }
    if (cinfo->ZDU[63] == 0) writebits(EOB,cinfo); // last DU is zero
    
}

#define _PROCESS_DU_
#endif

#endif
#endif

