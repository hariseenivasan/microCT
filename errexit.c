/* errexit.c - errexit */
/***** Author: Vidula Palod, Meghana K Siddappa, Anshu Verma *****/

#include"sockfunctions.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32

 LPSTR WSAstrerror(int errCode) {
	LPSTR errString=NULL;
int size = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,0,  errCode, 0,(LPSTR)&errString,0, 0 );


return errString;}
#endif
/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int
errexit(const char *format, ...)
{
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}
