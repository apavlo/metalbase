/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#define UTIL_C
#include "mbase.h"
#include "internal.h"

cache  mb_cache[MAX_CACHE];
cache  mb_ctop;
int    ncache = 0;
cache *mb_clast = NULL;
long   mb_nlast = 0L;

cache *mb_hash[MAX_CACHE];  /* Attempted */

/****************************************************************************/

cache *
_read_cache (rel, rcd, idx)
relation    *rel;
long              rcd;
int                    idx;
{
   cache *ptr, *end;
   long   n;

   end = &mb_cache[ncache];  /* Can't check when ptr==end */

   if (mb_nlast == rcd && mb_clast != NULL)  /* Now in practice, we seem to */
      {                                      /* query the same one twice    */
      return mb_clast;                       /* in a row a lot, so here's   */
      }                                      /* a hack to optimize for it.  */

   mb_nlast = rcd;

   if (rcd == 0L)
      {
      if (mb_ctop.num != 0L)
         {
         return (mb_clast = &mb_ctop);
         }

      GO_TOP (rel, idx);
      readx  (rel->relcode, &(mb_ctop.num), 4);
      mb_ctop.changed = 0;
      return (mb_clast = &mb_ctop);
      }

   if ((ptr = mb_hash[ n=(rcd % MAX_CACHE) ]) != NULL && ptr->num == rcd)
      {
      return (mb_clast = ptr);
      }

   for (ptr = mb_cache; ptr < end; ptr++)
      if (ptr->num == rcd)
         {
         return (mb_clast = mb_hash[n] = ptr);
         }

   ptr = _new_cache (rel, idx);

   GO_INDEX (rel, rcd, idx);
   readx (rel->relcode, &(ptr->left),   4);
   readx (rel->relcode, &(ptr->right),  4);
   readx (rel->relcode, &(ptr->parent), 4);
   readx (rel->relcode, &(ptr->parbal), 1);

   ptr->num     = rcd;
   ptr->changed = 0;

   return (mb_clast = mb_hash[n] = ptr);
}

void
_flush_cache (rel, idx)
relation     *rel;
int                idx;
{
   cache *ptr, *end;

   if (mb_ctop.changed)
      {
      GO_TOP (rel, idx);
      writx  (rel->relcode, &(mb_ctop.num), 4);
      }

   end = &mb_cache[ncache];  /* Can't check when ptr==end */

   for (ptr = mb_cache; ptr < end; ptr++)
      if (ptr->changed)
         {
         GO_INDEX (rel, ptr->num, idx);
         writx (rel->relcode, &(ptr->left),   4);
         writx (rel->relcode, &(ptr->right),  4);
         writx (rel->relcode, &(ptr->parent), 4);
         writx (rel->relcode, &(ptr->parbal), 1);
         }

   _free_cache ();
}

cache *
_new_cache (rel, idx)
relation   *rel;
int              idx;
{
   if (ncache >= MAX_CACHE) /* If this happens, expand cache for performance */
      {
      _flush_cache (rel, idx);
      }
   ncache ++;

   return &mb_cache[ncache-1];
}

void
_free_cache ()
{
   register int  i;
   mb_clast = NULL;  /* If we free the cache, it invalidates all cache*'s. */
   ncache = 0;
   for (i = 0; i < MAX_CACHE; i++)
      mb_hash[i] = NULL;
   mb_ctop.num = 0L;
   mb_ctop.changed = 0;
}

