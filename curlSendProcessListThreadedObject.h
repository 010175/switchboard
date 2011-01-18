#ifndef _THREADED_OBJECT
#define _THREADED_OBJECT

class curlSendProcessListThreadedObject : public thread_runner{
	
public:
	
	
	//--------------------------
	curlSendProcessListThreadedObject(){

	}
	
	void start(){
		startThread(true, false);   // blocking, verbose
	}
	
	void stop(){
		stopThread();
	}
	
	//--------------------------
	void threadedFunction(){
		
		//if(!isThreadRunning()){
			if( lock() ){
				sendProcessListToWebConsole();
				unlock();
				stop();
			}		
		
		//}
		
	}
	
	//--------------------------
	void step(){
		
		
		/*
		 if( lock() ){
		 if (lastCount!=count){
		 printf("I am a slowly increasing thread. \nmy current count is: %i\n", count);
		 lastCount = count;
		 }
		 unlock();
		 }else{
		 printf("can't lock!\neither an error\nor the thread has stopped\n");
		 }
		 */
		
	}
	
};



#endif
