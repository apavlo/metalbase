/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#define PARSE_C
#include "mbase.h"

long  _lpos = 0L;
int   quoted = 0;

int
skip (f, s)  /* 0 means didn't skip the word, 1 means we did. */
int   f;
char    *s;
{
   int   i;
   char  a;

   i = 0;
   _lpos = lseek (f, 0L, 1);

   while (s[i] != 0)
    {
      if (read (f, &a, 1) != 1)  return -1;

      if (i != 0 || (i==0 && !iswhite(a)))
       { if (s[i] != tolower(a))
          { lseek (f, -1L -(long)i, 1);
            break;
          }
         else
            i++;
       }
    }

   return (s[i] == 0);
}

char *
getword (fle)
int      fle;
{
   int          okay = 1, go = 0;
   static char  buffer[256];
   char        *ptr,   a;

   while (read (fle, &a, 1) == 1)
      if (! iswhite (a))  break;
   _lpos = lseek (fle, 0L, 1)-1L;

   quoted = 0;

   for (ptr = buffer; okay; okay = read (fle, &a, 1))
    { if (go == 1 && !quoted && istoken (a))
       { lseek (fle, -1L, 1);  /* Backup--we don't want the token yet */
         break;
       }
      if (a == '\"')
         if (quoted)  break;
         else
          { quoted = 1;
            continue;
          }

      if (quoted)  *ptr = a;
      else
         if (iswhite(a)) break;
         else            *ptr = tolower(a);

      ptr++; go = 1;
      if (! quoted && istoken (a))  break;
    }
   *ptr = 0;

   return buffer;
}

void
goeol (fle, str)
int    fle;
char       *str;
{
   char  a, *ptr;
   char  f;

   _lpos = lseek (fle, 0L, 1);

   for (ptr = str; read (fle, &a, 1) == 1; )
    { if (a == '\n' || a == '\r')  break;
      if (ptr != NULL)  { *ptr = a; ptr++; }
    }
   if (ptr != NULL)  *ptr = 0;

   if (read (fle, &f, 1) == 1)
      if ((f != '\n' && f != '\r') || f == a)
         lseek (fle, -1L, 1);
}

