/*
 *  logger.h
 *  Switchboard
 *
 *  Created by guillaume on 07/04/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>


#define MAX_LOG 22

using namespace std;

class logger{
	
public:
	
	logger();
	~logger();
	
	void addLog(CFStringRef _log, bool _addTimeStamp);
	void addLog(CFStringRef _log);
	void setTitle(CFStringRef _title);
	
	void clearLog();
	void showLog();
	
	void removeLastLog();
	
	private:
	
	CFStringRef title;
	CFMutableArrayRef logArray;
	
	bool needUpdate;

};
