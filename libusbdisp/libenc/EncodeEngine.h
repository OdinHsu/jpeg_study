
#ifndef ENCODE_ENGINE_H
#define ENCODE_ENGINE_H

#include "ThreadBase.h"
#include "event.h"
#include "ClientUSB.h"
#include "Encoder.h"
#include <sys/time.h>

#define EVENT_ENCODE 0x00000001
#define EVENT_REMOVE 0x00000002
#define EVENT_ALL (EVENT_ENCODE | EVENT_REMOVE)

#define PROFILE 0

#define Enter_Critical_Section(mutex) pthread_mutex_lock(&mutex)
#define Leave_Critical_Section(mutex) pthread_mutex_unlock(&mutex)

typedef struct {
	unsigned char	*bmp_buffer;
	unsigned char	*pBitstream;
	unsigned int	width;
	unsigned int	height;
	unsigned int    pitch;
	unsigned int    h_blocks;
	unsigned int    v_blocks;
	unsigned int	szframebuffer;
	unsigned int	filelen;
	bool		    resolution_chahged;
	bool			first;

#if PROFILE
	unsigned int    StartEncodeTime;
	unsigned int    AnalysisTime;
	unsigned int    CompressTime;
	unsigned int    TransmitTime;
#endif
} task_t;

extern event_t sync_event;

#define NUM_THREADS 3

#define PIPE_ANALYSIS 2
#define PIPE_COMPRESS 1
#define PIPE_TRANSMIT 0

class EncodeEngine : public ThreadBase
{
public:
	EncodeEngine();
	virtual ~EncodeEngine();

private:
	void SendEvent( unsigned flag );

private:
	pthread_mutex_t m_critical_section;
	event_t 	  m_engine;

	event_t		  m_ready[NUM_THREADS-1];
	event_t		  m_start[NUM_THREADS-1];

	bool		  m_done;

	BlockEncoder  *m_encoder;

	task_t  *m_pNewTask;
	task_t  *m_AnalysisTask;
	task_t  *m_CompressTask;
	task_t  *m_TransmitTask;

	pthread_t m_CompressThread;
	pthread_t m_TransmitThread;

	event_t m_CompressEnd;
	event_t m_TransmitEnd;

protected:
	ClientUSB  	  *m_device;

private:
	unsigned int	m_width;
	unsigned int	m_height;
	unsigned int	m_pitch;

	bool			m_first;

public:
	virtual void Run();
	bool DeviceReset();

	bool SetResolution(int width,int height);
	bool Write(unsigned char* buffer,int len);

	void StartEncode(int width,int height,int pitch,unsigned char *framebuffer);

	void Analysis();
	void Compress();
	void Transmit();

};


#endif
