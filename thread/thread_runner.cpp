#include "thread_runner.h" 

//------------------------------------------------- 
thread_runner::thread_runner(){ 
	threadRunning = false; 
#ifdef TARGET_WIN32 
	InitializeCriticalSection(&critSec); 
#else 
	pthread_mutex_init(&myMutex, NULL); 
#endif 
} 

//------------------------------------------------- 
thread_runner::~thread_runner(){ 
#ifndef TARGET_WIN32 
	pthread_mutex_destroy(&myMutex); 
#endif 
	stopThread(); 
} 

//------------------------------------------------- 
bool thread_runner::isThreadRunning(){ 
	//should be thread safe - no writing of vars here 
	return threadRunning; 
} 

//------------------------------------------------- 
void thread_runner::startThread(bool _blocking, bool _verbose){ 
	if( threadRunning ){ 
		if(verbose)printf("thread_runner: thread already running\n"); 
		return; 
	} 
	
	//have to put this here because the thread can be running 
	//before the call to create it returns 
	threadRunning   = true; 
	
#ifdef TARGET_WIN32 
	//InitializeCriticalSection(&critSec); 
	myThread = (HANDLE)_beginthreadex(NULL, 0, this->thread,  (void *)this, 0, NULL); 
#else 
	//pthread_mutex_init(&myMutex, NULL); 
	pthread_create(&myThread, NULL, thread, (void *)this); 
#endif 
	
	blocking      =   _blocking; 
	verbose         = _verbose; 
} 

//------------------------------------------------- 
//returns false if it can't lock 
bool thread_runner::lock(){ 
	
#ifdef TARGET_WIN32 
	if(blocking)EnterCriticalSection(&critSec); 
	else { 
		if(!TryEnterCriticalSection(&critSec)){ 
            if(verbose)printf("thread_runner: mutext is busy \n"); 
            return false; 
		} 
	} 
	if(verbose)printf("thread_runner: we are in -- mutext is now locked \n"); 
#else 
	
	if(blocking){ 
		if(verbose)printf("thread_runner: waiting till mutext is unlocked\n"); 
		pthread_mutex_lock(&myMutex); 
		if(verbose)printf("thread_runner: we are in -- mutext is now locked \n"); 
	}else{ 
		int value = pthread_mutex_trylock(&myMutex); 
		if(value == 0){ 
            if(verbose)printf("thread_runner: we are in -- mutext is now locked \n"); 
		} 
		else{ 
            if(verbose)printf("thread_runner: mutext is busy - already locked\n"); 
            return false; 
		} 
	} 
#endif 
	
	return true; 
} 

//------------------------------------------------- 
bool thread_runner::unlock(){ 
	
#ifdef TARGET_WIN32 
	LeaveCriticalSection(&critSec); 
#else 
	pthread_mutex_unlock(&myMutex); 
#endif 
	
	if(verbose)printf("thread_runner: we are out -- mutext is now unlocked \n"); 
	
	return true; 
} 

//------------------------------------------------- 
void thread_runner::stopThread(){ 
	if(threadRunning){ 
#ifdef TARGET_WIN32 
		CloseHandle(myThread); 
#else 
		//pthread_mutex_destroy(&myMutex); 
		pthread_detach(myThread); 
#endif 
		if(verbose)printf("thread_runner: thread stopped\n"); 
		threadRunning = false; 
	}else{ 
		if(verbose)printf("thread_runner: thread already stopped\n"); 
	} 
}
