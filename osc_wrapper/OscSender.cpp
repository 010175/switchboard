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


#include "OscSender.h"

#include "UdpSocket.h"

#include <assert.h>

OscSender::OscSender()
{
}

OscSender::~OscSender()
{
}

void OscSender::setup( std::string hostname, int port )
{
	socket = new UdpTransmitSocket( IpEndpointName( hostname.c_str(), port ) );
}

void OscSender::sendBundle( OscBundle& bundle )
{
	static const int OUTPUT_BUFFER_SIZE = 32768;
	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE );

	// serialise the bundle
	appendBundle( bundle, p );

	socket->Send( p.Data(), p.Size() );
}

void OscSender::sendMessage( OscMessage& message )
{
	static const int OUTPUT_BUFFER_SIZE = 16384;
	char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

	// serialise the message
	p << osc::BeginBundleImmediate;
	appendMessage( message, p );
	p << osc::EndBundle;

	socket->Send( p.Data(), p.Size() );
}

void OscSender::appendBundle( OscBundle& bundle, osc::OutboundPacketStream& p )
{
	// recursively serialise the bundle
	p << osc::BeginBundleImmediate;
	for ( int i=0; i<bundle.getBundleCount(); i++ )
	{
		appendBundle( bundle.getBundleAt( i ), p );
	}
	for ( int i=0; i<bundle.getMessageCount(); i++ )
	{
		appendMessage( bundle.getMessageAt( i ), p );
	}
	p << osc::EndBundle;
}

void OscSender::appendMessage( OscMessage& message, osc::OutboundPacketStream& p )
{
    p << osc::BeginMessage( message.getAddress().c_str() );
	for ( int i=0; i< message.getNumArgs(); ++i )
	{
		if ( message.getArgType(i) == OSC_TYPE_INT32 )
			p << message.getArgAsInt32( i );
		else if ( message.getArgType( i ) == OSC_TYPE_FLOAT )
			p << message.getArgAsFloat( i );
		else if ( message.getArgType( i ) == OSC_TYPE_STRING )
			p << message.getArgAsString( i ).c_str();
		else
		{
			assert( false && "bad argument type" );
		}
	}
	p << osc::EndMessage;
}
