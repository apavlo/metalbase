/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#ifndef INTERNAL_H
#define INTERNAL_H

/*
 * RELATION FORMAT ----------------------------------------------------------
 *
 */

#define POS_SIGNATURE  0L   /* This information is described in */
#define POS_UNUSED1    1L   /* more detail in the file          */
#define POS_UNUSED2    2L   /*    ../dox/format.dox             */
#define POS_UNUSED3    4L
#define POS_FIELDPTR(x) (x == 40 ? 2L : 6L)  /* For 4.0 compatibility */
#define POS_INDEXPTR  10L
#define POS_RECZERO   14L
#define POS_NUMREC    18L
#define POS_NEXTSER   22L
#define POS_NUMFIELD  26L
#define POS_NUMINDEX  28L
#define POS_INDICES   30L

#define lckPOS_USERS    0L   /* more detail in the file          */
#define lckPOS_ELOCK    2L   /*    ../dox/format.dox             */
#define lckPOS_HLOCK    4L
#define lckPOS_QUEUE   10L
#define lckPOS_STROBE  70L

/*
 * INTERNAL STRUCTURE DEFINITIONS AND UTILITIES -----------------------------
 *
 */

typedef struct
   {
   long    num;
   long    left, right, parent;
   char    parbal;
   int     changed;
   } cache;

#define _change_cache(p,f,v)   { p->changed = 1; _changeqcache(p,f,v); }
#define _changeqcache(p,f,v)     p->f = v
#define _cache_field(p,d) (d==1?p->right: (d==-1?p->left : p->parent))

#ifndef BLAST_C
#ifdef LONGARGS
   extern relation  *_fill_info   (relation *, long, long);
   extern long       _append      (relation *, dataptr);
   extern mb_err     _balance     (relation *, long,    int,     int);
   extern mb_err     _check       (relation *, long,    long,    int);
   extern mb_err     _check_dup   (relation *, dataptr, int,     long);
   extern void       _close_proc  (relation *);
   extern mb_err     _clr_lck     (relation *);
   extern int        _comp_double (double *, double *);
   extern int        _comp_fld    (relation *, dataptr, dataptr, int);
   extern int        _comp_float  (float  *, float  *);
   extern int        _comp_long   (long   *, long   *);
   extern int        _comp_short  (short  *, short  *);
   extern int        _comp_string (char   *, char   *, int);
   extern int        _comp_ulong  (ulong  *, ulong  *);
   extern int        _comp_ushort (ushort *, ushort *);
   extern int        _compare     (relation *, dataptr, dataptr, int);
   extern void       _crypt       (relation *, dataptr);
   extern void       _cryptf      (dataptr,    int,     int);
   extern long       _delete      (relation *, long);
   extern void       _dislink     (relation *, long, int,  long);
   extern void       _divine_mask (relation *, int);
   extern void       _drop        (relation *, long,    int,     long);
   extern void       _dumprec     (relation *, dataptr);
   extern long       _find_ends   (relation *, int,  int);
   extern long       _find_seq    (relation *, long, long, int,  int);
   extern mb_err     _format      (relation *, dataptr, int);
   extern int        _identify    (relation *);
   extern mb_err     _link        (relation *, long);
   extern dataptr    _memrec      (relation *, long, dataptr);
   extern dataptr    _rec         (relation *, long);
   extern void       _remove      (relation *, long);
   extern void       _replace     (relation *, long, long, int);
   extern long       _search      (relation *, long, int,  mb_action, dataptr);
   extern mb_err     _set_lck     (relation *);
   extern void       _seterr      (int);
   extern void       _strobe      (relation *, int);
   extern void       _zero        (relation *, long, int);
   extern cache     *_read_cache  (relation *, long, int);  /* CACHE.C */
   extern void       _flush_cache (relation *,       int);  /* CACHE.C */
   extern cache     *_new_cache   (relation *,       int);  /* CACHE.C */
   extern void       _free_cache  (void);                   /* CACHE.C */
#else
   extern relation  *_fill_info();
   extern long       _append();
   extern mb_err     _balance();
   extern mb_err     _check();
   extern mb_err     _check_dup();
   extern void       _close_proc();
   extern mb_err     _clr_lck();
   extern int        _comp_double();
   extern int        _comp_fld();
   extern int        _comp_float();
   extern int        _comp_long();
   extern int        _comp_short();
   extern int        _comp_string();
   extern int        _comp_ulong();
   extern int        _comp_ushort();
   extern int        _compare();
   extern void       _crypt();
   extern void       _cryptf();
   extern long       _delete();
   extern void       _dislink();
   extern void       _divine_mask();
   extern void       _drop();
   extern void       _dumprec();
   extern long       _find_ends();
   extern long       _find_seq();
   extern mb_err     _format();
   extern int        _identify();
   extern mb_err     _link();
   extern dataptr    _memrec();
   extern dataptr    _rec();
   extern void       _remove();
   extern void       _replace();
   extern long       _search();
   extern mb_err     _set_lck();
   extern void       _seterr();
   extern void       _strobe();
   extern void       _zero();
   extern cache     *_read_cache();     /* CACHE.C */
   extern void       _flush_cache();    /* CACHE.C */
   extern cache     *_new_cache();      /* CACHE.C */
   extern void       _free_cache();     /* CACHE.C */
#endif

#define GO_BASE(rel,rcd,idx,off) \
 lseek (rel->relcode,          \
        rel->recz+(rcd-1)*(rel->rec_len+(long)13*rel->num_i)+(long)13*idx+off,0)
#define GO_TOP(rel,idx)  lseek (rel->relcode, POS_INDICES + (long)4*idx, 0)
#define GO_NEXTS(rel)    lseek (rel->relcode, POS_NEXTSER, 0)
#define GO_INDEX(rel,rcd,idx)    GO_BASE (rel, rcd,        idx,  0L)
#define GO_START(rel,rcd)        GO_BASE (rel, rcd,          0,  0L)
#define GO_RECID(rel,rcd)        GO_BASE (rel, rcd, rel->num_i,  0L)
#define GO_BAL(rel,rcd,idx)      GO_BASE (rel, rcd,        idx, 12L)
#define GO_POINT(rel,rcd,idx,d) \
                GO_BASE (rel, rcd, idx, (((d)==-1)?0L:((d)==1?4L:8L)) )

#define PARDIR (int)0x80
#define BAL    (int)0x7F

#define BAL_FL ';'    /* Balance == Far Left  (off balance) */
#define BAL_LT '<'    /* Balance == Left      (on balance)  */
#define BAL_EV '='    /* Balance == Even :-]  (ON balance)  */
#define BAL_RT '>'    /* Balance == Right     (on balance)  */
#define BAL_FR '?'    /* Balance == Far Right (off balance) */

#define VAL_BAL(b)  ((int)(b-BAL_EV))

#define NUM_BAL(b)  (b==BAL_FL?-1:(b==BAL_LT?-1:(b==BAL_EV?0:(b==BAL_RT?1:1))))
#define BALANCED(b) (b==BAL_LT || b==BAL_EV || b==BAL_RT)

#define togg(x) (x = 0-x)

#define lckerr(r,e,x)  { _clr_lck(r); _seterr(e); return (mb_err)(x); }
#define baderr(x)      {              _seterr(x); return (mb_err)(x); }
#define reterr(e,x)    {              _seterr(e); return (mb_err)(x); }
#define interr(e,x)    {              _seterr(e); return    (int)(x); }
#define longerr(e,x)   {              _seterr(e); return   (long)(x); }
#define relerr(e,x)    {              _seterr(e); return (relation *)(x); }

#define round2(x) (x%2==0?x:(x+2-(x%2)))
#define round4(x) (x%4==0?x:(x+4-(x%4)))
#define round8(x) (x%8==0?x:(x+8-(x%8)))
#endif

#endif

