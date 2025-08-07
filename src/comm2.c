/*
 * COMM2.C
 *
 * (c)1986 Matthew Dillon     9 October 1986
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 * Version 5.60M by amigazen project 2025-08-07
 *
 */

#include "shell.h"

/* comm2.c */
static long dptrtosecs(DPTR *d);
static long timeof(char *s);
static int evalif(void);
static int clinum(char *name);
static int copydir(long srcdir, long destdir, int recur, char *path);	/*GMD27Jun95 */
static int copyfile(char *srcname, long srcdir, char *destname, long destdir);
static void changedisk(char *name);
static int func_array( char *fav[], int fac);
static int func_int(int i);
static int func_err (void);	/*GMD 13FEB94*/
static int func_bool(int i);
static int func_string(char *str);
static int commas(char *av[], int ac, int n);
static int wordset(char **av, int ac, char **(*func)(char **,int,char**,int,int*,int));
static int split_arg(char **av, int ac);
static char *info_part(char **av, int ac, int n, char *buf);
static char *get_filedate (char *file);	/* GMD */
static char *get_filenote (char *file);	/* GMD */

/* Casting conveniences */
#define BPTR_TO_C(strtag, var)  ((struct strtag *)(BADDR( (ULONG) var)))
#define PROC(task)              ((struct Process *)task)
#define CLI(proc)               (BPTR_TO_C(CommandLineInterface, proc->pr_CLI))

/* Externs */
extern int has_wild;                    /* flag set if any arg has a ? or * */

int
do_abortline( void )
{
	Exec_abortline = 1;
	return 0;
}

int
do_error( void )
{
	return atoi(av[1]);
}

int
do_return( void )
{
	int retcode=(ac<2 ? 0 : atoi(av[1]));

	Exec_abortline = 1;
	if (Src_stack) {
		Src_abort[Src_stack-1]=1;
		return retcode;
	} else
		main_exit(retcode);
	return 0;
}

/*
 * STRHEAD
 *
 * place a string into a variable removing everything after and including
 * the 'break' character
 *
 * strhead varname breakchar string
 *
 */

int
do_strhead( void )
{
	char *s;
	if (*av[2] && (s=index( av[3], *av[2]))) 
		*s='\0';
	set_var (LEVEL_SET, av[1], av[3]);
	return 0;
}

int
do_strtail( void )
{
	char *s;
	if (*av[2] && (s=index(av[3],*av[2]))) s++; else s=av[3];
	set_var (LEVEL_SET, av[1], s);
	return 0;
}

static long
dptrtosecs(DPTR *d)
{
	struct DateStamp *ds=(&d->fib->fib_Date);
	return ds->ds_Days*86400 + ds->ds_Minute*60 + ds->ds_Tick/TICKS_PER_SECOND;
}

static long
timeof(char *s)
{
	DPTR *d;
	int  dummy;
	long n;

	if ( (d=dopen(s,&dummy))==NULL ) return 0L;
	n=dptrtosecs(d);
	dclose(d);
	return n;
}

/*
 * if -f file (exists) or:
 *
 * if A < B   <, >, =, <=, >=, <>, where A and B are either:
 * nothing
 * a string
 * a value (begins w/ number)
 */

#define IF_NOT 128

int
do_if( char *garbage, int com )
{
	int result;

	switch (com) {
	case 0:
		if (If_stack && If_base[If_stack - 1])
			If_base[If_stack++] = 1;
		else {
			result=evalif();
			If_base[If_stack++]=(options & IF_NOT ? result : !result);
		}
		break;
	case 1:
		if (If_stack > 1 && If_base[If_stack - 2]) break;
		if (If_stack) If_base[If_stack - 1] ^= 1;
		break;
	case 2:
		if (If_stack) --If_stack;
		break;
	}
	disable = (If_stack) ? If_base[If_stack - 1] : 0;
	if (If_stack >= MAXIF) {
		fprintf(stderr,"If's too deep\n");
		disable = If_stack = 0;
		return -1;
	}
	return 0;
}

static int
isint( char *s )
{
	while( ISSPACE(*s) ) ++s;
	return *s=='+' || *s=='-' || *s>='0' && *s<='9';
}

static int
evalif( void )
{
	char c, *str, *left, *right, *cmp;
	long num, t0, i=1;
	int nac=ac-1, err;

	if(!strcmp(av[ac-1],"then")) ac--;

	switch(options & ~IF_NOT) {
	case 0:
		for( i=1; i<ac; i++ )
			if( strlen(str=av[i])<=2 && *str && index("!<=>",*str) &&
			       (!str[1] || index("<=>",str[1])))
				break;

		if ( i==ac )
			return ac>1 && *av[1] && strcmp(av[1],"0");

		left =av[1];
		right=av[i+1];
		if( 1+1!=i )    left = compile_av(av,1,i   ,0xA0, 0);
		if( i+1+1!=ac ) right= compile_av(av,i+1,ac,0xA0, 0);
		cmp  = av[i];
		num  = atol(left) - atol(right);
		if( *cmp=='!' )  cmp++, options^=IF_NOT;
		if( !isint(left) || !isint(right)) num=strcmp(left,right);
		if( 1+1!=i )    free(left);
		if( i+1+1!=ac ) free(right);

		if (num < 0)       c='<';
		else if (num > 0)  c='>';
		else               c='=';
		return index(cmp, c) != NULL;
	case 1:
		return do_rpn(NULL,i);
	case 2:
	case 256:
		return exists(av[i]);
	case 4:
		t0=timeof(av[i++]);
		for ( ; i<ac ; i++)
			if (t0<=timeof(av[i])) return 1;
		return 0;
	case 8:
		return AvailMem( MEMF_FAST )!=0;
	case 16:
		return isdir(av[i])!=0;
	case 32:
		return get_var(LEVEL_SET,av[i]) != 0;
	case 64:
		if( ac>1 )
			return get_opt(av+1,&nac,-*av[1],&err);
		return 0;
	default:
		ierror(NULL,500);
	}
	return 0;
}

#if 0
do_while( void )
{
	char *com=av[--ac];
	int ret=0;

	while( evalif() && ret==0 && !CHECKBREAK() )
		ret=execute( com );
	return ret;
}
#endif


int
do_label( void )
{
	char aseek[32], *fwd;

	if (Src_stack == 0) {
		ierror (NULL, 502);
		return -1;
	}

	sprintf (aseek, "%ld %d", Src_pos[Src_stack-1], If_stack);
	set_var (LEVEL_LABEL + Src_stack - 1, av[1], aseek);

	fwd=get_var(LEVEL_SET,v_gotofwd);
	if (fwd && !strcmp(av[1],fwd))
		forward_goto = 0;
	return 0;
}

int
do_goto( void )
{
	int new;
	long pos;
	char *lab;

	if (Src_stack == 0) {
		ierror (NULL, 502);
	} else {
		lab = get_var (LEVEL_LABEL + Src_stack - 1, av[1]);
		if (lab == NULL) {
			forward_goto = 1;
			set_var (LEVEL_SET, v_gotofwd, av[1]);
			return(0);
		} else {
			pos = atoi(lab);
			fseek (Src_base[Src_stack - 1], pos, 0);
			Src_pos[Src_stack - 1] = pos;
			new = atoi(next_word(lab));
			for (; If_stack < new; ++If_stack)
				If_base[If_stack] = 0;
			If_stack = new;
		}
	}
	Exec_abortline = 1;
	return (0);      /* Don't execute rest of this line */
}


int
do_inc(char *garbage, int com)
{
	char *var, num[32];

	if (ac>2) com *= atoi(av[2]);
	if (var = get_var (LEVEL_SET, av[1])) {
		sprintf (num, "%d", atoi(var)+com);
		set_var (LEVEL_SET, av[1], num);
	}
	return 0;
}

BPTR Input(void);
BPTR Output(void);

int
do_input( void )
{
	char *str, in[256], *get, *put;
	int i, quote=0;

	if( options&2 ) {
		if( !IsInteractive(Input()) ) return 20;
		setrawcon( -1, 0 );
		in[1]=0;
		for ( i=1; i<ac && !CHECKBREAK(); ++i) {
			Read( Input(), in, 1 );
			set_var (LEVEL_SET, av[i], in);
		}
		setrawcon( 0, 0 );
		return 0;
	}

	for ( i=1; i<ac; ++i)
		if (fgets(in,256,stdin)) {
			str=in, put=in+strlen(in)-1;
			if( *put=='\n' ) *put=0;
			if( !(options&1) ) {
				while( *str==' ' ) str++;
				for( put=get=str; *get; ) {
					if( *get=='\"' )
						quote=1-quote, get++;
					else if( *get==' ' && !quote ) {
						while( *get==' ' ) get++;
						if( *get ) *put++=0xA0;
					} else 
						*put++ = *get++;
				}
				*put=0;
			}
			set_var (LEVEL_SET, av[i], str);
		}
	return 0;
}

int
do_version( void )
{
	extern char shellname[];
	extern char shellcompiled[];
	int i;
	char str[256];

	if (ac > 1) {
		for ( i=1; i<ac; ++i) {
			sprintf(str,"C:Version%s%s%s%s \"%s\"",
				(options&1) ? " FILE" : "",		/* -f */
				(options&2) ? " INTERNAL" : "",		/* -i */
				(options&4) ? " FULL" : "",		/* -l */
				(options&8) ? " RES" : "",		/* -r */
				av[i]);
			execute(str);
		}
	}
	else {
		puts(shellname);
		puts("1.00 Lattice (c) 1986 Matthew Dillon\n"
			 "2.05 Manx(M) versions by Steve Drew\n"
			 "3.00 ARP (A) versions by Carlo Borreo, Cesare Dieni\n"
			 "4.00 ARP 1.3 versions by Carlo Borreo, Cesare Dieni\n"
			 "5.00 Lattice versions by U. Dominik Mueller\n"
			 "5.20+ OS 2.0 versions by Andreas M. Kirchwitz\n");
		printf( shellcompiled );
		printf("Currently running: ");
		fflush(stdout);
		sprintf(str,"C:Version%s%s%s%s",
			(options&1) ? " FILE" : "",		/* -f */
			(options&2) ? " INTERNAL" : "",		/* -i */
			(options&4) ? " FULL" : "",		/* -l */
			(options&8) ? " RES" : ""		/* -r */
			);
		execute(str);
	}

	return 0;
}


static int
clinum( char *name )
{
	long ncli=MaxCli(), count;
	struct Process *proc;
	char cmd[40+1];

	if( *name>='0' && *name<='9' )
		return atoi( name );

	Forbid();
	for (count = 1; count <= ncli ; count++)
		if (proc = FindCliProc(count)) {
			if( !proc->pr_TaskNum || proc->pr_CLI == 0)
				continue;
			BtoCStr(cmd,CLI(proc)->cli_CommandName, 40L);
			if( !stricmp( FilePart( cmd ), FilePart( name )))
				goto done;
		}
	count = -1;
done:
	Permit();
	return count;
}


char task_state(struct Task *task)
{
	char state = '?';
	switch (task->tc_State) {
		case TS_INVALID: state='i'; break;
		case TS_ADDED  : state='a'; break;
		case TS_RUN    : state='r'; break;
		case TS_READY  : state='r'; break;
		case TS_WAIT   : state='w'; break;
		case TS_EXCEPT : state='e'; break;
		case TS_REMOVED: state='d'; break;
		default: break;
	}
	return(state);
}

int
do_ps( void )
{
	/* this code fragment based on ps.c command by Dewi Williams */
	long count;            /* loop variable         */
	struct Task *task;     /* EXEC descriptor       */
	struct Process *proc;  /* EXEC descriptor       */
	char strbuf[64+1];     /* scratch for btocstr() */
	char cmd[40+1], *com;  /* holds cmd name        */
	long ncli,mycli,cli,i;
	long ssize;            /* stack size */
	char fmt[256];

	char **dev_list=NULL;
	long dev_num=0;

	char onoff[80];
	memset( onoff, 0, 80 );
	for( i=1; i<ac; i++ )
		onoff[ 1+clinum( av[i] ) ]=1;
	if( options&2 )
		for( i=0; i<80; i++ )
			onoff[i]=1-onoff[i];

	if (options&4)
		printf("Proc Command Name         CLI Type    Pri.  Address  Directory\n");
	else
		printf("Proc Command Name         Typ  Stack  Pri.  Address  Directory\n");

	Forbid();

	ncli=(LONG)MaxCli();
	mycli= Myprocess->pr_TaskNum;

	for (count = 1; count <= ncli ; count++) {
		if (proc = FindCliProc(count)) {	/* Sanity check */
			task = (struct Task *)proc;
			cli=proc->pr_TaskNum;
			if( ac>1 && !onoff[1+cli] )
				continue;
			if ( cli==0 || proc->pr_CLI == 0) continue; /* or complain? */
				BtoCStr(cmd,   CLI(proc)->cli_CommandName, 40L);
				BtoCStr(strbuf,CLI(proc)->cli_SetName    , 64L);
			com=cmd;
			if( !(options&1) )
				com=FilePart(com);
			if (options&4) {
				sprintf(fmt,"%c%2d  %-20.20s %-10.10s %4d  %8lx  %s",
					cli==mycli ? '*' : ' ',
					count,
					com,
					task->tc_Node.ln_Name,
					(signed char)task->tc_Node.ln_Pri,
					proc,
					strbuf
				);
			}
			else {
				if (task->tc_SPReg>=task->tc_SPLower && task->tc_SPReg<task->tc_SPUpper)
					ssize = (long)task->tc_SPUpper-(long)task->tc_SPLower;
				else
					ssize = (CLI(proc)->cli_DefaultStack)*4;
				sprintf(fmt,"%c%2d  %-20.20s %c%c %7ld %4d  %8lx  %s",
					cli==mycli ? '*' : ' ',
					count,
					com,
					CLI(proc)->cli_Background?'b':'f',
					task_state(task),
					ssize,
					(signed char)task->tc_Node.ln_Pri,
					proc,
					strbuf
				);
			}
			add_array_list(&dev_list,&dev_num,fmt);
		}
	}

	Permit();

	for(i=0; !dobreak() && i<dev_num; i++)
		printf("%s\n",dev_list[i]);

	free_array_list(dev_list,dev_num);

	return 0;
}

#if 0
int
do_ps( void )
{
	/* this code fragment based on ps.c command by Dewi Williams */
	int count;             /* loop variable         */
	struct Task *task;     /* EXEC descriptor       */
	char strbuf[64+1];     /* scratch for btocstr() */
	char cmd[40+1], *com;  /* holds cmd name        */
	long ncli,mycli,cli,i;

	char onoff[80];
	memset( onoff, 0, 80 );
	for( i=1; i<ac; i++ )
		onoff[ 1+clinum( av[i] ) ]=1;
	if( options&2 )
		for( i=0; i<80; i++ )
			onoff[i]=1-onoff[i];

	printf("Proc Command Name         CLI Type    Pri.  Address  Directory\n");
	Forbid();

	ncli=(LONG)MaxCli();
	mycli= Myprocess->pr_TaskNum;
	for (count = 1; count <= ncli ; count++)             /* or just assume 20?*/
		if (task = (struct Task *)FindCliProc((LONG)count)) {/* Sanity check      */
			cli=PROC(task)->pr_TaskNum;
			if( ac>1 && !onoff[1+cli] )
				continue;
			if ( cli==0 || PROC(task)->pr_CLI == 0) continue; /* or complain? */
				BtoCStr(cmd,   CLI(PROC(task))->cli_CommandName, 40L);
				BtoCStr(strbuf,CLI(PROC(task))->cli_SetName    , 64L);
			com=cmd;
			if( !(options&1) )
				com=FilePart(com);
				printf("%c%2d  %-20.20s %-11.11s %3d  %8lx  %s\n",
				cli==mycli ? '*' : ' ',
				count,
				com,
				task->tc_Node.ln_Name,
				(signed char)task->tc_Node.ln_Pri,
				task,
				strbuf
			);
		}

	Permit();
	return 0;
}
#endif

/*
 * CP [-d] [-u] file file
 * CP [-d] [-u] file file file... destdir
 * CP [-r][-u][-d] dir dir dir... destdir
 */

#define CP_RECUR         0x1
#define CP_UPDATE        0x2
#define CP_NODATE        0x4
#define CP_NOFLAGS       0x8
#define CP_FRESH         0x10
#define CP_MOVE          0x20
#define CP_QUIET         0x40
#define CP_OVERWRITE     0x80
#define CP_LEAVE_ARCHIVE 0x100
#define CP_OUTPUTFORMAT2 0x200

static char *errstr = NULL;          /* let's be a little more informative */
static int level;
static char **dir_lst;		/*GMD27Jun95 */
static int dir_lst_sz;		/*GMD27Jun95 */

void print_path(int j);		/*GMD27Jun95 */
void push_dir_path(char *zz);	/*GMD27Jun95 */
void pop_dir_path(int offs);	/*GMD27Jun95 */

int do_copy( void )
{
	int recur, ierr;
	char *destname;
	char destisdir;
	FIB *fib;
	int i;

	if (OPT(CP_OUTPUTFORMAT2)) {
		level = 0;		/*GMD27Jun95, be safe */
		dir_lst = NULL;		/*GMD27Jun95, be safe */
		dir_lst_sz = 0;		/*GMD27Jun95, be safe */
	}

	errstr = NULL;
	ierr   = 0;

	fib = (FIB *)SAllocMem((long)sizeof(FIB), MEMF_PUBLIC);

	recur    = OPT( CP_RECUR );
	destname = av[ac - 1];

	if (ac < 3) {
		ierr = 500;
		goto done;
	}
	destisdir = isdir(destname);
	if (ac > 3 && !destisdir) {
		ierr = 507;
		goto done;
	}

/*
 * copy set:                        reduce to:
 *    file to file                     file to file
 *    dir  to file (NOT ALLOWED)
 *    file to dir                      dir to dir
 *    dir  to dir                      dir to dir
 *
 */

	for ( i=1; i<ac-1 && !dobreak(); ++i) {
		short srcisdir = isdir(av[i]);

		/*
		 *  hack to stop dir's from
		 *  getting copied if specified
		 *  from wild expansion         
		 */

		if (srcisdir && has_wild && (ac >2))
			continue;

		if (srcisdir) {
			BPTR srcdir, destdir;
			if (!destisdir) {
				if (exists(destname)) {
					ierr = 507;	/* disallow dir to file */
					goto done;
					}
				if (destdir = CreateDir(destname)) UnLock(destdir);
				destisdir = 1;
			}
			if (!(destdir = Lock(destname, ACCESS_READ))) {
				ierr = 205;
				errstr = strdup(destname);
				goto done;
			}
			if (!(srcdir = Lock(av[i], ACCESS_READ))) {
				ierr = 205;
				errstr = strdup(av[i]);
				UnLock(destdir);
				goto done;
			}
			ierr = copydir(srcdir, destdir, recur, av[i]);	/*GMD27Jun95 */
			UnLock(srcdir);
			UnLock(destdir);
			if (ierr) break;
		} else {                   /* FILE to DIR,   FILE to FILE   */
			BPTR destdir, srcdir, tmp;
			char *destfilename;

			srcdir = (BPTR)(Myprocess->pr_CurrentDir);

			if ((tmp = Lock(av[i], ACCESS_READ)) == NULL || !Examine(tmp,fib)) {
				if (tmp) UnLock(tmp);
				ierr = 205;
				errstr = strdup(av[i]);
				goto done;
			}
			UnLock(tmp);
			if (destisdir) {
				destdir = Lock(destname, ACCESS_READ);
				destfilename = fib->fib_FileName;
			} else {
				destdir = srcdir;
				destfilename = destname;
			}
			ierr = copyfile(av[i], srcdir, destfilename, destdir);
			if (destisdir) UnLock(destdir);
			if (ierr) break;
		}
	}

done:

	FreeMem(fib, sizeof(FIB));

	if (ierr) {
		ierror( errstr?errstr:"", ierr);
		if (errstr) {
			free(errstr);
			errstr = NULL;
		}
		return 20;
	}

	/* just to be sure if ierr was 0 */
	if (errstr) {
		free(errstr);
		errstr = NULL;
	}

	if (OPT(CP_OUTPUTFORMAT2)) {
		if (dir_lst) {		/*GMD27Jun95 */
			while (level)
				pop_dir_path(level--);	/* free dirname str */
			free(dir_lst);
		}
	}

	return 0;
}


static int copydir(BPTR srcdir, BPTR destdir, int recur, char *path)
							/*GMD27Jun95 */
{
	BPTR cwd;
	BPTR destlock, srclock;
	char *srcname;
	FIB *srcfib;
	int ierr=0;

	srcfib = (FIB *)SAllocMem((long)sizeof(FIB), MEMF_PUBLIC);
	if( !Examine(srcdir, srcfib)) {
		ierr=IoErr();
		goto done;
	}
	srcname=srcfib->fib_FileName;

	if (OPT(CP_OUTPUTFORMAT2)) {
		push_dir_path(path);	/*GMD27Jun95 */
	}

	while (ExNext(srcdir, srcfib)) {
		if (CHECKBREAK())
			break;
		if (srcfib->fib_DirEntryType < 0) {
			ierr = copyfile(srcname,srcdir,srcname,destdir);
			if (ierr) break;
		} else {
			if ( srcfib->fib_DirEntryType!=ST_USERDIR ) {
				if (OPT(CP_OUTPUTFORMAT2)) {
					print_path(3);	/*GMD27Jun95 */
					printf(" ....[Skipped. Is a link]\n");
				}
				else {
					printf("%*s%s (Dir)....[Skipped. Is a link]\n",level * 6," ",srcname);
				}
				continue;
			}
			if (recur) {
				cwd = CurrentDir(srcdir);
				if (srclock = Lock(srcname, ACCESS_READ)) {
					CurrentDir(destdir);
					if (!(destlock = Lock(srcname,ACCESS_READ))) {
						destlock = CreateDir(srcname);
						if (!OPT(CP_OUTPUTFORMAT2)) {
							printf("%*s%s (Dir)....[Created]\n",level * 6," ",srcname);
						}
#if 0
						printf("....[Created]\n");	/*GMD27Jun95 */
#endif

						/* UnLock and re Lock if newly created
						 * for file_date() to work properly */
						if (destlock)
							UnLock(destlock);
						destlock = Lock(srcname, ACCESS_READ);
					} else {
						if (!OPT(CP_OUTPUTFORMAT2)) {
							printf("%*s%s (Dir)\n",level * 6," ",srcname);
						}
					}
					if (destlock) {
						level++;
						ierr = copydir(srclock, destlock, recur, srcname);	/*GMD27Jun95 */

						if (OPT(CP_OUTPUTFORMAT2)) {
							pop_dir_path(level);	/*GMD27Jun95 */
						}

						level--;
						UnLock(destlock);
					} else
						ierr = IoErr();
					UnLock(srclock);
					if (!OPT(CP_NOFLAGS)) {
						setProtection(srcname,srcfib->fib_Protection );
						if( *srcfib->fib_Comment )
							SetComment( srcname, srcfib->fib_Comment );
					}
				} else {
					ierr = IoErr();
				}
				CurrentDir(cwd);
				if (ierr)
					break;
			}
		}
	}
done:
	FreeMem(srcfib, sizeof(FIB));
	return ierr;
}



#define COPYBUF 32768

static int
copyfile(char *srcname, BPTR srcdir, char *destname, BPTR destdir)
{
	BPTR cwd;
	BPTR f_src  = NULL;
	BPTR f_dest = NULL;
	long dest_mode;
	long j;
	int  stat,ierr=0,reset=0;
	char *buf;
	DPTR *dpd = NULL;
	DPTR *dps;
	long src_pbits;

	if (errstr) {
		free(errstr);
		errstr = NULL;
	}

	if ( !(buf= (char *)AllocMem(COPYBUF, MEMF_PUBLIC|MEMF_CLEAR)))
		return ERROR_NO_FREE_STORE;

	cwd = CurrentDir(srcdir);

	if ( !(dps = dopen(srcname,&stat))) {
		errstr = strdup(srcname);	/* set up file name [GMD] */
		ierr = IoErr();
		goto done;
	}
	UnLock(dps->lock);
	dps->lock = NULL;
	src_pbits = dps->fib->fib_Protection ;
	/*
	 * ensure src file can be read - if not, force it so (maybe)
	 */
	if ( src_pbits & FIBF_READ) {            /* not readable */
		if (!OPT(CP_OVERWRITE)) {
			errstr	= strdup(srcname);
			ierr	= ERROR_READ_PROTECTED ;
			goto done;
		}
		if (!setProtection(srcname, 0)) {    /* ----rwed */
			pError(srcname);
			goto done;
		}
		reset=1;
	}

	if ( (f_src=Open(srcname, MODE_OLDFILE)) == NULL) { 
		errstr	= strdup(srcname);
		ierr	= IoErr();
		goto done;
	}

	CurrentDir(destdir);

	/*
	 *	if dest does not exist, open it with 'rwed' protections
	 *	if dest does exist, get its protections.
	 *	if -o option , set its protections to enable a write
	 *		(and reset them to original status when copied)
	 *	if not -o , then error
	 */

	dest_mode	= MODE_NEWFILE;

	if( (dpd=dopen(destname, &stat)) != NULL) {
		UnLock(dpd->lock);
		dpd->lock  = NULL;

		if ( OPT(CP_OVERWRITE)) {
			if (!setProtection(destname, 0)) {    /* ----rwed */
				pError(destname);
				goto done;
			}
		} else if( dpd->fib->fib_Protection & FIBF_WRITE ) { /* not writeable */
			errstr = strdup(destname);
			ierr   = ERROR_WRITE_PROTECTED;
			goto done;
		}
	}

	/*
	 *  check if copying necessary (update, fresh)
	 */
	if (OPT( CP_UPDATE | CP_FRESH )) {
		if (!dpd && OPT(CP_FRESH)) {
			if( !OPT(CP_QUIET)) {
				if (OPT(CP_OUTPUTFORMAT2)) {
					print_path(21);		/*GMD27Jun95 */
					printf("%s....not there\n", srcname);
				}
				else {
					printf("%*s%s....not there\n",level*6," ",srcname);
				}
			}
			goto done;
		} else if ( dpd && dptrtosecs(dpd) >= dptrtosecs(dps) &&
		             !stricmp(dps->fib->fib_FileName, dpd->fib->fib_FileName)) {
			if (!OPT(CP_QUIET)) {
				if (OPT(CP_OUTPUTFORMAT2)) {
					print_path(22);		/*GMD27Jun95 */
					printf("%s....not newer\n", srcname);
				}
				else {
					printf("%*s%s....not newer\n",level*6," ",srcname);
				}
			}
			goto done;
		}
	}

	/*
	 *	now open dest file
	 */
	if ( !(f_dest= Open(destname, dest_mode))) {
		errstr   = strdup(destname);
		ierr     = IoErr();
		goto done;
	}

	if (OPT(CP_OUTPUTFORMAT2)) {
		print_path(23);		/*GMD27Jun95 */
		printf("%s..", srcname);	/*GMD27Jun95 */
	}
	else {
		printf("%*s%s..",level*6," ",srcname);
	}

	fflush(stdout);

	/*
	 * now actually copy something :-)
	 */
	while (j = Read(f_src, buf, COPYBUF)) {
		if( dobreak() ) { 
			ierr = 513;
			break; 
		}
		if (Write(f_dest, buf, j) != j) { 
			ierr = IoErr(); 
			break; 
		}
	}
	Close(f_src);
	Close(f_dest);
	f_src= f_dest= NULL;

	/*
	 *	replace original src protection bits - maybe
	 */
	if ( reset ) {
		CurrentDir( srcdir );
		if (!setProtection(srcname, src_pbits )) {
			pError(srcname);
			goto done;
		}
		CurrentDir( destdir );
	}

	if (!ierr) {
		LONG tmp_pbits = src_pbits;
		/*
		 *  now check archive bit options (GMD)
		 */
		if (!OPT(CP_LEAVE_ARCHIVE)) {
			tmp_pbits &= ~FIBF_ARCHIVE;	/* no -a option, so clear bit */
		}

		setProtection(destname,tmp_pbits);

		if (!OPT(CP_NODATE)) 
			SetFileDate( destname, &dps->fib->fib_Date );

		if (!OPT(CP_NOFLAGS)) {
			if( *dps->fib->fib_Comment )
				SetComment( destname, dps->fib->fib_Comment );
		}
		if( OPT(CP_MOVE) ) {
			CurrentDir(srcdir);
			DeleteFile(srcname);
			printf("..moved\n");
		} else
			printf("..copied\n");
	} else {
		DeleteFile(destname);
	}

done:

	if ( f_src  )    Close(f_src);
	if ( f_dest )    Close(f_dest);
	if ( dps )       dclose(dps);
	if ( dpd )       dclose(dpd);
	if ( buf )       FreeMem(buf, COPYBUF);
	CurrentDir(cwd);

	return ierr;
}

/***************************************************************/
/*GMD27Jun95 stuff concerning dir copy */

#define DIR_PTRS_INC 32		/* increase dirname array by this amount  */

void push_dir_path(char *ptr)
{
	char *z_ptr;
	int nbytes;
	char **cur_dir_lst = dir_lst;

	/*
	 * now create (or increase) an array to hold dirname ptrs
	 */
	if (dir_lst == NULL) {
		dir_lst_sz = DIR_PTRS_INC;
		nbytes = dir_lst_sz * sizeof(char *);

		if ((dir_lst = (char **) malloc(nbytes)) == NULL) {
			return;
		}
	}
	else
		/* array exists */
	{
		/*
		 * if array full, rebuild it bigger
		 */
		if ((level % DIR_PTRS_INC) == 0) {
			dir_lst_sz = level + DIR_PTRS_INC;
			nbytes = dir_lst_sz * sizeof(char *);

			if ((dir_lst = (char **) malloc(nbytes)) == NULL) {
				return;
			}

			memcpy(dir_lst, cur_dir_lst, nbytes);	/* copy old array to new */

			free(cur_dir_lst);	/* free old array */
		}
	}

	dir_lst[level] = NULL;
	/*
	 * copy name to a malloc'd buffer
	 */

	if ((z_ptr = (char *) malloc(strlen(ptr) + 1)) == NULL) {
		return;
	}

	strcpy(z_ptr, ptr);

	dir_lst[level] = z_ptr;
}

void pop_dir_path(int offs)
{
	free(dir_lst[offs]);
	dir_lst[offs] = NULL;
}


void print_path(int z)
{
	int j;

	if (dir_lst[0] == NULL)
		return;

	for (j = 0; j <= level; ++j) {
		char *ptr = dir_lst[j];

		printf("%s", ptr);
		if (j == 0) {
			if (ptr[strlen(ptr) - 1] == ':')
				continue;
		}
		printf("/");
	}
}
/***************************************************************/

/* AMK: "touch" uses Dos.SetFileDate() now instead of internal file_date() */
int
do_touch( void )
{
	struct DateStamp ds;
	int i;
	BPTR lock;
	DateStamp(&ds);
	for (i=1; i<ac; i++) {
		if (SetFileDate(av[i],&ds))
			clear_archive_bit( av[i] );
		else {
			/* create file if it does not exist */
			if(lock=Open(av[i],MODE_READWRITE))
				Close(lock);
			else
				ierror(av[i],500);
		}
	}
	return 0;
}


int
do_addbuffers( void )
{
	long i,buffs,new;

	if (ac==2) {
		if (lastch(av[1])==':') {
			if (new=AddBuffers(av[1],0)) {
				if (new==DOSTRUE)
					new = IoErr();
				printf("%s has %ld buffers\n",av[1],new);
			}
			else {
				pError(av[1]);
			}
		}
		else
			printf("Invalid device or volume name '%s'\n",av[1]);

		return 0;
	}

	for( i=1; i<=ac-2; i+=2 ) {
		if( i==ac-1 ) {
			ierror( av[i], 500 );
			return 20;
		}
		buffs=myatoi(av[i+1],-32768,32767);
		if (atoierr)
			return 20;
		if (lastch(av[i])==':') {
			if (new=AddBuffers(av[i],buffs)) {
				if (new==DOSTRUE)
					new = IoErr();
				printf("%s has %ld buffers\n",av[i],new);
			}
			else {
				pError(av[i]);
			}
		}
		else
			printf("Invalid device or volume name '%s'\n",av[i]);

	}
	return 0;
}

int
do_relabel( void )
{
	/* AMK: using dos.Relabel() */
	if(!Relabel(av[1],av[2]))
		pError(av[2]);
#if 0
	Delay(10);
	changedisk(av[1]);
#endif
	return 0;
}

int
do_diskchange( void )
{
	int i;
	for (i=1; i<ac; i++)
		changedisk(av[i]);
	return 0;
}


#if 0
static void
changedisk(char *name)
{
	/*
	 *  2 calls to Inhibit() will break some stupid handlers,
	 *  so that DeviceProc()/GetDeviceProc() will fail on an
	 *  inhibited device.  So we first call GetDeviceProc()
	 *  2 times and send the packet ourself.  1 call to Get-
	 *  DeviceProc() won't be enough because it's not guaranteed
	 *  that the address is still valid after the first packet
	 *  sent to it (eg, for "diskchange CON:").
	 */

	struct MsgPort *task1=(struct MsgPort *)GetDeviceProc(name,NULL);
	struct MsgPort *task2=(struct MsgPort *)GetDeviceProc(name,NULL);

	printf("bla1 (%lu/%lu)\n",task1,task2);
	if (task1 && task2) {
		long rc;
		rc=DoPkt( task1, ACTION_INHIBIT, DOSTRUE,  NULL,NULL,NULL,NULL );
		printf("bla2 (%ld)\n",rc);
		Delay(10);
		rc=DoPkt( task2, ACTION_INHIBIT, DOSFALSE, NULL,NULL,NULL,NULL );
		printf("bla3 (%ld)\n",rc);
		Delay(10);
	}

	if (task1) FreeDeviceProc((struct DevProc *)task1);
	if (task2) FreeDeviceProc((struct DevProc *)task2);
}

#else

static void
changedisk(char *name)
{
	/* AMK: using dos.Inhibit() instead of DOS-Packets */
	Inhibit(name,DOSTRUE);
	Inhibit(name,DOSFALSE);
}
#endif


#if 0
/* old changedisk */
static void changedisk(struct MsgPort *task)
{
	DoPkt( task, ACTION_INHIBIT, DOSTRUE,  NULL,NULL,NULL,NULL );
	DoPkt( task, ACTION_INHIBIT, DOSFALSE, NULL,NULL,NULL,NULL );
}
#endif


extern int atoierr;

static int
func_array( char *fav[], int fac)
{
	char *ret;
	if( atoierr ) return 20;
	if( fac ) {
		ret=compile_av( fav, 0, fac, 0xa0, 0);
		set_var( LEVEL_SET, v_value, ret );
		free( ret );
	} else
		unset_var( LEVEL_SET, v_value );
	return 0;
}

static int
func_int( int i )
{
	char buf[12];
	if( atoierr ) return 20;
	sprintf(buf,"%d",i);
	set_var( LEVEL_SET, v_value, buf );
	return 0;
}

/*
 * called on error to create a NULL stringed VAR
 *
 */
static int
func_err () /*GMD 13FEB94*/
{
  set_var (LEVEL_SET, v_value, "");
  return 0;
}

static int
func_bool( int i )
{
	if( atoierr ) return 20;
	set_var( LEVEL_SET, v_value, i ? "1" : "0" );
	return 0;
}

static int
func_string( char *str )
{
	if( atoierr ) return 20;
	set_var( LEVEL_SET, v_value, str ? str : "" );
	return 0;
}

static int
commas( char *av[], int ac, int n )
{
	int i=0;

	while( --ac>=0 )
		if( !strcmp( *av++, ",") )
			i++;
	if( i-=n )
		fprintf( stderr, "Need %d comma%s\n", n, (n==1) ? "" : "s" );
	return i;
}

static int
wordset( char *av[], int ac, char **(*func)(char **,int,char**,int,int*,int) )
{
	char **av1=av, **av2;
	int  ac1=0, ac2, ret;

	if( commas( av, ac, 1 ) ) return 20;
	while( strcmp( *av++, ",") ) ac1++;
	av2=av, ac2=ac-ac1-1;
	av=(*func)( av1, ac1, av2, ac2, &ac, 0 );
	ret=func_array( av, ac );
	free( av );
	return ret;
}

static int
split_arg( char **av, int ac )
{
	char **arr, **old, *arg;
	int i, j=1, ret;

	for( i=strlen(av[0])-1; i>=0; i-- )
		if( av[0][i]==' ' )
			av[0][i]=0, j++;

	arr=old=(char **)salloc( j*sizeof( char * ) );
	arg = *av;
	for( ; j>0; j-- ) {
		*arr++=arg;
		arg+=strlen(arg)+1;
	}
	ret=func_array( old, arr-old );
	free(old);
	return ret;
}

static char *
info_part( char **av, int ac, int n, char *buf )
{
	char *str;
	DPTR *dp;
	int  len=0, i, t;

	buf[0]=0;
	while( --ac>=0 ) {
		if( dp=dopen( *av++, &t) ) {
			if( n==0 ) {
				for (str=buf, i=7; i>=0; i--)
					*str++= (dp->fib->fib_Protection & (1L<<i) ?
					        "hspa----" : "----rwed")[7-i];
				*str=0;
			} else
				len+= dp->fib->fib_NumBlocks+1;
			dclose( dp );
		}
	}
	if( n ) sprintf(buf, "%d", len);
	return buf;
}


/* AMK: now with asl filerequester */
static char *
file_request(char **av, int ac, char *path)
{
	struct FileRequester *frq;
	struct TagItem frq_tags[4];
	int tag = 0;
	BOOL ret = FALSE;

	if (ac>0) {
	  frq_tags[tag].ti_Tag  = ASL_Hail;
	  frq_tags[tag].ti_Data = (ULONG)av[0];
	  tag++;
	  if (ac>1) {
	    frq_tags[tag].ti_Tag  = ASL_Dir;
	    frq_tags[tag].ti_Data = (ULONG)av[1];
	    tag++;
	    if (ac>2) {
	      frq_tags[tag].ti_Tag  = ASL_File;
	      frq_tags[tag].ti_Data = (ULONG)av[2];
	      tag++;
	    }
	  }
	}
	frq_tags[tag].ti_Tag  = TAG_DONE;

	if (frq = AllocAslRequest(ASL_FileRequest,frq_tags)) {
	  if (ret=AslRequest(frq,NULL)) {
	    strcpy(path,frq->rf_Dir);
	    AddPart(path,frq->rf_File,200L);
	    /* AMK: length from dofunc().buf */
	  }
	  FreeAslRequest(frq);
	}

	if (ret) return(path);

	return(NULL);
}



/* NOTE (if you change something): ask() and confirm() are very similar */
int confirm( char *title, char *file )
{
	char buf[80];
	char *p;

	buf[0]=0;

	if( !confirmed ) {
		fprintf(stderr,"%s %s%-16s%s [YES/no/all/done] ? ",
		                title,o_hilite,file,o_lolite);
		fflush(stderr);
		if (p=fgets(buf,80,stdin))
			strupr(p);
		if (*buf=='A')
			confirmed=1;
		if (*buf=='D' || breakcheck())
			confirmed=2;
	}

	if( confirmed==2 )
		return 0;
	return confirmed || *buf != 'N';
}

/* NOTE (if you change something): ask() and confirm() are very similar */
int ask(char *title, char *file)
{
	char buf[80];
	char *p;

	buf[0]=0;

	if (!asked) {
		fprintf(stderr,"%s %s%-16s%s [yes/NO/all/quit] ? ",
		                title,o_hilite,file,o_lolite);
		fflush(stderr);
		if (p=fgets(buf,80,stdin))
			strupr(p);
		if (*buf=='A')
			asked=1;
		if (*buf=='Q' || breakcheck())
			asked=2;
	}

	if (asked==2)
		return 0;
	return asked || *buf == 'Y';
}



enum funcid {
	FN_STUB=1, FN_MEMBER, FN_DIRS, FN_NAMEEXT, FN_NAMEROOT, FN_FILES,
	FN_FILELEN, FN_SORTARGS, FN_UPPER, FN_WORDS, FN_ABBREV, FN_ABS,
	FN_BINTODEC, FN_CENTER, FN_COMPARE, FN_DECTOHEX, FN_DELWORD,
	FN_DELWORDS, FN_EXISTS, FN_INDEX, FN_STRCMP, FN_SUBWORDS,
	FN_WORD, FN_MIN, FN_MAX, FN_DRIVES, FN_WITHOUT, FN_UNION, FN_INTERSECT,
	FN_AVAILMEM, FN_UNIQUE, FN_RPN, FN_CONCAT, FN_SPLIT, FN_DRIVE,
	FN_FILEPROT, FN_FILEBLKS, FN_LOWER, FN_HOWMANY, FN_COMPLETE, FN_FIRST,
	FN_LAST, FN_MATCH, FN_CLINUM, FN_FREEBYTES, FN_FREEBLKS, FN_INFO,
	FN_MEGS, FN_FREESTORE, FN_CHECKPORT, FN_PICKOPTS, FN_PICKARGS,
	FN_FILEREQ, FN_VOLUME, FN_LOOKFOR, FN_APPSUFF, FN_DIRNAME, FN_AGE,
	FN_AGE_MINS,			/*GMD 13FEB94 */
	FN_GETCLASS, FN_CONFIRM, FN_WINWIDTH, FN_WINHEIGHT, FN_WINTOP,
	FN_WINLEFT, FN_CONSOLE, FN_SORTNUM, FN_IOERROR, FN_TRIM, FN_MOUNTED,
	FN_RND, FN_DIRSTR, FN_MIX, FN_WINROWS, FN_WINCOLS, FN_SUBFILE,
	FN_SCRHEIGHT, FN_SCRWIDTH, FN_FLINES, FN_STRICMP, FN_ASK,
	FN_HEXTODEC, FN_FILEINFO, FN_FILESIZE, FN_FILEDATE, FN_FILENOTE,
	FN_MKTEMP			/* GMD 21Dec95 */
};

#define MAXAV		30000		/* Max. # of arguments			*/

struct FUNCTION {
	short id, minargs, maxargs;
	char *name;
} Function[]={
FN_ABBREV,   2, 3,     "abbrev",
FN_ABS,      1, 1,     "abs",
FN_AGE,      1, 1,     "age",
FN_AGE_MINS, 1, 1,     "age_mins",	/*GMD 13FEB94 */
FN_APPSUFF,  2, 2,     "appsuff",
FN_PICKARGS, 0, MAXAV, "arg",
FN_ASK,      1, MAXAV, "ask",
FN_AVAILMEM, 0, 1,     "availmem",
FN_STUB,     1, 1,     "basename",
FN_CENTER,   2, 2,     "center",
FN_CHECKPORT,1, 1,     "checkport",
FN_CLINUM,   1, 1,     "clinum",
FN_COMPLETE, 1, MAXAV, "complete",
FN_CONCAT,   0, MAXAV, "concat",
FN_CONFIRM,  1, MAXAV, "confirm",
FN_CONSOLE,  1, 1,     "console",
FN_DECTOHEX, 1, 1,     "dectohex",
FN_DELWORD,  1, MAXAV, "delword",
FN_DELWORDS, 2, MAXAV, "delwords",
FN_DIRNAME,  1, 1,     "dirname",
FN_DIRS,     0, MAXAV, "dirs",
FN_DIRSTR,   2, 2,     "dirstr",
FN_DRIVE,    1, 1,     "drive",
FN_DRIVES,   0, 0,     "drives",
FN_EXISTS,   1, 1,     "exists",
FN_FILEBLKS, 1, MAXAV, "fileblks",
FN_FILEDATE, 1, 1,     "filedate",	/* GMD */
FN_FILEINFO, 2, 2,     "fileinfo",	/* GMD , alias for FN_DIRSTR */
FN_FILELEN,  0, MAXAV, "filelen",
FN_FILENOTE, 1, 1,     "filenote",	/* GMD */
FN_FILEPROT, 1, 1,     "fileprot",
FN_FILEREQ,  0, 3,     "filereq",
FN_FILES,    0, MAXAV, "files",
FN_FILESIZE, 1, 1,     "filesize",	/* GMD , alias for FN_FILELEN */
FN_FIRST,    0, MAXAV, "first",
FN_FLINES,   1, 1,     "flines",
FN_FREEBLKS, 1, 1,     "freeblks",
FN_FREEBYTES,1, 1,     "freebytes",
FN_FREESTORE,1, 1,     "freestore",
FN_STUB,     1, 1,     "getenv",
FN_GETCLASS, 1, 1,     "getclass",
FN_HEXTODEC, 1, 1,     "hextodec",	/* GMD */
FN_HOWMANY,  0, 0,     "howmany",
FN_IOERROR,  1, 1,     "ioerr",
FN_INDEX,    2, 2,     "index",
FN_INFO,     1, 1,     "info",
FN_INTERSECT,1, MAXAV, "intersect",
FN_LAST,     0, MAXAV, "last",
FN_LOOKFOR,  2, 2,     "lookfor",
FN_LOWER,    0, MAXAV, "lower",
FN_MATCH,    1, MAXAV, "match",
FN_MAX,      1, MAXAV, "max",
FN_MEGS,     1, 1,     "megs",
FN_MEMBER,   1, MAXAV, "member",
FN_MIN,      1, MAXAV, "min",
FN_MIX,      0, MAXAV, "mix",
FN_MKTEMP,   0, 0,     "mktemp",	/* GMD; 21Dec95 */
FN_MOUNTED,  1, 1,     "mounted",
FN_NAMEEXT,  1, 1,     "nameext",
FN_NAMEROOT, 1, 1,     "nameroot",
FN_PICKOPTS, 0, MAXAV, "opt",
FN_DIRNAME,  1, 1,     "pathname",
FN_PICKARGS, 0, MAXAV, "pickargs",
FN_PICKOPTS, 0, MAXAV, "pickopts",
FN_RND,      0, 1,     "rnd",
FN_RPN,      1, MAXAV, "rpn",
FN_SCRHEIGHT,0, 0,     "scrheight",
FN_SCRWIDTH, 0, 0,     "scrwidth",
FN_SORTARGS, 0, MAXAV, "sortargs",
FN_SORTNUM,  0, MAXAV, "sortnum",
FN_SPLIT,    0, MAXAV, "split",
FN_STRCMP,   2, 2,     "strcmp",
FN_STRICMP,  2, 2,     "stricmp",
FN_STUB,     2, 2,     "strhead",
FN_STUB,     2, 2,     "strleft",
FN_STUB,     2, 3,     "strmid",
FN_STUB,     2, 2,     "strright",
FN_STUB,     2, 2,     "strtail",
FN_SUBFILE,  3, 3,     "subfile",
FN_SUBWORDS, 2, MAXAV, "subwords",
FN_STUB,     2, 2,     "tackon",
FN_TRIM,     0, MAXAV, "trim",
FN_UNION,    1, MAXAV, "union",
FN_UNIQUE,   0, MAXAV, "unique",
FN_UPPER,    0, MAXAV, "upper",
FN_VOLUME,   1, 1,     "volume",
FN_WINCOLS,  0, 0,     "wincols",
FN_WINHEIGHT,0, 0,     "winheight",
FN_WINLEFT,  0, 0,     "winleft",
FN_WINROWS,  0, 0,     "winrows",
FN_WINTOP,   0, 0,     "wintop",
FN_WINWIDTH, 0, 0,     "winwidth",
FN_WITHOUT,  1, MAXAV, "without",
FN_WORD,     1, MAXAV, "word",
FN_WORDS,    0, MAXAV, "words",
0,           0, 0,     NULL
};

extern char shellctr[];
extern int  w_width, w_height;

int
dofunc( int id, char **av, int ac)
{
	char **oldav=av, **get=av, buf[200], *str=buf, *t;
	int oldac=ac, i=0, j=0, n=0, n2=1, l;
	buf[0]=0;
	av[ac]=NULL;
	atoierr=0;

	switch( id ) {
	case FN_ABBREV:
		if( ac==3 ) i=posatoi(av[2] ); else i=strlen(av[0]);
		return func_bool( !strnicmp( av[0], av[1], i ));
	case FN_ABS:
		i=unlatoi(av[0]);
		return func_int( i>= 0 ? i : -i );
	case FN_AGE: {
		struct DateStamp ds;
		long time;

		DateStamp(&ds);
		if( ds.ds_Days==0 )
			return func_err(); /* GMD 13FEB94, previously: return func_int(99999); */
		if( !(time = timeof(av[0])))
			return func_err(); /* GMD 13FEB94, previously: return func_int(99999); */
		return func_int( (ds.ds_Days*86400+ds.ds_Minute*60-time)/86400 );
		}
	case FN_AGE_MINS: {		/* GMD 13FEB94 */
		struct DateStamp ds;
		long time;
		long secs;

		DateStamp(&ds);
		secs = ds.ds_Days * 24 * 60 * 60;
		secs += ds.ds_Minute * 60;

		if (!(time = timeof(av[0])))
			return func_err();

		return func_int((secs - time) / 60);
		}
	case FN_APPSUFF:
		strcpy(buf,av[0]);
		l=strlen(av[0])-strlen(av[1]);
		if( l<0 || stricmp(av[0]+l,av[1])) {
			strcat(buf,".");
			strcat(buf,av[1]);
		}
		return func_string( buf );
	case FN_ASK:
		for (i=1, get++, asked=0; i<ac; i++)
			if (ask(av[0],av[i]))
				*get++=av[i];
		return func_array(av+1, (get-av)-1);
	case FN_AVAILMEM:
		if( ac==1 && !stricmp(av[0],"chip")) n=MEMF_CHIP;
		if( ac==1 && !stricmp(av[0],"fast")) n=MEMF_FAST;
		return func_int( AvailMem( n ));
	case FN_CENTER:
		if( (n=posatoi( av[1] )) > (l=strlen(av[0])) ) i=(n-l)/2, j=n-i-l;
		sprintf( buf, "%*s%s%*s", i,"",av[0], j,"" );
		return func_string( buf );
	case FN_CHECKPORT:
		return func_bool( (int)FindPort( av[0] ) );
	case FN_GETCLASS:
		if( str=getclass(av[0]) )
			if( str=index(strncpy( buf,str,100 ),0xA0) )
				*str=0;
		return func_string(buf);
	case FN_COMPLETE:
		for( i=1, l=strlen(av[0]); i<ac; i++ )
			if( !strnicmp( av[0], av[i], l ) )
				{ str=av[i]; break; }
		return func_string( str );
	case FN_CONCAT:
		return func_string( compile_av( av, 0, ac, ' ', 1));
	case FN_CONFIRM:
		for( i=1, get++, confirmed=0; i<ac; i++ )
			if( confirm( av[0], av[i]) )
				*get++=av[i];
		return func_array( av+1, (get-av)-1 );
	case FN_CONSOLE:
		if( !strcmp(av[0],"STDIN")) i=IsInteractive(Input());
		else if( !strcmp(av[0],"STDOUT")) i=IsInteractive(Output());
		return func_bool(i);
	case FN_DECTOHEX:
		sprintf( buf, "%x", unlatoi( av[0] ));
		return func_string( buf );
	case FN_DELWORDS:
		n2=posatoi( av[--ac] ); if( atoierr ) return 20;
	case FN_DELWORD:
		n=posatoi( av[--ac] )-1;
		for( ; i<ac && i<n; i++ ) *av++ = *get++;
		for( ; i<ac && i<n+n2; i++ ) get++;
		for( ; i<ac ; i++ ) *av++ = *get++;
		return func_array( oldav, av-oldav );
	case FN_DIRNAME:
		str=av[0]+strlen(av[0])-1;
		while( str>av[0] && *str!=':' && *str!='/' ) str--;
		if( *str==':' ) str++;
		*str=0;
		return func_string(av[0]);
	case FN_DIRS:
		for( ; --ac>=0; get++ )
			if( exists( *get ) && isdir( *get ) )
				*av++ = *get;
		return func_array( oldav, av-oldav );
	case FN_FILEINFO:
	case FN_DIRSTR:
		if( av=expand(oldav[1],&n2)) {
			lformat(oldav[0], buf, (FILEINFO *)av[0]-1);
			free_expand(av);
		}
		if( str=index(buf,'\n' )) *str=0;
		return func_string( buf );
	case FN_DRIVE:
		return func_string( drive_name( av[0] ) );
	case FN_DRIVES:
		get_drives( buf );
		return func_string( buf );
	case FN_EXISTS:
		return func_bool( exists( av[0] ));
	case FN_FILEBLKS:
		return func_string( info_part( av, ac, 1, buf ) );
	case FN_FILESIZE:
	case FN_FILELEN:
		while( --ac>=0 )
			i+=filesize( *av++ );
		return func_int( i );
	case FN_FLINES:
		if( str=get_var(LEVEL_SET, av[0]))
			for( n=1; str=index(str,0xA0); str++, n++ ) ;
		return func_int( n );
	case FN_FILEPROT:
		return func_string( info_part( av, ac, 0, buf ) );
	case FN_FILEREQ:
		return func_string( file_request( av, ac, buf ) );
	case FN_FILES:
		for( ; --ac>=0; get++ )
			if( exists( *get ) && !isdir( *get ) )
				*av++ = *get;
		return func_array( oldav, av-oldav );
	case FN_FILEDATE:
		return func_string(get_filedate(av[0]));
	case FN_FILENOTE:
		return func_string(get_filenote(av[0]));
	case FN_FIRST:
		return func_string(av[0]);
	case FN_FREEBLKS:
		return func_string(oneinfo(av[0],3));
	case FN_FREEBYTES:
		return func_string(oneinfo(av[0],2));
	case FN_FREESTORE:
		return func_string(oneinfo(av[0],4));
	case FN_HEXTODEC:
		{
		long num = 0;
		stch_l((av[0]),&num);
		sprintf(buf,"%ld",num);
		return func_string(buf);
		}
	case FN_HOWMANY:
		/* AMK: OS20-GetVar replaces ARP Getenv */
		GetVar(shellctr,buf,10,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
		return func_string( buf );
	case FN_IOERROR:
		return func_string( ioerror( atoi( av[0] )));
	case FN_INDEX:
		str=strstr( av[0], av[1] );
		return func_int( str ? (str-av[0])+1 : 0 );
	case FN_INFO:
		return func_string( oneinfo( av[0], 1 ));
	case FN_INTERSECT:
		return wordset( av, ac, and );
	case FN_LAST:
		return func_string( ac ? av[ac-1] : "" );
	case FN_LOOKFOR:
		return func_string( dofind( av[0], "", buf, av[1]));
	case FN_LOWER:
		while( --ac>=0 ) strlwr( *av++ );
		return func_array( oldav, av-oldav );
	case FN_MATCH:
		{ PATTERN *pat=compare_preparse( av[--ac], 0 );
		for( ; --ac>=0; get++ )
			if( compare_ok( pat, *get ) )
				*av++ = *get;
		compare_free(pat);
		return func_array( oldav, av-oldav );
		}
	case FN_MAX:
		for( n=MININT; i<ac; i++ ) 
			{ if( (j=unlatoi(av[i] )) > n ) n=j; if( atoierr ) return 20; }
		return func_int( n );
	case FN_MEGS:
		return func_string( itok( atoi( av[0] )));
	case FN_MEMBER:
		for( i=1; i<ac && stricmp(av[0],av[i]) ; i++ ) ;
		return func_bool( ac!=i );
	case FN_MIN:
		for( n=MAXINT; i<ac; i++ )
			{ if( (j=unlatoi(av[i] )) < n ) n=j; if( atoierr ) return 20; }
		return func_int( n );
	case FN_MIX:
		for( ; i<ac; i++ )
			j=rand()%ac, str=av[j], av[j]=av[i], av[i]=str;
		return func_array(av,ac);
	case FN_MKTEMP: {		/* GMD 21Dec95 */
		sprintf(buf, "T:");
		if (exists(buf)) {
			do {
				sprintf(&buf[strlen (buf)], "tmp%08X", rand());
			} while (exists(buf));
		}
		else {
			do {
				sprintf(buf, "tmp%08X", rand());
			} while (exists(buf));
		}
		return func_string(buf);
		}
	case FN_MOUNTED:
		return func_bool((int)mounted(av[0]));
	case FN_NAMEEXT:
		return func_string( rindex(av[0],'.')?rindex(av[0],'.')+1:(char *)NULL);
	case FN_NAMEROOT:
		if( rindex(av[0],'.') ) *rindex(av[0],'.')=0;
		return func_string( av[0] );
	case FN_PICKARGS:
		while( *get && **get=='-' ) get++;
		while( *get )  *av++ = *get++ ;
		return func_array( oldav, av-oldav );
	case FN_PICKOPTS:
		while( *get && **get=='-' ) *av++ = *get++;
		return func_array( oldav, av-oldav );
	case FN_CLINUM:
		return func_int( clinum( av[0] ) );
	case FN_RND: {
		if (ac>0) {
			if (atoi(av[0]) == 0) {
				static long prev_time = 0;
				long curr_time = time(NULL);
				while (curr_time == prev_time) {
					Delay(TICKS_PER_SECOND / 5);
					curr_time = time(NULL);
				}
				srand(curr_time);
				prev_time = curr_time;
				/*
				 *  make numbers after successive calls
				 *  of @rnd(0) more random ...
				 */
				rand();
				rand();
				rand();
			}
			else
				srand(atoi(av[0]));
		}
		return func_int( rand() );
		}
	case FN_RPN:
		return func_int( eval_rpn( av, ac, 1 ));
	case FN_SCRHEIGHT:
		return func_int( Mywindow ? Mywindow->WScreen->Height: 0 );
	case FN_SCRWIDTH:
		return func_int( Mywindow ? Mywindow->WScreen->Width : 0 );
	case FN_SORTARGS:
		QuickSort( av, ac );
		return func_array( av, ac );
	case FN_SORTNUM:
		DirQuickSort( av, ac, numcmp, 0, 0 );
		return func_array( av, ac );
	case FN_SPLIT:
		return split_arg( av, ac );
	case FN_STRCMP:
		return func_int( strcmp( av[0], av[1] ) );
	case FN_STRICMP:
		return func_int( stricmp( av[0], av[1] ) );
	case FN_TRIM:
		for( ; *av; av++ ) {
			for( ; **av==' '; (*av)++ ) ;
			for( str = *av+strlen(*av); str>*av && str[-1]==' '; *--str=0 ) ;
		}
		return func_array( oldav, av-oldav );
	case FN_UNION:
		return wordset( av, ac, or );
	case FN_UNIQUE:
		QuickSort( av, ac );
		while( *get )
			{ *av++ = *get++; while( *get && !stricmp(*get,*(get-1)))
				get++; }
		return func_array( oldav, av-oldav );
	case FN_UPPER:
		while( --ac>=0 ) strupr( *av++ );
		return func_array( oldav, oldac );
	case FN_VOLUME:
		return func_string( oneinfo( av[0], 5 ));
	case FN_WINCOLS:
		newwidth(); /**/
		return func_int(w_width);
	case FN_WINTOP:
		return func_int( Mywindow ? Mywindow->TopEdge : 0 );
	case FN_WINLEFT:
		return func_int( Mywindow ? Mywindow->LeftEdge: 0 );
	case FN_WINHEIGHT:
		return func_int( Mywindow ? Mywindow->Height : 0 );
	case FN_WINROWS:
		newwidth(); /**/
		return func_int(w_height);
	case FN_WINWIDTH:
		return func_int( Mywindow ? Mywindow->Width  : 0 );
	case FN_WORD:
		n2=1; goto wordf;
	case FN_SUBFILE:
		n2= posatoi( av[2] ); if( atoierr ) return 20;
		n = posatoi( av[1] ); if( atoierr ) return 20;
		if( !(str=get_var( LEVEL_SET, av[0] ))) return 20;
		for( i=1; i<n; i++ )
			str= (str=index(str,0xA0)) ? str+1 : "";
		for( t=str, i=0; i<n2; i++ )
			if(t=index(t+1,0xA0));
		if( t ) *t=0;
		set_var( LEVEL_SET, v_value, str );
		if( t ) *t=0xA0;
		return 0;
	case FN_SUBWORDS:
		n2=posatoi( av[--ac] ); if( atoierr ) return 20;
	wordf:
		n=posatoi( av[--ac] )-1; if( atoierr ) return 20;
		for( i=0; i<ac && i<n; i++ ) get++;
		for(    ; i<ac && i<n+n2; i++ ) *av++ = *get++;;
		return func_array( oldav, av-oldav );
	case FN_WITHOUT:
		return wordset( av, ac, without );
	case FN_WORDS:
		return func_int( ac );
	}
	return func_string( "" );
}



static char *get_filenote(char *file)	/* new function by GMD */
{
	static char buf[80];
	DPTR *dp;
	int stat;

	buf[0] = '\0';
	if (dp = dopen(file, &stat)) {
		strcpy(buf, dp->fib->fib_Comment);
		dclose(dp);
	}
	return buf;
}

static char *get_filedate(char *file)	/* new function by GMD */
{
	static char buf[80];
	DPTR *dp;
	int stat;
	char *ptr;

	buf[0] = '\0';
	if (dp = dopen(file,&stat)) {
		ptr = dates(&dp->fib->fib_Date,0);
		strcpy(buf,ptr);
		dclose(dp);
	}
	return buf;
}

