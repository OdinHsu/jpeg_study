//
//  encoder.h
//
//  Created by Liu Yo Chung on 2014/7/4.
//  Copyright (c) 2014 SunplusMedia All rights reserved.
//

#ifndef ENCODER_H
#define ENCODER_H

#include "event.h"
#include "jpeg_enc.h"

#define MAX_WIDTH  1920
#define MAX_HEIGHT 1200

#define MAX_SCREEN_BLOCK  9000 // 1920x1200
#define BLOCK_WIDTH       32
#define BLOCK_HEIGHT      8
#define RAW_DEPTH         4

#define NUM_ENCODER       3
#define NUM_SCREEN        2
#define ENC_THREADS       4

#define QUALITY_RAW       0
#define QUALITY_LOW       1
#define QUALITY_MEDIAN    2
#define QUALITY_HIGH      3
#define QUALITY_CLONE     4

#define COLOR_SIZE        (BLOCK_WIDTH*BLOCK_HEIGHT)
#define BLOCK_SIZE        (COLOR_SIZE*3) // YUV 444
#define BGRX_SIZE         (COLOR_SIZE*4) // BGRX

#define _PACK(index,quality) (int)(((index)<<3)|(quality))
#define _UNPACK(index,quality,val) { index = ((val)>>3)&0x0000FFFF; quality = val&0x7; }

typedef struct {
    BYTE*       pframebuffer;                   // RGB buffer, planar 256*4 bytes per block
	BYTE*       pencode_buffer;
	UINT32*     pheader;
    INT32       encode_count;
    UINT32      encode_list[MAX_SCREEN_BLOCK]; // block_index[31:2], encode quality[1:0]
	BYTE        quality[MAX_SCREEN_BLOCK];     // save quality value of each block
	BYTE		buff_idx[MAX_SCREEN_BLOCK];
} screen_t;

class EncodeUnit {
public:
	EncodeUnit();
	~EncodeUnit();

	void SetEncodeQuality(int Low,int Medium,int High);
	int  GetQuantTable(unsigned char* p_table,int len,int index);
	void Setup(int start,int end,int size,screen_t* pScreen,bool first=0);
	int  EncodeBlocks();

	void ThreadStart();
	void Trigger();
	void WaitComplete();
	void ThreadEnd();
	void WaitThreadEnd()
	{
		WaitEvent(m_done);
	}

private:
	cinfo_t   	*JPEGEncoder;

	screen_t	*m_pScreen;

	int     	block_start;
	int     	block_end;
	int     	data_size;

	bool    	done;
	bool    	m_first;

	event_t     m_start;
	event_t     m_ready;

	event_t     m_done;

private:

	int     	enc_jpeg(int index,int quality);
	int     	enc_raw(int index);
};

class BlockEncoder
{
public:
	BlockEncoder();
	~BlockEncoder();

private:
	screen_t  	screen[2];
	screen_t  	*p_analysis;
	screen_t  	*p_compress;

	int			m_width;
	int		  	m_height;
	int       	m_hBlocks;			// block count in horizontal
	int       	m_vBlocks;			// block count in vertical
	int		  	m_hLastBlackValid;
	int		  	m_vLastBlackValid;
	int       	m_totalBlocks;

    int		   	m_pitch;

	// extra encode thread
	pthread_t  	m_thread;

	// for rotation use only
	UINT32     	*work;

	BYTE		*yuv_buffer;

	BYTE		*encode_buffer;
	UINT32		*header;

	JSAMPROW   	pRGBRow[BLOCK_HEIGHT];
	JSAMPROW   	pYRow[BLOCK_HEIGHT];
	JSAMPROW   	pURow[BLOCK_HEIGHT];
	JSAMPROW   	pVRow[BLOCK_HEIGHT];
	JSAMPARRAY 	component[3];

private:

	void update_block(int x,int y,BYTE* framebuffer);

	void init_screen(screen_t *screen);
	void uninit_screen(screen_t *screen);

	unsigned int get_bitstream(BYTE* bitstream);

	EncodeUnit m_EncodeUnit[ENC_THREADS];

public:

	/*
	 *  Setup geometry of input image
	 */
	void SetInputImageGeometry(int width,int height,int pitch);

	/*
	 *  Setup encode quality
	 */
	void SetEncoderQuality(int low,int medium,int high)
	{
		for(int i=0;i<ENC_THREADS;i++) {
			m_EncodeUnit[i].SetEncodeQuality(low,medium,high);
		};
	};

	int GetQuantTable(unsigned char* p_table,int len,int index)
	{
		return m_EncodeUnit[0].GetQuantTable(p_table,len,index);
	};


	unsigned int Analysis(BYTE* framebuffer,bool first);

	/*
	 *  Compress all blocks in framebuffer
	 */
	unsigned int Compress(BYTE* bitstream);

	/*
	 *  Toggle working screen of dual buffer
	 */
	void ToggleScreen();

private:

	bool CheckFrameBufferBlock(int x,int y,BYTE* framebuffer);  //BRG to RGB
	void UpdateFrameBufferBlock(int x,int y,BYTE* framebuffer);  //BRG to RGB


};

#endif
