
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "EncodeEngine.h"
#include "debug_trace.h"

event_t sync_event;

static void* ThreadCompressFunction(void* lpParam);
static void* ThreadTransmitFunction(void* lpParam);

EncodeEngine::EncodeEngine()
{
	DBGTRACE(DBG_ENGINE,"EncodeEngine::EncodeEngine()\n");

	init_event(&m_engine);
	pthread_mutex_init(&m_critical_section,NULL);

	for(int i=0;i<NUM_THREADS;i++)
	{
		init_event(&m_ready[i]);
		init_event(&m_start[i]);
	}

	m_device = NULL;

	m_width  = 0;
	m_height = 0;
	m_pitch  = 0;

	m_first = true;

	m_done = false;

	m_pNewTask = NULL;

	m_encoder = new BlockEncoder();

	init_event(&m_CompressEnd);
	init_event(&m_TransmitEnd);

	pthread_create(&m_CompressThread,NULL,ThreadCompressFunction,this);
	pthread_create(&m_TransmitThread,NULL,ThreadTransmitFunction,this);

};

EncodeEngine::~EncodeEngine()
{
	// Wait
	SendEvent(EVENT_REMOVE);
	DBGTRACE(DBG_ENGINE,"EncodeEngine::Wait thread terminate\n");

    WaitThreadExit();

    DBGTRACE(DBG_ENGINE,"EncodeEngine::thread terminate\n");
	// Close threads
	m_done = true;

	for(int i=0;i<NUM_THREADS-1;i++) {
		SetEvent(m_start[i]);
	};

    DBGTRACE(DBG_ENGINE,"EncodeEngine::wait compress and transmit terminate\n");

	WaitEvent(m_CompressEnd);
	WaitEvent(m_TransmitEnd);

	DBGTRACE(DBG_ENGINE,"EncodeEngine::compress and transmit terminate\n");

	if( m_device )
	{
		delete m_device;
		m_device = NULL;
	};

	delete m_encoder;

	uninit_event(&m_engine);
	pthread_mutex_destroy(&m_critical_section);

	for(int i=0;i<NUM_THREADS;i++)
	{
		uninit_event(&m_ready[i]);
		uninit_event(&m_start[i]);
	}

	uninit_event(&m_CompressEnd);
	uninit_event(&m_TransmitEnd);

	DBGTRACE(DBG_ENGINE,"EncodeEngine::~EncodeEngine()\n");
};

void EncodeEngine::Run()
{
	unsigned int flags=0;

	m_first = true;

	DBGTRACE(DBG_THREAD,"EncodeEngine: Thread start.\n");
	SetEvent(sync_event);

	while( 1 ) {
		// Wait for events
		pthread_mutex_lock(&m_engine.mutex);
		while(!m_engine.flag) pthread_cond_wait(&m_engine.cond,&m_engine.mutex);
		// keep flags
		flags = m_engine.flag & EVENT_ALL;
		m_engine.flag = 0;
		pthread_mutex_unlock(&m_engine.mutex);

		// Process events
		while( flags ) {
			if(flags & EVENT_REMOVE) {
				DBGTRACE(DBG_ENGINE,"EncodeEngine: receive EVENT_REMOVE\n");
				flags ^= EVENT_REMOVE;

				DBGTRACE(DBG_THREAD,"EncodeEngine: Thread terminate.\n");
				return;
			};
			if(flags & EVENT_ENCODE) {
				DBGTRACE(DBG_ENGINE,"EncodeEngine: receive EVENT_ENCODE\n");
				flags ^= EVENT_ENCODE;

				Analysis();
				SetEvent(sync_event);
			};
		};
	};
};


bool EncodeEngine::DeviceReset()
{
	if(m_device) {
		m_device->ResetDevice();
	}
	return true;
}

void EncodeEngine::SendEvent( unsigned flag )
{
    pthread_mutex_lock(&m_engine.mutex);
    m_engine.flag |= flag;
    pthread_cond_signal(&m_engine.cond);
    pthread_mutex_unlock(&m_engine.mutex);
};

bool EncodeEngine::SetResolution(int width,int height)
{

	m_encoder->SetInputImageGeometry(width,height,width);

	return true;
};

bool EncodeEngine::Write(unsigned char* buffer,int len)
{
	unsigned long ActLength=0;

	if(m_device->IsConnected())
	{
		return m_device->Write(buffer,len,&ActLength);
	} else {
		return false;
	}
};

void EncodeEngine::StartEncode(int width,int height,int pitch,unsigned char *framebuffer)
{
    int height_4 = ((height+3)/4)*4;

	Enter_Critical_Section(m_critical_section);
	if( m_pNewTask == NULL) {
		// New task

		m_pNewTask = new task_t;
		m_pNewTask->width     = width;
		m_pNewTask->height    = height;
		m_pNewTask->h_blocks  = 0;
		m_pNewTask->v_blocks  = 0;

		m_pNewTask->pitch         = ((width+3)/4)*4; // align to 4x4 block
		m_pNewTask->szframebuffer = m_pNewTask->pitch*height_4*4;

		m_pNewTask->bmp_buffer = NULL;
		m_pNewTask->pBitstream = NULL;
		m_pNewTask->filelen    =  0;
		m_pNewTask->first      = m_first;

		m_pNewTask->resolution_chahged = false;  // initialize value
	}
	else if( ((unsigned int)width != m_pNewTask->width) || ((unsigned int)height != m_pNewTask->height))
	{ // resolution changed

		m_pNewTask->width     = width;
		m_pNewTask->height    = height;
		m_pNewTask->h_blocks  = 0;
		m_pNewTask->v_blocks  = 0;

		m_pNewTask->pitch         = ((width+3)/4)*4; // align to 4x4 block
		m_pNewTask->szframebuffer = m_pNewTask->pitch*height_4*4;

		if(m_pNewTask->bmp_buffer!= NULL) free( m_pNewTask->bmp_buffer );
		m_pNewTask->bmp_buffer =  NULL;
		m_pNewTask->first      = m_first;
	};

	if(m_pNewTask->bmp_buffer == NULL) {
		m_pNewTask->bmp_buffer = (BYTE*)memalign(16,m_pNewTask->szframebuffer+BLOCK_WIDTH*4);
	}

	DBGTRACE(DBG_ENGINE,"StartEncode: w = %d,h = %d, pitch = %d, size = %d\n",width,height,pitch,m_pNewTask->szframebuffer);
#if PROFILE
		struct timeval start_time,end_time;
		gettimeofday(&start_time,NULL);
#endif

	// copy framebuffer to m_pNewTask->bmp_buffer ( remove framebuffer pitch )
	unsigned int* dest = (unsigned int*) m_pNewTask->bmp_buffer;
	unsigned int* src  = (unsigned int*) framebuffer;

	for(int i=0;i < m_pNewTask->height;i++) {
		memcpy(dest,src,m_pNewTask->width*sizeof(int));
		src  += pitch;
		dest += m_pNewTask->pitch;
	}

#if PROFILE
		gettimeofday(&end_time,NULL);

		int total_time = end_time.tv_usec - start_time.tv_usec;
		if(total_time < 0) total_time += 1000000;

		m_pNewTask->StartEncodeTime = total_time;
#endif

	Leave_Critical_Section(m_critical_section);

    DBGTRACE(DBG_ENGINE,"StartEncode: finish\n");
	SendEvent(EVENT_ENCODE);

}

void EncodeEngine::Analysis()
{

	DBGTRACE(DBG_ENGINE,"EncodeEngine: Analysis()\n");

	// Get data from pNewTask;
	Enter_Critical_Section(m_critical_section);
	m_AnalysisTask = m_pNewTask;
	m_pNewTask = NULL;
	Leave_Critical_Section(m_critical_section);

	if(m_AnalysisTask != NULL) {
		///////////   Analysis   /////////////

		m_AnalysisTask->pitch   = ((m_AnalysisTask->width+3)/4)*4;

		m_AnalysisTask->h_blocks  = (m_AnalysisTask->width + BLOCK_WIDTH-1)/BLOCK_WIDTH;
		m_AnalysisTask->v_blocks  = (m_AnalysisTask->height+BLOCK_HEIGHT-1)/BLOCK_HEIGHT;

		// Check resolution change
		if((m_width != m_AnalysisTask->width) || (m_height != m_AnalysisTask->height)) {
			m_encoder->SetInputImageGeometry(m_AnalysisTask->width,m_AnalysisTask->height,m_AnalysisTask->pitch);

			m_width  = m_AnalysisTask->width;
			m_height = m_AnalysisTask->height;
			m_pitch  = m_AnalysisTask->pitch;
			m_AnalysisTask->resolution_chahged = true;
			m_AnalysisTask->first = true;
			m_first  = true;
		}

		DBGTRACE(DBG_ENGINE,"EncodeEngine: m_encoder->Analysis()\n");

#if PROFILE
		struct timeval start_time,end_time;
		gettimeofday(&start_time,NULL);
#endif

		DBGTRACE(DBG_ENGINE,"EncodeEngine: m_pitch = %d\n",m_pitch);
		int blocks = 0;

        blocks = m_encoder->Analysis(m_AnalysisTask->bmp_buffer,m_AnalysisTask->first);

#if PROFILE
		gettimeofday(&end_time,NULL);

		int total_time = end_time.tv_usec - start_time.tv_usec;
		if(total_time < 0) total_time += 1000000;

		m_AnalysisTask->AnalysisTime = total_time;
#endif

		if(blocks == 0) {
			if(m_AnalysisTask->bmp_buffer) free( m_AnalysisTask->bmp_buffer );
			delete m_AnalysisTask;
			return;
		}
		// prepare memory for bitsteam
		m_AnalysisTask->pBitstream = new BYTE[(blocks+1)*(BLOCK_SIZE+4)];

		//////// Wait Compress ready /////////
		WaitEvent(m_ready[PIPE_COMPRESS]);

		// Set data for Compress
		m_CompressTask = m_AnalysisTask;

		m_encoder->ToggleScreen();

		if(m_CompressTask->bmp_buffer) {
			free( m_CompressTask->bmp_buffer );
			m_CompressTask->bmp_buffer = NULL;
		}

		// Trigger Compress thread
		SetEvent(m_start[PIPE_COMPRESS]);

	}
	DBGTRACE(DBG_ENGINE,"EncodeEngine: Analysis(), complete\n");
}

void EncodeEngine::Compress()
{

	SetEvent(m_ready[PIPE_COMPRESS]);

	while(!m_done)
	{
		WaitEvent(m_start[PIPE_COMPRESS]);
		if( m_done ) break;
		DBGTRACE(DBG_ENGINE,"EncodeEngine: Compress()\n");
		///////////   Compress   /////////////
#if PROFILE
		struct timeval start_time,end_time;
		gettimeofday(&start_time,NULL);
#endif

		m_CompressTask->filelen = m_encoder->Compress(m_CompressTask->pBitstream);

#if PROFILE
		gettimeofday(&end_time,NULL);

		int total_time = end_time.tv_usec - start_time.tv_usec;
		if(total_time < 0) total_time += 1000000;

		m_CompressTask->CompressTime = total_time;
#endif
		//////// Wait Transmit ready /////////
		WaitEvent(m_ready[PIPE_TRANSMIT]);

		// Set data for Transmit
		m_TransmitTask = m_CompressTask;

		// Compress thread is ready for next frame
		SetEvent(m_ready[PIPE_COMPRESS]);

		// Trigger Transmit thread
		SetEvent(m_start[PIPE_TRANSMIT]);

	}
	SetEvent(m_CompressEnd);
}

void EncodeEngine::Transmit()
{
//printf("Eddie EncodeEngine: SetQuantTable\n");
	unsigned char Qtable[140];
	int index;
	SetEvent(m_ready[PIPE_TRANSMIT]);

	while(!m_done)
	{//printf("Eddie3333333333333Table\n");
		WaitEvent(m_start[PIPE_TRANSMIT]);
		//if( m_done ) break;
		///////////   Transmit   /////////////
//printf("Eddie En111111111ble\n");
		if( m_TransmitTask->resolution_chahged ) {
			if(m_device->IsConnected())
			{
				for(index=1;index<4;index++) {
					m_encoder->GetQuantTable(Qtable,138,index);
					if(m_device->SetQuantTable(Qtable,138,index))
					{
						printf("Eddie EncodeEngine: SetQuantTable %d\n",index);
					} else {
						printf("Eddie EncodeEngine: Fails at SetQuantTable %d\n",index);
//						DeviceReset();
//						exit(1);
//                        return;
					}
				}

				m_device->SetResolution(m_TransmitTask->width,m_TransmitTask->height);
				DBGTRACE(DBG_PROFILE,"SetResolution : %d x %d\n",m_TransmitTask->width,m_TransmitTask->height);
			} else {
				MSG("EncodeEngine: Fails on SetResolution() device not connected !\n");
//				exit(1);
			}

			DBGTRACE(DBG_ENGINE,"EncodeEngine: SetResolution( %d , %d)\n",m_TransmitTask->width,m_TransmitTask->height);
		}
		DBGTRACE(DBG_ENGINE,"EncodeEngine: Transmit(), len = %d\n",m_TransmitTask->filelen);

#if PROFILE
		struct timeval start_time,end_time;
		gettimeofday(&start_time,NULL);
#endif

		if(m_TransmitTask->filelen) {
			if(Write(m_TransmitTask->pBitstream,m_TransmitTask->filelen)) {
				m_first = false;
			} else {
				m_first = true;
				m_width = 0;
				DeviceReset();          //Add 2015-3-2 : fixed crash after unplug
			}
		}

#if PROFILE
		gettimeofday(&end_time,NULL);

		int total_time = end_time.tv_usec - start_time.tv_usec;
		if(total_time < 0) total_time += 1000000;
		m_TransmitTask->TransmitTime = total_time;

		printf("StartEncode(%3d ms)/Analysis(%3d ms)/Compress(%3d ms)/Transmit(%3d ms)\n"
				,m_TransmitTask->StartEncodeTime/1000
				,m_TransmitTask->AnalysisTime/1000
				,m_TransmitTask->CompressTime/1000
				,m_TransmitTask->TransmitTime/1000);
#endif
		// remove processed data
		if(m_TransmitTask->pBitstream) {
			delete m_TransmitTask->pBitstream;
			m_TransmitTask->pBitstream = NULL;
		}

		delete m_TransmitTask; // all tasks are completed
		m_TransmitTask = NULL;

		// Transmit thread is ready for next frame
		SetEvent(m_ready[PIPE_TRANSMIT]);

	}
	SetEvent(m_TransmitEnd);
}

void* ThreadCompressFunction(void* lpParam)
{
	EncodeEngine *pAssociatedClass = (EncodeEngine*) lpParam;
	pAssociatedClass->Compress();
	return NULL;
}

void* ThreadTransmitFunction(void* lpParam)
{
	EncodeEngine *pAssociatedClass = (EncodeEngine*) lpParam;
	pAssociatedClass->Transmit();
	return NULL;
}
