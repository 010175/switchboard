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

#ifndef _OscBUNDLE_H
#define _OscBUNDLE_H

#include <vector>
#include "OscMessage.h"

class OscBundle
{
public:	
	OscBundle();
	~OscBundle();
	OscBundle( const OscBundle& other ) { copy ( other ); }
	OscBundle& operator= ( const OscBundle& other ) { return copy( other ); }
	/// for operator= and copy constructor
	OscBundle& copy( const OscBundle& other );
	
	/// erase contents
	void clear() { messages.clear(); bundles.clear(); }

	/// add bundle elements
	void addBundle( const OscBundle& element );
	void addMessage( const OscMessage& message );
	
	/// get bundle elements
	int getBundleCount() const { return bundles.size(); }
	int getMessageCount() const { return messages.size(); }
	/// return the bundle or message at the given index
	OscBundle& getBundleAt( int i ) { return bundles[i]; }
	OscMessage& getMessageAt( int i ) { return messages[i]; }
	
private:
		
	std::vector< OscMessage > messages;
	std::vector< OscBundle > bundles;
};


#endif

