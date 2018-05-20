/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#undef MODULE

/*
 * Lemme explain the above line--it's not necessary 'cause MODULE isn't
 * defined anywhere else, but I put it in so I could put in this comment.
 * The headers created by "build" and "form" include actual data, that should
 * be local to only one module of a multiple .c-program executable; all the
 * others should have the headers' variables declared as external.  So the
 * headers use #ifdefs and check for the definition of MODULE--if it's there,
 * variables are declared external, if not, they're declared local.  That way,
 * variables always go in this one piece of .c code; if you were to link this
 * with any others, add "#define MODULE" to 'em and the headers will tell the
 * program that the variables therein are actually local to another C program.
 * Works rather well.
 *
 */

#include <mbase.h>

#include "sample.h"     /* Created during "% build sample.s"  */
#include "sampl_fm.h"  /* Created during "% form sample.frm" */

#ifdef LONGARGS
   void  main      (void);
   void  add_cust  (void);
   int   validate  (de_form *);
   void  do_datent (void);
   void  do_error  (char *);
   int   verify    (char *);
#else
   void  main();
   void  add_cust();
   int   validate();
   void  do_datent();
   void  do_error();
   int   verify();
#endif

#ifdef MSDOS
#define Standout()   /* For some reason, the MSDOS curses package I use   */
#define Standend()   /* craps out with standout and standend.  Try yours. */
#else
#define Standout() standout()
#define Standend() standend()
#endif

static sample_str arr[] =
 { { "Johnson Bill", 0L, 123.4567, 0L, 0L, "N", (ushort)15, "494-0220" },
   { "Calvin John",  0L,  23.00,   0L, 0L, "N", (ushort)3,  "216-881-2624"  },
   { "Moore Bob",    0L, 456.1234, 0L, 0L, "Y", (ushort)45, "882-8080x14651" },
   { "",             0L,   0.0,    0L, 0L, "",  (ushort)0,  ""  } };

relation   *samp;
int         iserr;
sample_str  rec;

void
main ()
{
   int       i;
   long      l;
   char      temp[80];

/*
 * First, if we need one, we've gotta get an encryption key for sample.rel:
 *
 */

#ifdef NOENCRYPT
   temp[0] = 0;
#else
   printf ("%s%s%sMetalBase 5.0 Sample________________________________%s",
            SNGCR, CLS, SNGCR, SNGCR);
   printf ("%sEncryption password : ", DUBCR);
   gets   (temp);
   printf (SNGCR);
#endif

   if ((samp = mb_inc ("sample", strtokey (temp))) == RNULL)
    { fprintf (stderr, "Database could not be opened (%s).%s", mb_error,SNGCR);
      mb_exit (1);
    }

   for (;;)
    { if ((l = mb_num(samp)) == 1L)
         printf ("%s%sThere is currently 1 customer in the database.%s",
                      CLS, SNGCR, DUBCR);
      else
         printf ("%s%sThere are currently %ld customers in the database.%s",
                 CLS, SNGCR, l, DUBCR);
      printf ("It is currently %s %s%s",
                fmt_time(curtime(),0), fmt_date(curdate(),0), SNGCR);

      printf (SNGCR);
      printf (" [D] - Add demonstration customers%s", SNGCR);
      printf (" [F] - Use data-entry form%s", SNGCR);
      printf (" [Q] - Quit%s", SNGCR);
      printf ("%sOption : ", SNGCR);
      gets (temp);  if ((i = tolower (temp[0])) == 'q')  break;

      switch (i)
       { case 'd':  add_cust  ();      gets(temp);  break;
         case 'f':  do_datent ();                   break;
         default:                                   break;
       }
    }

   printf ("%s%s%s", SNGCR, CLS, SNGCR);
   if (mb_rmv (samp))
    { fprintf (stderr, "Database could not be closed (%s).%s", mb_error,SNGCR);
      mb_exit (1);
    }
   mb_exit (0);
}

/*
 * Here, we add three customers to the relation.  Basically so you can have
 * some test data to play with, without thinking of anyone's names or
 * whatever; also helps demonstrate mb_add().
 *
 */

void
add_cust ()
{
   int  i, n;

   printf (SNGCR);
   for (n=0; ; n++)
      if (arr[n].custname[0] == 0)  break;
   for (i=0; i<n; i++)
    { printf ("Adding customer #%d/%d...", i+1, n);
      arr[i].date_en = curdate();
      arr[i].time_en = curtime();

/*
 * Note that the serial number is not set by us--it's assigned automatically
 * by the system.  We just send in 0L, or whatever happens to be there--
 * its value when passed is discarded.
 *
 */

      if (mb_add (samp, &arr[i]) == MB_OKAY)
         printf ("Successfully added%s", SNGCR);
      else
         printf ("%s%s", mb_error, SNGCR);

/*
 * Note that after a successful mb_add() call, the record's serial number
 * has been filled in automatically; if you looked in arr[i].custnum, the
 * number is now set.  Most engines require you to look up the record before
 * you can find out what number it was assigned; likewise, money values have
 * been truncated appropriately.  Nya nya nya, Informix.  :)
 *
 */

    }
   printf ("%sPress RETURN :", SNGCR);
}

/*
 * Here's the code for the data-entry stuff.  I'm including my notes
 * for when I first designed this, to press a point--data-entry is SIMPLE
 * if you're positive of what exactly you want BEFORE you start to code.
 * I suggest you do something exactly like what I've got here, and DE will
 * completely cease to be a problem for you.  :)  Likely, anyway...
 *
 * We've got:
 *
 *       +--------------------------------------------+
 *       |                                            |
 *       |  Customer Number...[custnum  ]             |
 *       |  Customer Name.....[custname  ]            |
 *       |                                            |
 *       |  Current Balance...${balance  }            |
 *       |  Accept Credit.....[A]..[B  ]              |
 *       |                                            |
 *       |  Date Entered......[date_en  ][time_en  ]  |
 *       |                                            |
 *       +--------------------------------------------+
 *
 * We want a menu that sez:
 *  Add  Find  Change  Delete
 *
 * Where
 * Add     == Mode 1 (everything inout, save date/time and custnum--
 *                 they're set by the system)
 * Find    == Mode 2 (only number and name inout, everything else out)
 * Change  == Mode 1 (same stuff as for Add, above)
 * Delete  == No mode--just a yes/no question.
 *
 * On second thought, let's not make it a menu.  Let's put 'em in the data-
 * entry form... since there's no data, we'll start with Find == mode 2.  When
 * they've entered something valid, we'll go on to Mode 1/Change; if they enter
 * something invalid, we'll go to Mode 1/Add.  If they're in Mode 1/Change..
 * hell, let's make it a mode 3.. If they're in Mode 3, and they hit
 * EOF==Ctrl-D, we'll delete the record if they want to--and they'll go back
 * to Mode 2 with no data.
 * Let's also let 'em move around.  Ctrl-N and Ctrl-P will select the Next
 * and previous records if they're in custnum or custname fields... if they're
 * not, print an error.  Those two keys will work in.. hummm.  If they're
 * in find mode, -N will do a FIRST, and -P will do a LAST; after any -N/-P,
 * it'll pull up the record & put 'em in change mode, same field.  From change
 * mode, they'll do NEXT and PREVIOUS, respectively... if you've updated the
 * data, it'll ask you if you wanna save it.
 *
 * So:
 *      Set Mode 2
 *      Set Field Custnum
 *      Go
 *       : If (selected Previous or Next)
 *       :  : if (! in custnum or custname)
 *       :  :  : error--can't search on any other fields (nya nya)
 *       :  : if (in mode 1 ("Add"))
 *       :  :  : error--can't search during add
 *       :  : if (in mode 2 ("Find"))
 *       :  :  : make 'em do FIRST and LAST
 *       :  :  : pull up record go to mode 3 ("Change")
 *       :  : if (in mode 3 ("Change"))
 *       :  :  : if they've updated the record, don't ask to save it  :)
 *       :  :  : pull up record
 *       :  : don't change fields
 *       :  : return 0 (okay)
 *       : If (mode 2 ("Find"))
 *       :  : If (Abort DE)
 *       :  :  : return -1 --abort
 *       :  : If (valid custnum or valid custname)
 *       :  :  : load information
 *       :  :  : change to mode 3 ("Change")
 *       :  : If (invalid custnum or invalid custname)
 *       :  :  : error--cannot find record (stay in field)
 *       :  : If (custname == "new")
 *       :  :  : clear the form and go to mode 1
 *       : If (mode 1 ("Add"))
 *       :  : If (EOF)
 *       :  :  : clear data--change to mode 2
 *       :  : If (Accept DE)
 *       :  :  : add new record
 *       :  :  : change to mode 3 ("Change")
 *       :  : If (Abort DE)
 *       :  :  : clear data--change to mode 2
 *       : If (mode 3 ("Change"))
 *       :  : If (EOF)
 *       :  :  : ask to delete--if so, clear data and change to mode 2 ("Find")
 *       :  : If (Accept DE)
 *       :  :  : add new record
 *       :  : If (Abort DE)
 *       :  :  : clear data--change to mode 2
 *      Done
 *
 */

void
do_datent ()
{
   de_form *form = &sample_fm; /* Shorthand, basically. */

   sprintf (quit_chars, "%c%c%c", CTRL_D, CTRL_P, CTRL_N);

   form->curmode  = 2;
   form->curfield = fm_fldnum (form, "custnum");
   form->valid_fn = validate;

/*
 * Oh yeah.  If we left looking at a record, let's pull it up and start in
 * Change mode:
 *
 */

/* (naw)
 *
 * if (mb_sel (samp, 0, &rec, CURRENT, NULL) == MB_OKAY)
 *  { form->curmode = 3;
 *    reltoform (samp, form, &rec);
 *  }
 *
 */

   do_form (form);
}

int
validate (form)
de_form  *form;
{
   int   idx, n;

   if (iserr)  do_error("");  /* Get rid of any old message */

   if (form->key == CTRL_P || form->key == CTRL_N)
    { idx = -1;
      if (form->curfield == fm_fldnum (form, "custnum"))
         idx = idxnum (samp, "ix_number");
      if (form->curfield == fm_fldnum (form, "custname"))
         idx = idxnum (samp, "ix_name");
      if (idx == -1)
       { do_error ("Ctrl-N and Ctrl-P only work on CustNum / CustName");
         form->nextfield = form->curfield;
         return 0;
       }
      if (form->curmode == 1)
       { do_error ("you may not search during an add operation");
         form->nextfield = form->curfield;
         return 0;
       }
      do_error ("wait...");
      if (form->curmode == 3)
       { if (mb_sel (samp, idx, &rec, (form->key==CTRL_P) ?PREV:NEXT, NULL))
          { do_error ("no more records in that direction");
            form->nextfield = form->curfield;
            return 0;
          }
       }
      if (form->curmode == 2)
       { if (mb_sel (samp, idx, &rec, (form->key==CTRL_P) ?LAST:FIRST, NULL))
          { do_error ("no more records in that direction");
            form->nextfield = form->curfield;
            return 0;
          }
         fm_mode (form, 3);
       }
      form->nextfield = form->curfield;
      reltoform  (samp, form, &rec);
      fm_refrall (form);
      do_error ("record found successfully");
      return 0;
    }

   switch (form->curmode)
    {
      case 2:  formtorel (form, samp, &rec);
               if (form->key == -1)
                  return -1;

               if (rec.custname[0] != 0)
                { idx = idxnum (samp, "ix_name");

                  if (! strcmp (rec.custname, "new"))
                   { fm_mode (form, 1);
                     form->nextfield = form->curfield;  /* Don't move */
                     fm_zero (form);
                     if ((n = fm_fldnum (form, "custname")) >= 0)
                        fm_refrnum (form, n);
                     do_error ("enter new record");
                     break;
                   }
                }
               else
                  if (rec.custnum != 0L)
                     idx = idxnum (samp, "ix_number");
                  else
                     break;

               do_error ("wait...");
               if (mb_sel (samp, idx, &rec, GTEQ, NULL) == MB_OKAY)
                { reltoform  (samp, form, &rec);
                  fm_refrall (form);
                  fm_mode (form, 3);
                  do_error ("record found successfully");
                  break;
                }
               do_error ("the specified record cannot be found.");
               form->nextfield = form->curfield;
              break;

      case 1:  if (form->key == 4)
                { fm_zero    (form);
                  fm_refrall (form);
                  fm_mode (form, 2);
                  do_error ("add aborted");
                  break;
                }
               if (form->key == 1)
                { formtorel (form, samp, &rec);
                  do_error ("wait...");
                  rec.date_en = curdate();
                  rec.time_en = curtime();
                  if (mb_add (samp, &rec) != MB_OKAY)
                     do_error (mb_error);
                  else
                   { do_error   ("record added successfully");
                     fm_mode    (form, 3);
                     reltoform  (samp, form, &rec);
                     fm_refrall (form);
                   }
                  break;
                }
               if (form->key == -1)
                { do_error   ("add aborted");
                  fm_mode    (form, 2);
                  fm_zero    (form);
                  fm_refrall (form);
                  formtorel  (form, samp, &rec);
                  break;
                }
              break;

      case 3:  if (form->key == 4)
                { if (! verify ("delete this record ? [yN] "))
                   { do_error ("delete aborted");
                     break;
                   }
                  do_error ("wait...");
                  if (mb_del (samp) != MB_OKAY)
                   { do_error (mb_error);
                     break;
                   }
                  fm_zero    (form);
                  formtorel  (form, samp, &rec);
                  fm_refrall (form);
                  do_error ("record deleted");
                  break;
                }
               if (form->key == 1)
                { formtorel (form, samp, &rec);
                  do_error ("wait...");
                  if (mb_upd (samp, &rec) != MB_OKAY)
                     do_error (mb_error);
                  else
                     do_error ("record updated successfully");
                  break;
                }
               if (form->key == -1)
                { do_error   ("change aborted");
                  fm_mode    (form, 2);
                  fm_zero    (form);
                  fm_refrall (form);
                  formtorel  (form, samp, &rec);
                  break;
                }
             break;
    }

   return 0;
}

/*
 * Stuff copied from vr.c to make life a little easier and a little prettier:
 *
 */

void
do_error (line)
char     *line;
{
   move (23, 0);   clrtoeol();  iserr = 0;
   if (! *line)  { refresh(); return; }
   Standout();     addstr (line);
   Standend();     refresh();   iserr = 1;
}

int
verify (str)
char   *str;
{
   char  c;
   do_error (str);
   for (;;)
    { c = getarr();
      switch (tolower(c))
       { case 'q': case 'n':  case ' ':   do_error (""); return 0; break;
         case  27: case '\r': case '\n':  do_error (""); return 0; break;
         case 'y':                        do_error (""); return 1; break;
       }
    }
}

