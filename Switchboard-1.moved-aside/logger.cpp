/*
 *  logger.cpp
 *  Switchboard
 *
 *  Created by guillaume on 07/04/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "logger.h"

logger::logger(){
	title = (CFSTR("\33[4m\33[7mUntitled\33[0m"));
	needUpdate = true;
	
	logArray = CFArrayCreateMutable(NULL, 0, NULL);
}

logger::~logger(){
	CFRelease(logArray);
}

void logger::addLog(CFStringRef _log){
	addLog(_log, false);
}

void logger::addLog(CFStringRef _log,  bool _addTimeStamp){
	
	if (_addTimeStamp){
		CFAbsoluteTime timeNow = CFAbsoluteTimeGetCurrent();
		CFDateRef d = CFDateCreate(kCFAllocatorDefault, timeNow);
		
		CFLocaleRef currentLocale = CFLocaleCopyCurrent();
		
		CFDateFormatterRef dateFormatter = CFDateFormatterCreate(NULL, currentLocale, kCFDateFormatterShortStyle, kCFDateFormatterMediumStyle);
		
		CFStringRef t = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, dateFormatter, d);
	
		CFMutableStringRef tmpString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, t);
		CFStringAppend(tmpString, CFSTR("\t"));
		CFStringAppend(tmpString, _log);
		
		CFArrayAppendValue(logArray, tmpString);

		CFRelease(d);
		CFRelease(currentLocale);
		CFRelease(dateFormatter);
		CFRelease(t);
				
	} else {
		
		
		CFArrayAppendValue(logArray, _log);
		
	}
	needUpdate = true;
	
	if (CFArrayGetCount(logArray)>MAX_LOG) CFArrayRemoveValueAtIndex(logArray, 0);
	
	

}

void logger::removeLastLog(){
	size_t logArraySize = CFArrayGetCount(logArray);
	if (logArraySize>0)	CFArrayRemoveValueAtIndex(logArray, logArraySize-1);
	
}

void logger::setTitle(CFStringRef _Title){
	title = _Title;
	
	needUpdate = true;
}

void logger::clearLog(){
	CFArrayRemoveAllValues(logArray);
	needUpdate == true;
}

void logger::showLog(){
	
	if (!needUpdate) return;
	//CFShow(CFSTR("\33[2J")); // clear screen
	//CFShow(CFSTR("\33[4m\33[7m")); //reversed  underline
	CFShow(title);
	//CFShow(CFSTR("\33[0m")); // normal
	
	size_t logArraySize = CFArrayGetCount(logArray);
	

	for (int i = 0; i < logArraySize; i++){ // loop through all logs
		
		CFStringRef s = CFStringCreateCopy(kCFAllocatorDefault, (CFStringRef)CFArrayGetValueAtIndex(logArray, i));
		CFShow(s);
		CFRelease(s);
	}
	
	for(int i = 0; i < (MAX_LOG-logArraySize); i++)CFShow(CFSTR("-"));

	needUpdate = false;
	
}