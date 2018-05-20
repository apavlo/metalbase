/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#define TIMEDATE_C
#include "mbase.h"

#define TMsHR(x,n) x|=((((ulong)(n))&   31) << 19)
#define TMsMN(x,n) x|=((((ulong)(n))&   63) << 13)
#define TMsSC(x,n) x|=((((ulong)(n))&   63) <<  7)
#define TMsMI(x,n) x|= (((ulong)(n))&  127)
#define DTsYR(x,n) x|=((((ulong)(n))& 8191) <<  9)
#define DTsMO(x,n) x|=((((ulong)(n))&   15) <<  5)
#define DTsDY(x,n) x|= (((ulong)(n))&   31)
#define TMgHR(x)   (int)((x >> 19) &   31)
#define TMgMN(x)   (int)((x >> 13) &   63)
#define TMgSC(x)   (int)((x >>  7) &   63)
#define TMgMI(x)   (int)(x &          127)
#define DTgYR(x)   (int)((x >>  9) & 8191)
#define DTgMO(x)   (int)((x >>  5) &   15)
#define DTgDY(x)   (int)(x & 31)

struct tm *
_getlt ()                 /* == Get LocalTime, in {struct tm *} */
{
   time_t date;
   time (&date);          /* If your compiler complains, try date=time(); */
   return localtime (&date);
}

long
elap_t (tme)
mb_time tme;
{
   long     dif;
   mb_time  now;
   now = curtime();
   dif  = 3600L * (long)(TMgHR(now) - TMgHR(tme));
   dif +=   60L * (long)(TMgMN(now) - TMgMN(tme));
   dif +=    1L * (long)(TMgSC(now) - TMgSC(tme));
   return dif;
}

mb_time
tmtotime  (tim)  /* Pass NULL to get current time */
struct tm *tim;
{
   struct tm  *ptr;
   mb_time     ret;
   ptr = (tim == (struct tm *)0) ? _getlt() : tim;
   ret = (mb_time)0;
   TMsHR(ret, ptr->tm_hour);
   TMsMN(ret, ptr->tm_min);
   TMsSC(ret, ptr->tm_sec);
   TMsMI(ret, 0);
   return ret;
}

mb_date
tmtodate  (tim)
struct tm *tim;
{
   struct tm  *ptr;
   mb_date     ret;
   ptr = (tim == (struct tm *)0) ? _getlt() : tim;
   ret = (mb_date)0;
   DTsYR(ret, ptr->tm_year+5996L); /* 1992 : tm_year = 92, ret.year = 6178 */
   DTsMO(ret, ptr->tm_mon+1L);
   DTsDY(ret, ptr->tm_mday);
   return ret;
}

struct tm *
datetimetotm (dat, tim)
mb_date       dat;
mb_time            tim;
{
   static struct tm ret, *ptr;
   dat = (! dat) ? curdate() : dat;
   tim = (! tim) ? curtime() : tim;
   ptr = _getlt();
   ptr->tm_sec  = TMgSC(tim);
   ptr->tm_min  = TMgMN(tim);
   ptr->tm_hour = TMgHR(tim);
   ptr->tm_mday = DTgDY(dat);
   ptr->tm_mon  = DTgMO(dat)-1;
   ptr->tm_year = (int)(DTgYR(dat)-5996L);
   ptr->tm_wday = ptr->tm_yday = ptr->tm_isdst = 0;
   return &ret;
}

char *
fmt_date (dat, opt)
mb_date   dat;
int            opt;
{
   static char buf[20];
   if (DTgYR(dat) < 3096)  DTsYR(dat,4096);
   switch (opt)
    { case 1: sprintf (buf, "%02d/%02d/%02d", DTgMO(dat),
                             DTgDY(dat), DTgYR(dat)-5996L);
               break;
      case 2: sprintf (buf, "%02d%02d%02d", DTgYR(dat)-5996L,
                             DTgMO(dat), DTgDY(dat));
               break;
      default:sprintf (buf, "%02d/%02d/%04d", DTgMO(dat),
                             DTgDY(dat), DTgYR(dat)-4096L);
               break;
    }
   return buf;
}

char *
fmt_time (tim, opt)
mb_time   tim;
int            opt;
{
   static char buf[20];
   switch (opt)
    { case  1:  sprintf (buf, "%02d:%02d %s", (TMgHR(tim) % 12),
                         TMgMN(tim), (TMgHR(tim) >= 12) ? "pm" : "am");
               break;
      case  2:  sprintf (buf, "%02d:%02d", TMgHR(tim),
                         TMgMN(tim));
               break;
      default: sprintf (buf,"%02d:%02d:%02d", TMgHR(tim),
                         TMgMN(tim), TMgSC(tim));
               break;
    }
   return buf;
}

mb_date
scn_date (str)
char     *str;
{
   char     buf[80];
   char    *a, *b;
   long     x;
   mb_date  rtn = (mb_date)0;

   if (! str || ! *str)  return rtn;
   strcpy (buf, str);
   if ((a=strchr (buf, '/'))==NULL)
    {
      strzcpy (buf, &str[0], 2);  DTsYR (rtn, atol (buf)+5996L);
      strzcpy (buf, &str[2], 2);  DTsMO (rtn, atol (buf));
      strzcpy (buf, &str[4], 2);  DTsDY (rtn, atol (buf));
      return rtn;
    }
   b=strchr(str,'/')+1;

   *a = 0;  DTsMO (rtn, atol (buf));  strcpy (buf, b);
   if ((b=strchr (b, '/'))==NULL)  return rtn;

   a=strchr(buf, '/'); *a = 0; DTsDY(rtn, atol(buf));

   strcpy (buf, b+1);  x=atol(buf);
   if (strlen (buf) < 3)  x += 1900;
   DTsYR (rtn, x + 4096);

   return rtn;
}

mb_time
scn_time (str)
char     *str;
{
   char     buf[80];
   char    *a, *b;
   long     x;
   mb_time  rtn = (mb_time)0;

   if (! str || ! *str)  return rtn;
   strcpy (buf, str);
   if ((a=strchr (buf, ':'))==NULL)  return rtn;
   b=strchr(str,':')+1;

   *a = 0;  TMsHR(rtn, atol (buf));  strcpy (buf, b);
   if ((a=strchr (b, ':'))==NULL)
    { x=TMgHR (rtn);  rtn=(mb_time)0;  TMsMN (rtn, atol(buf));
      if ((a=strchr (str, 'p'))==NULL)  a=strchr (str, 'P');
      if (a)
         if ((*(a+1) == 'm' || *(a+1) == 'M') && x < 12)
            x += 12;
      TMsHR (rtn, x);
      return rtn;
    }
   b=strchr(buf,':');  *b = 0;  TMsMN (rtn, atol (buf));

   strcpy (buf, a+1);  TMsSC (rtn, atol (buf));
   
   return rtn;
}

mb_time
add_time (str)
char     *str;
{
   long     s,m,h;
   mb_time  now, tmp;

   now = curtime  ();
   tmp = scn_time (str);

   s = TMgSC (now) + TMgSC (tmp);
   m = TMgMN (now) + TMgMN (tmp);
   h = TMgHR (now) + TMgHR (tmp);

   while (s >= 60L)  { s -= 60L; m += 1L; }
   while (m >= 60L)  { m -= 60L; h += 1L; }
   while (h >= 24L)  { h -= 24L;          }

   tmp = 0L;

   TMsSC (tmp, s);
   TMsMN (tmp, m);
   TMsHR (tmp, h);

   return tmp;
}

char *
fmt_phone (ac, pre, num, ext, opt)  /* OPT: 1=use () for AC, 0=don't     */
long       ac, pre, num, ext;       /* (-1==internal use only--tis ugly) */
int                           opt;
{
   static char buf[25];

   buf[0] = 0;

   if (!ac && !pre && !num && !ext)
      return buf;

   if (opt != -1)
      {
      opt = (opt == 0 || opt == 3) ? 1 : 0;

      if (! ext)  opt = (opt == 0) ? 3 : 4;
      if (! ac)   opt = (opt <= 1) ? 2 : 5;
      }

   switch (opt)
      {
      case -1: sprintf (buf, "%03ld-%03ld-%04ldx%05ld",ac,pre,num, ext); break;
      case  1: sprintf (buf, "%ld-%ld-%04ld x%ld",     ac,pre,num, ext); break;
      case  2: sprintf (buf, "%ld-%04ld x%ld",            pre,num, ext); break;
      case  3: sprintf (buf, "(%ld) %ld-%04ld",        ac,pre,num);      break;
      case  4: sprintf (buf, "%ld-%ld-%04ld",          ac,pre,num);      break;
      case  5: sprintf (buf, "%ld-%04ld",                 pre,num);      break;
      default: sprintf (buf, "(%ld) %ld-%04ld x%ld",   ac,pre,num, ext); break;
               break;
      }
   return buf;
}

void
scn_phone (ac, pre, num, ext, str)
long      *ac,*pre,*num,*ext;
char                            *str;
{
   char *a, *b, buf[128];

   *ac = *pre = *num = *ext = 0L;
   strcpy (buf, str);

   if ((a = strchr (buf, '(')) != NULL)
      {
      if ((b = strchr (a, ')')) == NULL)  return;
      *b = 0;  b++;
      *ac = atol (1+a);
      }
   else
      {
      if ((a = strchr (buf, '-')) == NULL)  return;
      if (strchr (1+a, '-') != NULL)
         {
         b = a; *b = 0;  b++;
         *ac = atol(buf);
         }
      else
         {
         b = &buf[0];  /* No area code */
         }
      }

   if ((a = strchr (b, '-')) == NULL)  { *ac = 0; return; }
   *a = 0; a++;
   *pre = atol (b);

   if ((b = strpbrk (a, " xX")) != NULL)
      {
      *b = 0;
      b++;
      }

   *num = atol (a);

   if (b != NULL)
      {
      if ((a = strpbrk (b, "xX")) != NULL)
         b = 1+a;

      *ext = atol (b);
      }
}


