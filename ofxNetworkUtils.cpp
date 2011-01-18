/*
 *  ofxNetworkUtils.cpp
 *  
 *	part of code from : http://www.developerweb.net/forum/showthread.php?t=5085
 *  Created by Guillaume Stagnaro <http://www.010175.net> on 20/02/10.
 *	With modifications from Pierre Rossel <http://head.hesge.ch/>
 */


#include "ofxNetworkUtils.h"

//--------------------------------------
// networkInterface Class
networkInterface::networkInterface(){
	networkInterface("nul", "0.0.0.0", "0.0.0.0", "0.0.0.0"); // dummy constructor call
}

networkInterface::networkInterface(string _name, string _address, string _subnetMask, string _broadcastAddress){
	name = _name;
	address = _address;
	subnetMask = _subnetMask;
	broadcastAddress = _broadcastAddress;
}

networkInterface::~networkInterface(){
	
}

//--------------------------------------
// ofxNetworkUtils Class
ofxNetworkUtils::ofxNetworkUtils(){
	
}

ofxNetworkUtils::~ofxNetworkUtils(){
	networkInterfaces.clear();
}


void ofxNetworkUtils::init(unsigned int filterFlags, unsigned int excludeFlags){
	networkInterfaces.clear(); // 
	struct ifaddrs * ifap;
	if (getifaddrs(&ifap) == 0)
	{
		struct ifaddrs * p = ifap;
		while(p)
		{
			int ifaAddr  = SockAddrToUint(p->ifa_addr);
			int maskAddr = SockAddrToUint(p->ifa_netmask);
			int dstAddr  = SockAddrToUint(p->ifa_dstaddr);
			if (ifaAddr != 0 && (p->ifa_flags & filterFlags) && (p->ifa_flags & excludeFlags) == 0)
			{
				
				networkInterface aNetworkInterface;
				
				char ifaAddrStr[32];  Inet_NtoA(ifaAddr,  ifaAddrStr);
				char maskAddrStr[32]; Inet_NtoA(maskAddr, maskAddrStr);
				char dstAddrStr[32];  Inet_NtoA(dstAddr,  dstAddrStr);
				
				char logMsg[512];
				sprintf(logMsg, "Found interface: name=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", p->ifa_name, ifaAddrStr, maskAddrStr, dstAddrStr);
				
				//ofLog(OF_LOG_NOTICE, logMsg);
				networkInterfaces.push_back( new networkInterface(p->ifa_name, ifaAddrStr, maskAddrStr, dstAddrStr));
				
			}
			p = p->ifa_next;
		}
		freeifaddrs(ifap);
	}
	
}

int ofxNetworkUtils::getInterfacesCount(){
	return networkInterfaces.size();
	
}

string ofxNetworkUtils::getInterfaceName(int interfaceId){
	if ((interfaceId<0)||(interfaceId>=networkInterfaces.size())) return "nul";
	return networkInterfaces[interfaceId]->name;
}

string ofxNetworkUtils::getInterfaceAddress(int interfaceId){
	if ((interfaceId<0)||(interfaceId>=networkInterfaces.size())) return "nul";
	return networkInterfaces[interfaceId]->address;
}

string ofxNetworkUtils::getInterfaceSubnetMask(int interfaceId){
	if ((interfaceId<0)||(interfaceId>=networkInterfaces.size())) return "nul";
	return networkInterfaces[interfaceId]->subnetMask;
}

string ofxNetworkUtils::getInterfaceBroadcastAddress(int interfaceId){
	if ((interfaceId<0)||(interfaceId>=networkInterfaces.size())) return "nul";
	return networkInterfaces[interfaceId]->broadcastAddress;
}

// return IPv4 numerique IP adress from ifaddrs 
int ofxNetworkUtils::SockAddrToUint(struct sockaddr * a)
{
	return ((a)&&(a->sa_family == AF_INET)) ? ntohl(((struct sockaddr_in *)a)->sin_addr.s_addr) : 0;

}

// convert an IPv4 numeric IP address into its string representation
void ofxNetworkUtils::Inet_NtoA(int addr, char * ipbuf)
{
	sprintf(ipbuf, "%i.%i.%i.%i", (addr>>24)&0xFF, (addr>>16)&0xFF, (addr>>8)&0xFF, (addr>>0)&0xFF);
}