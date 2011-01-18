/*

 Copyright 2007, 2008 Damian Stewart damian@frey.co.nz
 Distributed under the terms of the GNU Lesser General Public License v3

 This file is part of the osc openFrameworks OSC addon.

 osc is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 osc is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with osc.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OSCARG_H
#define OSCARG_H


#include <string>

typedef enum oscArgType
{
	OSC_TYPE_NONE,
	OSC_TYPE_INT32,
	OSC_TYPE_FLOAT,
	OSC_TYPE_STRING,
	OSC_TYPE_BLOB,
	OSC_TYPE_BUNDLE,
	OSC_TYPE_INDEXOUTOFBOUNDS
} OscArgType;

/*

oscArg

base class for arguments

*/
using namespace std;

class OscArg
{
public:
	OscArg() {};
	virtual ~OscArg() {};

	virtual OscArgType getType() { return OSC_TYPE_NONE; }
	virtual string getTypeName() { return "none"; }

private:
};


/*

subclasses for each possible argument type

*/

#if defined TARGET_WIN32 && defined _MSC_VER
// required because MSVC isn't ANSI-C compliant
typedef long int32_t;
#endif

class OscArgInt32 : public OscArg
{
public:
	OscArgInt32( int32_t _value ) { value = _value; }
	~OscArgInt32() {};

	/// return the type of this argument
	OscArgType getType() { return OSC_TYPE_INT32; }
	string getTypeName() { return "int32"; }

	/// return value
	int32_t get() const { return value; }
	/// set value
	void set( int32_t _value ) { value = _value; }

private:
	int32_t value;
};

class OscArgFloat : public OscArg
{
public:
	OscArgFloat( float _value ) { value = _value; }
	~OscArgFloat() {};

	/// return the type of this argument
	OscArgType getType() { return OSC_TYPE_FLOAT; }
	string getTypeName() { return "float"; }

	/// return value
	float get() const { return value; }
	/// set value
	void set( float _value ) { value = _value; }

private:
		float value;
};

class OscArgString : public OscArg
{
public:
	OscArgString( string _value ) { value = _value; }
	~OscArgString() {};

	/// return the type of this argument
	OscArgType getType() { return OSC_TYPE_STRING; }
	string getTypeName() { return "string"; }

	/// return value
	string get() const { return value; }
	/// set value
	void set( const char* _value ) { value = _value; }

private:
	std::string value;
};

#endif
