#include "ThreadBase.h"
#include "debug_trace.h"

ThreadBase::ThreadBase()
{
	m_created = false;
	m_thread_ready = false;

	pthread_mutex_init(&m_critical_section,NULL);
	init_event(&m_wait_thread);
	init_event(&m_ThreadEnd);
};

ThreadBase::~ThreadBase()
{
	WaitThreadExit();

	pthread_mutex_destroy(&m_critical_section);

	uninit_event(&m_wait_thread);
	uninit_event(&m_ThreadEnd);
	m_thread_ready = false;

};

static void* threadfunction(void* lpParam) //__attribute__((cdecl))
{
	ThreadBase *pAssociatedClass = (ThreadBase*) lpParam;

	pAssociatedClass->PrivateStart();
	pAssociatedClass->Run();
	pAssociatedClass->PrivateClose();

	return NULL;
}

bool ThreadBase::Start()
{

	pthread_mutex_lock(&m_critical_section);

	if( m_created == false)
	{
		if(pthread_create(&m_thread,NULL,threadfunction,this) != 0) // create thread fail.
		{
			pthread_mutex_lock(&m_critical_section);
			printf("Create Thread fail.\n");
			return false;
		}
		m_created = true;
		printf("Create Thread OK.\n");
	}

	pthread_mutex_unlock(&m_critical_section);

	WaitThreadReady();
	return true;

};

void ThreadBase::WaitThreadExit()
{
	if(m_thread_ready) WaitEvent(m_ThreadEnd);
}

void ThreadBase::PrivateStart()
{
    pthread_mutex_lock(&m_wait_thread.mutex);
    m_wait_thread.flag = 1;
	m_thread_ready = true;

	pthread_cond_signal(&m_wait_thread.cond);
    pthread_mutex_unlock(&m_wait_thread.mutex);
}

void ThreadBase::PrivateClose()
{
	pthread_mutex_lock(&m_critical_section);
	m_created = false;
	m_thread_ready = false;
	SetEvent(m_ThreadEnd);
	pthread_mutex_unlock(&m_critical_section);
};
