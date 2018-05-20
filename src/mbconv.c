/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#include "mbase.h"
#include "internal.h"  /* Because we do some non-standard record moving */

#ifdef LONGARGS
   void   main       (int,        char **);
   mb_err convert    (relation *, relation *);
   void   finalize   (char *,     char *);
   int    renamefile (char *,     char *);
#else
   void   main();
   mb_err convert();
   void   finalize();
   int    renamefile();
#endif

relation *old;
relation *new;

char    tempname[128];

void
main  (argc, argv)
int    argc;
char **argv;
{
   char *str;
   int   done = 0;

   for (--argc,++argv; argc; --argc,++argv)
      {
      if (*(str = *argv) == '-')
         {
         fprintf (stderr, "mbconv: option %s unrecognized.%s", 1+str, SNGCR);
         continue;
         }

      done = 1;

      if ((old = mb_old (str, 0)) == RNULL)
         {
         fprintf (stderr, "mbconv: %s: %s.%s", str, mb_error, SNGCR);
         continue;
         }

      if (old->ver == verCURRENT)
         {
         fprintf (stderr, "mbconv: %s is already in 5.0 format.%s", str,SNGCR);
         MB_RemoveRelation (old);
         continue;
         }

      if ((new = mb_new ()) == RNULL)
         {
         fprintf (stderr, "mbconv: %s.%s", mb_error, SNGCR);
         mb_die();
         continue;
         }

      strcpy (tempname, str);
      if (! strncmp (&tempname[strlen(tempname)-4], ".rel", 4))
         {
         tempname[strlen(tempname)-4] = 0;
         }
      strcat (tempname, ".tmp");

      if (convert (new, old) == MB_OKAY)
         {
         finalize (tempname, str);  /* Removes original and renames new */
         }
      else
         {
         fprintf (stderr, "mbconv: %s.%s", mb_error, SNGCR);
         }

                   /* BECAUSE WE USED MB_NEW(), FREE IT. DON'T DO THIS */
      free (new);  /* WITH ANY OTHER RELATION!!!  mb_die() won't free  */
                   /* the memory for a relation made with mb_new().    */

      MB_RemoveRelation (old);  /* Maybe it's still open, maybe not. */
      }

   if (! done)
      {
      fprintf (stderr, "format: mbconv oldrelation [oldrelation...]%s", SNGCR);
      mb_exit (1);
      }

   mb_exit (0);
}

mb_err
convert  (new,  old)
relation *new, *old;
{
   long    nexts, numrec, arg;
   char    desc[128], temp[5], t2[5];
   char   *ptr;
   int     i, j, n, len;

   lseek (old->relcode, 12L +POS_FIELDPTR(old->ver), 0);
   readx (old->relcode, &numrec, 4);
   readx (old->relcode, &nexts,  4);

/*
 * First, the fields...
 *
 */

   for (i = 0; i < old->num_f; i++)
      {
      arg = 0L;

      if (old->type[i] == T_SERIAL)    arg = nexts;
      else
         if (old->type[i] == T_CHAR)   arg = old->siz[i];

      if (mb_addfield (new, old->name[i], old->type[i], arg) != MB_OKAY)
         return mb_errno;
      }

/*
 * Next, the indices...
 *
 */

   for (i = 0; i < old->num_i; i++)
      {
      desc[0] = 0;
      strzcpy (temp, old->idxs[i], 3);
      n = atoi (temp);

      for (j = 0; j < n; j++)
         {
         strzcpy (temp, &old->idxs[i][3+ j*3], 3);
         sprintf (t2, "%d", (int)atoi (temp));

         if (j != 0)  strcat (desc, ",");
         strcat (desc, t2);
         }

      if (mb_addindex (new, old->iname[i], old->itype[i], desc) != MB_OKAY)
         return mb_errno;
      }

/*
 * Now create it, and open the resulting file...
 *
 */

   if (mb_create (new, tempname, 0) != MB_OKAY)
      {
      unlink (tempname);
      return mb_errno;
      }

   if ((new->relcode = openx (tempname, OPENMODE)) <= 0)
      {
      unlink (tempname);
      baderr (MB_NO_READ);
      }

/*
 * The number of records is reset to zero inside the new relation, so since
 * we read it from the old relation earlier, write it out where it needs to
 * be (see why I had to include internal.h?).  Oh, and grab a buffer big
 * enough to move an entire record, with indices intact...
 *
 */

   lseek (new->relcode, 12L +POS_FIELDPTR(verCURRENT), 0);
   writx (new->relcode, &numrec, 4);

   len = (int)(old->rec_len +13L*(old->num_i));

   if ((ptr = (char *)malloc (len +1)) == NULL)
      {
      close (new->relcode);
      unlink (tempname);
      baderr (MB_NO_MEMORY);
      }

/*
 * Great.  Problem is, the new header is bigger than older versions... so
 * read each record, and write it out at the new place in the new relation.
 * Record numbers are offsets relative to ->recz, so they won't have to change
 * this way.
 *
 * We also have to initialize the top-of-index pointers, which aren't set
 * by mb_create() (obviously).
 *
 */

   lseek (old->relcode, 24+ POS_FIELDPTR(old->ver),   0);
   lseek (new->relcode, 24+ POS_FIELDPTR(verCURRENT), 0);

   for (i = 0; i < old->num_i; i++)
      {
      readx (old->relcode, &arg, 4);
      writx (new->relcode, &arg, 4);
      }


   lseek (old->relcode, old->recz, 0);
   lseek (new->relcode, new->recz, 0);

   for (arg = 0L; arg < numrec; arg++)  /* arg == which number we're moving */
      {
      if ((readx (old->relcode, ptr, len)) != len)
         {
         free (ptr);
         close (new->relcode);
         unlink (tempname);
         baderr (MB_CORRUPT);
         }
      if ((writx (new->relcode, ptr, len)) != len)
         {
         free (ptr);
         close (new->relcode);
         unlink (tempname);
         baderr (MB_DISKFULL);
         }
      }

   free (ptr);
   close (new->relcode);     /* Close this filehandle.  */
   MB_RemoveRelation (old);  /* Unnecessary now.        */
   baderr (MB_OKAY);
}

int
renamefile (new, old)
char       *new,*old;
{
#ifdef MSDOS
   return rename(old,new);
#else
   if (link (old, new) != 0)  return -1;
   if (unlink (old) != 0)     return -2;
   return 0;
#endif
}

void
finalize (newname, oldname)
char     *newname,*oldname;
{
   long  sizea, sizeb;
   int   fh;

/*
 * If we got here, convert() already closed all file pointers, so we can
 * do this safely.  First come the sanity checks--make sure we can open
 * and read/write both files, and make sure the new file is larger than
 * the original (if it isn't, we didn't finish converting, regardless of
 * what convert() said).
 *
 */

   if (strncmp (&oldname[strlen(oldname)-4], ".rel", 4))
      {
      strcat (oldname, ".rel");
      }

   if ((fh = openx (oldname, OPENMODE)) < 0)
      {
      fprintf (stderr, "mb_conv: could not open %s.%s", oldname, SNGCR);
      return;
      }
   sizea = lseek (fh, 0L, 2);  /* Find the filesize */
   close (fh);

   if ((fh = openx (newname, OPENMODE)) < 0)
      {
      fprintf (stderr, "mb_conv: could not open %s.%s", oldname, SNGCR);
      return;
      }
   sizeb = lseek (fh, 0L, 2);  /* Find the filesize */
   close (fh);

   if (sizea >= sizeb)
      {
      fprintf (stderr, "mb_conv: could not finish conversion!%s", SNGCR);
      return;
      }

/*
 * Fine--looks like we converted it just dandy.  So delete the original
 * file, and rename our temporary one so it looks like the old one.  Note that
 * rename() for DOS doesn't exist with almost any *nix compiler, so I use my
 * own renamefile(); args work like *nix MV would (new=first, old=second).
 *
 */

   printf ("mb_conv: %s converted to 5.0 successfully.%s", oldname, SNGCR);

   unlink (oldname);
   if (renamefile (oldname, newname) != 0)
      {
      fprintf (stderr, "But, the rename didn't work.%s", SNGCR);
      fprintf (stderr, "Rename %s to %s yourself.%s", newname, oldname, SNGCR);
      }
}

