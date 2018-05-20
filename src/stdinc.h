/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#ifndef STDINC_H
#define STDINC_H

#ifndef XENIX
#ifdef M_V7
#define XENIX
#else
#ifdef M_SYS3
#define XENIX
#else
#ifdef M_SYS5
#define XENIX
#endif
#endif /* M_SYS3 */
#endif /* M_V7 */
#endif /* XENIX */

#ifdef applec
#define NON_ANSI  /* Define to skip vt100 codes in output      */
#endif

#ifdef MSDOS
#include <io.h>
#include <stdlib.h>
#include <process.h>
#ifndef ANSI_CODES
#define NON_ANSI  /* Define to skip vt100 codes in output */
#endif
#endif /* MSDOS */

#ifdef COHERENT
#define SYS_FCNTL
#endif

#ifdef SYS_FCNTL
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>    /* May need to change to <strings.h> on some systems */
#include <time.h>      /* May need to change to whatever on other systems   */
#include <sys/stat.h>

#ifdef NeXT
#include <dir.h>
#endif

#ifdef COHERENT   /* Another great thing--isdigit() under COHERENT doesn't */
#ifdef isdigit    /* work right: it keeps returning 8.  Odd.  Oh well.  :) */
#undef isdigit
#endif
#define isdigit(x) ((x) >= '0' && (x) <= '9')
#endif /* COHERENT */

#ifdef MSDOS
#define SNGCR    "\r\n"
#define DUBCR    "\r\n\r\n"
#define NUMCR    2
#define DIRSEP   '\\'
#else
#define SNGCR    "\n"
#define DUBCR    "\n\n"
#define NUMCR    1
#define DIRSEP   '/'
#endif

#ifdef ESC
#undef ESC
#endif

#define CTRL_A (char)1
#define CTRL_B (char)2
#define CTRL_C (char)3
#define CTRL_D (char)4
#define CTRL_E (char)5
#define CTRL_F (char)6
#define CTRL_G (char)7
#define CTRL_H (char)8
#define CTRL_I (char)9
#define CTRL_J (char)10
#define CTRL_K (char)11
#define CTRL_L (char)12
#define CTRL_M (char)13
#define CTRL_N (char)14
#define CTRL_O (char)15
#define CTRL_P (char)16
#define CTRL_Q (char)17
#define CTRL_R (char)18
#define CTRL_S (char)19
#define CTRL_T (char)20
#define CTRL_U (char)21
#define CTRL_V (char)22
#define CTRL_W (char)23
#define CTRL_X (char)24
#define CTRL_Y (char)25
#define CTRL_Z (char)26
#define ESC    (char)27

#ifdef CHAR_IS_UNS
   typedef char uchar;
#else
   typedef unsigned char uchar;
#endif

typedef char * charptr;

/*
 * STANDARD SYSTEM DEPENDENCIES
 *
 */

#ifdef COHERENT
#ifndef NOVOIDPTR
#define NOVOIDPTR
#endif
#ifndef NEED_USHORT
#define NEED_USHORT
#endif
#ifndef NEED_ULONG
#define NEED_ULONG
#endif
#endif

#ifdef applec
#ifndef NEED_USHORT
#define NEED_USHORT
#endif
#endif

#ifdef sgi
#ifdef NEED_ULONG
#undef NEED_ULONG
#endif
#endif

#ifdef MSDOS
#ifndef LONGARGS
#define LONGARGS
#endif
#ifndef NEED_ULONG
#define NEED_ULONG
#endif
#ifndef NEED_USHORT
#define NEED_USHORT
#endif
#endif

/*
 *
 */

#ifdef NOVOIDPTR
   typedef char *dataptr;
#else
   typedef void *dataptr;
#endif

#ifdef NEED_ULONG
   typedef unsigned long ulong;
#endif

#ifdef NEED_USHORT
   typedef unsigned short ushort;
#endif

#ifdef AMIGA
#define CLS  "\014"            /* amiga std */
#else

#ifdef COHERENT
#define CLS  "\033[2O\033[1;1H"  /* I prefer this one myself */
#undef LONGARGS
#else

#define CLS  "\033[2J\033[1;1H"  /* ansi std */

#ifndef MSDOS
#ifndef applec
#define UNIX

#include <stdlib.h>
#include <unistd.h>

#endif /* applec */
#endif /* UNIX */

#endif /* COHERENT */
#endif /* AMIGA */

#ifdef NON_ANSI
#define ANSI ""
#define NORM ""
#define BOLD ""
#define SUBD ""
#define ITAL ""
#define UNDR ""
#define INVR ""
#else
#define ANSI "\033["
#define NORM "\033[0m"
#define BOLD "\033[1m"
#define SUBD "\033[2m"
#define ITAL "\033[3m"
#define UNDR "\033[4m"  /* These are so common, I won't even bother with */
#define INVR "\033[7m"  /* tgetstr.  Sorry, non-VT users.  :-)           */
#endif

#define sendchar(x) putchar ((char)(x))
#define until(x)    while (!(x))

#define New(x) (x *)malloc (sizeof(x))  /* Borrowed from C++.  So sue me. */

#ifdef LONGARGS
   extern time_t time(time_t *);
#else
   extern time_t time();
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef tolower  /* Strange to think, but on most systems, tolower() and */
#undef tolower  /* toupper() just don't work as you'd expect.  Humph.   */
#endif
#ifdef toupper
#undef toupper
#endif
#define tolower(x)  (char)(((x)>='A' && (x)<='Z')?((x)+('a'-'A')):(x))
#define toupper(x)  (char)(((x)>='a' && (x)<='z')?((x)+('A'-'a')):(x))

/*
 * Generic, portable low-level file operations --------------------------------
 *
 */

#define readx(f,b,n) read  (f, (char *)b, n)  /* DO NOT use buffered I/O!!! */
#define writx(f,b,n) write (f, (char *)b, n)  /* DO NOT use buffered I/O!!! */
#define modex(f,n)   chmod (f, n)

#ifndef S_IWRITE
#undef modex
#define modex(f,n)
#endif

#ifdef applec
#define creatx(x) creat(x)
#else
#define creatx(x) creat(x,0666)
#endif

#ifdef MSDOS
#define OPENMODE O_RDWR|O_BINARY
#define READMODE O_RDONLY|O_BINARY
#include <share.h>
#define openx(f,m)  sopen (f, m, SH_DENYNO, S_IREAD|S_IWRITE)
#else
#define OPENMODE O_RDWR
#define READMODE O_RDONLY
#define openx(f,m)  open (f, m)
#endif  /* MSDOS */

#endif

