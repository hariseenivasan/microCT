/* passivesock.c - passivesock. Reused some of the source code from Comer's V1 3e textbook*/
/***** Author: Vidula Palod, Meghana K Siddappa, Anshu Verma *****/

#include "sockfunctions.h"

unsigned short	portbase = 0;	/* port base, for non-root servers	*/

/*------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP or UDP
 *------------------------------------------------------------------------
 */
#ifdef _WIN32
SOCKET __cdecl
#else
int
#endif
passivesock(int portnumber,  char *transport,  int qlen)
/*
 * Arguments:
 *      portnumber   - desired port number
 *      transport - transport protocol to use ("tcp" or "udp")
 *      qlen      - maximum server request queue length
 */
{
	struct hostent 	*phe= (struct hostent*)malloc(sizeof(struct hostent));	/* pointer to host information entry 	*/
	struct servent	*pse= (struct servent*)malloc(sizeof(struct servent));	/* pointer to service information entry	*/
	struct protoent *ppe= (struct protoent*)malloc(sizeof(struct protoent));	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	 type;	/* socket descriptor and socket type	*/
#ifdef _WIN32
	SOCKET s;
	  WORD wVersionRequested;
    WSADATA wsaData;
    int err;

/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

/* Confirm that the WinSock DLL supports 2.2.*/
/* Note that if the DLL supports versions greater    */
/* than 2.2 in addition to 2.2, it will still return */
/* 2.2 in wVersion since that is the version we      */
/* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return 1;
    }
    else
        printf("The Winsock 2.2 dll was found okay\n");
        

/* The Winsock DLL is acceptable. Proceed to use it. */

/* Add network programming using Winsock here */

/* then call WSACleanup when done using the Winsock dll */
    
#else
	int s;
#endif
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	/* Map port number */
	//sin.sin_port = htons(ntohs((unsigned short)portnumber)
	sin.sin_port = htons((unsigned short)portnumber);

	/* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(transport)) == 0){
		printf("%s",strerror(errno));
		errexit("can't get \"%s\" protocol entry\n ", transport);
	}

 	/* Use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else 
		type = SOCK_STREAM;

 	/* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s < 0) 
		errexit("can't create socket: %s\n", strerror(errno));

 	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		printf("%s",strerror(errno));
		errexit("can't bind to : %s", strerror(errno));
	}
	/* listen to the client requests */
	if (type == SOCK_STREAM && listen(s, qlen) < 0)
		errexit("can't listen on port: %s\n", strerror(errno));
	
	return s;
}
