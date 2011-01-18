/***********************************************************************
 ** __MyCompanyName__
 ** guillaume
 ** Copyright (c) 2010. All rights reserved.
 **********************************************************************/
#ifndef THREAD_RUNNER_H_
#define THREAD_RUNNER_H_



#ifdef TARGET_WIN32
#include <process.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif

#include <CoreFoundation/CoreFoundation.h>


using namespace std;

class thread_runner{
	
public:
	thread_runner();
	virtual ~thread_runner();
	bool isThreadRunning();
	void startThread(bool _blocking = true, bool _verbose = true);
	bool lock();
	bool unlock();
	void stopThread();
	
protected:
	
	//-------------------------------------------------
	//you need to overide this with the function you want to thread
	virtual void threadedFunction(){
		if(verbose)printf("thread_runner: overide threadedFunction with your own\n");
	}
	
	//-------------------------------------------------
	

#ifdef TARGET_WIN32
	static unsigned int __stdcall thread(void * objPtr){
		ofxThread* me	= (ofxThread*)objPtr;
		me->threadedFunction();
		return 0;
	}
	
#else
	static void * thread(void * objPtr){
		thread_runner* me	= (thread_runner*)objPtr;
		me->threadedFunction();
		return 0;
	}
#endif
	
#ifdef TARGET_WIN32
	HANDLE            myThread;
	CRITICAL_SECTION  critSec;  	//same as a mutex
#else
	pthread_t        myThread;
	pthread_mutex_t  myMutex;
#endif
	
	bool threadRunning;
	bool blocking;
	bool verbose;
};

#endif
