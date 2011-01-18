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


#ifndef OscSENDER_H
#define OscSENDER_H

/**

OscSender

an OscSender sends messages to a single host/port

*/

class UdpTransmitSocket;
#include <string>
#include "OscTypes.h"
#include "OscOutboundPacketStream.h"

#include "OscBundle.h"
#include "OscMessage.h"


class OscSender
{
public:
	OscSender();
	~OscSender();

	/// send messages to hostname and port
	void setup( std::string hostname, int port );

	/// send the given message
	void sendMessage( OscMessage& message );
	/// send the given bundle
	void sendBundle( OscBundle& bundle );

private:
		
	// helper methods for constructing messages
	void appendBundle( OscBundle& bundle, osc::OutboundPacketStream& p );
	void appendMessage( OscMessage& message, osc::OutboundPacketStream& p );

	UdpTransmitSocket* socket;
};

#endif
