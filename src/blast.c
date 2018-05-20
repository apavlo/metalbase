/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#include "stdinc.h"

#ifndef linux
#ifndef MSDOS
#ifdef LONGARGS
   extern char *getenv (char _FAR_ *);
#else
   extern char *getenv();
#endif
#endif
#endif

void
main  (argc, argv)
int    argc;
char **argv;
{
   char  buf[128], name[128], *pch;
   int   i;

   if (argc == 1)
      {
      fprintf (stderr, "format: blast relation [relation]*\n");
      exit (1);
      }

   for (--argc, argv++; argc; argc--, argv++)
      {
      if ((*argv)[0] == '-')
         {
         fprintf (stderr, "blast: unrecognized switch -%c\n", (*argv)[1]);
         exit (2);
         }

      if (access (*argv, 0) == -1)
         {
         fprintf (stderr, "blast: cannot find relation %s\n", *argv);
         continue;
         }

/*
 * First, get the basename for the relation (take off the path and/or
 * extension)--put it in buf:
 *
 */

      name[0] = 0;

      if ((pch = strrchr (*argv, DIRSEP)) == NULL)
         pch = *argv;
      else
         pch++;
      strcpy (buf, pch);

      if (! strcmp (buf, ".rel") || ! strcmp (buf, ".REL"))
         {
         *(strrchr (buf, '.')) = 0;
         }

/*
 * Great.  Now get the temporary directory where the lockfile will be...
 *
 */

      if ((pch = getenv ("TMP")) != NULL || (pch = getenv ("TEMP")) != NULL)
         {
         strcpy (name, pch);  /* If they define a directory, use it. */
         }
      else                   /* Otherwise, try to guess a default directory. */
         {
#ifdef UNIX
         strcpy (name, "/tmp");
#endif
         }
      if (! name[0])
         {
         fputs ("blast: you must have a TMP directory defined.\n", stderr);
         exit (3);
         }
      if (name[(i = strlen(name))-1] != DIRSEP)
         {
         name[i] = DIRSEP;
         name[i+1] = 0;
         }

/*
 * And attach the filename + .LCK to get a lockfile name.  Then delete the
 * stupid thing.
 *
 */

      strcat (name, buf);
      strcat (name, ".lck");

      if (access (name, 0) == -1)
         {
         fprintf (stderr, "blast: %s was not locked\n", *argv);
         continue;
         }

      unlink (name);

      fprintf (stderr, "%s blasted successfully\n", *argv);
      }
}

