/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#ifndef MBASE_H
#define MBASE_H

#include <stdinc.h>
#ifdef NCURSES
#include <ncurses.h>
#else
#include <curses.h>
#endif

#define verCURRENT 50  /* Signature for 5.0 relations */

extern WINDOW *win;

#ifdef KEY_RIGHT   /* If this is defined in curses.h, the curses package  */
#define USE_CURKEY /* supports keypad mode, and can trap our keys itself. */
#endif             /* Otherwise, we have to use our own esc-sequences. /  */

/*
 * USER-DEFINABLE DEFINITIONS -----------------------------------------------
 *
 */

#ifndef MAX_REL
#define MAX_REL  100      /* Max relations open at once for any given user */
#endif
#ifndef MAX_FLD
#define MAX_FLD   40      /* Maximum number of fields in a relation        */
#endif
#ifndef MAX_IDX
#define MAX_IDX   20      /* Maximum number of indices in a relation       */
#endif
#ifndef MAX_CACHE
#define MAX_CACHE 500     /* Maximum # of records to cache before flushing */
#endif

/*
 * ERROR CODES --------------------------------------------------------------
 *
 */

typedef enum
   {
   MB_OKAY = 0,
   MB_NO_ROOM,    /* MAX_REL is #defined to be too small   */
   MB_NO_MEMORY,  /* Not enough memory for requested task  */
   MB_NO_OPEN,    /* Cannot open given filename            */
   MB_NO_READ,    /* Cannot read given filename            */
   MB_FORMAT,     /* Relation is not in MB 4.0+ format     */
   MB_LOCKED,     /* Relation is locked by another user    */
   MB_BUSY,       /* Relation has too many users at once   */
   MB_BAD_REL,    /* Function passed bad relation struct   */
   MB_NO_WRITE,   /* Cannot write given filename           */
   MB_TIMEOUT,    /* Temporary lock has not been removed   */
   MB_BAD_REC,    /* A null rec pointer has been received  */
   MB_CORRUPT,    /* A corrupt index has been detected     */
   MB_BAD_DUP,    /* Addition would violate a nodups idx   */
   MB_NO_CURR,    /* Current record required for operation */
   MB_BAD_IDX,    /* A bad index number has been received  */
   MB_NO_SUCH,    /* The specified record can't be found   */
   MB_UNKNOWN,    /* Search command invalid                */
   MB_NO_FIELDS,  /* The new relation has no fields        */
   MB_NO_INDICES, /* The new relation has no indices       */
   MB_BAD_INDEX,  /* A proposed new index has no fields    */
   MB_DISKFULL,   /* There is not enough free space left   */
   MB_BAD_SERIAL, /* Serial #'s for records can't change   */
   MB_TMPDIR,     /* You must define a TMP directory       */
   MB_TMPERR,     /* Cannot work with TMP directory        */
   } mb_err;

/*
 * SEARCH CRITERIA ----------------------------------------------------------
 *
 */

typedef enum
   {
   FRST = 0,
   LAST,
   CURR,
   NEXT,
   PREV,
   GTEQ,
   GTHN,
   LTEQ,
   LTHN,
   EQUL
   } mb_action;

typedef enum
   {
   T_CHAR = 0,  /*  0 -- length ? (char [])         */
   T_SHORT,     /*  1 -- length 2 (short)           */
   T_USHORT,    /*  2 -- length 2 (unsigned short)  */
   T_LONG,      /*  3 -- length 4 (long)            */
   T_ULONG,     /*  4 -- length 4 (unsigned long)   */
   T_FLOAT,     /*  5 -- length 4 (float)           */
   T_DOUBLE,    /*  6 -- length 8 (double)          */
   T_MONEY,     /*  7 -- length 8 (double)          */
   T_TIME,      /*  8 -- length 4 (long)            */
   T_DATE,      /*  9 -- length 4 (long)            */
   T_SERIAL,    /* 10 -- length 4 (long)            */
   T_PHONE      /* 11 -- lenght 20 (char [])        */
   } ftype;

#define FIRST    FRST
#define CURRENT  CURR
#define PREVIOUS PREV
#define GTHAN    GTHN
#define LTHAN    LTHN
#define EQUAL    EQUL

#define AR_UP    (char)129  /* Arrows for input.c */
#define AR_DOWN  (char)130
#define AR_LEFT  (char)131
#define AR_RIGHT (char)132
#define AR_INS   (char)133  /* Insert, Delete, Home, End, PgUp, PgDn */
#define AR_DEL   (char)134
#define AR_HOME  (char)135
#define AR_END   (char)136
#define AR_PGUP  (char)137
#define AR_PGDN  (char)138

/*
 * TIME/DATE/PHONE STRUCTURES -----------------------------------------------
 *
 */

typedef long mb_time;
typedef long mb_date;
typedef char mb_phone[20];

/*
 * RELATION STRUCTURE -------------------------------------------------------
 *
 */

typedef struct
   {
   int    relcode;                       /* File handle for relation     */
   int    lckcode;                       /* Handle for lockfile          */
   int    num_i, num_f, rec_len;
   long   recz,  pos,   hack;
   long   serial;                        /* Serial value last queried    */
   int    iser;                          /* Serial field index, or num_f */

   char   relname[30];                   /* Relation name--no path       */

   int    start[MAX_FLD], siz[MAX_FLD];  /* Byte-wise info for fields    */
   ftype  type[MAX_FLD];                 /* Field types                  */
   char   name[MAX_FLD][21];             /* Field names                  */

   int    itype[MAX_IDX];                /* Dups/Nodups                  */
   char   idxs[MAX_IDX][100];            /* Index fields                 */
   char   iname[MAX_IDX][21];            /* Index name                   */

   char   mask;                          /* Encryption mask              */
   ushort pid;                           /* This Process ID              */
   int    rdonly;                        /* True if we can't write       */
   int    exc;                           /* True if we've locked it      */
   int    ver;                           /* Version number               */

   char    strobe[30];      /* Last read strobe value for each strobe */
   mb_time times[30];       /* Time strobe value last changed         */
   } relation;

#define RNULL (relation *)0

/*
 * DEFINITIONS --------------------------------------------------------------
 *
 */

#ifndef UTIL_C
#define mb_inc(a,b) mb_open(a,b,0)
#define mb_old(a,b) mb_open(a,b,1)
#define mb_tst(a)   mb_test(a,0)

#define MB_IncludeRelation   mb_inc
#define MB_TestInclude       mb_tst
#define MB_RemoveRelation    mb_rmv
#define MB_CloseAllRelations mb_die
#define MB_NumberOfRecords   mb_num
#define MB_ResetNumUsers     mb_rst
#define MB_AddRecord         mb_add
#define MB_DeleteRecord      mb_del
#define MB_DebugRelation     mb_dbg
#define MB_UpdateRecord      mb_upd
#define MB_SelectRecord      mb_sel
#define MB_LockRelation      mb_lck
#define MB_UnlockRelation    mb_unl
#define MB_FormatDate        fmt_date
#define MB_FormatTime        fmt_time
#define MB_FormatPhone       fmt_phone
#define MB_ScanDate          scn_date
#define MB_ScanTime          scn_time
#define MB_ScanPhone         scn_phone
#endif

#define curtime()     tmtotime((struct tm *)0)
#define curdate()     tmtodate((struct tm *)0)
#define curdatetime() datetimetotm((mb_date)0,(mb_time)0)
#define iswhite(x)    (x==' ' ||x=='\n' ||x=='\r' ||x=='\t')
#define istoken(x)    (x==',' ||x==':'  ||x==';'  ||x=='#' ||x=='(' || x==')')
#define putback(f)    lseek(f,_lpos,0)

/*
 * GLOBAL VARIABLES ---------------------------------------------------------
 *
 */

#ifdef INPUT_C
   char      quit_chars[20] = "";
   WINDOW   *win = (WINDOW *)0;
#else
   extern WINDOW *win;
   extern char    quit_chars[20];
#endif

#ifdef MBASE_C
   char     *mb_error = "";
   mb_err    mb_errno = MB_OKAY;
#else
   extern char     *mb_error;
   extern mb_err    mb_errno;
#endif

/*
 * DATA ENTRY STRUCTURES ----------------------------------------------------
 *
 */

#define FM_IN    1
#define FM_OUT   2
#define FM_INOUT (FM_IN|FM_OUT)
#define fm_refrnum(f,n) fm_refresh(f,&(form->fields[n]))

typedef int (*int_fn)();

typedef struct
 { int      y,    x,     len;
   ftype    type;
   int      inout, option;
   int     *mode;
   dataptr  buffer;
   char     name[40];
   charptr *opt_arr; } field;

typedef struct
 { int      curmode;
   int      key;          /* Return code from input() */
   int      curfield;
   int      nextfield;
   int      numfields;
   int      nummodes;
   int_fn   valid_fn;
   int      numlines, y, x;
   field   *fields;
   charptr *_scrn;       } de_form;

/*
 * FUNCTION PROTOTYPES ------------------------------------------------------
 *
 */

#ifdef LONGARGS

#ifndef UTIL_C
   extern relation *mb_open  (char     *, int, int);
   extern mb_err    mb_test  (char     *, int);
   extern mb_err    mb_rmv   (relation *);
   extern void      mb_die   (void);
   extern long      mb_num   (relation *);
   extern mb_err    mb_rst   (relation *, int);
   extern mb_err    mb_add   (relation *, dataptr);
   extern mb_err    mb_upd   (relation *, dataptr);
   extern mb_err    mb_del   (relation *);
   extern void      mb_dbg   (relation *);
   extern mb_err    mb_sel   (relation *, int, dataptr, mb_action, dataptr);
   extern int       compare  (relation *, char *, char *, int);
   extern int       idxnum   (relation *, char *);
   extern void      mb_exit  (int);
   extern int       strtokey (char *);
   extern mb_err    mb_lck   (relation *);
#endif

   extern mb_err   _chk_elck (relation *);
   extern mb_err    mb_unl   (relation *);
   extern void      strzcpy  (char *,     char *, int);

#ifndef MBASE_C
   extern long       elap_t       (mb_time);
   extern mb_time    tmtotime     (struct tm *);
   extern mb_date    tmtodate     (struct tm *);
   extern struct tm *datetimetotm (mb_date,     mb_time);
   extern char      *fmt_date     (mb_date,     int);
   extern char      *fmt_time     (mb_time,     int);
   extern char      *fmt_phone    (long, long, long, long, int);
   extern void       scn_phone    (long *, long *, long *, long *, char *);
   extern mb_date    scn_date     (char      *);
   extern mb_time    scn_time     (char      *);
   extern mb_time    add_time     (char      *);
   extern char      input    (dataptr,    int,    int);
   extern char      getarr   (void);
   extern void      display  (dataptr,    int,    int);
   extern void      init_curses(void);
   extern int       skip     (int,        char *);
   extern void      goeol    (int,        char *);
   extern char     *getword  (int);
   extern int       fm_fldnum  (de_form  *,  char     *);
   extern void      reltoform  (relation *,  de_form  *,  dataptr);
   extern void      formtorel  (de_form  *,  relation *,  dataptr);
   extern void      fm_refresh (de_form  *,  field    *);
   extern void      fm_refrall (de_form  *);
   extern void      fm_zero    (de_form  *);
   extern int       do_form    (de_form  *);
   extern dataptr   fm_data    (de_form  *,  char     *);
   extern void      fm_mode    (de_form *,   int);
   extern relation *mb_new      (void);
   extern mb_err    mb_addindex (relation *, char *, int,   char *);
   extern mb_err    mb_addfield (relation *, char *, ftype, long);
   extern mb_err    mb_create   (relation *, char *, int);
   extern int       mb_getname  (relation *, char *, int);
#endif

#else  /* ifndef LONGARGS */

#ifndef UTIL_C
   extern relation *mb_open();
   extern mb_err    mb_test();
   extern mb_err    mb_rmv();
   extern void      mb_die();
   extern long      mb_num();
   extern mb_err    mb_rst();
   extern mb_err    mb_add();
   extern mb_err    mb_upd();
   extern mb_err    mb_del();
   extern void      mb_dbg();
   extern mb_err    mb_sel();
   extern int       compare();
   extern int       idxnum();
   extern void      mb_exit();
   extern int       strtokey();
   extern mb_err    mb_lck();
#endif

   extern mb_err   _chk_elck();
   extern mb_err    mb_unl();
   extern void      strzcpy();

#ifndef MBASE_C
   extern long       elap_t();
   extern mb_time    tmtotime();
   extern mb_date    tmtodate();
   extern struct tm *datetimetotm();
   extern char      *fmt_date();
   extern char      *fmt_time();
   extern char      *fmt_phone();
   extern void       scn_phone();
   extern mb_date    scn_date();
   extern mb_time    scn_time();
   extern mb_time    add_time();
   extern char      input();
   extern char      getarr();
   extern void      display();
   extern void      init_curses();
   extern int       skip();
   extern void      goeol();
   extern char     *getword();
   extern int       fm_fldnum();
   extern void      reltoform();
   extern void      formtorel();
   extern void      fm_refresh();
   extern void      fm_refrall();
   extern void      fm_zero();
   extern int       do_form();
   extern dataptr   fm_data();
   extern void      fm_mode();
   extern relation *mb_new();
   extern mb_err    mb_addindex();
   extern mb_err    mb_addfield();
   extern mb_err    mb_create();
   extern int       mb_getname();
#endif

#endif  /* LONGARGS */

#endif  /* MBASE_H */

