/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#define UTIL_C
#include "mbase.h"
#include "internal.h"

extern int       _started;
extern relation *_list[MAX_REL];

/****************************************************************************/

#ifdef LONGARGS
   static void  _dec_user (relation *);
#else
   static void  _dec_user();
#endif

void
_seterr (err)
int   err;
{
   switch (mb_errno = err)
      {
      case MB_OKAY:     mb_error="No error";                             break;
      case MB_NO_ROOM:  mb_error="MAX_REL is #defined to be too small";  break;
      case MB_NO_MEMORY:mb_error="Not enough memory for requested task"; break;
      case MB_NO_OPEN:  mb_error="Cannot open given filename";           break;
      case MB_NO_READ:  mb_error="Cannot read given filename";           break;
      case MB_FORMAT:   mb_error="Relation is not in MB 5.0 format";     break;
      case MB_LOCKED:   mb_error="Relation is locked by another user";   break;
      case MB_BUSY:     mb_error="Relation is too busy";                 break;
      case MB_BAD_REL:  mb_error="Function passed bad relation struct";  break;
      case MB_NO_WRITE: mb_error="Cannot write given to relation";       break;
      case MB_TIMEOUT:  mb_error="Temporary lock has not been removed";  break;
      case MB_BAD_REC:  mb_error="A null rec pointer has been received"; break;
      case MB_CORRUPT:  mb_error="A corrupt index has been detected";    break;
      case MB_BAD_DUP:  mb_error="Addition would violate a nodups idx";  break;
      case MB_NO_CURR:  mb_error="Current record required for operation";break;
      case MB_BAD_IDX:  mb_error="A bad index number has been received"; break;
      case MB_NO_SUCH:  mb_error="The specified record can't be found";  break;
      case MB_UNKNOWN:  mb_error="Search command invalid";               break;
      case MB_NO_FIELDS:  mb_error="The new relation has no fields";     break;
      case MB_NO_INDICES: mb_error="The new relation has no indices";    break;
      case MB_BAD_INDEX:  mb_error="The new index has no fields";        break;
      case MB_DISKFULL: mb_error="There is not enough free space left";  break;
      case MB_BAD_SERIAL: mb_error="The record's serial number changed"; break;
      case MB_TMPDIR:   mb_error="You must define a TMP directory";      break;
      case MB_TMPERR:   mb_error="Cannot work with TMP directory";       break;
      default:          mb_error="Undefined error--rebuild and pray";    break;
      }
}

relation *
_fill_info (rel, fld, idx)
relation   *rel;
long             fld, idx;
{
   register int  i, j;
   int           s, e;
   char          temp[4], buf[128];
   short         tshort;
#ifdef STRUCT_4
   int           done = 0;
#endif

   rel->iser = rel->num_f;  /* Until we find otherwise */

   for (i=rel->rec_len=0; i<rel->num_f; i++)
      {
      if (lseek (rel->relcode, fld, 0) != fld)
         {
         _clr_lck (rel);  free (rel);
         relerr (MB_FORMAT, RNULL);
         }

      readx (rel->relcode,  buf,    1);  rel->type[i] = (ftype)buf[0];
      readx (rel->relcode, &tshort, 2);
      switch (rel->type[i])
         {
         case T_SHORT:
         case T_USHORT:  rel->siz[i] =  2; /* sizeof(short)  */   break;
         case T_FLOAT:   rel->siz[i] =  4; /* sizeof(float)  */   break;
         case T_DOUBLE:
         case T_MONEY:   rel->siz[i] =  8; /* sizeof(double) */   break;
         case T_TIME:
         case T_LONG:
         case T_ULONG:   rel->siz[i] =  4; /* sizeof(long)   */   break;
         case T_DATE:    rel->siz[i] =  4; /* sizeof(long)   */   break;
         case T_PHONE:   rel->siz[i] = 20; /* char[20]       */   break;
         case T_SERIAL:  rel->siz[i] =  4; /* sizeof(long)   */
                         rel->iser = i;
                        break;
         default:  rel->siz[i] = (int)tshort;              break;
         }

#ifdef STRUCT_1
      if (rel->type[i] != T_CHAR && rel->type[i] != T_PHONE)
         {
         if (rel->siz[i] == 8)       rel->rec_len = round8(rel->rec_len);
         else if (rel->siz[i] == 4)  rel->rec_len = round4(rel->rec_len);
         else                        rel->rec_len = round2(rel->rec_len);
         }
#endif

#ifdef STRUCT_3
      if (rel->type[i] != T_CHAR && rel->type[i] != T_PHONE)
         {
         if (rel->siz[i] == 2)  rel->rec_len = round2(rel->rec_len);
         else                   rel->rec_len = round4(rel->rec_len);
         }
#endif

#ifdef STRUCT_4
      if (rel->type[i] != T_CHAR && rel->type[i] != T_PHONE)
         {
         done=1;
         rel->rec_len = round2(rel->rec_len);
         }
#endif

      rel->start[i] = rel->rec_len;
      rel->rec_len += rel->siz[i];

      readx (rel->relcode, buf, 20);

      for (j=0; buf[j]!='|'; j++)
         ;
      fld += (long)j+4;  /* Advance to next field */

      buf[j] = 0;  strcpy (rel->name[i], buf);
      }

#ifdef STRUCT_1
   rel->rec_len = round8(rel->rec_len);
#endif

#ifdef STRUCT_3
   rel->rec_len = round4(rel->rec_len);
#endif

#ifdef STRUCT_4
   if (done)
      rel->rec_len = round2(rel->rec_len);
#endif

   if (lseek (rel->relcode, idx, 0) != idx)
      {
      _clr_lck (rel);
      free (rel);
      relerr (MB_FORMAT, RNULL);
      }

   for (i=0; i<rel->num_i; i++)
      { 
      readx (rel->relcode, buf,  1);  e=(int)buf[0];  /* Dups/NoDups */
      readx (rel->relcode, buf,  1);  s=(int)buf[0];  /* Num/Fields  */
      readx (rel->relcode, buf, 20);

      for (j=0; buf[j]!=':'; j++)
         ;

      idx += (long)j + 3L;
      lseek (rel->relcode, idx, 0); /* Advance to index' fields */

      buf[j] = 0;
      strcpy (rel->iname[i], buf);
      rel->itype[i]=e;  sprintf (rel->idxs[i], "%03d", s);

      for (j=0; j<s; j++)
         {
         readx (rel->relcode, &tshort, 2);
         sprintf (temp, "%03d", tshort);
         strcat (rel->idxs[i], temp);
         }

      idx = lseek (rel->relcode, 0L, 1);    /* Get current position */
      }

   rel->hack = idx;  /* Current position == reserved segment */

   if (rel->ver == verCURRENT && ! rel->rdonly)
      {
      lseek (rel->lckcode, lckPOS_USERS, 0);
      readx (rel->lckcode, buf, 1);
      buf[0] = (char)((unsigned int)buf[0] +1);   /* Increment the   */
      lseek (rel->lckcode, lckPOS_USERS, 0);      /* number of users */
      writx (rel->lckcode, buf, 1);

      _clr_lck (rel);
      }

   relerr (MB_OKAY, rel);
}

void
_close_proc (rel)
relation    *rel;
{
   if (rel->ver == verCURRENT)
      {
      (void)mb_unl (rel);
      _dec_user (rel);
      }

   close (rel->relcode);
   free  (rel);
}

void
_divine_mask (rel, key)
relation     *rel;
int                key;
{
   rel->mask = (char)(
      ((key & (int)0x04) << 5) |  /* 00000100 -> 10000000 */
      ((key & (int)0x30) << 1) |  /* 00110000 -> 01100000 */
      ((key & (int)0x80) >> 3) |  /* 10000000 -> 00010000 */
      ((key & (int)0x03) << 2) |  /* 00000011 -> 00001100 */
      ((key & (int)0x40) >> 5) |  /* 01000000 -> 00000010 */
      ((key & (int)0x08) >> 3));  /* 00001000 -> 00000001 */
}

int
_identify (rel)
relation  *rel;
{
   register int  i;
   if (rel == RNULL || !_started)  return -1;
   for (i=0; i<MAX_REL; i++)
      if (_list[i] == rel)  break;
   if (i == MAX_REL)   return -1;
   if (rel->ver != verCURRENT)  return -2;
   return i;
}

mb_err
_format  (rel, rec, stage)
relation *rel;
dataptr        rec;
int                 stage;
{
   register int  i;
   double        tdbl;
   long          nexts, tlong;
   int           a;
   long          ac,pre,num,ext;
   char         *pch, temp[25];

   if (! rec)  baderr (MB_BAD_REC);

   if (stage == 2)
    { GO_NEXTS (rel);
      readx    (rel->relcode, &nexts, 4);
    }

   for (i=a=0; i<rel->num_f; i++)
      {
      if (stage == 1)
         {
         if (rel->type[i] == T_PHONE)
            {
            pch = (char *)rec + rel->start[i];
            scn_phone (&ac,&pre,&num,&ext, pch);
            strcpy (temp, fmt_phone (ac,pre,num,ext, -1));
            strncpy (pch, temp, 20);
            }
         if (rel->type[i] == T_MONEY)
            {
            tdbl = *(double *)((char *)rec+rel->start[i]);
            tlong = (long)(tdbl * 100.0);  tdbl = (double)tlong / 100.0;
            *(double *)((char *)rec+rel->start[i]) = tdbl;
            }
         }
      else
         {
         if (rel->type[i] == T_SERIAL)
            {
            *(long *)((char *)rec+rel->start[i]) = nexts;
            rel->serial = nexts;
            _cryptf ((dataptr)((char *)rec+rel->start[i]), 4, rel->mask);
            a = 1;
            }
         }
      }

   if (a)
      {
      GO_NEXTS (rel);  nexts++;
      writx    (rel->relcode, &nexts, 4);
      }

   return MB_OKAY;
}

void
_crypt   (rel, rec)
relation *rel;
dataptr        rec;
{
#ifndef NOENCRYPT
   register int  i;

   if (rel->mask)
      for (i=0; i<rel->num_f; i++)
         _cryptf ((char *)rec+rel->start[i], rel->siz[i], rel->mask);
#else
   ;  /* Some compilers complain about null-functions */
#endif
}

void
_cryptf (rec, siz, mask)
dataptr  rec;
int           siz, mask;
{
#ifndef NOENCRYPT
   register int    i;
   register char  *c;

   if (mask != 0)
      for (i=0,c=rec; i<siz; i++,c++)
       { *c ^= mask;
         mask  = (mask + 1) & (int)0xFF;
       }
#else
   ;  /* Some compilers complain about null-functions */
#endif
}

mb_err
_check_dup (rel, rec, idx, ign)
relation   *rel;
dataptr          rec;
int                   idx;
long                       ign;
{
   dataptr  mem;
   long     pos;
   int      dir;

   if (rel->itype[idx] == 1)  reterr (MB_OKAY, 0);

   GO_TOP (rel, idx);
   readx (rel->relcode, &pos, 4);
   if (pos==0L)
      reterr (MB_OKAY, 0);

   if ((mem = (dataptr)malloc (1+ rel->rec_len)) == NULL)
      reterr (MB_NO_MEMORY, -1);

   for (;;)
      {
      dir = _compare (rel, rec, _memrec(rel,pos,mem), idx);

      if (dir == 0)
         {
         if (pos == ign)  break;  /* If we're about to change the rec, okay */
         free (mem);

         reterr (MB_BAD_DUP, -1);
         }

      GO_POINT (rel, pos, idx, dir);
      readx (rel->relcode, &pos, 4);

      if (pos == 0L)  break;
      }
   free (mem);

   reterr (MB_OKAY, MB_OKAY);
}

long
_append  (rel, rec)
relation *rel;
dataptr        rec;
{
   long           rcd, temp;
   register int   i;
   char           buf[2];

   lseek (rel->relcode, POS_NUMREC, 0);
   readx (rel->relcode, &rcd, 4);  rcd++;
   lseek (rel->relcode, POS_NUMREC, 0);
   writx (rel->relcode, &rcd, 4);

   GO_START (rel, rcd);

   for (i=0,temp=0L,buf[0]=BAL_EV; i<rel->num_i; i++)
      {
      writx (rel->relcode, &temp, 4);
      writx (rel->relcode, &temp, 4);
      writx (rel->relcode, &temp, 4);
      writx (rel->relcode, buf,   1);
      }
   writx (rel->relcode, rec, rel->rec_len);

   return rcd;
}

void
_remove  (rel, bad)
relation *rel;
long           bad;
{
   int      i,   dir;
   long     rep, tmp;
   char     temp[5];
   dataptr  buf;

   lseek (rel->relcode, POS_NUMREC, 0);
   readx (rel->relcode, &rep, 4);  rep--;
   lseek (rel->relcode, POS_NUMREC, 0);
   writx (rel->relcode, &rep, 4);  rep++;  /* Find replacement record */

   if (rep == bad)  return;

   if ((buf=(dataptr)malloc (rel->rec_len +13*rel->num_i +1)) == NULL)  return;

   GO_START (rel, rep);
   readx    (rel->relcode, buf, rel->rec_len +13*rel->num_i);
   GO_START (rel, bad);
   writx    (rel->relcode, buf, rel->rec_len +13*rel->num_i);

   for (i = 0; i < rel->num_i; i++)
      {
      GO_POINT (rel, rep, i, 0);
      readx (rel->relcode, &tmp, 4);
      GO_BAL   (rel, rep, i);
      readx (rel->relcode, temp, 1);  dir = (temp[0] & PARDIR);

      if (tmp == 0L)
         GO_TOP (rel, i);
      else
         GO_POINT (rel, tmp, i, (dir ? 1 : -1));
      writx (rel->relcode, &bad, 4);

      GO_POINT (rel, rep, i, -1);
      readx (rel->relcode, &tmp, 4);
      if (tmp != 0L)
         {
         GO_POINT (rel, tmp, i, 0);
         writx (rel->relcode, &bad, 4);
         }

      GO_POINT (rel, rep, i, 1);
      readx (rel->relcode, &tmp, 4);
      if (tmp != 0L)
         {
         GO_POINT (rel, tmp, i, 0);
         writx (rel->relcode, &bad, 4);
         }
      }

   free (buf);
}

mb_err
_link    (rel, rcd)   /* CACHED */
relation *rel;
long           rcd;
{
   register int  i;

   _free_cache ();

   for (i=0; i<rel->num_i; i++)
      {
      _drop (rel, rcd, i, 0L);
      if (_check (rel, rcd, 0L, i))
         {
         _flush_cache (rel, i);
         return mb_errno;
         }
      _flush_cache (rel, i);
      }
   return MB_OKAY;
}

void
_drop    (rel, rcd, idx, top)  /* CACHED */
relation *rel;
long           rcd,      top;
int                 idx;
{
   long     pos, par;
   int      dir;
   char     temp[5];
   dataptr  a, b;
   cache   *ptr;

   ptr = _read_cache (rel, top, idx);
   if (top == 0L)
      {
      pos = ptr->num;
      par = 0L;
      }
   else
      {
      par = ptr->parent;
      pos = top;
      }

   a = (dataptr)malloc (1+ rel->rec_len);
   b = (dataptr)malloc (1+ rel->rec_len);

   for ( ; pos != 0L; )
      {
      ptr = _read_cache (rel, pos, idx);
      temp[0] = ptr->parbal;

      dir = _compare (rel, _memrec(rel,rcd,a), b=_memrec(rel,pos,b), idx );
/*
 * POSSIBLE ERROR POINT -- _memrec is being used twice in one
 * function call.  If it's going to be a problem, it'll show up after
 * the second add, delete or update.... if it doesn't, this isn't
 * the cause of your trouble.
 *
 */

      if (dir == 0)
         {
         dir = ((temp[0] & BAL) == BAL_RT || (temp[0] & BAL) == BAL_FR) ? -1:1;
         }
      temp[0] += dir;

      _change_cache (ptr, parbal, temp[0]);

      par = pos;
      pos = _cache_field (ptr, dir);
      }

   free (a);  free (b);

   if (par == 0L)
      {
      ptr = _read_cache (rel, 0L, idx);
      _change_cache (ptr, num, rcd);
      }
   else
      {
      temp[0] = (char)(((dir==1) ? PARDIR:0) | BAL_EV);

      ptr = _read_cache (rel, rcd, idx);
      _change_cache (ptr, parbal, temp[0]);

      ptr = _read_cache (rel, par, idx);
      if (dir == -1)      _changeqcache (ptr, left, rcd);
      else if (dir == 0)  _changeqcache (ptr, parent, rcd);
      else                _changeqcache (ptr, right, rcd);
      ptr->changed = 1;
      }

   ptr = _read_cache (rel, rcd, idx);
   _change_cache (ptr, parent, par);
}

mb_err
_check   (rel, st, ed, idx)  /* CACHED */
relation *rel;
long           st, ed;
int                    idx;
{
   long   pos, tmp;
   cache *ptr;

   for (pos = st; pos != ed; )
      {
      if (pos == 0L)  break;

      ptr = _read_cache (rel, pos, idx);
      tmp = ptr->parent;

      if (! BALANCED ( (ptr->parbal & BAL) ))
         if (_balance (rel, pos, idx, (int)(ptr->parbal & BAL)))
            return mb_errno;
      pos = tmp;
      }

   return MB_OKAY;
}

/****************************************************************************/

int
compare  (rel, ptra, ptrb, idx)  /* -1 == ptra < ptrb */
relation *rel;
dataptr        ptra, ptrb;
int                        idx;
{
   int  i;
   if (_identify (rel) < 0)           interr (MB_BAD_REL, -1);
   if (idx < 0 || idx >= rel->num_i)  interr (MB_BAD_IDX, -1);
   if (ptra == ptrb)                  interr (MB_OKAY,     0);

   _crypt (rel, ptra);
   _crypt (rel, ptrb);
   i = _compare (rel, ptra, ptrb, idx);
   _crypt (rel, ptra);
   _crypt (rel, ptrb);
   interr (MB_OKAY, i);
}

int
idxnum   (rel, name)
relation *rel;
char          *name;
{
   int  i;

   if (_identify (rel) < 0)  interr (MB_BAD_REL, -1);

   for (i = 0; i < rel->num_i; i++)
      if (! strcmp (name, rel->iname[i]))
         break;
   if (i == rel->num_i)  interr (MB_BAD_IDX, -1);

   interr (MB_OKAY, i);
}

static void
_dec_user (rel)
relation  *rel;
{
   char  buf[2];

   lseek (rel->lckcode, lckPOS_USERS, 0);
   readx (rel->lckcode, buf, 1);
   buf[0] = (char)((unsigned int)buf[0] -1);
   if ((uchar)buf[0] > 254)  buf[0] = 0;
   lseek (rel->lckcode, lckPOS_USERS, 0);  writx (rel->lckcode, buf, 1);
}

