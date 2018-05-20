/*
 * METALBASE 5.0
 *
 * Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
 *                                       [ t-richj@microsoft.com ]
 */

#include "mbase.h"

#define movech(c,y,x)  move(y,x);refresh();c=getarr();
#define doing(x,o)	 (arr == x && opt == o && go == 1)

#ifdef LONGARGS
void	main		 (int, char **);
void	paint_scrn (int, int);
int	 	get_rel	 (char *);
void	get_index	 (void);
void	get_rel_name (void);
void	add_rec		(int);
void	del_rec		(void);
void	do_error	(char *);
void	sel_rec	 (int);
void	fill_page  (void);
int	 	verify	  (char *);
void	zero_rec	(void);
void	do_line	 (int, int);
#else
void	main();
void	paint_scrn();
int	 	get_rel();
void	get_index();
void	get_rel_name();
void	do_error();
void	add_rec();
void	del_rec();
void	sel_rec();
void	fill_page();
int	 	verify();
void	zero_rec();
void  	do_line();
#endif

static int num_[]	  = { 13 };
static int opt_[][20] = {{ 0, 6, 11, 16, 21, 26, 30, 34, 41, 45, 49, 53, 57 }};
static char *line_[]  =
 { "First Last Next Prev Gteq Equ Idx Screen Add Upd Del Rel Quit" };

#ifdef MSDOS
#define Standout()
#define Standend()
#else
#define Standout() standout()
#define Standend() standend()
#endif

relation *rel = (relation *)0;
char	name[20];
int		pg, pgmax, iserr=0, inw=0, pglst, idx;
dataptr	buf;
#define ROWS	(LINES - 7)
#define FIRSTCOL	18

void clean(int i)
{

	clear();
	refresh();
	resetty();
	endwin();
	mb_exit(i);
}

void
main  (argc, argv)
int	 argc;
char **argv;
{
int  c, opt = 0, arr = 0, go, esc=0;

	init_curses();

	if (argc > 2) { 
			fprintf (stderr, "vr: format: vr [relation]%s", SNGCR);
			clean(1);
	 }

	if (argc == 2)
			if (! get_rel (argv[1])) {
		  		fprintf (stderr,"vr: invalid argument (%s)%s",mb_error,SNGCR);
				clean(1);
		 	}

	inw = 1; 
	savetty();
	raw(); 
	noecho(); 
	nl();
	paint_scrn (0, 1);

	if (rel)  fill_page();

	for (;;) {
		movech(c, 0,opt_[arr][opt]);  
		go = 0;  
		if (iserr)  
			do_error("");
#ifdef VI_EMU
		if (c != 27)  
			esc = 0;
		else
			if (esc == 1)  
				break;
			else			  
				esc = 1;
#endif

		switch (c) { 
		case ' ': case AR_RIGHT: 
			opt++;	
			break;
	  	case '\b': case 127: case AR_LEFT:  
	  		opt--;	
	  		break;
	  	case '?': case '/':  
	  		paint_scrn(-1,0);		
	  		break;
	  	case CTRL_L: 
	  		clearok(win,TRUE); 
	  		refresh();	  
	  		break;
	 	}
		if (arr == 0)
			switch (c=tolower(c)) {
			case '\n': case '\r': go = 1;  break;
			case  'f':  opt = 0;  go = 1;  break;
			case  'l':  opt = 1;  go = 1;  break;
			case  'n':  opt = 2;  go = 1;  break;
			case  'p':  opt = 3;  go = 1;  break;
			case  'g':  opt = 4;  go = 1;  break;
			case  'e':  opt = 5;  go = 1;  break;
			case  'i':  opt = 6;  go = 1;  break;
			case  's':  opt = 7;  go = 1;  break;
			case  'a':  opt = 8;  go = 1;  break;
			case  'u':  opt = 9;  go = 1;  break;
			case  'd':  opt =10;  go = 1;  break;
			case  'r':  opt =11;  go = 1;  break;
			case  'q':  opt =-1;  go = 1;  break;
		 	}

			if (opt == -1) 
				opt = num_[arr]-1;
			if (opt == num_[arr])  opt = 0;

			if (arr==0 && go==1 && opt < 11 && rel==RNULL && opt >= 0)
				do_error("specify new relation first"),go=0;
			if (arr==0 && go==1 && (opt==9||opt==10) && rel->pos == 0L)
				do_error("select a record first"),go=0;

			if (doing (0, -1))  break;
			if (doing (0, 12))  break;
			if (doing (0,  0))  sel_rec (FIRST);
			if (doing (0,  1))  sel_rec (LAST);
			if (doing (0,  2))  sel_rec (NEXT);
			if (doing (0,  3))  sel_rec (PREVIOUS);
			if (doing (0,  4))  sel_rec (GTEQ);
			if (doing (0,  5))  sel_rec (EQUAL);
			if (doing (0,  6))  get_index();
			if (doing (0,  7)) {
				pg++;
				if (pg > pgmax) pg = 1;
				fill_page();
				paint_scrn(arr,0);
			}
			if (doing (0,  8))  add_rec(1);
			if (doing (0,  9))  add_rec(0);
			if (doing (0, 10))  del_rec();
			if (doing (0, 11))  get_rel_name();
	}

	if (buf) 
		free(buf);
	
	clean(0);
}

void
paint_scrn (opt, init)
int			opt, init;
{
char temp[128];

	if (init) { 
		clear();
		attron(A_BOLD);
		mvaddstr (0, COLS - 14, "MetalBase 5.0");
		attroff(A_BOLD);
		move(1,  0);
		hline(ACS_HLINE, COLS);
		addch(ACS_HLINE);
	}
	if (opt != -1) {
		mvaddstr (0,  0, line_[opt]);
		refresh();
	}
	if (opt == -1 || init) {
		if (rel == RNULL)
			strcpy  (temp, "no relation given");
		else
			sprintf (temp, "%s.rel -- %ld records, %d indices -- Page %d of %d",
						 	name, mb_num(rel), rel->num_i, pg,pgmax);
		move(2, 0);  
		clrtoeol();
		mvaddstr(2, (COLS-strlen(temp))/2, temp);
	}
	refresh();
}

void
get_index ()
{
int  i;
char ptr[128];

	move (4, 0);  
	clrtobot();
	for (i = 0; i < min (rel->num_i, 16); i++) {
		sprintf (ptr, "Index #%2d - %s", i+1, rel->iname[i]);
		mvaddstr (6+i, 10, ptr);
	}
	sprintf (ptr, "%s", rel->iname[idx]);  
	do_error ("Select index");

	mvaddstr (4, 10, "Index Name or Number : ");  
	refresh();
	if ((input (ptr, 0, 20) < 0) || ptr[0] == 0) {
	  	fill_page();  
	 	do_error("Index selection aborted"); 
	 	return; 
	 }

	if ((i = atoi (ptr)-1) < 0)  i = idxnum (rel, ptr);

	if (i < 0 || i > 16 || i >= rel->num_i) {
	  	fill_page ();
		do_error  ("Invalid index -- selection aborted");
		return;
	}

	idx = i;  
	sprintf (ptr, "New index: #%d (%s)", idx+1, rel->iname[idx]);
	fill_page();  
	do_error(ptr);
}

void
get_rel_name ()
{
char	nm[128], *c;
mb_err 	d;

	move(4, 0);  
	clrtobot();
	addstr("Relation Name		: ");  
	refresh();  
	nm[0] = 0;
	input(nm, 0, 60);
	if ((! *nm) || (*nm && !get_rel(nm))) {
	  	c = mb_error;
	 	d = mb_errno;
		fill_page();  
		if (d)  do_error(c);
		paint_scrn(-1, 0);
	 }
}

int
get_rel (file)
char	 *file;
{
char	pw[80], t[80];
int  	i;

	i = strlen(file);
	if (! strcmp (&file[i-4], ".rel"))  file[i-4] = 0;
	for (i = strlen(file); i >= 0; i--)
		if (file[i] == '/' || file[i] == ':')  break;
	strcpy (t, &file[i+1]);

	if (mb_tst (file))  return 0;

	if (rel)  mb_rmv(rel),rel=RNULL;
	if (buf)  free(buf),  buf=NULL;
	pg = pgmax = pglst = 0; strcpy (name, t);

#ifdef NOENCRYPT
	pw[0] = 0;
#else
	if (! inw) {
	  	fputs("Encryption password: ", stdout); 
	  	fflush(stdout); 
	  	fgets(stdin, pw, 80);
	 } else {
	 	move(5, 0);
		addstr("Encryption password: ");  
		refresh();
		pw[0] = 0;
		input(pw, 0, 60);
		move(4, 0);  
		clrtobot();  
		refresh();
	 }
#endif
	if ((rel = mb_inc (file, strtokey (pw))) == RNULL)  return 0;
	buf = (dataptr)malloc (rel->rec_len + 1);
	pgmax = (rel->num_f / ROWS) + 1;  
	idx = 0;
	pglst = rel->num_f % ROWS;  
	pg = 1;  
	if (inw)  
		fill_page();
	zero_rec();
	return 1;
}

void
do_error (line)
char	  *line;
{
	move (LINES-1, 0);	
	clrtoeol();  
	iserr = 0;
	if (! *line) { 
		refresh(); 
		return; 
	}
	Standout();	  
	addstr(line);
	Standend();	  
	refresh();	
	iserr = 1;
}

void
sel_rec (act)
int		act;
{
	char *a, c;
	int	i, m, arr[40], n;

	if (act == GTEQ || act == EQUAL) {
		move (4,0); 
		clrtobot();
		do_error("Enter comparison values (vi-style)");
		a = rel->idxs[idx]; 
		m = (a[0]-'0')*100 +(a[1]-'0')*10 +(a[2]-'0');
		m = min(m,39);
		for (i = 0; i < m; i++) {
			arr[i]= (a[i*3+3]-'0')*100 +(a[i*3+4]-'0')*10 +(a[i*3+5]-'0');
			do_line (4+i, arr[i]);
		}
		refresh();
		for (i = 0; ; ) {
			move (4+i, FIRSTCOL);
			switch (rel->type[arr[i]]) {
			case T_CHAR:
				n=rel->siz[arr[i]]; 
				break;
			case T_SHORT: case T_USHORT: 
				n=6;		
				break;
			case T_LONG: case T_ULONG:  
				n=11;		
				break;
			case T_FLOAT: case T_SERIAL: 
				n=11;
				break;
			case T_DOUBLE: case T_MONEY:
			  	n=14;
			  	break;
			case T_TIME:	
				n=8;
				break;
			case T_DATE:	
				n=10;
				break;
			case T_PHONE:	
				n=20;
				break;
			}
			if ((c=input ((char*)buf+rel->start[arr[i]], rel->type[arr[i]], n))<0)
				break;
			if (c == 1)
				break;
			if (c == 0 && i == m-1)  break;
			if (c == 0)  c='+';
			switch (c) {
			case '-': case 'k': case AR_UP:	
				i--; 
				break;
			case '+': case 'j': case AR_DOWN: 
				i++; 
				break;
			}
			if (i == -1)  i = m-1;
			if (i ==  m)  i = 0;
		}
		if (c < 0) {
			fill_page();  do_error("Search aborted");
			return;
		}
	}

	if (mb_sel (rel, idx, buf, act, buf) != MB_OKAY) {
		a = mb_error;  
		fill_page();
		do_error (a);
		return;
	 }
	fill_page();
}

void
fill_page ()
{
int  i, n;

	move(4, 0);  
	clrtobot();

	if (rel)
		for (i = 0, n = (pg-1)*ROWS; i < (pg == pgmax ? pglst : ROWS); i++, n++)
			do_line(4+i, n);

	paint_scrn(-1, 0);
}

#define getdata(b,f,t) sprintf (b, f, *(t *)((char *)buf +rel->start[n]));

void
do_line (y, n)
int		y, n;
{
char temp[80];
int	 t;
long ac,pre,num,ext;

	if (n >= rel->num_f) 
		return;
	move (y, 0);  
	clrtoeol();

	strzcpy(temp, rel->name[n], 13);
	mvaddstr(y,  0, temp);
	mvaddstr(y, 15, "-");

	switch (rel->type[n]) {
	case T_CHAR:	 
		mvaddch (y, 17, '\"');
		mvaddch (y, FIRSTCOL +min(rel->siz[n],60), '\"');
		break;
	case T_SHORT: case T_USHORT:  
		mvaddstr (y, 17, "[		]");					
		break;
	case T_LONG: case T_ULONG: case T_FLOAT:	
		mvaddstr (y, 17, "[			  ]");			 
		break;
	case T_DOUBLE:  
		mvaddstr (y, 17, "[				  ]");		 
		break;
	case T_MONEY:	
		mvaddstr (y, 17, "$				  ");		  
		break;
	case T_TIME:	 
		mvaddstr (y, 17, "(  :  :  )");
		break;
	case T_DATE:	 
		mvaddstr (y, 17, "(  /  /	 )");
		break;
	case T_SERIAL:  
		mvaddstr (y, 17, "(			  )");	
		break;
	case T_PHONE:	
		mvaddstr (y, 17, "(	-	-				)"); 
		break;
	}

	if (rel->pos != 0L) {
		switch (rel->type[n]) {
			case T_CHAR:  	t = min (rel->siz[n], 60);
						  	strzcpy(temp, (char *)buf+rel->start[n], t);
						 	break;
			case T_SHORT:	getdata(temp, "%d",	 short); 	break;
			case T_USHORT:  getdata(temp, "%u",	 ushort);  	break;
			case T_LONG:	getdata(temp, "%ld",	 long);	 	break;
			case T_ULONG:	getdata(temp, "%lu",	 ulong); 	break;
			case T_FLOAT:	getdata(temp, "%f",	 float); 	break;
			case T_DOUBLE:  getdata(temp, "%lf",	 double);  	break;
			case T_MONEY:	getdata(temp, "%-.2lf", double);  	break;

			case T_SERIAL:  getdata (temp, "%ld", long);		break;
			case T_PHONE:	
				scn_phone(&ac,&pre,&num,&ext, ((char *)buf + rel->start[n]));
				strcpy(temp, fmt_phone (ac, pre, num, ext, 0)); 
				break;
			case T_TIME:	
				strcpy(temp, fmt_time (*(mb_time *) ((char *)buf +rel->start[n]), 0)); 
				break;
			case T_DATE:	 
				strcpy(temp, fmt_date (*(mb_date *) ((char *)buf +rel->start[n]), 0)); 
				break;
		}
		fprintf(stderr, "moving %s to %d and %d\n", temp, y, FIRSTCOL);
		mvaddstr (y, FIRSTCOL, temp);
	}
}

void
add_rec (opt)
int		opt;
{
int  	i;
int		_pg, n, m, o;  /* Handles up to 500 fields */
mb_err	e;
char	c,  *p;
long	tlong;

	_pg = pg;

	if (opt) {
		 tlong = rel->pos;
		 rel->pos = 1L; 
		 zero_rec();  /* Add new rec */
	}

	for (pg = 1, i = 0; ; ) {
		fill_page ();
		if (opt)  
			do_error ("enter data to add");
		else		
			do_error ("enter new data for this record");

		for (o = ROWS*pg-ROWS, m = o + (pg == pgmax ? pglst : ROWS); ; ) {
		  	move(4+i - ((pg-1)*ROWS), FIRSTCOL);
			switch (rel->type[i]) {
				case T_CHAR: 					n=rel->siz[i];  break;
				case T_SHORT: case T_USHORT:  	n=6;			break;
				case T_LONG:  case T_ULONG:		n=11;			break;
				case T_FLOAT: case T_SERIAL:  	n=11;			break;
				case T_DOUBLE: case T_MONEY:	n=14;			break;
				case T_TIME:				  	n=8;			break;
				case T_DATE:					n=10;			break;
				case T_PHONE:					n=20;			break;
			}
			n = min (n, 60);
			if (rel->type[i] == T_SERIAL) {
				if (rel->num_f == 1)  
					c = 1;	 /* Nothing but serial#?  Done. */
				else
					c = '+';  /* Don't let 'em enter serial# */
			} else {
				if ((c = input ((char *)buf + rel->start[i], rel->type[i], n)) < 0)
					break;
			}
			if (c == 0 && i == m-1 && pg==pgmax)  c=1;
			if (c == 0)  c='+';
			if (c == 1)  break;
			switch (c) {
			 	case '-': case 'k': case AR_UP:	i--;  c='-';  break;
				case '+': case 'j': case AR_DOWN: i++;  c='+';  break;
			}
			if (c == '-' && rel->type[i] == T_SERIAL)  i--;
			if (i >=  m) { 
				pg++;
				i = m;  
				c = 0;
				if (pg > pgmax) pg = 1;
				i = 0;
				break;
			}
			if (i <	o) { pg--;i=o-1;c=0;if(pg==0) pg=pgmax,i=rel->num_f-1;break;}
		 }
		if (c != 0)  break;
	 }
	if (opt)  pg=_pg, rel->pos=tlong;

	if (c < 0)
	 { mb_sel (rel, idx, buf, CURRENT, NULL);  if (! rel->pos)  zero_rec();
		fill_page ();
		do_error ("add aborted");
		return;
	 }

	fill_page();
	do_error ("wait...");

	if (opt)  e=mb_add (rel, buf), p=mb_error;
	else		e=mb_upd (rel, buf), p=mb_error;

	mb_sel (rel, idx, buf, CURRENT, NULL);  if (! rel->pos)  zero_rec();
	fill_page ();

	if (e != MB_OKAY)
		do_error (p);
	else
	 { if (opt)  do_error ("record successfully added");
		else		do_error ("record successfully updated");
		paint_scrn (-1, 0);
	 }
}

void
del_rec ()
{
	if (! verify ("delete this record ? "))
	 { do_error ("delete aborted");
		return;
	 }
	do_error ("wait...");
	if (mb_del (rel) != MB_OKAY)
	 { do_error (mb_error);
		return;
	 }
	if (! rel->pos)  zero_rec();
	fill_page();
	do_error ("record deleted");
}

int
verify (str)
char	*str;
{
	char  c;
	do_error (str);
	for (;;)
	 { c = getarr();
		switch (tolower(c))
		 { case 'q': case 'n':  case ' ':	do_error (""); return 0; break;
			case  27: case '\r': case '\n':  do_error (""); return 0; break;
			case 'y':								do_error (""); return 1; break;
		 }
	 }
}

void
zero_rec ()
{
int  i;
char *a;

	for (i = 0; i < rel->num_f; i++) {
		a=(char *)buf +rel->start[i];
		switch (rel->type[i]) {
			case T_CHAR:	*a = 0;	 break;
			case T_PHONE:	*a = 0;	 break;
			case T_SHORT:	*(short  *)a = (short)0;  break;
			case T_USHORT:  *(ushort *)a = (ushort)0; break;
			case T_LONG:	*(long	*)a = (long)0;	break;
			case T_ULONG:	*(ulong  *)a = (ulong)0;  break;
			case T_FLOAT:	*(float  *)a = (float)0.0;	break;
			case T_DOUBLE:
			case T_MONEY:	*(double *)a = (double)0.0;  break;
			case T_TIME:	*(mb_time*)a = (mb_time)0;	break;
			case T_DATE:	*(mb_date*)a = (mb_date)0;	break;
			case T_SERIAL:  *(long	*)a = (long)0;	break;
		}
	}
}

