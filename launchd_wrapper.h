#ifndef LAUNCHD_WRAPPER
#define LAUNCHD_WRAPPER

/************************************************************
 
 launchd_wrapper class
 
 File: 			launchd_wrapper.h
 Description: 	launchd wrapper
 
 
 Last Modified: 2011.01.14
 Creator: Guillaume Stagnaro
 
 ************************************************************/



#include <CoreFoundation/CoreFoundation.h>
#include <Security/Authorization.h>
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <streambuf>

#include <fcntl.h>

#include <launch.h>

using namespace std;
extern char **environ;

class launchd_wrapper{
	
public:
	
	launchd_wrapper();
	~launchd_wrapper();
	
	void setProcessPath(CFStringRef _processPath);
	void setProcessLabel(CFStringRef _processLabel);
	void setKeepAlive(bool _keepAlive);
	void setThrottleInterval(int _throttleInterval);

	string listProcess();
	
	bool isProcessRunning(const char *_processLabel);
	int getPIDForProcessLabel(const char *_processLabel);
	
	int submitProcess(const char *_processLabel, const char *_processPath);
	int removeProcess(const char *_processLabel);
	
private:
	
	CFStringRef		processPath;
	CFStringRef		processLabel;
	bool			keepAlive;
	int				throttleInterval;
	int				lastLaunchedProcessPID;
	
};




#endif
