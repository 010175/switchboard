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

#include "OscMessage.h"
#include <iostream>
#include <assert.h>

OscMessage::OscMessage()

{
}

OscMessage::~OscMessage()
{
	clear();
}

void OscMessage::clear()
{
	for ( unsigned int i=0; i<args.size(); ++i )
		delete args[i];
	args.clear();
	address = "";
}

/*

get methods

*/

int OscMessage::getNumArgs() const
{
	return (int)args.size();
}

OscArgType OscMessage::getArgType( int index ) const
{
    if ( index >= (int)args.size() )
    {
        fprintf(stderr,"OscMessage::getArgType: index %d out of bounds\n", index );
        return OSC_TYPE_INDEXOUTOFBOUNDS;
    }
    else
        return args[index]->getType();
}

string OscMessage::getArgTypeName( int index ) const
{
    if ( index >= (int)args.size() )
    {
        fprintf(stderr,"OscMessage::getArgTypeName: index %d out of bounds\n", index );
        return "INDEX OUT OF BOUNDS";
    }
    else
        return args[index]->getTypeName();
}


int32_t OscMessage::getArgAsInt32( int index ) const
{
	if ( getArgType(index) != OSC_TYPE_INT32 )
	{
	    if ( getArgType( index ) == OSC_TYPE_FLOAT )
        {
            fprintf(stderr, "OscMessage:getArgAsInt32: warning: converting int32 to float for argument %i\n", index );
            return ((OscArgFloat*)args[index])->get();
        }
        else
        {
            fprintf(stderr, "OscMessage:getArgAsInt32: error: argument %i is not a number\n", index );
            return 0;
        }
	}
	else
        return ((OscArgInt32*)args[index])->get();
}


float OscMessage::getArgAsFloat( int index ) const
{
	if ( getArgType(index) != OSC_TYPE_FLOAT )
	{
	    if ( getArgType( index ) == OSC_TYPE_INT32 )
        {
            fprintf(stderr, "OscMessage:getArgAsFloat: warning: converting float to int32 for argument %i\n", index );
            return ((OscArgInt32*)args[index])->get();
        }
        else
        {
            fprintf(stderr, "OscMessage:getArgAsFloat: error: argument %i is not a number\n", index );
            return 0;
        }
	}
	else
        return ((OscArgFloat*)args[index])->get();
}


string OscMessage::getArgAsString( int index ) const
{
    if ( getArgType(index) != OSC_TYPE_STRING )
	{
	    if ( getArgType( index ) == OSC_TYPE_FLOAT )
        {
            char buf[1024];
            sprintf(buf,"%f",((OscArgFloat*)args[index])->get() );
            fprintf(stderr, "OscMessage:getArgAsString: warning: converting float to string for argument %i\n", index );
            return buf;
        }
	    else if ( getArgType( index ) == OSC_TYPE_INT32 )
        {
            char buf[1024];
            sprintf(buf,"%i",((OscArgInt32*)args[index])->get() );
            fprintf(stderr, "OscMessage:getArgAsString: warning: converting int32 to string for argument %i\n", index );
            return buf;
        }
        else
        {
            fprintf(stderr, "OscMessage:getArgAsString: error: argument %i is not a string\n", index );
            return "";
        }
	}
	else
        return ((OscArgString*)args[index])->get();
}



/*

set methods

*/


void OscMessage::addIntArg( int32_t argument )
{

	args.push_back( new OscArgInt32( argument ) );
}

void OscMessage::addFloatArg( float argument )
{
	args.push_back( new OscArgFloat( argument ) );
}

void OscMessage::addStringArg( string argument )
{
	args.push_back( new OscArgString( argument ) );
}


/*

 housekeeping

 */

OscMessage& OscMessage::copy( const OscMessage& other )
{
	// copy address
	address = other.address;

	remote_host = other.remote_host;
	remote_port = other.remote_port;

	// copy arguments
	for ( int i=0; i<(int)other.args.size(); ++i )
	{
		OscArgType argType = other.getArgType( i );
		if ( argType == OSC_TYPE_INT32 )
			args.push_back( new OscArgInt32( other.getArgAsInt32( i ) ) );
		else if ( argType == OSC_TYPE_FLOAT )
			args.push_back( new OscArgFloat( other.getArgAsFloat( i ) ) );
		else if ( argType == OSC_TYPE_STRING )
			args.push_back( new OscArgString( other.getArgAsString( i ) ) );
		else
		{
			assert( false && "bad argument type" );
		}
	}

	return *this;
}
