/*
 * SHELL.H
 *
 * (c)1986 Matthew Dillon     9 October 1986
 *
 *
 * SHELL include file.. contains shell parameters and extern's
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 16-Mar-91
 *
 */

#define RAW_CONSOLE 1   /* Set to 0 to compile out command line editing */

/* uncommented by AMK for SAS/C 6.1 */
/* #define strlen strlen */

#ifdef _DCC
# define C_Args
# define FUNCARG(x,y,z) ()
#else
# define FUNCARG(x,y,z) (x,y,z)
# define KICK20
#endif


/* AMK: more includes for OS 2.0 */

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <intuition/intuitionbase.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dos/datetime.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <utility/tagitem.h>
#include <resources/battclock.h>


/*
 *  Magnus Lilja, lilja@lysator.liu.se, IRC: lilja - for multiuser-support
 *  out-comment the following line if you don't have MultiUser installed
 */

#define MULTIUSER_SUPPORT 1

#ifdef MULTIUSER_SUPPORT
# include <libraries/multiuser.h>
# include <proto/multiuser.h>
# include <pragmas/multiuser.h>
  extern struct muBase *muBase;
#endif



LONG AllocPattern    (STRPTR, ULONG);
LONG MatchThePattern (LONG, STRPTR);
void FreePattern     (LONG);



typedef struct FileInfoBlock FIB;

#ifdef AZTEC_C
# include <functions.h>
# define DEVTAB(x)    (BPTR)(_devtab[x->_unit].fd)
# define CHARSWAIT(x) (x->_bp < x->_bend)
# define RESETIO(x)   (x->_bp = x->_bend)
# define COMPILER "Aztec C 5.0d"
# pragma amicall(PatternBase, 0x48, AllocPattern(a0,d0))
# pragma amicall(PatternBase, 0x60, MatchThePattern(d0,a0))
# pragma amicall(PatternBase, 0x66, FreePattern(d0))
#endif

#ifdef LATTICE
# include <proto/all.h>
# include <ios1.h>
/* # define DEVTAB(x)    _ufbs[(x)->_file].ufbfh*/
# define DEVTAB(x)    __ufbs[(x)->_file].ufbfh
# define CHARSWAIT(x) (x->_rcnt != x->_wcnt)
# define RESETIO(x)   (x->_rcnt =  x->_wcnt)

  /* AMK: already defined in <ios1.h>, extern struct UFB _ufbs[];*/
# ifndef memmove
# define memmove(t,f,l) movmem((f),(t),(l))
# endif
# define COMPILER "SAS/C 6"
# define swapmem(x,y,z) swmem(x,y,z)
# pragma libcall PatternBase AllocPattern 48 0802
# pragma libcall PatternBase MatchThePattern 60 8002
# pragma libcall PatternBase FreePattern 66 001
#endif

#ifdef _DCC
# define COMPILER "DICE 2.06"
# define DEVTAB(x)    (_IoFD[x->sd_Fd].fd_Fh)
# define CHARSWAIT(x) (x->sd_RPtr != x->sd_RBuf)
# define RESETIO(x)   (x->sd_RPtr =  x->sd_RBuf)
# define index strchr
# define rindex strrchr
  void * AllocMem(long byteSize, long requirements);
  struct MsgPort *FindPort( UBYTE *name );
#endif

#undef  toupper
#define toupper(c)    ((c)>='a'&&(c)<='z'?((c)-('a'-'A')):(c))

#ifndef MAX
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#endif

#define OPT(x) (options & (x))

#define MAXSRC		5		/* Max. # of source file levels	*/
#define MAXIF		20		/* Max. # of if levels			*/
#define MAXALIAS	20		/* Max. # of alias levels		*/
#define MAXMYFILES	9		/* Max. # of internal files		*/
#define MAXHASH		32		/* Max. # of hash chains		*/
#define MAXFILENAME	30

#define LEVEL_SET		0		/* which variable list to use   */
#define LEVEL_ALIAS		1
#define LEVEL_LABEL		2
#define LEVEL_LOCAL		256

#define SBYTE signed char
#define MAXMENUS	31	/* GadTools maximum */
#define MAXMENUITEMS	63	/* GadTools maximum */

#define VERBOSE_SOURCE 1
#define VERBOSE_ALIAS  2
#define VERBOSE_HILITE 4

#define SCAN_FILE     1
#define SCAN_DIR      2
#define SCAN_RECURSE  4
#define SCAN_DIRENTRY 8
#define SCAN_DIREND   16

#ifndef NULL
#define NULL 0L
#endif

#define CHECKBREAK() dobreak()
#define ISSPACE(c) ((c)==' ' || (c)==9 || (UBYTE)(c)==0xA0)

#ifndef AZTEC_C
struct _dev {
	long  fd;
	short mode;
	};
#endif

typedef struct hnode {
	struct hnode *next, *prev;		/* doubly linked list */
	char *line;				/* line in history    */
} HIST;

typedef struct PErr {
	int errnum;				/* Format of global error lookup */
	char *errstr;
} PERROR;

#ifdef MA_AMK
#undef MA_AMK
#endif
/* #define MA_AMK 1 /* */
typedef struct dirptr {				/* Format of directory fetch pointer */
	BPTR lock;				/* lock on directory   */
	FIB *fib;				/* mod'd fib for entry */
#ifdef MA_AMK
	struct DevProc *dvp;			/* for multi-assigns */
	char *dname;				/* for multi-assigns */
#endif
} DPTR;

extern HIST *H_head, *H_tail;
extern PERROR Perror[];
extern char **av;
extern char *Current;
extern int  H_len, H_tail_base, H_stack, H_num;
extern int  E_stack;
extern int  Src_stack, If_stack, forward_goto;
extern int  ac;
extern int  max_ac;
extern int  debug, Rval, Verbose, disable, Quit;
extern int  Lastresult, atoierr;
extern int  Exec_abortline;
extern int  S_histlen;
extern unsigned int options;

extern FILE *Src_base[MAXSRC];
extern long  Src_pos[MAXSRC];
extern short Src_if[MAXSRC], Src_abort[MAXSRC];
extern char  If_base[MAXIF];
extern struct Process *Myprocess;		/* The shell's process  */
extern struct CommandLineInterface *Mycli;	/* Cli() */
extern struct DosPacket *Mypacket;		/* initialization packet from AmigaDOS */

extern char v_titlebar[], v_prompt[], v_hist[], v_histnum[], v_debug[],
	v_verbose[], v_stat[], v_lasterr[], v_cwd[], v_except[], v_passed[],
	v_path[], v_gotofwd[], v_linenum[], v_every[], v_lcd[], v_rxpath[],
	v_hilite[], v_scroll[], v_minrows[], v_result[], v_qcd[], v_noreq[],
	v_value[], v_nobreak[], v_bground[], v_pipe[], v_datefmt[], v_ioerr[],
	v_abbrev[], v_rback[], v_insert[], v_failat[], v_clipri[],
	v_dirformat[], v_nomatch[], v_prghash[], v_cquote[], v_mappath[],
	v_timeout[], v_promptdep[], v_complete[], v_warn[];

extern char o_hilite[], o_lolite[], *o_rback, *o_csh_qcd, o_internal;
extern char o_aux, o_minrows, o_scroll, o_nowindow, o_noraw, o_vt100;
extern char o_nofastscr, o_nobreak, o_bground, o_resident, o_nomatch;
extern char o_cquote, o_mappath, *o_complete;
extern char o_pipe[], o_datefmt, o_abbrev, o_insert, *o_every;
extern long o_noreq, o_failat, o_timeout, o_promptdep, o_warn;
extern char Buf[], isalph[], confirmed, asked, *classfile;

extern char *MyMem;

#define isalphanum(x) isalph[x]

typedef struct file_info {
	LONG flags;
	LONG type;
	LONG size;
	LONG blocks;
	char class[12];
	struct DateStamp date;
	UWORD uid;
	UWORD gid;
} FILEINFO;

typedef struct pattern {
	int  casedep;
	int  queryflag;
	LONG patptr;
	char pattern[1];
} PATTERN;

#define INFO_COMMENT (1<<30)
#define INFO_INFO    (1<<29)

typedef struct Class {
	struct Class *next;
	char name[1];
} CLASS;

extern CLASS *CRoot, *LastCRoot;
extern struct Window *Mywindow;

extern long IoError;

typedef struct VNode {
	struct VNode *next;
	long len;
	char *text;
	char name[1];
} NODE;

typedef struct VRoot {
	NODE         *first[MAXHASH];
	struct VRoot *parent;
} ROOT;

struct PathList {
	BPTR pl_NextPath;
	BPTR pl_PathLock;
};

/* internal programm path hash-list */
extern char **prghash_list;
extern long prghash_num;

#include "proto.h"
#include "WindowBounds.h"

/* DEBUG */
#include <mindebug.h>

