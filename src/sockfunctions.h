#ifdef _WIN32
//#include <WS2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")

#include <Winsock2.h>
#include <ws2tcpip.h>
#include<WinError.h>
#include <windows.h>

LPSTR WSAstrerror(int errCode);

#define errno WSAGetLastError()

#define strerror(e) WSAstrerror(e)

//#pragma comment(lib, "User32.lib")
// Need to link with Ws2_32.lib
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#endif
#include <sys/types.h>
//#include <stdlib.h>


#ifndef INADDR_NONE
#define INADDR_NONE	0xffffffff
#endif /* INADDR_NONE */
int	__cdecl errexit(const char *format, ...);
#ifdef _WIN32
SOCKET __cdecl
#else
int
#endif
passivesock(int portnumber,  char *transport,  int qlen);