/*
 * generic.h -- allows portable use of the following functions:
 *
 *	dup2(cur,new)		dups fd cur into fd new
 *	killpg(pg,sig)		sends sig to process group pg
 *	signal(s,f)		set function f to catch signal s -- return old f
 *	freesignal(s)		unblock signal s
 *	memcpy(b1,b2,n)		copy n bytes from b2 to b1 -- return b1
 *	memzero(b,n)		zero n bytes starting at b -- return b
 *	strchr(s,c)		return ptr to c in s
 *	strrchr(s,c)		return ptr to last c in s
 *	vfork()			data share version of fork()
 */

union				/* handles side effects in macros */
{
	char	*ustring;
} __GeN__;

#ifndef a64_lnx
# define a64_lnx
#endif
#define DMAKEPP "/lib/cpp"
# define DOUBLEBYTE
# define INGRESII
#define LSTAT_PROVIDED
#define INDEX_PROVIDED
#define BCOPY_PROVIDED
#define MKDIR_PROVIDED
#include <memory.h>
#include <string.h>
#include <strings.h>
#define memzero(b,n) memset(b,'\\0',n)
#define freesignal(s) sigsetmask(sigblock(0)&~(1<<(s-1)))
#define TYPESIG void
# define PRINTF_PERCENT_P
