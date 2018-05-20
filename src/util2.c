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

mb_err
_balance (rel, rcd, idx, bal)  /* CACHED */
relation *rel;
long           rcd;
int                 idx, bal;
{
   long   rep, rp;
   cache *ptr;

   if (! (rep = _find_seq (rel, 0L, rcd, idx, NUM_BAL(bal))) )
      baderr (MB_CORRUPT);

   ptr = _read_cache (rel, rep, idx);
   rp = ptr->parent;

   _dislink (rel, rep, idx, rcd);
   _replace (rel, rcd, rep, idx);
   _drop    (rel, rcd, idx, rep);

   if (rp != rcd)  _check (rel, rp, rep, idx);  /* Stop at _rep_ */

   ptr = _read_cache (rel, rcd, idx);
   rp = ptr->parent;

   _check (rel, rp, rep, idx);

   baderr (MB_OKAY);
}

void
_dislink (rel, pos, idx, end)  /* CACHED */
relation *rel;
long           pos,      end;
int                 idx;
{
   char   temp[5];
   long   ch, par, tmp;
   int    dir = 0, tdir;
   cache *ptr;

   ptr = _read_cache (rel, pos, idx); /* There will be one child, at most */
   ch = ptr->left;
   if (ch)
      dir = -1;
   else
      {
      ch = ptr->right;     /* We already read in this record! */
      dir = (ch ? 1 : 0);
      }

   par = ptr->parent;
   temp[0] = (char)((int)ptr->parbal & PARDIR);
   tdir = temp[0]?1:-1;

   ptr = _read_cache (rel, par, idx);
   ptr->changed = 1;
   if (par == 0L)
      {
      _changeqcache (ptr, num, ch);
      }
   else
      {
      if (temp[0]) _changeqcache (ptr, right, ch);
      else         _changeqcache (ptr, left,  ch);
      }

   if (ch)
      {
      ptr = _read_cache (rel, ch, idx);
      _change_cache (ptr, parent, par);
      temp[0] = ptr->parbal;
      temp[0] = (char)((int)(temp[0] & BAL) | (tdir == 1 ? PARDIR : 0));

      _change_cache (ptr, parbal, temp[0]);
      }

   for (tmp=par, dir=tdir; tmp != 0L; )   /* Update balances: */
      {
      ptr = _read_cache (rel, tmp, idx);
      temp[0] = ptr->parbal;
      temp[0] = (char)((int)(temp[0] & PARDIR) | ((temp[0] & BAL) - dir));

      _change_cache (ptr, parbal, temp[0]);

      dir = (temp[0] & PARDIR) ? 1 : -1;
      if (tmp == end)  break;

      tmp = ptr->parent;
      }
}

void
_replace (rel, old, new, idx)  /* CACHED */
relation *rel;
long           old, new;
int                      idx;
{
   char   pba;
   long   lef, rig, par;
   cache *ptr;

   ptr = _read_cache (rel, old, idx);
      lef = ptr->left;
      rig = ptr->right;
      par = ptr->parent;
      pba = ptr->parbal;
   ptr = _read_cache (rel, new, idx);
      _change_cache (ptr, left,   lef);
      _changeqcache (ptr, right,  rig);
      _changeqcache (ptr, parent, par);
      _changeqcache (ptr, parbal, pba);

   if (par == 0L)   /* Parent */
      {
      ptr = _read_cache (rel, 0L, idx);
      _change_cache (ptr, num, new);
      }
   else
      {
      ptr = _read_cache (rel, par, idx);
      if (pba & PARDIR)  _changeqcache (ptr, right, new);
      else               _changeqcache (ptr, left,  new);
      ptr->changed = 1;
      }

   if (lef != 0L)  /* Left child */
      {
      ptr = _read_cache (rel, lef, idx);
      _change_cache (ptr, parent, new);
      }

   if (rig != 0L)  /* Right child */
      {
      ptr = _read_cache (rel, rig, idx);
      _change_cache (ptr, parent, new);
      }

   _zero (rel, old, idx);  /* Remove old record's pointers */
}

void
_zero    (rel, pos, idx)  /* CACHED */
relation *rel;
long           pos;
int                 idx;
{
   cache *ptr;
   ptr = _read_cache (rel, pos, idx);
   _change_cache (ptr, parbal, BAL_EV);
   _changeqcache (ptr, left,   0L);
   _changeqcache (ptr, right,  0L);
   _changeqcache (ptr, parent, 0L);
}

long
_find_ends (rel, idx, dir)  /* CACHED */
relation   *rel;
int              idx, dir;
{
   long    pos, tmp;
   cache  *ptr;

   _strobe (rel, 0);  /* Don't let anyone think we're dead */

   ptr = _read_cache (rel, 0L, idx);
   pos = ptr->num;

   if (pos == 0L)  return 0L;

   for (tmp = pos; ; pos = tmp)
      {
      ptr = _read_cache (rel, tmp, idx);
      tmp = _cache_field (ptr, dir);

      if (tmp == 0L)  return pos;
      }
}

long
_search  (rel, pos, idx, act, comp)  /* CACHED */
relation *rel;
long           pos;
int                 idx;
mb_action                act;
dataptr                       comp;
{
   long     x;
   int      r,  dir;
   dataptr  rec;
   cache   *ptr;

   if (pos == 0L)  return 0;

   _strobe (rel, 0);  /* Don't let anyone think we're dead */

   ptr = _read_cache (rel, pos, idx);

   dir = r = _compare (rel, comp, rec=_rec(rel,pos), idx);
   free (rec);

   if (dir == 0)
    { if (act == GTHN || act == LTEQ)                 dir =  1;
      if (act == GTEQ || act == LTHN || act == EQUL)  dir = -1;
    }

   if ((x = _search (rel, (dir==1)?ptr->right:ptr->left, idx,act,comp)) != 0L)
      return x;

   if  (act != GTHN && act != LTHN  && r    ==  0)  return pos;
   if ((act == GTEQ || act == GTHN) && dir  == -1)  return pos;
   if ((act == LTEQ || act == LTHN) && dir  ==  1)  return pos;

   return 0L;
}

long
_find_seq (rel, top, rcd, idx, dir)  /* CACHED */
relation  *rel;
long            top, rcd;
int                       idx, dir;
{
   char   temp[5];
   long   pos, tmp;
   cache *ptr;

   _strobe (rel, 0);  /* Don't let anyone think we're dead */

   ptr = _read_cache (rel, rcd, idx);
   pos = _cache_field (ptr, dir);

   if (pos == 0L)
      {
      if (rcd == top)  return 0L;      /* hit top=no sequential available */
      for (pos = rcd; ; pos = tmp)
         {
         ptr = _read_cache (rel, pos, idx);
         tmp = ptr->parent;

         if (tmp == top)  return 0L;    /* hit top=no sequential available */

         temp[0] = ptr->parbal;
         if (dir == ((temp[0] & PARDIR) ? -1 : 1))  return tmp;
         } 
      } 

   for (dir = 0-dir; ; pos = tmp)
      {
      ptr = _read_cache (rel, pos, idx);
      tmp = _cache_field (ptr, dir);
      if (tmp == 0L)  return pos;
      }
}

long
_delete  (rel, bad)  /* CACHED */
relation *rel;
long           bad;
{
   static   int  lastmove = -1;
   register int  i;
   long          bp, rep, rp;
   char          buf[5];
   int           bal;
   cache        *ptr;

   _free_cache ();

   for (i = 0; i < rel->num_i; i++)
      {
      ptr = _read_cache (rel, bad, i);
      bp     = ptr->parent;
      buf[0] = ptr->parbal;

      bal= buf[0] & BAL;

      if (bal != BAL_EV)
         rep = _find_seq (rel, bad, bad, i, NUM_BAL(bal));
      else
         if (! (rep = _find_seq (rel, bad, bad, i, togg (lastmove))))
            rep = _find_seq (rel, bad, bad, i, togg (lastmove));

      if (! rep)
         {
         _dislink (rel, bad, i, 0L);
         }
      else
         {
         ptr = _read_cache (rel, rep, i);
         rp = ptr->parent;

         _dislink (rel, rep, i, 0L);
         _replace (rel, bad, rep, i);
         if (rp != bad)
            if (_check (rel, rp, bp, i))
               {
               _flush_cache (rel, i);
               longerr (mb_errno, -1L);
               }
         }

      if (_check (rel, bp, 0L, i))
         {
         _flush_cache (rel, i);
         longerr (mb_errno, -1L);
         }

      _flush_cache (rel, i);
      }
}

int
_compare (rel, ptra, ptrb, idx)  /* -1 == ptra < ptrb */
relation *rel;
dataptr        ptra, ptrb;
int                        idx;
{
   register int  i;
   int           mx, n, p;
   char          temp[5];

   strzcpy (temp, rel->idxs[idx], 3);  mx = atoi (temp);

   for (i = 0; i < mx; i++)
      {
      strzcpy (temp, &rel->idxs[idx][3*i +3], 3);  p = atoi (temp);
      if ((n = _comp_fld (rel, ptra, ptrb, p)) != 0)  return n;
      }

   return 0;
}

void
_dumprec (rel, rec)  /* rec must not be encrypted */
relation *rel;
dataptr        rec;
{
   char          buf[128], temp[80], *p;
   register int  i;

   buf[0] = 0;
   for (i=0; i<rel->num_f; i++)
      {
      p=(char *)rec +rel->start[i];
      switch (rel->type[i])
         {
         case T_CHAR:    strzcpy (temp, p, rel->siz[i]);               break;
         case T_SHORT:   sprintf (temp,    "%d",  (int)*(short  *)p);  break;
         case T_USHORT:  sprintf (temp,    "%u",  (int)*(ushort *)p);  break;
         case T_LONG:    sprintf (temp,   "%ld", *(long   *)p);        break;
         case T_ULONG:   sprintf (temp,   "%lu", *(ulong  *)p);        break;
         case T_FLOAT:   sprintf (temp,    "%f", *(float  *)p);        break;
         case T_DOUBLE:  sprintf (temp,   "%lf", *(double *)p);        break;
         case T_MONEY:   sprintf (temp,"%-.2lf", *(double *)p);        break;
         case T_TIME:    sprintf (temp,   "%ld", *(long   *)p);        break;
         case T_DATE:    sprintf (temp,   "%ld", *(long   *)p);        break;
         case T_SERIAL:  sprintf (temp,   "%ld", *(long   *)p);        break;
         case T_PHONE:   strzcpy (temp, p, 20);                        break;
         }
      if (strlen (buf) + strlen (temp) > 126)  break;
      strcat (buf, temp);
      strcat (buf, "|");
      }

   printf ("%s\n", buf);
}

int
_comp_fld (rel, ptra, ptrb, fld)
relation  *rel;
dataptr         ptra, ptrb;
int                         fld;
{
   char  *a, *b;
   int    n;

   n = rel->siz[fld];

   _cryptf ((dataptr)(a = (char *)ptra +rel->start[fld]), n, rel->mask);
   _cryptf ((dataptr)(b = (char *)ptrb +rel->start[fld]), n, rel->mask);

   switch (rel->type[fld])
      {
      case T_SHORT:   n = _comp_short  ((short  *)a, (short  *)b);     break;
      case T_USHORT:  n = _comp_ushort ((ushort *)a, (ushort *)b);     break;
      case T_LONG:    n = _comp_long   ((long   *)a, (long   *)b);     break;
      case T_ULONG:   n = _comp_ulong  ((ulong  *)a, (ulong  *)b);     break;
      case T_FLOAT:   n = _comp_float  ((float  *)a, (float  *)b);     break;
      case T_DOUBLE:  n = _comp_double ((double *)a, (double *)b);     break;
      case T_MONEY:   n = _comp_double ((double *)a, (double *)b);     break;
      case T_TIME:    n = _comp_long   ((long   *)a, (long   *)b);     break;
      case T_DATE:    n = _comp_long   ((long   *)a, (long   *)b);     break;
      case T_SERIAL:  n = _comp_long   ((long   *)a, (long   *)b);     break;
      case T_PHONE:   n = _comp_string (a, b, n);  break;
      default:        n = _comp_string (a, b, n);  break;
      }

   _cryptf ((dataptr)a, rel->siz[fld], rel->mask);
   _cryptf ((dataptr)b, rel->siz[fld], rel->mask);

   return n;
}

int
_comp_short (a, b)
short       *a,*b;
{
   return ((*a < *b) ? -1 : (*a > *b) ? 1 : 0);
}

int
_comp_ushort (a, b)
ushort       *a,*b;
{
   return ((*a < *b) ? -1 : (*a > *b) ? 1 : 0);
}

int
_comp_long (a, b)
long       *a,*b;
{
   return ((*a < *b) ? -1 : (*a > *b) ? 1 : 0);
}

int
_comp_ulong (a, b)
ulong       *a,*b;
{
   return ((*a < *b) ? -1 : (*a > *b) ? 1 : 0);
}

int
_comp_float (a, b)
float       *a,*b;
{
   return ((*a < *b) ? -1 : (*a > *b) ? 1 : 0);
}

int
_comp_double (a, b)
double       *a,*b;
{
   return ((*a < *b) ? -1 : (*a > *b) ? 1 : 0);
}

int
_comp_string (a, b, n)
char         *a,*b;
int                 n;
{
   int  i;
   i = strncmp (a,b,n);            /* THIS WON'T RETURN -1,0,1!! It returns  */
   return (i < 0) ? -1 : (i > 0);  /* different amounts on different systems */
}

dataptr
_rec     (rel, rcd)
relation *rel;
long           rcd;
{
   dataptr a;
   a = (dataptr)malloc (1+ rel->rec_len);
   return _memrec (rel, rcd, a);
}

dataptr
_memrec  (rel, rcd, a)
relation *rel;
long           rcd;
dataptr             a;
{
   GO_RECID   (rel, rcd);
   readx      (rel->relcode, a, rel->rec_len);
   return a;
}

