/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#include <mbase.h>
#include "bench.h"             /* Created during "% build bench.s"  */

#define NUM 10 /* When building chart, each '.' == how many adds? */

#ifdef LONGARGS
   void  main (int, char **);
#else
   void  main ();
#endif

relation  *rel;

long   elaparr[79];

void
main  (argc, argv)
int    argc;
char **argv;
{
   int     i, num, x, top;
   long    sec;
   mb_time start;

/*
 * First, parse the command line, and set {num}.
 *
 */

   if (argc == 1)
      num = 0;
   else if (argc == 2)
      num = (int)atoi (argv[1]);
   else
      {
      fprintf (stderr, "format: bench [numberofadds]%s", SNGCR);
      mb_exit (1);
      }
   if (num < 0)
      {
      fprintf (stderr, "You gotta add at least one record, idiot.%s", SNGCR);
      mb_exit (1);
      }

/*
 * Great.  Now open the database and start it rolling...
 *
 */

   if ((rel = mb_inc ("bench", 0)) == RNULL)
      {
      fprintf (stderr, "%s.%s", mb_error, SNGCR);
      mb_exit (2);
      }

   start = curtime();

   if (num != 0)
      {
      printf ("Number of records: %ld%s", mb_num(rel), SNGCR);
      printf ("Start time:        %s%s", fmt_time (start, 0), SNGCR);
      }

   for (x = 0; x < 79; x++)
      {
      top = num ? num : NUM;
      for (i = 0; i < top; i++)
         {
         if ((mb_add (rel, &bench_rec)) != MB_OKAY)
            {
            printf ("ADD FAILED: %s%s", mb_error, SNGCR);
            mb_exit(3);
            }
         }
      if (! num)
         fprintf (stderr, ".");

      sec = elap_t (start);

      if (num != 0)  break;

      elaparr[x] = sec;
      }

/*
 * All done.  Print the tally.
 *
 */

   if (! num)  printf ("\n");
   printf ("Number of records: %ld%s", mb_num (rel), SNGCR);
   if (num)
      {
      printf ("End time:          %s%s", fmt_time (curtime(), 0), SNGCR);

      if (i != 0)
         {
         printf ("Seconds per add:   %.2lf%s", (double)sec/(double)i, SNGCR);
         }
      }
   else if (elaparr[78] != 0)
      {
      printf ("Average seconds per add = %.2lf:\n",
              ((double)elaparr[78]/(double)(NUM*79)));
      for (x = 19; x >= 0; x--)
         {
         for (i = 0; i < 79; i++)
            {
            if ( ((elaparr[i] * 20) / elaparr[78]) >= x )
               printf ("X");
            else
               printf (" ");
            }
         printf ("\n");
         }
      }

   mb_exit (0);
}

