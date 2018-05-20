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

#define ENTRY_C
#include "mbase.h"

/*
 * Prototypes
 *
 */

WINDOW     *win;

#ifdef LONGARGS
   int   _std_val  (de_form *);
   void  _getnext  (de_form *);
   int   _validopt (de_form *, field *, char);
#else
   int   _std_val();
   void  _getnext();
   int   _validopt();
#endif

 /***************************************************************************/

/*
 * form->key will be:
 *  -1 == Abort transaction--they hit CTRL-C or CTRL-Q or some such.
 *   1 == Accept transaction--they hit CTRL-A or some such.
 *  Otherwise, it's the key they hit to get out of the field; look at
 *     input.c to see what it returns.
 *
 */

int
_std_val (form)
de_form  *form;
{
   if (form->key == -1 || form->key == 1)  return form->key;
   return 0;
}

void
_getnext (form)
de_form  *form;
{
   int  mv;

   if (form->key == -1 || form->key == 1)  return;

   form->nextfield = form->curfield;

   switch (form->key)
    { case '-': case 'k':            mv = -1;  break;
      case  0 : case '+': case 'j':  mv =  1;  break;
      default :                      mv =  0;  break;
    }
   if (mv == 0)
      if (form->fields[form->curfield].inout&FM_IN)  return;
      else
         mv = 1;

   for (;;)
    { form->nextfield += mv;

      if (form->nextfield <  0)
         form->nextfield=form->numfields-1;
      if (form->nextfield >= form->numfields)
         form->nextfield=0;

      if (form->fields[form->nextfield].inout & FM_IN)
         break;
    }
}

/*
 * validopt() is used to check a given Choice field, to make sure its data
 * is valid.  It also looks for any Link fields which reference it, and
 * sets/refreshes those.
 *
 */

int
_validopt (form, fld, old)
de_form   *form;
field           *fld;
char                  old;
{
   int   i, j;
   char  now;

   now = *(char *)(fld->buffer);

   if (now == 0)
      {
      i = 10;
      }
   else
      {
      for (i = 1; i < 10 && fld->opt_arr[i][0] != 0; i++)
         {
         if (strchr (fld->opt_arr[i], now) != NULL)
            break;
         }
      if (i == 10 || fld->opt_arr[i][0] == 0)
         {
         *((char *)(fld->buffer))   = old;
         *((char *)(fld->buffer)+1) = 0;
         if (old != 0)  return 0;
         i = 10;
         }
      }

   for (j = 0; j < form->numfields; j++)
      if (form->fields[j].option == 2 &&
          ! strcmp (form->fields[j].opt_arr[0], fld->name))
      {
      if (i == 10)
         *(char *)(form->fields[j].buffer) = 0;
      else
         strcpy (form->fields[j].buffer, form->fields[j].opt_arr[i]);
      fm_refresh (form, &(form->fields[j]));
      }

   return 1;
}

 /***************************************************************************/

int
fm_fldnum (form, str)
de_form   *form;
char            *str;
{
   char         *ptr;
   register int  i;

   if (!form || !str || !*str)  return -1;

   for (i = 0; i < form->numfields; i++)
      if (! strcmp (form->fields[i].name, str))  return i;

   for (i = 0; i < form->numfields; i++)
      if ((ptr = strchr (form->fields[i].name, '.')) != NULL)
         if (! strcmp (ptr+1, str))  return i;

   return -1;
}

/*
 * formtorel() is great.  It takes any data in a data-entry form and moves
 * it into the appropriate places in a relation's structure... it goes by
 * name, and only those fields in both are copied.  Wonderful toy.  Pass
 * it pointers to the form and relation, and the structure record for the
 * relation.  reltoform() works the same way but backwards, moving data from
 * a relation's record into a form, BUT MAKE SURE YOU CHECK THE ARGUMENTS
 * FIRST.  They're backwards.  On purpose.
 *
 */

void
formtorel (form, rel, ptr)
de_form   *form;
relation        *rel;
dataptr               ptr;
{
   char          temp[80], *a, *b;
   dataptr       frm;
   register int  i;
   int           n;

   if (! form || ! rel || ! ptr)  return;

   for (n = 0; n < rel->num_f; n++)
      {
      sprintf (temp, "%s.%s", rel->relname, rel->name[n]);
      if ((frm = fm_data (form, temp)) != NULL)
         {
         a=(char *)ptr +rel->start[n];
         b=(char *)frm;
         for (i = 0; i < rel->siz[n]; i++, a++, b++)
            *a = *b;
         }
      }
}

void
reltoform (rel, form, ptr)
relation  *rel;
de_form        *form;
dataptr               ptr;
{
   char          temp[80];
   dataptr       frm;
   register int  i;
   int           n;

   if (! form || ! rel || ! ptr)  return;

   for (n = 0; n < rel->num_f; n++)
    { sprintf (temp, "%s.%s", rel->relname, rel->name[n]);
      if ((frm = fm_data (form, temp)) != NULL)
         for (i = 0; i < rel->siz[n]; i++)
            ((char *)frm)[i] = ((char *)ptr)[i +rel->start[n]];
    }
}

/*
 * Refresh a field.  Basically redisplays whatever's in it, and doesn't
 * move your current field number.
 *
 */

void
fm_refresh (form, fld)
de_form    *form;
field            *fld;
{
   if (! fld)  return;
   move (form->y +fld->y, form->x +fld->x);
   display (fld->buffer, fld->type, fld->len);
   if (fld->option == 1)  (void)_validopt (form, fld, 0);
}

/*
 * Refresh the whole forsaken form.
 *
 */

void
fm_refrall (form)
de_form    *form;
{
   register int  i;
   field        *fld;

   if (! form)  return;

   for (i = 0; i < form->numfields; i++)
    { fld = &(form->fields[i]);
      move (form->y +fld->y, form->x +fld->x);
      display(form->fields[i].buffer,form->fields[i].type,form->fields[i].len);
      if (form->fields[i].option == 1)
         (void)_validopt (form, &form->fields[i], 0);
    }
}

/*
 * zero all data in the form.  Yayy.....
 *
 */

void
fm_zero (form)
de_form *form;
{
   register int  i;
   dataptr       ptr;

   for (i = 0; i < form->numfields; i++)
      {
      ptr = form->fields[i].buffer;
      switch (form->fields[i].type)
         {
         case T_SHORT:   *(short  *)ptr = (short) 0;  break;
         case T_USHORT:  *(ushort *)ptr = (ushort)0;  break;
         case T_TIME:
         case T_DATE:
         case T_SERIAL:
         case T_LONG:    *(long   *)ptr = (long)  0;  break;
         case T_ULONG:   *(ulong  *)ptr = (ulong) 0;  break;
         case T_FLOAT:   *(float  *)ptr = (float) 0;  break;
         case T_DOUBLE:
         case T_MONEY:   *(double *)ptr = (double)0;  break;
         default:        *(char   *)ptr = 0;          break;
         }
      }
}

/*
 * Give fm_data a form pointer, and a name (like "sample.custnum"), and it
 * will return a pointer to the buffer used to contain sample.custnum's number
 * for data-entry.  Useful, eh?
 *
 */

dataptr
fm_data (form, str)
de_form *form;
char          *str;
{
   int  i;
   if (! form || ! str)  return NULL;
   if ((i = fm_fldnum (form, str)) < 0)  return NULL;
   return form->fields[i].buffer;
}

/*
 * Set a mode... those basically just determine which fields you can get into
 * and which you can't.  Logically, it helps to remember what you're doing
 * on the data-entry form in the first place too.
 *
 */

void
fm_mode (form, n)
de_form *form;
int            n;
{
   register int  i;
   if (!form || n < 0 || n > form->nummodes)  return;
   form->curmode = n;
   for (i = 0; i < form->numfields; i++)
      if (form->curmode == 0)
         form->fields[i].inout = FM_INOUT;
      else
         form->fields[i].inout = form->fields[i].mode[form->curmode-1];
}

/*
 * do_form()'s flow:
 *   display the data-entry form
 *   call fm_mode() to set the current mode
 *   call fm_refrall()
 *   set valid_fn if it's not set already
 *   .-> call input() on the appropriate field  <---------------.
 *   |   if in a choice field, verify its validity --(invalid)--'
 *   |   call _getnext() to set the next field
 *   |   call (*valid_fn)() to validate the transaction:
 *   `---- if returns 0
 *
 */

   /* do_form()  returns 0 if failed, -1 if aborted, and 1 if accepted. */

int
do_form (form)
de_form *form;
{
   register int  i;
   int           wedidit = 0;
   char          buffer;
   field        *fld;

   if (!form)  return 0;

   if (! win)
    { wedidit = 1;  init_curses();
      savetty(); raw(); noecho(); nl();
    }
   clear(); refresh();

   for (i = 0; i < form->numlines; i++)
      mvaddstr (i+form->y, form->x, form->_scrn[i]);
   refresh();

   fm_mode    (form, form->curmode);
   fm_refrall (form);

   if (form->valid_fn == (int (*)())0)  form->valid_fn = _std_val;

   for (;;)
      {
      fld = &(form->fields[form->curfield]);
      move (form->y +fld->y, form->x +fld->x);
      if (fld->option == 1)  buffer = *(char *)(fld->buffer);

      do   form->key = input (fld->buffer, fld->type, fld->len);
      while (fld->option == 1 && !_validopt (form, fld, buffer));

      _getnext (form);
      if ((i = (*form->valid_fn)(form)) != 0)  break;
      form->key = '.';

      form->curfield = form->nextfield;
      _getnext (form);
      form->curfield = form->nextfield;
      }

/*
 * The second _getnext() function is used to ensure the user hasn't placed
 * us on a bad field--one which we normally couldn't get to.
 *
 */

   if (wedidit)
      {
      clear(); refresh(); resetty(); endwin();
      win = (WINDOW *)0;
      }
   return i;
}

