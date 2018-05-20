/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#ifdef MSDOS
#include <stdio.h>
#include <stdlib.h>
#endif

struct              /* MP_OPTS: -o=str */
 { char   a[13];
   long   b;
   char   c[7];
   double d;
   char   e[3];
 } test;

#define STDPOS(x) x-(char*)&test
#define CHRPOS(x) (char *)&x-(char *)&test

/*
 * In general:
 *
 * STRUCT_1 -- SunOS 4, DECStation/ULTRIX
 *
 * STRUCT_2 -- COHERENT, DOS/Zortec C
 *
 * STRUCT_3 -- SunOS 3, Xenix
 *
 * STRUCT_4 -- DOS/Microsoft C, Mac Programmers' Workshop
 *
 */

int match_1 = 0;  /* Incremented 1 for each that matches */
int match_2 = 0;  /* Incremented 1 for each that matches */
int match_3 = 0;  /* Incremented 1 for each that matches */
int match_4 = 0;  /* Incremented 1 for each that matches */

int array[][4] =
   { {  0,    0,    0,    0 },
     { 16,   13,   16,   14 },
     { 20,   17,   20,   18 },
     { 32,   24,   28,   26 },
     { 40,   32,   36,   34 },
     { 48,   35,   40,   38 }  };

void
printl (s, n, l)
char   *s;
int        n, l;
{
   printf ("%-6.6s : %2d -- %2d,   %2d,   %2d,   %2d\n", s, n,
            array[l][0], array[l][1], array[l][2], array[l][3]);

   if (n == array[l][0])  match_1++;
   if (n == array[l][1])  match_2++;
   if (n == array[l][2])  match_3++;
   if (n == array[l][3])  match_4++;
}

void
main ()
{
   int  n;

   printf ("\n");
   printf ("        REAL  STR1  STR2  STR3  STR4\n");
   printf ("        ----  ----  ----  ----  ----\n");
   printl ("test.a", STDPOS(test.a), 0);
   printl ("test.b", CHRPOS(test.b), 1);
   printl ("test.c", STDPOS(test.c), 2);
   printl ("test.d", CHRPOS(test.d), 3);
   printl ("test.e", STDPOS(test.e), 4);
   printl ("size  ", sizeof(test),   5);
   printf ("\n");

   n = 0;
   if (match_1 == 6)  n |= 0x01;
   if (match_2 == 6)  n |= 0x02;
   if (match_3 == 6)  n |= 0x04;
   if (match_4 == 6)  n |= 0x08;

   if (! n || (n != 1 && n != 2 && n != 4 && n != 8))
      {
      printf ("Your compiler does not match any of the expected values.\n");
      printf ("Look in your compiler manuals for any compile-time switches\n");
      printf ("which control structure-packing; add such a switch to\n");
      printf ("CFLAGS in the makefile, and recompile struct.\n");
      n = 0;
      }
   else
      {
      if (n == 4)  n = 3;
      if (n == 8)  n = 4;

      printf("For your compiler, you need to #define STRUCT_%d when\n", n);
      printf("compiling the library... do this by setting -DSTRUCT_%d in\n",n);
      printf("the makefile.\n");
      }
   printf ("\n");

   exit (0);
}

