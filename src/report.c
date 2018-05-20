/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 *
 * Special thanks go to Bruce Momjian (root@candle.uucp@ls.com) for his
 * suggestions and code.
 *
 */

#include "mbase.h"

/*
 * Definitions
 *
 */

#define usage() fprintf(stderr,"format: report [-k encryption_key] [templatename]%s", SNGCR)
#define syntax(x) { fprintf(stderr,"syntax error in template: expected keyword '%s'%s", x, SNGCR);  mb_exit(3); }

#define comment() skip(templ,";"); while(skip(templ,"#")) goeol(templ,NULL);
#define skipl(x) for (I=0; I<x; I++)  printf(SNGCR);

#define MAX_WIDTH  80
#define FULL_WIDTH "%-79.79s"

#define NONE   0
#define BEFORE 1
#define DURING 2
#define AFTER  4

 /* CREDIT is displayed as the variable system without "!date" or anything */

#define CREDIT "MetalBase 5.0 Report Writer"

/*
 * Prototypes
 *
 */

#ifdef LONGARGS
   void  parse_args (int, char **);
   int   get_size   (long);
   void  do_prints  (long, int);
   int   fill_data  (dataptr, int, char *);
#else
   void  parse_args();
   int   get_size();
   void  do_prints();
   int   fill_data();
#endif

/*
 * Variables
 *
 */

extern    long   _lpos;  /* These are from parse.c */
extern    int    quoted; /* Referenced in libmb.a  */

int       templ, I;
long      lpos = 0L;
char      key[MAX_WIDTH] = "";
relation *rel;
long      l_pos = 0L;
long      h_pos = 0L;
long      f_pos = 0L;
long      o_pos = 0L;
int       keep_l = 0, keep_h = 0, keep_f = 0;
dataptr   rec, buf;

int       num_col = 80;   /* 80-column (std pagesize) paper */
int       num_row = 66;   /* 66-line (std pagesize) paper   */
int       top_mar =  4;   /* 2/3" top margin by default     */
int       bot_mar =  6;   /* 1" bottom margin by default    */
int       lef_mar = 10;   /* 1" left margin by default      */
int       rig_mar = 10;   /* 1" right margin by default     */
int       wid;
int       hgt;
int       pageno  =  1;   /* Start with page 1, obviously   */

/*
 * Main code
 *
 */

void
main  (argc, argv)
int    argc;
char **argv;
{
   int    act, idx, numlines, lines, stop, dir, i, on_siz;
   int    didother, doneone,  badrec;
   char   temp[128];

   parse_args (argc, argv);

   comment();
   if (! skip (templ, "data"))  syntax ("data");
   strcpy (temp, getword(templ));
   comment();

   if (! strcmp (&temp[strlen(temp)-4], ".rel"))
      temp[strlen(temp)-4] = 0;
   if ((rel = mb_inc (temp, strtokey (key))) == RNULL)
    { fprintf (stderr, "Cannot open relation '%s' : %s\n", temp, mb_error);
      close   (templ);
      exit    (4);
    }
   if ((rec = (dataptr)malloc (2+rel->rec_len)) == NULL)
    { fprintf (stderr, "Out of memory\n");
      close   (templ);
      mb_exit (4);
    }
   if ((buf = (dataptr)malloc (2+rel->rec_len)) == NULL)
    { fprintf (stderr, "Out of memory\n");
      free    (rec);
      close   (templ);
      mb_exit (4);
    }

   for (didother = 0; ; )
    {
      if (skip (templ, ";") == -1)  break;
      for (;;)
       {
         if (skip (templ, ";") == -1)  break;
         comment();
         if (skip (templ, "size"))
          {
            for (didother = 1; ; )
             {
               if (skip (templ, ";") == -1)  break;
               comment();

               strcpy (temp, getword (templ));
               if (! strcmp (temp, "columns"))  num_col = atoi(getword(templ));
               else
               if (! strcmp (temp, "rows"))     num_row = atoi(getword(templ));
               else
               if (! strcmp (temp, "top"))
                { skip (templ, "margin");  top_mar = atoi (getword (templ)); }
               else
               if (! strcmp (temp, "bottom"))
                { skip (templ, "margin");  bot_mar = atoi (getword (templ)); }
               else
               if (! strcmp (temp, "left"))
                { skip (templ, "margin");  lef_mar = atoi (getword (templ)); }
               else
               if (! strcmp (temp, "right"))
                { skip (templ, "margin");  rig_mar = atoi (getword (templ)); }
               else
               if (!strcmp (temp, "page")   || !strcmp (temp, "newpage") ||
                   !strcmp (temp, "header") || !strcmp (temp, "last")    ||
                   !strcmp (temp, "on")     || !strcmp (temp, "footer"))
                {
                  putback(templ);  /* Go back before this word */
                  break;
                }
               else
                { fprintf (stderr, "token '%s' unrecognized in Size\n", temp);
                  free    (rec);
                  free    (buf);
                  close   (templ);
                  mb_exit (3);
                }
               continue;
             }
          }
         if (skip (templ, "newpage"))
          {
            didother = 1;
            printf ("\f");
            comment();
            continue;
          }
         if (skip (templ, "page"))
          {
            didother = 1;
            pageno = atoi (getword (templ));
            comment();
            continue;
          }
         if (skip (templ, "header"))
          {
            keep_h = skip (templ, "keep");
            didother = 2;  /* Ignore errors until next keyword */
            comment();
            h_pos = lseek (templ, 0L, 1);  (void)getword(templ);
            continue;
          }
         if (skip (templ, "last"))
          {
            keep_l = skip (templ, "keep");
            didother = 2;  /* Ignore errors until next keyword */
            comment();
            l_pos = lseek (templ, 0L, 1);  (void)getword(templ);
            continue;
          }
         if (skip (templ, "footer"))
          {
            keep_f = skip (templ, "keep");
            didother = 2;  /* Ignore errors until next keyword */
            comment();
            f_pos = lseek (templ, 0L, 1);  (void)getword(templ);
            continue;
          }

         if (skip (templ, "on"))
            break;

         if (! didother)
          { fprintf (stderr,
               "release 5.0 cannot use more than one relation%s", SNGCR);
            close (templ);
            syntax ("size");
          }

         if (didother != 2)
          { fprintf (stderr, "unexpected keyword '%s'%s",getword(templ),SNGCR);
            close   (templ);
            free    (rec);
            free    (buf);
            mb_exit (3);
          }
         (void)getword(templ);  /* Skip this one--it's in a header or etc */
       }

      if (skip (templ, ";") == -1)  break;
      didother = 1;
      wid = num_col - rig_mar - lef_mar;
      hgt = num_row - top_mar - bot_mar;

/*
 * Perform ON clause.  We are currently sitting right after the keyword On,
 * and the following are conditionals on the operation of this clause:
 *   h_pos:  0L==no header - else, location of first print/skip command
 *   l_pos:  0L==no last   - else, location of first print/skip command
 *   f_pos:  0L==no footer - else, location of first print/skip command
 *
 */

      strcpy (temp, getword (templ));
      if ((idx = idxnum (rel, temp)) < 0)
       { fprintf (stderr, "invalid index '%s' referenced%s", temp, SNGCR);
         free    (rec);
         free    (buf);
         close   (templ);
         mb_exit (5);
       }

      act = FIRST, stop = NONE;
      dir = 0;
      if (skip (templ, "<="))  act = FIRST, stop=AFTER;
      if (skip (templ, "=<"))  act = FIRST, stop=AFTER;
      if (skip (templ, "<"))   act = FIRST, stop=DURING|AFTER;
      if (skip (templ, ">="))  act = GTEQ,  stop=NONE;
      if (skip (templ, "=>"))  act = GTEQ,  stop=NONE;
      if (skip (templ, ">"))   act = GTHAN, stop=NONE;
      if (skip (templ, "=="))  act = EQUAL, stop=AFTER;
      if (skip (templ, "="))   act = EQUAL, stop=AFTER;

      if (act != FIRST || stop != NONE)
       { strcpy (temp, getword(templ));
         if (!strcmp (temp, "print") || !strcmp (temp, "skip"))
          { fprintf (stderr, "query requires comparison value%s", SNGCR);
            free    (rec);
            free    (buf);
            close   (templ);
            mb_exit (5);
          }

         if (fill_data (rec, idx, temp))
            fill_data (buf, idx, temp);
         else
          { act = FIRST;
            stop = NONE;
          }
       }

      dir = NEXT;
      if (skip (templ, "reverse"))
       { dir = PREV;
         switch (act)
          { case FIRST:  if (stop == NONE)    act = LAST;
                         if (stop == AFTER)   act = LTEQ,  stop = NONE;
                         if (stop &  DURING)  act = LTHAN, stop = NONE;
                        break;
            case  GTEQ:  if (stop == NONE)    act = LAST,  stop = BEFORE;
            case GTHAN:  if (stop == NONE)    act = LAST,  stop = DURING|BEFORE;
          }
       }

      o_pos    = lseek(templ,0L,1);
      on_siz   = get_size(o_pos);
      numlines = hgt - get_size(h_pos) - get_size(f_pos);

      doneone = 0;
      for (lines = 0; ; )
       { badrec = 0;
         if (mb_sel (rel, idx, rec, act, buf) != MB_OKAY)
            if (doneone || mb_errno != MB_NO_SUCH)  break;
            else
               badrec = 1;
         doneone = 1;
         if (badrec == 0 && stop != NONE)
            if (((i = compare (rel, rec, buf, idx)) < 0 && stop&BEFORE) ||
                 (i == 0 && stop&DURING) || (i >  0 && stop&AFTER))
               break;
         lseek (templ, o_pos, 0);

         if (lines+on_siz > numlines-1)
           { skipl(numlines-lines);
             do_prints(f_pos, 1);
             lines=0; skipl(bot_mar); pageno++;
           }
         if (lines == 0) { skipl(top_mar); do_prints (h_pos, 1); }

         do_prints (o_pos, !badrec);
         if (badrec)  break;
         lines += on_siz;
         act = dir;
       }

      if (mb_errno != MB_NO_SUCH && mb_errno != MB_OKAY)
       { fprintf (stderr, "aborted -- %s%s", mb_error, SNGCR);
         free    (rec);
         free    (buf);
         close   (templ);
         mb_exit (6);
       }

      o_pos = lseek (templ, 0L, 1);
      skipl (numlines-lines);
      do_prints (l_pos ? l_pos : f_pos, 1);
      lseek (templ, o_pos, 0);
      skipl (bot_mar);  pageno++;

   didother = 2;  /* Skip stuff until valid keyword */

/*
 * Done with this clause.  Erase current header/footer/last positions--they
 * don't carry from one clause to another.  That is, unless they had 'keep'
 * keywords...
 *
 */

      if (! keep_l)  l_pos = 0L;
      if (! keep_f)  f_pos = 0L;
      if (! keep_h)  h_pos = 0L;
    }

   close   (templ);
   free    (rec);
   free    (buf);
   mb_exit (0);
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
   char name[256];

   while (agc > 1 && agv[1][0] == '-')
      {
      switch (agv[1][1])
         {
         case 'k':  if (agv[1][2])
                       strcpy (key, &agv[1][2]);
                    else
                     { agc--;  agv++;
                       strcpy (key, agv[1]);
                     }
                   break;
         default:   fprintf (stderr, "unrecognized option '%s'\n", agv[1]);
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

   strcpy (name, agv[1]);
   if (strcmp (&name[strlen(name)-4], ".rpt"))  strcat (name, ".rpt");
   if ((templ = openx (name, O_RDONLY)) < 0)
    { fprintf (stderr, "cannot open template '%s'\n", name);
      exit    (2);
    }
}

/*
 * get_size() reads from position 'pos' until it doesn't see a print or
 * skip command--it keeps track of the number of lines used in the section,
 * and returns it.  It's used to make headers and footers work well with
 * top and bottom margins.
 *
 */

int
get_size (pos)
long      pos;
{
   int  num = 0;

   for (lseek (templ, pos, 0); ; )
    {
      if (skip (templ, ";") == -1)  break;
      comment();
      if (skip (templ, "print"))
       {
         if (! skip (templ, "continued"))
          { if (! strcmp (":", getword(templ)))  num++;
            else
               if (! skip (templ, "continued"))  num++;
          }

         for (;;)
            if (skip (templ, ";"))  break;
            else
               (void)getword(templ);
       }
      else
         if (skip (templ, "skip"))
          {
            num += atoi (getword (templ));
            skip (templ, "lines");  skip (templ, "line");
          }
         else
            break;
    }

   return num;
}

/*
 * do_prints() actually performs the printings.  :)
 *
 */

void
do_prints (pos, really)
long       pos;
int             really;
{
   long    lx;  /* typ == 1 */
   double  fx;  /* typ == 2 */
   mb_time tx;  /* typ == 3 */
   mb_date dx;  /* typ == 4 */
   int     cnt, jst, x, w, l, typ, fmt;
   char    spc[128], temp[256], t2[128], t3[128], *p;

   if (! pos)  return;

   sprintf (spc, FULL_WIDTH, "");  spc[lef_mar] = 0;

   w = wid;  l = lef_mar;
   for (lseek (templ, pos, 0); ; )
      {
      if (skip (templ, ";") == -1)  break;
      comment();

      if (skip (templ, "skip"))
         {
         x = atoi (getword (templ));
         skip (templ, "lines");  skip (templ, "line");
         skip (templ, ";");
         if (really)
            for ( ; x > 0; x--)  printf (SNGCR);
         continue;
         }

      if (! skip (templ, "print"))
         {
         break;
         }

      cnt = jst = 0;
      if (skip (templ, "continued"))  cnt = 1;
      if (skip (templ, "centered"))   jst = 1;
      if (skip (templ, "continued"))  cnt = 1;
      if (skip (templ, "right"))      jst = 2;
      if (skip (templ, "continued"))  cnt = 1;

      skip (templ, ":");

/*
 * process the print command
 *
 */

      for (temp[0] = 0; ; )
         {
         strcpy (t2, getword(templ));
         if (! quoted && ! strcmp (t2, ";"))  break;

         if (quoted)
            {
            typ=0;
            }
         else
            {
            if (isdigit(t2[0]) || t2[0] == '.')
               {
               if (strchr (t2, '.'))  typ=2, fx=(double)atof(t2);
               else                   typ=1, lx=(long)atol(t2);
               }
            else
               {
               if (!strcmp (t2, "column"))
                { sprintf (t2, FULL_WIDTH, "");
                  x = atoi(getword(templ))-l-strlen(temp);
                  if (x < 0)  x = 0;
                  t2[x] = 0;  typ = 0;
                }
               else
                  {
                  if (!strcmp (t2, ","))
                     t2[0] = ' ', typ = 0;
                  else
                     {

/*
 * It's a variable of some sort; make t2 the name and t3 any modifer.
 *
 */

                     fmt = 0;
                     t3[0] = 0;
                     if (p = strchr (t2, '!'))
                        {
                        strcpy (t3, p+1);
                        *p = 0;
                        }

/*
 * That done, figure out what type it should be and assign it.
 *
 */
                     typ = -1;

                     if (!strcmp (t2, "system"))
                        {
                        if (! strcmp (t3, "time"))
                         { tx = curtime();  typ = 3; }
                        else if (! strcmp (t3, "date"))
                         { dx = curdate();  typ = 4; }
                        else if (! strcmp (t3, "page"))
                         { lx = pageno;     typ = 1; }
                        else 
                         { strcpy (t2, CREDIT);  typ = 0; }
                        }

/*
* If they use the relation name, trash it...
*
*/

                     if (typ == -1 && (p = strchr (t2, '.')))
                        {
                        strcpy (spc, 1+p);
                        strcpy (t2, spc);
                        sprintf (spc, FULL_WIDTH, "");  spc[lef_mar] = 0;
                        }

                     if (typ == -1)
                        {
                        for (x=0; x < rel->num_f; x++)
                           if (! strcmp (rel->name[x], t2))  break;
                        typ = 0;
                        if (x != rel->num_f)
                           {

/*
 * It's a valid name, so use its data (field # is in x)
 *
 */

                           if (! t3[0])
                              {
                              p = (char *)rec + rel->start[x];

            switch (rel->type[x])
               {
               case T_SHORT:   lx = (long) *(short  *)p;  typ = 1;  break;
               case T_USHORT:  lx = (long) *(ushort *)p;  typ = 1;  break;
               case T_LONG:
               case T_SERIAL:  lx = (long) *(long   *)p;  typ = 1;  break;
               case T_ULONG:   lx = (long) *(ulong  *)p;  typ = 1;  break;
               case T_FLOAT:   fx = (float)*(float  *)p;  typ = 2;  break;
               case T_DOUBLE:
               case T_MONEY:   fx = (float)*(double *)p;  typ = 2;  break;
               case T_TIME:    tx =       *(mb_time *)p;  typ = 3;  break;
               case T_DATE:    dx =       *(mb_date *)p;  typ = 4;  break;
               default:        strcpy (t2, p);            typ = 0;  break;
               }

                              }
                           }
                        }
                     }
                  }
               }
            }

         if (skip (templ, "using") || skip (templ, "format"))
          { if (typ == 3 || typ == 4)
               fmt = atoi (getword (templ));
            else
             { strcpy (t3, getword(templ));
               switch (typ)
                { case 0:  strcpy  (spc, t2);
                           sprintf (t2, t3, spc);  typ = 0;
                           sprintf (spc,FULL_WIDTH,"");  spc[lef_mar]=0;
                          break;
                  case 1:  sprintf (t2, t3, lx);  typ = 0;  break;
                  case 2:  sprintf (t2, t3, fx);  typ = 0;  break;
                }
             }
          }

         if (typ == 1)  sprintf (t2, "%ld", lx);
         if (typ == 2)  sprintf (t2, "%lg", fx);
         if (typ == 3)  strcpy  (t2, fmt_time (tx, fmt));
         if (typ == 4)  strcpy  (t2, fmt_date (dx, fmt));

         if (skip (templ, "to"))
          { sprintf (t3, FULL_WIDTH, "");
            x = atoi (getword(templ)) - l - strlen (t2) - strlen (temp);
            if (x < 0)  x = 0;
            t3[x] = 0;
            strcat (t2, t3);
          }

         strcat (temp, t2);
         }

/*
 * and actually print it.
 *
 */

      if (really)
         {
         switch (jst)
            {
            case  0:  printf ("%s%s", spc, temp);  break;
            case  1:  x = ((w - strlen (temp)) / 2);  x = max (x, 0);
                      if (w == wid)  x += l;
                      sprintf (t2, FULL_WIDTH, "");  t2[x] = 0;
                      printf ("%s%s", t2, temp);   break;
            case  2:  x = (w - strlen (temp)); x = max (x, 0);
                      if (w == wid)  x += l;
                      sprintf (t2, FULL_WIDTH, "");  t2[x] = 0;
                      printf ("%s%s", t2, temp);   break;
            }
         if (! cnt)  w  = wid,           l  = lef_mar;
         else        w -= strlen (temp), l += strlen (temp);
         if (! cnt)  printf (SNGCR);
         }
      }
}

int
fill_data (ptr, idx, str)
dataptr    ptr;
int             idx;
char                *str;
{
   char    temp[128], t2[5];
   dataptr x;
   long    y;
   int     i, j, k, f;

   if (idx < 0 || idx >= rel->num_i)  return 0;

   i = 0;  /* Left edge of word in str */
   for (k = 0; ; k++)
      {
      if (! str[i])  break;
      for (j = i; str[j] && str[j] != ','; j++)
         ;
      strcpy (temp, &str[i]);  temp[j-i] = 0;  i = j;

      strzcpy (t2, &rel->idxs[idx][3*k +3], 3);  f = atoi (t2);

      x = (dataptr)((char *)ptr + rel->start[f]);  /* Position of field */

      switch (rel->type[f])
         {
         case T_SHORT:   *(short   *)x = (short) atoi (temp);  break;
         case T_USHORT:  *(ushort  *)x = (ushort)atoi (temp);  break;
         case T_LONG:    *(long    *)x = (long)  atol (temp);  break;
         case T_ULONG:   *(ulong   *)x = (ulong) atol (temp);  break;
         case T_FLOAT:   *(float   *)x = (float) atof (temp);  break;
         case T_DOUBLE:
         case T_MONEY:   *(double  *)x = (double)atof (temp);  break;
         case T_TIME:    *(mb_time *)x = scn_time (temp);      break;
         case T_DATE:    *(mb_date *)x = scn_date (temp);      break;
         case T_SERIAL:  *(long    *)x = (long)  atol (temp);  break;
         default:        strcpy (x, temp);                     break;
         }

      if (rel->type[f] == T_MONEY)
         {
         y = (long)(100.0 * (double)atof (temp));
         *(double *)x = (double)y / 100.0;
         }
      }
   return 1;
}

