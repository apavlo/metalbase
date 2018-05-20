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

#define fieldopt(x) (*(options[x]))
#define fieldmode(x) (*(modes[x]))
#define wrdata(x)   write (H, x, strlen(x));
#define wrline(x) { wrdata(x); wrdata(SNGCR); }

/*
 * Prototypes
 *
 */

#ifdef LONGARGS
   extern void  id_field     (char *, char *);
          void  writeit      (void);
          void  wr_header    (void);
          void  wr_buffer    (void);
          void  wr_mode      (void);
          void  wr_option    (void);
          void  wr_field     (void);
          void  wr_screen    (void);
          void  wr_form      (void);
          void  strqucpy     (char *, char *);
#else
   extern void  id_field();
          void  writeit();
          void  wr_header();
          void  wr_header();
          void  wr_buffer();
          void  wr_mode();
          void  wr_option();
          void  wr_field();
          void  wr_screen();
          void  wr_form();
          void  strqucpy();
#endif

typedef char optlist[10][40];
typedef int  modelist[20];

/*
 * Global Variables
 *
 */

int  H;

extern char      formname[30];
extern char      displ[25][140];
extern int       num_l, pos_y, pos_x, num_f, num_m;

extern field     fld[MAXnFLD];
extern int       lens[MAXnFLD];
extern optlist  *options[MAXnFLD];
extern modelist *modes[MAXnFLD];

void
writeit ()
{
   int   i;
   char  temp[128], *ptr;

   if ((ptr = strrchr (formname, '.')) != NULL)  *ptr = 0;

   sprintf (temp, "%-1.5s_fm.h", formname);

   if ((H = openx (temp, O_RDWR)) != -1)
    { close  (H);
      unlink (temp);
    }
   if ((H = creatx (temp)) == -1)
    { fprintf (stderr, "cannot create %s -- aborted%s", temp, SNGCR);
      mb_exit (10);
    }
   modex (temp, 0666);   /* Make the file   -rw-rw-rw-  */

   wr_header ();
   wr_buffer ();
   wr_mode   ();
   wr_option ();
   wr_field  ();
   wr_screen ();
   wr_form   ();

   close (H);

   for (i = 0; i < MAXnFLD; i++)
    {
      if (options[i] != (optlist *)0)
       { free (options[i]);
         options[i] = (optlist *)0;
       }
      if (modes[i] != (modelist *)0)
       { free (modes[i]);
         modes[i] = (modelist *)0;
       }
    }

   printf ("%-1.5s_fm.h written successfully\n", formname);
}

void
wr_header ()
{
   char  temp[128];
   int   i;

   for (i = 0; formname[i] != 0; i++)
      temp[i] = toupper (formname[i]);
   temp[i] = 0;
   strcat (temp, "_FM_H");

   wrdata ("#");  wrdata ("ifndef ");  wrline (temp);
   wrdata ("#");  wrdata ("define ");  wrline (temp);  wrdata (SNGCR);

   wrdata ("/");  wrline ("*");
   wrline (" * This file was created by MetalBase version 5.0 from the data-entry");
   wrdata (" * template \"");  wrdata (formname);  wrdata (".frm");  wrline ("\"");
   wrline (" *");
   wrline (" * MetalBase 5.0 released October 1st by richid@owlnet.rice.edu");
   wrline (" *");
   wrdata (" *");  wrline ("/");
   wrdata (SNGCR);

   wrdata ("#");  wrline ("ifdef MODULE");
   wrdata ("   extern de_form ");  wrdata (formname);  wrline ("_fm;");
   wrdata ("#");  wrline ("else");
}

void
wr_buffer ()
{
   char  temp[128];
   int   i;

   for (i = 0; i < num_f; i++)
      {
      wrdata ("   ");
      switch (fld[i].type)
         {
         case T_SHORT:   wrdata ("short    ");  break;
         case T_USHORT:  wrdata ("ushort   ");  break;
         case T_LONG:    wrdata ("long     ");  break;
         case T_ULONG:   wrdata ("ulong    ");  break;
         case T_FLOAT:   wrdata ("float    ");  break;
         case T_DOUBLE:  wrdata ("double   ");  break;
         case T_MONEY:   wrdata ("double   ");  break;
         case T_TIME:    wrdata ("mb_time  ");  break;
         case T_DATE:    wrdata ("mb_date  ");  break;
         case T_SERIAL:  wrdata ("long     ");  break;
         case T_PHONE:   wrdata ("mb_phone ");  break;
         default:        wrdata ("char     ");  break;
         }

      if (fld[i].type == T_CHAR)
         sprintf (temp, "%s_f%d[%2d] = ", formname, i, lens[i]);
      else
         sprintf (temp, "%s_f%d     = ",  formname, i);
      wrdata (temp);

      switch (fld[i].type)
         {
         case T_SHORT:   wrdata ("(short)0");     break;
         case T_USHORT:  wrdata ("(ushort)0");    break;
         case T_LONG:    wrdata ("0L");           break;
         case T_ULONG:   wrdata ("(ulong)0L");    break;
         case T_FLOAT:   wrdata ("(float)0.0");   break;
         case T_DOUBLE:  wrdata ("(double)0.0");  break;
         case T_MONEY:   wrdata ("(double)0.0");  break;
         case T_TIME:    wrdata ("(mb_time)0");   break;
         case T_DATE:    wrdata ("(mb_date)0");   break;
         case T_SERIAL:  wrdata ("0L");           break;
         default:        wrdata ("\"\"");         break;
         }
      wrline (";");
      }
   wrdata (SNGCR);
}

void
wr_mode ()
{
   char  temp[128];
   int   i, j;

   for (i = 0; i < num_f; i++)
    {
      sprintf (temp, "   int %s_m%d[%d] = ", formname, i, num_m);
      wrdata  (temp);
      wrdata  (LBRCQ);
      wrdata  (" ");

      for (j = 0; j < num_m; j++)
       {
         if (fieldmode(i)[j] == FM_INOUT)  strcpy (temp, "FM_INOUT");
         if (fieldmode(i)[j] == FM_IN)     strcpy (temp, "FM_IN");
         if (fieldmode(i)[j] == FM_OUT)    strcpy (temp, "FM_OUT");
         if (j < num_m-1)  strcat (temp, ",");
         strcat (temp, "     ");  temp[10] = 0;
         wrdata (temp);
       }
      wrdata  (" ");
      wrdata  (RBRCQ);
      wrline  (";");
    }

   wrdata (SNGCR);
}

void
wr_option ()
{
   char  temp[128];
   int   i, j;

   for (i = 0; i < num_f; i++)
      if (options[i] != (optlist *)0)
       {
         for (j = 1; j < 10; j++)
            if (fieldopt(i)[j][0] == 0)  break;
         sprintf (temp, "   char *%s_o%d[%d] = ", formname, i, j+1);
         wrdata (temp);
         wrdata (LBRCQ);
         for (j = 0; j < 10; j++)
            if (j != 0 && fieldopt(i)[j][0] == 0)  break;
            else
             {
               if (j != 0)  wrdata (",");
               wrdata (" \"");
               wrdata (fieldopt(i)[j]);
               wrdata ("\"");
             }
         wrdata (", \"\" ");
         wrdata (RBRCQ);
         wrline (";");
       }
   wrdata (SNGCR);
}

void
wr_field ()
{
   char  temp[128];
   int   i;

   sprintf (temp, "   field %s_f[%d] =", formname, num_f);
   wrline  (temp);

   for (i = 0; i < num_f; i++)
    {
      if (i == 0)
       { wrdata ("    ");  wrdata (LBRCQ);  wrdata (" "); }
      else
       { wrline (",");  wrdata ("      "); }

      wrdata (LBRCQ);  wrdata (" ");

      sprintf (temp, "%2d, %2d, %2d,", fld[i].y, fld[i].x, fld[i].len);
      wrdata  (temp);
      sprintf (temp, " %2d, 0, %d,", fld[i].type, fld[i].option);
      wrdata  (temp);
      sprintf (temp, " %s_m%d,", formname, i);
      wrdata  (temp);
      wrdata ( (fld[i].type==T_CHAR || fld[i].type==T_PHONE) ? "  " : " &" );
      sprintf (temp, "%s_f%d, \"%s\",", formname, i, fld[i].name);
      wrdata  (temp);
      if (options[i] == (optlist *)0)  strcpy (temp, " NULL ");
      else
         sprintf (temp, " %s_o%d ", formname, i);
      wrdata  (temp);
      wrdata  (RBRCQ);
    }
   wrdata (" ");  wrdata (RBRCQ);  wrline (";");

   wrdata (SNGCR);
}

void
wr_screen ()
{
   char  temp[128];
   int   i;

   sprintf (temp, "   char *%s_s[%d] =", formname, num_l);
   wrline (temp);

   for (i = 0; i < num_l; i++)
    {
      if (i == 0)
       { wrdata ("    ");   wrdata (LBRCQ);   wrdata (" ");
       }
      else
       { wrline (",");   wrdata ("      ");
       }

      wrdata ("\"");
      strqucpy (temp, displ[i]);
      wrdata (temp);
      wrdata ("\"");
    }
   wrdata ("  ");
   wrdata (RBRCQ);
   wrline (";");
   wrdata (SNGCR);
}

void
strqucpy (oa, ob)
char     *oa,*ob;
{
   register char *a, *b;
   for (a=oa, b=ob; *b; a++,b++)
      {
      if (*b == '\"')
         {
         *a = '\\';
         a++;
         }
      *a = *b;
      }
   *a = 0;
}

void
wr_form ()
{
   char  temp[128];

   wrdata ("   de_form "); wrdata (formname); wrline ("_fm =");
   wrdata ("    ");  wrdata (LBRCQ);

   sprintf (temp, " 1, 0, 0, 0, %d, %d, (int_fn)0, %d,", num_f, num_m, num_l);
   wrdata  (temp);
   sprintf (temp, " %d, %d, %s_f, %s_s ", pos_y, pos_x, formname, formname);
   wrdata  (temp);
   wrdata  (RBRCQ);
   wrline  (";");

   wrdata (SNGCR);
   wrdata ("#");  wrline ("endif");  wrdata (SNGCR);
   wrdata ("#");  wrline ("endif");  wrdata (SNGCR);
}

