#include "Switchboard.h"


#define CFReleaseIfNotNULL(cf) if( cf ) CFRelease(cf);
#define CURL_UPDATE_INTERVAL 1
#define OSC_SENDER_PORT 4444
#define OSC_RECEIVER_PORT 5555
#define DELAY_BEFORE_NEXT_PROCESS_LAUNCH 5

#define PROCESS_LABEL "org.monolithe.process"

#define PROCESS_PLIST_FILE_NAME "switchboardProcess.plist"
#define FOLDER_WATCHER_PLIST_FILE_NAME "folderWatcher.plist"

#define DEFAULT_WEB_CONSOLE_URL "http://localhost/~guillaume/monolithe_web_console/"
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

#pragma mark globals declaration
int activeProcessIndex = -1;
int selectedProcessIndex = -1;
int activeProcessIsDuplex = 0;

CFAbsoluteTime lastLaunchProcessTime = 0;

applicationLister myApplicationLister;
launchdWrapper myLaunchdWrapper;

//osc remote structure
struct remote_t {
    string name;
    string ip;
    CFAbsoluteTime lastPingTime;
    ofxOscSender    osc_sender;
};
// remote array
CFMutableArrayRef remotesArray = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);

// 
CFStringRef location = CFSTR(DEFAULT_LOCATION);
CFStringRef webConsoleURL = CFSTR(DEFAULT_WEB_CONSOLE_URL);
CFStringRef activeProcessName = NULL;
CFStringRef duplexProcessName = NULL;
CFStringRef duplexProcessLocation = NULL;
// osc
ofxOscReceiver	osc_receiver;
string          lastRemoteIp="";

ofxNetworkUtils networkUtils;

// xml
TiXmlDocument calendar_xml("calendar.xml");
TiXmlDocument uploads_xml("uploads.xml");
TiXmlDocument duplex_xml;


# pragma mark curl Send Process List call back
size_t curlSendProcessListToWebConsoleCallBack( void *ptr, size_t size, size_t nmemb, void *stream){
	
    duplex_xml.Parse((const char*)ptr);
    
	if ( duplex_xml.Error() )
	{
		NSLog(CFSTR("duplex xml error %s: %i"), duplex_xml.Value(), duplex_xml.ErrorId() );
		return nmemb;
	} else {
       // NSLog(CFSTR("duplex xml ok")); 
    }
	
	int count = 0;
	
	TiXmlNode* node = 0;
	TiXmlElement* element = 0;
    
    duplexProcessName = NULL;
    duplexProcessLocation = NULL;
	while ( (node = duplex_xml.FirstChildElement()->IterateChildren( node ) ) != 0 ) {
		
		element = node->ToElement();
        if (atoi(element->Attribute("duplex"))==1){
            
          //  NSLog(CFSTR("got duplex attribute"));
            // Check if other location process is duplex
            CFStringRef locationAsCFString = CFStringCreateWithCString(
                                                                       kCFAllocatorDefault,
                                                                       element->Attribute("location"),CFStringGetSystemEncoding()
                                                                       );
            
            if (CFStringCompare(location,locationAsCFString , kCFCompareCaseInsensitive)!=kCFCompareEqualTo){
            //if (1){
                    
               // NSLog(CFSTR("duplex %s from %s"), element->Attribute("name"),element->Attribute("location"));
                
                duplexProcessName =  CFStringCreateWithCString(
                                                               kCFAllocatorDefault,
                                                               element->Attribute("name"),
                                                               CFStringGetSystemEncoding()
                                                               );
                
                duplexProcessLocation =  CFStringCreateWithCString(
                                                               kCFAllocatorDefault,
                                                               element->Attribute("location"),
                                                               CFStringGetSystemEncoding()
                                                               );
                
                // send  duplex message to all remotes
               /*
                ofxOscMessage sm;
                sm.setAddress("/monolithe/duplex");
                sm.addStringArg(element->Attribute("name"));	
                
                CFIndex remoteCount =  CFArrayGetCount(remotesArray);
                for (int i = 0; i<remoteCount;i++){
                    struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,i); 
                    aRemote->osc_sender.sendMessage(sm);
                }*/
                
            }
        }
		
		++count;
	}
   // NSLog(CFSTR("count is %i"), count);
	duplex_xml.Clear();
	
	return nmemb;
}

# pragma mark curl get calendar call back
size_t curlGetCalendarCallBack( void *ptr, size_t size, size_t nmemb, void *stream){
	
	calendar_xml.Parse((const char*)ptr);
	
	if ( calendar_xml.Error() )
	{
		NSLog(CFSTR("xml error %s: %i"), calendar_xml.Value(), calendar_xml.ErrorId() );
		return nmemb;
	}
	
	int count = 0;
	
	TiXmlNode* node = 0;
	TiXmlElement* element = 0;
	while ( (node = calendar_xml.FirstChildElement()->IterateChildren( node ) ) != 0 ) {
		
		element = node->ToElement();
		
		NSLog(CFSTR("%s %s %s"), element->Attribute("timestamp"),element->Attribute("action"),element->Attribute("argument"));
		
		if (strcmp(element->Attribute("action"),"launch_now")==0){
			int desire_process_index = atoi(element->Attribute("argument"));
			
			NSLog(CFSTR("%s %i"),element->Attribute("action"),atoi(element->Attribute("argument")));
			
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
	
	NSLog(CFSTR("Clalendar updated %i events"), count);
	calendar_xml.Clear();
	
	return nmemb;
}

# pragma mark web console commnication
void sendProcessListToWebConsole(){
	
	// log time info
	time_t rawtime;
	struct tm * timeinfo;
	
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	
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
					 CURLFORM_END);
		
	}
	
	
	char idch[4];
	sprintf(idch, "%i",activeProcessIndex);
	
	char duplexch[4];
	sprintf(duplexch, "%i",activeProcessIsDuplex);
	
	
    
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_active_index",
				 CURLFORM_COPYCONTENTS, idch, CURLFORM_END);
	
	
	// send active process name
	char *activeProcessNameAsBytes;
    
    if (NULL!=activeProcessName){
        CFIndex activeProcessNAmeLength = CFStringGetLength(activeProcessName);
        activeProcessNameAsBytes = (char *)malloc( activeProcessNAmeLength + 1 );
        activeProcessNameAsBytes = (char*)CFStringGetCStringPtr(activeProcessName, encodingMethod);
        
        curl_formadd(&post, &last,
                     CURLFORM_COPYNAME, "process_active_name",
                     CURLFORM_COPYCONTENTS, activeProcessNameAsBytes, CURLFORM_END);
        
        
        
    } else {
       	curl_formadd(&post, &last,
                     CURLFORM_COPYNAME, "process_active_name",
                     CURLFORM_COPYCONTENTS, "", CURLFORM_END);
        
    }
    
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_active_is_duplex",
				 CURLFORM_COPYCONTENTS, duplexch, CURLFORM_END);
	
	
	char *locationAsBytes;
	CFIndex locationLength = CFStringGetLength(location);
	locationAsBytes = (char *)malloc( locationLength + 1 );
	locationAsBytes = (char*)CFStringGetCStringPtr(location, encodingMethod);
	
	
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "location",
				 CURLFORM_COPYCONTENTS, locationAsBytes, CURLFORM_END);
	    
	curl_formadd(&post, &last,
				 CURLFORM_COPYNAME, "process_list_submit",
				 CURLFORM_COPYCONTENTS, "submit", CURLFORM_END);
	
	
	/* Set the form info */
	
	// CFString to c_string
	char *webConsoleURLAsBytes;
	CFIndex webConsoleURLLength = CFStringGetLength(webConsoleURL);
	webConsoleURLAsBytes = (char *)malloc( webConsoleURLLength + 1 );
	webConsoleURLAsBytes = (char*)CFStringGetCStringPtr(webConsoleURL, encodingMethod);
	
	curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post);
	curl_easy_setopt(hnd, CURLOPT_URL, webConsoleURLAsBytes);
	    
	ret = curl_easy_perform(hnd);
	curl_easy_cleanup(hnd);
	
    
    // free ?
    /*
     free(activeProcessNameAsBytes);
    free(locationAsBytes);
    free(webConsoleURLAsBytes);
     */
    
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
# pragma mark process waiting osc messages
void oscMessagesTimerFunction(CFRunLoopTimerRef timer, void *info)
{
    // remove timed out remote
    CFIndex idx = 0;
    CFIndex currentRemoteIndex = -1;
    
    while (idx !=  CFArrayGetCount(remotesArray)){
        struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,idx); 
        CFTimeInterval pingAge =  CFAbsoluteTimeGetCurrent () -aRemote->lastPingTime;
        if (pingAge > 10){
            NSLog(CFSTR("remove remote: %s"),aRemote->ip.c_str() );
            CFArraySetValueAtIndex(remotesArray, idx, NULL);
            CFArrayRemoveValueAtIndex(remotesArray, idx);
        }
        else idx++;
    } 
    
	while( osc_receiver.hasWaitingMessages() )
	{
		ofxOscMessage rm;
		osc_receiver.getNextMessage( &rm );
		
        // remote array stuff
        string remoteIp = rm.getRemoteIp();
        CFIndex remoteCount =  CFArrayGetCount(remotesArray);
        
        for (int i = 0; i<remoteCount;i++){
            struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,i); 
            string anIp =aRemote->ip;
            
            if (remoteIp.compare(anIp)==0) {
                //update remote last ping time
                currentRemoteIndex = i;
                aRemote->lastPingTime = CFAbsoluteTimeGetCurrent ();
                // NSLog(CFSTR("update remote %s"), anIp .c_str());
            }
        }
        
        if (currentRemoteIndex==-1){ // new remote
            struct remote_t *newRemote = new remote_t;
            if (newRemote!=NULL){
                newRemote->ip = remoteIp;
                newRemote->lastPingTime = CFAbsoluteTimeGetCurrent();
                newRemote->name = "no name";
                newRemote->osc_sender.setup(remoteIp, OSC_SENDER_PORT);
                
                CFArrayAppendValue(remotesArray, newRemote);
                NSLog(CFSTR("new remote: %s"), newRemote->ip.c_str() );
                
                currentRemoteIndex = remoteCount;
            } else{
                NSLog(CFSTR("Error creating new remote"));
            }
        }
        
# pragma mark osc ping
        
        struct remote_t *currentRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,currentRemoteIndex); 
        
		if ( rm.getAddress() == "/monolithe/ping" )
		{
			ofxOscMessage sm;
			
			sm.setAddress("/monolithe/pong"); // pong back to remote
			sm.addStringArg(networkUtils.getInterfaceAddress(0));
			currentRemote->osc_sender.sendMessage(sm);
			
            // send duplex message if needed
            if (duplexProcessName!=NULL){
                sm.clear();
                sm.setAddress("/monolithe/duplex");
                char *duplexProcessNameAsBytes;
                CFIndex length = CFStringGetLength(duplexProcessName);
                duplexProcessNameAsBytes = (char*)malloc( length + 1 );
                duplexProcessNameAsBytes = (char*)CFStringGetCStringPtr(duplexProcessName,CFStringGetSystemEncoding());
                
                char *duplexProcessLocationAsBytes;
                length = CFStringGetLength(duplexProcessLocation);
                duplexProcessLocationAsBytes = (char*)malloc( length + 1 );
                duplexProcessLocationAsBytes = (char*)CFStringGetCStringPtr(duplexProcessLocation,CFStringGetSystemEncoding());
                
                sm.addStringArg(string(duplexProcessNameAsBytes));
                sm.addStringArg(string(duplexProcessLocationAsBytes));
                
              //  printf("%s, %s\n", duplexProcessLocationAsBytes,duplexProcessNameAsBytes );
                currentRemote->osc_sender.sendMessage(sm);
            }
			break;
		}
		
        if ( rm.getAddress() == "/monolithe/whereami" )
		{
           
            // CFString to c_string
            CFStringEncoding encodingMethod;
            encodingMethod = CFStringGetSystemEncoding();
            
            
            char *locationAsBytes;
            CFIndex locationLength = CFStringGetLength(location);
            locationAsBytes = (char *)malloc( locationLength + 1 );
            locationAsBytes = (char*)CFStringGetCStringPtr(location, encodingMethod);
            
            string s = locationAsBytes;
            
            //free (locationAsBytes);
            
            ofxOscMessage sm;
			
			sm.setAddress("/monolithe/whereyouare"); // pong back to remote
			sm.addStringArg(s);
			currentRemote->osc_sender.sendMessage(sm);
            
            break;
            
        }
		
		// launch
# pragma mark osc launch
		if ( rm.getAddress() == "/monolithe/launchprocess" )
		{
			CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
			if (now<lastLaunchProcessTime+DELAY_BEFORE_NEXT_PROCESS_LAUNCH){
				NSLog(CFSTR("to soon, now is %f, last is %f"), now, lastLaunchProcessTime);
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
				appIndex = myApplicationLister.getApplicationIndex(
                                                                   CFStringCreateWithCString(
                                                                                             kCFAllocatorDefault, 
                                                                                             appName.c_str(),
                                                                                             kCFStringEncodingUTF8
                                                                                             )
                                                                   );
				
			}
			
			if (appIndex == activeProcessIndex) break; // don't relaunch current app
			if (appIndex == -1) break; // don't relaunch current app
			
			// hide mouse cursor
			CGDisplayHideCursor (kCGDirectMainDisplay);
			CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, CGPointMake(1279, 799));
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
			activeProcessName = myApplicationLister.getApplicationName(appIndex);
			
            
            NSLog(CFSTR("Starting process %@"),activeProcessName);
            
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
				NSLog(CFSTR("Process is duplex !"));
				activeProcessIsDuplex = true;
				
			} else {
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
			//printf("pid is %i\r", pid);
			
			ProcessSerialNumber psn;
			GetProcessForPID (
							  (long) pid,
							  &psn
							  );
			
			OSErr err = SetFrontProcess(&psn);
            if(err) NSLog(CFSTR("Error set front process"));
			//printf("error ? %i\r", er);
			
            // send back process index to all remotes
            ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	

            CFIndex remoteCount =  CFArrayGetCount(remotesArray);
            for (int i = 0; i<remoteCount;i++){
                struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,i); 
                aRemote->osc_sender.sendMessage(sm);
            }
			sendProcessListToWebConsoleThread.start();
			
			break;
		}
		
		// send process list
# pragma mark osc process list
		if ( rm.getAddress() == "/monolithe/getprocesslist" )	
		{
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
					NSLog(CFSTR("error getting process name"));
				}					
			}
			
			currentRemote->osc_sender.sendMessage(sm);
			
			break;
		}
		
		// send current process index
# pragma mark osc process index
		if ( rm.getAddress() == "/monolithe/getprocessindex" ) 
		{
			ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	
			
			currentRemote->osc_sender.sendMessage(sm);
            
			break;
			
		}
        
		// send current process name
# pragma mark osc process name
		if ( rm.getAddress() == "/monolithe/getprocessname" ) 
		{
			ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessname");
			
            // CFStringRef to CString complicated mess
            CFIndex          nameLength;
            Boolean          success;
            
            CFStringRef appName = myApplicationLister.getApplicationName(activeProcessIndex);
            
            if (appName == NULL) // double check if app name is not null.
                appName = CFSTR("Unnamed App");
            
            nameLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(appName), kCFStringEncodingASCII);
            
            char             filePath[FILENAME_MAX]; // buffer[nameLength+1];
            assert(filePath != NULL);
            
            success = CFStringGetCString(appName, filePath, FILENAME_MAX, kCFStringEncodingASCII);
            
            if (success){
                sm.addStringArg(filePath);
                
            } else {
                NSLog(CFSTR("error getting process name"));
            }					
			currentRemote->osc_sender.sendMessage(sm);
            
			break;
			
		}
		
# pragma mark osc process description
		if ( rm.getAddress() == "/monolithe/getprocessdescription" )	
		{
			// get osc argument
			int appIndex = rm.getArgAsInt32( 0 );
			
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
				NSLog(CFSTR("loading %s..."),filePath);
				ifstream existanceChecker(filePath);
				
				if(!existanceChecker.is_open()){
					NSLog(CFSTR("file %s not found"),filePath);
					return;
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
				
				sm.addStringArg(buffer);
				
				delete[] buffer;
				
				currentRemote->osc_sender.sendMessage(sm);
				break;
				
			}else NSLog(CFSTR("error getting application description"));
			
		}					
		
        
# pragma mark osc process description
		if ( rm.getAddress() == "/monolithe/getallprocessdescriptions" )	
		{
            
          	//ofxOscBundle sb;
            
            //printf("get all %i process\n",myApplicationLister.getApplicationCount());
            
			for (int i = 0; i<myApplicationLister.getApplicationCount();i++){
                
                printf("process %i\n",i);
                
                ofxOscMessage sm;
                sm.setAddress("/monolithe/setprocessdescription");
                
                CFIndex          pathLength;
                Boolean          success;
                
                CFStringRef appPath = myApplicationLister.getApplicationEnclosingDirectoryPath(i);
                CFMutableStringRef appXmlDescriptionFilePath= CFStringCreateMutable(NULL, 0);
                
                if (appPath == NULL) // double check if app name is not null.
                    appPath = CFSTR("Path error");
                
                // build path to xml description file (path/application_name.xml)
                CFStringAppend(appXmlDescriptionFilePath, appPath);
                CFStringAppend(appXmlDescriptionFilePath,CFSTR("/"));
                CFStringAppend(appXmlDescriptionFilePath, myApplicationLister.getApplicationName(i));
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
                    NSLog(CFSTR("loading %s..."),filePath);
                    ifstream existanceChecker(filePath);
                    
                    if(!existanceChecker.is_open()){
                        NSLog(CFSTR("file %s not found"),filePath);
                        continue;
                    }
                    
                    existanceChecker.close();
                    
                    ifstream::pos_type length;
                    char * buffer;
                    
                    std::ifstream inFile(filePath,ios::in|ios::binary|ios::ate);
                    if (!inFile.is_open())
                        continue;
                    
                    // get length of file:
                    inFile.seekg (0, ios::end);
                    length = inFile.tellg();
                    inFile.seekg (0, ios::beg);
                    
                    // allocate memory:
                    buffer = new char [length];
                    
                    // read data as a block:
                    inFile.read (buffer,length);
                    
                    inFile.close();
                    
                    sm.addStringArg(buffer);
                    
                    delete[] buffer;
                    
                    currentRemote->osc_sender.sendMessage(sm);
                    
                    
                }else NSLog(CFSTR("error getting application description"));
			}
            break;
        }
		// stop current process
# pragma mark osc stop process
		if ( rm.getAddress() == "/monolithe/stopprocess" ) 
		{
			myLaunchdWrapper.removeProcess(PROCESS_LABEL); // unload current process
			
			activeProcessIndex = -1;
			activeProcessName = NULL;
            
            // send back process index to all remotes
            ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	
            
            CFIndex remoteCount =  CFArrayGetCount(remotesArray);
            for (int i = 0; i<remoteCount;i++){
                struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,i); 
                aRemote->osc_sender.sendMessage(sm);
            }
            
			sendProcessListToWebConsoleThread.start();
			break;
		}
		
		// new application aviable
		if ( rm.getAddress() == "/monolithe/newapplication" ) 
		{
			
            // send back process index to all remotes
            ofxOscMessage sm;
			sm.setAddress("/monolithe/setprocessindex");
			sm.addIntArg(activeProcessIndex);	
            
            CFIndex remoteCount =  CFArrayGetCount(remotesArray);
            for (int i = 0; i<remoteCount;i++){
                struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,i); 
                aRemote->osc_sender.sendMessage(sm);
            }
            
			sendProcessListToWebConsoleThread.start();
			break;
			
		}
        
        if ( rm.getAddress() == "/monolithe/reset" ) 
		{
			
            // send back reset to all remotes
            ofxOscMessage sm;
			sm.setAddress("/monolithe/reset");
			sm.addIntArg(activeProcessIndex);	
            
            CFIndex remoteCount =  CFArrayGetCount(remotesArray);
            for (int i = 0; i<remoteCount;i++){
                struct remote_t *aRemote =(remote_t *) CFArrayGetValueAtIndex(remotesArray,i); 
                aRemote->osc_sender.sendMessage(sm);
            }
            
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

# pragma mark preferences
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
	
	
	//if (!noErr==err) return;
	
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
	
	
	NSLog(CFSTR("Location : %@"), location);
	NSLog(CFSTR("Web Console URL : %@"), webConsoleURL);
    
}

# pragma mark main
int main (int argc, const char * argv[]) {
    
    NSLog(CFSTR("\n\n--------------------------\nWelcome to the SwitchBoard\n--------------------------\n"));
	// Set up a signal handler so we can clean up when we're interrupted from the command line
	InstallExceptionHandler();
	
	// Read preferences
	readPreferences();
	
	// Setup runLoops
	CFRunLoopRef		mRunLoopRef;
	mRunLoopRef = CFRunLoopGetCurrent();
	
	CFRunLoopTimerRef oscTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 1, 1, 0, 0, oscMessagesTimerFunction, NULL); // 1 sec
	CFRunLoopTimerRef webConsoleTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 5, 5, 0, 0, webConsoleTimerFunction, NULL); // 5 sec
	
	CFRunLoopAddTimer(mRunLoopRef, oscTimer, kCFRunLoopDefaultMode);
	CFRunLoopAddTimer(mRunLoopRef, webConsoleTimer, kCFRunLoopDefaultMode);
	
	// Init networkUtils
	networkUtils.init();
	
	// Init osc receiver
	osc_receiver.setup(OSC_RECEIVER_PORT);
	
	// unload running switchboard launchd process
	myLaunchdWrapper.removeProcess(PROCESS_LABEL);
	
	
	// Start the loop
	CFRunLoopRun();
	
	sendProcessListToWebConsoleThread.stop();
	
	// Unload running switchboard launchd process
	myLaunchdWrapper.removeProcess(PROCESS_LABEL);
	
	NSLog(CFSTR("Switchboard END"));
	return 0;
}


