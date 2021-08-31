/*
 * sconsole.h - portable TCP socket serial port emulator
 *
 * Copyright (c) Michal Tomek 2018-2019 <mtdev79b@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define TCPIP_error WSAGetLastError()
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SOCKET int
#define closesocket close
#define TCPIP_error errno
#define SD_BOTH SHUT_RDWR
#define ioctlsocket ioctl
#endif

// MAX_SOCKET_PORTS and BASE_PORT needs to be defined
SOCKET listen_sockets[MAX_SOCKET_PORTS];
SOCKET client_sockets[MAX_SOCKET_PORTS];

int init_TCPIP() {
	int i;
	for (i=0;i<MAX_SOCKET_PORTS;i++) {
		client_sockets[i] = INVALID_SOCKET;
		listen_sockets[i] = INVALID_SOCKET;
	}
#ifdef _WIN32
	int e;
	static WSADATA wsaData;
	if ((e = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0) {
		printf("Serial: WSAStartup err %d\n", e);
		return -1;
	}
#endif
	return 0;
}

void shutdown_TCPIP() {
#ifdef _WIN32
	WSACleanup();
#endif
}

int init_socket_port(int port) {

	struct addrinfo *res = NULL;
	struct addrinfo h;
	char port_str[6];
	int e;

	memset(&h, 0, sizeof(h));
	h.ai_family = AF_INET;
	h.ai_socktype = SOCK_STREAM;
	h.ai_protocol = IPPROTO_TCP;
	h.ai_flags = AI_PASSIVE;

	sprintf(port_str,"%d",BASE_PORT+port);
	if ( (e = getaddrinfo(NULL, port_str, &h, &res)) != 0 ) {
		printf("Serial: getaddrinfo err %d\n", e);
		return -1;
	}
	listen_sockets[port] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listen_sockets[port] == INVALID_SOCKET) {
		printf("Serial: socket err %d\n", TCPIP_error);
		freeaddrinfo(res);
		return -1;
	}
#ifndef _WIN32
	e = 1; // enable socket reuse
	setsockopt(listen_sockets[port], SOL_SOCKET, SO_REUSEADDR, &e, sizeof(int));
#endif
	if (bind( listen_sockets[port], res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
		printf("Serial: bind err %d\n", TCPIP_error);
		freeaddrinfo(res);
		closesocket(listen_sockets[port]);
		return -1;
	}
	freeaddrinfo(res);

	if (listen(listen_sockets[port], SOMAXCONN) == SOCKET_ERROR) {
		printf("Serial: listen err %d\n", TCPIP_error);
		closesocket(listen_sockets[port]);
		return -1;
	}

	printf("Serial port %d listening on %s\n", port, port_str);
	return 0;
}

int open_socket_port(int port) {
	
	if (client_sockets[port] != INVALID_SOCKET) {
	   printf("Serial port %d connection lost\n", port);
	   closesocket(client_sockets[port]);
	}

	client_sockets[port] = INVALID_SOCKET;
	client_sockets[port] = accept(listen_sockets[port], NULL, NULL);
	if (listen_sockets[port]!= INVALID_SOCKET ) {  // don't complain when shutting down
		if (client_sockets[port] == INVALID_SOCKET) {
			printf("Serial: accept err %d\n", TCPIP_error);
			//closesocket(listen_sockets[port]);
			return -1;
		}
		unsigned long mode = 1;
		ioctlsocket(client_sockets[port], FIONBIO, &mode); // nonblocking
		printf("Serial port %d connected\n",port);
	}
	return 0;
}

void shutdown_socket_ports() {

	int i;
	for (i=0;i<MAX_SOCKET_PORTS;i++) 
	{
		if (client_sockets[i] != INVALID_SOCKET) {
			shutdown(client_sockets[i], SD_BOTH);
			closesocket(client_sockets[i]);
			client_sockets[i] = INVALID_SOCKET;
		}
		if (listen_sockets[i] != INVALID_SOCKET) {
			shutdown(listen_sockets[i], SD_BOTH);
			closesocket(listen_sockets[i]);
			listen_sockets[i] = INVALID_SOCKET;
		}
	}
	shutdown_TCPIP();
}

int char_available_socket_port(int port) {
	  unsigned long iMode = 0;
	  ioctlsocket(client_sockets[port], FIONREAD, &iMode);
	  return iMode!=0;
}

int is_connected_socket_port(int port) {
	char buf;
	  return client_sockets[port] != INVALID_SOCKET && recv(client_sockets[port], &buf, 1, MSG_PEEK)!=0;
}

void tx_socket_port(int port, uint8_t data) {
	send( client_sockets[port], (char*)&data, 1, 0 );
}

int rx_socket_port(int port) {
	int data = 0;
	recv( client_sockets[port], (char*)&data, 1, 0);
	return data;
}
