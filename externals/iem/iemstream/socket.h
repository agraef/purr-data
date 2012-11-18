#ifndef __SOCKET_H
#define __SOCKET_H

#ifdef _WIN32
	#include <io.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>

	#define closesocket close
	#define SOCKET int
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
#endif

#endif
