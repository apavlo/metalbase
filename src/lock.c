/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#define UTIL_C
#include "mbase.h"
#include "internal.h"

#ifdef LONGARGS
   static mb_err     _set_hack  (relation *);
   static void       _clr_hack  (relation *);
   static void       _lck_pause (void);
   static int        _is_dead   (relation *, int);
   static void       _clrstrobe (relation *);
#else
   static mb_err     _set_hack();
   static void       _clr_hack();
   static void       _lck_pause();
   static int        _is_dead();
   static void       _clrstrobe();
#endif

/*****************************************************************************
 *
 * LOCK TIMING
 *
 */

static void
_lck_pause ()  /* Around .1 second pause, to reduce disk I/O */
{
   int  i, j = 1;
   for (i = 0; i < 40000; i++)  /* UGLY!  CHANGE THIS! */
      j = 1-j;
}

/*****************************************************************************
 *
 * EXCLUSIVE (RELATION-WIDE) LOCKS
 *
 */

mb_err           /* Fair warning:                                            */
_chk_elck (rel)  /* THIS ROUTINE CANNOT BE TRUSTED unless you have a         */
relation  *rel;  /* temporary lock placed on the relation before calling it! */
{
   ushort pid;
   if (rel->exc & 1)  baderr (MB_OKAY);  /* _we_ have the lock set */

   lseek (rel->lckcode, lckPOS_ELOCK, 0);
   readx (rel->lckcode, &pid, 2);
   if (! pid)  baderr (MB_OKAY);     /* or no one has the lock */

   baderr (MB_LOCKED);
}

mb_err
mb_unl   (rel)
relation *rel;
{
   ushort pid;
   if (_identify (rel) < 0)  reterr (MB_BAD_REL,  -1);
   if (! (rel->exc & 1))     reterr (MB_OKAY, MB_OKAY); /* We didn't lock it */

   if (_set_lck (rel) != MB_OKAY)  baderr (mb_errno);

   lseek (rel->lckcode, lckPOS_ELOCK, 0);
   readx (rel->lckcode, &pid,      2);

   if (pid == rel->pid)
      {
      pid = 0;
      lseek (rel->lckcode, lckPOS_ELOCK, 0);
      writx (rel->lckcode, &pid,      2);
      }

   rel->exc &= 2;  /* Clear the exclusive-lock bit */
   lckerr (rel, MB_OKAY, MB_OKAY);
}

mb_err
mb_lck   (rel)
relation *rel;
{
   ushort pid;
   if (_identify (rel) < 0)  reterr (MB_BAD_REL,  -1);
   if (rel->exc & 1)         baderr (MB_OKAY);  /* We've already locked it */

   if (_set_lck (rel) != MB_OKAY)  baderr (mb_errno);

   lseek (rel->lckcode, lckPOS_ELOCK, 0);
   readx (rel->lckcode, &pid, 2);
   if (pid != 0)  lckerr (rel, MB_LOCKED, -1);

   lseek (rel->lckcode, lckPOS_ELOCK, 0);
   writx (rel->lckcode, &rel->pid, 2);
   rel->exc |= 1;  /* Set the exclusive-lock bit */

   lckerr (rel, MB_OKAY, MB_OKAY);
}

/*****************************************************************************
 *
 * HACK LOCKS (CONCURRENCY CONTROL)
 *
 */

static void
_clr_hack (rel)
relation  *rel;
{
   ushort *pid;
   char    pids[6];

   lseek (rel->lckcode, lckPOS_HLOCK, 0);
   readx (rel->lckcode, pids, 6);

   if (*(pid = (ushort *)&pids[0]) == rel->pid)  *pid = 0L;
   if (*(pid = (ushort *)&pids[2]) == rel->pid)  *pid = 0L;
   if (*(pid = (ushort *)&pids[4]) == rel->pid)  *pid = 0L;

   lseek (rel->lckcode, lckPOS_HLOCK, 0);
   writx (rel->lckcode, pids, 6);
}

static mb_err
_set_hack (rel)
relation  *rel;
{
   ushort *pid;
   char    pids[6];
   int     fChange;
   mb_time timeStart;

   timeStart = curtime();

   for (;;)
      {
      if (elap_t (timeStart) > 5)
         {
         pids[0] = pids[1] = pids[2] = pids[3] = pids[4] = pids[5] = 0;
         lseek (rel->lckcode, lckPOS_HLOCK, 0);
         writx (rel->lckcode, pids, 6);
         timeStart = curtime();
         continue;
         }

/*
 * FIRST ITERATION:
 *
 */

      fChange = 0;
      lseek (rel->lckcode, lckPOS_HLOCK, 0);
      readx (rel->lckcode, pids, 6);

      if (*(pid = (ushort *)&pids[0]) == rel->pid) { *pid = 0; fChange |= 1; }
      if (*pid != 0)  fChange |= 2;
      if (*(pid = (ushort *)&pids[2]) == rel->pid) { *pid = 0; fChange |= 1; }
      if (*pid != 0)  fChange |= 2;
      if (*(pid = (ushort *)&pids[4]) == rel->pid) { *pid = 0; fChange |= 1; }
      if (*pid != 0)  fChange |= 2;

      if (! (fChange & 2))
         {
         *pid = rel->pid;  fChange |= 1;
         }

      if (fChange & 1)
         {
         lseek (rel->lckcode, lckPOS_HLOCK, 0);
         writx (rel->lckcode, pids, 6);
         }

      if (fChange & 2)
         {
         continue;
         }

/*
 * SECOND ITERATION:
 *
 */

      lseek (rel->lckcode, lckPOS_HLOCK, 0);
      readx (rel->lckcode, pids, 6);

      if (*(pid = (ushort *)&pids[0]) != 0)          continue; /* NOTE ORDER */
      if (*(pid = (ushort *)&pids[4]) != rel->pid)   continue; /* NOTE ORDER */
      if (*(pid = (ushort *)&pids[2]) != 0)          continue; /* NOTE ORDER */

      *pid = rel->pid;

      lseek (rel->lckcode, lckPOS_HLOCK, 0);
      writx (rel->lckcode, pids, 6);

/*
 * THIRD ITERATION:
 *
 */

      lseek (rel->lckcode, lckPOS_HLOCK, 0);
      readx (rel->lckcode, pids, 6);

      if (*(pid = (ushort *)&pids[4]) != rel->pid)   continue; /* NOTE ORDER */
      if (*(pid = (ushort *)&pids[2]) != rel->pid)   continue; /* NOTE ORDER */
      if (*(pid = (ushort *)&pids[0]) != 0)          continue; /* NOTE ORDER */

      *pid = rel->pid;

      lseek (rel->lckcode, lckPOS_HLOCK, 0);
      writx (rel->lckcode, pids, 6);

/*
 * FINAL CHECK:
 *
 */

      lseek (rel->lckcode, lckPOS_HLOCK, 0);
      readx (rel->lckcode, pids, 6);

      if (*(pid = (ushort *)&pids[4]) != rel->pid)   continue; /* NOTE ORDER */
      if (*(pid = (ushort *)&pids[2]) != rel->pid)   continue; /* NOTE ORDER */
      if (*(pid = (ushort *)&pids[0]) != rel->pid)   continue; /* NOTE ORDER */

      break;
      }

   return MB_OKAY;
}

/*****************************************************************************
 *
 * TEMPORARY LOCKS/ QUEUEING
 *
 */

mb_err
_set_lck (rel)
relation *rel;
{
   char    pids[60];
   ushort *pid, tpid;
   int     i, j;

/*
 * FLOW FOR GETTING   ( example queue:  12  13  14  00  19  22  00  00  00... )
 * INTO THE QUEUE:    (           pos:   0   1   2   3   4   5   6   7   8... )
 *
 *     set hacklock  -- This guarantees that only one process will try to get
 *                      into the queue at once--avoids race conditions.
 *
 * WAIT:
 *     pos = the first zero in the right-hand set of contiguous zeroes
 *           (position 6 in the example above)
 *     look for a queuehole (position 3 above): -- if we were to just set
 *        a lock when the queue has a hole in it, we'd possibly escalate
 *        the length of the queue, whereas if we wait a few seconds, it'll
 *        shrink itself (when process 19 wakes up and moves itself).
 *     if queuehole
 *        check strobe for our blocking process (pos 5, pid 22 in this case)
 *        if strobe hasn't changed and elapsed time > 3 seconds
 *           pos -= 1      -- move over the dead process and erase its hold on
 *           write PID, 0  -- the queue--try again and we'll start here.
 *        else
 *           pause         -- let other processes work without extra I/O
 *        goto WAIT        -- go check the queue for a hole again.
 *     if the queue's full (pos == 30 -- no free slots), return MB_BUSY
 *
 *     clear hacklock      -- we're assured this position in the queue now
 *
 */

   if (rel->exc & 2)  baderr (MB_OKAY);

   if (_set_hack (rel))   baderr (mb_errno);

   _clrstrobe (rel);

lockwait:

   lseek (rel->lckcode, lckPOS_QUEUE, 0);
   readx (rel->lckcode, pids, 60);

   for (i = 29; i >= 0; i--)
      if (*(pid = (ushort *)&pids[i*2]) != 0)
         break;
   i++;           /* "i" now == first available zero. */

   if (i != 0)    /* Check for a queuehole before taking the slot. */
      {
      for (j = i-1; j >= 0; j--)
         if (*(pid = (ushort *)&pids[j*2]) == 0)
            break;

      if (j != -1)  /* If this != -1, there's a 0 right here in the queue */
         {
         if (! _is_dead (rel, i-1))  /* If it's not dead, we expect it's     */
            {                   /* checking the guy before it, and so on--   */
            _lck_pause ();      /* and that eventually, someone will see the */
            _lck_pause ();      /* queuehole exists and will try to get it   */
            }                   /* filled.                                   */
         else
            {
            i--;       /* If it IS dead, though, move over it and try again. */
            tpid = 0;
            lseek (rel->lckcode, lckPOS_QUEUE +2*i, 0);
            writx (rel->lckcode, &tpid, 2);
            }
         goto lockwait; /* Look, GOTO was useful here, all right?  Sheesh... */
         }
      }
   if (i == 30)
      {
      _clr_hack (rel);
      baderr (MB_BUSY);
      }

   lseek (rel->lckcode, lckPOS_QUEUE +2*i, 0);
   writx (rel->lckcode, &rel->pid, 2);

   _clr_hack (rel);

/*
 * FLOW FOR WORKING OUR    ( example queue:   15  13  12  92  34  16  00... )
 * WAY UP THE QUEUE:       (           pos:    0   1   2   3   4   5   6... )
 *
 * (we're in slot #4, PID==34, in the example above):
 *
 * WAIT:
 *   If we're in slot 0, goto DONE
 *   Otherwise,
 *      Read pos OurPos-1 (#3)--check pid (92)
 *      If PID==0,                     -- The process that WAS there has moved,
 *      OR PID is dead                 -- or hasn't strobed in 3 seconds,
 *         Write our PID in that slot  -- move up over it.  This way, free
 *         Write zero in our last slot -- slots bubble upwards...
 *         Goto WAIT
 *      Strobe our position
 *      Goto WAIT
 *
 * DONE:
 *   We're finished, and a temporary lock is in place.  Make sure to strobe
 *   every second during operations, or you'll lose your lock.
 *
 */

   _clrstrobe (rel);

   for (j = 0; i > 0; j++)
      {
      lseek (rel->lckcode, lckPOS_QUEUE +2*(i-1), 0);
      readx (rel->lckcode, &tpid, 2);
      if (tpid == 0 || _is_dead (rel, i-1))
         {
         i--;
         tpid = 0;
         lseek (rel->lckcode, lckPOS_QUEUE +2*i, 0);
         writx (rel->lckcode, &rel->pid, 2);
         writx (rel->lckcode, &tpid,     2);
         continue;
         }

      _strobe (rel, i);  /* Don't let anyone think we're dead, but let */
      _lck_pause ();     /* other processes think for a second without */
      continue;          /* tons of I/O slowing everything down.       */
      }

   rel->exc |= 2;

   baderr (MB_OKAY);
}

mb_err
_clr_lck (rel)
relation *rel;
{
   ushort tpid = 0;

   if (! (rel->exc & 2))  baderr (MB_OKAY);

   rel->exc &= 1;  /* Clear the temp lock bit */

   lseek (rel->lckcode, lckPOS_QUEUE, 0);
   writx (rel->lckcode, &tpid, 2);

   return MB_OKAY;
}

static int
_is_dead (rel, pos)
relation *rel;
int            pos;
{
   char    newstrobe[30];   /* Values just read from lockfile         */
   mb_time cur;             /* Current time (reduces curtime() calls) */
   int     i;

   cur = curtime();

/*
 * If you have lots of frequently-dying processes, you may want to change the
 * thing below to "#if 0"--that way, you can detect ALL processes dying in
 * exactly three seconds instead of three sec per process--does a bit more
 * I/O, though.
 *
 */

#if 1
   i = pos;

   lseek (rel->lckcode, lckPOS_STROBE +2*i, 0);    /* Get just this one */
   readx (rel->lckcode, &newstrobe[i], 1);         /* position's strobe */
#else
   lseek (rel->lckcode, lckPOS_STROBE, 0);  /* First, we read all thirty */
   readx (rel->lckcode, newstrobe, 30);     /* strobes into an array.    */

   for (i = 0; i < 30; i++)                /* For each strobe, check if the  */
#endif
      if (rel->strobe[i] != newstrobe[i])  /* value's changed--if so, update */
         rel->times[i] = cur;              /* times[] array to current time. */

/*
 * Note: elap_t() will fail at midnight--it'll return a BIG negative number,
 *       which won't pass the IsItDead test below.  So it may take 6 seconds
 *       to detect if a process is dead, if successive checks occur right then.
 *
 * Now why 10 seconds?
 *    1-second granularity means two seconds are minimum.
 *    Previous value == current value adds two seconds for trigger (strobing
 *       process won't change it, even if it expects to--and won't try again
 *       for 1 sec, plus granularity safeguard).
 *    6-second safeguard (just to be SURE, 'cause it's a Bad Thing to be
 *       trigger happy, and a one-time timeout isn't worth fussing over).
 *
 */

   return (elap_t (rel->times[pos]) > 10);  /* If not changed yet, dead. */
}

void
_strobe  (rel, pos)
relation *rel;
int            pos;
{
   if (elap_t (rel->times[pos]) >= 1)
      {
      lseek (rel->lckcode, lckPOS_STROBE +pos, 0);
      rel->strobe[pos] = (char)( ((int)rel->strobe[pos] +1) % 255 );
      writx (rel->lckcode, &rel->strobe[pos], 1);
      rel->times[pos] = curtime();
      }
}

static void
_clrstrobe (rel)
relation   *rel;
{
   int     i;
   mb_time cur;
   cur = curtime();
   for (i = 0; i < 30; i++)
      rel->times[i] = curtime();
}

