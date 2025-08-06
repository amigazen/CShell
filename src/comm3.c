/*
 * COMM3.C
 *
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 *
 */

#include "shell.h"

/* comm3.c */
static void doassign(char *log, char *phy);
static void assignlist(void);
static int strings_in_file(long mask, char *s, char *path);
static int htype_a_file   (long mask, char *s, char *path);
static void install_menu(char **mav, int mac);
static int line_filter( char *(*line)(char *) );


/* Casting conveniences */
#define BPTR_TO_C(strtag, var)  ((struct strtag *)(BADDR( (ULONG) var)))
#define PROC(task)              ((struct Process *)task)
#define CLI(proc)               (BPTR_TO_C(CommandLineInterface, proc->pr_CLI))


int do_ln(void)
{
	char *linkname;
	long link_type;
	long dest_data;
	BOOL success;

	if( ac!=2 && ac!=3 ) {
		show_usage( NULL );
		return 20;
	}

	if (ac==2)
		linkname = FilePart(av[1]);
	else
		linkname = av[2];

	if (options&1) {
		link_type = LINK_SOFT;
		dest_data = (long)av[1];
	}
	else {
		link_type = LINK_HARD;
		if (!( dest_data = (long)Lock(av[1],ACCESS_READ) )) {
			pError(av[1]);
			return 20;
		}
	}

	success = MakeLink(linkname,dest_data,link_type);

	if (link_type==LINK_HARD)
		UnLock((BPTR)dest_data);

	if (!success) {
		pError(linkname);
		return 20;
	}

	return 0;
}


do_tee( void )
{
	char buf[256];
	FILE *out;

	prepscroll( ac==1 );
	if( ac>2 ) { ierror( av[2], 500 ); return 20; }
	if( ac==1 ) out=stderr;
	else if( !(out=fopen( av[1], "w" ))) { pError( av[1] ); return 20; }
	while (safegets(buf,stdin)) {
		puts(buf);
		quickscroll();
		fprintf(out,"%s\n",buf);
	}
	if( ac!=1 ) fclose( out );
	return 0;
}

do_head( char *garbage, int com )
{
	FILE *f;
	int i, n = 10;
	char buf[256];

	if (ac==1) {					/* use stdin */
		f = stdin;
	}
	else {
		f=fopen(av[1],"r");
		if (f==NULL) {
			if (ac==2 && isnum(av[1])) {	/* use stdin */
				f = stdin;
				n = atol(av[1]);
			}
			else {
				pError(av[1]);
				return 20;
			}
		}
	}

	if (ac>2) {
		if (!isnum(av[2])) {   /* AMK: IoErr() replaced by isnum() */
			ierror(av[2],511);
			return 20;
		}
		n=(int)atol(av[2]);
	}

	if (com) {	/* tail specific part */
		i=0;
		while (fgets(buf, 256, f) && ! dobreak()) i++;
		rewind(f);
		if (n>i) n=i;
		i=i-n;
		while (i-- && fgets(buf, 256, f) && ! dobreak()) ;
	}

	for (i=1; i<=n && fgets(buf, 256, f) && ! dobreak(); i++)
		printf("%s", buf);

	if (f!=stdin)
		fclose(f);

	return 0;
}

#if 0
do_head( char *garbage, int com )
{
	int i, n;
	FILE *f;
	char buf[256];

	if (ac>2) {
		if (!isnum(av[2])) {   /* AMK: IoErr() replaced by isnum() */
			ierror(av[2],511);
			return 20;
		}
		n=(int)atol(av[2]);
	} else n=10;

	f=fopen(av[1], "r");
	if (f==NULL) {
		pError(av[1]);
		return 20;
	}
	if (com) {	/* tail specific part */
		i=0;
		while (fgets(buf, 256, f) && ! dobreak()) i++;
		rewind(f);
		if (n>i) n=i;
		i=i-n;
		while (i-- && fgets(buf, 256, f) && ! dobreak()) ;
	}
	for (i=1; i<=n && fgets(buf, 256, f) && ! dobreak(); i++)
		printf("%s", buf);
	fclose(f);
	return 0;
}
#endif

static int
exword( char **src, char *buf )
{
	*buf=0;
	if( **src==0 ) return 0;
	while( **src && **src!=0xA0 )
		*buf++ = *(*src)++;
	*buf=0;
	if( **src ) (*src)++;
	return 1;
}

static char helpfound=0;

void
man (FILE *f, char *s)
{
	char buf[140], entry[100];
	int  len = sprintf (entry, "    %s", s);
	char *example = "    ";

	prepscroll (0);
	rewind (f);
	do {						/* look for required argument */
		if (fgets (buf, sizeof(buf), f)==NULL || dobreak())
			return;				/* GMD */
	} while (strnicmp (entry, buf, len)) ;
	helpfound = 1;

	do {				/* display help */
		if (dobreak())
			return;
		quickscroll ();
		printf ("%s", buf);
		if (fgets (buf, sizeof(buf), f) == NULL)
			return;
	} while ((!isalphanum (*buf)) && strncmp (buf, example, strlen(example))) ;
}


do_man( void )
{
	FILE *f;
	int i;
	char buf[200], name[60], *src, *var, docfound=0;

	buf[0]=0;
	if( var=get_var(LEVEL_SET,"_man" ) )
		strcpy(buf,var);

	if (ac==1) ac=2, av[1]="MAN";
	for (i=1; i<ac; i++) {
		src=buf, helpfound=0;
		while( exword( &src, name) )
			if( f=fopen(name, "r") ) {
				docfound=1;
				man(f, av[i]);
				fclose(f);
				if( helpfound )
					break;
			}
		if( !docfound )
			fprintf(stderr,"%s not found\n",buf);
		else if( !helpfound )
			fprintf(stderr, "Help not found for %s\n", av[i]);
	}
	return 0;
}



do_assign( void )
{
	int i;

	if     (  ac==1  ) assignlist();
	else if(  ac==2  ) doassign(av[1], NULL);
	else if( !(ac&1) ) ierror(NULL, 500);
	else
		for( i=1; i<ac; i+=2 )
			doassign( av[i],av[i+1] );
	return 0;
}


/* AMK: rewritten code, removed bug when strlen(log) was 0 */
static void
doassign(char *log, char *phy)
{
	int last=strlen(log);

	if (last<2 || log[last-1] != ':') fprintf(stderr, "Bad name %s\n", log);
	else {
		int succ=0;

		log[last-1] = 0;

		if (!phy)
			succ=AssignLock(log,NULL);
		else if (options&1) {   /* add assign, CLI: assign ADD */
			BPTR lock;
			if( lock=Lock(phy,ACCESS_READ) )
				if( !(succ=AssignAdd(log,lock)))
					UnLock(lock);
		}

		/* late-binding assign, CLI: assign DEFER */
		else if (options&2 || options&8)
			succ=AssignLate(log,phy);

		/* non-binding assign, CLI: assign PATH */
		else if (options&4 || options&16)
			succ=AssignPath(log,phy);
		else if (!options) {
			BPTR lock;
			if( lock=Lock(phy,ACCESS_READ) )
				if( !(succ=AssignLock(log,lock)))
					UnLock(lock);
		}

		if( !succ )
			pError( log );
	}
}


static void
assignlist()
{
	char *ptr;
	struct DosList *dl;
	struct AssignList *path;
	ULONG flags;
	char fmt[256],devname[256];
	char **dev_list;
	long dev_num,i,cnt;

	dev_list = NULL;
	dev_num  = 0;
	flags    = LDF_VOLUMES|LDF_READ;

	Myprocess->pr_WindowPtr = (APTR)(-1);

	if (dl=LockDosList(flags)) {
		while (dl=NextDosEntry(dl,flags)) {
			BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
			sprintf(fmt,"%s%s",devname,
			        dl->dol_misc.dol_volume.dol_LockList?"":" [Mounted]");
			add_array_list(&dev_list,&dev_num,fmt);
		}
		UnLockDosList(flags);
	}

	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;

	QuickSort(dev_list,dev_num);
	printf("Volumes:\n");
	for(i=0; i<dev_num; i++)
		printf("%s\n",dev_list[i]);
	free_array_list(dev_list,dev_num);

	if (dobreak())
		return;

	dev_list = NULL;
	dev_num  = 0;
	flags    = LDF_ASSIGNS|LDF_READ;

	Myprocess->pr_WindowPtr = (APTR)(-1);

	if (dl=LockDosList(flags)) {
		while (dl=NextDosEntry(dl,flags)) {
			BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */

			sprintf(fmt,"%-14s ",devname);
			ptr = fmt+strlen(fmt);

			switch (dl->dol_Type) {
				case DLT_DIRECTORY :
					if(dl->dol_Lock) {
						if (!NameFromLock(dl->dol_Lock,ptr,200L)) {
							struct FileLock *fl_tmp = BADDR(dl->dol_Lock);
							/* get volume name of assign to an unmounted volume */
							if (fl_tmp->fl_Volume) {
								struct DosList *dl_tmp = BADDR(fl_tmp->fl_Volume);
								if (dl_tmp->dol_Name) {
									BtoCStr(devname,dl_tmp->dol_Name,254L);  /* 256 - '\0' + ':' */
									sprintf(ptr,"Volume: %s",devname);
								}
								else
									strcpy(ptr,"n/a");
							}
							else
								strcpy(ptr,"n/a");
							/*fprintf(stderr,"csh.assignlist.1: NameFromLock() failed\n");*/
							/*strcpy(ptr,"Lock's name too long");*/
						}
					}
					else
						strcpy(ptr,"Nonexisting lock");
/* --- */
					if (path=dl->dol_misc.dol_assign.dol_List) {
						char **ass_list=NULL;
						long ass_num=0,str_len=0;
						char *ass_str;

						add_array_list(&ass_list,&ass_num,fmt);
						str_len += strlen(fmt);

						for (; path; path=path->al_Next) {
							sprintf(fmt,"%-13s+ ","");
							ptr = fmt+strlen(fmt);
							if (path->al_Lock) {
								if (!NameFromLock(path->al_Lock,ptr,200L)) {
									struct FileLock *fl_tmp = BADDR(path->al_Lock);
									/* get volume name of assign to an unmounted volume */
									if (fl_tmp->fl_Volume) {
										struct DosList *dl_tmp = BADDR(fl_tmp->fl_Volume);
										if (dl_tmp->dol_Name) {
											BtoCStr(devname,dl_tmp->dol_Name,254L);  /* 256 - '\0' + ':' */
											sprintf(ptr,"Volume: %s",devname);
										}
										else
											strcpy(ptr,"n/a");
									}
									else
										strcpy(ptr,"n/a");
									/*fprintf(stderr,"csh.assignlist.2: NameFromLock() failed\n");*/
									/*strcpy(ptr,"Lock's name too long");*/
								}
							}
							else
								strcpy(ptr,"Nonexisting lock");
							add_array_list(&ass_list,&ass_num,fmt);
							str_len += strlen(fmt)+1;
						}

						if (ass_str=malloc(str_len+1)) {
							char *p=ass_str;
							for(i=0; i<ass_num; i++) {
								strcpy(p,ass_list[i]);
								p += strlen(p);
								if ((i+1)<ass_num) {
									*p++ = '\n';
									*p   = '\0';
								}
							}
							add_array_list(&dev_list,&dev_num,ass_str);
							free(ass_str);
						}

						free_array_list(ass_list,ass_num);
					}
					else
						add_array_list(&dev_list,&dev_num,fmt);
/* --- */
#if 0
					add_array_list(&dev_list,&dev_num,fmt);

					for(path=dl->dol_misc.dol_assign.dol_List; path; path=path->al_Next) {
						sprintf(fmt,"%-13s+ ","");
						ptr = fmt+strlen(fmt);
						if (path->al_Lock) {
							if (!NameFromLock(path->al_Lock,ptr,200L)) {
								struct FileLock *fl_tmp = BADDR(path->al_Lock);
								/* get volume name of assign to an unmounted volume */
								if (fl_tmp->fl_Volume) {
									struct DosList *dl_tmp = BADDR(fl_tmp->fl_Volume);
									if (dl_tmp->dol_Name) {
										BtoCStr(devname,dl_tmp->dol_Name,254L);  /* 256 - '\0' + ':' */
										sprintf(ptr,"Volume: %s",devname);
									}
									else
										strcpy(ptr,"n/a");
								}
								else
									strcpy(ptr,"n/a");
								/*fprintf(stderr,"csh.assignlist.3: NameFromLock() failed\n");*/
								/*strcpy(ptr,"Lock's name too long");*/
							}
						}
						else
							strcpy(ptr,"Nonexisting lock");
						add_array_list(&dev_list,&dev_num,fmt);
					}
#endif
					break;
				case DLT_LATE      :
					sprintf(ptr,"<%s>",dl->dol_misc.dol_assign.dol_AssignName);
					add_array_list(&dev_list,&dev_num,fmt);
					break;
				case DLT_NONBINDING:
					sprintf(ptr,"[%s]",dl->dol_misc.dol_assign.dol_AssignName);
					add_array_list(&dev_list,&dev_num,fmt);
					break;
				default:
					strcpy(ptr,"Unknown assign");
					add_array_list(&dev_list,&dev_num,fmt);
					break;
			}
		}
		UnLockDosList(flags);
	}

	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;

	QuickSort(dev_list,dev_num);
	printf("\nDirectories:\n");
	for(i=0; !dobreak() && i<dev_num; i++)
		printf("%s\n",dev_list[i]);
	free_array_list(dev_list,dev_num);

	if (dobreak())
		return;

	dev_list = NULL;
	dev_num  = 0;
	flags    = LDF_DEVICES|LDF_READ;

	Myprocess->pr_WindowPtr = (APTR)(-1);

	if (dl=LockDosList(flags)) {
		while (dl=NextDosEntry(dl,flags)) {
			if (dl->dol_Task) {
				BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
				add_array_list(&dev_list,&dev_num,devname);
			}
		}
		UnLockDosList(flags);
	}

	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;

	QuickSort(dev_list,dev_num);
	printf("\nDevices (with FileSystem):\n");
	for(i=0,cnt=0; i<dev_num; i++) {
		if (IsFileSystem(dev_list[i])) {
			if (cnt>0 && cnt%5==0)
				printf("\n");
			printf("%s ",dev_list[i]);
			++cnt;
		}
	}
	printf("\n");
	free_array_list(dev_list,dev_num);

	if (dobreak())
		return;

	dev_list = NULL;
	dev_num  = 0;
	flags    = LDF_DEVICES|LDF_READ;

	Myprocess->pr_WindowPtr = (APTR)(-1);

	if (dl=LockDosList(flags)) {
		while (dl=NextDosEntry(dl,flags)) {
			BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
			add_array_list(&dev_list,&dev_num,devname);
		}
		UnLockDosList(flags);
	}

	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;

	QuickSort(dev_list,dev_num);
	printf("\nDevices:\n");
	for(i=0; i<dev_num; i++) {
		if (i>0 && i%5==0)
			printf("\n");
		printf("%s ",dev_list[i]);
	}
	printf("\n");
	free_array_list(dev_list,dev_num);

return;
#if 0
	printf("\nDirectories:\n");
	for( ; arr[i] && !dobreak(); i++ ) {
		log=arr[i]+1; ptr=log+strlen(log)+1;
		switch( *(log-1)) {
		case 3:
			printf("%-20s%s\n", log, ptr);
			for(;;) {
				ptr+=strlen(ptr)+1;
				if( !*ptr ) break;
					printf("%-19s+%s\n", "", ptr);
			}
			break;
		case 4: printf("%-20s<%s>\n", log, ptr); break;
		case 5: printf("%-20s[%s]\n", log, ptr); break;
		}
	}
#endif
}



do_join( void )
{
	BPTR sou, dest;
	char *buffer;
	int i;
	long n;
	char *namedest=av[--ac];

	if (options==0 && exists(namedest)) { ierror(namedest,203); return 20; }
	if ( (buffer=malloc(8192)) == NULL ) { ierror(NULL,103); return 20; }
	if ( (dest=Open(namedest, MODE_NEWFILE)) == NULL )
		{ pError(namedest); goto fail1; }
	for (i=1; i<ac; i++) {
		if ( (sou=Open(av[i], MODE_OLDFILE)) == NULL ) pError(av[i]);
		else
			while( (n=Read(sou, buffer, 8192L)) > 0 )
				if (Write(dest, buffer, n) != n)
					{ pError(namedest); Close(sou); goto fail2; }
		Close(sou);
	}
fail2:
	Close(dest);
fail1:
	free(buffer);
	return 0;
}

#define BUFDIM 512L
#define MAXSTR 256

int minstr;

static int
strings_in_file(long mask, char *s, char *path)
{
	long n,i;
	char c;
	char readbuf[BUFDIM+1], strbuf[MAXSTR+1];
	BPTR fh;
	int strctr=0;
	BOOL out, hdout = TRUE;  /* hdout = header output */
	BOOL usestdin = (s==NULL);
	BOOL inter = IsInteractive(Output());

	prepscroll(0);

	if (usestdin)
		fh = Input();
	else
		fh = Open(s,MODE_OLDFILE);

	if (fh) {
		while ( (n=Read(fh, readbuf, BUFDIM))>0 && !CHECKBREAK() )
			for (i=0; i<n && !CHECKBREAK(); i++) {   /* GMD: speed up ^C reaction */
				c=readbuf[i];
				if (c<0x20 || c>0x7f) {
					out=(strctr>=minstr);
					if (!out) strctr=0;
				} else {
					strbuf[strctr++]=c;
					out=(strctr>=BUFDIM);
				}
				if (out) {
					strbuf[strctr]='\0';
					if (options&8 && hdout) {
							printf("Strings in %s (len>=%d):\n",path?path:"STDIN",minstr);
							hdout=FALSE;
					}
					printf("%s%s%s%s%s\n",
						(options&4) ? (path?path:"STDIN") : "",
						(options&4) ? ": " : "",
						(options&2) ? "|" : "",
						strbuf,
						(options&2) ? "|" : "");
#if 0
					if (options&2) {
						printf("%s: |%s|\n",path?path:"STDIN",strbuf);
					}
					else {
						if (hdout) {
							printf("Strings in %s (len>=%d):\n",path?path:"STDIN",minstr);
							hdout=FALSE;
						}
						puts(strbuf);
					}
#endif
					if (inter) fflush(stdout);
					quickscroll();
					strctr=0;
				}
			}
		/* only close file if not standard input */
		if (!usestdin)
			Close(fh);
	} else
		pError(s);

	return 0;
}

do_strings( void )
{
	if (isnum(av[ac-1])) {
		minstr = myatoi(av[--ac],1,255);
		if (atoierr) {
			fprintf(stderr,"Need a valid string length parameter (1-255)!\n");
			return 0;
		}
	}
	else
		minstr = 4;

	if (ac<2)
		strings_in_file(0, NULL, NULL);
	else
		all_args( strings_in_file, 0);

	return 0;
}

BPTR myfile[MAXMYFILES];

do_open( void )
{
	long mode;
	int n;

	switch (toupper(av[2][0])) {
		case 'R': mode=MODE_OLDFILE; break;
		case 'W': mode=MODE_NEWFILE; break;
		default : ierror(NULL,500); return 1;
	}
	n=myatoi(av[3],0,MAXMYFILES-1); if (atoierr) return 20;
	if (myfile[n]) myclose(n);
	myfile[n]=Open(av[1],mode);
	return myfile[n]==NULL;
}

do_close( void )
{
	int i, n;

	if (ac==1)
		for (i=1; i<MAXMYFILES; i++)
			myclose(i);
	for (i=1; i<ac; i++) {
		n=myatoi(av[i],0,MAXMYFILES-1); if (atoierr) return 20;
		myclose(n);
	}
	return 0;
}

void
myclose(int n)
{
	if (myfile[n]) { Close(myfile[n]); myfile[n]=NULL; }
}

do_fileslist( void )
{
	int i, flag=0;

	printf("Open files:");
	for (i=0; i<MAXMYFILES; i++)
		if (myfile[i]) { printf(" %d",i); flag=1; }
	if (!flag) printf(" None!");
	printf("\n");
	return 0;
}

BPTR
extOpen( char *name, long mode )
{
	if (name[0]=='.' && name[1]>='0' && name[1]<='9')
		return myfile[atoi(name+1)];
	return Open(name,mode);
}


void
extClose(BPTR fh)
{
	int i;

	for (i=0; i<MAXMYFILES; i++)
		if (myfile[i]==fh) return;
	Close(fh);
}

do_basename( void )
{
	char *res;
	int  i;

	for( i=2; i<ac; i++ )
		av[i]=FilePart(av[i]);
	set_var(LEVEL_SET, av[1], res=compile_av(av,2,ac,0xA0,0));
	free(res);
	return 0;
}

do_tackon( void )
{
	char buf[256];

	strcpy(buf, av[2]);
	AddPart(buf, av[3], 256L);
	set_var(LEVEL_SET, av[1], buf);
	return 0;
}

extern char shellres[];

do_resident( void )
{
	struct Segment *seg;
	char buf[256],devname[256];
	char **dev_list1=NULL,**dev_list2=NULL;
	long dev_num1=0,dev_num2=0,i;

	if (options==0 && ac>1) options=1;
	switch (options) {
	case 0:
		Forbid();
		seg = (struct Segment *)BADDR(((struct DosInfo *)BADDR(DOSBase->dl_Root->rn_Info))->di_NetHand);
		while (seg) {
			BtoCStr(devname,MKBADDR(seg->seg_Name),254L);  /* 256 - '\0' + ':' */
			if (seg->seg_UC >= 0) {
				sprintf(buf,"%-18s%4.1ld",devname,seg->seg_UC-1);
				add_array_list(&dev_list1,&dev_num1,buf);
			}
			else {
				switch (seg->seg_UC) {
				case CMD_INTERNAL:
					sprintf(buf,"%-18s%s",devname,"INTERNAL");
					add_array_list(&dev_list2,&dev_num2,buf);
					break;
				case CMD_SYSTEM:
/* don't print this */ /*
					sprintf(buf,"%-18s%s",devname,"SYSTEM");
					add_array_list(&dev_list2,&dev_num2,buf);
*/
					break;
				case CMD_DISABLED:
					sprintf(buf,"%-18s%s",devname,"DISABLED");
					add_array_list(&dev_list2,&dev_num2,buf);
					break;
				default:
					sprintf(buf,"%-18s%s",devname,"UNKNOWN");
					add_array_list(&dev_list2,&dev_num2,buf);
					break;
				}
			}
			seg = (struct Segment *)BADDR(seg->seg_Next);
		}
		Permit();
		printf("NAME              USE COUNT\n\n");

		QuickSort(dev_list1,dev_num1);
		for(i=0; !dobreak() && i<dev_num1; i++)
			printf("%s\n",dev_list1[i]);
		free_array_list(dev_list1,dev_num1);

		for(i=0; !dobreak() && i<dev_num2; i++)
			printf("%s\n",dev_list2[i]);
		free_array_list(dev_list2,dev_num2);

		break;
	case 1:
		for (i=1; i<ac; i++)
			if (loadres(av[i]))
				printf("OK! %s is now resident\n", FilePart(av[i]));
			else
				pError(av[i]);
		break;
	case 2:
		for (i=1; i<ac; i++) {
			Forbid();
			if (seg=FindSegment(av[i],NULL,0L)) {
				if (RemSegment(seg)) {
					Permit();
					printf("Removed %s\n",av[i]);
				}
				else {
					Permit();
					ierror(av[i],ERROR_OBJECT_IN_USE);
				}
			}
			else {
				Permit();
				ierror(av[i],ERROR_OBJECT_NOT_FOUND);
			}
		}
		break;
	case 4:
		for (i=1; i<ac; i++) {
			if( !o_resident ) {
				/* AMK: OS20-SetVar replaces ARP-Setenv */
				SetVar(shellres,"1",-1L,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
				o_resident=1;
			}
			sprintf(buf,"res_%s",FilePart(av[i]));
			/* AMK: OS20-SetVar replaces ARP-Setenv */
			SetVar(buf,av[i],-1L,GVF_GLOBAL_ONLY|GVF_BINARY_VAR);
		}
		break;
	default:
		ierror(NULL,500);
		break;
	}
	return 0;
}

int
loadres(char *s)
{
	BPTR seg;
	BOOL okay = FALSE;

	if (seg=NewLoadSeg(s,NULL))
		okay=AddSegment(FilePart(s),seg,1L);

	return (okay);
}

#define FIRSTCOMMAND 4
extern BPTR redir_out, redir_in;
extern int find_command2( char * );

struct TagItem tags[]={
	{SYS_Input,  0},
	{SYS_Output, 0},
	{SYS_Asynch, 1},
	{TAG_DONE,   0} };

do_truerun(char *avline, int backflag)
{
	BPTR input, output;
	char *args;
	char *new_args;
	char *aliastxt;
	int err;

	int cmdnum;
	BOOL execcsh = FALSE;

	if (backflag) {
		input  = Open("NIL:",MODE_NEWFILE);
		output = Open("NIL:",MODE_NEWFILE);
	} else {
		input = redir_in ? redir_in : Open("NIL:",MODE_NEWFILE);
		if (redir_out)
			output = redir_out;
#if 0
		else if (Output() && IsInteractive(Output())) {
			if (((struct FileHandle *)BADDR(Output()))->fh_Port!=DOSFALSE) {
				struct MsgPort *old;
				old = SetConsoleTask(((struct FileHandle *)BADDR(Output()))->fh_Type);
				output = Open("CONSOLE:",MODE_OLDFILE);
				(void)SetConsoleTask(old);
			}
		}
#else
		else if (output = Open("CONSOLE:",MODE_OLDFILE)) {
		}
#endif
		else
			output = Open("NIL:",MODE_NEWFILE);
	}

	cmdnum = find_command2(av[1]);
	/* kprintf("find_command2(av[1]) : %d\n",cmdnum); */

	if (cmdnum >= FIRSTCOMMAND)
		execcsh = TRUE;

	if (aliastxt=get_var(LEVEL_ALIAS,av[1])) {
		execcsh = TRUE;
		/*kprintf("do_truerun: %s is alias = %s\n",av[1],aliastxt);*/
	}

	/* kprintf("run (avline,unparsed): %s\n",avline); */
	args=compile_av(av,1,ac,' ',1);

	tags[0].ti_Data= input;
	tags[1].ti_Data= output;

	/* kprintf("run (args,parsed)    : %s\n",args); */

	/* execcsh ist bei "alias sl search; run sl" ja noch korrekt,
	   aber bei "alias clc calculator; run clc" kommt nur Mist raus */

	if (execcsh) {
		if (new_args = malloc(strlen(args)+strlen("csh -c ")+1)) {
			sprintf(new_args,"csh -c %s",args);
			/* kprintf("run (new_args)       : %s\n",new_args); */
			err=System( new_args, tags );
			free(new_args);
		}
		else
			err = -1;
	}
	else {
		/* kprintf("run (args)           : %s\n",args); */
		err = System( args, tags );
	}

	if( err!=0 ) {
		Close( tags[0].ti_Data );
		Close( tags[1].ti_Data );
		pError(av[1]);
	}
	free(args);

	return 0;
}

#if 0
extern BPTR redir_out, redir_in;

static struct ProcessControlBlock pcb={
	4000,		/* pcb_StackSize	*/
	0,		/* pcb_Pri		*/
	};
/* remaining fields are NULL */

do_truerun(char *avline, int backflag)
{
	char name[100], *args, buf[10];
	int cli;

	args=next_word(next_word(avline));
	if (backflag) {
		pcb.pcb_Control= 0;
		pcb.pcb_Input  = Open("NIL:",MODE_OLDFILE);
		pcb.pcb_Output = Open("NIL:",MODE_OLDFILE);
	} else {
		pcb.pcb_Control= 0;
		pcb.pcb_Input  = redir_in;
		pcb.pcb_Output = redir_out;

	}

	if((cli=ASyncRun(av[1],args,&pcb))<0)
		if (dofind(av[1], "", name,v_path))
			cli=ASyncRun(name,args,&pcb);

	if( cli<0 && cli >= -11 ) {
		if( redir_out ) extClose( redir_out );
		if( redir_in  ) extClose( redir_in  );
	}

	sprintf(buf,"%d",cli);
	set_var(LEVEL_SET,"_newproc",buf);

	if( cli<0 ) {
		ierror(av[1],205);
		return 20;
	}
	return 0;
}
#endif

int
exists( char *name )
{
	BPTR lock;
	char *nam=FilePart(name);
	int ret=0;

	Myprocess->pr_WindowPtr = (APTR)(-1);
	if ( strlen(nam)<=MAXFILENAME && (lock=Lock(name,ACCESS_READ))) {
		UnLock(lock);
		ret=1;
	}
	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;
	return ret;
}

int
mounted( char *dev )
{
	struct DosList *dl;
	ULONG flags = LDF_ALL|LDF_READ;
	int gotcha = FALSE;
	char devname[256];

	if (dl=LockDosList(flags)) {
		while ((!gotcha) && (dl=NextDosEntry(dl,flags))) {
			BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
			if (stricmp(devname,dev)==0)
				gotcha = TRUE;
		}
		UnLockDosList(flags);
	}

	return (gotcha);
}

#define HTYPELINE 16L

static int
htype_a_file(long mask, char *s, char *path)
{
	BPTR fh;
	long n, filesize=0;
	UBYTE buf[HTYPELINE+1];
	char out[80], *put;
	int i, inter=IsInteractive(Output());
	BOOL usestdin = (s==NULL);

	if (usestdin)
		fh = Input();
	else
		fh = Open(s,MODE_OLDFILE);

	if (fh==NULL) { pError(s); return 20; }
	prepscroll(0);
	while ( (n=Read(fh,(char *)buf,HTYPELINE))>0 && !dobreak()) {
		put=out;
		put+=sprintf(put,"%06lx: ",filesize);
		filesize+=n;
		for (i=0; i<n; i++) {
			put+=sprintf( put,(i&3) ? "%02x" : " %02x",buf[i]);
			if ((buf[i]&127)<0x20) buf[i]='.';
		}
		for ( ; i<HTYPELINE; i++) {
			put+=sprintf( put, (i&3) ? "  " : "   ");
			buf[i]=' ';
		}
		buf[i]=0;
		sprintf(put,"    %s",buf);
		puts(out);
		if( inter ) fflush(stdout);
		quickscroll();
	}
	/* only close file if not standard input */
	if (!usestdin)
		Close(fh);
	return 0;
}

do_htype( void )
{
	if (ac<2)
		htype_a_file(0, NULL, NULL);
	else
		all_args( htype_a_file, 0);
	return 0;
}

#define STACK_MIN_FREE 10000
do_stack( void )
{
	long n;

	if (ac>1) {
		if (isnum(av[1])) {   /* AMK: IoErr() replaced by isnum() */
			n=atol(av[1]);
			if ( n < 1600 )
				printf("Requested size too small\n");
			else if ( n > (AvailMem(MEMF_LARGEST)-STACK_MIN_FREE) )
				printf("Requested size too large\n");
			else
			Mycli->cli_DefaultStack=(long)(n >> 2L);
		}
		else {
			ierror(av[1],511);   /* AMK: message if error */
			return 20;
		}
	}
	else {
		if (options&1)
			printf("%ld\n",(long)Mycli->cli_DefaultStack << 2L);
		else
			printf("current stack size is %ld bytes\n",
					(long)Mycli->cli_DefaultStack << 2L);
	}
	return 0;
}

do_fault( void )
{
	PERROR *p;
	int i, n;

	for (i=1; i<ac; i++) {
		n=myatoi(av[i],0,32767);
		if (!atoierr) {
			for (p=Perror; p->errnum && p->errnum!=n; p++);
			if (p->errnum)
				printf("Fault %d: %s\n",n,p->errstr);
			else
				printf("Fault %d not recognized\n",n);
		}
	}
	return 0;
}

struct rpncommand {
	char *str;
	int parsin, parsout;
	};

static struct rpncommand rpn[]={
	"+",	2,	1,
	"-",	2,	1,
	"*",	2,	1,
	"/",	2,	1,
	"%",	2,	1,
	"&",	2,	1,
	"|",	2,	1,
	"~",	1,	1,
	">",	2,	1,
	"<",	2,	1,
	"==",	2,	1,
	"!",	1,	1,
	"MAX",	2,	1,
	"MIN",	2,	1,
	"DUP",	1,	2,
	"DROP",	1,	0,
	"SWAP",	2,	2,
	"HELP",	0,	0,
	NULL,	0,	1,	/* this looks for a number */
};

static long stack[50];
static int sp;


eval_rpn( char **av, int ac, int flag )
{
	char *zero="Division by zero\n";
	struct rpncommand *temp;
	long n0, n1, t;
	int j, i=0, oldsp=sp;

	for (; i<ac; i++) {
		for (j=0; rpn[j].str && stricmp(rpn[j].str,av[i]); j++) ;
		n0=stack[sp-1];
		n1=stack[sp-2];
		sp -= (rpn[j].parsin);
		if (sp<0) { fprintf(stderr, "RPN: Empty stack\n"); goto error; }
		switch (j) {
		  case 0:	n0 += n1;			break;
		  case 1:	n0 = n1-n0;			break;
		  case 2:	n0 *= n1;			break;
		  case 3:	if(n0) n0=n1/n0; else fprintf(stderr,zero); break;
		  case 4:	if(n0) n0=n1%n0; else fprintf(stderr,zero); break;
		  case 5:	n0 &= n1;			break;
		  case 6:	n0 |= n1;			break;
		  case 7:	n0 =  ~n0	;		break;
		  case 8:	n0 = (n1 > n0);		break;
		  case 9:	n0 = (n1 < n0);		break;
		  case 10:	n0 = (n0 == n1);	break;
		  case 11:	n0 = !n0;			break;
		  case 12:	n0=n1>n0 ? n1 : n0;	break;
		  case 13:	n0=n1<n0 ? n1 : n0;	break;
		  case 14:	n1=n0;				break;
		  case 15:	t=n0; n0=n1; n1=t;	break;
		  case 16:						break;
		  case 17:	printf("In Commands Out\n");
			for (temp=rpn; temp->str; temp++)
			    printf(" %d %-10s%d\n",
				temp->parsin,temp->str,temp->parsout);
			break;
		  default:	n0=atol(av[i]);
				if (!isnum(av[i])) {   /* AMK: IoErr() replaced by isnum() */
					fprintf(stderr, "Bad RPN cmd: %s\n",av[i]);
					goto error;
				}
				break;
		  }
		stack[sp]=n0;
		stack[sp+1]=n1;
		sp += rpn[j].parsout;
	}
	if( flag && sp-1)
		fprintf( 
		  stderr,
		  "RPN: Stack not empty\n"
		);

	t=sp; sp=oldsp;
	if( flag )
		return stack[t-1]; /* return top value */
	else
		return t-sp;

error:
	sp=oldsp;
	return 0;
}


do_rpn(char *garbage,int ifflag) /* ifflag!=0 if called from if */
{
	int i=1;
	long t;

	t=eval_rpn( av+i, ac-i, ifflag );
	if (ifflag) return t;              /* called from if: return top value */
	for (i=sp+t-1;i>=sp;i--) printf("%ld\n", stack[i]);/* else print stack */

	return t ? 0 : 20;
}



void *DosAllocMem(long size)
{
	return( AllocVec(size,MEMF_CLEAR|MEMF_PUBLIC) );
}



void DosFreeMem(void *block)
{
	FreeVec(block);
}



/* do_rehash related stuff ... */

#define PATHHASH_RSRC "CSH-PathHash"
#define PATHHASH_SEMA "CSH-PathHash"

struct PathHashRsrc {
	struct Library phr_Lib;
	struct SignalSemaphore phr_Sema;
	long phr_entries;
	char *phr_data;
};



char *get_rehash_prog(char *last,char *progname)
{
	long i;
	char *filepart;

	if (last) {
		for(i=0; i<prghash_num && prghash_list[i]!=last; i++)
			;

		for(++i; i<prghash_num; i++) {
			if (filepart=FilePart(prghash_list[i])) {
				if (strnicmp(progname,filepart,strlen(progname))==0)
					return( prghash_list[i] );
			}
		}
	}

	for(i=0; i<prghash_num; i++) {
		if (filepart=FilePart(prghash_list[i])) {
			if (strnicmp(progname,filepart,strlen(progname))==0)
				return( prghash_list[i] );
		}
	}

	return( NULL );
}



void resolve_multiassign(char *ma,char ***dev_list,long *dev_num)
{
	struct DosList *dl;
	struct AssignList *path;
	ULONG flags;
	char *colon;
	char fmt[256],devname[256];

	flags = LDF_ASSIGNS|LDF_READ;
	colon = strchr(ma,':');

	if (colon && (dl=LockDosList(flags))) {
		*colon = '\0';
		while (dl=NextDosEntry(dl,flags)) {
			BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */

			if (stricmp(devname,ma)==0) {

				if (dl->dol_Type == DLT_DIRECTORY) {
					if(dl->dol_Lock) {
						if (NameFromLock(dl->dol_Lock,fmt,255L)) {
							if (*(colon+1))
								AddPart(fmt,colon+1,255);
							add_array_list(dev_list,dev_num,fmt);
						}
						else {
							fprintf(stderr,"csh.resolve_multiassign.1: NameFromLock() failed\n");
						}
					}
					else
						printf("Nonexisting lock (1) %s\n",devname);

					if (path=dl->dol_misc.dol_assign.dol_List) {
						for (; path; path=path->al_Next) {
							if (dl->dol_Lock) {
								if (NameFromLock(path->al_Lock,fmt,255L)) {
									if (*(colon+1))
										AddPart(fmt,colon+1,255);
									add_array_list(dev_list,dev_num,fmt);
								}
								else {
									fprintf(stderr,"csh.resolve_multiassign.2: NameFromLock() failed\n");
								}
							}
							else
								printf("Nonexisting lock (2) %s\n",devname);
						}
					}
				}
			}
		}
		UnLockDosList(flags);
		*colon = ':';
	}
}



void remove_local_pathhash(void)
{
	free_array_list(prghash_list,prghash_num);
	prghash_list = NULL;
	prghash_num  = 0;
}



BOOL remove_global_pathhash(void)
{
	struct PathHashRsrc *rsrc;
	BOOL success = FALSE;

	Forbid();
	if (rsrc = OpenResource(PATHHASH_RSRC)) {
		if (rsrc->phr_Lib.lib_OpenCnt == 0) {
			RemResource(rsrc);
			RemSemaphore(&rsrc->phr_Sema);
			FreeVec(rsrc->phr_Sema.ss_Link.ln_Name);
			FreeVec(rsrc->phr_Lib.lib_Node.ln_Name);
			if (rsrc->phr_data) FreeVec(rsrc->phr_data);
			FreeVec(rsrc);
			success = TRUE;
		}
	}
	Permit();

	return(success);
}



void load_pathhash_from_mem(struct PathHashRsrc *rsrc)
{
	char *cp;
	long i;

	remove_local_pathhash();

	cp = rsrc->phr_data;

	for (i=rsrc->phr_entries; i>0; i--) {
		add_array_list(&prghash_list,&prghash_num,cp);
		cp += strlen(cp)+1;
	}
}



void save_pathhash_to_mem(struct PathHashRsrc *rsrc,long hashlen)
{
	char *new_data;

	if (new_data = AllocVec(hashlen,MEMF_PUBLIC|MEMF_CLEAR)) {
		long slen,i;

		/* free old path hash data */
		if (rsrc->phr_data) FreeVec(rsrc->phr_data);

		/* set new memory pointer */
		rsrc->phr_data = new_data;

		/* copy path hash data to memory */
		for(i=0; i<prghash_num; i++) {
			slen = strlen(prghash_list[i])+1;
			CopyMem(prghash_list[i],new_data,slen);
			new_data += slen;
		}

		/* set new number of entries */
		rsrc->phr_entries = prghash_num;
	}
}



do_rehash( void )
{
	/*
	        (re-)build internal hash-list        (no option)
	   -c : clear hash-list from memory          (options&1)
	   -l : load hash-list from disk to memory   (options&2)
	   -o : output hash-list from memory         (options&4)
	   -s : save hash-list from memory to disk   (options&8)
	   -g : free global hash-list                (options&16)
	*/
	struct PathHashRsrc *rsrc;
	long i;
	char buf[256];

	if (options&1) {	/* clear hash-list */

		remove_local_pathhash();
	}
	else if (options&2) {	/* load hash-list */
		FILE *fp;
		char *lf,*fname;
		BOOL readin = FALSE;

		if (!(fname=get_var(LEVEL_SET,v_prghash))) {
			fprintf(stderr,"$_prghash unset\n");
			return 20;
		}

		Forbid();
		if (!(rsrc = OpenResource(PATHHASH_RSRC))) {
			if (rsrc=AllocVec(sizeof(struct PathHashRsrc),MEMF_PUBLIC|MEMF_CLEAR)) {
				rsrc->phr_Lib.lib_Node.ln_Type = NT_RESOURCE;
				rsrc->phr_Lib.lib_Node.ln_Pri  = 0;
				if (rsrc->phr_Lib.lib_Node.ln_Name=AllocVec(strlen(PATHHASH_RSRC)+1,MEMF_PUBLIC|MEMF_CLEAR)) {
					strcpy(rsrc->phr_Lib.lib_Node.ln_Name,PATHHASH_RSRC);
					if (rsrc->phr_Sema.ss_Link.ln_Name=AllocVec(strlen(PATHHASH_SEMA)+1,MEMF_PUBLIC|MEMF_CLEAR)) {
						strcpy(rsrc->phr_Sema.ss_Link.ln_Name,PATHHASH_SEMA);
						rsrc->phr_Sema.ss_Link.ln_Pri = 0;
						AddSemaphore(&rsrc->phr_Sema);
						AddResource(rsrc);
						readin = TRUE;
					}
					else {
						FreeVec(rsrc->phr_Lib.lib_Node.ln_Name);
						FreeVec(rsrc);
					}
				}
				else {
					FreeVec(rsrc);
				}
			}
		}
		if (rsrc) {
			rsrc->phr_Lib.lib_OpenCnt++;
			if (readin)
				ObtainSemaphore(&rsrc->phr_Sema);
			else
				ObtainSemaphoreShared(&rsrc->phr_Sema);
		}
		Permit();

		if (!rsrc) {
			printf("csh: cannot create resource\n");
			return 10;
		}

		/* clear old list -- if any */
		remove_local_pathhash();

		if (readin) {
			long hashlen = 0;
			if (fp=fopen(fname,"r")) {
				while (fgets(buf,255,fp)) {
					if (lf=strchr(buf,'\n')) {
						*lf = '\0';
					}
					add_array_list(&prghash_list,&prghash_num,buf);
					hashlen += strlen(buf)+1;
				}
				fclose(fp);
			}
			else
				pError(fname);
			save_pathhash_to_mem(rsrc,hashlen);
		}
		else {
			load_pathhash_from_mem(rsrc);
		}

		Forbid();
		ReleaseSemaphore(&rsrc->phr_Sema);
		rsrc->phr_Lib.lib_OpenCnt--;
		Permit();
	}
	else if (options&4) {	/* output hash-list */

		for(i=0; !dobreak() && i<prghash_num; i++)
			printf("%s\n",prghash_list[i]);
	}
	else if (options&8) {	/* save hash-list */
		FILE *fp;
		char *fname;

		if (!(fname=get_var(LEVEL_SET,v_prghash))) {
			fprintf(stderr,"$_prghash unset\n");
			return 20;
		}

		if (fp=fopen(fname,"w")) {
			for(i=0; i<prghash_num; i++)
				fprintf(fp,"%s\n",prghash_list[i]);
			fclose(fp);
		}
		else
			pError(fname);

	}
	else if (options&16) {	/* free global hash-list */

		if (!remove_global_pathhash())
			printf("csh: cannot clear, currently in use or does not exist\n");
	}
	else {
		BPTR lock;
		struct PathList *pl;
		struct FileInfoBlock *fib;
		char **path_list = NULL;
		long path_num = 0;
		long hashlen = 0;
		long slen;

		/* clear old list -- if any */
		remove_local_pathhash();

		Forbid();
		pl = (struct PathList *)BADDR(Mycli->cli_CommandDir);
		while (pl) {
			if (pl->pl_PathLock) {
				if (NameFromLock(pl->pl_PathLock,buf,255L))
					add_array_list(&path_list,&path_num,buf);
				else
					fprintf(stderr,"csh.do_rehash: NameFromLock() failed\n");
			}
			pl = (struct PathList *)BADDR(pl->pl_NextPath);
		}
		Permit();

		resolve_multiassign("C:",&path_list,&path_num);

		if (fib=AllocVec(sizeof(struct FileInfoBlock),MEMF_PUBLIC|MEMF_CLEAR)) {
			for(i=0; !dobreak() && i<path_num; i++) {
				if (lock=Lock(path_list[i],SHARED_LOCK)) {
					if (Examine(lock,fib)) {
						while (!dobreak() && ExNext(lock,fib)) {
							if (fib->fib_DirEntryType<0 && (!(fib->fib_Protection&FIBF_EXECUTE) || fib->fib_Protection&FIBF_SCRIPT)) {
								slen = strlen(fib->fib_FileName);
								if (slen<5 || stricmp(fib->fib_FileName+slen-5,".info")!=0) {
									strcpy(buf,path_list[i]);
									if (AddPart(buf,fib->fib_FileName,255)) {
										add_array_list(&prghash_list,&prghash_num,buf);
										hashlen += strlen(buf)+1;
									}
									else {
										printf("path too long: %s -> %s\n",path_list[i],fib->fib_FileName);
									}
								}
							}
						}
					}
					UnLock(lock);
				}
			}
			FreeVec(fib);
		}

		free_array_list(path_list,path_num);

		Forbid();
		if (!(rsrc = OpenResource(PATHHASH_RSRC))) {
			if (rsrc=AllocVec(sizeof(struct PathHashRsrc),MEMF_PUBLIC|MEMF_CLEAR)) {
				rsrc->phr_Lib.lib_Node.ln_Type = NT_RESOURCE;
				rsrc->phr_Lib.lib_Node.ln_Pri  = 0;
				if (rsrc->phr_Lib.lib_Node.ln_Name=AllocVec(strlen(PATHHASH_RSRC)+1,MEMF_PUBLIC|MEMF_CLEAR)) {
					strcpy(rsrc->phr_Lib.lib_Node.ln_Name,PATHHASH_RSRC);
					if (rsrc->phr_Sema.ss_Link.ln_Name=AllocVec(strlen(PATHHASH_SEMA)+1,MEMF_PUBLIC|MEMF_CLEAR)) {
						strcpy(rsrc->phr_Sema.ss_Link.ln_Name,PATHHASH_SEMA);
						rsrc->phr_Sema.ss_Link.ln_Pri = 0;
						AddSemaphore(&rsrc->phr_Sema);
						AddResource(rsrc);
					}
					else {
						FreeVec(rsrc->phr_Lib.lib_Node.ln_Name);
						FreeVec(rsrc);
					}
				}
				else {
					FreeVec(rsrc);
				}
			}
		}
		if (rsrc) {
			rsrc->phr_Lib.lib_OpenCnt++;
			ObtainSemaphore(&rsrc->phr_Sema);
		}
		Permit();

		if (!rsrc) {
			printf("csh: cannot create resource\n");
			return 10;
		}

		save_pathhash_to_mem(rsrc,hashlen);

		Forbid();
		ReleaseSemaphore(&rsrc->phr_Sema);
		rsrc->phr_Lib.lib_OpenCnt--;
		Permit();
	}

	return 0;
}



do_path( void )
{
	struct Process *proc;
	BPTR lock,dup_lock;
	long mycli,clinum,mincli,maxcli;
	struct PathList *pl,*new_pl,*old,*last;
	char buf[256];
	int i;

	if ( options&1 ) {
		long noram = 0;
		Forbid();
		mincli = 1;
		maxcli = MaxCli();
		mycli  = Myprocess->pr_TaskNum;
		if (!(options&2))
			mincli = maxcli = mycli;
		for (clinum = mincli; clinum<=maxcli; clinum++) {
			if (proc=FindCliProc(clinum)) {
				pl = (struct PathList *)BADDR(CLI(proc)->cli_CommandDir);
				while (pl) {
					if (pl->pl_PathLock) {
						UnLock(pl->pl_PathLock);
					}
					old = pl;
					pl = (struct PathList *)BADDR(pl->pl_NextPath);
					if (TypeOfMem(old) && ((ULONG)old & 4L))
						DosFreeMem( old );
					else
						++noram;
				}
				CLI(proc)->cli_CommandDir=NULL;
			}
		}
		Permit();
		if (noram>0) {
			fprintf(stderr,"%ld structure%s (%ld bytes) not freed, memory isn't in known RAM\n",
					noram,
					(noram>1) ? "s":"",
					noram*sizeof(struct PathList));
		}
	} else if( ac==1 ) {
		char **dev_list=NULL;
		long dev_num=0,i;
		puts("Current dir");
		Forbid();
		pl = (struct PathList *)BADDR(Mycli->cli_CommandDir);
		while (pl) {
			if (pl->pl_PathLock) {
				if (NameFromLock(pl->pl_PathLock,buf,255L))
					add_array_list(&dev_list,&dev_num,buf);
				else
					fprintf(stderr,"csh.do_path: NameFromLock() failed\n");
			}
			pl = (struct PathList *)BADDR(pl->pl_NextPath);
		}
		Permit();

		for(i=0; !dobreak() && i<dev_num; i++)
			printf("%s\n",dev_list[i]);
		free_array_list(dev_list,dev_num);

		if (!dobreak())
			puts("C:");
		return 0;
	}
	for( i=1; i<ac; i++ ) {
		if( !(lock=Lock(av[i],ACCESS_READ)) ) {
			ierror(av[i],205);
			continue;
		}
		if( !isdir(av[i])) {
			ierror(av[i],212);
			UnLock(lock);
			continue;
		}
		Forbid();
		mincli = 1;
		maxcli = MaxCli();
		mycli  = Myprocess->pr_TaskNum;
		if (!(options&2))
			mincli = maxcli = mycli;
		for (clinum = mincli; clinum<=maxcli; clinum++) {
			if (proc=FindCliProc(clinum)) {
				if (options&2)
					dup_lock = DupLock(lock);
				else
					dup_lock = lock;
				last = pl = (struct PathList *)BADDR(CLI(proc)->cli_CommandDir);
				while (pl && dup_lock) {
					last = pl;
					if (pl->pl_PathLock) {
						if (SameLock(pl->pl_PathLock,dup_lock)==LOCK_SAME) {
							UnLock(dup_lock);
							dup_lock=NULL;
						}
					}
					pl = (struct PathList *)BADDR(pl->pl_NextPath);
				}
				if (dup_lock) {
					if (new_pl=DosAllocMem( sizeof(struct PathList) )) {
						new_pl->pl_NextPath = NULL;
						new_pl->pl_PathLock = dup_lock;
						if (last)
							last->pl_NextPath = MKBADDR(new_pl);
						else
							CLI(proc)->cli_CommandDir = MKBADDR(new_pl);
					}
					else
						UnLock(dup_lock);
				}
			}
		}
		Permit();
		if (options&2)
			UnLock(lock);
	}
	return 0;
}

do_pri( void )
{
	int t, pri;
	struct Process *proc;

	t=(LONG)MaxCli();
	t=myatoi(av[1],0,t); if (atoierr) return 20;
	pri=myatoi(av[2],-128,127); if (atoierr) return 20;
	Forbid();
	proc=(t==0 ? Myprocess : FindCliProc((LONG)t));
	if (proc==NULL) fprintf(stderr, "process not found\n");
	else SetTaskPri((struct Task *)proc, (long)pri);
	Permit();
	return 0;
}

do_strleft( void )
{
	int n;

	n=posatoi(av[3]); if (atoierr) return 20;
	set_var_n(LEVEL_SET, av[1], av[2], n);
	return 0;
}

do_strright( void )
{
	int n, len=strlen(av[2]);

	n=posatoi(av[3]); if (atoierr) return 20;
	if( n>len ) n=len;
	set_var(LEVEL_SET, av[1], av[2]+len-n );
	return 0;
}

do_strmid( void )
{
	int n1, n2=999999, len=strlen(av[2]);

	n1=myatoi(av[3],1,999999)-1; if (atoierr) return 20;
	if (n1>len) n1=len;
	if (ac>4) {
		n2=posatoi(av[4]); if (atoierr) return 20;
	}
	set_var_n(LEVEL_SET, av[1], av[2]+n1, n2);
	return 0;
}

do_strlen( void )
{
	char buf[16];

	sprintf(buf,"%d",strlen(av[2]));
	set_var(LEVEL_SET, av[1], buf);
	return 0;
}

int atoierr;

myatoi(char *s,int mmin,int mmax)
{
	int n;

	atoierr=0;   /* GMD: initialise external flag! */
	n=atol(s);

	if (!isnum(s)) {   /* AMK: IoErr() replaced by isnum() */
		atoierr=1;
		ierror(s,511);
	}
	else if (n<mmin || n>mmax) {
		/*fprintf( stderr, "%s(%d) not in (%d,%d)\n",s,n,mmin,mmax );*/
		fprintf( stderr, "%s not in (%d,%d)\n",s,mmin,mmax );
		atoierr=1; n=mmin;
	}
	return n;
}

unlatoi(char *s)
{
	int n=atol(s);
	atoierr=0;
	if (!isnum(s)) {   /* AMK: IoErr() replaced by isnum() */
		atoierr=1;
		ierror(s,511);
		n=0;
	}
	return n;
}

posatoi(char *s)
{
	int n=atol(s);
	atoierr=0;
	if (!isnum(s)) {   /* AMK: IoErr() replaced by isnum() */
		atoierr=1;
		ierror(s,511);
		n=0;
	}
	else if (n<0 )
		atoierr=1, n=0, fprintf( stderr, "%s must be positive\n",s );
	return n;
}


do_fltlower( void )
{
	return line_filter( strlwr );
}

do_fltupper( void )
{
	return line_filter( strupr );
}

#if 0
char *
stripcr( char *get )
{
	char *old=get, *put;

	for( put=get; *get; get++ )
		if( *get!=13 )
			*put++ = *get;
	*put++=0;
	return old;
}

do_fltstripcr( void )
{
	return line_filter( stripcr );
}
#endif

static int
line_filter( char *(*func)( char * ) )
{
	char buf[256];

	while (!CHECKBREAK() && myfgets(buf,stdin))
		puts((*func)(buf));
	return 0;
}

int
do_linecnt( void )
{
	int count=0;
	char buf[256];

	while (!CHECKBREAK() && fgets(buf,255,stdin)) ++count;
	printf("%d lines\n",count);
	return 0;
}

int
do_uniq( void )
{
	int firstline=1;
	char buf[256], oldbuf[256];

	while (!CHECKBREAK() && myfgets(buf,stdin)) {
		if ( firstline || strcmp(buf, oldbuf)) {
			strcpy(oldbuf, buf);
			puts(buf);
		}
		firstline=0;
	}
	return 0;
}


#define RXFB_RESULT  17
#define RXCOMM    0x01000000

static struct rexxmsg {
	struct Message rm_Node;             /* EXEC message structure        */
	APTR     rm_TaskBlock;              /* global structure (private)    */
	APTR     rm_LibBase;                /* library base (private)        */
	LONG     rm_Action;                 /* command (action) code         */
	LONG     rm_Result1;                /* primary result (return code)  */
	LONG     rm_Result2;                /* secondary result              */
	STRPTR   rm_Args[16];               /* argument block (ARG0-ARG15)   */

	struct MsgPort *rm_PassPort;        /* forwarding port               */
	STRPTR   rm_CommAddr;               /* host address (port name)      */
	STRPTR   rm_FileExt;                /* file extension                */
	LONG     rm_Stdin;                  /* input stream (filehandle)     */
	LONG     rm_Stdout;                 /* output stream (filehandle)    */
	LONG     rm_avail;                  /* future expansion              */
} mymsg;                                /* size: 128 bytes               */


do_rxsend( char *avline )
{
	int i, ret=0;
	long result;
	struct MsgPort *port, *reply;
	long len;
	char buf[20], *resptr;

	if (!(port = (struct MsgPort *)FindPort(av[1])))
		{ fprintf(stderr, "No port %s!\n", av[1]); return 20; }
	mymsg.rm_Node.mn_Node.ln_Type = NT_MESSAGE;
	mymsg.rm_Node.mn_Length = sizeof(struct rexxmsg);
	mymsg.rm_Action = RXCOMM | (options&1 ? 1L << RXFB_RESULT : 0);
	if (!(reply = CreateMsgPort())) {
		fprintf(stderr, "No reply port\n");
		return 20;
	}
	mymsg.rm_Node.mn_ReplyPort = reply;

	if( options&2 )
		av[2]=compile_av( av,2,ac,' ',0), ac=3;
	for ( i=2; i<ac; i++) {
		mymsg.rm_Args[0] = av[i];
		mymsg.rm_Result2 = 0;        /* clear out the last result. */
		PutMsg(port, &mymsg.rm_Node);

		Wait( 1<<reply->mp_SigBit | SIGBREAKF_CTRL_C );

		if( CHECKBREAK() ) {
			ret=5;
			break;
		}

		if (options&1) {
			if( (result=mymsg.rm_Result2)<1000000 ) { /* like AREXX */
				sprintf(buf,"%d",result);              
				set_var(LEVEL_SET,v_result,buf);
			} else {
				resptr=(char *)(result-4);
				len = *(long *)resptr;
				memmove(resptr,resptr+4,len);  /* Null terminate */
				resptr[len]=0;      
				set_var(LEVEL_SET,v_result,resptr);
				FreeMem(resptr, len+4 );
			}
		} else 
			unset_var( LEVEL_SET, v_result );
	}
	if( options&2 )
		free( av[2] );

	if (reply) DeleteMsgPort(reply);
	return ret;
}

static char *rxreturn;

do_rxrec( void )
{
	struct MsgPort *port;
	struct rexxmsg *msg;
	char *portname, *str;

	if (ac > 1)
		portname=av[1];
	else
		portname="rexx_csh";

	if (!(port=CreateMsgPort())) {
		fprintf(stderr, "Can't have MsgPort %s\n", portname);
		return 20;
	}
	port->mp_Node.ln_Name = portname;
	port->mp_Node.ln_Pri  = 0;
	AddPort(port);
	for (;;) {
		WaitPort(port);
		while (msg=(struct rexxmsg *)GetMsg(port)) {
			if ( ! stricmp(msg->rm_Args[0], "bye")) {
				ReplyMsg((struct Message *)msg);
				RemPort(port);
				DeleteMsgPort(port);
				return 0;
			}
			rxreturn=NULL;
			exec_command(msg->rm_Args[0]);
			if (msg->rm_Action & (1L << RXFB_RESULT)) {
				if( rxreturn ) {
					str= SAllocMem( strlen( rxreturn )+5 , 0 );
					*(long *)str=strlen( rxreturn );
					strcpy( str+4, rxreturn );
					msg->rm_Result2=(long)str;
				} else {
					str = get_var(LEVEL_SET, v_lasterr);
					msg->rm_Result2=(str) ? atoi(str) : 20;
				}
			}
			ReplyMsg((struct Message *)msg);
		}
	}
}

int
do_waitport( void )
{
	int count=4*10;
	struct MsgPort *port=NULL;

	if( ac==3 ) 
		{ count=2*myatoi(av[2],0, 32000); if( atoierr ) return 20; }

	while( --count>=0 && !(port=FindPort(av[1])) && !dobreak() )
		Delay(12);

	return port ? 0 : 20;
}

int
do_rxreturn( void )
{
	rxreturn=compile_av( av, 1, ac, ' ', 1 );
	return 0;
}

do_ascii( void )
{
	int x=1, y, c, c1, t;
	char *fmt1=" %3d %c%c |", *fmt2=" %4d";

	if( options&1 ) fmt1=" %3o %c%c |", fmt2="%4o";
	if( options&2 ) fmt1=" %3x %c%c |", fmt2="%4x";
	if( ac==x )
		for( y=0; y<32 && !dobreak(); y++ ) {
			printf("|");
			for( x=0; x<8; x++ ) {
				c1=c=y+32*x; t=' ';
				if( c<32 ) t='^', c1+=64;
				printf(fmt1,c, t, c1<128 || c1>=160?c1:'.');
			}
			printf("\n");
		}
	else 
		for( ; x<ac && !dobreak(); x++ ) {
			for( y=0; y<strlen(av[x]); y++ )
				printf(fmt2,av[x][y]);
			printf("\n");
		}
	return 0;
}

void
appendslash( char *path )
{
	int c;

	if( (c=path[strlen(path)-1]) !='/' && c!=':' )
		strcat(path,"/");
}

static void
whereis( char *path, char *file )
{
	char **eav, buf[100];
	int  eac, j;

	buf[0]=0;
	if( path ) {
		strcpy(buf,path);
		appendslash(buf);
	}
	strcat(buf,".../");
	strcat(buf,file);
	if( !index( file, '*' ) && !index( file, '?') )
		strcat(buf,"*");
	if(eav=expand(buf,&eac)) {
		for( j=0; j<eac && !dobreak(); j++ )
			printf("%s\n",eav[j]);
		free_expand(eav);
	}
}

do_whereis( void )
{
	char buf[200], *prev, *devs;
	int i;

	if( index( av[1],':') || index( av[1],'/' ) )
		{ fprintf(stderr,"No paths please\n"); return 20; };

	if( options&1 ) {
		Myprocess->pr_WindowPtr = (APTR)(-1);
		get_drives( devs=buf );
		do {
			prev=devs; devs=index(devs,0xA0);
			if( devs ) *devs++=0; 
			whereis( prev, av[1] );
		} while( devs );
		Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;
	} else if( ac==2 ) {
		whereis( NULL, av[1] );
	} else {
		for( i=2; i<ac; i++ ) {
			strcpy(buf,av[i]);
			appendslash( buf );
			whereis( buf, av[1] );
		}
	}
	return 0;
}

do_usage( void )
{
	int i;

	if( ac==1 ) {
		printf("Usage: usage [command...command]\n");
		printf("[ ]=option   [ | ]=choice   { }=repetition   name...name=1 or more names\n");
	} else
		for( i=1; i<ac; i++ )
			show_usage( av[i] );
	return 0;
}



/* these defines are new in OS 3.x */
#ifndef WA_NewLookMenus
#define WA_NewLookMenus (WA_Dummy + 0x30)
#endif
#ifndef GTMN_NewLookMenus
#define GTMN_NewLookMenus GT_TagBase+67
#endif
#ifndef WFLG_NEWLOOKMENUS
#define WFLG_NEWLOOKMENUS 0x00200000
#endif



static struct NewMenu *NewMenus = NULL;
static int AnzMenus = 0, AnzItems = 1;
static APTR *visualInfo = NULL;
static struct Menu *menuStrip = NULL;
static char *fontName = NULL;
static struct TextAttr textAttr;
static struct TextFont *textFont = NULL;

char *MenuCommand[MAXMENUS][MAXMENUITEMS];

do_menu( void )
{
	if( o_nowindow || !Mywindow )
		return 5;

	if( options&1 )
		remove_menu();

	if( ac==2 )
		show_usage( NULL );
	else if( AnzMenus<MAXMENUS && ac!=1)
		install_menu( av+1, ac-1 );

	set_menu();
	return 0;
}

static void
install_menu( char *mav[], int mac )
{
	char *p, *com;
	long msize;
	struct NewMenu *new_NewMenus;
	int i;

	if (o_nowindow || !Mywindow)
		return;

	/* mac holds number of items, including title which counts as item */

	if (mac>MAXMENUITEMS)
		mac=MAXMENUITEMS;

	ClearMenuStrip( Mywindow );
	if (menuStrip) FreeMenus(menuStrip);
	if (visualInfo) FreeVisualInfo(visualInfo);
	Delay(3);

	msize = sizeof(struct NewMenu) * (mac+1 + AnzItems-1);	/* one extra 'end' item */
	new_NewMenus = salloc(msize);
	memset(new_NewMenus,0,msize);
	if (NewMenus) {
		memcpy(new_NewMenus,NewMenus,sizeof(struct NewMenu)*(AnzItems-1));
		free(NewMenus);
	}
	else {
		int i,j;
		for (i=0; i<MAXMENUS; i++)
			for (j=0; j<MAXMENUITEMS; j++)
				MenuCommand[i][j] = NULL;
	}
	NewMenus = new_NewMenus;

	NewMenus[AnzItems-1].nm_Type  = NM_TITLE;
	NewMenus[AnzItems-1].nm_Label = strcpy(salloc(strlen(mav[0])+1),mav[0]);

	for( i=1; i<mac; i++) {
		com=NULL;
		if( p=index(mav[i],',')) {
			*p=0; com = ++p;
			if( p=index(com,',')) {
				*p=0;
				NewMenus[AnzItems].nm_CommKey = strdup(++p);
			}
		}

		if( !com || !*com) {
			com=strcpy(salloc(strlen(mav[i])+2),mav[i]);
			MenuCommand[AnzMenus][i-1]=com;
			com+=strlen(com);
			*com++=13;
			*com=0;
		} else {
			MenuCommand[AnzMenus][i-1]=strcpy(salloc(strlen(com)+1),com);
		}

		NewMenus[AnzItems].nm_Type  = NM_ITEM;
		NewMenus[AnzItems].nm_Label = strcpy(salloc(strlen(mav[i])+2),mav[i]);

		AnzItems++;
	}

	AnzMenus++;
	NewMenus[AnzItems++].nm_Type = NM_END;

	if (!(visualInfo=GetVisualInfo(Mywindow->WScreen,TAG_END))) {
		printf("no visual info\n");
		free(NewMenus);
	}

	if (!(menuStrip=CreateMenus(NewMenus,TAG_END))) {
		printf("no create menus\n");
		FreeVisualInfo(visualInfo);
		free(NewMenus);
	}

	/* do we want a monospaced (non-proportional) font? */
	if (options&2 && !textFont && AnzMenus<=1) {
		Forbid();
		if (GfxBase->DefaultFont) {
			textAttr.ta_Name = fontName = strdup(GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name);
			textAttr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
		}
		else {
			textAttr.ta_Name  = "topaz.font";
			textAttr.ta_YSize = 8;
		}
		Permit();
		textAttr.ta_Style = FS_NORMAL;
		textAttr.ta_Flags = 0;
		if (textAttr.ta_Name)
			textFont = OpenDiskFont(&textAttr);
	}

	/*
	 *  "CON:" windows don't have the WFLG_NEWLOOKMENUS flag set,
	 *  so that NewLook menus doesn't work :-(
	 */

#if 0
	if (Mywindow->Flags & WFLG_NEWLOOKMENUS)
		kprintf("Found WFLG_NEWLOOKMENUS.\n");
	else
		kprintf("Cannot find WFLG_NEWLOOKMENUS.\n");
/*
	Mywindow->Flags |= WFLG_NEWLOOKMENUS;
	RefreshWindowFrame(Mywindow);
*/
	if (!LayoutMenus(menuStrip,visualInfo,
				GTMN_NewLookMenus, TRUE,
				textFont==NULL ? TAG_IGNORE : GTMN_TextAttr, &textAttr,
				TAG_END))
#else
	if (!LayoutMenus(menuStrip,visualInfo,
				Mywindow->Flags&WFLG_NEWLOOKMENUS ? GTMN_NewLookMenus : TAG_IGNORE, TRUE,
				textFont==NULL ? TAG_IGNORE : GTMN_TextAttr, &textAttr,
				TAG_END))
#endif
	{
		printf("csh: layout menus failed\n");
		FreeMenus(menuStrip);
		FreeVisualInfo(visualInfo);
		free(NewMenus);
	}

	return;
}

void remove_menu()
{
	if (AnzMenus>0) {
		struct NewMenu *nm;
		int i,j;

		for (i=0; i<MAXMENUS; i++) {
			for (j=0 ;j<MAXMENUITEMS; j++)
				if (MenuCommand[i][j]) {
					free(MenuCommand[i][j]);
					MenuCommand[i][j] = NULL;
				}
		}

		for (nm=NewMenus; nm->nm_Type!=NM_END; nm++) {
			if (nm->nm_Label) {
				free(nm->nm_Label);
				nm->nm_Label = NULL;
			}
			if (nm->nm_CommKey) {
				free(nm->nm_CommKey);
				nm->nm_CommKey = NULL;
			}
		}

		AnzMenus=0;
		set_menu();
	}
}

void set_menu()
{
	if (o_nowindow || !Mywindow)
		return;

	if (AnzMenus>0)
		SetMenuStrip(Mywindow,menuStrip);
	else {
		ClearMenuStrip(Mywindow);
		FreeMenus(menuStrip);
		FreeVisualInfo(visualInfo);
		free(NewMenus);
		menuStrip  = NULL;
		visualInfo = NULL;
		NewMenus   = NULL;
		AnzMenus = 0;
		AnzItems = 1;
		if (textFont) {
			CloseFont(textFont);
			textFont = NULL;
		}
		if (fontName) {
			free(fontName);
			fontName = NULL;
		}
	}

	Delay(3);
}


#if 0
int NumMenus;

static struct Menu DefaultMenu= {0, 0,0,TITWID,10, MENUENABLED,0,0};
static struct IntuiText DefaultIntuiText= {0,1,JAM2, 1,1,NULL,0,0};
static struct MenuItem DefaultMenuItem=
  {0, 0,0,ITEWID,0, HIGHCOMP|ITEMTEXT|ITEMENABLED,0,0,0,0,0,0};

struct Menu Menus[10];
char *MenuCommand[MAXMENUS][MAXMENUITEMS];

static void
install_menu( char *mav[], int mac )
{
	struct TextAttr *ta;
	struct Menu *m;
	struct MenuItem *mi, **pmi;
	struct IntuiText *it;
	int y, i, fonthei;
	char *p, *com;

	if( o_nowindow || !Mywindow )
		return;

	if( mac>=MAXMENUITEMS )
		mac=MAXMENUITEMS-1;

	ClearMenuStrip( Mywindow );
	Delay(3);

	if( NumMenus )
		Menus[NumMenus-1].NextMenu=Menus+NumMenus;
	m  = &Menus[NumMenus];
	*m = DefaultMenu;
	m->LeftEdge  = NumMenus*TITWID;
	m->MenuName  = strcpy(salloc(strlen(mav[0])+1),mav[0]);
	if( strlen(m->MenuName)>TITWID/8 )
		m->MenuName[TITWID/8+1]=0;
	DefaultIntuiText.ITextFont=ta=Mywindow->WScreen->Font;
	DefaultMenuItem.Height=2+(fonthei=ta->ta_YSize);

	y=0;
	pmi = &m->FirstItem;
	for( i=1; i<mac; i++) {
		it =(void *)salloc(sizeof(struct IntuiText));
		*it=DefaultIntuiText;
		mi =(void *)salloc(sizeof(struct MenuItem ));
		*mi=DefaultMenuItem;

		com=NULL;
		if( p=index(mav[i],',')) {
			*p=0; com = ++p;
			if( p=index(com,',')) {
				*p=0;
				mi->Command=p[1];
				mi->Flags |=COMMSEQ;
			}
		}

		if( !com || !*com) {
			com=strcpy(salloc(strlen(mav[i])+2),mav[i]);
			MenuCommand[NumMenus][i-1]=com;
			com+=strlen(com);
			*com++=13;
			*com=0;
		} else {
			MenuCommand[NumMenus][i-1]=strcpy(salloc(strlen(com)+1),com);
		}

		it->IText=(UBYTE *)strcpy(salloc(strlen(mav[i])+2),mav[i]);

		*pmi= mi;
		pmi = &mi->NextItem;
		mi->TopEdge = y;
		mi->ItemFill= (APTR)it;

		y+=DefaultMenuItem.Height;
	}

	NumMenus++;
MError:
	return;
}

void remove_menu()
{
	if (NumMenus>0) {
		struct MenuItem *mi, *nextmi;
		int i,j;

		for( i=0; i<NumMenus; i++ ) {
			for( mi=Menus[i].FirstItem,j=0 ; mi; mi=nextmi,j++ ) {
				free( ((struct IntuiText *)mi->ItemFill)->IText );
				free( ((struct IntuiText *)mi->ItemFill) );
				nextmi=mi->NextItem;
				free(mi);
				free(MenuCommand[i][j]);
			}
		}

		NumMenus=0;
		set_menu();
	}
}

void set_menu()
{
	if (o_nowindow || !Mywindow)
		return;

	if (NumMenus>0)
		SetMenuStrip(Mywindow,Menus);
	else
		ClearMenuStrip(Mywindow);

	Delay(3);
}
#endif

int
do_getenv( void )
{
	char buf[256], *val=buf;

	if( ac!=3 && ac!=2 ) {
		show_usage( NULL );
		return 20;
	}
	/* AMK: OS20-GetVar replaces ARP-Getenv */
	if( GetVar(av[ac-1],buf,256,GVF_GLOBAL_ONLY|GVF_BINARY_VAR) < 0L )
		val="";

	if( ac==2 )
		printf( "%s\n", val );
	else
		set_var( LEVEL_SET, av[1], val );
	return 0;
}

int
do_setenv( void )
{
	if( ac!=3 ) {
		show_usage( NULL );
		return 20;
	} else
		setenv( av[1], av[2] );
	return 0;
}

char **
read_name( char *name, int *ac )
{
	FILE *file;
	char **av=NULL;

	*ac=0;
	if( file=name ? fopen( name, "r") : stdin ) {
		av=read_file( file, ac );
		if( name ) fclose( file );
	} else 
		pError( name );
	return av;
}


char **
read_file( FILE *file, int *ac )
{
	int buflen=4096, lines=0, i, offs, got=0;
	char *buf, *tmp, *ptr, **lineptr;

	if( !(buf=ptr=DosAllocMem( buflen )))
		goto error;
	do {
		while( ptr+400 < buf+buflen && (got=myfgets(ptr, file)) && !dobreak())
			ptr+=strlen(ptr)+1, lines++;
		if( ptr+256 < buf+buflen ) {
			offs=ptr-buf;
			if( !(tmp=DosAllocMem( buflen*2 )))
				goto error;
			memcpy( tmp, buf, buflen );
			DosFreeMem( buf );
			buflen*=2, buf=tmp;
			ptr=buf+offs;
		}
	} while( got && !dobreak());
	if( !(lineptr=(char **)DosAllocMem( (lines+1)*sizeof( char * ))))
		goto error;
	*lineptr++=buf;
	for( ptr=buf, i=0; i<lines; i++ ) {
		lineptr[i]=ptr;
		ptr+=strlen(ptr)+1;
	}
	*ac=lines;
	return lineptr;

error:
	if( buf ) DosFreeMem( buf );
	fprintf( stderr, "Out of memory\n" );
	*ac=0;
	return NULL;
}

void
free_file( ptr )
	char **ptr;
{
	if( ptr-- ) {
		if( *ptr )
			DosFreeMem( *ptr );
		DosFreeMem(ptr);
	}
}


do_qsort( void )
{
	char **lineptr;
	int  lines, i;

	if( ac==1 ) {
		lineptr=read_file( stdin, &lines);
		DirQuickSort( lineptr, lines, (options&2) ? cmp_case : cmp, options&1, 0 );
		prepscroll(0);
		for( i=0; i<lines && !dobreak(); i++ ) {
			quickscroll();
			puts( lineptr[i] );
		}
		free_file( lineptr );
	} else
		ierror( NULL,506 );
	return 0;
}

extern int w_width;

do_truncate( void )
{
	char buf[256];
	int  w=newwidth(), c; /**/
	/*int  w = w_width, c;*/
	char *ptr;

	if( ac==2 )
		w=atoi( av[1] );

	prepscroll(0);
	while( myfgets(buf,stdin) && !dobreak() ) {
		for( c=0, ptr=buf; *ptr && c<w; ptr++ )
			if( *ptr=='\t' )
				c+=8-(c&7);
			else if( *ptr==27 ) {
				while( *ptr<'@' )
					ptr++;
			} else 
				c++;
		*ptr=0;
		quickscroll();
		puts(buf);
	}
	return 0;
}

int
do_readfile( void )
{
	char **rav, *str=NULL, *file=NULL;
	int rac;

	if( ac>2 ) file=av[2];
	if( rav=read_name( file, &rac ) ) { 
		if( str= compile_av( rav, 0, rac, 0xA0, 0 ) )
			set_var( LEVEL_SET, av[1], str );
		free_file( rav );
	}
	return str ? 0 : 20;
}

void
foreach( char **s, int (*func)(char *s) )
{
	char *str;

	for( ;; ) {
		str = *s;
		if( !(*s=index(*s,0xA0)))
			break;
		**s=0;
		(*func)(str);
		*(*s)++=0xA0;
		if( breakcheck())
			return;
	}
	(*func)(str);
}

int
do_writefile( void )
{
	char *ptr;

	if( !(ptr=get_var(LEVEL_SET,av[1])))
		{ fprintf(stderr,"Undefined variable %s\n",av[1]); return 20; }

	foreach( &ptr, puts );

	return 0;
}

int
do_split( void )
{
	int i;
	char *val, *gap, *oldval;

	if( !(val=get_var( LEVEL_SET, av[1] )))
		{ fprintf( stderr, "undefined variable %s\n", av[1] ); return 20; }
	oldval=val=strcpy(salloc(strlen(val)+1),val);
	for( i=2; i<ac-1; i++ ) {
		if( gap=index(val,0xA0 )) *gap=0;
		set_var( LEVEL_SET, av[i], val );
		val="";
		if( gap ) *gap=0xA0, val=gap+1;
	}
	set_var( LEVEL_SET, av[ac-1], val );
	free(oldval);
	return 0;
}

char *
copyof( char *str )
{
	return strcpy(salloc(strlen(str)+1),str);
}

int
do_class( char *avline )
{
	CLASS *new;

	if( options&1 ) {
		avline=next_word(avline);
		for( new=CRoot,CRoot=NULL; new; new=new->next )
			Free(new);
	}

	if( ac==1 ) {
		for( new=CRoot; new; new=new->next )
			printf("%s\n",new->name);
		return 0;
	}

	avline=next_word(avline);
	if(!(new=malloc( strlen(avline)+5)))
		ierror( NULL, 512 );
	else {
		new->next=NULL;
		strcpy( new->name,avline );
		if( CRoot )
			LastCRoot->next=new;
		else 
			CRoot=new;
		LastCRoot=new;
	}
	return 0;
}

do_getcl( void )
{
	char *s=getclass(av[1]);
	if( s ) printf("%s\n",s);
	return 0;
}

do_action( char *argline )
{
	char *args, err;
	int abort=options&1;

	args=compile_av( av,3,ac,' ',0 );
	err=doaction(av[2],av[1],args);
	if( !abort )
		if( err==9 )     ierror(av[2], 205 );
		else if(err==10) fprintf(stderr,"Can't identify %s\n", av[2] );
		else if(err==11) fprintf(stderr,"Can't '%s' this file\n",av[1] );
		else if(err==12) fprintf(stderr,"Error executing the program to '%s' this file\n",av[1] );
	free(args);
	return abort ? !err : err;
}


