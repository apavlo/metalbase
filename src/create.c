/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#define CREATE_C
#include "mbase.h"
#include "internal.h"

/*
 ******************************************************************************
 *
 */

relation *
mb_new ()
{
   relation *rel;

   if ((rel = New (relation)) == RNULL)
      relerr (MB_NO_MEMORY, RNULL);

   rel->num_i  = 0;
   rel->num_f  = 0;
   rel->serial = 0L;

   relerr (MB_OKAY, rel);
}

mb_err
mb_addindex (rel, name, dups, desc)
relation    *rel;
char             *name,      *desc;
int                     dups;
{
   char  *pch, *line, temp[128], t2[5];
   int    i;

   if (_identify (rel) != -1 || dups < 0 || dups > 1 ||
       ! name || ! *name || ! desc || *desc < '0' || *desc > '9')
      {
      baderr (MB_BAD_INDEX);
      }

   rel->itype[rel->num_i] = dups;  /* 1 == dups, 0 == nodups */

   strcpy (rel->iname[rel->num_i], name);
   strcpy (rel->idxs[rel->num_i], "000");

   strcpy (temp, desc);
   line  = temp;

   for (i = 0; i < MAX_IDX-1; i++)
      {
      if ((pch = strchr (line, ',')) == NULL)
         break;
      *pch = 0;

      sprintf (t2, "%03d", atoi (line));
      strcat (rel->idxs[rel->num_i], t2);

      line = pch+1;
      }

   sprintf (t2, "%03d", atoi (line));
   strcat (rel->idxs[rel->num_i], t2);

   sprintf (t2, "%03d", i+1);
   strncpy (rel->idxs[rel->num_i], t2, 3);

   rel->num_i ++;

   baderr (MB_OKAY);
}

mb_err
mb_addfield (rel, name, type, arg)
relation    *rel;
char             *name;
ftype                   type;
long                          arg;
{
   strcpy (rel->name[rel->num_f], name);

   if (type == T_SERIAL)
      {
      rel->serial = arg;  /* serial is temporary storage for NextSerial */
      }

   switch (rel->type[rel->num_f] = type)
      {
      case T_CHAR:    rel->siz [rel->num_f] = (int)arg;  break;
      case T_SHORT:   rel->siz [rel->num_f] =  2;        break;
      case T_USHORT:  rel->siz [rel->num_f] =  2;        break;
      case T_LONG:    rel->siz [rel->num_f] =  4;        break;
      case T_ULONG:   rel->siz [rel->num_f] =  4;        break;
      case T_FLOAT:   rel->siz [rel->num_f] =  4;        break;
      case T_DOUBLE:  rel->siz [rel->num_f] =  8;        break;
      case T_MONEY:   rel->siz [rel->num_f] =  8;        break;
      case T_TIME:    rel->siz [rel->num_f] =  4;        break;
      case T_DATE:    rel->siz [rel->num_f] =  4;        break;
      case T_SERIAL:  rel->siz [rel->num_f] =  4;        break;
      }

   rel->num_f ++;

   baderr (MB_OKAY);
}


/*
 ******************************************************************************
 *
 */

mb_err
mb_create (rel, name, mem)
relation  *rel;
char           *name;
int                   mem;
{
   char   temp[35];
   int    i, R, n, j;
   short  tshort;
   long   tlong;

   (void)mem;    /* Reference for compiler's sake; unused in MetalBase 5.0 */

   if (! rel || _identify (rel) != -1)
      baderr (MB_BAD_REL);

   if (! rel->num_i)   baderr (MB_NO_INDICES);
   if (! rel->num_f)   baderr (MB_NO_FIELDS);


/*
 * See if we can create the file (if it already exists, delete it).
 *
 */

   if (access (name, 0) != -1)
      unlink (name);
   if ((R = creatx (name)) == -1)
      baderr (MB_NO_WRITE);
   modex (name, 0666);   /* Make the file   -rw-rw-rw-  */
   close (R);

   if ((R = openx (name, OPENMODE)) == -1)
      {
      unlink (name);  /* We made it, but can't open it, so delete it. */
      baderr (MB_NO_WRITE);
      }


/*
 * Great; we've created the file.  Now fill it out...
 *
 */

   temp[0] = 50;  writx (R, temp, 1);  /* MetalBase 5.0 Signature */
   temp[0] =  0;  writx (R, temp, 1);  /* Zero users              */

   for (tshort = 0, i = 0; i < 2; i++)
      write (R, &tshort, 2);           /* Temporary and exclusive locks */

   for (tlong = 0L, i = 0; i < 3; i++)
      writx (R, &tlong, 4);            /* Pointers to fields,indices,recs */

   writx (R, &tlong, 4);               /* Number of records */
   writx (R, &rel->serial, 4);         /* Next serial value */

   tshort = (short)rel->num_f;  writx (R, &tshort, 2);  /* Number of fields  */
   tshort = (short)rel->num_i;  writx (R, &tshort, 2);  /* Number of indices */

   for (i = 0; i < rel->num_i; i++)
      writx (R, &tlong, 4);       /* Pointers to top of each index tree */

/*
 * That was ugly.  We're now ready to write the fields' descriptions...
 *
 */

   tlong = lseek (R, 0L, 1);               /* Current position?             */
   lseek (R, POS_FIELDPTR(verCURRENT), 0); /* Pointer to fields' positions  */
   writx (R, &tlong, 4);
   lseek (R, tlong, 0);           /* Move back to where we were    */

/*
 * A: var*F.....Fields' descriptions:
 *                 byte    0 : Type (0-10, as listed above)
 *                 bytes 1-2 : Size (short/ used only for char fields)
 *                 bytes 3-? : Name (max len = 20, terminated by '|')
 *
 */

   for (i = 0; i < rel->num_f; i++)
      {
      temp[0] = (char)rel->type[i];   writx (R,  temp,   1);
      tshort  = (short)rel->siz[i];   writx (R, &tshort, 2);

      writx (R, rel->name[i], strlen(rel->name[i]));
      writx (R, "|", 1);
      }

/*
 * That was uglier.  We're now ready to write the indices' descriptions...
 *
 */

   tlong = lseek (R, 0L, 1);   /* Current position?             */
   lseek (R, POS_INDEXPTR, 0); /* Pointer to indices' positions */
   writx (R, &tlong, 4);
   lseek (R, tlong, 0);        /* Move back to where we were    */

/*
 * B: var*I.....Indices' descriptions:
 *                 byte    0 : Type (0-1, 0==nodups, 1==dups)
 *                 bytes   1 : Number of fields in this index
 *                 bytes 2-? : Name (max len = 20, terminated by ':')
 *                       --- : Each field's sequential # (as short, 0-based)
 *      1.......Separator ('\n')
 *
 */

   for (i = 0; i < rel->num_i; i++)
      {
      strzcpy (temp, rel->idxs[i], 3);
      n = atoi (temp);

      temp[0] = (char)rel->itype[i];  /* 0==nodups, 1==dups                */
      temp[1] = (char)n;              /* N==number of fields in this index */

      writx (R, temp, 2);

      writx (R, rel->iname[i], strlen (rel->iname[i]));
      writx (R, ":", 1);

      for (j = 0; j < n; j++)
         {
         strzcpy (temp, &rel->idxs[i][3 + j*3], 3);
         tshort = (short)atoi (temp);
         writx (R, &tshort, 2);
         }
      }

/*
 * Next, there's the stuff that's new to 5.0-- rel->hack points to the position
 * of all the new stuff.  Included are:
 *    6 bytes (3 * sizeof(short))  - Hacklock positions
 *   60 bytes (30 * sizeof(short)) - Thirty-position service queue
 *   30 bytes (30 * sizeof(char))  - Queue strobes
 *   32 bytes (8 * sizeof(long))   - Reserved for later use
 *
 */

   rel->hack = lseek (R, 0L, 1);  /* Remember current position */
   tshort = 0;
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);

   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);
   writx (R, &tshort, 2); writx (R, &tshort, 2); writx (R, &tshort, 2);

   for (i = 0; i < 30; i++)
      temp[i] = 0;

   writx (R, temp, 30);

   tlong = 0L;
   writx (R, &tlong, 4); writx (R, &tlong, 4);  /* RESERVED SPACE */
   writx (R, &tlong, 4); writx (R, &tlong, 4);  /* RESERVED SPACE */
   writx (R, &tlong, 4); writx (R, &tlong, 4);  /* RESERVED SPACE */
   writx (R, &tlong, 4); writx (R, &tlong, 4);  /* RESERVED SPACE */

   writx (R, SNGCR, NUMCR);

   tlong = lseek (R, 0L, 1);   /* Current position?             */
   lseek (R, POS_RECZERO, 0);  /* Pointer to record #0          */
   writx (R, &tlong, 4);

   rel->recz = tlong;

   close (R);

   baderr (MB_OKAY);
}

int
mb_getname (rel, name, fIndex)
relation   *rel;
char            *name;
int                    fIndex;
{
   int   i;

   if (fIndex)
      {
      for (i = 0; i < rel->num_i; i++)
         if (! strcmp (rel->iname[i], name))
            return i;
      }
   else
      {
      for (i = 0; i < rel->num_f; i++)
         if (! strcmp (rel->name[i], name))
            return i;
      }

   return -1;
}

