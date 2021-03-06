/*
 *  Switchboard.h
 *  Switchboard
 *
 *  Created by guillaume on 08/04/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 */

#ifndef SWITCHBOARD
#define SWITCHBOARD

#define TARGET_OSX
#define _VERSION "0.02"

//#define TIXML_USE_STL

#ifdef TIXML_USE_STL
	#include <iostream>
	#include <sstream>
	using namespace std;
#else
	#include <stdio.h>
#endif

#include <CoreFoundation/CoreFoundation.h>

#include <signal.h>

#include <time.h>
#include "sys/time.h"

#include "applicationLister.h"

#include "launchdWrapper.h"

#include "ip/UdpSocket.h"
#include "ip/PacketListener.h"

#include "thread/thread_runner.h"

#include "ofxOsc.h"

#include "ofxNetworkUtils.h"

#include "screen_fader.h"

#include "curl/curl.h"

#include "tinyxml/tinyxml.h"

#include <stdexcept> 

double getSystemTime();

// signal handler
void SignalHandler(int signal);
void InstallExceptionHandler();

// curl callbacks
size_t curlSendProcessListToWebConsoleCallBack( void *ptr, size_t size, size_t nmemb, void *stream);
size_t curlGetCalendarCallBack( void *ptr, size_t size, size_t nmemb, void *stream);

// web console synchronisation
void sendProcessListToWebConsole();
void getCalendarFromWebConsole();

// runloop timer functions
void oscMessagesTimerFunction(CFRunLoopTimerRef timer, void *info);
void webConsoleTimerFunction(CFRunLoopTimerRef timer, void *info);

// read preferences
void readPreferences();

// main function
int main (int argc, const char * argv[]);

// curl threaded function
#include "curlSendProcessListThreadedObject.h"
curlSendProcessListThreadedObject sendProcessListToWebConsoleThread;


#endif