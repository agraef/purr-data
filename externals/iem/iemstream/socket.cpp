/*************************************************************
 *
 *    streaming external for PD
 *
 * File: socket.cpp
 *
 * Description: Implementation of socket functions
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/

#include "stream.h"

// explicit definition of PD report functions
extern "C" {
	extern void	post(char *fmt, ...);
	extern void error(char *fmt, ...);
}

/* Checks if data is waiting on the socket */
static bool CheckForData(SOCKET sock,int wait = 20)
{
	struct timeval tv;
	tv.tv_sec = wait/1000;
	tv.tv_usec = (wait%1000)*1000;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock,&set);

	// return true when data is waiting
	return ::select((int)sock+1, &set, NULL, NULL, &tv) > 0;
}

SOCKET Stream::Connect(const char *hostname,const char *mountpoint,int portno)
{
	const int STRBUF_SIZE = 1024;
	char request[STRBUF_SIZE]; // string to be sent to server
	sockaddr_in server;
  
	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd == SOCKET_ERROR) {
		throw "Error opening socket";
	}
  
	// get IP address
	hostent *hp = gethostbyname(hostname);
	if(!hp) {
		closesocket(sockfd);
		throw "Could not get IP address of hostname";
	}

	server.sin_family = AF_INET;
	memcpy(&server.sin_addr,hp->h_addr, hp->h_length);
	server.sin_port = htons((unsigned short)portno);
	  
	// try to connect
	if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) == SOCKET_ERROR) {
		closesocket(sockfd);
		throw "Connection failed!";
	}
  
	// check if we can read from the socket
	{
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(sockfd,&fdset);

		timeval  tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 500;
		  
		int ret = select((int)sockfd + 1, &fdset, NULL, NULL, &tv);
		if(ret < 0) {
			closesocket(sockfd);
			throw "Can not read from socket";
		}
	}
	  
	// build up stuff we need to send to server
	sprintf(request, "GET /%s HTTP/1.0 \r\nHost: %s\r\nUser-Agent:   0.2\r\nAccept: audio/x-ogg\r\n\r\n", mountpoint, hostname);

	// try to contact server
	if(send(sockfd, request, (int)strlen(request), 0) == SOCKET_ERROR) {
		closesocket(sockfd);
		throw "Could not contact server";
	}
  
	// read first line of response
	int i = 0;
	while(i < STRBUF_SIZE-1) {
		if( CheckForData(sockfd) ) {
			if(recv(sockfd, request+i, 1, 0) <= 0) {
				closesocket(sockfd);
				throw "Could not read from socket, quitting";
			}

			if(request[i] == '\n') break;
			if(request[i] != '\r') i++;
		}
	}
	request[i] = '\0';
  
	bool eof = false;

	// parse content of the response...
	if(strstr(request, "HTTP/1.0 200 OK"))    /* server is ready */
    {
		//post(" : IceCast2 server detected");

		while(!eof) {
			i = 0;
			while(i < STRBUF_SIZE-1) {
				if( CheckForData(sockfd) ) {
					if(recv(sockfd, request + i, 1, 0) <= 0) {
						closesocket(sockfd);
						throw "Could not read from socket, quitting";
					}

					if(request[i] == '\n')  /* leave at end of line */
						break;
					if(request[i] == 0x0A)  /* leave at end of line */
						break;
					if(request[i] != '\r')	/* go on until 'return' */
						i++;
				}
			}

			// make it a null terminated string
			request[i] = '\0';	
		  
			char *sptr;
			if( ( sptr = strstr(request, "application/x-ogg") ) ) {		
				/* check for content type */
				//post(" : Ogg Vorbis stream found");
			}

			if( ( sptr = strstr(request, "ice-name:") ) ) {		
				/* display ice-name */
				//post(" : \"%s\"", sptr + 10);
			}

			if(i == 0) 
				// we got last '\r\n' from server
				eof = true;	
		}
	}
	else if(strstr(request, "HTTP/1.0 404")) {
		// file not found
		closesocket(sockfd);
		throw "File not found";
	}
	else {
		// wrong server or wrong answer
		closesocket(sockfd);
		throw request;
    }

	//post(" : connected to http://%s:%d/%s", hp->h_name, portno, mountpoint);
  
	return sockfd;
}


void Stream::Disconnect(SOCKET fd)
{
	if(fd != INVALID_SOCKET) {
		closesocket(fd);
		post("connection closed");
	}
}

int Stream::Read(SOCKET fd,char *buf,int size,int timeout)
{
	// check for data availability so that recv doesn't block
	if(CheckForData(fd,timeout)) {
		int ret = recv(fd,buf,size, 0);
		if(ret == SOCKET_ERROR) {
			post("socket error!");
			return -1;
		}
		else
			return ret;
	}
	else
		return -1;
}
