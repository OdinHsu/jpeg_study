#ifndef THREAD_BASE_H
#define THREAD_BASE_H

#include <pthread.h>
#include "event.h"

class ThreadBase
{
public:
	ThreadBase();
	virtual ~ThreadBase();

	virtual bool Start();

	void WaitThreadExit();
private:
	pthread_mutex_t m_critical_section;

	pthread_t m_thread;
	bool      m_created;

	event_t m_wait_thread;
	event_t m_ThreadEnd;
	bool    m_thread_ready;

private:
	bool WaitThreadReady()
	{
		if( m_thread_ready ) {
			return true;
		} else {
			WaitEvent(m_wait_thread);
		}
		return true;
	};

public:
	void PrivateStart();
	void PrivateClose();
	virtual void Run() = 0;

};

#endif
