
#include <string.h>
#include "Encoder.h"
#include <malloc.h>

typedef	struct {
	unsigned int quality:     2;   // 3/2/1/0-> H/M/L/raw
	unsigned int len:         8;   // stream length,768/4=192
	unsigned int block_index:16;
	unsigned int reserved0:   1;
	unsigned int end:         1;   // 1:last block;or 0
	unsigned int id:          4;   // block header ID = 4'b1101
} blockheader_t;

static void* threadfunc(void* lpParam)
{
	EncodeUnit* pEncodeUnit = (EncodeUnit*) lpParam;
	pEncodeUnit->ThreadStart();
	return 0;
}

BlockEncoder::BlockEncoder()
{

	DBGTRACE(DBG_ENCODER,"BlockEncoder::BlockEncoder()\n");

	yuv_buffer    = (BYTE*)memalign(16,MAX_SCREEN_BLOCK*BLOCK_SIZE*2);

	encode_buffer = (BYTE*)memalign(16,MAX_SCREEN_BLOCK*BLOCK_SIZE);
	header        = (UINT32*)malloc(MAX_SCREEN_BLOCK*sizeof(UINT32));

	init_screen(&screen[0]);
	init_screen(&screen[1]);

	memset(screen[0].buff_idx,0,MAX_SCREEN_BLOCK);
	memset(screen[1].buff_idx,1,MAX_SCREEN_BLOCK);

	p_analysis = &screen[0];
	p_compress = &screen[1];

	m_width   = 0;
	m_height  = 0;
	m_pitch   = 0;
	m_hBlocks = 0;                // block count in horizontal
	m_vBlocks = 0;				  // block count in vertical
	m_hLastBlackValid = 0;
	m_vLastBlackValid = 0;

	component[0] = pYRow;
	component[1] = pURow;
	component[2] = pVRow;

	work = (UINT32*)memalign(16,sizeof(UINT32)*256);

	SetEncoderQuality(90,95,98);

	for(int i=1;i<ENC_THREADS;i++) {
		pthread_create(&m_thread,NULL,threadfunc,&m_EncodeUnit[i]);
	};

};

BlockEncoder::~BlockEncoder()
{
	DBGTRACE(DBG_ENCODER,"BlockEncoder::~BlockEncoder()\n");

	for(int i=1;i<ENC_THREADS;i++) {
		m_EncodeUnit[i].ThreadEnd();
	}
	for(int i=1;i<ENC_THREADS;i++) {
		m_EncodeUnit[i].WaitThreadEnd();
	}

	free(work);
	free(yuv_buffer);

	free(encode_buffer);
	free(header);

	uninit_screen(&screen[0]);
	uninit_screen(&screen[1]);
};


void BlockEncoder::SetInputImageGeometry(int width,int height,int pitch)
{
	m_width  = width;
	m_height = height;
	if((width == 0 ) || (height == 0)) return;
	m_pitch  = pitch;
	m_hBlocks = (width+BLOCK_WIDTH-1)/BLOCK_WIDTH;
	m_vBlocks = (height+BLOCK_HEIGHT-1)/BLOCK_HEIGHT;

	m_hLastBlackValid = width % BLOCK_WIDTH;
	m_vLastBlackValid = height % BLOCK_HEIGHT;

	m_totalBlocks = m_hBlocks*m_vBlocks;

	SetEncoderQuality(90,95,98);
}

unsigned int BlockEncoder::Analysis(BYTE* framebuffer,bool first)
{
	int x=0;
	int y=0;
	int index = 0;
	int quality = QUALITY_RAW;

	p_analysis->encode_count = 0;

	if(first) {
		for( x = 0;x < m_hBlocks;x++) {
			for(y = 0;y < m_vBlocks;y++){
				index = y*m_hBlocks+x;
				// Convert BGRA framebuffer to YCbCr and store to framebuffer
				UpdateFrameBufferBlock(x,y,framebuffer);
				quality = QUALITY_RAW;
				p_analysis->quality[index] = quality;
				p_analysis->encode_list[p_analysis->encode_count++] = _PACK(index,quality);
			}
		}
//		DBGTRACE(DBG_ALL,"BlockEncoder::Analysis(),encode_count = %d\n",p_analysis->encode_count);
	} else {
		// current update
		for( x = 0;x < m_hBlocks;x++) {
			for(y = 0;y<m_vBlocks;y++){
				index = y*m_hBlocks+x;
				// Convert BGRA framebuffer to YCbCr and store to framebuffer
				if(CheckFrameBufferBlock(x,y,framebuffer)) {
					quality = (p_analysis->buff_idx[index]==p_compress->buff_idx[index]) ? QUALITY_CLONE : QUALITY_RAW;
					p_analysis->quality[index] = quality;
					p_analysis->encode_list[p_analysis->encode_count++] = _PACK(index,quality);
				}
			}
		}
	}

	// others
	DBGTRACE(DBG_ENCODER,"BlockEncoder::Analysis(), encode_count = %d, first = %d,blocks=%d\n",p_analysis->encode_count,first,m_hBlocks*m_vBlocks);
	return p_analysis->encode_count; // total blocks need to compress
}

#define PROCESS_BLOCKS (MAX_SCREEN_BLOCK/ENC_THREADS)

unsigned int BlockEncoder::Compress(BYTE* bitstream)
{
    int     len = 0;
    int	    threads = (p_compress->encode_count/PROCESS_BLOCKS)+1;

    if( threads > ENC_THREADS ) threads = ENC_THREADS;

    int     i,start,end;
    int     packagesize = 800000/threads;

    start = 0;
    for( i=1;i<threads;i++) {
	end = start + PROCESS_BLOCKS;
	m_EncodeUnit[i].Setup(start,end,packagesize,p_compress);
	start = end;
    }
    m_EncodeUnit[0].Setup(start,p_compress->encode_count,packagesize,p_compress);

    for( i=1;i<threads;i++)  m_EncodeUnit[i].Trigger();
    m_EncodeUnit[0].EncodeBlocks();
    
    for( i=1;i<threads;i++)  m_EncodeUnit[i].WaitComplete();
    len = get_bitstream(bitstream);

    return len;
}

void BlockEncoder::ToggleScreen()
{
	if( p_analysis == &screen[0] )
	{
		p_analysis = &screen[1];
		p_compress = &screen[0];
	} else {
		p_analysis = &screen[0];
		p_compress = &screen[1];
	}
}

bool BlockEncoder::CheckFrameBufferBlock(int x,int y,BYTE* framebuffer)
{

	int  height = BLOCK_HEIGHT;
    unsigned int idx;

	UINT32* p_src;
	UINT32* p_dest;

	if( (y == m_vBlocks-1) && (m_vLastBlackValid != 0) ) height = m_vLastBlackValid;

	idx = y*m_hBlocks + x;

	x = x*BLOCK_WIDTH;	 // convert from block to pixel
	y = y*BLOCK_HEIGHT;  // convert from block number to line

	int buf_idx = (1 ^ p_compress->buff_idx[idx]) & 0x01;

	int dest = idx*2+buf_idx;
	int src  = idx*2+(1 ^ buf_idx);

    p_src  = (UINT32*) &framebuffer[(y*m_pitch+x)*RAW_DEPTH];  // block buffer
    p_dest = (UINT32*) &p_analysis->pframebuffer[dest*BLOCK_SIZE]; // linear buffer

 	for(int i=0;i<height;i++) {
		pRGBRow[i] = (BYTE*)p_src;
		pYRow[i]   = (BYTE*)p_dest;
		pVRow[i]   = pYRow[i] + COLOR_SIZE;
		pURow[i]   = pVRow[i] + COLOR_SIZE;
		p_src     += m_pitch;
		p_dest    += BLOCK_WIDTH/4;
	};

	jsimd_extbgrx_ycc_convert( BLOCK_WIDTH, pRGBRow, component, 0, height  );

	p_src  = (UINT32*) &p_analysis->pframebuffer[idx*2*BLOCK_SIZE];
	p_dest = (UINT32*) &p_analysis->pframebuffer[(idx*2+1)*BLOCK_SIZE];

	if(memcmp(p_src,p_dest,BLOCK_SIZE) == 0) {
	    bool new_block = (p_compress->buff_idx[idx] != p_analysis->buff_idx[idx]);
    	p_analysis->buff_idx[idx] = p_compress->buff_idx[idx];
		return new_block;
	} else {
		p_analysis->buff_idx[idx] = buf_idx;
	    return true;
    }
};
void BlockEncoder::UpdateFrameBufferBlock(int x,int y,BYTE* framebuffer)
{
	int  height = BLOCK_HEIGHT;
    unsigned int idx;

	UINT32* p_src;
	UINT32* p_dest;

	if( (y == m_vBlocks-1) && (m_vLastBlackValid != 0) ) height = m_vLastBlackValid;

	idx = y*m_hBlocks + x;

	x = x*BLOCK_WIDTH;	 // convert from block to pixel
	y = y*BLOCK_HEIGHT;      // convert from block number to line

	int buf_idx = (1 ^ p_compress->buff_idx[idx])&0x01;
	int dest = idx*2+buf_idx;

    p_src  = (UINT32*) &framebuffer[(y*m_pitch+x)*RAW_DEPTH];  // block buffer
    p_dest = (UINT32*) &p_analysis->pframebuffer[dest*BLOCK_SIZE]; // linear buffer

 	for(int i=0;i<height;i++) {
		pRGBRow[i] = (BYTE*)p_src;
		pYRow[i]   = (BYTE*)p_dest;
		pVRow[i]   = pYRow[i] + COLOR_SIZE;
		pURow[i]   = pVRow[i] + COLOR_SIZE;
		p_src     += m_pitch;
		p_dest    += BLOCK_WIDTH/4;
	};

	jsimd_extbgrx_ycc_convert( BLOCK_WIDTH, pRGBRow, component, 0, height  );

	p_analysis->buff_idx[idx] = buf_idx;

};


unsigned int BlockEncoder::get_bitstream(BYTE* bitstream)
{
	int    index,len,i;
	int	   quality __attribute__((unused));
	int*   pp;
	int    block_length;
    BYTE*  pstream = bitstream;
	blockheader_t*  pblock_header=NULL;

	if(p_compress->encode_count == 0) return 0;

	for(i=0;i<p_compress->encode_count;i++) {
		_UNPACK(index, quality, p_compress->encode_list[i]);

		// Tag header
		pblock_header = (blockheader_t*)pstream;
		pstream += sizeof(DWORD);

		// write header
		*(DWORD*)pblock_header =  p_compress->pheader[index];

		// get length from header
		block_length = (int)pblock_header->len + 1;

		// write payload
		memcpy(pstream,p_compress->pencode_buffer+(index*BLOCK_SIZE),block_length*4);
		pstream += block_length*4;
	}
	pblock_header->end = 1; // last header
	len = (int)(pstream - bitstream);
    pp  = (int*)pstream;

    // Padding 0xFFFFD9FF to 128 bytes alignment ( AXI burst length )
    do{
		*pp++ = 0xFFFFD9FF; // end of JPEG and end of frame
	    len  += 4;
    }while(len%128);
    // end of alignment
	return len;
};

void BlockEncoder::init_screen(screen_t *screen)
{

	screen->pframebuffer  = yuv_buffer;

	screen->pencode_buffer = encode_buffer;
	screen->pheader = header;

	screen->encode_count = 0;

};

void BlockEncoder::uninit_screen(screen_t *screen)
{
	screen->pframebuffer  = NULL;
	screen->pencode_buffer = NULL;
	screen->pheader = NULL;

	screen->encode_count  = 0;
};


//////////////////////////////////////////////////////////

EncodeUnit::EncodeUnit()
{
	BYTE QTable[NUM_ENCODER] = { 90,95,100 };
	JPEGEncoder = (cinfo_t*) memalign(16,sizeof(cinfo_t)*NUM_ENCODER);
	for (int i = 0; i < NUM_ENCODER; i++) {
		JPEGEncoder[i].Ximage = BLOCK_WIDTH;
		JPEGEncoder[i].Yimage = BLOCK_HEIGHT;
		Initialize(&JPEGEncoder[i],QTable[i]);
	};

    init_event(&m_ready);
	init_event(&m_start);

	init_event(&m_done);
	done = false;


};

EncodeUnit::~EncodeUnit()
{
	DeInitialize();
	if(JPEGEncoder) free(JPEGEncoder);

	uninit_event(&m_ready);
	uninit_event(&m_start);
	uninit_event(&m_done);

};

void EncodeUnit::SetEncodeQuality(int Low,int Medium,int High)
{
    DWORD Qindex[NUM_ENCODER];

    Qindex[0] = Low;
    Qindex[1] = Medium;
    Qindex[2] = High;

	for (int i = 0; i < NUM_ENCODER; i++) {
		JPEGEncoder[i].Ximage = BLOCK_WIDTH;
		JPEGEncoder[i].Yimage = BLOCK_HEIGHT;
		Initialize(&JPEGEncoder[i],Qindex[i]);
	};
}

int EncodeUnit::GetQuantTable(unsigned char* p_table,int len __attribute__((unused)),int index)
{
	cinfo_t *pJPEGEncoder;
	jpeg_stream_t data;
	
	data.pbitstream = p_table;
	data.pos = 0;
	if(index <=0 || index > 3) return 0;

	pJPEGEncoder = &JPEGEncoder[index-1];

	return GetDQTinfo(pJPEGEncoder,&data);
}

void EncodeUnit::Setup(int start,int end,int size,screen_t* pScreen,bool first)
{
	m_pScreen   = pScreen;
	block_start = start;
	block_end   = end;
	data_size   = size;
	m_first     = first;
};

int EncodeUnit::EncodeBlocks()
{
	int             index,quality,uncompress_data;
	blockheader_t*  pblock_header=NULL;
	int             len = 0;
	int             jpeg_block_size = 0;
	int             count = block_end-block_start;

    for(int i=block_start;i<block_end;i++,count--) {
        // get block index and quality from encode list
        _UNPACK(index, quality, m_pScreen->encode_list[i]);

        if(quality == QUALITY_CLONE) {
        	pblock_header = (blockheader_t*) &m_pScreen->pheader[index];
        	len += (pblock_header->len+2)*4;
        	continue;
        }

		m_pScreen->pheader[index] = 0xD0000000; // ID

        pblock_header = (blockheader_t*)&m_pScreen->pheader[index];
	    pblock_header->block_index = index;

		if( !m_first ) {
			uncompress_data = count*772;
			if( (uncompress_data + len) > data_size ) {
			    quality = QUALITY_MEDIAN;
			}
		}

		if( quality > QUALITY_RAW) { // JPEG
			jpeg_block_size = enc_jpeg(index,quality);
			if( jpeg_block_size > BLOCK_SIZE ) {
				quality = QUALITY_RAW;
			}
		};

		if (quality == QUALITY_RAW)  { // RAW
			jpeg_block_size = enc_raw(index);
		};

		m_pScreen->quality[index] = quality;

		pblock_header->quality  = quality;
        pblock_header->len = (jpeg_block_size/4)-1;
		len += (int)(jpeg_block_size + 4);
    };

	return len;

};

int EncodeUnit::enc_jpeg(int index,int quality)
{
	int			jpeg_block_size;
    cinfo_t*    pJPEGEncoder = &JPEGEncoder[quality-1];

    int src = index*2+m_pScreen->buff_idx[index];
    pJPEGEncoder->Y_buffer  = (SBYTE*)m_pScreen->pframebuffer+(src*BLOCK_SIZE);
	pJPEGEncoder->Cb_buffer = pJPEGEncoder->Y_buffer  + COLOR_SIZE;
	pJPEGEncoder->Cr_buffer = pJPEGEncoder->Cb_buffer + COLOR_SIZE;

	pJPEGEncoder->jpeg_stream.pbitstream = m_pScreen->pencode_buffer+(index*BLOCK_SIZE);
	pJPEGEncoder->jpeg_stream.pos = 0;

	jpeg_block_size = encode(pJPEGEncoder);

	// block alignment to 32 bits
	return (jpeg_block_size+3)&0xFFFFFFFC;
}

int EncodeUnit::enc_raw(int index)
{
	BYTE  *y,*u,*v;

    int src = index*2+m_pScreen->buff_idx[index];
	y  = m_pScreen->pframebuffer+(src*BLOCK_SIZE);
	u  = y + COLOR_SIZE;
	v  = u + COLOR_SIZE;

	BYTE* dest = m_pScreen->pencode_buffer+(index*BLOCK_SIZE);

	for(int i=0 ;i<COLOR_SIZE/4; i++) {
		dest[0] = y[0]; dest[1] = u[0]; dest[ 2] = v[0]; dest[ 3] = y[1];
		dest[4] = u[1]; dest[5] = v[1]; dest[ 6] = y[2]; dest[ 7] = u[2];
		dest[8] = v[2]; dest[9] = y[3]; dest[10] = u[3]; dest[11] = v[3];
		dest += 12; y += 4; u += 4; v += 4;
	}
    return BLOCK_SIZE;
};

/////// Thread related methods ///////

void EncodeUnit::ThreadStart()
{
	DBGTRACE(DBG_ALL,"EncodeUnit::ThreadStart()\n");
	while(!done) {
		WaitEvent(m_start);
		if( done ) break;
		EncodeBlocks();
		SetEvent(m_ready);
	}

	SetEvent(m_done);
}

void EncodeUnit::Trigger()
{
	SetEvent(m_start);
}

void EncodeUnit::WaitComplete()
{
	WaitEvent(m_ready);
}

void EncodeUnit::ThreadEnd()
{
	done = true;
	SetEvent(m_start);
}
