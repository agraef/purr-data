/*
	oscpack -- Open Sound Control packet manipulation library
	http://www.audiomulch.com/~rossb/oscpack

	Copyright (c) 2004-2005 Ross Bencina <rossb@audiomulch.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "ip/UdpSocket.h"

#ifdef _WIN32
#include <winsock2.h>   // this must come first to prevent errors with MSVC7
#include <windows.h>
#include <mmsystem.h>   // for timeGetTime()
#else
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h> // for sockaddr_in
#include <errno.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define closesocket close

#endif

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <assert.h>
#include <signal.h>

#include "ip/NetworkingUtils.h"
#include "ip/PacketListener.h"
#include "ip/TimerListener.h"

#ifdef _WIN32
typedef int socklen_t;
#elif defined(__APPLE__) && !defined(_SOCKLEN_T)
// pre system 10.3 didn have socklen_t
typedef ssize_t socklen_t;
#endif



static void SockaddrFromIpEndpointName( struct sockaddr_in& sockAddr, const IpEndpointName& endpoint )
{
    memset( (char *)&sockAddr, 0, sizeof(sockAddr ) );
    sockAddr.sin_family = AF_INET;

	sockAddr.sin_addr.s_addr = 
		(endpoint.address == IpEndpointName::ANY_ADDRESS)
		? INADDR_ANY
		: htonl( endpoint.address );

	sockAddr.sin_port =
		(endpoint.port == IpEndpointName::ANY_PORT)
		? (short)0
		: htons( (short)endpoint.port );
}


static IpEndpointName IpEndpointNameFromSockaddr( const struct sockaddr_in& sockAddr )
{
	return IpEndpointName( 
		(sockAddr.sin_addr.s_addr == INADDR_ANY) 
			? IpEndpointName::ANY_ADDRESS 
			: ntohl( sockAddr.sin_addr.s_addr ),
		(sockAddr.sin_port == 0)
			? IpEndpointName::ANY_PORT
			: ntohs( sockAddr.sin_port )
		);
}


class UdpSocket::Implementation{
    NetworkInitializer networkInitializer_;

	bool isBound_;
	bool isConnected_;

	SOCKET socket_;
	struct sockaddr_in connectedAddr_;
	struct sockaddr_in sendToAddr_;

public:

	Implementation()
		: isBound_( false )
		, isConnected_( false )
		, socket_( INVALID_SOCKET )
	{
		if( (socket_ = socket( AF_INET, SOCK_DGRAM, 0 )) == INVALID_SOCKET ){
            throw std::runtime_error("unable to create udp socket\n");
        }

		memset( &sendToAddr_, 0, sizeof(sendToAddr_) );
        sendToAddr_.sin_family = AF_INET;
	}

	~Implementation()
	{
		if (socket_ != INVALID_SOCKET) closesocket(socket_);
	}

	IpEndpointName LocalEndpointFor( const IpEndpointName& remoteEndpoint ) const
	{
		assert( isBound_ );

		// first connect the socket to the remote server
        
        struct sockaddr_in connectSockAddr;
		SockaddrFromIpEndpointName( connectSockAddr, remoteEndpoint );
       
        if (connect(socket_, (struct sockaddr *)&connectSockAddr, sizeof(connectSockAddr)) < 0) {
            throw std::runtime_error("unable to connect udp socket\n");
        }

        // get the address

        struct sockaddr_in sockAddr;
        memset( (char *)&sockAddr, 0, sizeof(sockAddr ) );
        socklen_t length = sizeof(sockAddr);
        if (getsockname(socket_, (struct sockaddr *)&sockAddr, &length) < 0) {
            throw std::runtime_error("unable to getsockname\n");
        }
        
		if( isConnected_ ){
			// reconnect to the connected address
			
			if (connect(socket_, (struct sockaddr *)&connectedAddr_, sizeof(connectedAddr_)) < 0) {
				throw std::runtime_error("unable to connect udp socket\n");
			}

		}else{
			// unconnect from the remote address
		
			struct sockaddr_in unconnectSockAddr;
			SockaddrFromIpEndpointName( unconnectSockAddr, IpEndpointName() );

            int connectResult = connect(socket_, (struct sockaddr *)&unconnectSockAddr, sizeof(unconnectSockAddr));
            if ( connectResult < 0 &&
#ifdef _WIN32
			    WSAGetLastError() != WSAEADDRNOTAVAIL 
#else
                errno != EAFNOSUPPORT 
#endif
            ) {
				throw std::runtime_error("unable to un-connect udp socket\n");
			}
		}

		return IpEndpointNameFromSockaddr( sockAddr );
	}

	void Connect( const IpEndpointName& remoteEndpoint )
	{
		SockaddrFromIpEndpointName( connectedAddr_, remoteEndpoint );
       
        if (connect(socket_, (struct sockaddr *)&connectedAddr_, sizeof(connectedAddr_)) < 0) {
            throw std::runtime_error("unable to connect udp socket\n");
        }

		isConnected_ = true;
	}

	void Send( const char *data, int size )
	{
		assert( isConnected_ );

        send( socket_, data, size, 0 );
	}

    void SendTo( const IpEndpointName& remoteEndpoint, const char *data, int size )
	{
		sendToAddr_.sin_addr.s_addr = htonl( remoteEndpoint.address );
        sendToAddr_.sin_port = htons( (short)remoteEndpoint.port );

        sendto( socket_, data, size, 0, (sockaddr*)&sendToAddr_, sizeof(sendToAddr_) );
	}

	void Bind( const IpEndpointName& localEndpoint )
	{
		struct sockaddr_in bindSockAddr;
		SockaddrFromIpEndpointName( bindSockAddr, localEndpoint );

        if (bind(socket_, (struct sockaddr *)&bindSockAddr, sizeof(bindSockAddr)) < 0) {
            throw std::runtime_error("unable to bind udp socket\n");
        }

		isBound_ = true;
	}

	bool IsBound() const { return isBound_; }

    int ReceiveFrom( IpEndpointName& remoteEndpoint, char *data, int size )
	{
		assert( isBound_ );

        /////////////////////////////////////////////
        // grrrr.org - modified for non-blockage
    	fd_set readfds;
    	FD_ZERO(&readfds);
		FD_SET(socket_,&readfds);
		timeval tv; 
		tv.tv_sec = tv.tv_usec = 0; // don't block
#ifdef _WIN32
		int nsocks = 1;
#else
		int nsocks = socket_+1;
#endif
		int result = select(nsocks,&readfds,NULL,NULL,&tv);
        if(result <= 0)
            return 0;
        /////////////////////////////////////////////

		struct sockaddr_in fromAddr;
        socklen_t fromAddrLen = sizeof(fromAddr);

        result = recvfrom(socket_, data, size, 0,
                    (struct sockaddr *) &fromAddr, (socklen_t*)&fromAddrLen);
		if( result < 0 )
			return 0;

		remoteEndpoint.address = ntohl(fromAddr.sin_addr.s_addr);
		remoteEndpoint.port = ntohs(fromAddr.sin_port);

		return result;
	}

	SOCKET& Socket() { return socket_; }
};

UdpSocket::UdpSocket()
{
	impl_ = new Implementation();
}

UdpSocket::~UdpSocket()
{
	delete impl_;
}

IpEndpointName UdpSocket::LocalEndpointFor( const IpEndpointName& remoteEndpoint ) const
{
	return impl_->LocalEndpointFor( remoteEndpoint );
}

void UdpSocket::Connect( const IpEndpointName& remoteEndpoint )
{
	impl_->Connect( remoteEndpoint );
}

void UdpSocket::Send( const char *data, int size )
{
	impl_->Send( data, size );
}

void UdpSocket::SendTo( const IpEndpointName& remoteEndpoint, const char *data, int size )
{
	impl_->SendTo( remoteEndpoint, data, size );
}

void UdpSocket::Bind( const IpEndpointName& localEndpoint )
{
	impl_->Bind( localEndpoint );
}

bool UdpSocket::IsBound() const
{
	return impl_->IsBound();
}

int UdpSocket::ReceiveFrom( IpEndpointName& remoteEndpoint, char *data, int size )
{
	return impl_->ReceiveFrom( remoteEndpoint, data, size );
}

