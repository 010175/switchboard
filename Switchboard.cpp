#include "Switchboard.h"


#define CFReleaseIfNotNULL(cf) if( cf ) CFRelease(cf);
#define CURL_UPDATE_INTERVAL 1
#define _SENDER_PORT 4444
#define _RECEIVER_PORT 5555
#define LAUNCH_PROCESS_DELAY_BEFORE_YOU_CAN_DO_IT 5

#define PROCESS_LABEL "org.monolithe.process"

#define PROCESS_PLIST_FILE_NAME "switchboardProcess.plist"
#define FOLDER_WATCHER_PLIST_FILE_NAME "folderWatcher.plist"

#define MONOLITHE_WEB_CONSOLE_URL "http://localhost/~guillaume/monolithe_web_console/"
//#define MONOLITHE_WEB_CONSOLE_URL "http://www.010175.net/monolithe/"

#define LOCATION "monolithe"

#define kIPKeyCFStringRef CFSTR("IP")        // dictionary key
#define kTimeKeyCFStringRef CFSTR("Time")    // dictionary key

int activeProcessIndex = -1;
int selectedProcessIndex = -1;
double lastLaunchProcessTime = 0;

applicationLister myApplicationlister;

launchd_wrapper myLaunchd_wrapper;

OscReceiver	receiver;

ofxNetworkUtils networkUtils;

TiXmlDocument calendar_xml("calendar.xml");
TiXmlDocument uploads_xml("uploads.xml");

#pragma mark get systeme time 
double getSystemTime(){
	
	timeval Time = {0, 0};
	gettimeofday(&Time, NULL);
	
	return Time.tv_sec + Time.tv_usec / 1000000.0;	
	
}

#pragma mark curl Send Process List call back
size_t curlSendProcessListToWebConsoleCallBack( void *ptr, size_t size, size_t nmemb, void *stream){
	
	printf("%s\n",(char*) ptr);
	return nmemb;
}

#pragma mark curl get calendar call back
size_t curlGetCalendarCallBack( void *ptr, size_t size, size_t nmemb, void *stream){
	
	calendar_xml.Parse((const char*)ptr);
	
	if ( calendar_xml.Error() )
	{
		
		printf("xml error %s: %i\n", calendar_xml.Value(), calendar_xml.ErrorId() );
		return nmemb;
		
	}
	
	int count = 0;
	
	TiXmlNode* node = 0;
	TiXmlElement* element = 0;
	while ( (node = calendar_xml.FirstChildElement()->IterateChildren( node ) ) != 0 ) {
		
		element = node->ToElement();
		
		printf("%s %s %s\n", element->Attribute("timestamp"),element->Attribute("action"),element->Attribute("argument"));
		
		if (strcmp(element->Attribute("action"),"launch_now")==0){
			int desire_process_index = atoi(element->Attribute("argument"));
			
			printf("%s %i\n",element->Attribute("action"),atoi(element->Attribute("argument")));
			
			if ((desire_process_index>-1)&&(desire_process_index!=activeProcessIndex)){
				
				CFStringRef appPath = myApplicationlister.getApplicationPath(desire_process_index);
				doFadeOperation(FillScreen, 0.2f, true);
				
				if (appPath == NULL) // double check if app name is not null.
					appPath = CFSTR("Unnamed App");
				
				
				myLaunchd_wrapper.removeProcess(PROCESS_LABEL);	// load new process
				
				usleep(1000000); // give some break
				
				int pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appPath), kCFStringEncodingASCII);
				char buffer[pathLength+1]; // buffer[nameLength+1];
				assert(buffer != NULL);
				bool success = CFStringGetCString(appPath, buffer, PATH_MAX, kCFStringEncodingASCII);
				myLaunchd_wrapper.submitProcess(PROCESS_LABEL, buffer);
				
				activeProcessIndex = desire_process_index;
				sleep(2);
				doFadeOperation(CleanScreen, 0.2f, true);
				sendProcessListToWebConsole();
				
			}
		}
		++count;
	}
	
	printf("Clalendar updated %i events\n", count);
	calendar_xml.Clear();
	
	return nmemb;
}

#pragma mark web console commnication
void sendProcessListToWebConsole(){
	
	// log time info
	time_t rawtime;
	struct tm * timeinfo;
	
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	
	printf("%.2i:%.2i:%.2i Send Process List to Web Console\n", timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	
	// setup curl
	CURLcode ret;
	CURL *hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, &curlSendProcessListToWebConsoleCallBack);
	
	struct curl_httppost *post=NULL;
	struct curl_httppost *last=NULL;
	
	CFStringEncoding encodingMethod;
	encodingMethod = CFStringGetSystemEncoding();
	
	CFStringRef processNameCFString;
	
	// build curl application list form
	for (int i= 0 ; i < myApplicationlister.getApplicationCount(); i++){
		
		processNameCFString = myApplicationlister.getApplicationName(i);
		char *bytes;
		CFIndex length = CFStringGetLength(processNameCFString);
		bytes = (char *)malloc( length + 1 );
		bytes = (char*)CFStringGetCStringPtr(processNameCFString, encodingMethod);
		
		curl_formadd(&post, &last,
					 CURLFORM_COPYNAME, "process_list[]",
					 CURLFORM_COPYCONTENTS, bytes, CURLFORM_END);
	}
	
	
	char idch[4];
	sprintf(idch, "%i",activeProcessIndex);
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_active_index",
				 CURLFORM_COPYCONTENTS, idch, CURLFORM_END);
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "location",
				 CURLFORM_COPYCONTENTS, LOCATION, CURLFORM_END);
	
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_list_submit",
				 CURLFORM_COPYCONTENTS, "submit", CURLFORM_END);
	
	
	/* Set the form info */
	curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post);
	curl_easy_setopt(hnd, CURLOPT_URL, MONOLITHE_WEB_CONSOLE_URL);
	
	ret = curl_easy_perform(hnd);
	curl_easy_cleanup(hnd);
	
}


/* Fetch Calendar Info from Web Console */
void getCalendarFromWebConsole(){
	// setup curl
	CURLcode ret;
	CURL *hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, &curlGetCalendarCallBack);
	
	struct curl_httppost *post=NULL;
	struct curl_httppost *last=NULL;
	
	
	CFStringEncoding encodingMethod;
	encodingMethod = CFStringGetSystemEncoding();
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "calendar_get_events",
				 CURLFORM_COPYCONTENTS, "1", CURLFORM_END);
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "location",
				 CURLFORM_COPYCONTENTS, LOCATION, CURLFORM_END);
	
	
	/* Set the form info */
	curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post);
	curl_easy_setopt(hnd, CURLOPT_URL, MONOLITHE_WEB_CONSOLE_URL);
	
	ret = curl_easy_perform(hnd);
	curl_easy_cleanup(hnd);
}

#pragma mark process waiting osc messages
void oscMessagesTimerFunction(CFRunLoopTimerRef timer, void *info)
{
	//printf("osc timer function");
	while( receiver.hasWaitingMessages() )
	{
		
		time_t rawtime;
		struct tm * timeinfo;
		
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		
		OscMessage rm;
		receiver.getNextMessage( &rm );
		
		printf("%.2i:%.2i:%.2i @%s : %s\n", timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,rm.getRemoteIp().c_str(), rm.getAddress().c_str());
		
		//CFStringRef remoteIP = CFStringCreateWithCString(kCFAllocatorDefault,rm.getRemoteIp().c_str(),kCFStringEncodingASCII);
		//CFNumberRef timecf = CFNumberCreate(kCFAllocatorDefault, kCFNumberNSIntegerType, &rawtime);
		//CFDictionaryAddValue(clientListDictionnary, remoteIP, timecf);
		
		//CFShow(timecf);
		//CFShow(remoteIP);
		
		//CFShow(clientListDictionnary);
		
		// ping
		if ( rm.getAddress() == "/monolithe/ping" )
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);// reset sender remote ip
			
			OscMessage sm;
			
			sm.setAddress("/monolithe/pong"); // pong back to remote
			sm.addStringArg(networkUtils.getInterfaceAddress(0));
			
			try{
				sender.sendMessage(sm);
			}
			catch(std::exception const& e)
			{
				std::cout << "Exception: " << e.what() << "\n";
			}
			break;
		}
		
		
		// launch
		if ( rm.getAddress() == "/monolithe/launchprocess" )
		{
			
			
			double now = getSystemTime();
			if (now<lastLaunchProcessTime+LAUNCH_PROCESS_DELAY_BEFORE_YOU_CAN_DO_IT){
				printf("to soon, now is %f, last is %f\r", now, lastLaunchProcessTime);
				break;
			}
			lastLaunchProcessTime = now;
			
			int appIndex = rm.getArgAsInt32( 0 );
			
			if (appIndex == activeProcessIndex) break; // don't relaunch current app
			
			//CFStringRef tmpPathCFString = myApplicationlister.getApplicationPath(appIndex);
			
			doFadeOperation(FillScreen, 0.2f, true); // fade in
			
			//remove current process
			myLaunchd_wrapper.removeProcess(PROCESS_LABEL);
			
			CFStringRef appPath = myApplicationlister.getApplicationPath(appIndex);
			
			if (appPath == NULL) // double check if app name is not null.
				appPath = CFSTR("Unnamed App");
			
			CFIndex pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appPath), kCFStringEncodingASCII);
			char buffer[pathLength+1]; //  PATH_MAX  ou buffer[nameLength+1];
			assert(buffer != NULL);
			Boolean success = CFStringGetCString(appPath, buffer, PATH_MAX, kCFStringEncodingASCII);
			
			myLaunchd_wrapper.submitProcess(PROCESS_LABEL, buffer);
			activeProcessIndex = appIndex;
			
			usleep(1000000); // give some break
			
			doFadeOperation(CleanScreen, 2.0f, true); // fade out
			
			// set process to front
			int pid = myLaunchd_wrapper.getPIDForProcessLabel(PROCESS_LABEL);
			printf("pid is %i\r", pid);
			
			ProcessSerialNumber psn;
			GetProcessForPID (
							  (long) pid,
							  &psn
							  );
			
			OSErr er = SetFrontProcess(&psn);
			printf("error ? %i\r", er);
			
			
			sendProcessListToWebConsoleThread.start();
			
			break;
			
		}
		
		// send app list
		if ( rm.getAddress() == "/monolithe/getprocesslist" )	
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);
			
			OscMessage sm;
			
			sm.setAddress("/monolithe/setprocesslist");
			myApplicationlister.updateApplicationsList();
			
			for (int i = 0; i<myApplicationlister.getApplicationCount();i++){
				
				// CFStringRef to CString complicated mess
				CFIndex          nameLength;
				Boolean          success;
				
				CFStringRef appName = myApplicationlister.getApplicationName(i);
				
				if (appName == NULL) // double check if app name is not null.
					appName = CFSTR("Unnamed App");
				
				nameLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appName), kCFStringEncodingASCII);
				
				char             buffer[FILENAME_MAX]; // buffer[nameLength+1];
				assert(buffer != NULL);
				
				success = CFStringGetCString(appName, buffer, FILENAME_MAX, kCFStringEncodingASCII);
				
				if (success){
					sm.addStringArg(buffer);
					
				} else {
					printf("error while getting application name\r");
				}					
			}
			
			try{
				sender.sendMessage(sm);
			}
			catch(std::exception const& e)
			{
				std::cout << "Exception: " << e.what() << "\n";
			}
			
			
			break;
		}
		
		// send current process index
		if ( rm.getAddress() == "/monolithe/getprocessindex" ) 
		{
			
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);// reset sender remote ip
			
			OscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	
			
			try{
				sender.sendMessage(sm);
			}
			catch(std::exception const& e)
			{
				std::cout << "Exception: " << e.what() << "\n";
			}
			
			break;
			
		}
		
		// stop current process
		if ( rm.getAddress() == "/monolithe/stopprocess" ) 
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT); // reset sender remote ip
			
			myLaunchd_wrapper.removeProcess(PROCESS_LABEL); // unload current process
			
			activeProcessIndex = -1;
			
			sendProcessListToWebConsoleThread.start();
			
			break;
			
		}
		
		// 
		if ( rm.getAddress() == "/monolithe/newapplication" ) 
		{
			break;
			
			sendProcessListToWebConsoleThread.start();
			
		}
	}
	
}

void webConsoleTimerFunction(CFRunLoopTimerRef timer, void *info)
{
	printf("web console timer function");
	sendProcessListToWebConsoleThread.start();
	
	
}

#pragma mark main
int main (int argc, const char * argv[]) {
	printf("%s", argv[0]);
	
	// init array
	clientListDictionnary =CFDictionaryCreateMutable (kCFAllocatorDefault,
													  0,
													  NULL,
													  NULL
													  );
	
	// setup runLoops
	CFRunLoopRef		mRunLoopRef;
	mRunLoopRef = CFRunLoopGetCurrent();
	
	CFRunLoopTimerRef oscTimer =		CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 1, 1, 0, 0, oscMessagesTimerFunction, NULL); // 1 sec
	//CFRunLoopTimerRef webConsoleTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 2, 2, 0, 0, webConsoleTimerFunction, NULL); // 2 sec
	
	CFRunLoopAddTimer(mRunLoopRef, oscTimer, kCFRunLoopDefaultMode);
	//CFRunLoopAddTimer(mRunLoopRef, webConsoleTimer, kCFRunLoopDefaultMode);
	
	networkUtils.init();
	receiver.setup( _RECEIVER_PORT );
	
	// unload running switchboard launchd process
	myLaunchd_wrapper.removeProcess(PROCESS_LABEL);
	
	// start the loop
	CFRunLoopRun();
	
	sendProcessListToWebConsoleThread.stop();
	printf("END\n");
	return 0;
}


