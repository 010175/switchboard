#include "Switchboard.h"


#define CFReleaseIfNotNULL(cf) if( cf ) CFRelease(cf);
#define CURL_UPDATE_INTERVAL 1
#define _SENDER_PORT 4444
#define _RECEIVER_PORT 5555
#define DELAY_BEFORE_NEXT_PROCESS_LAUNCH 5

#define PROCESS_LABEL "org.monolithe.process"

#define PROCESS_PLIST_FILE_NAME "switchboardProcess.plist"
#define FOLDER_WATCHER_PLIST_FILE_NAME "folderWatcher.plist"

#define DEFAULT_WEB_CONSOLE_URL "http://localhost/~guillaume/monolithe_web_console/"
//#define MONOLITHE_WEB_CONSOLE_URL "http://www.010175.net/monolithe/"

#define DEFAULT_LOCATION "nowhere"

// for NSLog
#if __cplusplus
extern "C" {
#endif
	void NSLog(CFStringRef format, ...);
	void NSLogv(CFStringRef format, va_list args);
#if __cplusplus
}
#endif

int activeProcessIndex = -1;
int selectedProcessIndex = -1;
int activeProcessIsDuplex = 0;

double lastLaunchProcessTime = 0;

applicationLister myApplicationLister;
launchdWrapper myLaunchdWrapper;

// 
CFStringRef location = CFSTR(DEFAULT_LOCATION);
CFStringRef webConsoleURL = CFSTR(DEFAULT_WEB_CONSOLE_URL);

// osc
ofxOscSender	sender;
ofxOscReceiver	receiver;

ofxNetworkUtils networkUtils;

TiXmlDocument calendar_xml("calendar.xml");
TiXmlDocument uploads_xml("uploads.xml");
TiXmlDocument duplex_xml;

#pragma mark get systeme time 
double getSystemTime(){
	
	timeval Time = {0, 0};
	gettimeofday(&Time, NULL);
	
	return Time.tv_sec + Time.tv_usec / 1000000.0;	
	
}

#pragma mark curl Send Process List call back
size_t curlSendProcessListToWebConsoleCallBack( void *ptr, size_t size, size_t nmemb, void *stream){
	
	//printf("web console call back: %s\n",(char*) ptr);
	
	
	CFXMLTreeRef    cfXMLTree;
	CFDataRef       xmlData;
	char			buffer[1024];
	CFIndex			bufferLenght;
	
	bufferLenght = sprintf(buffer, "%s",(char*) ptr);
	
	printf("%s", buffer);
	
	xmlData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)buffer, bufferLenght);
	
	
	// Parse the XML and get the CFXMLTree.
	cfXMLTree = CFXMLTreeCreateFromData(kCFAllocatorDefault,
										xmlData,
										NULL,
										kCFXMLParserSkipWhitespace,
										kCFXMLNodeCurrentVersion);
	
	
	
	CFXMLTreeRef    xmlTreeNode;
	CFXMLNodeRef    xmlNode;
	int             childCount;
	int             index;
	
	if (NULL==cfXMLTree) return 0;
	
	// Get a count of the top level node’s children.
	childCount = CFTreeGetChildCount(cfXMLTree);
	
	// Print the data string for each top-level node.
	for (index = 0; index < childCount; index++) {
		
		CFXMLTreeRef    xmlTreeSubNode;
		CFXMLNodeRef    xmlSubNode;
		int             subChildCount;
		int             subIndex;
		
		xmlTreeNode = CFTreeGetChildAtIndex(cfXMLTree, index);
		xmlNode = CFXMLTreeGetNode(xmlTreeNode);
		
		CFShow(CFXMLNodeGetString(xmlNode));
		
		subChildCount = CFTreeGetChildCount(xmlTreeNode);
		for (subIndex = 0; subIndex < subChildCount; subIndex++) {
			
			xmlTreeSubNode =  CFTreeGetChildAtIndex(xmlTreeNode, subIndex);
			xmlSubNode = CFXMLTreeGetNode(xmlTreeSubNode);
			//CFXMLNodeRef pp = 
			CFShow(CFXMLNodeGetString(xmlSubNode));
		}
		
	}
	
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
				
				CFStringRef appPath = myApplicationLister.getApplicationPath(desire_process_index);
				doFadeOperation(FillScreen, 0.2f, true);
				
				if (appPath == NULL) // double check if app name is not null.
					appPath = CFSTR("Unnamed App");
				
				myLaunchdWrapper.removeProcess(PROCESS_LABEL);	// load new process
				
				usleep(1000000); // give some break
				
				int pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appPath), kCFStringEncodingASCII);
				char buffer[pathLength+1]; // buffer[nameLength+1];
				assert(buffer != NULL);
				if (CFStringGetCString(appPath, buffer, PATH_MAX, kCFStringEncodingASCII))
				{
					myLaunchdWrapper.submitProcess(PROCESS_LABEL, buffer);
					activeProcessIndex = desire_process_index;
					sleep(2);
				}
				
				doFadeOperation(CleanScreen, 0.2f, true); // fade out
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
	
	//printf("%.2i:%.2i:%.2i Send Process List to Web Console\n", timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	
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
	for (int i= 0 ; i < myApplicationLister.getApplicationCount(); i++){
		
		processNameCFString = myApplicationLister.getApplicationName(i);
		char *bytes;
		CFIndex length = CFStringGetLength(processNameCFString);
		bytes = (char *)malloc( length + 1 );
		bytes = (char*)CFStringGetCStringPtr(processNameCFString, encodingMethod);
		
		curl_formadd(&post, &last,
					 CURLFORM_COPYNAME, "process_list[]",
					 CURLFORM_COPYCONTENTS, bytes,
					 // CURLFORM_COPYCONTENTS, randomch,
					 CURLFORM_END);
		
	}
	
	
	char idch[4];
	sprintf(idch, "%i",activeProcessIndex);
	
	char duplexch[4];
	sprintf(duplexch, "%i",activeProcessIsDuplex);
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_active_index",
				 CURLFORM_COPYCONTENTS, idch, CURLFORM_END);
	
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_active_is_duplex",
				 CURLFORM_COPYCONTENTS, duplexch, CURLFORM_END);
	
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "location",
				 CURLFORM_COPYCONTENTS, DEFAULT_LOCATION, CURLFORM_END);
	
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_list_submit",
				 CURLFORM_COPYCONTENTS, "submit", CURLFORM_END);
	
	
	/* Set the form info */
	curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post);
	curl_easy_setopt(hnd, CURLOPT_URL, DEFAULT_WEB_CONSOLE_URL);
	
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
				 CURLFORM_COPYCONTENTS, DEFAULT_LOCATION, CURLFORM_END);
	
	
	/* Set the form info */
	curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post);
	curl_easy_setopt(hnd, CURLOPT_URL, DEFAULT_WEB_CONSOLE_URL);
	
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
			
			sender.sendMessage(sm);
			
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
			
			int appIndex = -1;
			string appName = "";
			
			if (rm.getArgType(0)==	OFXOSC_TYPE_INT32){ // app by index
				appIndex = rm.getArgAsInt32( 0 );
			}
			else if (rm.getArgType(0)== OFXOSC_TYPE_STRING){ // app by name
				appName = rm.getArgAsString( 0 );
				appIndex = myApplicationLister.getApplicationIndex(CFStringCreateWithCString(kCFAllocatorDefault, appName.c_str(), kCFStringEncodingUTF8));
				
			}
			
			if (appIndex == activeProcessIndex) break; // don't relaunch current app
			
			// hide mouse cursor
			CGDisplayHideCursor (kCGDirectMainDisplay);
			CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, CGPointZero);
			doFadeOperation(FillScreen, 0.2f, true); // fade in
			
			//remove current process
			myLaunchdWrapper.removeProcess(PROCESS_LABEL);
			
			CFStringRef appPath = myApplicationLister.getApplicationPath(appIndex);
			
			if (appPath == NULL) // double check if app name is not null.
				appPath = CFSTR("Unnamed App");
			
			CFIndex pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appPath), kCFStringEncodingASCII);
			char filePath[pathLength+1]; //  PATH_MAX  ou buffer[nameLength+1];
			assert(filePath != NULL);
			Boolean success = CFStringGetCString(appPath, filePath, PATH_MAX, kCFStringEncodingASCII);
			
			if (!success) { 
				doFadeOperation(CleanScreen, 2.0f, true); // fade out
				return;
			}
			myLaunchdWrapper.submitProcess(PROCESS_LABEL, filePath);
			activeProcessIndex = appIndex;
			
			
			// get duplex status
			CFMutableStringRef appDuplexFilePath= CFStringCreateMutable(NULL, 0);
			
			if (appPath == NULL) // double check if app name is not null.
				appPath = CFSTR("Path error");
			
			// build path to duplex  file (path/duplex)
			CFStringAppend(appDuplexFilePath, myApplicationLister.getApplicationEnclosingDirectoryPath(appIndex));
			CFStringAppend(appDuplexFilePath,CFSTR("/"));
			CFStringAppend(appDuplexFilePath, CFSTR("duplex"));
			
			// lenght of path
			pathLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appDuplexFilePath), kCFStringEncodingASCII);
			
			// check if duplex file exist
			char  duplexFilePath[FILENAME_MAX]; // buffer[pathLength+1];
			assert(duplexFilePath != NULL);
			
			// CFString to c_string
			CFStringGetCString(appDuplexFilePath, duplexFilePath, FILENAME_MAX, kCFStringEncodingASCII);
			
			CFRelease(appDuplexFilePath);
			
			// check if duplex file or directory exist
			struct stat st;
			if(stat(duplexFilePath,&st) == 0){
				printf("process is duplex\n");
				activeProcessIsDuplex = true;
				
			} else {
				printf("process is not duplex\n");
				activeProcessIsDuplex= false;
			}
			// end duplex status
			
			int i=5;
			do {
				usleep(2000000); // give some break
			} while ((--i)&&(!myLaunchdWrapper.isProcessRunning(PROCESS_LABEL)));
			
			
			doFadeOperation(CleanScreen, 2.0f, true); // fade out
			
			CGDisplayHideCursor (kCGDirectMainDisplay);
			
			
			// set process to front
			int pid = myLaunchdWrapper.getPIDForProcessLabel(PROCESS_LABEL);
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
			myApplicationLister.updateApplicationsList();
			
			for (int i = 0; i<myApplicationLister.getApplicationCount();i++){
				
				// CFStringRef to CString complicated mess
				CFIndex          nameLength;
				Boolean          success;
				
				CFStringRef appName = myApplicationLister.getApplicationName(i);
				
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
			
			sender.sendMessage(sm);
			
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
			
			sender.sendMessage(sm);
			
			
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
			
			CFStringRef appPath = myApplicationLister.getApplicationEnclosingDirectoryPath(appIndex);
			CFMutableStringRef appXmlDescriptionFilePath= CFStringCreateMutable(NULL, 0);
			
			if (appPath == NULL) // double check if app name is not null.
				appPath = CFSTR("Path error");
			
			// build path to xml description file (path/application_name.xml)
			CFStringAppend(appXmlDescriptionFilePath, appPath);
			CFStringAppend(appXmlDescriptionFilePath,CFSTR("/"));
			CFStringAppend(appXmlDescriptionFilePath, myApplicationLister.getApplicationName(appIndex));
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
				
				ifstream::pos_type length;
				char * buffer;
				
				std::ifstream inFile(filePath,ios::in|ios::binary|ios::ate);
				if (!inFile.is_open())
					return;
				
				// get length of file:
				inFile.seekg (0, ios::end);
				length = inFile.tellg();
				inFile.seekg (0, ios::beg);
				
				// allocate memory:
				buffer = new char [length];
				
				// read data as a block:
				inFile.read (buffer,length);
				
				inFile.close();
				
				/*inFile.seekg (0, ios::beg);
				 size = inFile.tellg();
				 
				 memblock = new char [size];
				 
				 inFile.read (memblock, size);
				 inFile.close();
				 */
				
				cout << "the complete file content is in memory : " << length << "bytes\n";
				
				sm.addStringArg(buffer);
				
				delete[] buffer;
				
				sender.sendMessage(sm);
				break;
				
			}else printf("error getting application description\r");
			
		}					
		
		
		// stop current process
#pragma mark osc stop process
		if ( rm.getAddress() == "/monolithe/stopprocess" ) 
		{
			sender.setup(rm.getRemoteIp(), _SENDER_PORT); // reset sender remote ip
			
			myLaunchdWrapper.removeProcess(PROCESS_LABEL); // unload current process
			
			activeProcessIndex = -1;
			
			sendProcessListToWebConsoleThread.start();
			
			break;
			
		}
		
		// new application aviable
		if ( rm.getAddress() == "/monolithe/newapplication" ) 
		{
			
			ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	
			
			
			sender.sendMessage(sm);
			
			sendProcessListToWebConsoleThread.start();
			break;
			
		}
		
	}
}

void webConsoleTimerFunction(CFRunLoopTimerRef timer, void *info)
{
	//printf("web console timer function");
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
	
	NSLog(CFSTR("Exception Handler installed"));
	
}

#pragma mark preferences
void readPreferences()
{
	
	OSStatus err;
	// build url to xml pref file
    CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef	mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	CFURLRef	preferenceFileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault,
																		  mainBundleURL,
																		  CFSTR("preferences.plist"),
																		  false);
	CFPropertyListRef	propertyList;
	CFDataRef			resourceData;
	CFStringRef			errorString;
	SInt32				errorCode;
	
	// Read the XML file.
	err = CFURLCreateDataAndPropertiesFromResource(
												   kCFAllocatorDefault,
												   preferenceFileURL,
												   &resourceData,            // place to put file data
												   NULL,
												   NULL,
												   &errorCode);
	
	if (!noErr==err) return;
	
	// Reconstitute the dictionary using the XML data.
	if (errorCode==0){
		propertyList = CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
													   resourceData,
													   kCFPropertyListImmutable,
													   &errorString);
		
		CFRelease( resourceData );
		
		if (CFGetTypeID(propertyList)==CFDictionaryGetTypeID()){
			
			// set values from plist file if exist
			CFDictionaryGetValueIfPresent(( CFDictionaryRef )propertyList, CFSTR("Location"), (const void **)&location);
			CFDictionaryGetValueIfPresent(( CFDictionaryRef )propertyList, CFSTR("Web Console URL"), (const void **)&webConsoleURL);
			
		
			
		}
	}else {
		
		NSLog(CFSTR("preferences read error %i"), errorCode);
	}
	
	NSLog(CFSTR("location : %@"), location);
	NSLog(CFSTR("Web Console URL : %@"), webConsoleURL);

}

#pragma mark main
int main (int argc, const char * argv[]) {
	// Set up a signal handler so we can clean up when we're interrupted from the command line
	InstallExceptionHandler();
	
	// Read preferences
	readPreferences();
	
	// Setup runLoops
	CFRunLoopRef		mRunLoopRef;
	mRunLoopRef = CFRunLoopGetCurrent();
	
	CFRunLoopTimerRef oscTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 1, 1, 0, 0, oscMessagesTimerFunction, NULL); // 1 sec
	CFRunLoopTimerRef webConsoleTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 4, 4, 0, 0, webConsoleTimerFunction, NULL); // 4 sec
	
	CFRunLoopAddTimer(mRunLoopRef, oscTimer, kCFRunLoopDefaultMode);
	CFRunLoopAddTimer(mRunLoopRef, webConsoleTimer, kCFRunLoopDefaultMode);
	
	// Init networkUtils
	networkUtils.init();
	
	// Init osc receiver
	receiver.setup(_RECEIVER_PORT);
	
	// unload running switchboard launchd process
	myLaunchdWrapper.removeProcess(PROCESS_LABEL);
	
	
	// Start the loop
	CFRunLoopRun();
	
	sendProcessListToWebConsoleThread.stop();
	
	// Unload running switchboard launchd process
	myLaunchdWrapper.removeProcess(PROCESS_LABEL);
	
	NSLog(CFSTR("END"));
	return 0;
}


