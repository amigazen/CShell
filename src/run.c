/*
 * RUN.C
 *
 * (c)1986 Matthew Dillon     9 October 1986
 *
 *    RUN   handles running of external commands.
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 *
 */

#include "shell.h"

int MySyncRun( char *com, char *args, BPTR in, BPTR out, int nosync );
int echofunc(void);

int
do_run( char *str, int nosync )
{
	int retcode;
	char buf[256];        /* enough space for 100 char cmd name + path stuff */
	char *path, *path2, *argline, *copy = NULL, *ext, *end;

	if( !*av[0] )
		return 0;

	if( (retcode=echofunc())>=0 )
		return retcode;

	a0tospace( av[0] );                                 /* allow "com mand" */

	argline=compile_av(av, 1, ac, ' ', 1);

	if (strlen(av[0]) > 100) { ierror(NULL,509); return -1; }

	if( ac==1 && isdir(av[0])) {
		sprintf(buf,"cd \"%s\"",av[0]);
		return execute( buf );
	}

	IoError=IoErr();
	if( (IoError==218 || IoError==225 || IoError==226) && index(av[0],':')) {
		ierror( av[0], IoError );
		return 20;
	}

	sprintf(buf,"res_%s",FilePart(av[0]));               /* delayed residents */
	/* AMK: OS20-GetVar replaces ARP-Getenv */
	if (o_resident && GetVar(buf,buf+100,90L,GVF_GLOBAL_ONLY|GVF_BINARY_VAR)>=0L) {
		/* AMK: OS20-SetVar replaces ARP-Setenv */
		SetVar(buf,NULL,NULL,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
		loadres(buf+100);
	}

	if( (retcode=MySyncRun(av[0],argline,0,0,nosync))>=0 )   /* AmigaDOS path */
		goto done2;

	if( retcode == -2 /*PR_NOMEM*/ ) {
		ierror( av[0], 103 );
		return 20;
	}

	IoError=IoErr();
	if( (IoError==218 || IoError==225 || IoError==226) && index(av[0],':')) {
		ierror( av[0], IoError );
		return 20;
	}

	if (path = dofind(av[0],"",buf+80,v_path)) {             /* shell path    */
		DPTR *dp;
		BPTR fh;
		int stat, script;
		if((retcode = MySyncRun(path,argline,0,0,nosync))>=0)
			goto done2;
		if(dp=dopen(path,&stat)) {
			script= dp->fib->fib_Protection&FIBF_SCRIPT;
			dclose(dp);
			if( !stat && script ) {
				char *t,*dynbuf;
				int dynret;
				buf[0]=0;
				if( fh=Open(path,MODE_OLDFILE )) {
					Read(fh,buf,79);
					Close(fh);
					buf[79] = 0;
					if(t=index(buf,'\n')) *t=0;
				}

				dynbuf = salloc(strlen(buf)+strlen(str)+10);

				if( buf[0]=='/' && buf[1]=='*' ) {
					sprintf(dynbuf, "Rx %s", str );
				} else if( (buf[0]!=';' || buf[0]!='#') && buf[1]=='!' ) {
					memmove(dynbuf,buf+2,strlen(buf+2)+1);
					strcat( dynbuf," ");
					strcat( dynbuf,str);
				} else {
					sprintf(dynbuf,"Execute %s", str );
				}
				dynret = execute( a0tospace(dynbuf));

				free(dynbuf);
				return(dynret);
			}
		}
	}

	if(!(end=rindex(av[0],'.'))) end="";               /* automatic sourcing */
	ext=strcmp(end,".sh") ? ".sh" : "";
	if (path = dofind(av[0],ext,buf,v_path)) {
		av[1] = buf;
		copy = salloc(strlen(str)+3);
		sprintf(copy,"x %s",str);
		retcode = do_source(copy);
		goto done;
	}

#if 0
	/* what is it good for??? -amk */
	copy=salloc(strlen(av[0])+strlen(argline)+5);
	sprintf(copy,"%s %s",av[0],argline);
#endif

	ext=strcmp(end,".rexx") ? ".rexx" : "";           /* automatic rx-ing   */
	if( path = dofind(av[0], ext, buf, v_rxpath )) {
		copy = salloc(strlen(path)+strlen(argline)+5);
		sprintf(copy,"%s %s",path,argline);
#if 1
/* dynamic command line, no limits! -amk */
		if ( (retcode=MySyncRun("rx",copy,0,0,0)) >=0 )
			goto done;
		if (path2 = dofind("rx","",buf,v_path)) {
			retcode = MySyncRun(path2,copy,0,0,0);
			goto done;
		}
#else
/* static command line, limited to 140 chars! -amk */
		strcat (path," ");
		if( strlen(argline)>140 ) argline[140]=0;
		strcat (path,argline);
/*		strncpy(path+strlen(path),argline,190); */
		if( (retcode=MySyncRun("rx",path,0,0,0)) >=0 ) goto done;
		kprintf("balu: %s\n",buf+160);
		if (path2 = dofind("rx","",buf+160,v_path)) {
			retcode = MySyncRun(path2,path,0,0,0);
			goto done;
		}
#endif

	}

	if( !doaction(av[0],"exec",argline)) {
		retcode=0;
		goto done;
	}

	retcode = -1;
	fprintf(stderr,"Command not found %s\n",av[0]);

done:
	if (copy)
		free(copy);
done2:
	setioerror( IoErr() );
	free( argline );
	return retcode;
}



#ifndef END_STREAM_CH
#define END_STREAM_CH -1L
#endif
BPTR
	new_input,	/* for execute'ing a script file */
	old_inp_fh,	/* old input filehandle */
	old_out_fh,	/* old output filehandle */
	seglist_cmd;	/* to be returned from NewLoadSeg */


void set_returncodes(long returncode,long result2)
{
	Mycli->cli_ReturnCode = returncode;
	Mycli->cli_Result2    = result2;
}



void clean_up_io(void)
{
	long ch;
	Flush(Output());
	ch = UnGetC(Input(),END_STREAM_CH) ? 0 : '\n';
	while ((ch != '\n') && (ch != END_STREAM_CH))
		ch = FGetC(Input());
}

long command_examine(char *fname,BPTR *plock)
{
    /*
	Given a filename, attempt to determine if we can process it. Either
	by running it, or by 'executing' it (a script).

	Returns:
	    0 = can RunCommand the file
	    1 = can source/script/execute the file
	  < 0 = error
    */
	struct FileInfoBlock *fib;
	long i;
	BPTR lock;

	*plock = NULL;

	if (!(lock=Lock(fname,ACCESS_READ)))
		return -1;

	if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL))) {
		UnLock(lock);
		return -9;
	}

	if (!Examine(lock,fib)) {
		UnLock(lock);
		FreeDosObject(DOS_FIB,fib);
		return -2;
	}

	i = fib->fib_DirEntryType;

	if (i==ST_SOFTLINK) {
		/*
		   Let our caller resolve the link, and if it resolves to a file,
		   call us again.
		*/
		UnLock(lock);
		FreeDosObject(DOS_FIB,fib);
		return -10;
	}

	if (!((i==ST_FILE) || (i==ST_LINKFILE))) {
		UnLock(lock);
		FreeDosObject(DOS_FIB,fib);
		return -3;
	}

	i = fib->fib_Protection;
	i = (i & 0x70) | (0x0f & ~i);
	if (!((i & FIBF_SCRIPT) || (i & FIBF_EXECUTE))) {
		/* Not an executable or a script file. */
		UnLock(lock);
		FreeDosObject(DOS_FIB,fib);
		return -4;
	}

	FreeDosObject(DOS_FIB,fib);
	seglist_cmd = NULL;
	new_input = NULL;
	*plock = ParentDir(lock);

	if (i & FIBF_SCRIPT) {
		/*
		    Open the file, but let the 'outside world' dick with CurrentInput.
		    Not me. Outside of my definition. :)
		*/
		if (!(new_input=OpenFromLock(lock))) {
			UnLock(lock);
			UnLock(*plock);
			return -5;
		}
		/* Remember that 'lock' is now INVALID and should not be touched. */
		return 1;
	}

	if (i & FIBF_EXECUTE) {
		/* LoadSeg the sucker. */
		if (!(seglist_cmd=NewLoadSeg(fname,NULL))) {
#if 0
			/* AMK: distinguish between "no memory" and "not an executable" */
			if (IoErr()==ERROR_NO_FREE_STORE) {
				UnLock(lock);
				UnLock(*plock);
				return -11;
			}
#endif
			/* Probably a 'bad' file (i.e., not actually an executable). */
			UnLock(lock);
			UnLock(*plock);
			return -6;
		}
		UnLock(lock);
		return 0;
	}

	if (lock) UnLock(lock);
	if (*plock) UnLock(*plock);
	return -7;	/* should NEVER reach this point */
}



long command_device(char *device,char *fname)
{
    /*
	For the Device specified by *device, search each element of the
	assign (since it could be a multi-assign) and try to find a
	command file. A command file can be either an executable file, or
	a script file (one with the script bit set).

	Returns:
	    0 = can RunCommand this file (seglist_cmd set)
	    1 = can source/script/execute this file (new_input set)
	  < 0 = error
	   -8 = Bad device name

	Note that this routine generates only one error of its own. All
	other results are passed straight thru from command_examine ().
    */

	long gotlock,			/* we have a directory lock or not */
	     result = 0,		/* from command_examine () */
	     /* AMK: result initialized with 0 */
	     done = 0;			/* found something we could use */
	struct DevProc *dp = NULL;	/* for searching multi-assigned paths */
	struct MsgPort *fstask;		/* for GetFileSysTask () */
	BPTR plock,			/* parent of fname */
	     lock,			/* on source directory */
	     dir_lock;			/* save current directory */

	/*printf("search: %s -> %s\n",device,fname);*/

	fstask = GetFileSysTask ();

	do {
		dp = GetDeviceProc(device,dp);
		if (dp) {
			SetFileSysTask(dp->dvp_Port);

			lock = NULL;
			gotlock = 0;
			if (dp->dvp_Lock) {
				dir_lock = CurrentDir(dp->dvp_Lock);
				gotlock = 1;
			}
			else {
				if (lock=Lock(device,ACCESS_READ)) {
					dir_lock = CurrentDir(lock);
					gotlock = 1;
				}
			}
			if (gotlock) {
				result = command_examine(fname,&plock);
				/*
				    NOTE: Philosophically speaking, if result <= -2, what
				    should we do? This means that command_examine () actually
				    found a file by the correct name, but it was unsuitable for
				    some reason.
				    Currently, we continue to search, in hopes that St. Nicholas
				    will soon be here...(ie, we will find a file that meets our
				    criteria). This is questionable as to its correctness.
				*/
				if (result<0) {
					done = 0;
				}
				else {
					BPTR hdir_lock;
					if (plock)
						hdir_lock = plock;
					else if (lock)
						hdir_lock = lock;
					else if (dp->dvp_Lock)
						hdir_lock = dp->dvp_Lock;
					else
						hdir_lock = NULL;
/*
					{char buf[256];
					if (hdir_lock && NameFromLock(hdir_lock,buf,256))
						printf("hdir_lock: %s\n",buf);
					}
*/
					done = 1;
					/*
					--- This was from XSHell, but wasn't correct ---
					Myprocess->pr_HomeDir = DupLock(lock?lock:dp->dvp_Lock);
					*/
					Myprocess->pr_HomeDir = DupLock(hdir_lock);
					UnLock(plock);
				}
				if (lock) {
					UnLock(lock);
					lock = NULL;
				}
				CurrentDir(dir_lock);
			}
		}
	} while (!done                               &&
		 dp && (dp->dvp_Flags & DVPF_ASSIGN) &&
		 IoErr() == ERROR_OBJECT_NOT_FOUND);

	SetFileSysTask (fstask);
	if (dp) FreeDeviceProc(dp);

	if (!done && result >= 0) {
		/* Can happen when GetDeviceProc returns a NULL dp on the first go. */
		result = -8;
	}

	return result;
}



long command_processor(long abs_cmd,char *device,char *fname)
{
    /*
	Results:
	    Those returned by command_examine () and command_device ().

	    0 = can RunCommand (seglist_cmd set)
	    1 = can source/script/execute (new_input set)
	  < 0 = error
    */

	struct PathList *pl;	/* to parse the PATH */
	long result,		/* from command_examine () or command_device () */
	     sub_cmd;		/* command is in a sub-directory */
	BPTR plock;		/* points to parent directory of fname */
	char buf [256];		/* where to put elements of the Cli Path */
	long i;			/* browse thru prghash_num's */
	char *filepart;		/* file-part of prghash_list items */
	long fnamelen;		/* length of fname */

	sub_cmd = abs_cmd & 2;	/* a '/' was found in the input filename */
	abs_cmd &= 1;		/* so it only has one meaning: a ':' was found */

	if (*device == '\0')
		abs_cmd = 0;	/* handle special case of empty device */

	if (abs_cmd) {		/* absolute path like 'dh0:myprog' */
		result = command_device(device,fname);
		return result;
	}
	if (sub_cmd) {		/* relative path like 'mydir/myprog' */
		result = command_examine (fname, &plock);
		if (result < 0) return result;
		/*
		    NOTE: Is the following OK? Or should we DupLock() and then
		    UnLock()? I don't see why....
		*/
		Myprocess->pr_HomeDir = plock;
		plock = NULL;
		return result;
	}
	/* file in current directory */
	result = command_examine(fname,&plock);
	if (result >= 0) {
		Myprocess->pr_HomeDir = plock;
		plock = NULL;
		return result;
	}

	/* BEGIN -- AMK TEST FOR HASH-LIST -- BEGIN */

	fnamelen = strlen(fname);

	for(i=0; i<prghash_num; i++) {
		if (filepart=FilePart(prghash_list[i])) {
			if ((o_abbrev&2 && strnicmp(fname,filepart,fnamelen)==0)
			|| (!(o_abbrev&2) && stricmp(fname,filepart)==0))
			{
				/*printf("hash hit: %s -> %s\n",fname,prghash_list[i]);*/
				result = command_examine(prghash_list[i],&plock);
				if (result >= 0) {
					Myprocess->pr_HomeDir = plock;
					plock = NULL;
					return result;
				}
			}
		}
	}

	if (prghash_num>0 && !(o_abbrev&4))
		return result;

	for(i=0; i<prghash_num; i++) {
		if (filepart=FilePart(prghash_list[i])) {
			if (strnicmp(fname,filepart,fnamelen)==0) {
				/*printf("hash hit: %s -> %s\n",fname,prghash_list[i]);*/
				result = command_examine(prghash_list[i],&plock);
				if (result >= 0) {
					Myprocess->pr_HomeDir = plock;
					plock = NULL;
					return result;
				}
			}
		}
	}

	if (prghash_num>0 && !(o_abbrev&8))
		return result;

	/* END -- AMK TEST FOR HASH-LIST -- END */

	/* DOS search path */
	/* Forbid(); */
	pl = (struct PathList *)BADDR(Mycli->cli_CommandDir);
	while (pl && !dobreak()) {
		if (pl->pl_PathLock) {
			if (NameFromLock(pl->pl_PathLock,buf,256)) {
				result = command_device(buf,fname);
				if (result >= 0) {
					/* Permit(); */
					return result;
				}
			}
			else {
				fprintf(stderr,"csh.command_processor: NameFromLock() failed\n");
			}
		}
		pl = (struct PathList *)BADDR(pl->pl_NextPath);
	}
	/* Permit(); */

	/* Last, but CERTAINLY not least, search 'C:' */
	if (!dobreak())
		result = command_device("C:",fname);

	return result;
}



/* no endless loop for recursive calls of MySyncRun() */
static BOOL IsRecursiveMySyncRun = FALSE;

int
MySyncRun( char *com, char *args, BPTR in, BPTR out, int nosync )
{
	struct Segment *cmdseg = NULL;
	int len=strlen(args);
	char myname[256];
	BPTR myhdir,mymod;
	long abs_cmd = 0;
	char *p,*pname=strdup(com), *ppath=strdup(com), *pdev=strdup(com);
	long rslt = -1;

/*
	old_inp_fh = SelectInput (Mycli->cli_CurrentInput);
	old_out_fh = SelectOutput(Mycli->cli_StandardOutput);
	Flush(Input());
	Flush(Output());
*/
	GetProgramName(myname,255);
	myhdir = Myprocess->pr_HomeDir;
	mymod  = Mycli->cli_Module;

	/* clear it so that we don't "free" ourself on MySyncRun_done: */
	Myprocess->pr_HomeDir = NULL;
	Mycli->cli_Module = NULL;

	args[len]='\n'; args[len+1]=0;		/* trailing '\n' */

	if (!pname || !ppath || !pdev)
		goto MySyncRun_done;

	if (p=FilePart(com))
		strcpy(pname,p);

	if (p=strchr(pdev,':')) {
		if (*(p+1)) {		/* is ':' last char? */
			abs_cmd |= 1;
			strcpy(ppath,++p);
			*p='\0';
		}
		else {			/* allow "bla:" for program name */
			strcpy(pdev,"");
			strcpy(ppath,com);
			strcpy(pname,com);
		}
	}
	else strcpy(pdev,"");

	if (strchr(ppath,'/'))
		abs_cmd |= 2;

#if 0
	kprintf("prog: %s, arg: %s, async: %s\n",com,args,nosync?"yes":"no");
	kprintf("pdev: %s, ppath: %s, pname: %s [%ld]\n",pdev,ppath,pname,abs_cmd);
#endif

	seglist_cmd = NULL;
	new_input = NULL;

	if (!abs_cmd || strnicmp(com,"C:",2)==0) {
		char *comname = (strnicmp(com,"C:",2)==0) ? FilePart(com) : com ;
		Forbid();
		if (!(cmdseg = FindSegment(comname,NULL,FALSE)))
			cmdseg = FindSegment(comname,NULL,TRUE);
		if (cmdseg) {
/*
	Randell says ...
			if ((cmdseg->seg_UC < CMD_DISABLED) ||
	... but if a command is disabled why we should execute it?
*/
			if ((cmdseg->seg_UC <= CMD_DISABLED) ||
			    (cmdseg->seg_UC == CMD_SYSTEM))
				cmdseg = NULL;
			else if (cmdseg->seg_UC >= 0)
				cmdseg->seg_UC++;
		}
		Permit();
		if (cmdseg) {
			seglist_cmd = cmdseg->seg_Seg;
			Myprocess->pr_HomeDir = NULL;
		}
	}

	if (!cmdseg) {
		rslt = command_processor(abs_cmd,pdev,ppath);
		/*printf("cmd_processor returned %ld, %ld\n",rslt,IoErr());*/
		if (rslt<0) {
			/*printf("Object not found (rc=%d): %s\n",rslt,pname);*/
			goto MySyncRun_done;
			/*return -1;*/
		}
	}

	if (new_input) {				/* rslt = 1 */
		enum {DOS_SCRIPT,REXX_SCRIPT,OTHER_SCRIPT} scp_type;
		char scp_name[256];
		char scmdbuf[256];
		char *exec_buf;
		char *scommand = "execute";
		FILE *sfp;
		int escnum,i;

		NameFromFH(new_input,scp_name,255);
		Close(new_input);
		new_input = NULL;

		/*kprintf("launch script (%s) %s, %s, %s, args: %s ...\n",scp_name,pdev,ppath,pname,args);*/
#if 0
		sys_tags[1].ti_Tag  = SYS_Output;
		sys_tags[1].ti_Data = (ULONG)Myprocess->pr_COS;
#endif
		scp_type = DOS_SCRIPT;

		if (sfp=fopen(scp_name,"r")) {
			char *p;
			if (fgets(scmdbuf,255,sfp)) {
				if (strncmp(scmdbuf,"/*",2)==0) {
					scommand = "rx";
					scp_type = REXX_SCRIPT;
					/*
					    Space im Filenamen mit:
					    scommand = "rx \"call 'tgled led' rudi\"";
					    Allerdings dann Probleme mit Return-Codes.
					 */
				}
				else if (strncmp(scmdbuf,"#!",2)==0 || strncmp(scmdbuf,";!",2)==0) {
					if (p = strchr(scmdbuf,'\n')) {
						*p = '\0';
						for (p=scmdbuf+2; *p==' ' || *p=='\t'; ++p)
							;
						scommand = p;
						scp_type = OTHER_SCRIPT;
						
						if (o_mappath && scommand[0]=='/') {
							if (p=strchr(scommand+1,'/')) {
								++scommand;
								*p = ':';
							}
						}
					}
					else
						fprintf(stderr,"%s: first line too long\n",scp_name);
				}
			}
			fclose(sfp);
		}

		/* escape critical chars like <>"=* with * (asterix) */

		/* count number of chars to be escaped */
		for (escnum=0,i=strlen(args)-1; i >= 0; --i) {
			if (args[i]=='>' || args[i]=='<' || args[i]=='*') /*|| args[i]=='"' || args[i]=='=')*/
				++escnum;
		}

		/*kprintf("scommand: %s, scp_name: %s, args: %s",scommand,scp_name,args);*/

		if (exec_buf = malloc(strlen(scommand)+2+strlen(scp_name)+2+strlen(args)+1+escnum)) {

			/* RunCommand(Execute) doesn't work, must use System() */

			if (!IsRecursiveMySyncRun && scp_type!=DOS_SCRIPT && stricmp(scommand,"execute")) {
				sprintf(exec_buf,"%s%s%s %s",
					(scp_type!=REXX_SCRIPT) ? "\"" : "",
					scp_name,
					(scp_type!=REXX_SCRIPT) ? "\"" : "",
					args
				);
				/*kprintf("yeah, recursive MySyncRun: %s %s\n",scommand,exec_buf);*/
				IsRecursiveMySyncRun = TRUE;
				if (MySyncRun(scommand,exec_buf,in,out,nosync) >= 0) {
					/*kprintf("okay... exit...\n");*/
					IsRecursiveMySyncRun = FALSE;
					free(exec_buf);
					goto MySyncRun_done;
				}
				IsRecursiveMySyncRun = FALSE;
			}

			sprintf(exec_buf,"%s %s%s%s %s",
				scommand,
				(scp_type!=REXX_SCRIPT) ? "\"" : "",
				scp_name,
				(scp_type!=REXX_SCRIPT) ? "\"" : "",
				args
			);

			/*kprintf("exec_buf: %s",exec_buf);*/

			/*{int i;
			for (i=0; i<strlen(args); i++)
				printf("args[%d] : %d\n",i,args[i]);
			}*/

			/* escape critical chars, end when all chars are found */

			for (i=strlen(exec_buf)-1; i>=0 && escnum>0; --i) {
				if (exec_buf[i]=='>' || exec_buf[i]=='<' || exec_buf[i]=='*') /*|| exec_buf[i]=='"' || exec_buf[i]=='=')*/ {
					strins(&exec_buf[i],"*");
					--escnum;
				}
			}

			{char *p;
			if (p = strchr(exec_buf,'\n'))
				*p = '\0';
			}

			/*kprintf("exec_buf: %s\n",exec_buf);*/

			/*kprintf("start: |%s|\n",exec_buf);*/

#define SYSTEM_MAXLEN 512
/* eigentlich sogar 518 */

#if 0
			if (getenv("cshexeclen")) {
				long elen = atoi(getenv("cshexeclen"));
				if (strlen(exec_buf) > elen) {
					kprintf("csh: command line too long, cutting at %ld\n",elen);
					exec_buf[elen] = '\0';
				}
			}
#endif
			if (strlen(exec_buf) > SYSTEM_MAXLEN) {
				fprintf(stderr,"csh: command line too long (cutting at %ld)\n",SYSTEM_MAXLEN);
				exec_buf[SYSTEM_MAXLEN] = '\0';
			}

			rslt = SystemTags(exec_buf,SYS_Input,(ULONG)Input(),SYS_Output,(ULONG)Output(),TAG_END);
			/*kprintf("  end: return code %ld\n",(LONG)rslt);*/
			free(exec_buf);
		}
		else {
			fprintf(stderr,"%s: we are out of memory for System()\n",scp_name);
			set_returncodes(RETURN_FAIL,ERROR_NO_FREE_STORE);
			rslt = RETURN_FAIL;
		}

		/*clean_up_io();*/
	}

	if (seglist_cmd) {				/* rslt = 0 */
		struct Task *mytask;
		ULONG old_sigalloc;
		BYTE old_pri;

		/*
		 *  This is inspired by Gary Duncan who was inspired by Wshell ;-)
		 *  Save the old signal mask and task priority to see if some
		 *  programs don't deallocate their allocated signals or modify
		 *  the task priority.
		 */

		mytask       = (struct Task *)Myprocess;	/* just for readability.. */
		old_sigalloc = mytask->tc_SigAlloc;		/* hold values before command execution */
		old_pri      = mytask->tc_Node.ln_Pri;		/* hold values before command execution */

		SetIoErr(0);

		if (!SetProgramName(pname))
			SetProgramName("CSH-program");

		if (Mycli->cli_DefaultStack < 1000)
			Mycli->cli_DefaultStack=4000;

		Mycli->cli_Module = seglist_cmd;
		rslt = RunCommand(seglist_cmd,Mycli->cli_DefaultStack*4,args,strlen(args));

		if ((rslt == -1) && (IoErr()==ERROR_NO_FREE_STORE)) {
			fprintf(stderr,"we are out of memory for RunCommand()\n");
			set_returncodes(RETURN_FAIL,ERROR_NO_FREE_STORE);
			rslt = RETURN_FAIL;
			SetProgramName(myname);
			goto MySyncRun_done;
		}

		set_returncodes(rslt,IoErr());

		SetProgramName(myname);

		/*
		 *  now see if naughty command messed things up ...
		 */
		if (old_sigalloc != mytask->tc_SigAlloc) {
			fprintf(stderr, "Warning! Unreleased signal(s); (%08X)\n",
			        old_sigalloc ^ mytask->tc_SigAlloc);
		}
		if (old_pri != mytask->tc_Node.ln_Pri) {
			fprintf(stderr, "Warning! CLI priority changed from %d to %d\n",
			        old_pri, mytask->tc_Node.ln_Pri);
		}
	}

MySyncRun_done:
	if (cmdseg) {
		Forbid();
		if (cmdseg->seg_UC>0)
			cmdseg->seg_UC--;
		Permit();
		Mycli->cli_Module = NULL;
	}

	if (!cmdseg && seglist_cmd) {
		if (Mycli->cli_Module)
			UnLoadSeg(Mycli->cli_Module);
		Mycli->cli_Module = NULL;
		seglist_cmd = NULL;
	}

	if (Myprocess->pr_HomeDir) {
		/*
		char hd[256];
		if (NameFromLock(Myprocess->pr_HomeDir,hd,255))
			printf("homedir set to %s, now unlocking.\n",hd);
		else {
			fprintf(stderr,"csh.MySyncRun: NameFromLock() failed\n");
			printf("homedir's name too long, now unlocking.\n");
		}
		*/
		UnLock(Myprocess->pr_HomeDir);
		Myprocess->pr_HomeDir = NULL;
	}

	Myprocess->pr_HomeDir = myhdir;
	Mycli->cli_Module     = mymod;

	if (pname) free(pname);
	if (ppath) free(ppath);
	if (pdev)  free(pdev);

	/*clean_up_io();*/

/*
	old_out_fh = SelectOutput (Mycli->cli_StandardOutput);
	old_inp_fh = SelectInput  (Mycli->cli_CurrentInput);
*/

	return rslt;
#if 0
	if (done) return rslt;

	char buf2[200];
	args[len]=0;		/* remove trailing '\n' */

	printf("trying old style...\n");

	if( nosync ) {
		sprintf(buf2,"%s %s",com,args);
		Execute(buf2,0,Myprocess->pr_COS);
		return 0;
	}

	if( (ret= SyncRun( com, args, in, out ))>=0 )
		return ret;

	if( ret==PR_NOMEM /* -2L */ ) {
		ierror(NULL,103);
		return 20;
	}

	return -1;
#endif
}


#if 0
int
do_which( char *str )
{
	char *got, *com=av[1];

	if( get_var(LEVEL_ALIAS,com) ) {
		printf("Shell Alias '%s'\n",com);
		return 0;
	}

	if( *(got=find_internal( com ))>1 ) {
		printf("Shell Internal '%s'\n",got);
		return 0;
	}



	printf( "Not found\n" );
	return 20;
}
#endif


char *
dofind( char *cmd, char *ext, char *buf, char *path)
{
	char *ptr, *s=path, *ret=NULL;

	Myprocess->pr_WindowPtr = (APTR)(-1);
	sprintf(buf,"%s%s",cmd,ext);
	if (exists(buf)) {
		ret=buf;
		goto terminate;
	}
	if ((char*)FilePart(buf)==buf) {
		if( *path=='_' )
			s = get_var(LEVEL_SET, path);
		while (s && *s) {
			for (ptr=buf; *s && *s!=','; ) *ptr++ = *s++;
			if( ptr[-1]!=':' && ptr[-1]!='/')
				*ptr++='/';
			sprintf(ptr, "%s%s", cmd, ext);
			if (exists(buf)) {
				ret=buf;
				goto terminate;
			}
			if (*s) s++;
		}
	}
terminate:
	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;
	return ret;
}

