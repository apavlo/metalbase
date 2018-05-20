/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 *
 * Special thanks go to Adrian Corston (adrian@internode.com.au) for his
 * suggestions and code.  While this code is of my penning, the idea and
 * style of implementation for this are direct ports from his own
 * excellent work.
 *
 */

#include "mbase.h"

#ifndef MAXnREL
#define MAXnREL 20    /* Max # of relations in any given DE form */
#endif
#ifndef MAXnFLD
#define MAXnFLD 40    /* Max # of fields in any given DE form    */
#endif

/*
 * Definitions
 *
 */

#define LPARQ "("
#define LBRCQ "{"   /* These are pulled out   */
#define LBKT  '['   /* so that vi's ()/[]/{}  */
#define LBRC  '{'   /* matchin works properly */
#define RBRC  '}'   /* in moving through the  */
#define RBKT  ']'   /* code.                  */
#define RBRCQ "}"
#define RPARQ ")"

#define usage() fprintf (stderr, "form: format: %sform [formname]%s",SNGCR,SNGCR)

#define comment() skip(form,";"); while(skip(form,"#")) goeol(form,NULL);
#define nocolon()                 while(skip(form,"#")) goeol(form,NULL);

#define fieldopt(x) (*(options[x]))
#define fieldmode(x) (*(modes[x]))

/*
 * Prototypes
 *
 */

#ifdef LONGARGS
   void  main         (int, char **);
   void  parse_args   (int, char **);
   void  check_data   (int);
   void  check_defin  (int);
   void  check_screen (int);
   void  check_fields (int);
   void  check_modes  (int);
   void  id_field     (char *, char *);
   extern void  writeit      (void);
#else
   void  main();
   void  parse_args();
   void  check_data();
   void  check_defin();
   void  check_screen();
   void  check_fields();
   void  check_modes();
   void  id_field();
   extern void  writeit();
#endif

typedef relation *relptr;
typedef char      optlist[10][40];
typedef int       modelist[20];

/*
 * Global Variables
 *
 */

char      formname[30];
int       form;

ftype     gen_type;
int       gen_len;

char      defins[26][50];
char      displ[25][140];
int       num_l, pos_y, pos_x, num_f, num_m;
field     fld[MAXnFLD];
int       lens[MAXnFLD];
optlist  *options[MAXnFLD];
modelist *modes[MAXnFLD];

int       num_r;
relptr    rel[MAXnREL];

/*
 * Main code
 *
 */

void
main  (argc, argv)
int    argc;
char **argv;
{
   parse_args   (argc, argv);

   check_data   (form);
   check_defin  (form);
   check_screen (form);
   check_fields (form);
   check_modes  (form);

   close   (form);
   mb_die  ();  /* Close all open relations */

   writeit ();
   exit (0);
}

/*
 * Utilities
 *
 */

void
parse_args (agc, agv)
int         agc;
char           **agv;
{
   while (agc > 1 && agv[1][0] == '-')
    {
      switch (agv[1][1])
       { default:   fprintf (stderr, "unrecognized option '%s'\n", agv[1]);
                    usage   ();
                    exit    (1);
                   break;
       }

      agc--;  agv++;
    }

   if (agc != 2)
    { usage ();
      exit  (1);
    }

   strcpy (formname, agv[1]);
   if (strcmp (&formname[strlen(formname)-4], ".frm"))
      strcat (formname, ".frm");
   if ((form = openx (formname, O_RDONLY)) < 0)
    { fprintf (stderr, "cannot open form '%s'\n", formname);
      exit    (2);
    }
}

void
check_data (form)
int         form;
{
   char  temp[80];

   comment();
   num_r = 0;
   if (skip(form,"data") || skip(form,"relations") || skip(form,"relation"))
    {
      while (! skip (form, ";"))
       {
         if (num_r == MAXnREL)
          { fprintf (stderr, "Too many relations--see trouble.dox%s", SNGCR);
            close   (form);
            mb_exit (4);
          }

         strcpy (temp, getword(form));
         skip (form, ","); nocolon();

         if (! strcmp (&temp[strlen(temp)-4], ".rel"))
            temp[strlen(temp)-4] = 0;

         if ((rel[num_r] = mb_inc (temp, 0)) == RNULL)
          { fprintf (stderr, "Cannot open relation '%s' : %s\n",temp,mb_error);
            close   (form);
            mb_exit (4);
          }
         num_r++;
       }
      if (num_r == 0)
       { fprintf (stderr,"Data keyword implies at least one relation%s",SNGCR);
         close   (form);
         mb_exit (5);
       }
    }
}

void
check_defin (form)
int          form;
{
   int   i;
   char  temp[80], t2[80];

   for (i = 0; i < 26; i++)  defins[i][0] = 0;

   while (skip (form, "define"))
    {
      strcpy (temp, getword (form));
      strcpy (t2,   getword (form));

      if (strlen (temp) != 1 || t2[0] == 0)
       { fprintf (stderr, "DEFINE (%s:%s) syntax error%s", temp, t2, SNGCR);
         close   (form);
         mb_exit (6);
       }
      if (defins[(i = tolower(temp[0])-'a')][0] != 0)
       { fprintf (stderr, "Multiple references to DEFINE %c%s",temp[0],SNGCR);
         close   (form);
         mb_exit (6);
       }

      strcpy  (defins[i], t2);
      comment ();
    }
}

void
check_screen (form)
int           form;
{
   int   i, j, k;
   char  temp[80], c;

   pos_y = pos_x = 0;

   if (! skip (form, "screen"))
    { fprintf (stderr, "Screen{} segment must follow Data and Define%s",SNGCR);
      close   (form);
      mb_exit (7);
    }
   if (! skip (form, LBRCQ))
    {
      pos_y = atoi (getword (form));

      if (! skip (form, LBRCQ))
       {
         pos_x = atoi (getword (form));

         if (! skip (form, LBRCQ))
          { fprintf (stderr, "Left brace must follow SCREEN keyword%s",SNGCR);
            close   (form);
            mb_exit (7);
          }
       }
    }
   goeol (form, NULL);

   num_f = 0;

   for (num_l = 0; num_l < 24; num_l++)
    {
      goeol (form, displ[num_l]);
      if (displ[num_l][0] == RBRC)  break;

      for (;;)
       {
         for (i = 0; displ[num_l][i] != 0; i++)
            if (displ[num_l][i] == LBKT || displ[num_l][i] == LBRC)
               break;
         if (displ[num_l][i] == 0)
            break;

         for (j = i+1; displ[num_l][j] != 0; j++)
            if ((displ[num_l][j] == RBKT && displ[num_l][i] == LBKT) ||
                (displ[num_l][j] == RBRC && displ[num_l][i] == LBRC))
               break;
            else
               temp[j-i-1] = displ[num_l][j];
         temp[j-i-1] = 0;
         if (displ[num_l][j] == 0)
            break;

         if (num_f == MAXnFLD)
          { fprintf (stderr, "Too many fields--see trouble.dox%s", SNGCR);
            close   (form);
            mb_exit (8);
          }

         for (k = 0; k < j-i-1; k++)
            if (temp[k] == ' ' || temp[k] == '\t')
               break;
         temp[k] = 0;
         if (k == 1)
          {
            k = (toupper (temp[0]) - 'A');
            if (defins[k][0] == 0)
             {
               fprintf (stderr, "Field %c undefined%s", temp[0], SNGCR);
               close   (form);
               mb_exit (8);
             }
            strcpy (temp, defins[k]);
          }

         gen_len = j-i;
         id_field (fld[num_f].name, temp);
         fld[num_f].type  = gen_type;
         fld[num_f].y     = num_l;
         fld[num_f].x     = i + ((c = displ[num_l][i]) == LBKT);
         fld[num_f].len   = j-i-1;
         lens[num_f]      = gen_len;

         num_f++;

         for (k = i; i <= j; i++)
            displ[num_l][i] = ' ';
         if (c == LBRC)
          {
            for (i = k; displ[num_l][i+1] != 0; i++)
               displ[num_l][i] = displ[num_l][i+1];
            displ[num_l][i] = 0;
            i = j-1;
            for ( ; displ[num_l][i+1] != 0; i++)
               displ[num_l][i] = displ[num_l][i+1];
            displ[num_l][i] = 0;
          }
       }
    }
   comment();
}

void
check_fields (form)
int           form;
{
   char   temp[80];
   int    i, j, t;

   for (i = 0; i < MAXnFLD; i++)
      options[i] = (optlist *)0;

   while (skip (form, "field"))
    {
      id_field (temp, getword(form));

      for (i = 0; i < num_f; i++)
         if (! strcmp (fld[i].name, temp))  break;
      if (i == num_f)
       { fprintf (stderr, "FIELD variable '%s' unused%s", temp, SNGCR);
         close   (form);
         mb_exit (9);
       }

      skip (form, "type");

       /*
        * Field credit type choice ("Yy" "Nn" "?");
        * Field temp   type link to credit ("Yes" "No" "Maybe");
        * Field other  type money;
        *
        */

      strcpy (temp, getword (form));

      if (! strcmp (temp, "choice") || ! strcmp (temp, "link"))
       {
         t = 0;
         if (!strcmp (temp, "link"))
          {
            skip (form, "to");
            id_field (temp, getword(form));
            t = 1;
          }

         if (! skip (form, LPARQ))
          { fprintf (stderr, "(...) must surround options in FIELD%s", SNGCR);
            close   (form);
            mb_exit (9);
          }

         if ((options[i] = New (optlist)) == (optlist *)0)
          { fprintf (stderr, "fatal error: out of memory%s", SNGCR);
            close   (form);
            mb_exit (9);
          }

         fieldopt(i)[0][0] = 0;  /* Link-To field name */
         if (t == 1)
            strcpy (fieldopt(i)[0], temp);  /* Link-To field name */

         for (j = 1; !skip (form, RPARQ); j++)
          {
            if (j == 10)  break;
            strcpy (fieldopt(i)[j], getword(form));
          }
         if (j != 10)  fieldopt(i)[j][0] = 0;

         fld[i].option = t+1;  /* t: 0 == choice, 1 == link */

         comment();
         continue;
       }

      if (! strcmp (temp, "char"))       fld[i].type = T_CHAR;
      if (! strcmp (temp, "string"))     fld[i].type = T_CHAR;
      if (! strcmp (temp, "character"))  fld[i].type = T_CHAR;
      if (! strcmp (temp, "short"))      fld[i].type = T_SHORT;
      if (! strcmp (temp, "ushort"))     fld[i].type = T_USHORT;
      if (! strcmp (temp, "long"))       fld[i].type = T_LONG;
      if (! strcmp (temp, "ulong"))      fld[i].type = T_ULONG;
      if (! strcmp (temp, "float"))      fld[i].type = T_FLOAT;
      if (! strcmp (temp, "double"))     fld[i].type = T_DOUBLE;
      if (! strcmp (temp, "money"))      fld[i].type = T_MONEY;
      if (! strcmp (temp, "time"))       fld[i].type = T_TIME;
      if (! strcmp (temp, "date"))       fld[i].type = T_DATE;
      if (! strcmp (temp, "serial"))     fld[i].type = T_SERIAL;
      if (! strcmp (temp, "phone"))      fld[i].type = T_PHONE;

      comment ();
    }
}

void
check_modes  (form)
int           form;
{
   char   temp[80];
   int    i, j, k;

   for (i = 0; i < MAXnFLD; i++)
      modes[i] = (modelist *)0;
   for (i = 0; i < num_f; i++)
      if ((modes[i] = New (modelist)) == (modelist *)0)
       { fprintf (stderr, "fatal error: out of memory%s", SNGCR);
         close   (form);
         mb_exit (9);
       }

   num_m = 0;

   while (skip (form, "mode"))
    {
      if ((i = atoi (getword (form))) < 1) { goeol (form, NULL);  continue; };

      strcpy (temp, getword (form));
      k = FM_INOUT;
      if (! strcmp (temp, "in"))   k = FM_IN;
      if (! strcmp (temp, "out"))  k = FM_OUT;

      for (j = 0; j < num_f; j++)
         fieldmode(j)[i-1] = k;

      while (! skip (form, ";"))
       {
         id_field (temp, getword (form));

         for (j = 0; j < num_f; j++)
            if (! strcmp (fld[j].name, temp))  break;
         if (j == num_f)
          { fprintf (stderr, "MODE variable '%s' unused%s", temp, SNGCR);
            close   (form);
            mb_exit (9);
          }

         k = FM_INOUT;
         strcpy (temp, getword (form));
         if (! strcmp (temp, "in"))   k = FM_IN;
         if (! strcmp (temp, "out"))  k = FM_OUT;
         fieldmode(j)[i-1] = k;

         skip (form, ",");
       }

      num_m ++;
      nocolon ();
    }

   if (! skip (form, "end"))
    { fprintf (stderr, "unexpected keyword: END%s", SNGCR);
      close   (form);
      mb_exit (9);
    }
}

void
id_field (buf, str)
char     *buf,*str;
{
   int  i, j;

   strcpy (buf, str);
   gen_type = T_CHAR;

   if (strchr (str, '.') == NULL)
      for (i = 0; i < num_r; i++)
       {
         for (j = 0; j < rel[i]->num_f; j++)
            if (! strcmp (str, rel[i]->name[j]))
               {
               sprintf (buf, "%s.%s", rel[i]->relname, rel[i]->name[j]);
               gen_type = rel[i]->type[j];
               gen_len  = rel[i]->siz[j];
               break;
               }
         if (j != rel[i]->num_f)  break;
       }
}

