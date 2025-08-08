/*
 * GLOBALS.C
 *
 * (c)1986 Matthew Dillon     9 October 1986
 *
 *    Most global variables.
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L and 5.50 by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 * Version 5.60M+ by amigazen project 2025-08-07
 *
 */

#include "shell.h"

char v_titlebar	[]="_titlebar";	/* Window title				*/
char v_prompt	[]="_prompt";	/* your prompt (ascii command)		*/
char v_hist	[]="_history";	/* set history depth (value)		*/
char v_histnum	[]="_histnum";	/* set history numbering var		*/
char v_debug	[]="_debug";	/* set debug mode			*/
char v_verbose	[]="_verbose";	/* set verbose for source files		*/
char v_stat	[]="_maxerr";	/* worst return value to date		*/
char v_lasterr	[]="_lasterr";	/* return value from last comm.		*/
char v_cwd	[]="_cwd";	/* current directory			*/
char v_except	[]="_except";	/* "nnn;command"			*/
char v_every	[]="_every";	/* executed before prompt		*/
char v_passed	[]="_passed";	/* passed arguments to source file	*/
char v_path	[]="_path";	/* search path for external commands	*/
char v_gotofwd	[]="_gtf";	/* set name for fwd goto name		*/
char v_linenum	[]="_linenum";	/* name for forline line #		*/
char v_lcd	[]="_lcd";	/* last current directory		*/
char v_rxpath	[]="_rxpath";	/* path for .rexx commands		*/
char v_hilite	[]="_hilite";	/* hilighting escape sequence		*/
char v_scroll	[]="_scroll";	/* scroll jump in fast mode		*/
char v_minrows	[]="_minrows";	/* minimum # of rows for fast scroll	*/
char v_result	[]="_result";	/* result from rxsend			*/
char v_qcd	[]="_qcd";	/* file name for csh-qcd		*/
char v_noreq	[]="_noreq";	/* turn off system requesters		*/
char v_value	[]="_value";	/* return value of a function		*/
char v_nobreak	[]="_nobreak";	/* disabling of ^C			*/
char v_bground	[]="_bground";	/* started in background		*/
char v_pipe	[]="_pipe";	/* path for pipes			*/
char v_datefmt	[]="_datefmt";	/* format of dates			*/
char v_ioerr	[]="_ioerr";	/* last secondary result		*/
char v_abbrev	[]="_abbrev";	/* disable command abbreviations	*/
char v_rback	[]="_rback";	/* command to be used for '&'		*/
char v_insert	[]="_insert";	/* insert mode				*/
char v_failat	[]="_failat";	/* batch file fail level		*/
char v_warn	[]="_warn";	/* execute warn level			*/
char v_clipri	[]="_clipri";	/* command line priority		*/
char v_dirformat[]="_dirformat";/* format string for "dir" command	*/
char v_nomatch	[]="_nomatch";	/* abort if pattern doesn't match	*/
char v_prghash	[]="_prghash";	/* file for program hash table		*/
char v_cquote	[]="_cquote";	/* commodore-shell compatible quotes	*/
char v_mappath	[]="_mappath";	/* map unix pathes /usr/... to usr:...	*/
char v_timeout	[]="_timeout";	/* timeout for window bounds report	*/
char v_promptdep[]="_promptdep";/* maximum number of dirs in %p/prompt	*/
char v_complete	[]="_complete";	/* files that match on file completion	*/

HIST *H_head, *H_tail;	/* HISTORY lists */

PERROR Perror[]= {	/* error code->string */
	103,	"not enough memory available",
	105,	"process table full",
	114,	"bad template",
	115,	"bad number",
	116,	"required argument missing",
	117,	"value after keyword missing",
	118,	"wrong number of arguments",
	119,	"unmatched quotes",
	120,	"argument line invalid or too long",
	121,	"file is not executable",
	122,	"invalid resident library",
/* ! */	201,	"No default directory",
	202,	"object is in use",
	203,	"object already exists",
	204,	"directory not found",
	205,	"object not found",
	206,	"invalid window description",
	207,	"object too large",
	209,	"packet request type unknown",
	210,	"object name invalid",
	211,	"invalid object lock",
	212,	"object is not of required type",
	213,	"disk not validated",
	214,	"disk is write-protected",
	215,	"rename across devices attempted",
	216,	"directory not empty",
	217,	"too many levels",
	218,	"device (or volume) is not mounted",
	219,	"seek failure",
	220,	"comment is too long",
	221,	"disk is full",
	222,	"object is protected from deletion",
	223,	"file is write protected",
	224,	"file is read protected",
	225,	"not a valid DOS disk",
	226,	"no disk in drive",
	232,	"no more entries in directory",
	233,	"object is soft link",
	234,	"object is linked",
	235,	"bad loadfile hunk",
	236,	"function not implemented",
	240,	"record not locked",
	241,	"record lock collision",
	242,	"record lock timeout",
	243,	"record unlock error",

 /* custom error messages */

	500,	"Bad arguments",
	501,	"Label not found",
	502,	"Must be within source file",
	503,	"Syntax error",
	504,	"Redirection error",
	505,	"Pipe error",
	506,	"Too many arguments",
	507,	"Destination not a directory",
	508,	"Cannot mv a filesystem",
	509,	"Error in command name",
	510,	"Bad drive name",
	511,	"Illegal number",
	512,	"Out of memory",
	513,	"User interrupt",
	0,	NULL
};

char **av;					/* Internal argument list				*/
FILE *Src_base[MAXSRC];		/* file pointers for source files		*/
long  Src_pos[MAXSRC];		/* seek position storage for same		*/
short Src_if[MAXSRC];		/* the if level at batch file start		*/
short Src_abort[MAXSRC];	/* abort this batch file				*/
char If_base[MAXIF];		/* If/Else stack for conditionals		*/
int H_len, H_tail_base;		/* History associated stuff				*/
int H_stack, H_num;			/* AddHistory disable stack				*/
int E_stack;				/* Exception disable stack				*/
int Src_stack, If_stack;	/* Stack Indexes						*/
int forward_goto;			/* Flag for searching for foward lables	*/
int ac;						/* Internal argc						*/
int max_ac=256;				/* Maximum # of args (increasable)		*/
int debug;					/* Debug mode							*/
int disable;				/* Disable com. execution (conditionals)*/
int Verbose;				/* Verbose mode for source files		*/
int Lastresult;				/* Last return code						*/
int Exec_abortline;			/* flag to abort rest of line			*/
int Quit;					/* Quit flag							*/
char stdin_redir;			/* stdin is redirected					*/
char stdout_redir;			/* stdout is redirected					*/
char *MyMem;
struct Process *Myprocess = NULL;	/* CSH's process */
struct Window *Mywindow = NULL;		/* CSH's window, may be NULL! */
struct CommandLineInterface *Mycli;
struct DosPacket *Mypacket;
int S_histlen = 20;			/* Max # history entries				*/

unsigned int options;
char Buf[280], confirmed, asked;
CLASS *CRoot, *LastCRoot;

long IoError;

/* internal programm path hash-list */
char **prghash_list = NULL;
long prghash_num = 0L;

