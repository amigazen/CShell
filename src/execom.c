/*
 * EXECOM.C
 *
 * Matthew Dillon, 10 August 1986
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 *
 */

#include "shell.h"

#define DEFAULTFRAME 1024
#define Strlen(x)    strlen((char*)x)
#define Strcpy(x,y)  strcpy((char*)x,(char*)y)
#define Strcat(x,y)  strcat((char*)x,(char*)y)
#define Index(x,y)   ((UBYTE*)index((char*)x,y))
#define CHECKAV(num) { if( ac+num+10>max_ac ) checkav( frameptr, num ); }

typedef struct StackFrame {
	struct StackFrame *next;
	int  bytesleft;
	char *ptr;
	char mem[1];
} FRAME;

/* execom.c */
int   fcomm(UBYTE *str,FRAME **frame,char *from );
int   hasspace( char *s );
int   myfgets( char *buf, FILE *in );

static char  *compile_avf(FRAME **frame,char **av,int start,int end,char delim,int quote);
static int   preformat(char *s, UBYTE *d);
static void  backtrans(UBYTE *str);
static UBYTE *exarg(UBYTE **ptr, UBYTE *elast);
static UBYTE *format_insert_string(FRAME **frameptr, UBYTE *str, char *from);
static int   find_command(char *str);
static int   checkav( FRAME **frame,int n );
static char  *tempname( int which );
static UBYTE *skipword( UBYTE *strskip );

static void  newframe( FRAME **frame, int bytes);
static void  *falloc( FRAME **frame, int bytes );
static void  *frealloc( FRAME **frame, char *oldstring, int morebytes );
static void  funalloc( FRAME **frame, int bytes );
static char  *push_cpy( FRAME **frame, void *s);
static void  deleteframe( FRAME *frame);

void  exec_every(void);

int do_nothing(void);

struct COMMAND {
	int (*func)();
	short minargs;
	short stat;
	int val;
	char *name;
	char *options;
	char *usage;
};

#define ST_COND   0x01 /* this is an 'if' type command, no redirection */
#define ST_NORED  0x02 /* this command can't be redirected             */
#define ST_NOEXP  0x04 /* no wild card expansion for this command      */
#define ST_A0     0x08 /* delimit args with 0xA0 instead of ' '        */
#define ST_FUNC   0x10 /* can be used as a function                    */
#define ST_STORED 0x20 /* stores redirection instead of execuing it    */
#define ST_AVLINE 0x40 /* needs avline                                 */

#define COND  ST_COND
#define NORED ST_NORED
#define STRED ST_STORED
#define NOEXP ST_NOEXP
#define A0   (ST_A0|ST_AVLINE)
#define AV    ST_AVLINE
#define FUNC  ST_FUNC

#define ALIAS LEVEL_ALIAS
#define SET   LEVEL_SET

#define FIRSTCOMMAND 4
#define COM_EXEC 3

static struct COMMAND Command[] = {
 do_run,       0,   A0,     0, "\001",       NULL, NULL, /* may call do_source, do_cd */
 do_nothing,   0,    0,     0, "\002",       NULL, NULL, /* dummy for aliases   */
 do_nothing,   0,    0,     0, "\003",       NULL, NULL, /* dummy for aliases with args */
 do_run,       0,NORED|A0,  1, "\004",       NULL, NULL, /* external that needs to be Execute()d */
 do_abortline, 0,    0,     0, "abortline",  NULL, "",
 do_action,    2,   AV,     9, "action",     "av", "action file [args]",
 do_addbuffers,1,    0,     0, "addbuffers", NULL, "{drive [bufs]}",
 do_tackon,    3, FUNC,     0, "addpart",    NULL, "var pathname filename",
 do_set_var,   0,    0, ALIAS, "alias",      NULL, "[name [string] ]",
 do_ascii,     0,    0,     0, "ascii",      "oh", "-oh [string]",
 do_assign,    0,    0,     0, "assign",     "adpln",",logical,-adp {logical physical}",
 do_basename,  2, FUNC,     0, "basename",   NULL, "var path",
 do_cat,       0,    0,     0, "cat",        "n",  "-n [file file...]",
 do_cd,        0,    0,     0, "cd",         "g",  "[path],-g path...path",
 do_chgrp,     2,    0,     0, "chgrp",      NULL, "group file...file",
 do_chmod,     2,    0,     0, "chmod",      NULL, "flags file...file",
 do_chown,     2,    0,     0, "chown",      "g", "owner file...file",
 do_class,     0,   A0,     0, "class",      "n",  "-n name {type=param} \"actions\" {action=command}",
 do_close,     0,    0,     0, "close",      NULL, "filenumber",
 do_copy,      1,    0,     0, "copy",       "rudpfmqoax","-adfmopqrux file file,-ud file...file dir,-ud dir...dir dir",
 do_copy,      1,    0,     0, "cp",         "rudpfmqoax","-adfmopqrux file file,-ud file...file dir,-ud dir...dir dir",
 do_date,      0,    0,     0, "date",       "srb","-bsr [date/time]",
 do_inc,       1,    0,    -1, "dec",        NULL, "varname [step]",
 do_rm,        0,    0,     0, "delete",     "rfpqv","-fpqrv file...file",
 do_dir,       0,NOEXP,     0, "dir",        "sfdcnhltbuikqavopzegL","-abcdefghiklnopqstuvL [-z lformat] [path...path]",
 do_diskchange,1,    0,     0, "diskchange", NULL, "drive...drive",
 do_echo,      0,    0,     0, "echo",       "ne", "-ne string",
 do_if,        0, COND,     1, "else",       NULL, "",
 do_if,        0, COND,     2, "endif",      NULL, "",
 do_error,     1,    0,     0, "error",      NULL, "num",
 do_exec,      1,   AV,     0, "exec",       "i",  "-i command",
 do_fault,     1,    0,     0, "fault",      NULL, "error",
 do_filenote,  1,    0,     0, "filenote",   "s",  "file...file note,-s file...file",
 do_fileslist, 0,    0,     0, "flist",      NULL, "",
 do_fltlower,  0,    0,     0, "fltlower",   NULL, "<in >out",
 do_fltupper,  0,    0,     0, "fltupper",   NULL, "<in >out",
 do_foreach,   3,    0,     0, "foreach",    "v",  "-v varname ( string ) command",
 do_forever,   1,   AV,     0, "forever",    NULL, "command",
 do_forline,   3,    0,     0, "forline",    NULL, "var filename command",
 do_fornum,    4,    0,     0, "fornum",     "vs", "-vs var n1 n2 command",
 do_getenv,    1, FUNC,     0, "getenv",     NULL, "shellvar envvar",
 do_goto,      1,    0,     0, "goto",       NULL, "label",
 do_head,      0,    0,     0, "head",       NULL, "[filename] [num]",
 do_help,      0,    0,     0, "help",       "f",  "help -f",
 do_history,   0,    0,     0, "history",    "nr", "-nr [partial_string]",
 do_howmany,   0,    0,     0, "howmany",    NULL, "",
 do_htype,     0,    0,     0, "htype",      "r",  "-r [file...file]",
 do_if,        1,COND|NORED,0, "if",         "rftmdvone","-n arg cond arg,-n arg,-nf file,-nd dir,-nm,-nt file...file,-nr rpn_expr,-no opt args,-v varname",
 do_inc,       1,    0,     1, "inc",        NULL, "varname [step]",
 do_info,      0,    0,     0, "info",       "pt", "-pt [drive...drive]",
 do_input,     1,    0,     0, "input",      "sr", "-rs var...var",
 do_join,      2,    0,     1, "join",       "r",  "-r file...file",
 do_keymap,    1,    0,     0, "keymap",     "n",  "-n number {key=function}",
 do_label,     1, COND,     0, "label",      NULL, "name",
 do_local,     0,    0,     0, "local",      NULL, "[var...var]",
 do_linecnt,   0,    0,     0, "linecnt",    NULL, "<in >out",
 do_ln,        1,    0,     0, "ln",         "s",  "-s original [link]",
 do_dir,       0,NOEXP,     0, "ls",         "sfdcnhltbuikqavopzegL","-abcdefghiklnopqstuvL [-z format] [path...path]",
 do_ln,        1,    0,     0, "makelink",   "s", "-s original [link]",
 do_man,       0,    0,     0, "man",        NULL, "command...command",
 do_mkdir,     1,    0,     0, "md",         "p",  "-p name...name",
 do_mem,       0,    0,     0, "mem",        "cfqsrl","-cflqsr",
 do_menu,      0,    0,     0, "menu",       "nm", "-nm [title item...item]",
 do_mkdir,     1,    0,     0, "mkdir",      "p",  "-p name...name",
 do_mv,        2,    0,     0, "mv",         "fv", "-fv from to,from...from todir",
 do_open,      3,    0,     0, "open",       NULL, "file mode number",
 do_path,      0,    0,     0, "path",       "rg", "-gr [dir...dir]",
 do_pri,       2,    0,     0, "pri",        NULL, "clinumber pri,0 pri",
 do_protect,   2,    0,     0, "protect",    NULL, "file...file flags",
 do_ps,        0,    0,     0, "ps",         "les","-els [commandname...commandname]",
 do_pwd,       0,    0,     0, "pwd",        NULL, "",
 do_qsort,     0,    0,     0, "qsort",      "rc",  "-cr <in >out",
 do_quit,      0,NORED,     0, "quit",       NULL, "",
 do_truerun,   1,NORED|AV,  1, "rback",      NULL, "command",
 do_mv,        2,    0,     0, "rename",     "fv", "-fv from to,from...from todir",
 do_readfile,  1,    0,     0, "readfile",   NULL, "varname [filename]",
 do_rehash,    0,    0,     0, "rehash",     "closg","-cglos",
 do_relabel,   2,    0,     0, "relabel",    NULL, "drive name",
 do_resident,  0,    0,     0, "resident",   "ard",",-ard file...file",
 do_return,    0,    0,     0, "return",     NULL, "[n]",
 do_rm,        0,    0,     0, "rm",         "rfpqv","-fpqrv file...file",
 do_rpn,       0,NOEXP,     0, "rpn",        NULL, "expression",
 do_rxrec,     0,    0,     0, "rxrec",      NULL, "[portname]",
 do_rxsend,    2,   AV,     0, "rxsend",     "rl", "-lc portname string",
 do_truerun,   1,STRED|AV,  0, "run",        NULL, "command",
 do_search,    2,    0,     0, "search",     "rcwneqvbfalo","-abceflnoqrvw file...file string",
 do_set_var,   0,    0,   SET, "set",        NULL, "[name [string] ]",
 do_setenv,    2,    0,     0, "setenv",     NULL, "var value",
 do_sleep,     0,    0,     0, "sleep",      "t", "timeout",
 do_split,     1,    0,     0, "split",      NULL, "srcvar dstvar...dstvar",
 do_source,    1,   A0,     0, "source",     NULL, "file",
 do_stack,     0,    0,     0, "stack",      "s",  "-s [bytes]",
 do_strhead,   3, FUNC,     0, "strhead",    NULL, "varname breakchar string",
 do_strings,   0,    0,     0, "strings",    "rbnv", "-bnrv [file...file] [minlength]",
 do_strleft,   3, FUNC,     0, "strleft",    NULL, "varname string n",
 do_strlen,    2, FUNC,     0, "strlen",     NULL, "varname string",
 do_strmid,    3, FUNC,     0, "strmid",     NULL, "varname string n1 [n2]",
 do_strright,  3, FUNC,     0, "strright",   NULL, "varname string n",
 do_strtail,   3, FUNC,     0, "strtail",    NULL, "varname breakchar string",
 do_tackon,    3, FUNC,     0, "tackon",     NULL, "var pathname filename",
 do_head,      0,    0,     1, "tail",       NULL, "[filename] [num]",
 do_tee,       0,    0,     0, "tee",        NULL, "<in >out",
 do_touch,     1,    0,     0, "touch",      NULL, "file...file",
 do_truncate,  0,    0,     0, "truncate",   NULL, "<in >out",
 do_cat,       0,    0,     0, "type",       "n",  "-n [file...file]",
 do_unset_var, 0,   AV, ALIAS, "unalias",    NULL, "name...name",
 do_uniq,      0,    0,     0, "uniq",       NULL, "<in >out",
 do_unset_var, 0,    0,   SET, "unset",      NULL, "name...name",
 do_usage,     0,    0,     0, "usage",      NULL, "[command...command]",
 do_version,   0,    0,     0, "version",    "filr", "[-filr name]",
 do_waitport,  1,    0,     0, "waitforport",NULL, "portname [seconds]",
 do_whereis,   1,NOEXP,     0, "whereis",    "r",  "-r file [path...path]",
 do_window,    0,NOEXP,     0, "window",     "slfbaqwk","-slfbaqwk",
 do_writefile, 1,    0,     0, "writefile",  NULL, "var >out",
 NULL,         0,    0,     0, NULL,         NULL, NULL,
};


/* do_which,     1,    0,     0, "which",      NULL, "command", */

#ifdef isalphanum
char isalph[256];
#endif

int
execute( char *str )
{
	ULONG toptions=options;
	int   ret, oldac=ac, oldmax=max_ac;
	char  **oldav=av;

	if( !str ) return -1;

	++H_stack;
	ret=exec_command(str);
	--H_stack;

	av=oldav; max_ac=oldmax; ac=oldac;
	options=toptions;

	return ret;
}


#if 1
int
exec_command( char *base )
{
	FRAME *frame=NULL;
	UBYTE *scr;
	char  buf[6];
	int   len, newlen, ret;

	if (!H_stack && S_histlen>1) {
		add_history(base);
		sprintf(buf, "%d", H_tail_base + H_len);
		set_var(LEVEL_SET, v_histnum, buf);
	}

	len=Strlen(base)*3+20;
	newframe( &frame, len+DEFAULTFRAME );

	scr   = falloc(&frame,len);
	newlen= preformat( base,scr );

	funalloc( &frame, len-newlen-2 );
	ret=fcomm(scr,&frame,NULL);

	deleteframe( frame );

	return ret;
}
#else
int
exec_command( char *base )
{
	FRAME *frame=NULL;
	UBYTE *scr;
	char  buf[6], expandbuf[1024];
	int   len, newlen, ret;

   preprocess(base, expandbuf);  /* the call to my routine to process
				    history shortcuts */

	if (!H_stack && S_histlen>1) {
		add_history(expandbuf);
		sprintf(buf, "%d", H_tail_base + H_len);
		set_var(LEVEL_SET, v_histnum, buf);
	}

	len=Strlen(expandbuf)*3+20;
	newframe( &frame, len+DEFAULTFRAME );

	scr   = falloc(&frame,len);
	newlen= preformat( expandbuf,scr );

	funalloc( &frame, len-newlen-2 );
	ret=fcomm(scr,&frame,NULL);

	deleteframe( frame );

	return ret;
}


/* inserthistory() is called by preprocess() when every a '!' is 
encountered.  So 'echo !!' will display the last line, for instance.
	!!  The last line in history
	!$  The last argument in the last line in history
		'echo a b c' => 'c'
	!*  Everything but the command in the last line in history
		'echo a b c' => 'a b c'
*/
static char  *inserthistory(char  *to, char  type)
{
   char  *lastline, *p;
   
   switch (type) {
      case '!':
         lastline = get_num_history(1);
         to = stpcpy(to, lastline);
         break;
      case '$':
         lastline = get_num_history(1);
         p = lastline + strlen(lastline) - 1;
		/* does not handle quoted arguments properly */
         while (p >= lastline && !isspace(*p))
            --p;
         to = stpcpy(to, ++p);
         break;
      case '*':
         lastline = get_num_history(1);
         while (!isspace(*lastline) && *lastline)
            ++lastline;
         while (isspace(*lastline) && *lastline)
            ++lastline;
         to = stpcpy(to, lastline);
         break;
      default:
         *to++ = type;
         break;
   }
   return to;
}

static void  preprocess(char  *from, char  *buf)
{
   int  printit = 0;
   char  *to = buf;
   int  quoteflag = 0;
   
   while (*from) {
      switch (*from) {
         case '!':
            ++from;
            to = inserthistory(to, *from);
            printit = 1;
            ++from;
            break;
         case '\\':
            *to++ = *from++;
            if (*from)
               *to++ = *from++;
            break;
         case '\'':
         case '"':
            if (quoteflag == 0)
               quoteflag = *from;
            else if (quoteflag == *from) {
               quoteflag = 0;
            }
            *to++ = *from++;
            break;
         default:
            *to++ = *from++;
            break;
      }
   }
   *to = '\0';
   if (printit)
      fprintf(stderr, "'%s' => '%s'\n", from, buf);
}
#endif

#ifndef isalphanum
isalphanum( char c )
{
	return (
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		(c == '_')
	);
}
#endif

int isnum(char *s)
{
	int num = 0;
	if (s) {
		while(isspace(*s))   /* skip spaces */
			s++;
		if (*s=='-' || *s=='+')   /* skip sign */
			s++;
		num = isdigit(*s);   /* must be a digit */
		while(isdigit(*s))   /* skip digits */
			s++;
		if (!(*s=='\0' || isspace(*s)))
			num = 0;
	}
	return(num);
}

#define HOT_GAP    0x80
#define HOT_BLANK  0x81
#define HOT_STAR   0x82
#define HOT_QUES   0x83
#define HOT_EXCL   0x84
#define HOT_SEMI   0x85
#define HOT_PIPE   0x86
#define HOT_DOLLAR 0x87
#define HOT_IN     0x88
#define HOT_OUT    0x89
#define HOT_BSLASH 0x8a
#define HOT_LASTCD 0x8b
#define HOT_BACKG  0x8c
#define HOT_CURLL  0x8d
#define HOT_CURLR  0x8e
#define HOT_CURDIR 0x8f
#define HOT_PARDIR 0x90
#define HOT_BEGOUT 0x91
#define HOT_ENDOUT 0x92
#define HOT_EQUAL  0x93
#define HOT_BEGIN  0x94
#define HOT_APOSTR 0x95
#if 1
/* AMK: detect DOS patterns */
#define HOT_PM1    0x96
#define HOT_ECKL   0x97
#define HOT_ECKR   0x98
#endif

/* USHORT ArgPos[256]; */

static UBYTE termchar[]={ 0,    '}',       ')',        '`' };
static UBYTE hotchar []={ 0, HOT_CURLR, HOT_ENDOUT, HOT_APOSTR };

static int
preformat( char *src, UBYTE *d )
{
	int qm=0, i, level, curmode, argc=0, beg=1;
	UBYTE mode[100], c, *s=(UBYTE*)src, *start=d;

	while (*s) {
		if (qm ) {
			/*
			   AMK:
			   Use 'o_cquote' here?
			   qm=quote mode?
			*/
			while( isalphanum( *s ) || (*s && *s!='\"' && *s!='\\' && *s!='\n'))
				*d++ = *s++;
			if( !*s ) break;
			if( *s=='\n' ) qm=0, *s=';';
		}
		if( beg ) c=HOT_BEGIN,beg=0; else c = *s;
		switch (c) {
		case ' ':
		case 9:
			*d++ = HOT_BLANK;
argstart:
			while (*s==' ' || *s==9) ++s;
			if( argc && (!*s || *s=='|' || *s==';' || *s=='#' || *s=='\n' ))--d;
			if(      *s=='\\' && !argc ) { *d++=HOT_BSLASH; if( *d++ = *++s ) ++s; }
			else if( *s=='~'  )          { *d++=HOT_LASTCD; s++; }
			else if( *s=='.'  )
				for( ;; ) {
					if( s[0]=='.' && issep(s[1]))
						s+=1+(s[1]=='/');
					else if( s[0]=='.' && s[1]=='.' && issep(s[2]))
						*d++='/', s+=2+(s[2]=='/');
					else
						break;
				}
			argc++;
			break;
		case '*':
			*d++ = HOT_GAP;
			*d++ = HOT_STAR;
			++s;
			break;
		case '?':
			*d++ = HOT_GAP;
			*d++ = HOT_QUES;
			++s;
			break;
#if 1
		/* AMK --> detect DOS patterns */
/*
		case '(':
			*d++ = HOT_GAP;
			*d++ = HOT_PM1;
			++s;
			break;
*/
		case '[':
			*d++ = HOT_GAP;
			*d++ = HOT_ECKL;
			++s;
			break;
		case ']':
			*d++ = HOT_GAP;
			*d++ = HOT_ECKR;
			++s;
			break;
		/* <-- AMK */
#endif
		case '!':
			*d++ = HOT_EXCL;
			++s;
			break;
		case HOT_BEGIN:
			argc=0;
			goto argstart;
		case '#':
			while (*s && *s!='\n') ++s;
			if( !*s ) break;
		case '\n':
		case ';':
			*d++= HOT_SEMI, argc=0, s++;
			goto argstart;
		case '|':
			*d++= HOT_PIPE, argc=0, s++;
			goto argstart;
		case '\\':
			if( (i = *++s-'0')>=0 && i<=7 ) {
				if( *++s>='0' && *s<='7' ) {
					i= 8*i + *s++-'0';
					if( *s>='0' && *s<='7' )
						i= 8*i + *s++-'0';
				}
				*d++ = i;
			} else {
				*d++ = *s;
				if (*s) ++s;
			}
			break;
		case '\"':
			/*
			   AMK:
			   Use 'o_cquote' here?
			*/
			if (o_cquote) {
				if (qm) {
					/*printf("\" ende (%s)\n",s);*/
					qm = 1 - qm;
					*d++ = HOT_BLANK;
					++s;
					goto argstart;
				}
				else {
					if (s==src || (*(s-1)==' ') ) {
						/*printf("\" am beginn (%s)\n",s);*/
						qm = 1 - qm;
						++s;
					}
					else {
						/*printf("\" mittendrin (%s)\n",s);*/
						*d++ = *s++;
					}
				}
			}
			else {	/* UNIX like quoting */
				qm = 1 - qm;
				++s;
			}
			break;
		case '^':
			*d++ = *++s & 0x1F;
			if (*s) ++s;
			break;
		case '<':
			*d++ = HOT_IN;
			++s;
			break;
		case '>':
			*d++ = HOT_OUT;
			++s;
			break;
		case '&':
			*d++ = HOT_BACKG;
			++s;
			break;
		case '$': /* search end of var name */
			if( s[1]=='(' ) {
				curmode=2;
				*d++=HOT_BEGOUT;
				*d++ = *++s;
				goto brace;
			}
			*d++ = HOT_GAP;
			*d++ = HOT_DOLLAR;
			++s;
			while (isalphanum(*s)) *d++ = *s++;
			*d++ = HOT_GAP;
			break;
		case '`':
			curmode=3;
			*d++=HOT_APOSTR;
			goto brace;
		case '{':
			curmode=1;
			*d++ = HOT_CURLL;
brace:
			level=0; s++;
			mode[level++]=curmode;
			while( *s && level ) {
				switch( *s ) {
				/*
				   AMK:
				   Use 'o_cquote' here?
				*/
				case '\"': *d++ = *s++;
				           while( *s && *s!='\"' && *s!='\n')
				               if( *s=='\\' ) { *d++ = *s++; if( *s ) *d++ = *s++; }
				               else *d++ = *s++;
				           if( *s ) *d++ = *s++;
				           break;
				case '\\': *d++ = *s++; if( *s ) *d++ = *s++;
				           break;
				case '{' : mode[level++]=1; *d++ = *s++;
				           break;
				case '}' :
				case ')' :
				case '`' : for( i=level-1; i>=0 && termchar[mode[i]] != *s;--i ) ;
				           if( i==0 ) *d++=hotchar [mode[i]];
				           else       *d++ = *s;
				           if( i>=0 ) level=i;
				           s++;
				           break;
				default  : *d++ = *s++;
				           break;
				}
			}
			break;
		default:
			*d++ = *s++;
			while( *s>=65 && (*s&31)<=26 )
				*d++ = *s++;
			break;
		}
	}
endwhile:
	*d++=0;
	*d=0;
	if (debug) fprintf (stderr,"PREFORMAT: %d :%s:\n", Strlen(start), start);
	return d-start;
}

static void
backtrans( UBYTE *str )
{
	if( !str )
		return;

	while( *str ) {
		while( *(signed char *)str>0 ) str++;
		if( !*str ) break;
		switch( *str) {
			case HOT_GAP   : *str++=0;    break;
			case HOT_BLANK : *str++=' ';  break;
			case HOT_STAR  : *str++='*';  break;
			case HOT_QUES  : *str++='?';  break;
			case HOT_EXCL  : *str++='!';  break;
			case HOT_SEMI  : *str++=';';  break;
			case HOT_PIPE  : *str++='|';  break;
			case HOT_DOLLAR: *str++='$';  break;
			case HOT_IN    : *str++='<';  break;
			case HOT_OUT   : *str++='>';  break;
			case HOT_BSLASH: *str++='\\'; break;
			case HOT_LASTCD: *str++='~';  break;
			case HOT_BACKG : *str++='&';  break;
			case HOT_CURLL : *str++='{';  break;
			case HOT_CURLR : *str++='}';  break;
			case HOT_CURDIR: *str++='.';  break;
			case HOT_PARDIR: *str++='.';  break;
			case HOT_BEGOUT: *str++='$';  break;
			case HOT_ENDOUT: *str++=')';  break;
			case HOT_EQUAL : *str++='=';  break;
			case HOT_APOSTR: *str++='`';  break;
#if 1
			/* AMK --> detect DOS patterns */
/*			case HOT_PM1   : *str++='(';  break;*/
			case HOT_ECKL  : *str++='[';  break;
			case HOT_ECKR  : *str++=']';  break;
			/* <-- AMK */
#endif
			default        : str++;       break;
		}
	}
}

/*
 * process formatted string.  ' ' is the delimeter.
 *
 *    0: check '\0': no more, stop, done.
 *    1: check $.     if so, extract, format, insert
 *    2: check alias. if so, extract, format, insert. goto 1
 *    3: check history or substitution, extract, format, insert. goto 1
 *
 *    4: assume first element now internal or disk based command.
 *
 *    5: extract each ' ' or 0x80 delimited argument and process, placing
 *       in av[] list (except 0x80 args appended).  check in order:
 *
 *             '$'         insert string straight
 *             '>'         setup stdout
 *             '>>'        setup stdout flag for append
 *             '<'         setup stdin
 *             '*' or '?'  do directory search and insert as separate args.
 *
 *             ';' 0 '|'   end of command.  if '|' setup stdout
 *                          -execute command, fix stdin and out (|) sets
 *                           up stdin for next guy.
 */

int  alias_count;
int  has_wild;                 /* set if any arg has wild card */
char *LastCommand;
BPTR redir_out, redir_in;

#define MAXACDEF 32

int
fcomm( UBYTE *str, FRAME **frameptr, char *from )
{
	UBYTE *nextstr, elast;
	char *pend_alias, *command;
	char cout_ispipe=0, cin_ispipe=0, cout_append=0;
	char backg, firstrun, override, block, nobuiltin;
	char *cin_name=NULL, *cout_name=NULL;
	char **oldav=av;
	signed char pendredir;
	int  oldac=ac, oldmax=max_ac, err, ccno, cstat;
	BOOL noclose_redir = FALSE;

	/* AMK: test for pattern matching detection */
	int patterns_num=0,patterns_fail=0;

	max_ac= MAXACDEF;
	av=(char **)falloc( frameptr, max_ac*sizeof(char *));
	ac=0;

	if (++alias_count >= MAXALIAS) {           /* Recursion getting too deep? */
		fprintf(stderr,"Alias Loop\n");
		err = 20;
		goto done1;
	}

nextcommand:
	command   = NULL;
	pend_alias= NULL;
	err       = 0;
	has_wild  = 0;
	firstrun  = 1;
	ccno=cstat= 0;
	elast     = 1;
	block     = 0;
	nobuiltin = 0;
	pendredir = 0;
	redir_in  = redir_out = 0;

	if (*str == 0)
		goto done1;

	if ( *str == HOT_EXCL) {
		UBYTE *p, c;
		char *istr;                       /* fix to allow !cmd1;!cmd2 */
		for(p = str; *p && *p != HOT_SEMI ; ++p);
		c = *p;
		*p = 0;
		if( str[1]==HOT_EXCL ) str[1]='!';
		istr = get_history((char *)str);
		*p = c;
		replace_head(istr);
		str = format_insert_string( frameptr, str, istr );
	}

	/*******************************************************
	 * Part one of the parser:
	 * The argument line is generated as an array of strings
	 */

	nextstr = str;
	ac = 0;
	do {              /* outer loop: each pass typically generates one av[ac] */
		UBYTE *ptr, *arg;
		signed char redir, doexpand, cont, inc;

		av[ac] = NULL;
		cont = 1;
		doexpand = redir = inc = 0;

		while (cont && elast) {       /* inner loop: adds one piece to av[ac] */
			ptr = exarg(&nextstr,&elast);
			inc = 1;
			cont = (elast == 0x80);
			if( (UBYTE)*ptr<0x80 )
				arg=ptr;
			else {
				switch ((UBYTE)*ptr) {           /* arg must be set in every case */
				case HOT_IN:
					redir = -2;
				case HOT_OUT:
					if (cstat & (ST_NORED | ST_COND)) {         /* don't extract   */
						redir = 0;                          /* <> stuff if its */
						arg = ptr;                          /* external cmd.   */
						break;
					}
					++redir;
					arg = ptr + 1;
					if (*arg == HOT_OUT && *ptr == HOT_OUT) {
						redir = 2;        /* append >> */
						++arg;
					}
					else if (*arg == HOT_OUT && *ptr == HOT_IN) {
						/* AMK: fprintf(stderr,"caution: csh does not support \"<>\" redirection!\n"); */
						/* AMK: how about "redir = -4;" ? */
						redir = -4;
						++arg;
					}
					cont = 1;
					break;
				case HOT_DOLLAR:
					/* restore args if from set command or pend_alias */
					if ((arg = get_var(LEVEL_SET, ptr + 1))) {
						UBYTE *pe, sv;
						while (pe = Index(arg,0xA0)) {
							sv = *pe;
							*pe = '\0';
							CHECKAV( 1 );

							if (av[ac]) {
								av[ac] = frealloc( frameptr,av[ac],Strlen(arg));
								Strcat(av[ac++], arg);
							} else
								av[ac++] = push_cpy(frameptr,arg);

							*pe = sv;
							av[ac] = NULL;
							arg = pe+1;
						}
					} else
						arg = ptr;
					break;
				case HOT_LASTCD:
					if ((!ptr[1] || ptr[1]=='/')&&(arg=get_var(LEVEL_SET, v_lcd))) {
						if( ptr[1] ) {
							Strcpy(Buf,arg);
							Strcat(Buf,ptr+1);
							arg=(UBYTE*)Buf;
						}
#if 0
						/* AMK: detecting DOS patterns BEGIN */
						/*else if (ptr[2]==HOT_PM1 || ptr[2]==HOT_PM2)*/
						else if (elast==0x80)
							arg = ptr;
						/* AMK: detecting DOS patterns END */
#endif
					} else
						arg = ptr;
					break;
				case HOT_CURDIR:
					arg=ptr;
					if( !ptr[1] && elast!=0x80 || ptr[1]=='/' ) {
						if( ac==0 ) nobuiltin=1;
						arg= ptr[1]=='/' ? ptr+2 : ptr+1;
					}
					break;
				case HOT_PARDIR:
					arg=ptr;
					if( !ptr[2] && elast!=0x80 || ptr[2]=='/' )
						arg= ptr[2]=='/' ? ptr+2 : (UBYTE*)"/";
					break;

#if 1
				/* AMK --> detect DOS patterns */
				case HOT_PM1:
				case HOT_ECKL:
				case HOT_ECKR:
				/* <-- AMK */
#endif
				case HOT_STAR:
				case HOT_QUES:
					if( !(cstat & ST_NOEXP) && !(pend_alias && *pend_alias=='*'))
						if( ac!=1 || av[1]&&*av[1] || *ptr!=HOT_QUES || ptr[1] )
							doexpand = 1;
					arg = ptr;
					break;
				default:
					arg = ptr;
					break;
				}
			}

			/* Append arg to av[ac] */

			if (av[ac]) {
				av[ac] = frealloc( frameptr,av[ac],Strlen(arg));
				Strcat(av[ac], arg);
			} else {
				av[ac] = push_cpy( frameptr, arg );
			}

			if (elast != 0x80)
				break;
		}

		/* one argument is now complete */

		backg   = *av[ac] && (UBYTE)av[ac][Strlen(av[ac])-1]==HOT_BACKG;
		override= (UBYTE)*av[ac] == HOT_BSLASH;

		if( firstrun && (UBYTE)*av[0] == HOT_CURLL ) {
			char *end;
			av[0]++;
			if( end=index(av[0],HOT_CURLR)) *end=0;
			block=1;
		}

		if( (UBYTE)*av[ac]==HOT_BEGOUT || (UBYTE)*av[ac]==HOT_APOSTR ) {
			BPTR cout, oldcout= Myprocess->pr_COS;
			FILE *in;
			UBYTE *t;
			int  apo;

			apo= (UBYTE)*av[ac]==HOT_APOSTR;
			inc=0;
			if( t=Index(av[ac]+1, apo ? HOT_APOSTR : HOT_ENDOUT ))
				*t=0;

			if(!(cout = Open(tempname(2),MODE_NEWFILE))) {
				err= 20;
				ierror (NULL, 504);
			} else {
				Myprocess->pr_COS = DEVTAB(stdout) = cout;
				execute(av[ac]+2-apo);
				fflush(stdout);
				Close(cout);

				if(!(in=fopen(tempname(2),"r"))) {
					err= 1;
					ierror (NULL, 504);
				} else {
					while( myfgets(Buf,in)) {
						char *str=Buf, *get, quote=0;
						CHECKAV( 1 );
						if( apo ) {
							while( *str==' ' ) str++;
							for( get=str; *get; ) {
								/*
								   AMK:
								   Use 'o_cquote' here?
								*/
								if( *get=='\"' )
									quote=1-quote, get++;
								else if( *get==' ' && !quote ) {
									*get++=0;
									CHECKAV( 1 );
									while( *get==' ' ) get++;
									if( !*get ) break;
									av[ac++]=push_cpy(frameptr,str);
									str=get;
								} else
									get++;
							}
							av[ac++]= push_cpy(frameptr,str);
						} else
							av[ac++]= push_cpy(frameptr,(UBYTE*)Buf);
					}
					fclose(in);
					DeleteFile(tempname(2));
					av[ac]=NULL;
				}
			}

			Myprocess->pr_COS = DEVTAB(stdout) = oldcout;
		}

		backtrans( (UBYTE *)av[ac] );

		if (doexpand) {                                  /* process expansion */
			char **eav, **ebase;
			int eac;
			has_wild = 1;
			eav = ebase = expand(av[ac], &eac);
			inc = 0;

			/* AMK: test for pattern matching detection */
			++patterns_num;

			if (eav) {
				/* FUTURE: check retval */
				CHECKAV( eac );
				for (; eac; --eac, ++eav)
					av[ac++] = push_cpy(frameptr,*eav);
				free_expand (ebase);
			}
			else {
				/* AMK: test for pattern matching detection */
				++patterns_fail;
#if 0
				/* always warn if pattern doesn't match */
#endif
#if 0
				if (o_nomatch) {
					/* only warn, if _nomatch is set */
					fprintf(stderr,"%s: No match.\n",av[0]);
					/*
					   AMK: Abort command execution if pattern
					        does not match.  Don't abort if
					        "_nomatch" is unset.
					*/
					/* AMK: how to abort here? */
					goto done1;
					/* AMK: or should we use done0? */
					/* AMK: pipes are causing problems, when
					        jumping to done0 -- the program
					        after the pipe gets no stdin and
					        hangs (e.g. "echo *.dfsdf | wc")
					        With done1 the complete line is
					        aborted (further commands are ignored).
					        It works -- but differs from Unix-csh
					        behaviour!
					*/
				}
#endif
			}
		} else if( av[ac] && av[ac][0]==')' ) {
			int i;
			UBYTE *pe, sv;
			for( i=ac-1; i>0; i-- )
				if( *av[i]=='@' )
					break;
			if( i>0 && av[i][Strlen(av[i])-1]=='(' ) {
				extern int exec_fn_err;
				char *avi=av[i], *last=av[ac];
				av[i]=v_value; av[ac]=NULL;
				arg=(UBYTE*)exec_function( avi+1, av+i, ac-i );
				av[i]=avi;     av[ac]=last;
				inc=0;
				if( exec_fn_err<0 )
					ac++;
				else if( exec_fn_err>0 || !arg )
					ac=i, av[ac]=NULL;
				else {
					ac=i;
					while (pe = Index(arg,0xA0)) {
						sv = *pe;
						*pe = '\0';
						CHECKAV( 1 );
						av[ac++] = push_cpy(frameptr,arg);
						*pe = sv;
						arg= pe+1;
					}
					av[ac] = falloc(frameptr,Strlen(arg)+Strlen(last+1)+4);
					Strcpy(av[ac],arg);
					Strcat(av[ac++], last+1 );
				}
			}
		}

		/*******************************
		 * special handling of first arg
		 */

		if( firstrun ) {                     /* we just found out the command */
			firstrun=0;
			command=av[0];

			if (command==NULL || *command==0)
				goto done0;

			if( block )
				pend_alias=command;
			else if ( override )
				memmove( command, command+1, Strlen(command));
			else {
				pend_alias=get_var(LEVEL_ALIAS,command);  /* if not \command */
				if( pend_alias && pend_alias==from )
					pend_alias=NULL;
			}

			if( pend_alias )
				ccno= *pend_alias=='%' || *pend_alias=='*' ? 2 : 1;
			else
				ccno= nobuiltin ? 0 : find_command(command);
			cstat=Command[ccno].stat;

			if ( !(cstat & ST_COND) && (disable || forward_goto) ) {
				while (elast && elast != HOT_SEMI && elast != HOT_PIPE)
					exarg(&nextstr,&elast);
				goto done0;
			}
		}

		/********************************
		 * handling of redirection
		 */

		if( pendredir )
			redir=pendredir, pendredir=0;

		if (redir && !err) {                            /* store redirection  */
			char *file = (doexpand) ? av[--ac] : av[ac];

			if( !*file ) {
				pendredir=redir;
			}
			else if (redir < 0) {
				cin_name = file;
				if (redir == -4) {	/* new OS2.0 <> redirection */
					cout_name = cin_name;
				}
			}
			else {
				cout_name = file;
				cout_append = (redir == 2);
			}
			inc = 0;
		}

		if (inc) {                                   /* check elast for space */
			++ac;
			CHECKAV( 1 );                            /* FUTURE: check retval */
		}
	} while( elast==HOT_BLANK );
	av[ac] = NULL;

	/******************************************************************
	 * Part two:
	 * The argument line is processed (pipes, commands, recursive calls
	 */

	/* process pipes via files */

	if (elast == HOT_PIPE && !err) {
		static int which;             /* 0 or 1 in case of multiple pipes */
		which = 1 - which;
		cout_name = tempname( which );
		cout_ispipe = 1;
	}

	if (err)
		goto done0;

	/* AMK: test for pattern matching detection */
	if (!o_nomatch && patterns_num==patterns_fail && patterns_num>0) {
		if (av[0])
			fprintf(stderr,"%s: No match.\n",av[0]);
		else
			fprintf(stderr,"No match.\n");
		goto done1;
	}

	{
		char *avline=NULL;
		char delim = ' ';
		BPTR  oldcin  = Myprocess->pr_CIS, cin=NULL;
		BPTR  oldcout = Myprocess->pr_COS, cout=NULL;
		char *cin_buf=NULL;
		struct FileHandle *ci=NULL;
		long oldbuf=NULL;
		int findcmd_ret;

		if( backg ) {
			char *larg=av[ac-1];
			memmove( av+1, av, ac*sizeof(*av));
			command=av[0]=push_cpy( frameptr, (UBYTE*)o_rback);
			ccno=find_command(command);
			cstat=Command[ccno].stat;
			if( Strlen(larg)>1 )
				larg[Strlen(larg)-1]=0, ac++;
		}

		if( ccno==2 || (cstat & ST_A0))            /* alias with argument */
			delim = 0xA0;

		if( cstat&ST_AVLINE || pend_alias || Verbose&VERBOSE_ALIAS )
			avline=compile_avf(frameptr,av,(pend_alias?1:0), ac, delim,ccno==1);

#if 0
		fprintf(stderr,"av[012]: %s %s %s, avline: %s\n",av[0]?av[0]:"×",av[1]?av[1]:"×",av[2]?av[2]:"×",avline?avline:"(NULL)");
		{
		char *p = find_internal(av[0]);
		int cmdnum = find_command(av[0]);
		printf("find_command(av[0]) : %d (%s)\n",cmdnum,p?p:"(NULL)");
		if (Command[cmdnum].func == do_truerun)
			printf("Achtung, do_truerun() wird ausgeführt!\n");
/*
		if (stricmp(av[0],"run") == 0)
			printf("Achtung, C:RUN wird ausgeführt!\n");
*/
		}
#endif
		if ((findcmd_ret = find_command(av[0])) != 0) {
			if (Command[findcmd_ret].func == do_truerun) {
				/*printf("Achtung, do_truerun() wird ausgeführt!\n");*/
				noclose_redir = TRUE;
			}
		}

		fflush(stdout);
		LastCommand=command;
		if ( !(cstat & (ST_NORED | ST_COND))) { /* redirection not disabled */
			if (cin_name == cout_name) { /* redir == -4, which means <> redirection */
				if (cin_name) {
					filemap(cin_name,0);

					cin = cout = NULL;

					if (cin=Open(cin_name,MODE_OLDFILE)) {
						if (stricmp(cin_name,"NIL:")==0) {
							cout = Open(cin_name,MODE_OLDFILE);
						}
						else if (((struct FileHandle *)BADDR(cin))->fh_Port!=DOSFALSE) {
							struct MsgPort *old;
							old = SetConsoleTask(((struct FileHandle *)BADDR(cin))->fh_Type);
							cout = Open("CONSOLE:",MODE_OLDFILE);
							(void)SetConsoleTask(old);
						}
					}
					if (!cin || !cout) {
						fprintf(stderr,"%s: neither interactive nor NIL:\n",cin_name);
						if (cin)  { Close(cin);  cin  = NULL; }
						if (cout) { Close(cout); cout = NULL; }
						cin_name = NULL;
						cout_name = NULL;
						ierror (NULL, 504);
						err = 20;
					} else {
						redir_in=cin;
						redir_out=cout;

						if( !(cstat&ST_NORED)) {
							if( !noclose_redir ) {
								Myprocess->pr_CIS = DEVTAB(stdin) = cin;
								Myprocess->pr_COS = DEVTAB(stdout) = cout;

								ci = (struct FileHandle *)(cin<<2);
								cin_buf = SAllocMem(202L, MEMF_PUBLIC);
								oldbuf = ci->fh_Buf;
								if (ci->fh_Buf == 0) /* fexec expects a CIS buffer */
									ci->fh_Buf = (long)cin_buf>>2;
							}
						}
					}
				}
			}
			else {
				if (cin_name) {
					filemap(cin_name,0);
					if (!(cin = extOpen(cin_name,MODE_OLDFILE))) {
						ierror (NULL, 504);
						err = 20;
						cin_name = NULL;
					} else {
						redir_in=cin;
						if( !(cstat&ST_NORED)) {
							if( !noclose_redir ) {
								Myprocess->pr_CIS = DEVTAB(stdin) = cin;
								ci = (struct FileHandle *)(cin<<2);
								cin_buf = SAllocMem(202L, MEMF_PUBLIC);
								oldbuf = ci->fh_Buf;
								if (ci->fh_Buf == 0) /* fexec expects a CIS buffer */
									ci->fh_Buf = (long)cin_buf>>2;
							}
						}
					}
				}
				if (cout_name) {
					filemap(cout_name,0);
					if (cout_append && (cout=extOpen(cout_name,MODE_OLDFILE))) {
						Seek(cout, 0L, OFFSET_END );
					} else {
#if 1
						/*
						 *  allow other programs to read contents of
						 *  redirected output, so we try MODE_READWRITE
						 */

						/* first, clear file with MODE_NEWFILE */
						BPTR pre_cout;
						if (pre_cout = Open(cout_name,MODE_NEWFILE))
							Close(pre_cout);

						/* try MODE_READWRITE */

						if (cout = extOpen(cout_name,MODE_READWRITE)) {
							/*
							 *  Just to be sure, clear it again.
							 *  It's possible that MODE_NEWFILE failed
							 *  but MODE_READWRITE succeeded. In this
							 *  case the file still needs to be cleared.
							 */
							SetFileSize (cout, 0L, OFFSET_BEGINNING);
						}
						else {
							/*
							 *  MODE_READWRITE is not understood by all
							 *  handlers, eg NULL:, so additionally we
							 *  try it the old way with MODE_NEWFILE.
							 */
							cout = extOpen(cout_name,MODE_NEWFILE);
						}
#else
						/*
						 *  allow other programs to read contents of
						 *  redirected output, so we use MODE_READWRITE
						 *  and SetFileSize(0) instead of just MODE_NEWFILE
						 */
						if (cout = extOpen(cout_name,MODE_READWRITE)) {
							if (SetFileSize (cout, 0L, OFFSET_BEGINNING) == -1L) {
								extClose(cout);
								cout = NULL;
								/*
								err = 20;
								ierror (NULL, 504);
								*/
							}
						}
						else {
							/*
							 *  MODE_READWRITE is not understood by all
							 *  handlers, eg NULL:, so additionally we
							 *  try it the old way with MODE_NEWFILE.
							 */
							cout = extOpen(cout_name,MODE_NEWFILE);
						}
#endif
					}
					if (cout == NULL) {
						err = 20;
						ierror (NULL, 504);
						cout_name = NULL;
						cout_append = 0;
					} else {
						redir_out=cout;
						if( !(cstat&ST_STORED))
							if( !noclose_redir )
								Myprocess->pr_COS = DEVTAB(stdout) = cout;
					}
				}
			}
		}

		if( Verbose&VERBOSE_ALIAS ) {
			if( Verbose&VERBOSE_HILITE )
				fprintf(stderr,"%s",o_hilite), fflush(stderr);
			if( pend_alias )
				fprintf(stderr,"%-*s%s %s\n",alias_count,">",av[0],avline);
			else
				fprintf(stderr,"%-*s%s\n",alias_count,">",avline);
			if( Verbose&VERBOSE_HILITE )
				fprintf(stderr,"%s",o_lolite), fflush(stderr);
		}

		if( pend_alias && !backg ) {	/* don't expand alias if running in background */
			UBYTE *ptr, *scr;
			FRAME *subframe=NULL;

			if( ccno==2 ) {                         /* has arguments */
				char *val=avline;
				int clen;

				ptr=pend_alias;
				clen=Strlen(ptr)*2+20;
				newframe( &subframe, clen+DEFAULTFRAME );
				push_locals( (ROOT *)falloc( &subframe, sizeof(ROOT) ));
				do {                                       /* set all args    */
					char *varname, *gap=NULL;
					for( varname= ++ptr; *ptr && *ptr!=' ' && *ptr!='%'; ++ptr);
					if( *ptr=='%' && (gap=index(val,0xA0 )) ) *gap=0;
					set_var( LEVEL_LOCAL, varname, val );
					val= gap ? gap+1 : "";
				} while( *ptr=='%' );
				scr = falloc(&subframe,clen);
			} else {
				int  clen=Strlen(pend_alias)+Strlen(avline)+20;

				newframe( &subframe,3*clen+DEFAULTFRAME );
				push_locals( (ROOT *)falloc( &subframe, sizeof(ROOT) ));
				ptr = falloc( &subframe,clen );
				sprintf(ptr,"%s %s",pend_alias,avline);
				scr = falloc( &subframe, 2*clen );
			}
			preformat( ptr,scr );
			err=fcomm (scr,&subframe,pend_alias );
			pop_locals();
			deleteframe(subframe);
		} else {
			if (ac < Command[ccno].minargs + 1) {
				show_usage( NULL );
				err = 20;
			} else if (!err) {
				int (*func)(char*,int)=Command[ccno].func;
				get_opt( av, &ac, ccno, &err );
				if( err || ccno>0 && ac==2 && !strcmp(av[1],"?") )
					show_usage(av[0]);
				else
					err = (*func)(avline, Command[ccno].val); /*  do the call */
			}
		}

		if (err < 0)
			err = 20;
		if (E_stack == 0 )
			seterr(err);
		fflush(stderr);

		if (!(cstat & (ST_NORED | ST_COND))) {
			if (cin_name && !(cstat & ST_STORED)) {

				if( !noclose_redir ) {
					ci->fh_Buf = oldbuf;
				}

				fflush(stdin);
				clearerr(stdin);
				RESETIO( stdin );

				if( !noclose_redir ) {
					extClose(cin);
					FreeMem(cin_buf, 202L);
				}
			}
			if (cout_name && !(cstat & ST_STORED)) {
				fflush(stdout);
				clearerr(stdout);
#ifdef AZTEC_C
				stdout->_flags &= ~_IODIRTY;    /* because of nil: device */
#endif
				if( !noclose_redir ) {
					extClose(cout);
				}

				cout_append = 0;
			}
		}
		Myprocess->pr_CIS = DEVTAB(stdin)  = oldcin;
		Myprocess->pr_COS = DEVTAB(stdout) = oldcout;

		if (cin_ispipe && cin_name)
			DeleteFile(cin_name);
		if (cout_ispipe) {
			cin_name = cout_name;         /* ok to assign.. static name */
			cin_ispipe = 1;
		} else {
			cin_name = NULL;
		}
		cout_name = NULL;
		cout_ispipe = 0;
	}

done0:
	{
		char *exc;
		if (err && E_stack==0) {
			exc = get_var(LEVEL_SET, v_except);
			if (err >= ((exc)?atoi(exc):1)) {
				if (exc) {
					++H_stack, ++E_stack;
					a0tospace(exc);
					exec_command(exc);
					--E_stack, --H_stack;
				} else {
					Exec_abortline = 1;
				}
			}
			seterr(err);
		}
		if (elast != 0 && Exec_abortline == 0) {
			memmove( str, nextstr, Strlen(nextstr)+1 );
			goto nextcommand;
		}
		Exec_abortline = 0;
		if (cin_name)
			DeleteFile(cin_name);
	}
done1:

	av=oldav; ac=oldac; max_ac=oldmax;
	--alias_count;

	return err;                      /* TRUE = error occured    */
}


static UBYTE *
exarg(UBYTE **ptr, UBYTE *elast)
{
	UBYTE *end, *start;

	start = end = *ptr;
	while ( *(signed char *)end>0 || *end && *end != 0x80 &&
	         *end != HOT_SEMI &&  *end != HOT_PIPE && *end != HOT_BLANK)
		++end;
	*elast = *end;
	*end = '\0';
	*ptr = end + 1;
	return start;
}


static void
newframe(FRAME **frameptr, int bytes)
{
	FRAME *new;

	new=salloc( bytes + 4 + sizeof(FRAME) );
	new->next= *frameptr;
	new->bytesleft = bytes;
	new->ptr       = new->mem;
	*frameptr=new;
}

static void *
falloc( FRAME **frameptr, int bytes )
{
	FRAME *frame= *frameptr;
	char *mem;

	bytes+=2;                                    /* 2 extra bytes for do_run  */
	bytes|=3;
	bytes++; 

	if( frame->bytesleft <= bytes )
		newframe( frameptr, bytes+DEFAULTFRAME ), frame = *frameptr;

	mem=frame->ptr;
	frame->bytesleft-=bytes;
	frame->ptr      +=bytes;
	return mem;
}

static void *
frealloc( FRAME **frameptr, char *oldstring, int morebytes )
{
	char *mem=oldstring;

	morebytes|=3;
	morebytes++; 

	if( (*frameptr)->bytesleft <= morebytes ) {
		int oldlen=(*frameptr)->ptr-oldstring;
		newframe( frameptr, oldlen+morebytes+DEFAULTFRAME );
		mem= (*frameptr)->ptr;
		strcpy((*frameptr)->ptr,oldstring);
		oldlen+=morebytes;
		(*frameptr)->ptr      +=oldlen;
		(*frameptr)->bytesleft-=oldlen;
	}
	(*frameptr)->bytesleft-=morebytes;
	(*frameptr)->ptr      +=morebytes;
	return mem;
}

static void
funalloc( FRAME **frameptr, int bytes )
{
	bytes&=~3;
	(*frameptr)->bytesleft+=bytes;
	(*frameptr)->ptr      -=bytes;
}



static void
deleteframe(FRAME *frame)
{
	FRAME *nxt;

	for( ; frame; frame=nxt ) {
		nxt=frame->next;
		free(frame);
	}
}

/*
 * Insert 'from' string in front of 'str' while deleting the
 * first entry in 'str'.  if freeok is set, then 'str' will be
 * free'd
 */

static UBYTE *
format_insert_string(FRAME **frameptr, UBYTE *str, char *from)
{
	UBYTE *new, *strskip;
	int len;

	strskip=skipword( str );
	len = Strlen(from)*3+20;
	new = falloc( frameptr, len);
	preformat(from, new);
	funalloc( frameptr, len -Strlen(new)  - 2 );
	frealloc( frameptr, (char*)new, Strlen(strskip)+2);
	strcat((char*)new, (char*)strskip);
	new[Strlen(new)+1] = 0;
	return new;
}

static UBYTE *
skipword( UBYTE *strskip )
{
	for ( ; *(signed char *)strskip>0
		|| *strskip && *strskip != HOT_BLANK 
		&& *strskip != HOT_SEMI && *strskip != HOT_PIPE
		&& *strskip != 0x80; ++strskip);
	return strskip;
}

char *
find_internal( char *str )
{
	return Command[find_command(str)].name;
}

static int
find_command( char *str )
{
	int i, len;
	struct COMMAND *com;
	char c;

	if (str != NULL) {	/* can happen when you call "<sys:" */

		len = Strlen(str);
		c   = *str;

		if( TRUE /*o_abbrev!=2*/ )
			for( com=Command, i=0; com->func; com++, i++ )
				if ( c == *com->name && !strncmp(str, com->name, len))
					if(o_abbrev&1 || len==Strlen(com->name))
						return i;
		if( !stricmp(FilePart(str),"Execute") || !stricmp(FilePart(str),"Run"))
			return COM_EXEC;
	}

	return 0;
}

/* for usage in other modules, i.e. in comm3.c/do_truerun() */
int find_command2( char *str )
{
	return(find_command(str));
}

int exec_fn_err;

extern struct FUNCTION {
	short id, minargs, maxargs;
	char *name;
} Function[];


char *gotfunc( int i, char **fav, int fac );

char *
exec_function( char *str, char **fav, int fac)
{
	int len=Strlen(str)-1, i;

	exec_fn_err=0;
	for (i = 0; Command[i].func; ++i)
		if ( Command[i].stat&ST_FUNC && !strncmp(str,Command[i].name,len)) {
			if( fac<Command[i].minargs ) {
				exec_fn_err=20;
				return NULL;
			} else {
				int (*func)( void )=Command[i].func;
				char **oldav=av;
				int  oldac=ac;
				av=fav-1, ac=fac+1;
				exec_fn_err=(*func)();
				av=oldav, ac=oldac;
				return get_var( LEVEL_SET, fav[0] );
			}
		}
	for (i = 0; Function[i].id; ++i)
		if ( len==Strlen(Function[i].name)&&!strncmp(str,Function[i].name,len))
			return gotfunc( i,fav,fac );

	exec_fn_err = -1;
	return NULL;
}

int
echofunc(void)
{
	int  i;
	char *str;

	if( !strlen(av[0]) ) return -1;
	exec_fn_err=0;
	for (i = 0; Function[i].id; ++i)
		if ( !strcmp(av[0],Function[i].name)) {
			if(str=gotfunc( i,av,ac ))
				printf("%s\n",str);
			return exec_fn_err;
		}
	return -1;
}


char *
gotfunc( int i, char **fav, int fac )
{
	fac--; fav++;
	if( fac<Function[i].minargs ) {
		fprintf( stderr, "Not enough arguments for @%s\n",
		                  Function[i].name );
		exec_fn_err=20;
		return NULL;
	} else if( fac>Function[i].maxargs ) {
		if( ac > Function[i].maxargs )
			fprintf( stderr, "Too many arguments for @%s\n",
			                  Function[i].name );
		exec_fn_err=20;
		return NULL;
	} else {
		exec_fn_err=dofunc( Function[i].id, fav, fac);
		return exec_fn_err ? (char *)NULL : (char *)get_var(LEVEL_SET, v_value);
	}
	return NULL;
}



do_help()
{
	struct COMMAND *com;
	int i=0;

	for (com = &Command[FIRSTCOMMAND]; com->func; ++com) {
		printf ("%-12s", com->name);
		if (++i % 6 == 0) printf("\n");
	}

	if (i % 6 != 0) printf("\n");

	if (options&1) {
		int num;
		printf("\n");
		for (i=0,num=0; Function[num].id; ++num) {
			printf("@%-11s",Function[num].name);
			if (++i % 6 == 0) printf("\n");
		}
		if (i % 6 != 0) printf("\n");
	}

	printf("\nUse   man <command>   for more information\n");
	return 0;
}

do_nothing()
{
	return 0;
}

static char *
push_cpy(FRAME **frameptr, void *s)
{
	return strcpy(falloc(frameptr,Strlen(s)+1), s);
}

void
exec_every(void)
{
	char *str = o_every;

	if (str) {
		++H_stack, ++E_stack;
		a0tospace( str );
		exec_command(str);
		--E_stack, --H_stack;
	}
}

char *
a0tospace( char *str )
{
	char *get=str, *put=str;

	while( *get )
		if( (UBYTE)*get==0xA0 )
			*put++=' ', get++;
		else 
			put++,get++;
	return str;
}

void
show_usage( char *str )
{
	int ccno, first=0, err=0;
	char *get, *put, buf[300];

	if( !str )
		str=LastCommand, err=1;
	for( put=str; *put && (*put&127)!=32; put++ ) ;
	*put=0;

	put=buf;
	ccno = find_command (str);
	if( get= Command[ccno].usage ) {
		do {
			put+=sprintf(put, first++?"       %s ":"Usage: %s ",
			             Command[ccno].name );
			if( *get=='-' ) {
				*put++='['; *put++='-';
				get++;
				while( *get && *get!=' ' && *get!=',' )
					*put++ = *get++;
				*put++=']';
			}
			while( *get && *get!=','  )
				*put++ = *get++;
			*put++='\n';
		} while( *get++ );
		*put=0;
		fprintf( err ? stderr : stdout, "%s", buf );
	}
}

do_exec( char *str )
{
	if( !options )
		return execute( next_word( str ) );
	execute( next_word(next_word( str )));
	return 0;
}

static int
checkav( FRAME **frameptr, int n )
{
	char **tmp;
	int newac;

	if( ac+n+10>=max_ac ) {
		newac=max_ac+n+40;
		tmp=(char **)falloc(frameptr,newac*sizeof(char *));
		memcpy(tmp,av,max_ac*sizeof(char *));
		av=tmp; max_ac=newac;
	}
	return 0;
}

/*	Parse the options specified in sw[]
	Setting a bit in global variable options
	for each one found
*/

int
get_opt( char **av, int *ac, int ccno, int *err )
{
	static char opts[2];
	char **get=av+1,**put=av+1, *c, *s, *str;
	int  i=1, l, usage=0, nac;
	long oldopts, origopts=options;

	*err=0;
	options=0;
	if( !ccno )
		return 0;
	if( ccno>0 ) {
		if ((str=Command[ccno].options)==NULL)
			return 0;   /* command has no options */
	}
	else
		str=opts, opts[0] = -ccno;

	for( ; i<*ac && *av[i]=='-'; i++, get++ ) {
		if( !*(c = *get+1))
			goto stop;
		if(  *c=='-' && !c[1] ) {
			i++, get++;
			goto stop;
		}
		oldopts=options;
		for ( ; *c ; c++) {
			if( (*c<'a' || *c>'z') && (*c<'A' || *c>'Z') )
				{ options=oldopts; goto stop; }
			for( l=0, s=str; *s && *s != *c; ++s )
				++l;
			if ( *s )
				options |= (1 << l);
			else if( !usage ) {
				usage=1;
				if( ccno>0 )
					*err=20;
			}
		}
	}
stop:
	if( ccno>0 ) {
		for( nac=1; i<*ac; i++ )
			*put++ = *get++, nac++;
		*put=NULL;
		*ac=nac;
	} else {
		i=options;
		options=origopts;
		return i;
	}
	return 0;
}

#if 0
USHORT Options[160];

int
do_options()
{
	for( i=1; i<ac; i+=2 ) {
		if( ac-i<=1 )
			{ ierror( av[i], 500 ); return 20; }
		if( *av[i+1]!='-' )
			{ ierror( av[i+1], 500 ); return 20; }
		options=0;
		parseopts( av[i+1]+1 );
	}
}
#endif

extern char *MyMem;
static char Pipe[3][32];

static char *
tempname( int which )
{
	sprintf(Pipe[which],"%spipe%c%d_%lx",o_pipe,'A'+which,alias_count,MyMem);
	return Pipe[which];
}

int
hasspace( char *s )
{
	if( !*s )
		return 1;
	for ( ; *s; s++)
		if (ISSPACE(*s) || *s==';') return 1;
	return 0;
}

static char *
compile_avf(FRAME **framep,char **av, int start, int end, char delim, int quote)
{
	char *cstr, *p, *s;
	int len, i;

	len = 3;
	for (i = start; i < end; ++i) len += Strlen(av[i]) + 3;
	if( framep ) {
		p = cstr = falloc(framep,len);
	} else 
		p = cstr = salloc(len);
	*cstr = '\0';

	for (i = start; i < end; ++i) {
		if (quote && hasspace(av[i]))
			p += sprintf(p, "\"%s\"", av[i]);
		else {
			for( s=av[i]; *p++ = *s++; ) ;
			p--;
		}
		if (i+1 < end) *p++=delim;
	}
	*p='\0';

	return cstr;
}

char *
compile_av(char **av, int start, int end, char delim, int quote)
{
	return compile_avf(NULL,av, start, end, delim, quote);
}

int
myfgets( char *buf, FILE *in )
{
	int l;
	char *ret;
	if( ret=fgets(buf,253,in) ) {
		l=Strlen(buf);
		if( buf[l-1]=='\n' )
			buf[l-1]=0;
	}
	return ret!=NULL && !dobreak();
}



/* AMK: last char of a string */
char lastch(char *str)
{
	int len;
	if (str) {
		if (len=strlen(str))
			return(str[len-1]);
		else
			return('\0');
	}
	else
		return('\0');
}



/* AMK: replaced ARP's BtoCStr() and CtoBStr() with own routines */

ULONG BtoCStr(char *cstr, BSTR bstr, LONG maxlen)
{
	char *astr;
	int i;
	astr = (char *)(bstr << 2);

	for ( i=1; i <= astr[0] && i <= maxlen; i++)
		cstr[i-1] = astr[i];
	cstr[i-1] = '\0';
	return((ULONG)(i-1));
}

ULONG CtoBStr(char *cstr, BSTR bstr, LONG maxlen)
{
	char *astr;
	int i,l;
	astr = (char *)(bstr << 2);

	for ( i=0,l=strlen(cstr); i < l && i < maxlen && i < 255; i++)
		astr[i+1] = cstr[i];
	astr[0] = i;
	return((ULONG)(i));
}

