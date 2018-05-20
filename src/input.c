/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

   /**********************************************************************/
   /* Compile with -DTROUBLE if you have usleep() and your arrows act up */
   /**//**************************************************************//**/
       /* Compile with -DVI_EMU if you really want some vi emulation */
       /**************************************************************/

#define INPUT_C
#include "mbase.h"

#ifndef linux
#ifndef MSDOS 
#ifdef LONGARGS
   extern long   atol(char *);
   extern double atof(char *);
#else
   extern long   atol();
   extern double atof();
#endif
#endif
#endif

#ifdef LONGARGS
   void display (dataptr, int, int);
   char getarr  (void);
   char input   (dataptr, int, int);
#else
   void display();
   char getarr();
   char input();
#endif

/*
 * An option at compile time:  if you want the field to be accepted (as with
 * a down arrow) automatically when the user has filled it completely, define
 * ADVANCE_AT_END as below.  This makes DE a bit more natural for Choice
 * fields and the like.
 *
 */

#define ADVANCE_AT_END

#define DELAY_TIME 10

#define movech(c,y,x)  move(y,x);refresh();c=getarr();

#ifndef ESC
#define ESC (char)27
#endif

static int  ins = 1;

char  str[150], org[150];
int   pos,y,x,len,tgt,inslt,cln,esc;

/*
 * getarr() functions like getch(), but returns special codes for arrow
 * keys as well.
 *
 */

#ifdef USE_CURKEY

char
getarr ()
{
   int   ch;

   do  ch = (int)getch();
   while (ch == 0);

   if (ch == KEY_UP)     return (char)AR_UP;
   if (ch == KEY_DOWN)   return (char)AR_DOWN;
   if (ch == KEY_LEFT)   return (char)AR_LEFT;
   if (ch == KEY_RIGHT)  return (char)AR_RIGHT;
   if (ch == KEY_IC)     return (char)AR_INS;
   if (ch == KEY_DC)     return (char)AR_DEL;
   if (ch == KEY_HOME)   return (char)AR_HOME;
   if (ch == KEY_LL)     return (char)AR_END;
   if (ch == KEY_PPAGE)  return (char)AR_PGUP;
   if (ch == KEY_NPAGE)  return (char)AR_PGDN;

   return (char)ch;
}

#else

char
getarr ()
{
   register int   x;
   char          *a,*b,*c,*d,*e,*f,*g,*h,*i,*j,ch;
   static char   *up="\033[A";
   static char   *down="\033[B";
   static char   *left="\033[D";
   static char   *right="\033[C";
   static char   *ins="\033[@";
   static char   *del="\033[P";
   static char   *home="\033[H";
   static char   *end="\033[24H";
   static char   *pgup="\033[V";
   static char   *pgdn="\033[U";

   do
    { ch = (char)getch();  if (ch < 0)  ch = 0;

      if (ch != *(a= up))     a=NULL;  else a++;
      if (ch != *(b= down))   b=NULL;  else b++;
      if (ch != *(c= left))   c=NULL;  else c++;
      if (ch != *(d= right))  d=NULL;  else d++;
      if (ch != *(e= ins))    e=NULL;  else e++;
      if (ch != *(f= del))    f=NULL;  else f++;
      if (ch != *(g= home))   g=NULL;  else g++;
      if (ch != *(h= end))    h=NULL;  else h++;
      if (ch != *(i= pgup))   i=NULL;  else i++;
      if (ch != *(j= pgdn))   j=NULL;  else j++;

      if (!a && !b && !c && !d && !e && !f && !g && !h && !i && !j)  break;

#if !defined(MSDOS) || !defined(AMIGA)
      fcntl (0, F_SETFL, O_NDELAY);  /* Turn off waiting for keys */
#endif

      for (;;)
       { if (a && !*a) { ch = AR_UP;     break; }
         if (b && !*b) { ch = AR_DOWN;   break; }
         if (c && !*c) { ch = AR_LEFT;   break; }
         if (d && !*d) { ch = AR_RIGHT;  break; }
         if (e && !*e) { ch = AR_INS;    break; }
         if (f && !*f) { ch = AR_DEL;    break; }
         if (g && !*g) { ch = AR_HOME;   break; }
         if (h && !*h) { ch = AR_END;    break; }
         if (i && !*i) { ch = AR_PGUP;   break; }
         if (j && !*j) { ch = AR_PGDN;   break; }

         if (!a && !b && !c && !d && !e && !f && !g && !h && !i && !j)
          { ch=*(up);
            break;
          }
         for (x=0; x < DELAY_TIME; x++)       /* DELAY_TIME quick reads */
          {
            if ((ch = (char)getch()) > 0)  break;
#ifdef TROUBLE
            usleep (100);
#endif
          }
         if (x == DELAY_TIME)
          { ch=*(up);
            break;
          }
         a=(a==NULL || ch != *a) ? NULL : a+1;
         b=(b==NULL || ch != *b) ? NULL : b+1;
         c=(c==NULL || ch != *c) ? NULL : c+1;
         d=(d==NULL || ch != *d) ? NULL : d+1;
         e=(e==NULL || ch != *e) ? NULL : e+1;
         f=(f==NULL || ch != *f) ? NULL : f+1;
         g=(g==NULL || ch != *g) ? NULL : g+1;
         h=(h==NULL || ch != *h) ? NULL : h+1;
         i=(i==NULL || ch != *i) ? NULL : i+1;
         j=(j==NULL || ch != *j) ? NULL : j+1;
       }

#if !defined(MSDOS) || !defined(AMIGA)
      fcntl (0, F_SETFL, 0);
#endif
    } while (! ch);

   return ch;
}

#endif

void
display (buf, typ, siz)
dataptr  buf;
int      typ;
int      siz;
{
   int   b, a;
   long  ac, num, pre, ext;
   long  tlong;
   char  temp[22];

   getyx    (win,b,a);
   sprintf  (str, "%-132.132s", "");  str[siz] = 0;
   mvaddstr (b,a, str);

   switch (typ)
      {
      case T_CHAR:    strzcpy (str, buf,   siz);                      break;
      case T_SHORT:   sprintf (str, "%d",  (int)*(short  *)buf);      break;
      case T_USHORT:  sprintf (str, "%u",  (int)*(ushort *)buf);      break;
      case T_LONG:    sprintf (str, "%ld", *(long   *)buf);           break;
      case T_ULONG:   sprintf (str, "%lu", *(ulong  *)buf);           break;
      case T_FLOAT:   sprintf (str, "%f",  *(float  *)buf);           break;
      case T_DOUBLE:  sprintf (str, "%lf", *(double *)buf);           break;
      case T_MONEY:   tlong = (long)(*(double *)buf * 100.0);
                      sprintf (str, "%-.2lf", (double)tlong / 100.0); break;
      case T_TIME:    strcpy (str, fmt_time (*(mb_time *)buf, 0));    break;
      case T_DATE:    strcpy (str, fmt_date (*(mb_date *)buf, 0));    break;
      case T_SERIAL:  sprintf (str, "%ld", *(long   *)buf);           break;
      case T_PHONE:   strzcpy (temp, buf, 20);
                      scn_phone (&ac, &pre, &num, &ext, temp);
                      strcpy (str, fmt_phone (ac,pre,num,ext, 0));    break;
      }
   str[siz] = 0;
   mvaddstr (b,a, str);
   move     (b,a);
   refresh  ();
}

/*
 * Returns:     0 -- Continue to next field
 *              1 -- Finished with DE
 *             -1 -- Abort DE
 *          other -- Field control (-/+/j/k)
 *
 */

char
input   (buf, typ, siz)
dataptr  buf;
int      typ;
int      siz;
{
   register int  i;
   char          c;
   int           y1,x1, start;
   long          tlong;
   long          ac, num, pre, ext;
   char          temp[22];

   inslt=pos=tgt=cln=esc=start=0;  raw();noecho();getyx(win,y,x);
   display (buf, typ, siz);        len=strlen(str);  strcpy (org,str);

   for (;;)
    { movech (c,y,x+pos);
      if (strchr (quit_chars, c))  break;
#ifdef VI_EMU
      if (c == ESC)
       { if (tgt)           tgt=0;
         else if (ins)      ins=0,pos--;
         else esc++;
         if (esc == 2)  { c = 'q'; break; }  /* Two ESC's aborts too */
         if (pos==-1)  pos=0;
         continue;
       }
#else
      if (c != ESC)  esc = 0;
      else
       { esc++;
         if (esc == 2)   { c = 'q';  break; }
       }
#endif
      if (c == CTRL_C)   { c = 'q';  break; }  /* Ctrl-C aborts */
      if (c == CTRL_Q)   { c = 'q';  break; }  /* Ctrl-Q aborts */
      if (c == AR_PGUP)  { c = 'k';  break; }
      if (c == AR_UP)    { c = 'k';  break; }
      if (c == AR_PGDN)  { c = 'j';  break; }
      if (c == AR_DOWN)  { c = 'j';  break; }
      if (c == CTRL_L)   { clearok (win, TRUE);  refresh();  continue; }
      if (c == AR_END)   { pos = len;  continue; }
      if (c == CTRL_U)   { strcpy  (str, org);
                           move (y, x); display (buf, typ, siz);
                           len=strlen(str); start = 0;
                           c = AR_HOME;  /* Pretend they hit HOME afterward. */
                         }
      if (c == AR_HOME)  { pos = 0;    continue; }

      if (c == AR_RIGHT)
       { if (pos < len)  pos++;
         continue;
       }
#ifndef VI_EMU
      if (c == AR_LEFT)
       { if (pos > 0)  pos--;
         continue;
       }
#endif
      esc = 0;
      if (ins)
       { if (c == '\b' || c == 127 || c == AR_LEFT || c == AR_DEL)
            if (pos != inslt)
             { pos--,len--,mvdelch(y,x+pos),mvinsch(y,x+siz-1,' ');
               for (i=pos; i<len; i++)
                  str[i] = str[i+1];
               str[i] = 0;
             }
         if (c == '\r' || c == '\n' || c == CTRL_A)  break;
         if (c < ' ' || c > 'z')  continue;
#ifndef VI_EMU
         if (pos == 0 && ! start)
          {
            for (i=0; i<len; i++)
               mvdelch(y,x),mvinsch(y,x+siz-1,' ');
            str[0] = 0;  len = 0;  move(y,x);  refresh();
          }
         start = 1;
#endif
         if (len == siz)  continue;
         insch(c);  mvdelch(y,x+siz);
         for (i=len-1; i>=pos; i--)
            str[i+1] = str[i];
         len++;  str[len] = 0;
         str[pos] = c;  pos++;
#ifdef ADVANCE_AT_END
         if (len == siz) { c = 'j'; break; }
#endif
         continue;
       }
      if (tgt)
       { if (c < ' ' || c > 'z' || len <= 1) { tgt=0; continue; }
         if (tgt < 3)
            for (i=pos-1; i>=0; i--)
               if (str[i] == c)  break;
         if (tgt > 2)
            for (i=pos+1; i<len; i++)
               if (str[i] == c)  break;
         if (i < 0 || i == len) { tgt=0; continue; }
         pos=i;
         if (tgt == 2) pos++;
         if (tgt == 3) pos--;
         tgt=0; continue;
       }
      if (cln==2) { move(23,0); clrtoeol(); cln=0; continue; }
      if (cln==1)
       { if (c > ' ' && c < 'z')
          { mvaddch (23, 1, c);
            c = (char)tolower (c);
            if (c == 'w' || c == 'x') { c = 'x'; break; }
            if (c == 'q')                        break;
          }
         cln=2; y=y1; x=x1;
         continue;
       }

      if (c=='\r' || c=='j' || c=='k' || c=='z' || c=='q' || c == CTRL_A ||
          c=='\n' || c=='+' || c=='-' || c=='Z' || c=='Q')  break;

      switch (c)
       { case 'F':  tgt = 1;  break;
         case 'T':  tgt = 2;  break;
         case 't':  tgt = 3;  break;
         case 'f':  tgt = 4;  break;
         case 'A':  pos=len;  ins=1;  break;
         case 'a':  if (pos != len)
                       pos++; ins=1;  break;
         case 'i':            ins=1;  break;
         case 'I':  pos=0;    ins=1;  break;
         case 'x':  if (len != 0)
                       len--,mvdelch(y,x+pos),mvinsch(y,x+siz-1,' ');
                    for (i=pos; i<len; i++)
                       str[i] = str[i+1];
                    str[i] = 0;
                    if (pos == len && pos != 0)  pos--;
                   break;
         case 'X':  if (pos != 0)
                     { pos--,len--,mvdelch(y,x+pos),mvinsch(y,x+siz-1,' ');
                       for (i=pos; i<len; i++)
                          str[i] = str[i+1];
                       str[i] = 0;
                     }
                   break;
         case ':':  cln=1; y1=y; x1=x; y=23; x=1;
                    move(23,0); clrtoeol(); mvaddch(23,0,':');  break;
         case '^':  pos=0;     break;
         case '_':  pos=0;     break;
         case '0':  pos=0;     break;
         case '$':  pos=len-1; if (pos==-1)  pos = 0;  break;
         case AR_RIGHT:
         case 'l':  if (pos < len-1)  pos++;  break;
         case ' ':  if (pos < len-1)  pos++;  break;
         case AR_LEFT:
         case 'h':  if (pos > 0)      pos--;  break;
         case 127:  if (pos > 0)      pos--;  break;
         case '\b': if (pos > 0)      pos--;  break;
         case 'D':  for (i=pos; i<len; i++)
                       mvdelch(y,x+pos),mvinsch(y,x+siz-1,' ');
                    len = pos;  pos--;  if (pos == -1)  pos = 0;
                    str[len] = 0;
                   break;
         case 'U':
         case 'u':  strcpy  (str, org);       move (y, x);
                    display (buf, typ, siz);  len=strlen(str);
                   break;
       }
      if (ins)  inslt=pos;
    }
   if (cln) { move(23,0);  clrtoeol();  }

   switch (typ)
      {
      case T_CHAR:    strncpy (buf, str, siz);                 break;
      case T_SHORT:   *(short  *)buf  = (short) atoi(str);     break;
      case T_USHORT:  *(ushort *)buf  = (ushort)atoi(str);     break;
      case T_LONG:    *(long   *)buf  = (long)  atol(str);     break;
      case T_ULONG:   *(ulong  *)buf  = (ulong) atol(str);     break;
      case T_FLOAT:   *(float  *)buf  = (float) atof(str);     break;
      case T_DOUBLE:  *(double *)buf  = (double)atof(str);     break;
      case T_MONEY:   tlong = (long)(atof(str) * 100.0);
                      *(double  *)buf  = (double)tlong / 100.0; break;
      case T_SERIAL:  *(long   *)buf  = (long)  atoi(str);     break;
      case T_TIME:    *(mb_time *)buf = scn_time (str);        break;
      case T_DATE:    *(mb_date *)buf = scn_date (str);        break;
      case T_PHONE:   strzcpy (temp, str, 20);
                      scn_phone (&ac, &pre, &num, &ext, temp);
                      strcpy (buf, fmt_phone (ac,pre,num,ext, 0));  break;
      }

   move    (y, x);
   display (buf, typ, siz);

   return (char) ( (c=='\r') ? (char)0
                 : (c=='x'||c=='Z'||c=='\n'||c==CTRL_A) ? (char)1
                 : (c=='q'||c=='Q') ? (char)-1
                 : tolower(c) );
}

void
init_curses ()
{
#if defined(MSDOS) || defined(AMIGA)
   initscr();
   win = stdscr;
#else
   win = initscr();
#endif
#ifdef USE_CURKEY
   keypad(win, TRUE);
#endif
}

