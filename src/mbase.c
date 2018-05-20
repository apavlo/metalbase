/*   ********************************************************************   *
  ***                                                   unix compatible! ***
 *    MetalBase 5.0.....................................................    *
 *                                                                          *
 *    Simultaneous multi-user use of multiple relations!                    *
 *    Users may have many relations open at once, even the same one!        *
 *    Environmentally safe--no chloroflourocarbons to destroy the ozone!    *
 *    Loads of wonderful utilities, like data entry and report writing!     *
 *    Unlimited indicies per relation/Up to 999 fields per composite index! *
 *    Up to 4.2 billion records per relation (that's a lot, friend...)      *
 *    Bizarre intermittent bugs (NOT), just like the expensive programs!    *
 *    Portable to most any small-scale platform you can think of!           *
 *    And, unless they're weird, your kids will eat it.                     *
 *                                                               /\         *
 *    Released October 1st, 1992 by Huan-Ti                  rj /  \        *
 *                                                             /    \       *
 *   "Ye hath mushrooms for $1.99 a pound.  Ye art             \    /       *
 *    truly a Calvinist."                                       \  / tp     *
 *                       -- II Calvin 7:1                        \/         *
 *                                                                          *
 *          206/881-2624 <-----= May 26, 1996 =-----> 615/494-0445          *
  ***       t-richj@microsoft.com / / virtual!root@owlnet.rice.edu       ***
 *   ********************************************************************   */

#define MBASE_C
#include "mbase.h"
#include "internal.h"

int       _started = 0;
int       _really  = 1;
relation *_list [MAX_REL];

#ifndef linux
#ifndef MSDOS
#ifdef LONGARGS
   extern char *getenv (char _FAR_ *);
#else
   extern char *getenv();
#endif
#endif
#endif

relation *
mb_open (filename, key, useold)
char    *filename;
int                key, useold;
{
   relation     *rel;
   int           i, rc, fZero = 0;
   char          buf[256], *pch;
   long          fld, idx, tlong;
   short         tshort;

   _really = 0;
   if (mb_test (filename, useold) != MB_OKAY)
    { if (_really != 0)
       { if (_really < 0)  close (0-_really);
         else              close (_really);
       }
      _really = 1;
      relerr (mb_errno, RNULL);
    }
   if ((rel = New (relation)) == RNULL)
    { if (_really != 0)
       { if (_really < 0)  close (0-_really);
         else              close (_really);
       }
      _really = 1;
      relerr (MB_NO_MEMORY, RNULL);
    }

   rel->rdonly = (_really < 0) ? 1 : 0;
   if (_really < 0)  _really = 0-_really;
   rel->relcode = rc = _really;  _really = 1;

   lseek (rel->relcode, 0L, 0);
   readx (rel->relcode, buf, 1);

   rel->pos = 0L;
   rel->exc = 0;
   rel->pid = getpid();
   rel->ver = (int)buf[0];

   for (i=0; i<MAX_REL; i++)
      if (_list[i] == RNULL)
         break;
   _list[i] = rel;        /* Assign it so mb_rmv()/mb_die() can find it */

   if ((pch = strrchr (filename, DIRSEP)) == NULL)
      pch = filename;
   else
      pch++;
   strcpy (buf, pch);

   if ((pch = strrchr (buf, '.')) != NULL)
      *pch = 0;
   strcpy (rel->relname, buf);

   lseek (rel->relcode, POS_FIELDPTR(rel->ver), 0);

   readx (rc, &fld,        4);
   readx (rc, &idx,        4);
   readx (rc, &rel->recz,  4);
   readx (rc, &tlong,      4);
   readx (rc, &tlong,      4);
   readx (rc, &tshort,     2);  rel->num_f = tshort;
   readx (rc, &tshort,     2);  rel->num_i = tshort;

   _divine_mask (rel, key);

   if (rel->ver == verCURRENT && ! rel->rdonly)
      {
      if ((pch = getenv ("TMP")) != NULL ||
          (pch = getenv ("TEMP")) != NULL)
         {
         strcpy (buf, pch);  /* If they define a directory, use it. */
         }
      else                   /* Otherwise, try to guess a default directory. */
         {
#ifdef UNIX
         strcpy (buf, "/tmp");
#endif
         }
      if (! buf[0])
         {
         close (rel->relcode);
         free (rel);
         relerr (MB_TMPDIR, RNULL);
         }
      if (buf[(i = strlen(buf))-1] != DIRSEP)
         {
         buf[i] = DIRSEP;
         buf[i+1] = 0;
         }
      strcat (buf, rel->relname);
      strcat (buf, ".lck");

      if (access (buf, 0) == -1)
         if ((rel->lckcode = creatx (buf)) > 0)
            {
            close (rel->lckcode);
            fZero = 1;
            rel->lckcode = -1;
            }

      rel->lckcode = openx (buf, OPENMODE);

      if (rel->lckcode <= 0)
         {
         close (rel->relcode);
         free (rel);
         relerr (MB_TMPERR, RNULL);
         }

      if (fZero)
         {
         modex (buf, 0666);              /* The 100 bytes consist of:    */
         for (i = 0; i < 100; i++)       /*   2 : Number of users in rel */
            buf[i] = 0;                  /*   2 : Exclustive lock        */
         lseek (rel->lckcode, 0L, 0);    /*   6 : 3 Hacklock positions   */
         writx (rel->lckcode, buf, 100); /*  60 : 30 Queue positions     */
         }                               /*  30 : 30 Strobe positions    */

/*
 * Lock file has been created; keep going.
 *
 */

      if (_set_lck (rel) || _chk_elck (rel))
         {
         if (rel->exc & 2)
            _clr_lck (rel);
         close (rel->relcode);
         free (rel);
         relerr (mb_errno, RNULL);
         }
      }

   return _fill_info (rel, fld, idx);
}

mb_err
mb_test (filename, useold)
char    *filename;
int                useold;
{
   int     i, rc, rdonly = 0;
   int     ver;
   char    buffer[256];

   if (_started == 0)
      {
      _started = 1;
      for (_started=1, i=0; i<MAX_REL; i++)  /* Initialize list */
         _list[i] = RNULL;                   /* (oh boy fun!)   */
      }

   for (i=0; i<MAX_REL; i++)
      if (_list[i] == RNULL)  break;

   if (i == MAX_REL)
      {
      _really = 0;
      reterr (MB_NO_ROOM, -1);
      }

   strcpy (buffer, filename);
   if (strcmp (&buffer[strlen(buffer)-4], ".rel"))
      strcat (buffer, ".rel");

   if ((rc = openx (buffer, OPENMODE)) == -1)
      {
      if ((rc = openx (buffer, READMODE)) == -1)
         {
         if (_really)  close (rc);
         else         _really = 0;
         reterr (MB_NO_OPEN, -1);                    /* Can we open it? */
         }
      rdonly = 1;
      }
   if (readx (rc, buffer, 2) != 2)
      {
      if (_really)  close (rc);  else _really = 0;
      reterr (MB_NO_READ, -1);                    /* Can we read it? */
      }

   ver = (int)buffer[0];

   if (useold   && (ver < 40 || ver > verCURRENT))
      ver = 0;
   if (! useold && ver != verCURRENT)
      ver = 0;

   if (!ver)
      {
      if (_really)  close (rc); else _really = 0;
      reterr (MB_FORMAT, -1);             /* Is it a 5.0 relation? */
      }

#ifndef UNIX_LOCKS
   if (ver == verCURRENT && !rdonly && ((int)((uchar)buffer[1]) == 255))
      {
      if (_really)  close (rc);  else _really = 0;
      reterr (MB_BUSY, -1);                   /* Are there 255 users already? */
      }
#endif

   if (_really)  close (rc);
   else          _really = (rdonly) ? 0-rc : rc;    /* - == readonly */

   reterr (MB_OKAY, MB_OKAY);
}

mb_err
mb_add   (rel, rec)
relation *rel;
dataptr        rec;
{
   int     i;
   long    rcd;
   int     err;

   if (_identify (rel) < 0)            reterr (MB_BAD_REL,  -1);
   if (rel->rdonly)                    reterr (MB_NO_WRITE, -1);

   if (_format (rel, rec, 1))          reterr (mb_errno,    -1);
   if (_set_lck (rel))                 reterr (mb_errno,    -1);
   if (_chk_elck (rel))                lckerr (rel, MB_LOCKED, -1);

   _crypt (rel, rec);
   for (i=0; i<rel->num_i; i++)
      if (_check_dup (rel, rec, i, 0L))  lckerr (rel, mb_errno, -1);

   _format (rel, rec, 2);

   if (! (rcd = _append (rel, rec)))  lckerr (rel, MB_NO_WRITE, -1);
   if (_link (rel, rcd))              lckerr (rel, MB_CORRUPT,  -1);

   rel->pos = rcd;

   _crypt (rel, rec);
   err = MB_OKAY;

   lckerr (rel, MB_OKAY, MB_OKAY);
}

mb_err
mb_upd   (rel, rec)
relation *rel;
dataptr        rec;
{
   int     i;
   long    rcd;

   if (_identify (rel) < 0)            reterr (MB_BAD_REL, -1);
   if ((rcd = rel->pos) == 0L)         reterr (MB_NO_CURR, -1);
   if (_format (rel, rec, 1))          reterr (mb_errno,   -1);
   if (rel->rdonly)                    reterr (MB_NO_WRITE,-1);
   if (_chk_elck (rel))                reterr (MB_LOCKED,  -1);

   if (rel->iser < rel->num_f)
      {
      if (*(long *)((char *)rec + rel->start[rel->iser]) != rel->serial)
         reterr (MB_BAD_SERIAL, -1);
      }
   _crypt (rel, rec);
   for (i=0; i<rel->num_i; i++)
      if (_check_dup (rel, rec, i, rcd)) reterr (mb_errno, -1);
   if (_set_lck (rel))                   reterr (mb_errno, -1);

   if (_delete (rel, rel->pos) <= 0L)
      if (mb_errno != MB_OKAY)         lckerr (rel, mb_errno, -1);

   GO_RECID (rel, rel->pos);
   writx (rel->relcode, rec, rel->rec_len);

   if (_link (rel, rel->pos))          lckerr (rel, MB_CORRUPT,  -1);

   _crypt (rel, rec);
   lckerr (rel, MB_OKAY, MB_OKAY);
}

mb_err
mb_del   (rel)
relation *rel;
{
   if (_identify (rel) < 0)    reterr (MB_BAD_REL, -1);
   if (rel->pos == 0L)         reterr (MB_NO_CURR, -1);
   if (_chk_elck (rel))        reterr (MB_LOCKED,  -1);
   if (rel->rdonly)            reterr (MB_NO_WRITE,-1);
   if (_set_lck (rel))         reterr (mb_errno,   -1);

   if (_delete (rel, rel->pos) <= 0L)
      if (mb_errno != MB_OKAY)  lckerr (rel, mb_errno, -1);

   _remove (rel, rel->pos);

   rel->pos = 0L;

   lckerr (rel, MB_OKAY, MB_OKAY);
}

mb_err
mb_rmv   (rel)
relation *rel;
{
   int  i;

   if ((i = _identify (rel)) == -1)  reterr (MB_BAD_REL, -1);
   _list[i] = RNULL;
   _close_proc (rel);
   baderr (MB_OKAY);
}

void
mb_exit (x)
int      x;
{
   mb_die ();
   exit   (x);
}

void
mb_die ()
{
   int  i;
   if (_started)
      for (i=0; i<MAX_REL; i++)
         if (_list[i] != RNULL)
          { _close_proc (_list[i]);
            _list[i] = RNULL;
          }
   _seterr (MB_OKAY);
}

long
mb_num   (rel)
relation *rel;
{
   long    x;
   if (_identify (rel) < 0)  longerr (MB_BAD_REL, -1);
   if (lseek (rel->relcode, POS_NUMREC, 0) != POS_NUMREC)
      longerr (MB_FORMAT, -1);
   readx (rel->relcode, &x, 4);
   longerr (MB_OKAY, x);
}

int
strtokey (str)
char     *str;
{
   char *a;
   int   x;
   for (x=0, a=str; a && *a; a++)
      x = (x + (int)*a) % 240 + 15;
   return x;
}

void
strzcpy (new, old, num)
char    *new,*old;
int                num;
{
   int  i;
   char         *a,*b;

   if (!new || !old)  return;
   for (a=new,b=old,i=0; i<num && *b; a++,b++,i++)
      *a = *b;
   *a = 0;
}

mb_err
mb_sel   (rel, idx, buf, act, comp)
relation *rel;
int            idx;
mb_action                act;
dataptr             buf,      comp;
{
   dataptr rec;
   long    off, top;

   _free_cache ();

   if (_identify (rel) < 0)                            baderr (MB_BAD_REL);
   if (act != CURR && (idx < 0 || idx >= rel->num_i))  baderr (MB_BAD_IDX);
   if (_chk_elck (rel))                                baderr (MB_LOCKED);
   if (_set_lck (rel))                                 baderr (mb_errno);

   rec = (comp == NULL) ? buf : comp;
   if (rec != NULL)  _crypt (rel, rec);

   if (rel->pos == 0L)
      {
      if (act == NEXT)  act = FRST;
      if (act == PREV)  act = LAST;
      }

   switch (act)
      {
      case FRST:
      case LAST:  off = _find_ends (rel, idx, (act == FRST) ? -1 : 1);
                 break;
      case CURR:  off = rel->pos;
                 break;
      case NEXT:
      case PREV:  off = _find_seq (rel, 0L, rel->pos, idx, (act == NEXT)?1:-1);
                 break;
      case GTEQ:
      case GTHN:
      case LTEQ:
      case LTHN:
      case EQUL:  GO_TOP (rel, idx);  readx (rel->relcode, &top, 4);
                  off = _search (rel, top, idx, act, rec);
                 break;
      default  :  baderr (MB_UNKNOWN);
                 break;
      }

   if (off == 0L)
      {
      _seterr (MB_NO_SUCH);
      }
   else
      {
      _seterr (MB_OKAY);
      rel->pos = off;
      }

   _crypt  (rel, rec);           /* Reverse-encrypt the comparison buffer */
   _memrec (rel, rel->pos, buf); /* Read in the output buffer, encrypted  */
   _crypt  (rel, buf);           /* Decrypt the output buffer             */

   if (rel->pos && rel->iser < rel->num_f)
      {
      rel->serial = *(long *)((char *)buf + rel->start[rel->iser]);
      }

   _clr_lck (rel);

   return mb_errno;
}

