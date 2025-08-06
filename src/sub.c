
/*
 * SUB.C
 *
 * (c)1986 Matthew Dillon     9 October 1986
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 *
 */

#include "shell.h"

static void del_history( void );
static int dnext( DPTR *dp, char **pname, int *stat);
static char *svfile( char *s1, char *s2, FIB *fib);
static void quicksort( char **av, int n );

void
seterr( int err )
{
	static int LastErr;
	char buf[32], *val;
	int  stat=0;

	Lastresult=err;

	if( LastErr!=err ) {
		LastErr=err;
		sprintf(buf, "%d", err);
		set_var(LEVEL_SET, v_lasterr, buf);

		if( val=get_var(LEVEL_SET, v_stat))
			stat = atoi(val);
		if (stat < Lastresult) set_var(LEVEL_SET, v_stat, buf);
	}
}

char *
next_word( char *str )
{
	while (*str && ! ISSPACE(*str)) ++str;
	while (*str &&   ISSPACE(*str)) ++str;
	return str;
}

/*
 * FREE(ptr)   --frees without actually freeing, so the data is still good
 *               immediately after the free.
 */


void
Free( void *ptr )
{
	static char *old_ptr;

	if (old_ptr) free (old_ptr);
	old_ptr = ptr;
}

/*
 * Add new string to history (H_head, H_tail, H_len,
 *  S_histlen
 */

void
add_history( char *str )
{
	HIST *hist;
	char *get;

	for( get=str; *get; get++ )
		if( (*get&127)<' ')
			*get=' ';

	if (H_head != NULL && !strcmp(H_head->line, str))
		return;
	while (H_len > S_histlen)
		del_history();
	hist = (HIST *)salloc (sizeof(HIST));
	if (H_head == NULL) {
		H_head = H_tail = hist;
		hist->next = NULL;
	} else {
		hist->next = H_head;
		H_head->prev = hist;
		H_head = hist;
	}
	hist->prev = NULL;
	hist->line = salloc (strlen(str) + 1);
	strcpy (hist->line, str);
	++H_len;
	H_num= H_tail_base+ H_len;
}

static void
del_history()
{
	if (H_tail) {
		--H_len;
		++H_tail_base;
		free (H_tail->line);
		if (H_tail->prev) {
			H_tail = H_tail->prev;
			free (H_tail->next);
		H_tail->next = NULL;
		} else {
			free (H_tail);
			H_tail = H_head = NULL;
		}
	}
}

char *
get_history( char *ptr )
{
	int   hnum=find_history( ptr+1, 1 );
	HIST *hist;

	for( hist=H_head; hist && hnum--; hist=hist->next) ;
	if( !hist ) {
		fprintf(stderr,"History failed\n");
		return "";
	}
	fprintf(stderr, "%s\n", hist->line );
	return hist->line;
}


int
find_history( char *ptr, int min )
{
	HIST *hist;
	int  len, num;

	if     ( *ptr=='!' )
		return 1;
	else if( *ptr=='-' )
		return atoi(ptr+1);
	else if (*ptr >= '0' && *ptr <= '9')
		return H_len-(atoi(ptr)-H_tail_base)-1;

	len = strlen(ptr);
	for (hist=H_head,num=0; hist; hist=hist->next, num++)
		if ( !strnicmp(hist->line, ptr, len) && num>=min)
			return num;
	return -1;
}

void
replace_head( char *str )
{
	if (str && strlen(str) && H_head) {
		free (H_head->line);
		H_head->line = salloc (strlen(str)+1);
		strcpy (H_head->line, str);
	}
}


void
pError(char *str )
{
	int ierr = (long)IoErr();
	ierror(str, ierr);
}

ierror( char *str, int err )
{
	PERROR *per = Perror;
	char msgbuf[256];

	setioerror(err);

	if (err) {
		if(err<500 && Fault(err,str,msgbuf,255)) {
			fprintf(stderr,"%s\n",msgbuf);
			return err;
		}
		else {
			for (; per->errstr; ++per) {
				if (per->errnum == err) {
					fprintf (stderr, "%s%s%s\n",
					per->errstr,
					(str) ? ": " : "",
					(str) ? str : "");
					return err;
				}
			}
		}
		fprintf (stderr, "Unknown DOS error %d: %s\n", err, (str) ? str : "");
	}
	return err;
}

void
setioerror( int err )
{
	static int LastIoError = -1;
	char buf[20];

	IoError=err;
	if( IoError<0 ) IoError=0;
	if( LastIoError!=IoError) {
		LastIoError=IoError;
		sprintf(buf, "%d", IoError);
		set_var(LEVEL_SET, v_ioerr, buf);
	}
}

char *
ioerror(int num)
{
	PERROR *per = Perror;

	for ( ; per->errstr; ++per)
		if (per->errnum == num)
			return per->errstr;
	return NULL;
}

/*
 * Disk directory routines
 *
 * dptr = dopen(name, stat)
 *    DPTR *dptr;
 *    char *name;
 *    int *stat;
 *
 * dnext(dptr, name, stat)
 *    DPTR *dptr;
 *    char **name;
 *    int  *stat;
 *
 * dclose(dptr)                  -may be called with NULL without harm
 *
 * dopen() returns a struct DPTR, or NULL if the given file does not
 * exist.  stat will be set to 1 if the file is a directory.  If the
 * name is "", then the current directory is openned.
 *
 * dnext() returns 1 until there are no more entries.  The **name and
 * *stat are set.  *stat != 0 if the file is a directory.
 *
 * dclose() closes a directory channel.
 *
 */

DPTR *
dopen( char *name, int *stat)
{
	DPTR *dp;

	IoError=0;
	*stat = 0;
	dp = (DPTR *)salloc(sizeof(DPTR));
	if (*name == '\0')
		dp->lock = DupLock(Myprocess->pr_CurrentDir);
	else
		dp->lock = Lock (name,ACCESS_READ);
	if (dp->lock == NULL) {
		IoError=IoErr();
		free (dp);
		return NULL;
	}
	dp->fib = (FIB *)SAllocMem((long)sizeof(FIB), MEMF_PUBLIC);
	if (!Examine (dp->lock, dp->fib)) {
		pError (name);
		dclose (dp);
		return NULL;
	}
	if (dp->fib->fib_DirEntryType >= 0) *stat = 1;

#ifdef MA_AMK
	{
	char *c;
	if (c=strchr(name,':')) {
		if (dp->dname = strdup(name))
			dp->dvp = GetDeviceProc(dp->dname,NULL);
		else
			dp->dvp = NULL;
	}
	else {
		dp->dname = NULL;
		dp->dvp   = NULL;
	}
	}
#endif

	return dp;
}

static int
dnext( DPTR *dp, char **pname, int *stat)
{
	if (dp == NULL) return (0);

	if (ExNext (dp->lock, dp->fib)) {
		*stat = 0;
		if( dp->fib->fib_DirEntryType >= 0)
			*stat= dp->fib->fib_DirEntryType!=ST_USERDIR ? 2 : 1;
		*pname = dp->fib->fib_FileName;
		return 1;
	}

#ifdef MA_AMK
	{
	BOOL ma_done = FALSE;
	if (IoErr()==ERROR_NO_MORE_ENTRIES) {
		while (!ma_done && dp->dvp && (dp->dvp->dvp_Flags&DVPF_ASSIGN)) {
			dp->dvp = GetDeviceProc(dp->dname,dp->dvp);
			if (dp->dvp && dp->dvp->dvp_Lock) {
				char buf[256];
				BPTR new_lock,old_lock;
				char *add = strchr(dp->dname,':')+1;
				if (NameFromLock(dp->dvp->dvp_Lock,buf,255)) {
					AddPart(buf,add,255);
					printf("«%s»\n",buf);

					if (new_lock=Lock(buf,ACCESS_READ)) {
						if (Examine(new_lock,dp->fib)) {
							if (dp->lock) UnLock(dp->lock);
							dp->lock = new_lock;
							if (dp->fib->fib_DirEntryType >= 0) *stat = 1;
							*pname = dp->fib->fib_FileName;
							return 1;
							ma_done = TRUE;
						}
					}
				}
				else {
					fprintf(stderr,"csh.dnext: NameFromLock() failed\n");
				}
			}
		}
	}
	}
#endif

	return 0;
}

int
dclose( DPTR *dp )
{
	if (dp == NULL)
		return 1;
	if (dp->fib)
		FreeMem (dp->fib,(long)sizeof(*dp->fib));
	if (dp->lock)
		UnLock (dp->lock);
#ifdef MA_AMK
	if (dp->dvp)
		FreeDeviceProc(dp->dvp);
#endif
	free (dp);
	return 1;
}


int
isdir( char *file )
{
	DPTR *dp;
	int stat;

	stat = 0;
	if (dp = dopen (file, &stat))
		dclose(dp);
	return (stat!=0);
}


void
free_expand( char **av )
{
	char **get = av;

	if (av) {
		while (*get)
		free (*get++-sizeof(FILEINFO));
		free (av);
	}
}

/*
 * EXPAND(base,pac)
 *    base           - char * (example: "df0:*.c")
 *    pac            - int  *  will be set to # of arguments.
 *
 * 22-May-87 SJD.  Heavily modified to allow recursive wild carding and
 *                 simple directory/file lookups. Returns a pointer to
 *                 an array of pointers that contains the full file spec
 *                 eg. 'df0:c/sear*' would result in : 'df0:C/Search'
 *
 *                 Now no longer necessary to Examine the files a second time
 *                 in do_dir since expand will return the full file info
 *                 appended to the file name. Set by formatfile().
 *
 *                 Caller must call free_expand when done with the array.
 *
 * base             bname =       ename =
 * ------           -------       -------
 *  "*"               ""            "*"
 *  "!*.info"         ""            "*.info" (wild_exclude set)
 *  "su*d/*"          ""            "*"      (tail set)
 *  "file.*"          ""            "file.*"
 *  "df0:c/*"         "df0:c"       "*"
 *  ""                ""            "*"
 *  "df0:.../*"       "df0:"        "*"      (recur set)
 *  "df0:sub/.../*"   "df0:sub"     "*"      (recur set)
 *
 * ---the above base would be provided by execom.c or do_dir().
 * ---the below base would only be called from do_dir().
 *
 *  "file.c"          "file.c"      ""       if (dp == 0) fail else get file.c
 *  "df0:"            "df0:"        "*"
 *  "file/file"       "file/file"   ""       if (dp == 0) fail
 *  "df0:.../"        "df0:"        "*"      (recur set)
 *
 */

char **
expand( char *base, int *pac )
{
	char *ptr;
	char **eav = (char **)salloc(sizeof(char *) * (2));
	short eleft, eac;
	char *name;
	char *bname, *ename, *tail;
	int  stat, recur, scr, bl;
	DPTR *dp;
	PATTERN *pat;

	IoError = *pac = recur = eleft = eac = 0;

	/* kprintf("Analyzing: %s\n",base); */

	base = strcpy(salloc(strlen(base)+1), base);
#if 1
	/*for (ptr = base; *ptr && *ptr != '?' && *ptr != '*'; ++ptr);*/
	for (ptr = base; *ptr && *ptr != '?' && *ptr != '*' && *ptr != '[' && *ptr != ']'; ++ptr);
#else
	/* AMK: experimental DOS pattern detection */
	/*printf("trying to expand... %s\n",base);*/
	for (ptr=base; *ptr && *ptr != '?' && *ptr!='*' && *ptr!='!' &&
	               *ptr != '(' && *ptr!=')' &&
	               *ptr != '|' && *ptr!='%' &&
	               *ptr != '~' && *ptr!='#' &&
	               *ptr != '[' && *ptr!=']';
	     ++ptr);
#endif

	if (!*ptr)   /* no wild cards */
		--ptr;
	else
		for (; ptr >= base && !(*ptr == '/' || *ptr == ':'); --ptr);

	if (ptr < base) {
		bname = strcpy (salloc(1), "");
	} else {
		scr = ptr[1];
		ptr[1] = '\0';
		if (!strcmp(ptr-3,".../")) {
			recur = 1;
			*(ptr-3) = '\0';
		}
		bname = strcpy (salloc(strlen(base)+2), base);
		ptr[1] = scr;
	}
	bl = strlen(bname);
	ename = ++ptr;
	for (; *ptr && *ptr != '/'; ++ptr);
	scr = *ptr;
	*ptr = '\0';
	if (scr) ++ptr;
	tail = ptr;

	if ((dp = dopen (bname, &stat)) == NULL || (stat == 0 && *ename)) {
		free (bname);
		free (base);
		free (eav);
		return NULL;
	}

	if (!stat) {                /* eg. 'dir file' */
		char *p,*s;
		for(s = p = bname; *p; ++p) if (*p == '/' || *p == ':') s = p;
		if (*s=='/' || *s==':') s++;
		*s=0;
		eav[eac++] = svfile(bname,dp->fib->fib_FileName,dp->fib);
		goto done;
	}
	if (!*ename) ename = "*";    /* eg. dir df0: */
	if (*bname && bname[bl-1] != ':' && bname[bl-1] != '/') { /* dir df0:c */
		bname[bl] = '/';
		bname[++bl] = '\0';
	}

#if 0
	if (stat) {
		kprintf("isdir: %s, examining: %s\n",bname,ename);
	}
#endif

	pat= compare_preparse( ename, 0 );
	while ((dnext (dp, &name, &stat)) && !breakcheck()) {
		int match = compare_ok( pat, name);
		if (match && (recur || !*tail)) {
			if (eleft < 2) {
				char **scrav = (char **)salloc(sizeof(char *) * (eac + 10));
				memmove (scrav, eav, (eac + 1) << 2);
				free (eav);
				eav = scrav;
				eleft = 10;
			}
			eav[eac++] = svfile(bname,name,dp->fib);
			--eleft;
		}
		if ((*tail && match) || recur) {
			int alt_ac;
			char *search, **alt_av, **scrav;
			BPTR lock;

			if (stat!=1)           /* expect more dirs, but this not a dir */
				continue;
			lock = CurrentDir (dp->lock);
			search = salloc(strlen(ename)+strlen(name)+strlen(tail)+6);
			strcpy (search, name);
			strcat (search, "/");
			if (recur) {
				strcat(search, ".../");
				strcat(search, ename);
			}
			strcat (search, tail);
			/*
			 *  expand() will crash in endless recursion
			 *  if you do a .../* and a directory name
			 *  contains a pattern character
			 *
			 *  possible solution (hack!) :
			 *  convert all pattern characters to some
			 *  "harmless" characters and change them
			 *  back later. It's obvious that this sucks
			 *  if a directory contains exactly that
			 *  escape character :-(
			 */
			scrav = alt_av = expand (search, &alt_ac);
			free(search);
			CurrentDir (lock);
			if (scrav) {
				while (*scrav) {
					int l;
					if (eleft < 2) {
						char **scrav = (char **)salloc(sizeof(char *)*(eac+10));
						memmove ( scrav, eav, (eac + 1) << 2);
						free (eav);
						eav = scrav;
						eleft = 10;
					}

					l = strlen(*scrav);
					eav[eac] = salloc(bl+l+1+sizeof(FILEINFO));
					memcpy( eav[eac], *scrav-sizeof(FILEINFO),sizeof(FILEINFO));
					eav[eac]+=sizeof(FILEINFO);
					strcpy( eav[eac], bname);
					strcat( eav[eac], *scrav);

					free (*scrav-sizeof(FILEINFO));
					++scrav;
					--eleft, ++eac;
				}
				free (alt_av);
			}
		}
	}
	compare_free( pat );
done:
	dclose (dp);
	*pac = eac;
	eav[eac] = NULL;
	free (bname);
	free (base);

	if (eac) {
		int i, j, len;

		QuickSort( eav, eac );
		for( i=0; i<eac-1; i++ ) {
			len=strlen(eav[i]);

			for( j=i+1; j<eac && !strnicmp(eav[i],eav[j],len); j++ )
				if( !stricmp(eav[j]+len,".info")) {
					((FILEINFO*)eav[i]-1)->flags |= INFO_INFO;
					break;
				}
		}
		return eav;
	}

	free (eav);
	return NULL;
}



char *strdel(char *s,int from,int len)
{
	while( s[from] ) {
		s[from] = s[from+len];
		++from;
	}
	return(s);
}



char *
strupr( char *s )
{
	char *old=s;
	while (*s) *s=toupper(*s), s++;
	return old;
}

char *
strlwr( char *s )
{
	char *old=s;
	while (*s) *s=tolower(*s), s++;
	return old;
}



/*
 * Compare a wild card name with a normal name
 */

extern void *PatternBase;

PATTERN *
compare_preparse( char *wild, int casedep )
{
/*	struct pattern *pat=salloc(256);*/
	struct pattern *pat;
	char *my_wild = wild;

	pat = salloc(sizeof(struct pattern) + strlen(wild)*2 + 3);
	/* this is not exact ('lil bit too much) but better readable */

	pat->casedep=casedep;
	if (pat->queryflag=(*wild=='&')) wild++;

	my_wild = salloc(strlen(wild)+3);
	strcpy(my_wild,wild);
	if (*my_wild=='!') *my_wild='~';
	if (!casedep) strlwr(my_wild);
	if (*my_wild=='~') {
		strins(my_wild+1,"(");
		strcat(my_wild,")");
	}

	DOSBase->dl_Root->rn_Flags |= RNF_WILDSTAR;
	if (PatternBase) {
		if ((pat->patptr=AllocPattern( my_wild, 0 ))<0) {
			free(pat);
			free(my_wild);
			return NULL;
		}
	}
	else {
		if ((ParsePattern(my_wild,pat->pattern,strlen(wild)*2+2)<0)) {
			free(pat);
			free(my_wild);
			return NULL;
		}
	}

	free(my_wild);
	return pat;
}

int
compare_ok( PATTERN *pat, char *name )
{
	char *lowname=name;

	if (!pat)
		return 0;

	if (!pat->casedep) {
		if (lowname=strdup(name))
			strlwr(lowname);
		else
			return 0;
	}

	DOSBase->dl_Root->rn_Flags |= RNF_WILDSTAR;
	if (PatternBase) {
		if (MatchThePattern(pat->patptr,lowname)!=1) {
			if (lowname!=name) free(lowname);
			return 0;
		}
	} else {
		if (!MatchPattern(pat->pattern,lowname)) {
			if (lowname!=name) free(lowname);
			return 0;
		}
	}

	if (lowname!=name)
		free(lowname);

	if (pat->queryflag) {
		char buf[260];
		printf("Select %s%-16s%s [y/n] ? ",o_hilite,name,o_lolite);
		gets(buf);
		return (toupper(*buf)=='Y');
	}

	return 1;
}

void
compare_free( PATTERN *pat )
{
	if( !pat )
		return;
	if( PatternBase )
		FreePattern(pat->patptr);
	free( pat );
}

#if 0
int
compare_strings( char *pattern, char *str, int casedep )
{
	PATTERN *pat=compare_preparse( pattern, casedep );
	int ret=compare_ok(pat,str);
	compare_free(pat);
	return ret;
}
#endif

static char *
svfile( char *s1, char *s2, FIB *fib)
{
	FILEINFO *info=salloc(sizeof(FILEINFO)+strlen(s1)+strlen(s2)+1);
	char     *p = (char *)(info+1);

	strcpy(p, s1);
	strcat(p, s2);
	info->flags = fib->fib_Protection | (*fib->fib_Comment ? INFO_COMMENT : 0);
	info->type  = fib->fib_DirEntryType;
	info->size  = (fib->fib_DirEntryType<0 || fib->fib_DirEntryType==ST_SOFTLINK) ? fib->fib_Size      : -1;
	info->blocks= (fib->fib_DirEntryType<0 || fib->fib_DirEntryType==ST_SOFTLINK) ? fib->fib_NumBlocks :  0;
	info->date  = fib->fib_Date;
	info->uid   = fib->fib_OwnerUID;
	info->gid   = fib->fib_OwnerGID;
	info->class[0]=1;
	return p;
}

static char   *FullPath;
static FIB    *PrevFile;
static int   (*Action)(long, char *, char *);
static long    Mask;
static int     Queued=0;

static int
passinfo( FIB *fib )
{
	char   *p = (char *)(PrevFile+1);
	int    oldlen, ret=0;

	if( Queued ) {
		oldlen=strlen(FullPath);
		/* AMK: don't know the exact length... used dummy 256L */
		AddPart( FullPath, p, 256L );
		ret=(*Action)( Mask, p, FullPath );
		FullPath[oldlen]=0;
		Queued=0;
	}

	if( fib ) {
		*PrevFile= *fib;
		strcpy( p, fib->fib_FileName);
		Queued=1;
	}

	return ret;
}


static int
nu_recurse( char *name )
{
	BPTR lock, cwd;
	FIB  *fib  =(FIB *)SAllocMem(sizeof(FIB),MEMF_PUBLIC);
	int  oldlen=strlen( FullPath ), ret=0;

	/* AMK: don't know the length... used dummy 256L */
	AddPart( FullPath, name, 256L );

	if (lock=Lock(name,ACCESS_READ)) {
		if( Mask & SCAN_DIRENTRY )
			(*Action)( SCAN_DIRENTRY, name, FullPath );

		cwd =CurrentDir(lock);
		if (Examine(lock, fib)) {
			while (ExNext(lock, fib) && !breakcheck()) {
				if (fib->fib_DirEntryType==ST_USERDIR) {
					if ( Mask & SCAN_RECURSE ) {
						if(!ret && !(ret=passinfo( NULL )))
							ret=nu_recurse(fib->fib_FileName);
					}
					if ( Mask & SCAN_DIR     )
						if( !ret )
							ret=passinfo( fib );
				} else if( fib->fib_DirEntryType<0 && Mask & SCAN_FILE )
					if( !ret )
						ret=passinfo( fib );
			}
			if( breakcheck() )
				ret=5;

			if( !ret )
				passinfo( NULL );

		}
		UnLock(CurrentDir(cwd));

		if( Mask & SCAN_DIREND )
			(*Action)( SCAN_DIREND, name, FullPath );
	} else
		pError(name);

	FullPath[oldlen]=0;
	FreeMem(fib, sizeof(FIB));
	return ret;
}

int
newrecurse(int mask, char *name, int (*action)FUNCARG(long,char *,char *))
{
	int ret;

	FullPath = salloc( 512 );
	PrevFile = salloc( sizeof(FIB)+108+1 );
	Action   = action;
	Mask     = mask;
	Queued   = 0;
	*FullPath= 0;

	ret=nu_recurse( name );

	free( PrevFile );
	free( FullPath );

	return ret;
}


/* Sort routines */

static int reverse, factor;

int
cmp( FILEINFO *s1, FILEINFO *s2)
{
	return stricmp( (char *)(s1+1), (char *)(s2+1) );
}

int
cmp_case( FILEINFO *s1, FILEINFO *s2)
{
	return strcmp( (char *)(s1+1), (char *)(s2+1) );
}

int
sizecmp( FILEINFO *s1, FILEINFO *s2)
{
	return s2->size - s1->size;
}

int
datecmp_csh( FILEINFO *s1, FILEINFO *s2 )
{
	int r;
	struct DateStamp *d1 = &s1->date, *d2 = &s2->date;
	if( !(r= d2->ds_Days - d1->ds_Days))
		if( !(r=d2->ds_Minute - d1->ds_Minute ) )
			r=d2->ds_Tick - d1->ds_Tick;
	return r;
}


int
numcmp( FILEINFO *s1, FILEINFO *s2 )
{
	return atoi((char *)(s1+1))-atoi((char *)(s2+1));
}

static void
enterclass( FILEINFO *info )
{
	char *class, *iclass=info->class, *t;

	if( *iclass==1 ) {
		if( class=getclass( (char *)(info+1))) {
			strncpy( iclass, class, 11 );
			iclass[11]=0;
			if( t=index(iclass,0xA0))
				*t=0;
		} else 
			iclass[0]=0;
	}
}

int
classcmp( FILEINFO *info1, FILEINFO *info2 )
{
	int r;

	enterclass( info1 );
	enterclass( info2 );

	r= stricmp( info1->class, info2->class );
	if( !r ) r=stricmp((char *)(info1+1),(char *)(info2+1));
	return r;
}


void
QuickSort( char *av[], int n)
{
	reverse=factor=0;
	DirQuickSort( av, n, cmp, 0, 0 );
}

static int (*compare)(FILEINFO *, FILEINFO *);

static int
docompare(char *s1,char *s2)
{
	FILEINFO *i1=(FILEINFO *)s1-1, *i2=(FILEINFO *)s2-1;
	int r=(*compare)( i1,i2 );

	if( reverse ) r = -r;
	if( factor )  r+= factor*((i2->size<0) - (i1->size<0));
	return r;
}

#define QSORT

void
DirQuickSort( char *av[], int n, int (*func)(FILEINFO *,FILEINFO *), int rev, int fac)
{
	reverse=rev; compare=func; factor=fac;

	quicksort( av, n-1 );
}

static void
quicksort( char **av, int n )
{
	char **i, **j, *x, *t;


	if( n>0 ) {
		i=av; j=av+n; x=av[ n>>1 ];
		do {
			while( docompare(*i,x)<0 ) i++;
			while( docompare(x,*j)<0 ) --j;
			if( i<=j )
				{ t = *i; *i = *j; *j=t; i++; j--; }
		} while( i<=j );

		if( j-av < av+n-i ) {
			quicksort( av, j-av  );
			quicksort( i , av+n-i);
		} else {
			quicksort( i , av+n-i);
			quicksort( av, j-av  );
		}
	}
}

int
filesize( char *name )
{
	BPTR lock;
	struct FileInfoBlock *fib;
	int  len=0;

	if( lock = Lock (name,ACCESS_READ)) {
		if( fib=(struct FileInfoBlock *)AllocMem(sizeof(*fib),MEMF_PUBLIC)) {
			if (Examine (lock, fib))
				len=fib->fib_Size;
			FreeMem( fib, sizeof(*fib));
		}
		UnLock(lock);
	}
	return len;
}


#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

char **
and( char **av1, int ac1, char **av2, int ac2, int *ac, int base )
{
	char **av=(char **)salloc(MIN(ac1,ac2)*sizeof(char *) ), *str;
	int i, j, k=0;

	for( i=0; i<ac1; i++ )
		for( j=0, str=base ? (char*)FilePart(av1[i]) : av1[i]; j<ac2; j++ )
			if( !stricmp(str, base ? (char*)FilePart(av2[j]) : av2[j]))
				av[k++]=av1[i];
	*ac=k;
	return av;
}

char **
without( char **av1, int ac1, char **av2, int ac2, int *ac, int base )
{
	char **av=(char **)salloc(ac1*sizeof(char *) ), *str;
	int i, j, k=0;

	for( i=0; i<ac1; i++ ) {
		for( j=0, str=base ? (char*)FilePart(av1[i]) : av1[i]; j<ac2; j++ )
			if( !stricmp(str, base ? (char*)FilePart(av2[j]) : av2[j] ) )
				break;
		if( j==ac2 )
			av[k++]=av1[i];
	}
	*ac=k;
	return av;
}

char **
or( char **av1, int ac1, char **av2, int ac2, int *ac, int base )
{
	char **av=(char **)salloc((ac1+ac2)*sizeof(char *) ), *str;
	int i, j, k=0;

	for( i=0; i<ac1; i++ )
		av[k++]=av1[i];

	for( i=0; i<ac2; i++ ) {
		for( j=0, str=base ? (char*)FilePart(av2[i]) : av2[i]; j<ac1; j++ )
			if( !stricmp(str, base ? (char*)FilePart(av1[j]) : av1[j] ) )
				break;
		if( j==ac1 )
			av[k++]=av2[i];
	}

	*ac=k;
	return av;
}

void
clear_archive_bit( char *name )
{
	DPTR *dp;
	int  stat;

	if(dp = dopen(name,&stat) ) {
		setProtection( name, dp->fib->fib_Protection&~FIBF_ARCHIVE);
		dclose( dp );
	}
}

char *
itoa( int i )
{
	static char buf[20];
	char *pos=buf+19;
	int count=4, flag=0;

	if( i<0 )
		flag=1, i = -i;

	do {
		if( !--count )
			count=3, *--pos=',';
		*--pos= i%10+'0';
	} while( i/=10 );

	if( flag )
		*--pos='-';

	return pos;
}

char *
itok( int i )
{
#if 0
	static char buf[16], which;
	char *exp=" KMG", *ptr= buf+(which=8-which);
#endif
	/* now three buffers instead of two as needed for do_info() */
	static char buf[24], which;
	char *exp=" kMG", *ptr= buf+(8*(which=(which+1)%3));
	int res,m=1024;

/* AMK: don't cripple small numbers */
	if (i<1000) {
		sprintf(ptr,"%d",i);
		return ptr;
	}

/* AMK: allow four places for kilobytes */
	if ((res=(i+512)/1024)<10000) {
		sprintf(ptr,"%dk",res);
		return ptr;
	}
/* AMK: because of HD floppies with 1760K (not 2MB :-) */

	do {
		/* res=(i+512)/m; */
		res=(i+(m/2))/m;
		m*=1024;
		++exp;
	}
	while (res>999);

	if (res==0)
		res=1;

	sprintf(ptr,"%d%c",res,*exp);

	return ptr;
}

char *
next_a0( char *str )
{
	while( *str && (UBYTE)*str!=0xA0 && *str!='=' && *str!=',') str++;
	if( *str )
		return str+1;
	return NULL;
}

static int
gethex( char *str, int l )
{
	int i, val=0, n, c;

	if( *str=='.' ) return l==2 ? 256 : 0;

	for( i=0; i<l || !l; i++ ) {
		c = *str++;
		if     ( c>='0' && c<='9' ) n=c-'0';
		else if( c>='a' && c<='f' ) n=c-'a'+10;
		else if( c>='A' && c<='F' ) n=c-'A'+10;
		else break;;
		val=16*val+n;
	}
	return (l && i!=l) ? -1 : val;
}

strwrdcmp( char *str, char *wrd )
{
	PATTERN *pat;
	int ret;
	char *ind=index(wrd,0xA0);

	if( ind ) *ind=0;
	pat=compare_preparse( wrd,0 );
	ret=compare_ok(pat,str);
	compare_free( pat );
	if( ind ) *ind=0xA0;
	return !ret;
}

int
wrdlen( char *str )
{
	char *old=str;

	while( *str && (UBYTE)*str!=0xA0 ) str++;
	return str-old;
}

char *
getclass(char *file)
{
	CLASS *cl;
	char *class, *str, *arg, *get, *buf;
	int offs, byte, len, fail;
	BPTR fh;

	if( isdir(file) ) return "dir";

	if( !(buf=calloc(1024,1))) return NULL;
	if( !(fh=Open(file,MODE_OLDFILE))) return NULL;
	len=Read( fh,buf,1023);
	Close(fh);

	for( cl=CRoot; cl; cl=cl->next ) {
		class=cl->name;
		if(!(str=next_a0(cl->name))) continue;
		while( str ) {
			if(!(arg=next_a0( str ))) goto nextclass;
			switch( *str ) {
			case 's':
				if( (offs=strlen(file)-wrdlen(arg))<0 ) break;
				if( !strwrdcmp(file+offs,arg)) goto found;
				break;
			case 'n':
				if( !strwrdcmp(FilePart(file),arg) ) goto found;
				break;
			case 'd':
				goto found;
			case 'o':
				offs=gethex(arg,0);
				if( !(arg=index(arg,','))) goto nextclass;
				if( offs>len-10 ) break;
				for( get=buf+offs, ++arg; (byte=gethex(arg,2))>=0; arg+=2 )
					if( (UBYTE)*get++!=byte && byte!=256 )
						goto nexttry;
				goto found;
			case 'c':
				if( !len )
					break;
				for( get=buf, fail=0; get<buf+len; get++ )
					if( *get<9 || *get>13 && *get<32 || *get>127  )
						fail++;
				if( fail*8>len )
					break;
				goto found;
			case 'i':
				break;
			case 'a':
				goto nextclass;
			default:
				goto nextclass;
			}
nexttry:	str=next_a0(arg);
		}
nextclass: ;
	}

	free(buf);
	return NULL;

found:
	free(buf);
	return (char *)class;
}

char *
superclass( char *subclass )
{
	CLASS *cl;
	char *cur;
	int len;

	for( len=0; subclass[len] && (UBYTE)subclass[len]!=0xA0; len++ ) ;
	for( cl=CRoot; cl; cl=cl->next ) {
		if( strncmp( cur=cl->name,subclass,len ))
			continue;
		do
			cur=index( cur,0xA0 );
		while( cur && *++cur!='i');

		if( cur && (cur=index(cur,'=')))
			return ++cur;
	}
	return NULL;
}

char *
getaction( char *class, char *action )
{
	CLASS *cl;
	char *cur, *ind;
	int len;

	for( len=0; class[len] && (UBYTE)class[len]!=0xA0; len++ ) ;
	for( cl=CRoot; cl; cl=cl->next ) {
		if( strncmp( cur=cl->name,class,len ) || !issep(cur[len]))
			continue;
		do
			cur=index( cur,0xA0 );
		while( cur && *++cur!='a');

		if( cur && (cur=index( ++cur,0xA0 ))) {
			do {
				if( !(ind=index( ++cur,'=' )))
					return NULL;
				len=ind-cur;
				if( len==strlen(action) && !strncmp(action,cur,len))
					return ++ind;
			} while( cur=index(cur,0xA0) );
		}
	}
	return NULL;
}

int
doaction( char *file, char *action, char *args )
{
	char *class, *com, *c, *copy;
	int ret, spc=hasspace(file), i=0;

	if( !(class=getclass(file)))
		return exists(file) ? 10 : 9;

	do
		if( com=getaction(class,action))
			break;
	while( (class=superclass( class )) && ++i<4 );
	if( !class )
		return 11;
	if( c=index(com,0xA0) )
		*c=0;
	copy=salloc( strlen(com)+strlen(file)+strlen(args)+7 );
	sprintf(copy,spc?"%s \"%s\" %s":"%s %s %s", com, file, args);
	ret=execute(copy);
	free(copy);
	if( c )
		*c=0xA0;
	return ret ? 12 : 0;
}

void *
salloc( int len )
{
	void *ret;

	if( !len ) len++;

	if( !(ret=malloc(len))) {
		fprintf(stderr,"Out of memory -- exiting\n");
		main_exit( 20 );
	}
	return ret;
}

void *
SAllocMem( long size, long req  )
{
	void *ret;

	if( !(ret=AllocMem(size,req))) {
		fprintf(stderr,"Out of memory -- exiting\n");
		main_exit( 20 );
	}
	return ret;
}



int
issep( char s )
{
	return !s || ISSPACE(s) || s=='/' || s==';' || s=='|' || s=='&';
}


char *
filemap( char *buf, int last )
{
	char *s=buf, *d, *lcd;
	int  len;

	if( last && *s=='~' && issep(s[1])) {
		if (!(lcd=get_var(LEVEL_SET,v_lcd)))
			return buf;
		len= strlen(lcd);
		memmove( s+len, s+1, strlen(s)+1);
		memcpy( s, lcd, strlen(lcd));
		if( s[len]!='/' )
			return buf;
		s+=len+1;
	}
	d=s;
	for( ;; ) {
		if( s[0]=='.' && issep(s[1]) ) {
#if 0
			char AmkBuf[256];
			strcpy(AmkBuf,get_var(LEVEL_SET, v_cwd));
			AddPart(AmkBuf,(s[1]==0)?s+1:s+2,255);
			printf(". = %s\n",AmkBuf);
#endif
			s+=1+(s[1]!=0);
		}
		else if( s[0]=='.' && s[1]=='.' && issep(s[2]))
			*d++='/', s+=2+(s[2]!=0);
		else
			break;
	}
	while( *d++ = *s++ ) ;

	return buf;
}


char *
safegets( char *buf, FILE *in )
{
	char *t;

	if( dobreak() )
		return NULL;

	if( !fgets(buf,249,in) )
		return NULL;

	if( t=index(buf,'\n' ))
		*t=0;

	return buf;
}



/* ------------------------ string array handling -------------------------- */

/*
 *  First call:  pass "*array=NULL" and "*len=0" !!
 */

#define AL_BLOCKSIZE 20

BOOL add_array_list(char ***array, long *len, char *str)
{
	char **new_array = *array;
	long i = *len;
	BOOL success=FALSE;

	if (i%AL_BLOCKSIZE == 0) {
		if (new_array = malloc((i+AL_BLOCKSIZE) * sizeof(char *))) {
			memcpy(new_array,*array,i * sizeof(char *));
			free(*array);			/* free old array */
			*array = new_array;		/* set new array */
		}
		else
			return(success);
	}

	if (new_array[i] = strdup(str)) {
		*len = ++i;				/* update length */
		success = TRUE;
	}

	return(success);
}

void free_array_list(char **array, long len)
{
	long i;
	if (len>0) {
		for (i=0; i<len; i++)
			free(array[i]);
		free(array);
	}
}

/* ------------------------ string array handling -------------------------- */



#ifndef ACTION_SET_OWNER
#define ACTION_SET_OWNER 1036
#endif

BOOL SetOwner37(UBYTE *name, LONG uidgid)
{
	struct DevProc *devproc = NULL;
	BPTR lock;
	char null[] = "\0\0\0\0";	/* longword aligned null-byte */
	BOOL Res = FALSE;

	while(devproc = GetDeviceProc(name, devproc))
	{
		if (lock = Lock(name, SHARED_LOCK))
		{
			Res = DoPkt(devproc->dvp_Port,
				ACTION_SET_OWNER,
				NULL, lock, MKBADDR(null+3), uidgid, NULL);
			UnLock(lock);
			break;
		}
		else if (IoErr() != ERROR_OBJECT_NOT_FOUND)
			break;
	}
	if (devproc)
		FreeDeviceProc(devproc);
	else
		if (IoErr() == ERROR_NO_MORE_ENTRIES)
			SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return Res;
}



long setProtection( char *name, long mask )
{
	long rc;
#ifdef MULTIUSER_SUPPORT
	if (muBase)
		rc = (long)muSetProtection( name, mask );
	else
		rc = (long)SetProtection( name, mask );
#else
	rc = (long)SetProtection( name, mask );
#endif
	return(rc);
}

