/*
 *  ofxNetworkUtils.h
 *  
 *	part of code from : http://www.developerweb.net/forum/showthread.php?t=5085
 *  Created by Guillaume Stagnaro <http://www.010175.net> on 20/02/10.
 *	With modifications from Pierre Rossel <http://head.hesge.ch/>
 */

//#include "ofMain.h"

#include <string>
#include <vector>

#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
# include <ifaddrs.h>

#include <net/if.h>

using namespace std;
class networkInterface
{
public:
	networkInterface();
	networkInterface(string _name, string _address, string _subnetMask, string _broadcastAddress);
	~networkInterface();
	
	string name; // interface name (eg: en0)
	string address; // interface IP adress
	string subnetMask; // interface subnet mask
	string broadcastAddress; // interface broadcast adress
		
};


class ofxNetworkUtils
{

public:
	ofxNetworkUtils();
	~ofxNetworkUtils();
	
	/*
	  filterFlags: only get interfaces with these flags set
	  excludeFlags: don't get interfaces with these flags set
	*/
	void	init(unsigned int filterFlags = 0xFFFF, unsigned int excludeFlags = IFF_LOOPBACK);
	
	// return network interfaces count
	int		getInterfacesCount();
	
	string	getInterfaceName(int interfaceId);
	string	getInterfaceAddress(int interfaceId);
	string	getInterfaceSubnetMask(int interfaceId);
	string	getInterfaceBroadcastAddress(int interfaceId);
	
private:
	
	int		SockAddrToUint(struct sockaddr * a);
	void	Inet_NtoA(int addr, char * ipbuf);
	
	std::vector<networkInterface*> networkInterfaces;
};

