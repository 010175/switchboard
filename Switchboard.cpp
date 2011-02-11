#include "Switchboard.h"


#define CFReleaseIfNotNULL(cf) if( cf ) CFRelease(cf);
#define CURL_UPDATE_INTERVAL 1
#define _SENDER_PORT 4444
#define _RECEIVER_PORT 5555
#define DELAY_BEFORE_NEXT_PROCESS_LAUNCH 5

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

ofxOscReceiver	receiver;

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
		
		ofxOscMessage rm;
		receiver.getNextMessage( &rm );
		
		printf("%.2i:%.2i:%.2i @%s : %s\n", timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,rm.getRemoteIp().c_str(), rm.getAddress().c_str());
		
		//CFStringRef remoteIP = CFStringCreateWithCString(kCFAllocatorDefault,rm.getRemoteIp().c_str(),kCFStringEncodingASCII);
		//CFNumberRef timecf = CFNumberCreate(kCFAllocatorDefault, kCFNumberNSIntegerType, &rawtime);
		//CFDictionaryAddValue(clientListDictionnary, remoteIP, timecf);
		
		//CFShow(timecf);
		//CFShow(remoteIP);
		
		//CFShow(clientListDictionnary);
		
		// ping
#pragma mark osc ping
		if ( rm.getAddress() == "/monolithe/ping" )
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);// reset sender remote ip
			
			ofxOscMessage sm;
			
			sm.setAddress("/monolithe/pong"); // pong back to remote
			sm.addStringArg(networkUtils.getInterfaceAddress(0));
			
			try{
				sender.sendMessage(sm);
			}
			catch (std::runtime_error &e) {
				std::cout << "Caught a runtime_error exception: "
				<< e.what () << '\n';
			}
			catch (std::exception &e) {
				std::cout << "Caught an exception of an unexpected type: "
				<< e.what () << '\n';
			} 
			catch (...) {
				std::cout << "Caught an unknown exception\n";
			}
			
			break;
		}
		
		
		// launch
#pragma mark osc launch
		if ( rm.getAddress() == "/monolithe/launchprocess" )
		{
			double now = getSystemTime();
			if (now<lastLaunchProcessTime+DELAY_BEFORE_NEXT_PROCESS_LAUNCH){
				printf("to soon, now is %f, last is %f\r", now, lastLaunchProcessTime);
				break;
			}
			lastLaunchProcessTime = now;
			
			int appIndex = rm.getArgAsInt32( 0 );
			
			if (appIndex == activeProcessIndex) break; // don't relaunch current app
			
			// hide mouse cursor
			CGDisplayHideCursor (kCGDirectMainDisplay);
			CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, CGPointZero);
			doFadeOperation(FillScreen, 0.2f, true); // fade in
			
			//remove current process
			myLaunchd_wrapper.removeProcess(PROCESS_LABEL);
			
			CFStringRef appPath = myApplicationlister.getApplicationPath(appIndex);
			
			if (appPath == NULL) // double check if app name is not null.
				appPath = CFSTR("Unnamed App");
			
			CFIndex pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appPath), kCFStringEncodingASCII);
			char filePath[pathLength+1]; //  PATH_MAX  ou buffer[nameLength+1];
			assert(filePath != NULL);
			Boolean success = CFStringGetCString(appPath, filePath, PATH_MAX, kCFStringEncodingASCII);
			
			myLaunchd_wrapper.submitProcess(PROCESS_LABEL, filePath);
			activeProcessIndex = appIndex;
			
			int i=5;
			do {
				usleep(2000000); // give some break
			} while ((--i)&&(!myLaunchd_wrapper.isProcessRunning(PROCESS_LABEL)));
			
			
			doFadeOperation(CleanScreen, 2.0f, true); // fade out
			
			CGDisplayHideCursor (kCGDirectMainDisplay);
			
			
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
		
		// send process list
#pragma mark osc process list
		if ( rm.getAddress() == "/monolithe/getprocesslist" )	
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);
			
			ofxOscMessage sm;
			
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
				
				char             filePath[FILENAME_MAX]; // buffer[nameLength+1];
				assert(filePath != NULL);
				
				success = CFStringGetCString(appName, filePath, FILENAME_MAX, kCFStringEncodingASCII);
				
				if (success){
					sm.addStringArg(filePath);
					
				} else {
					printf("error while getting application name\r");
				}					
			}
			
			try{
				sender.sendMessage(sm);
			}
			catch (std::runtime_error &e) {
				std::cout << "Caught a runtime_error exception: "
				<< e.what () << '\n';
			}
			catch (std::exception &e) {
				std::cout << "Caught an exception of an unexpected type: "
				<< e.what () << '\n';
			} 
			catch (...) {
				std::cout << "Caught an unknown exception\n";
			}
			
			
			break;
		}
		
		// send current process index
#pragma mark osc process index
		if ( rm.getAddress() == "/monolithe/getprocessindex" ) 
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);// reset sender remote ip
			
			ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	
			
			try{
				sender.sendMessage(sm);
			}
			catch (std::runtime_error &e) {
				std::cout << "Caught a runtime_error exception: "
				<< e.what () << '\n';
			}
			catch (std::exception &e) {
				std::cout << "Caught an exception of an unexpected type: "
				<< e.what () << '\n';
			} 
			catch (...) {
				std::cout << "Caught an unknown exception\n";
			}
			
			break;
			
		}
		
#pragma mark osc process description
		if ( rm.getAddress() == "/monolithe/getprocessdescription" )	
		{
			// get osc argument
			int appIndex = rm.getArgAsInt32( 0 );
			sender.setup(rm.getRemoteIp(), _SENDER_PORT);// reset sender remote ip
			
			ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessdescription");
			
			
			CFIndex          pathLength;
			Boolean          success;
			
			CFStringRef appPath = myApplicationlister.getApplicationEnclosingDirectoryPath(appIndex);
			CFMutableStringRef appXmlDescriptionFilePath= CFStringCreateMutable(NULL, 0);
			
			if (appPath == NULL) // double check if app name is not null.
				appPath = CFSTR("Path error");
			
			// build path to xml description file (path/application_name.xml)
			CFStringAppend(appXmlDescriptionFilePath, appPath);
			CFStringAppend(appXmlDescriptionFilePath,CFSTR("/"));
			CFStringAppend(appXmlDescriptionFilePath, myApplicationlister.getApplicationName(appIndex));
			CFStringAppend(appXmlDescriptionFilePath, CFSTR(".xml"));
			
			// lenght of path
			pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appXmlDescriptionFilePath), kCFStringEncodingASCII);
		
			// open xml file and send it
			char             filePath[FILENAME_MAX]; // buffer[pathLength+1];
			assert(filePath != NULL);
			
			// CFString to c_string
			success = CFStringGetCString(appXmlDescriptionFilePath, filePath, FILENAME_MAX, kCFStringEncodingASCII);
			
			CFRelease(appXmlDescriptionFilePath);
			
			if (success){ 
				
				//make sure the file exists on the disk
				printf("loading %s...\n",filePath);
				ifstream existanceChecker(filePath);
				
				if(!existanceChecker.is_open()){
					printf("file %s not found\n",filePath);
					return;
				} else {
					printf("data file %s ok\n",filePath);
				}
				
				existanceChecker.close();
				
				ifstream::pos_type size;
				char * memblock;
				
				std::ifstream inFile(filePath,ios::in|ios::binary|ios::ate);
				if (!inFile.is_open())
					return;
				
				size = inFile.tellg();
				memblock = new char [size];
				inFile.seekg (0, ios::beg);
				inFile.read (memblock, size);
				inFile.close();
				
				cout << "the complete file content is in memory : " << size << "bytes\n";
				
				sm.addStringArg(memblock);
				
				delete[] memblock;
				
				try{
					sender.sendMessage(sm);
				}
				catch (std::runtime_error &e) {
					std::cout << "Caught a runtime_error exception: "
					<< e.what () << '\n';
				}
				catch (std::exception &e) {
					std::cout << "Caught an exception of an unexpected type: "
					<< e.what () << '\n';
				} 
				catch (...) {
					std::cout << "Caught an unknown exception\n";
				}
				break;
				
			}else printf("error getting application description\r");
				
		}					
		
	
		// stop current process
#pragma mark osc stop process
		if ( rm.getAddress() == "/monolithe/stopprocess" ) 
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT); // reset sender remote ip
			
			myLaunchd_wrapper.removeProcess(PROCESS_LABEL); // unload current process
			
			activeProcessIndex = -1;
			
			sendProcessListToWebConsoleThread.start();
			
			break;
			
		}
		
		// new application aviable
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


void SignalHandler(int signal)
{
	std::cout << "Caught a signal : "
	<< signal << '\n';
	
	if ((signal==SIGINT)||(signal==SIGQUIT)||(signal==SIGSTOP))
	{
		// stop the RunLoop nicely
		CFRunLoopRef		mRunLoopRef;
		mRunLoopRef = CFRunLoopGetCurrent();
		CFRunLoopStop(mRunLoopRef);
		
	} else if (signal==SIGABRT) {
		
		std::cout << "SIGABRT"<< '\n';
	} else {
		
		exit(1);
	}
	
}


void InstallExceptionHandler()
{	
	
	/*
	 
	 {
	 Error # - Code - Signal	 Definition	-	Possible Causes
	 1. SIGHUP	-	terminal line hangup	-	this gets sent when your shell's terminal goes away (like the telephone line hung up, which is where HUP comes from). This signal is also used by convention to tell daemons to reload their configuration files. This works because daemons don't have controlling terminals, so a HUP signal won't get sent to them under normal circumstances
	 2. SIGINT	-	interrupt program	-	Control-C was pressed
	 3. SIGQUIT	-	quit program	-	Not technically a crash (can be sent by typing control-\)
	 4. SIGILL	-	illegal instruction	-	Compiler error, incorrect use of assembly code, or trying to use an Altivec instruction on a G3
	 5. SIGTRAP	-	trace trap	-	The code reached a breakpoint
	 6. SIGABRT	-	abort program	-	Generated by the abort() function, which can get called when you use the assert() macros
	 Forgetting to removeObserver from the NSNotificationCenter and then releasing that object that needs removed first can cause this signal (sometimes a SIGBUS). One hint you get from PB's debugger is the line "_CFNotificationCenterPostLocalNotification".
	 8. SIGFPE	-	floating-point exception	-	Incorrect use of Altivec code
	 9. SIGKILL	-	terminate process	-	Your process was deliberately terminated
	 10. SIGBUS	-	bus error	-	Incorrect memory access (generally due to incorrect use of retain and release)
	 11. SIGSEGV	-	segmentation violation	-	Use of a pointer that has not been initialized or points to an object that no longer exists (ie: forgot to retain); see also TerminateExplicitListWithNil
	 12. SIGSYS	-	non-existent system call invoked	- Attempt to call a function not in libkern while programming in the kernel
	 13. SIGPIPE	-	write on a pipe with no reader	-	Errors in a process you are interacting with
	 14. SIGALRM	-	real-time timer expired	-	You can set a timer, which just means that this signal will get sent to your program at a point in time in the future
	 15. SIGTERM	-	software termination signal	-	Your process was deliberately terminated
	 17. SIGSTOP	-	stop	-	Another process has suspended yours
	 18. SIGTSTP	-	stop signal from keyboard	-	Control-Z was pressed
	 19. SIGCONT -	continue
	 24. SIGXCPU	-	cpu time limit exceeded	-	Unless you're time-sharing on an XServe, this is extremely unlikely, but self-explanatory. You'll need to have quotas enabled
	 25. SIGXFSZ	-	file size limit exceeded	-	Memory leaks, loops that get stuck writing files
	 }
	 */
	
	signal(SIGABRT, SignalHandler);
	signal(SIGILL, SignalHandler);
	signal(SIGSEGV, SignalHandler);
	signal(SIGFPE, SignalHandler);
	signal(SIGBUS, SignalHandler);
	signal(SIGPIPE, SignalHandler);
	signal(SIGHUP, SignalHandler);
	
	// normal quit  ?
	signal(SIGINT, SignalHandler);
	signal(SIGSTOP, SignalHandler);
	signal(SIGQUIT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGTSTP, SignalHandler);
	
	printf("Exception Handler installed\r");
	
}


// Callback for the notification
static void myNotificationCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
	
    // We get the new price out of the user info dictionary
    // We also get the stock symbol out of there (true, the object is also
    //   the stock symbol, but that can be NULL, if the registry was made for "all")
    std::cout<<"notification !"<< '\n';
	
	CFShow(name);
	
    if (userInfo) {
		std::cout<<"notification !"<< '\n';
		/*
		 CFNumberRef newPriceNum = CFDictionaryGetValue(userInfo, CFSTR("NSWorkspaceApplicationKey"));
		 if (newPriceNum) CFNumberGetValue(newPriceNum, kCFNumberDoubleType, &newPrice);
		 stockSymbol = CFDictionaryGetValue(userInfo, CFSTR("Stock Symbol"));
		 */
    }	
}

#pragma mark main
int main (int argc, const char * argv[]) {
	printf("%s", argv[0]);
	
	// Set up a signal handler so we can clean up when we're interrupted from the command line
	InstallExceptionHandler();
	
	CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
	
	//CFNotificationCenterAddObserver(CFNotificationCenterRef center, const void *observer, CFNotificationCallback callBack, CFStringRef name, const void *object, CFNotificationSuspensionBehavior suspensionBehavior);
	CFNotificationCenterAddObserver(center, NULL, myNotificationCallback,CFSTR(kIOPublishNotification),  NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
	
	// init client list array
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
	
	// init networkUtils
	networkUtils.init();
	
	// init osc receiver
	receiver.setup(_RECEIVER_PORT);
	
	// unload running switchboard launchd process
	myLaunchd_wrapper.removeProcess(PROCESS_LABEL);
	
	// start the loop
	CFRunLoopRun();
	
	sendProcessListToWebConsoleThread.stop();
	
	// unload running switchboard launchd process
	myLaunchd_wrapper.removeProcess(PROCESS_LABEL);
	
	printf("END\n");
	return 0;
}


