/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 *
 * Special thanks go to Mike Cuddy (mcuddy@fensende.rational.com) for his
 * suggestions and code.
 *
 */

#define BLAST_C  /* I know, I know... */
#include "mbase.h"
#include "internal.h"

#define cr(x) ((x) == 0) ? DUBCR : SNGCR

#ifdef MSDOS
#define DESCLINE "/*\r\n * This file was created by MetalBase version 5.0 to reflect the structure\r\n * of the relation \"%s\".\r\n *\r\n * MetalBase 5.0 released October 1st, 1992 by richid@owlnet.rice.edu\r\n *\r\n */\r\n\r\ntypedef struct\r\n { "
#else
#define DESCLINE "/*\n * This file was created by MetalBase version 5.0 to reflect the structure\n * of the relation \"%s\".\n *\n * MetalBase 5.0 released October 1st, 1992 by virtual!richid@owlnet.rice.edu\n *\n */\n\ntypedef struct\n { "
#endif

#define lineF "Fields______________________________________________________%s"
#define lineI "\nIndices_____________________________________________________%s"

#define RBC '}'

#ifdef LONGARGS
   void   strlwrcpy (char *, char *);
   void   struprcpy (char *, char *);
   void   strmax    (char *, int);
   char  *repeat    (char,   int);
   void   main      (int,    char **);
   void   endoffile (int,    int);
   int    get_names (int,    char **);
   void   write_it  (int,    int);
   int    contains_serial (char *);
#else
   void   strlwrcpy();
   void   struprcpy();
   void   strmax();
   char  *repeat();
   void   main();
   void   endoffile();
   int    get_names();
   void   write_it();
   int    contains_serial();
#endif

#define  Printf   if (!quiet)  printf
#define fPrintf   if (!quiet) fprintf

#define qt(x) (quiet ? "" : x)

#define usage() \
   fprintf (stderr, "build: format: build [-q] [-h] schema.s%s", SNGCR);

#define fatal() { \
                fflush(stdout); \
                fprintf(stderr,"Cannot build relation--%s.%s",mb_error,SNGCR); \
                break; \
                }

#define comment() skip(fh,";"); while (skip (fh, "#"))  goeol(fh,NULL);

/*
 ******************************************************************************
 *
 */

static char *types[] =
 { "char *", "short", "ushort", "long", "ulong",  "float",
   "double", "money", "time",   "date", "serial", "phone"  };

/*
 ******************************************************************************
 *
 */

relation *data;

char   strname[40] = "";     /* Structure name (set by "typedef") */
char   rel[128],  hdr[128];  /* Filenames for relation and header */
char   names[20], nameb[40]; /* Name, and upper-case name         */
int    column=1;             /* Column we're displaying data in   */
int    header=0,  quiet=0;   /* Set by -q and -h on command-line  */
int    num_f=0,   num_i=0;   /* Start with 0 fields and 0 indices */
int    hasser=0;             /* 1 if we encounter a serial field  */


/*
 ******************************************************************************
 *
 */

void
main  (argc, argv)
int    argc;
char **argv;
{
   int        stage;     /* Processing stage; 1==fields, 2==indices, 3==done */
   int        fh;        /* File handle for schema                           */
   char       name[20];  /* Field/Index name                                 */
   ftype      typ;       /* Field type (or, for indices, 0==nodups, 1==dups) */
   int        siz;       /* Field size (for character arrays only)           */ 
   int        isExt;     /* TRUE if it's an external type, FALSE if not      */
   char       desc[128]; /* Character array of field numbers, for indices    */

   long       nexts = 0L;
   char       temp[128];
   char       t2[128];
   int        i;


   fh = get_names (argc, argv);  /* fh = file handle of relation */

   if ((data = mb_new()) == RNULL)
      {
      fprintf (stderr, "Cannot build relation--%s.%s", mb_error, SNGCR);
      exit(1);
      }

   for (stage = 1; stage != 3; )
      {
      strlwrcpy (temp, getword (fh));   /* temp = keyword */

      if (! strcmp (temp, "field"))
         {
         if (stage == 2)  /* Done with fields? */
            {
            fflush (stdout);
            fprintf (stderr, "%s%sField %s declared after indices.%s",
                     qt(SNGCR), qt(cr(column)), getword(fh), SNGCR);
            break;
            }

         strlwrcpy (temp, getword (fh));  /* New field?  Obtain, in lower,  */
         strmax (temp, 20);            /* its name.  Put it in 'temp' first */
         strcpy (name, temp);          /* in case it's really long.         */

         if (mb_getname (data, name, 0) != -1)
            {
            fflush  (stdout);
            fprintf (stderr, "%sField %s declared twice.%s", qt(cr(column)),
                             name, SNGCR);
            break;
            }

         (void)skip (fh, "type");         /* Got its name, and it's new.  So */
         strlwrcpy (temp, getword (fh));  /* get its field type...           */

         isExt = 0;
         if (! strcmp (temp, "extern") || ! strcmp (temp, "external"))
            {
            isExt = 1;
            strlwrcpy (temp, getword (fh));  /* External?  Get the next word. */
            }

         typ = (ftype)-1;
         if (! strcmp (temp, "char") || ! strcmp (temp, "character") ||
             ! strcmp (temp, "string"))
            {
            typ = T_CHAR;
            }
         if (! strcmp (temp, "short"))    typ = T_SHORT;
         if (! strcmp (temp, "ushort"))   typ = T_USHORT;
         if (! strcmp (temp, "long"))     typ = T_LONG;
         if (! strcmp (temp, "ulong"))    typ = T_ULONG;
         if (! strcmp (temp, "float"))    typ = T_FLOAT;
         if (! strcmp (temp, "double"))   typ = T_DOUBLE;
         if (! strcmp (temp, "money"))    typ = T_MONEY;
         if (! strcmp (temp, "time"))     typ = T_TIME;
         if (! strcmp (temp, "date"))     typ = T_DATE;
         if (! strcmp (temp, "serial"))   typ = T_SERIAL;
         if (! strcmp (temp, "phone"))    typ = T_PHONE;

         if (typ == (ftype)-1)
            {
            fflush  (stdout);
            fprintf (stderr, "%sType %s (field %s) undefined.%s",
                             qt(cr(column)), temp, name, SNGCR);
            break;
            }

         if (isExt)
            {
            sprintf (temp, "ix_%s", name);
            sprintf (desc, "%d",    num_i);

            if (mb_addindex (data, temp, 1, desc) != MB_OKAY)
               fatal();

            if (typ == T_SERIAL)
               typ = T_LONG;
            }

         if (typ == T_SERIAL)
            {
            if (hasser)
               {
               fflush  (stdout);
               fprintf (stderr, "%sMore than one serial field specified.%s",
                        qt(cr (column)), SNGCR);
               break;
               }
            hasser = 1;

            if (skip (fh, "start"))
               nexts = atol (getword (fh));
            }

         switch (typ)
            {
            case T_CHAR:
               (void)skip (fh, "length");
               (void)skip (fh, "*");
               siz = atoi (getword(fh));
               sprintf (temp, "%s [%s%d]", name, types[(int)typ], siz);
               mb_addfield (data, name, T_CHAR, siz);
               break;

            case T_SERIAL:
               sprintf (temp, "%s [%s @%ld]", name, types[(int)typ], nexts);
               mb_addfield (data, name, T_SERIAL, nexts);
               break;

            default:
               sprintf (temp, "%s [%s]", name, types[(int)typ]);
               mb_addfield (data, name, typ, 0);
               break;
            }

         if (mb_errno)
            fatal();

         if ((column = 1-column) == 0)
            { Printf ("%s%-30.30s%s", SUBD, temp, NORM); }
         else
            { Printf ("%s%s%s%s",     SUBD, temp, NORM, SNGCR); }

         num_f ++;

         comment();

         continue;
         }

      if (strcmp (temp, "index") == 0)
         {
         if (stage == 1)
            {
            if (column == 0)
               Printf (SNGCR);

            if (num_f == 0)
               {
               fflush  (stdout);
               fprintf (stderr, "%sNo fields declared before indices.%s",
                                 qt(SNGCR), SNGCR);
               break;
               }

            Printf (lineI, SNGCR);

            stage  = 2;
            column = 1;
            }

         strlwrcpy (temp, getword (fh));  /* New index?  Get the name (in   */
         strmax (temp, 20);            /* temp first in case it's long) and */
         strcpy (name, temp);          /* make sure it's unique.            */

         if (mb_getname (data, name, 1) != -1)
            {
            fflush  (stdout);
            fprintf (stderr, "%sField %s declared twice.%s", qt(cr(column)),
                             name, SNGCR);
            break;
            }

         (void)skip (fh, "on");

         for (temp[0] = desc[0] = 0; ; )
            {
            strlwrcpy (t2, getword (fh));

            if ((i = mb_getname (data, t2, 0)) == -1)
               {
               fflush  (stdout);
               fprintf (stderr, "%sIndex placed on undeclared field %s.%s",
                                qt(cr(column)), t2, SNGCR);
               exit (1);
               }

            strcat  (temp, t2);
            sprintf (t2, "%d", i);
            strcat  (desc, t2);

            if (! skip (fh, ","))
               break;

            strcat (temp, ",");
            strcat (desc, ",");
            }

         Printf ("%s%s", name, repeat ('.', 15-strlen (name)));
         Printf ("%s%s", temp, repeat ('.', 22-strlen (temp)));

         typ = (ftype)0;

         if (skip (fh, "without"))
            {
            if (skip (fh, "duplicates") || skip (fh, "dups"))
               typ = (ftype)0;
            else
               typ = (ftype)2;
            }
         else if (skip (fh, "with"))
            {
            if (skip (fh, "duplicates") || skip (fh, "dups"))
               typ = (ftype)1;
            else
               typ = (ftype)2;
            }
         if (typ == (ftype)2)
            {
            fflush  (stdout);
            fprintf (stderr, "?%sIncorrect syntax%s", qt(DUBCR), SNGCR);
            exit    (1);
            }

         if ((int)typ)  { Printf ("Duplicates allowed%s", SNGCR);     }
         else           { Printf ("Duplicates not allowed%s", SNGCR); }

         if (contains_serial (desc))
            typ = (ftype)1;

         if (mb_addindex (data, name, (int)typ, desc) != MB_OKAY)
            {
            fatal();
            }

         num_i ++;

         comment();

         continue;
         }

      if (strcmp (temp, "end") == 0 || temp[0] == 0)
         {
         Printf ("%s", cr (column));
         endoffile (num_f, num_i);
         stage = 3;

         continue;
         }

      if (! strcmp (temp, "typedef"))
         {
         strlwrcpy (strname, getword (fh));
         continue;
         }

      fflush  (stdout);
      fprintf (stderr, "%sIdentifier %s%s%s not recognized.%s",
                       qt(cr(column)), BOLD, temp, NORM, SNGCR);
      exit    (1);
      }

   if (stage != 3)
      {
      exit (1);
      }

   write_it (num_i, num_f);

   Printf ("Relation created -- zero entries.%s", SNGCR);

   exit (0);
}

void
write_it (num_i, num_f)
int       num_i, num_f;
{
   char  temp[512], temp2[30];
   int   R, H;
   int   i, j;


   if ((R = openx (rel, OPENMODE)) != -1)
      {
      if (read (R, temp, 1) != -1)
         {
         if (temp[0] != 50 && temp[0] != 42)  /* Check for 4.1a or 5.0 sig */
            {
            fPrintf (stderr, "%s%s%s%32.32s%-28.28s%s%s", SNGCR, SUBD, INVR,
                             "*** ERR", "OR ***", NORM, SNGCR);
            fprintf (stderr,
                     "%s   This relation is not in MetalBase 5.0 format.%s",
                     qt(SNGCR), DUBCR);
            close (R);
            exit (1);
            }
         }

      Printf ("%s%s%32.32s%-28.28s%s%s", SUBD, INVR, "** WARN", "ING **", NORM,
                                         SNGCR);
      Printf ("%s   The file about to be created already exists under the%s",
               SNGCR, SNGCR);
      Printf ("         target directory!  This data will be lost!%s", DUBCR);

      close (R);
      }

/*
 * That was ugly.  Now make sure they wanna continue first...
 *
 */

   if (! quiet)
      {
      Printf ("Continue with the creation of the relation [Y/n] ? ");
      gets(temp);  i = (int)temp[0];
      if (i == 'n' || i == 'N' || i == 'q' || i == 'Q')  exit (0);
      }

   if (header || quiet)
      {
      i = (header ? 'y' : 'n');
      }
   else
      {
      Printf ("Create header file for this relation       [y/N] ? ");
      fflush(stdin);  gets(temp);  i = (int)temp[0];
      }
   Printf (SNGCR);

/*
 * That was uglier.  At any rate, we now have permission to create the thing:
 *
 */

   if (mb_create (data, rel, 0) != MB_OKAY)
      {
      fflush(stdout);
      fprintf (stderr, "Cannot build relation--%s%s.", mb_error, SNGCR);
      return;
      }

/*
 * Now if they want the header created, we've gotta do all kindsa special shit:
 *
 */

   if (i != 'y' && i != 'Y')
      {
      return;
      }

   if ((H = openx (hdr, O_RDWR)) != -1)
      {
      close  (H);
      unlink (hdr);
      }
   if ((H = creatx (hdr)) == -1)
      {
      fprintf (stderr, "%sSorry--cannot create header file%s", qt(DUBCR),
               SNGCR);
      return;
      }
   modex (hdr, 0666);   /* Make the file   -rw-rw-rw-  */

   sprintf (temp, "#ifndef %s_H%s", nameb, SNGCR);
   writx   (H, temp, strlen(temp));
   sprintf (temp, "#define %s_H%s", nameb, DUBCR);
   writx   (H, temp, strlen(temp));
   sprintf (temp, DESCLINE, names);
   writx   (H, temp, strlen(temp));

   for (j = 0; j < data->num_f; j++)
      {
      switch (data->type[j])
         {
         case T_CHAR:   sprintf (temp, "char     %s[%d];",
                                        data->name[j], data->siz[j]);   break;
         case T_SHORT:  sprintf (temp, "short    %s;", data->name[j]);  break;
         case T_USHORT: sprintf (temp, "ushort   %s;", data->name[j]);  break;
         case T_LONG:   sprintf (temp, "long     %s;", data->name[j]);  break;
         case T_ULONG:  sprintf (temp, "ulong    %s;", data->name[j]);  break;
         case T_FLOAT:  sprintf (temp, "float    %s;", data->name[j]);  break;
         case T_DOUBLE: sprintf (temp, "double   %s;", data->name[j]);  break;
         case T_MONEY:  sprintf (temp, "double   %s;", data->name[j]);  break;
         case T_TIME:   sprintf (temp, "mb_time  %s;", data->name[j]);  break;
         case T_DATE:   sprintf (temp, "mb_date  %s;", data->name[j]);  break;
         case T_PHONE:  sprintf (temp, "mb_phone %s;", data->name[j]);  break;
         default:       sprintf (temp, "long     %s;", data->name[j]);  break;
         }

      i = 24;
      if (data->type[j] == T_CHAR)
         i -= 3 +(data->siz[j] >10) +(data->siz[j] >100) +(data->siz[j] >1000);

      strcat (temp, repeat (' ', i-strlen(data->name[j])));

      strcat (temp, "/");
      strcat (temp, "* field ");
      strcat (temp, data->name[j]);
      strcat (temp, " type ");

      if (data->type[j] != T_CHAR)
         {
         strcat (temp, types[(int)data->type[j]]);
         }
      else
         {
         sprintf (nameb, "string length %d", data->siz[j]);
         strcat  (temp, nameb);
         }
      if (data->type[j] == T_SERIAL && data->serial != 0L)
         {
         sprintf (nameb, " start %ld", data->serial);
         strcat  (temp, nameb);
         }
      strcat (temp, repeat (' ', 73-strlen (temp)));
      strcat (temp, " *");
      strcat (temp, "/");
      strcat (temp, SNGCR);
      strcat (temp, "   ");
      writx (H, temp, strlen (temp));
      }

   if (strname[0])
      {
      strcpy (temp2, strname);
      }
   else
      {
      strcpy (strname, names);
      strcat (strname, "_str");
      strcpy (temp2,   names);
      }

   strcat (temp2, "_rec");

   sprintf (temp, "%c %s;%s", RBC, strname, DUBCR);
   writx   (H, temp, strlen (temp));

   sprintf (temp, "#ifndef MODULE%s   %s %s;%s",
            SNGCR, strname, temp2, SNGCR);
   writx   (H, temp, strlen (temp));

   sprintf (temp, "#else%s   extern %s %s;%s#endif%s#endif%s",
            SNGCR, strname, temp2, SNGCR, DUBCR, DUBCR);
   writx   (H, temp, strlen (temp));

   Printf  ("Header file created.%s", SNGCR);
   close   (H);
}

void
endoffile (num_f, num_i)
int        num_f, num_i;
{
   if (num_f == 0)
      {
      fprintf (stderr, "No fields declared before end reached%s", SNGCR);
      exit    (1);
      }
   if (num_i == 0)
      {
      fprintf (stderr, "No indices declared before end reached%s", SNGCR);
      exit    (1);
      }
}

void
strlwrcpy (new, old)
char      *new,*old;
{
   register char *a,*b;
   if (!new || !old)  return;
   for (a=new,b=old; *b; a++,b++)
      *a = tolower (*b);
   *a=0;
}

void
struprcpy (new, old)
char      *new,*old;
{
   register char *a,*b;
   if (!new || !old)  return;
   for (a=new,b=old; *b; a++,b++)
      *a = toupper (*b);
   *a=0;
}

void
strmax (str, siz)
char   *str;
int          siz;
{
   register int   i;
   register char *a;

   for (i=0, a=str; *a; i++, a++)
      if (i == siz)
         {
         *a = 0;
         break;
         }
}

int
get_names (agc, agv)
int        agc;
char          **agv;
{
   char  temp[128];
   int   i, fh;

   while (agc > 1 && agv[1][0] == '-')
      {
      switch (agv[1][1])
         {
         case 'q':  quiet  = 1;  break;
         case 'h':  header = 1;  break;
         default:   fprintf (stderr,"unrecognized option '%s'%s",agv[1],SNGCR);
                    usage   ();
                    exit    (1);
                   break;
         }
      switch (agv[1][2])
         {
         case 'q':  quiet  = 1;  break;
         case 'h':  header = 1;  break;
         }

      agc--;  agv++;
      }

   if (agc != 2)
      {
      usage ();
      exit  (1);
      }

   strcpy (temp, agv[1]);
   if (strcmp (&temp[strlen(temp)-2], ".s"))
      strcat (temp, ".s");

   strcpy (rel, temp);

   for (i = strlen(temp)-1; i > -1 && temp[i] != ':' && temp[i] != DIRSEP; i--)
      ;
   if (i < 0)  i = 0;

   rel[i] = 0;

   if ((fh = openx (temp, O_RDONLY)) == -1)
      {
      fprintf (stderr, "cannot open %s.%s", temp, SNGCR);
      exit    (1);
      }

   comment();

   (void)skip (fh, "relation");

   strcpy (temp, getword (fh));

   if (temp[0] == 0)
      {
      fprintf (stderr, "file holds no schema definition.%s",SNGCR);
      exit    (1);
      }

   Printf ("%s", CLS);
   Printf ("Building relation %s under ", temp);

   strlwrcpy (names, temp);
   struprcpy (nameb, temp);

   if (rel[0] != 0)
      {
      Printf ("directory %s%s",   rel, DUBCR);
      sprintf (hdr, "%s%c%s.h",   rel, DIRSEP, temp);
      sprintf (rel, "%s%c%s.rel", rel, DIRSEP, temp);
      }
   else
      {
      Printf ("current directory%s", DUBCR);
      sprintf (hdr, "%s.h",   temp);
      sprintf (rel, "%s.rel", temp);
      }

   Printf (lineF, SNGCR);

   comment();

   return fh;
}

char *
repeat (ch, nm)
char    ch;
int         nm;
{
   static char buf[80];

   buf[(nm = (nm < 0) ? 0 : nm)] = 0;

   for (nm--; nm >= 0; nm--)  buf[nm] = ch;

   return buf;
}

int
contains_serial (desc)
char            *desc;
{
   char *line, *pch;

   for (line = desc; (pch = strchr (line, ',')) != NULL; line = pch+1)
      {
      *pch = 0;
      if (data->type[atoi(line)] == T_SERIAL)
         return 1;
      }
   if (data->type[atoi(line)] == T_SERIAL)
      return 1;

   return 0;
}

