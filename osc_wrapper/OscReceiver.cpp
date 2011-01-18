/*

 Copyright 2007, 2008 Damian Stewart damian@frey.co.nz
 Distributed under the terms of the GNU Lesser General Public License v3

 This file is part of the Osc openFrameworks OSC addon.

 Osc is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Osc is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with Osc.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OscReceiver.h"

#ifndef TARGET_WIN32
        #include <pthread.h>
#endif
#include <iostream>
#include <assert.h>

OscReceiver::OscReceiver()
{
#ifdef TARGET_WIN32
	mutex = CreateMutexA( NULL, FALSE, NULL );
#else
	pthread_mutex_init( &mutex, NULL );
#endif
}

void OscReceiver::setup( int listen_port )
{
	listen_socket = new UdpListeningReceiveSocket( IpEndpointName( IpEndpointName::ANY_ADDRESS, listen_port ), this );
#ifdef TARGET_WIN32
	thread	= CreateThread(
							   NULL,              // default security attributes
							   0,                 // use default stack size
							&OscReceiver::startThread,        // thread function
							   (void*)(listen_socket),             // argument to thread function
							   0,                 // use default creation flags
							   NULL);             // we don't the the thread id

#else
	pthread_create( &thread, NULL, &OscReceiver::startThread, (void*)(listen_socket) );


#endif
}

OscReceiver::~OscReceiver()
{
//	delete listen_socket;
}


/*
#if defined TARGET_OSX
#error target-osx
#elif defined TARGET_WIN32
#error target-win32
#else
#error no target
#endif
 */

#ifdef TARGET_WIN32
DWORD WINAPI
#else
void*
#endif

		OscReceiver::startThread( void* _socket )
{
	UdpListeningReceiveSocket* socket = (UdpListeningReceiveSocket*)_socket;
	socket->Run();
    #ifdef TARGET_WIN32
    	return 0;
    #else
	return NULL;
    #endif
}

void OscReceiver::ProcessMessage( const osc::ReceivedMessage &m, const IpEndpointName& remoteEndpoint )
{
	// convert the message to an OscMessage
	OscMessage* ofMessage = new OscMessage();

	// set the address
	ofMessage->setAddress( m.AddressPattern() );

	// set the sender ip/host
	char endpoint_host[ IpEndpointName::ADDRESS_STRING_LENGTH ];
	remoteEndpoint.AddressAsString( endpoint_host );
    ofMessage->setRemoteEndpoint( endpoint_host, remoteEndpoint.port );

	// transfer the arguments
	for ( osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		  arg != m.ArgumentsEnd();
		  ++arg )
	{
		if ( arg->IsInt32() )
			ofMessage->addIntArg( arg->AsInt32Unchecked() );
		else if ( arg->IsFloat() )
			ofMessage->addFloatArg( arg->AsFloatUnchecked() );
		else if ( arg->IsString() )
			ofMessage->addStringArg( arg->AsStringUnchecked() );
		else
		{
			assert( false && "message argument is not int, float, or string" );
		}
	}

	// now add to the queue

	// at this point we are running inside the thread created by startThread,
	// so anyone who calls hasWaitingMessages() or getNextMessage() is coming
	// from a different thread

	// so we have to practise shared memory management

	// grab a lock on the queue
	grabMutex();

	// add incoming message on to the queue
	messages.push_back( ofMessage );

	// release the lock
	releaseMutex();
}

bool OscReceiver::hasWaitingMessages()
{
	// grab a lock on the queue
	grabMutex();

	// check the length of the queue
	int queue_length = (int)messages.size();

	// release the lock
	releaseMutex();

	// return whether we have any messages
	return queue_length > 0;
}

bool OscReceiver::getNextMessage( OscMessage* message )
{
	// grab a lock on the queue
	grabMutex();

	// check if there are any to be got
	if ( messages.size() == 0 )
	{
		// no: release the mutex
		releaseMutex();
		return false;
	}

	// copy the message from the queue to message
	OscMessage* src_message = messages.front();
	message->copy( *src_message );

	// now delete the src message
	delete src_message;
	// and remove it from the queue
	messages.pop_front();

	// release the lock on the queue
	releaseMutex();

	// return success
	return true;
}

void OscReceiver::grabMutex()
{
#ifdef TARGET_WIN32
	WaitForSingleObject( mutex, INFINITE );
#else
	pthread_mutex_lock( &mutex );
#endif
}

void OscReceiver::releaseMutex()
{
#ifdef TARGET_WIN32
	ReleaseMutex( mutex );
#else
	pthread_mutex_unlock( &mutex );
#endif
}
