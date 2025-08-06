/*
 * MAIN.C
 *
 * Matthew Dillon, 24 Feb 1986
 * (c)1986 Matthew Dillon     9 October 1986
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by U. Dominik Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 *
 */

#include "shell.h"


#define CSH_VER "5"
#define CSH_REV "50"


static struct Window *getwindow(void);
static void exectimer(int stop);
static void set_kickversion(void);
char shellcompiled[]="Compiled: "__DATE__" "__TIME__" with "COMPILER"\n";

#if 1
char shellname[]    ="Csh "CSH_VER"."CSH_REV" (public release)";
#else
char shellname[]    ="Csh "CSH_VER"."CSH_REV" (BETA)";
#endif

char shellversion[] =CSH_VER""CSH_REV;
char shellvers[]    =CSH_VER"."CSH_REV;
char shellv   []    ="\0$VER: csh "CSH_VER"."CSH_REV" "__AMIGADATE__"";
char shellctr []    ="CshCounter";
char shellres []    ="CshResident";
char shellthere[]   ="CshLoggedIn";

char *oldtitle = NULL;
char trueprompt[100];
char Inline[260];
extern struct ExecBase *SysBase;	/* standard fare....*/
extern struct DosLibrary *DOSBase;	/* more standard fare.... */
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *GadToolsBase;
struct Library *AslBase;		/* AMK: Asl-FileRequester replaces ARP */
struct Library *BattClockBase;

struct Window *old_WindowPtr = NULL;
int    oldtaskpri = -999;
BPTR   OldCin;
void   *PatternBase;
BOOL   nologout = FALSE;
BOOL   nowintitle = FALSE;
struct MsgPort *acs_oldport = NULL;	/* old MsgPort */

#ifdef DO_ACS_KLUDGE
/*
 * - New option "-K" (Kludge) for KingCON/ToolManager, sends DOS-Paket
 *    ACTION_CHANGE_SIGNAL.  Don't use this option if you have no problems.
 */
int    acs_kludge = 0;			/* ACTION_CHANGE_SIGNAL kludge */
#endif

#ifdef MULTIUSER_SUPPORT
struct muBase *muBase = NULL;     /* LILJA: Added for multiuser-support */
#endif

static char *defset[]={
	v_histnum,  "0",
	v_titlebar, shellname,
	v_hist,     "50",
	v_lasterr,  "0",
	v_stat,     "0",
	v_path,     "RAM:,RAM:c,df0:c,df1:c,sys:system,csh:,s:",
	v_rxpath,   "REXX:",
	v_scroll,   "2",
	v_minrows,  "34",
	v_hilite,   "c7",
	v_lcd,      "",
	v_qcd,      "csh:csh-qcd",
	v_insert,   "1",
	v_abbrev,   "5",
	"_terminal","",
	"_man",     "csh:csh.doc",
	"_version", shellversion,
/*	v_nomatch,  "1",*/
	v_prghash,  "csh:csh-prgs",
	v_complete, "*",
	NULL,       NULL
};

static char *defalias[]={
	"cls",  "echo -n ^l",
	"dswap","cd $_lcd",
	"exit", "endcli;quit",
	"cdir", "%q cd $q; cls; dir",
	"q",    "quit",
	"rx",   "%q RX $q",
	"manlist", "search -nl $_man \"    \"",
	NULL,   NULL
};

struct MsgPort *Console=(struct MsgPort *)-1;
long ExecTimer, ExecRC;

#ifdef __SASC
long __stack = 17500L;
#endif

main(int argc, char *argv[])
{
	static ROOT locals;
	BPTR fh;
	int i;
	char buf[10];
	BOOL login=FALSE,nologin=FALSE,nocshrc=FALSE,noclrmenu=FALSE;
#ifdef AZTEC_C
	extern int Enable_Abort;
	Enable_Abort = 0;
#endif

	MyMem=salloc(4);

	if( argc==0 ) {              /* Prevent starting from workbench */
		Delay(60);
		exit(0);
	}

	Myprocess = (struct Process *)FindTask(NULL);
	OldCin    = Myprocess->pr_CIS;
	Mycli     = Cli();
/*	Mycli     = (struct CommandLineInterface *)((long)Myprocess->pr_CLI << 2);*/

	init_mbase();
	push_locals( &locals );
	initmap();

	if (!SysBase || SysBase->LibNode.lib_Version<37) {
		printf("Sorry, you'll need at least V37 to run CSH.\n");
		exit(20);
	}

	if (!DOSBase) {
		printf("No dos library\n");
		exit(20);
	}

	/* we do not work without it ... */
	DOSBase->dl_Root->rn_Flags |= RNF_WILDSTAR;

	if (!(GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",37L))) {
		printf("No graphics library\n");
		exit(20);
	}

	if (!(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37L))) {
		printf("No intuition library\n");
		CloseLibrary((struct Library *)GfxBase);
		exit(20);
	}

	if (!(AslBase=OpenLibrary("asl.library",37L))) {
		printf("No asl library\n");
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)GfxBase);
		exit(20);
	}

	if (!(GadToolsBase=OpenLibrary("gadtools.library",37L))) {
		printf("No gadtools library\n");
		CloseLibrary((struct Library *)AslBase);
		CloseLibrary((struct Library *)IntuitionBase);
		CloseLibrary((struct Library *)GfxBase);
		exit(20);
	}

#ifdef MULTIUSER_SUPPORT
	/* LILJA: Added for multiuser-support */
	muBase = (struct muBase *)OpenLibrary("multiuser.library",39);
#endif

	PatternBase = NULL; /* OpenLibrary("pattern.library",5L); */
	BattClockBase = OpenResource(BATTCLOCKNAME);

	set_var(LEVEL_SET,"_kick2x",(SysBase->LibNode.lib_Version>=37)?"1":"0");
	set_var(LEVEL_SET,"_kick3x",(SysBase->LibNode.lib_Version>=39)?"1":"0");

	set_kickversion();

	if( !IsInteractive(Input())) {
		o_bground=1;
		Mycli->cli_Background=DOSTRUE;
	} else
		Mycli->cli_Background=DOSFALSE;

	set_var( LEVEL_SET, v_bground, (o_bground) ? "1" : "0" );

	/* We don't really need "CONSOLE:", maybe good for stderr ... */
	if( fh=Open( "CONSOLE:" , MODE_NEWFILE) ) {
		Console= ((struct FileHandle *)(4*fh))->fh_Type;
		Close(fh);
	}

	Forbid();
	i=0;
	/* AMK: OS20-GetVar replaces ARP-Getenv, SetVar replaces Setenv */
	if ( GetVar(shellthere,buf,10,GVF_GLOBAL_ONLY|GVF_BINARY_VAR) <  0L) {
		login=TRUE;
		SetVar(shellthere,"1",-1L,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
	}
	if ( GetVar(shellres  ,buf,10,GVF_GLOBAL_ONLY|GVF_BINARY_VAR) >= 0L)
		o_resident=1;
	if ( GetVar(shellctr  ,buf,10,GVF_GLOBAL_ONLY|GVF_BINARY_VAR) >= 0L)
		i=atoi(buf);
	sprintf(buf, "%d", i+1);
	/* AMK: OS20-SetVar replaces ARP-Setenv */
	SetVar(shellctr, buf, -1L, GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
	Permit();

#ifdef AZTEC_C
	stdin->_flags	|= _IONBF;	/* make sure we're set as an unbuffered tty */
	stdout->_flags	|= _IONBF;	/* in case of redirection in .login */
	Close( (BPTR)_devtab[2].fd);
	_devtab[2].mode |= O_STDIO;
	_devtab[2].fd = _devtab[1].fd;	/* set stderr to Output() otherwise */
					/* don't work with aux driver */
#else
	/* if( setvbuf( stdout,NULL,_IOLBF,BUFSIZ )) exit(20); */
	/* setnbf( stdout ); */
	/* Close( _ufbs[2] );*/
	/*_ufbs[2]=_ufbs[1]; */
	/* setnbf( stderr ); */
#endif

	sprintf(buf,"%ld",Myprocess->pr_TaskNum);
	set_var(LEVEL_SET, "_clinumber", buf);

	seterr(0);
	if (Myprocess->pr_CurrentDir == NULL)
		execute("cd :");
	set_cwd();

	o_nowindow= 1;

	set_var(LEVEL_SET,v_prompt, (IsInteractive(Input())) ? "%c%p> ":"");

	for( i=0; defset[i]; i+=2 )
		set_var( LEVEL_SET, defset[i], defset[i+1] );
	for( i=0; defalias[i]; i+=2 )
		set_var( LEVEL_ALIAS, defalias[i], defalias[i+1] );

	o_nowindow= 0;

	for (i = 1; i < argc; ++i) {
		if (*argv[i]=='-' && (index(argv[i],'a') || index(argv[i],'t')))
			o_nowindow=1;
		if (*argv[i]=='-' && index(argv[i],'n'))
			nologin = TRUE;
		if (*argv[i]=='-' && index(argv[i],'N'))
			nocshrc = TRUE;
		if (*argv[i]=='-' && index(argv[i],'L'))
			nologout = TRUE;
		if (*argv[i]=='-' && index(argv[i],'M'))
			noclrmenu = TRUE;
		if (*argv[i]=='-' && index(argv[i],'W'))
			nowintitle = TRUE;
		if (*argv[i]=='-' && index(argv[i],'R'))
			o_noraw = 1;
		if (*argv[i]=='-' && index(argv[i],'w'))
			o_nowindow = 1;
		if (*argv[i]=='-' && (index(argv[i],'C') || index(argv[i],'c'))) {
			noclrmenu  = TRUE;
			nowintitle = TRUE;
		}
	}

	if (Mycli->cli_Background) {
		o_noraw    = 1;
		o_nowindow = 1;
		o_vt100    = 1;
	}

	if( !o_nowindow && (Mywindow=getwindow()) && IsInteractive(Input())) {
		old_WindowPtr = Myprocess->pr_WindowPtr;
		Myprocess->pr_WindowPtr = 0L/*Mywindow*/;
		newwidth();
		if (!nowintitle)
			oldtitle=(char *)(Mywindow->Title);
		if (!noclrmenu) {
			/* clear menus, even if we are not the owner */
			set_menu();
		}
	}

	if (login && !nologin) {
		/*printf("we're the first csh today, mon ami!\n");*/
		if( exists("S:.login"))
			execute("source S:.login");
	}

	if (!nocshrc) {
		if( exists("S:.cshrc"))
			execute("source S:.cshrc");
	}

	{
	char nam1[40],nam2[40],nam3[40];
	BOOL e1,e2,e3;
	strcpy(nam1,"ENVARC:"); strcat(nam1,shellctr);   e1=exists(nam1);
	strcpy(nam2,"ENVARC:"); strcat(nam2,shellres);   e2=exists(nam2);
	strcpy(nam3,"ENVARC:"); strcat(nam3,shellthere); e3=exists(nam3);

	if (e1 || e2 || e3) {
		printf("\nWARNING: please remove the following files from ENVARC:\n\n");
		if (e1)
		printf("          - %s\n",shellctr);
		if (e2)
		printf("          - %s\n",shellres);
		if (e3)
		printf("          - %s\n",shellthere);
		printf("\n         and reboot your system after removing the files.\n");
		printf("         Never copy ENV: to ENVARC: !!\n\n");
		Delay(150L);
	}
	}

	/* would be a nice idea to have a flag that tells if this is
	   a full shell or just executing some commands and the leaving */

	for (i = 1; i < argc; ++i) {
		if (*argv[i]=='-') {
			char *str=argv[i]+1;
			for( ; *str; str++ ) {
				switch( *str) {
				case 'c':
				case 'C': execute( compile_av( argv, i+1, argc, ' ',*str=='C'));
				          main_exit(Lastresult); break;
				case 'k': set_var(LEVEL_SET,v_nobreak,"1"); break;
#ifdef DO_ACS_KLUDGE
				case 'K': acs_kludge = 1; break;
#endif
				case 'v': Verbose=1; set_var(LEVEL_SET,v_verbose,"hs"); break;
				case 'b': oldtaskpri=Myprocess->pr_Task.tc_Node.ln_Pri;
				          SetTaskPri( &Myprocess->pr_Task, -1 ); break;
				case 'f': oldtaskpri=Myprocess->pr_Task.tc_Node.ln_Pri;
				          SetTaskPri( &Myprocess->pr_Task,  1 ); break;
				case 'a': o_nowindow= o_noraw= 1;
				          set_var( LEVEL_SET, v_hilite, "" ); break;
#if 0
				case 'm': unset_var(LEVEL_SET,v_nomatch); break;
#endif
				case 'm': set_var(LEVEL_SET,v_nomatch,"1"); break;
				case 't': o_nowindow= o_vt100= o_nofastscr= 1;
				          set_var( LEVEL_SET, v_hilite, "b" /*"r"*/ );
				          set_var( LEVEL_SET, v_noreq, "1" );
				          set_var( LEVEL_SET, "_terminal", "1" );
				          set_var( LEVEL_ALIAS, "cls", "e -n ^[[0\\;0H^[[J" );
				          break;
				case 's': DOSBase->dl_Root->rn_Flags |= RNF_WILDSTAR;
				          break;
				case 'w': /* just don't use window pointer */
				          o_nowindow = 1;
				          set_var( LEVEL_SET, "_timeout", "1" ); /* GWB_TIMEOUT_LOCAL */
				          break;
				case 'V': /* strict VT100 mode */
				          o_vt100 = 1;
				          break;
				}
			}
		} else {
			sprintf (Inline, "source %s",argv[i]);
			execute (Inline);
		}
	}

	/* we should do "main_exit(Lastresult);" here if
	   csh was called with some files to source ... */

#if 0
	if( fh=Open( "CONSOLE:" , MODE_NEWFILE) ) {
		if (Console != ((struct FileHandle *)(4*fh))->fh_Type)
			Write(fh,"Funny Console!\n",15);
		else
			Write(fh,"Real Console!\n",14);
		Close(fh);
	}
	else
		printf("No Console!\n");

	Write(Input(),"Input()!\n",9);
	Write(Output(),"Output()!\n",10);
#endif

#if 0
	/* cannot exit because of "echo mem | csh" or "csh <cmd_file */

	if( !IsInteractive(Input()) ) {
		main_exit( Lastresult );
	}
#endif

#ifdef DO_ACS_KLUDGE
	if (acs_kludge>0) {
		BPTR bfh = Open("*", MODE_OLDFILE);
		struct FileHandle *fh;
		long xrslt,xioerr;
		if (bfh != NULL) {
			fh = BADDR(bfh);
			if (fh->fh_Type != NULL) {
				xrslt = DoPkt( (void *)fh->fh_Type, ACTION_CHANGE_SIGNAL, fh->fh_Arg1, (long)&(Myprocess->pr_MsgPort), NULL,NULL,NULL);
				/* also possible instead of fh->fh_Type: Myprocess->pr_ConsoleTask */
				xioerr = IoErr();
				acs_oldport = (struct MsgPort *)xioerr;
				printf("ACS_1: DoPkt()=%ld, fh_Type=%ld, ConTask=%ld, pr_MsgPort=%ld, IoErr()=%ld\n",xrslt,(long)fh->fh_Type,(long)Myprocess->pr_ConsoleTask,(long)&(Myprocess->pr_MsgPort),xioerr);
			}
			Close(bfh);
		}
		acs_kludge = 2;
	}
#endif

	for (;;) {
		if (breakcheck())
			while (WaitForChar(Input(), 100L) || CHARSWAIT( stdin ))
				gets(Inline);
		clearerr(stdin);  /* prevent acidental quit */
		/*
		   AMK: new position of breakreset() before exec_every(),
		        old position caused enforcer hits
		        (do 'set _every "set cols = $(wincols)"' and
		         then abort (^c) a command)
		*/
		breakreset();
		exec_every();

		{ /* AMK TEST BEGIN */
			char *old;
			char pwd[256];

			/* no requesters when no disk is inserted */
			Myprocess->pr_WindowPtr = (APTR)(-1);

#if 1
			if (NameFromLock(Myprocess->pr_CurrentDir, pwd, 255L)) {
				if( !(old=get_var(LEVEL_SET, v_cwd)) )
					old="";
				if (strcmp(pwd,old)) {
					set_var(LEVEL_SET, v_lcd, old);
					set_cwd();
				}
			}
#else
			if (!NameFromLock(Myprocess->pr_CurrentDir, pwd, 255L)) {
				fprintf(stderr,"csh.main: NameFromLock() failed\n");
				strcpy(pwd,"<unknown>");
			}
			if( !(old=get_var(LEVEL_SET, v_cwd)) )
				old="";
			if (strcmp(pwd,old)) {
				set_var(LEVEL_SET, v_lcd, old);
				set_cwd();
			}
#endif
			/*update_sys_var(v_cwd);*/

			Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;

		} /* AMK TEST BEGIN */

		update_sys_var(v_titlebar);
		update_sys_var(v_prompt);
		/*
		   AMK: old position of breakreset(), causes enforcer hits
		        (do 'set _every "set cols = $(wincols)"' and
		         then abort (^c) a command)
		*/
		breakreset();
#if RAW_CONSOLE
		if (Quit || !rawgets(Inline, disable ? "_ " : trueprompt)) main_exit(0);
#else
		printf("%s", disable ? "_ " : trueprompt);
		fflush(stdout);
		if (Quit || !gets(Inline)) main_exit(0);
#endif
		breakreset();

#if 1
		/* start/stop timer only if non-empty command line */

		if (*Inline && strlen(Inline)>0) {
			exectimer(0);
			ExecRC = exec_command(Inline);
			exectimer(1);
		}
#else
		/* start/stop timer for every command  */

		exectimer(0);
		if (*Inline) ExecRC=exec_command(Inline);
		exectimer(1);
#endif
	}
}


void
main_exit(n)
{
	int i;
	char buf[10];

	/* AMK: OS20-GetVar replaces ARP-Getenv */
	GetVar(shellctr,buf,10L,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
	i=atoi(buf);
	sprintf(buf,"%d",i-1);
	/* AMK: OS20-SetVar replaces ARP-Setenv */
	SetVar(shellctr, buf, -1L, GVF_GLOBAL_ONLY|GVF_BINARY_VAR);

#ifdef DO_ACS_KLUDGE
	if (acs_kludge>0) {
		BPTR bfh = Open("*", MODE_OLDFILE);
		struct FileHandle *fh;
		long xrslt,xioerr;
		if (bfh != NULL) {
			fh = BADDR(bfh);
			if (fh->fh_Type != NULL) {
				xrslt = DoPkt( (void *)fh->fh_Type, ACTION_CHANGE_SIGNAL, fh->fh_Arg1, (long)acs_oldport, NULL,NULL,NULL);
				/* also possible instead of fh->fh_Type: Myprocess->pr_ConsoleTask */
				xioerr = IoErr();
				printf("ACS_2: DoPkt()=%ld, fh_Type=%ld, ConTask=%ld, oldport=%ld, IoErr()=%ld\n",xrslt,(long)fh->fh_Type,(long)Myprocess->pr_ConsoleTask,(long)acs_oldport,xioerr);
			}
			Close(bfh);
		}
		acs_kludge = 0;
	}
#endif

	if( !nowintitle && oldtitle )
		SetWindowTitles(Mywindow,oldtitle,(char *)-1);
	if( oldtaskpri != -999 )
		SetTaskPri(&Myprocess->pr_Task,oldtaskpri);

	if (Mywindow)
		Myprocess->pr_WindowPtr = old_WindowPtr;

	for (i=1; i<MAXMYFILES; i++) myclose(i);

	/* no effect if AnzMenus==0 (no menus in use) */
	remove_menu();

#ifdef MULTIUSER_SUPPORT
	/* LILJA: Added for multiuser-support */
	if (muBase) CloseLibrary((struct Library *)muBase); muBase=NULL;
#endif

	if (PatternBase) CloseLibrary( PatternBase );
	CloseLibrary((struct Library *)GadToolsBase);
	CloseLibrary((struct Library *)AslBase);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)GfxBase);
	exit(n);
}


#if 1
static void
exectimer(int stop)
{
	struct DateStamp dss;
	static struct DateStamp last;

	DateStamp(&dss);
	if (stop) {
		ExecTimer = dss.ds_Minute*6000 + 2*dss.ds_Tick;
		ExecTimer += (dss.ds_Days-last.ds_Days)*6000*1440;
		ExecTimer -= last.ds_Minute*6000 + 2*last.ds_Tick;
	}
	else
		last = dss;
}
#else
static void
exectimer(int stop)
{
	struct DateStamp dss;
	static long lasttime;
	long time;

	DateStamp(&dss);
	time=dss.ds_Minute*6000+2*dss.ds_Tick;   /* 2 = 100/TickPerSec   */
	if( stop )
		ExecTimer=time-lasttime;
	else
		lasttime=time;
}
#endif


/* print break message only once */
static BOOL dobreak_output = TRUE;

int
breakcheck()
{
	return !o_nobreak && SetSignal(0L,0L) & SIGBREAKF_CTRL_C;
}

void
breakreset()
{
	/* We reset CTRL_E here for GMD's foreach() hack ... */
	/* Why not just reset CTRL_F also -- for completeness? :) */

	SetSignal(0L, SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D|SIGBREAKF_CTRL_E);
	dobreak_output = TRUE;
}

dobreak()
{
	if (breakcheck()) {
		if (dobreak_output) {
			printf("^C\n");
			dobreak_output = FALSE;
		}
		return(1);
	}
	return(0);
}

/* this routine causes manx to use this Chk_Abort() rather than it's own */
/* otherwise it resets our ^C when doing any I/O (even when Enable_Abort */
/* is zero).  Since we want to check for our own ^C's			 */

long
Chk_Abort()
{
	return(0);
}

void
_wb_parse()
{
}

do_howmany()
{
	char buf[10];

	/* AMK: OS20-GetVar replaces ARP-Getenv */
	GetVar(shellctr, buf, 10L, GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
	printf("Shell(s) running: %s\n",buf);
	return 0;
}

static void set_kickversion(void)
{
	struct Library *verlib;
	static char verstr[10];
	if (verlib = OpenLibrary("version.library",0)) {
		sprintf(verstr,"%d",verlib->lib_Version);
		CloseLibrary(verlib);
	}
	else
		sprintf(verstr,"%d",SysBase->LibNode.lib_Version);
	set_var(LEVEL_SET,"_kick",verstr);
}

#if 1
static struct Window *
getwindow(void)
{
	struct Window *win = NULL;

	if( o_nowindow )
		return NULL;

	if( IsInteractive(Output()) )
		Write(Output(),"",1);     /*    make window appear */

	if( isconsole(Output()) && Output() ) {
		if ( ((struct FileHandle *)BADDR(Output()))->fh_Type ) {
			struct InfoData *infodata;
			infodata=(void *)SAllocMem((long)sizeof(struct InfoData),MEMF_CLEAR|MEMF_PUBLIC);
			if (DoPkt((void *)((struct FileHandle *)BADDR(Output()))->fh_Type,ACTION_DISK_INFO,((LONG)infodata)>>2,NULL,NULL,NULL,NULL)) {
				win=(struct Window *)infodata->id_VolumeNode;
			}
			FreeMem(infodata,sizeof(struct InfoData));
		}
		if( win==NULL )
			o_nowindow=1;
	}

	newwidth();

	return win;
}
#else
static struct Window *
getwindow(void)
{
	struct Window *win = NULL;

	if( o_nowindow )
		return NULL;
	if( isconsole(Output()))
		Write(Output(),"",1);     /*    make window appear */
	if( Myprocess->pr_ConsoleTask ) {
		struct InfoData *infodata;
		infodata=(void *)SAllocMem((long)sizeof(struct InfoData),MEMF_CLEAR|MEMF_PUBLIC);
		if (DoPkt((void *)Myprocess->pr_ConsoleTask,ACTION_DISK_INFO,((LONG)infodata)>>2,NULL,NULL,NULL,NULL)) {
			win=(struct Window *)infodata->id_VolumeNode;
		}
		FreeMem(infodata,sizeof(struct InfoData));
		if( win==NULL )
			o_nowindow=1;
	}
	newwidth();
	return win;
}
#endif

#ifdef LATTICE

int
setenv( char *var, char *val )
{
	char *buf=salloc(strlen(var)+strlen(val)+10);
	sprintf(buf, "%s=%s", var, val );
	Free(buf);
	return putenv(buf);
}

__regargs int __chkabort(void) { return(0); }

#endif

#ifdef _DCC
void
swapmem(void *_s1, void *_s2, size_t _n)
{
	char t, *s1=_s1, *s2=_s2;
	int  i;

	for( i=0; i<_n; i++ )
		t = *s1; *s1++ = *s2; *s2++=t;
}

int
setenv( char *var, char *val )
{
	BPTR fh;
	char buf[40];
	sprintf(buf,"ENV:%s",var );

	if( fh=Open(buf,MODE_NEWFILE) ) {
		Write(fh,val,strlen(val));
		Close(fh);
	}
}

int
rand()
{
	return 0;
}

_waitwbmsg() {return 0;};

#endif
