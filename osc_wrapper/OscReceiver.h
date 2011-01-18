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


#ifndef OscRECEIVER_H
#define OscRECEIVER_H

#include <deque>

#ifdef TARGET_WIN32
// threads
#include <windows.h>
#else
// threads
#include <pthread.h>
#endif

// osc
#include "OscTypes.h"
#include "OscPacketListener.h"
#include "UdpSocket.h"

// Osc
#include "OscMessage.h"

class OscReceiver : public osc::OscPacketListener
{
public:
	OscReceiver();
	~OscReceiver();

	/// listen_port is the port to listen for messages on
	void setup( int listen_port );

	/// returns true if there are any messages waiting for collection
	bool hasWaitingMessages();
	/// take the next message on the queue of received messages, copy its details into message, and
	/// remove it from the queue. return false if there are no more messages to be got, otherwise
	/// return true
	bool getNextMessage( OscMessage* );

protected:
	/// process an incoming osc message and add it to the queue
	virtual void ProcessMessage( const osc::ReceivedMessage &m, const IpEndpointName& remoteEndpoint );

private:

	// start the listening thread
#ifdef TARGET_WIN32
	static DWORD WINAPI startThread( void* listen_socket );
#else
	static void* startThread( void* listen_socket );
#endif
	// queue of osc messages
	std::deque< OscMessage* > messages;

	// socket to listen on
	UdpListeningReceiveSocket* listen_socket;

	// mutex helpers
	void grabMutex();
	void releaseMutex();

#ifdef TARGET_WIN32
	// thread to listen with
	HANDLE thread;
	// mutex for the thread queue
	HANDLE mutex;
#else
	// thread to listen with
	pthread_t thread;
	// mutex for the message queue
	pthread_mutex_t mutex;
#endif
};

#endif
