/***********************************************************************
 ** guillaume stagnaro
 ** Copyright (c) 2011. All rights reserved.
 **********************************************************************/

#include "launchdWrapper.h"

/************************************************************
 
 launchd_wrapper class
 
 File: 			launchdWrapper.cpp
 Description: 	launchd wrapper
 
 
 Last Modified: 2011.01.14
 Creator: Guillaume Stagnaro
 
 ************************************************************/


//----------------------------------------------------------
launchdWrapper::launchdWrapper(){
	
}

//----------------------------------------------------------
launchdWrapper::~launchdWrapper(){
	
}

//----------------------------------------------------------
void launchdWrapper::setProcessPath(CFStringRef _processPath){
	//CFShow(_processPath);
	processPath = _processPath;
	
}
//----------------------------------------------------------
void launchdWrapper::setProcessLabel(CFStringRef _processLabel){
	//CFShow(_processLabel);
	processLabel = _processLabel;
	
}
//----------------------------------------------------------
void launchdWrapper::setKeepAlive(bool _keepAlive){
	//printf("keepAlive is set\n");
	keepAlive = _keepAlive;
	
}
//----------------------------------------------------------
void launchdWrapper::setThrottleInterval(int _throttleInterval){
	//printf("throttleInterval is set\n");
	throttleInterval = _throttleInterval;
}

//----------------------------------------------------------
bool launchdWrapper::isProcessRunning(const char *_processLabel){
	// Use launch lib to buid a dictionnary of a certain process with process label.
	// Return dictionnary lookup result.
	launch_data_t resp, msg =NULL;
	launch_data_t exist;
	
	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	
	launch_data_dict_insert(msg, launch_data_new_string(_processLabel), LAUNCH_KEY_GETJOB);
	
	resp = launch_msg(msg);
	
	launch_data_free(msg);
	
	if (resp == NULL) {
		fprintf(stderr, "launch_msg(): %s\n", strerror(errno));
		launch_data_free(resp);
		return false; // error
	}	
	
	
	exist = launch_data_dict_lookup(resp, LAUNCH_JOBKEY_LABEL);
	
	launch_data_free(resp);
	
	return	(exist);
}

int launchdWrapper::getPIDForProcessLabel(const char *_processLabel){
	
	launch_data_t resp, msg =NULL;
	launch_data_t pido;
	
	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	
	launch_data_dict_insert(msg, launch_data_new_string(_processLabel), LAUNCH_KEY_GETJOB);
	
	resp = launch_msg(msg);
	
	launch_data_free(msg);
	
	if (resp == NULL) {
		fprintf(stderr, "launch_msg(): %s\n", strerror(errno));
		launch_data_free(resp);
		return false; // error
	}	
	
	pido = launch_data_dict_lookup(resp, LAUNCH_JOBKEY_PID);
	
	launch_data_free(resp);
	
	if (pido)
		return launch_data_get_integer(pido);
	
	return	0;
}

//----------------------------------------------------------

int launchdWrapper::submitProcess(const char *_processLabel, const char *_processPath)
{
	launch_data_t msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_t job = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_t resp = launch_data_alloc(LAUNCH_DATA_ARRAY);
	int r = 0;
	
	launch_data_dict_insert(job, launch_data_new_bool(true), LAUNCH_JOBKEY_KEEPALIVE); // keep alive
	
	launch_data_dict_insert(job, launch_data_new_integer(1), LAUNCH_JOBKEY_THROTTLEINTERVAL); // keep alive
	
	launch_data_dict_insert(job, launch_data_new_integer(1), LAUNCH_JOBKEY_STARTCALENDARINTERVAL); // keep alive
	
	launch_data_dict_insert(job, launch_data_new_string(_processLabel), LAUNCH_JOBKEY_LABEL);
	launch_data_dict_insert(job, launch_data_new_string(_processPath), LAUNCH_JOBKEY_PROGRAM);
	//launch_data_dict_insert(job, launch_data_new_string(optarg), LAUNCH_JOBKEY_STANDARDOUTPATH);
	//launch_data_dict_insert(job, launch_data_new_string(optarg), LAUNCH_JOBKEY_STANDARDERRORPATH);
	
	launch_data_dict_insert(msg, job, LAUNCH_KEY_SUBMITJOB);
	
	resp = launch_msg(msg);
	launch_data_free(msg);
	
	if (resp == NULL) {
		fprintf(stderr, "submit process launch_msg(): %s\n", strerror(errno));
		return 1;
		
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
		errno = launch_data_get_errno(resp);
		if (errno) {
			fprintf(stderr, "submit process error: %s\n", strerror(errno));
			r = 1;
		}
	} else {
		fprintf(stderr, "submit process error: %s\n", "unknown response");
	}
	
	launch_data_free(resp);
	
	return r;
}
//----------------------------------------------------------

int launchdWrapper::removeProcess(const char *_processLabel)
{
	
	launch_data_t resp, msg;
	const char *lmsgcmd = LAUNCH_KEY_REMOVEJOB;
	int e, r = 0;
	
	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_dict_insert(msg, launch_data_new_string(_processLabel), lmsgcmd);
	
	resp = launch_msg(msg);
	launch_data_free(msg);
	
	if (resp == NULL) {
		fprintf(stderr, "launch_msg(): %s\n", strerror(errno));
		return 1;
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
		if ((e = launch_data_get_errno(resp))) {
			fprintf(stderr, "%s %s error: %s\n", getprogname(), _processLabel, strerror(e));
			r = 1;
		}
	} else {
		fprintf(stderr, "%s %s returned unknown response\n", getprogname(), _processLabel);
		r = 1;
	}
	
	launch_data_free(resp);
	return r;
	
}
//----------------------------------------------------------
string launchdWrapper::listProcess(){
	return 0;
}