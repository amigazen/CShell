/*
 * COMM1.C
 *
 * Matthew Dillon, August 1986
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 * Version 5.60M by amigazen project (Fri, 08 Aug 2025)
 *
 */

#include "shell.h"

/* comm1.c */
static void display_file(char *filestr);
static int search_file( long mask, char *s, char *fullpath );
static int rm_file    ( long mask, char *s, char *fullpath );
static int quicksearch(char *name, int nocasedep, char *pattern);
static void setsystemtime(struct DateStamp *ds);
static int found( char *lstart, int lnum, int loffs, char *name, char left );

void lformat( char *s, char *d, FILEINFO *info );

extern int has_wild;

int
do_sleep( void )
{
	long ticks,secs;

	if (ac == 2) {
		if (options&1) {	/* ticks */
			ticks  = atoi(av[1]);
			secs   = ticks / 50;
			ticks -= secs*50;
		}
		else {	/* seconds */
			secs   = atoi(av[1]);
			ticks  = 0;
		}

		if (ticks>0)
			Delay(ticks);

		while (secs>0 && !CHECKBREAK()) {
			Delay(50);
			secs--;
		}
	}

	return 0;
}

#if 0
/* AMK: if you change this, you must change do_chmod() also!! */
int
do_protect( void )
{
	static char flags[]="DEWRAPSH";
	char *s, *p;
	long setmask=0, clrmask=0xFF, mask;
	int  i, mode=0, stat;
	DPTR *dp;

	for (s=strupr(av[--ac]); *s; s++) {
		if (*s=='=') { mode=0; continue; }
		if (*s=='+') { mode=1; clrmask=FIBF_ARCHIVE; continue; }
		if (*s=='-') { mode=2; clrmask=FIBF_ARCHIVE; continue; }

		if (*s=='X') *s='E';
		if (p=index(flags, *s)) {
			if( mode==0 ) setmask|= 1<<(p-flags), clrmask=0xFF;
			if( mode==1 ) setmask|= 1<<(p-flags);
			if( mode==2 ) clrmask|= 1<<(p-flags);
		} else {
			ierror(av[ac],500);
			return 20;
		}
	}

	for (i=1; i<ac; i++) {
		if( (dp=dopen(av[i],&stat))) {
			mask = dp->fib->fib_Protection ^ 0x0F;
			mask&=~clrmask;
			mask|= setmask;
			dclose(dp);
			if( !setProtection( av[i], mask ^ 0x0F))
				pError(av[i]);
		} else
			pError(av[i]);
	}
	return 0;
}
#endif

/* AMK: same as do_protect, but flags now as first argument */
#if 0
int
do_chmod( void )
{
	static char flags[]="DEWRAPSH";
	char *s, *p;
	long setmask=0, clrmask=0xFF, mask;
	int  i, mode=0, stat;
	DPTR *dp;

	for (s=strupr(av[1]); *s; s++) {       /* AMK: changed */
		if (*s=='=') { mode=0; continue; }
		if (*s=='+') { mode=1; clrmask=FIBF_ARCHIVE; continue; }
		if (*s=='-') { mode=2; clrmask=FIBF_ARCHIVE; continue; }

		if (*s=='X') *s='E';
		if (p=index(flags, *s)) {
			if( mode==0 ) setmask|= 1<<(p-flags), clrmask=0xFF;
			if( mode==1 ) setmask|= 1<<(p-flags);
			if( mode==2 ) clrmask|= 1<<(p-flags);
		} else {
			ierror(av[1],500);     /* AMK: changed */
			return 20;
		}
	}

	for (i=2; i<ac; i++) {                 /* AMK: changed */
		if( (dp=dopen(av[i],&stat))) {
			mask = dp->fib->fib_Protection ^ 0x0F;
			mask&=~clrmask;
			mask|= setmask;
			dclose(dp);
			if( !setProtection( av[i], mask ^ 0x0F))
				pError(av[i]);
		} else
			pError(av[i]);
	}
	return 0;
}
#endif

#define FIBB_HOLD 7
#define FIBF_HOLD (1<<FIBB_HOLD)

int do_chmod_internal(long arg_begin,long arg_end,long arg_flags)
{
	char *s;
	LONG mask;
	int  i, stat;
	DPTR *dp;
	BOOL do_user=FALSE,do_group=FALSE,do_other=FALSE,do_default=TRUE;
	BOOL do_incl=FALSE,do_excl=FALSE,do_set=TRUE;

	/*strlwr(av[1]);*/

	for (i=arg_begin; i<arg_end; i++) {  /* all arguments except 'arg_flags' */

		if( (dp=dopen(av[i],&stat))) {
			mask = dp->fib->fib_Protection;
			dclose(dp);

			s = av[arg_flags];

			if (strchr(s,'+') || strchr(s,'-') || strchr(s,'=')) {

				while (*s && !strchr("+-=",*s)) {
					switch (*s) {
					case 'u': do_user  = TRUE; break;
					case 'g': do_group = TRUE; break;
					case 'o': do_other = TRUE; break;
					case 'a': do_user  =
					          do_group = 
					          do_other = TRUE; break;
					default : ierror(av[1],500); return 20;
					}
					do_default = FALSE;
					++s;
				}

				do_set = FALSE;

				switch (*s) {
				case '+': do_incl = TRUE; break;
				case '-': do_excl = TRUE; break;
				case '=': do_set  = TRUE; break;
				default : ierror(av[1],500); return 20;
				}

				++s;

			}

			if (do_default) {
				do_user=TRUE;
			}

			if (do_set) {
				do_incl = TRUE;
				mask = FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE;
			}

			mask &= (~FIBF_ARCHIVE);

			while (*s) {
				switch (*s) {
				case 'r' :
				           if (do_incl) {
				             if (do_user)  mask &= (~FIBF_READ);
				             if (do_group) mask |= FIBF_GRP_READ;
				             if (do_other) mask |= FIBF_OTR_READ;
				           }
				           else {
				             if (do_user)  mask |= FIBF_READ;
				             if (do_group) mask &= (~FIBF_GRP_READ);
				             if (do_other) mask &= (~FIBF_OTR_READ);
				           }
				           break;
				case 'w' :
				           if (do_incl) {
				             if (do_user)  mask &= (~FIBF_WRITE);
				             if (do_group) mask |= FIBF_GRP_WRITE;
				             if (do_other) mask |= FIBF_OTR_WRITE;
				           }
				           else {
				             if (do_user)  mask |= FIBF_WRITE;
				             if (do_group) mask &= (~FIBF_GRP_WRITE);
				             if (do_other) mask &= (~FIBF_OTR_WRITE);
				           }
				           break;
				case 'e' :
				case 'x' :
				           if (do_incl) {
				             if (do_user)  mask &= (~FIBF_EXECUTE);
				             if (do_group) mask |= FIBF_GRP_EXECUTE;
				             if (do_other) mask |= FIBF_OTR_EXECUTE;
				           }
				           else {
				             if (do_user)  mask |= FIBF_EXECUTE;
				             if (do_group) mask &= (~FIBF_GRP_EXECUTE);
				             if (do_other) mask &= (~FIBF_OTR_EXECUTE);
				           }
				           break;
				case 'd' :
				           if (do_incl) {
				             if (do_user)  mask &= (~FIBF_DELETE);
				             if (do_group) mask |= FIBF_GRP_DELETE;
				             if (do_other) mask |= FIBF_OTR_DELETE;
				           }
				           else {
				             if (do_user)  mask |= FIBF_DELETE;
				             if (do_group) mask &= (~FIBF_GRP_DELETE);
				             if (do_other) mask &= (~FIBF_OTR_DELETE);
				           }
				           break;
				case 'a' :
				           if (do_incl)
				             mask |= FIBF_ARCHIVE;
				           else
				             mask &= (~FIBF_ARCHIVE);
				           break;
				case 'p' :
				           if (do_incl)
				             mask |= FIBF_PURE;
				           else
				             mask &= (~FIBF_PURE);
				           break;
				case 's' :
				           if (do_incl)
				             mask |= FIBF_SCRIPT;
				           else
				             mask &= (~FIBF_SCRIPT);
				           break;
				case 'h' :
				           if (do_incl)
				             mask |= FIBF_HOLD;
				           else
				             mask &= (~FIBF_HOLD);
				           break;
				default  :
				           ierror(av[1],500);
				           return 20;
				}
				++s;
			}

			if( !setProtection( av[i], mask ))
				pError(av[i]);
		}
		else
			pError(av[i]);
	}

	return 0;
}

int do_protect(void)
{
	return do_chmod_internal(1,ac-1,ac-1);
}

int do_chmod(void)
{
	return do_chmod_internal(2,ac,1);
}



int do_chown(void)
{
	LONG mask;  /* 2x UWORD */
	UWORD new_uid,new_gid = 0;
	int  i, stat;
	DPTR *dp;

#ifdef MULTIUSER_SUPPORT
	struct muUserInfo *info;
	struct muUserInfo *res;

	if (muBase) {
		if (info = muAllocUserInfo()) {
			if (isdigit(av[1][0])) {
				new_uid = myatoi(av[1],0,65535);
				if (atoierr) {
					fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
					muFreeUserInfo(info);
					return 20;
				}
				info->uid = new_uid;
				res = muGetUserInfo(info,muKeyType_uid);
				new_gid = info->gid;
			}
			else {
				if (strlen(av[1]) > muUSERIDSIZE-1) {
					fprintf(stderr,"%s: string too long for a User-ID.\n",av[1]);
					muFreeUserInfo(info);
					return 20;
				}
				strcpy(info->UserID,av[1]);
				if (!(res = muGetUserInfo(info,muKeyType_UserID))) {
					fprintf(stderr,"%s: no valid user.\n",av[1]);
					muFreeUserInfo(info);
					return 20;
				}
				new_uid = info->uid;
				new_gid = info->gid;
			}
			muFreeUserInfo(info);
		}
		else {
			fprintf(stderr,"Not enough memory.\n");
			return 20;
		}
	}
	else {
		new_uid = myatoi(av[1],0,65535);
		if (atoierr) {
			fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
			return 20;
		}
	}
#else
	new_uid = myatoi(av[1],0,65535);
	if (atoierr) {
		fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
		return 20;
	}
#endif /* MULTIUSER_SUPPORT */

	for (i=2; i<ac; i++) {  /* all arguments except first */

		if( (dp=dopen(av[i],&stat))) {
			/*printf("%s, gid %d, uid %d\n",av[i],dp->fib->fib_OwnerGID,dp->fib->fib_OwnerUID);*/
			mask = dp->fib->fib_OwnerGID + (new_uid<<16);
			if (options&1)	/* also set GID to primary group */
				mask = new_gid + (new_uid<<16);
			else
				mask = dp->fib->fib_OwnerGID + (new_uid<<16);
			dclose(dp);

			/*printf("  new mask: %ld\n",mask);*/
			if (DOSBase->dl_lib.lib_Version<39) {
				if( !SetOwner37( av[i], mask ))
					pError(av[i]);
			}
			else {
				if( !SetOwner( av[i], mask ))
					pError(av[i]);
			}
		}
		else
			pError(av[i]);
	}

	return 0;
}



int do_chgrp(void)
{
	LONG mask;  /* 2x UWORD */
	UWORD new_gid;
	int  i, stat;
	DPTR *dp;

#ifdef MULTIUSER_SUPPORT
	struct muGroupInfo *info;
	struct muGroupInfo *res;

	if (muBase && !isdigit(av[1][0])) {
		if (info = muAllocGroupInfo()) {
			if (strlen(av[1]) > muGROUPIDSIZE-1) {
				fprintf(stderr,"%s: string too long for a Group-ID.\n",av[1]);
				muFreeGroupInfo(info);
				return 20;
			}
			strcpy(info->GroupID,av[1]);
			if (!(res = muGetGroupInfo(info,muKeyType_GroupID))) {
				fprintf(stderr,"%s: no valid group.\n",av[1]);
				muFreeGroupInfo(info);
				return 20;
			}
			new_gid = info->gid;
			muFreeGroupInfo(info);
		}
		else {
			fprintf(stderr,"Not enough memory.\n");
			return 20;
		}
	}
	else {
		new_gid = myatoi(av[1],0,65535);
		if (atoierr) {
			fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
			return 20;
		}
	}
#else
	new_gid = myatoi(av[1],0,65535);
	if (atoierr) {
		fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
		return 20;
	}
#endif /* MULTIUSER_SUPPORT */

	for (i=2; i<ac; i++) {  /* all arguments except first */

		if( (dp=dopen(av[i],&stat))) {
			/*printf("%s, gid %d, uid %d\n",av[i],dp->fib->fib_OwnerGID,dp->fib->fib_OwnerUID);*/
			mask = (dp->fib->fib_OwnerUID<<16) + new_gid;
			dclose(dp);

			/*printf("  new mask: %ld\n",mask);*/
			if (DOSBase->dl_lib.lib_Version<39) {
				if( !SetOwner37( av[i], mask ))
					pError(av[i]);
			}
			else {
				if( !SetOwner( av[i], mask ))
					pError(av[i]);
			}
		}
		else
			pError(av[i]);
	}

	return 0;
}



#if 0
int do_chown(void)
{
	LONG mask;  /* 2x UWORD */
	UWORD new_id;
	int  i, stat;
	DPTR *dp;

	new_id = myatoi(av[1],0,65535);
	if (atoierr) {
		fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
		return 20;
	}

	for (i=2; i<ac; i++) {  /* all arguments except first */

		if( (dp=dopen(av[i],&stat))) {
			/*printf("%s, gid %d, uid %d\n",av[i],dp->fib->fib_OwnerGID,dp->fib->fib_OwnerUID);*/
			mask = dp->fib->fib_OwnerGID + (new_id<<16);
			dclose(dp);

			/*printf("  new mask: %ld\n",mask);*/
			if (DOSBase->dl_lib.lib_Version<39) {
				if( !SetOwner37( av[i], mask ))
					pError(av[i]);
			}
			else {
				if( !SetOwner( av[i], mask ))
					pError(av[i]);
			}
		}
		else
			pError(av[i]);
	}

	return 0;
}



int do_chgrp(void)
{
	LONG mask;  /* 2x UWORD */
	UWORD new_id;
	int  i, stat;
	DPTR *dp;

	new_id = myatoi(av[1],0,65535);
	if (atoierr) {
		fprintf(stderr,"UID/GID is a 16-bit wide UWORD (0-65535) !\n");
		return 20;
	}

	for (i=2; i<ac; i++) {  /* all arguments except first */

		if( (dp=dopen(av[i],&stat))) {
			/*printf("%s, gid %d, uid %d\n",av[i],dp->fib->fib_OwnerGID,dp->fib->fib_OwnerUID);*/
			mask = (dp->fib->fib_OwnerUID<<16) + new_id;
			dclose(dp);

			/*printf("  new mask: %ld\n",mask);*/
			if (DOSBase->dl_lib.lib_Version<39) {
				if( !SetOwner37( av[i], mask ))
					pError(av[i]);
			}
			else {
				if( !SetOwner( av[i], mask ))
					pError(av[i]);
			}
		}
		else
			pError(av[i]);
	}

	return 0;
}
#endif



int
do_filenote( void )
{
	DPTR *dp;
	char *note;
	int i, stat;

	if( options&1 ) {
		for( i=1; i<ac && !dobreak(); i++ )
			if( dp=dopen( av[i], &stat )) {
				printf( "%-12s %s\n", av[i],dp->fib->fib_Comment );
				dclose( dp );
			}
	} else {
		note=av[--ac];
		for (i=1; i<ac; i++) if (!SetComment(av[i], note)) pError(av[i]);
	}
	return 0;
}



long fgetsn(unsigned char *buf, unsigned long buflen, FILE *fp)
{
	long read = 0;
	int c = 0;
	BOOL go_on = TRUE;

	if (fp != NULL && buflen > 1) {
		buflen--;
		while (read < buflen && go_on) {
			c = fgetc(fp);
			if (c == '\n') {
				buf[read++] = c;
				buf[read] = '\0';
				go_on = FALSE;
			}
			else if (c == EOF) {
				buf[read] = '\0';
				go_on = FALSE;
			}
			else {
				buf[read++] = c;
				buf[read] = '\0';
			}
		}
	}

	return(read);
}



#if 1
int do_cat( void )
{
	/*
	 *  This command is unable to cat binary files like UNIX cat does,
	 *  because this command heavily relies on text-based line structure
	 *  of a file and cannot handle 0 characters.
	 *
	 *  The only solution would be to re-program fgets() to return the
	 *  number of bytes read and write off each chunk of bytes with
	 *  fwrite() instead of fputs().
	 *
	 *  NOTE: Now binary clean!, -amk[19950405]
	 */
	FILE *fi;
	long read;
	int lctr, i, docr = 0;
	char buf[256];

	prepscroll(0);
	if (ac<=1) {
		if (has_wild) { printf("No files matching\n"); return 20; }
		lctr=0;
		while ((read=fgetsn(buf,256,stdin))>0 && !dobreak()) {
			if (options) printf("%4d ",++lctr);
			quickscroll();
			fwrite(buf,1L,read,stdout);
			fflush(stdout);
			docr = strchr(buf, '\n')? 0 : 1;
		}
	} else {
		for (i=1; i<ac; i++)
			if (fi = fopen (av[i], "r")) {
				lctr=0;
				while ((read=fgetsn(buf,256,fi))>0 && !dobreak()) {
					if (options&1) printf("%4d ",++lctr);
					quickscroll();
					fwrite(buf,1L,read,stdout);
					fflush(stdout);
					docr = strchr(buf, '\n')? 0 : 1;
				}
				fclose (fi);
			} else
				pError(av[i]);
	}
	if ( docr && IsInteractive(Output()) )
		putchar('\n');
	return 0;
}
#else
int do_cat( void )
{
	/*
	 *  This command is unable to cat binary files like UNIX cat does,
	 *  because this command heavily relies on text-based line structure
	 *  of a file and cannot handle 0 characters.
	 *
	 *  The only solution would be to re-program fgets() to return the
	 *  number of bytes read and write off each chunk of bytes with
	 *  fwrite() instead of fputs().
	 */
	FILE *fi;
	int lctr, i, docr=0;
	char buf[256], *l;

	prepscroll(0);
	if (ac<=1) {
		if (has_wild) { printf("No files matching\n"); return 20; }
		lctr=0;
		while (fgets(buf,256,stdin) && !dobreak()) {
			if (options) printf("%4d ",++lctr);
			quickscroll();
			l=buf+strlen( buf )-1; docr=1;
			if( l>=buf && *l=='\n' ) docr=0;
			fputs(buf,stdout);
		}
	} else {
		for (i=1; i<ac; i++)
			if (fi = fopen (av[i], "r")) {
				lctr=0;
				while (fgets(buf,256,fi) && !dobreak()) {
					if (options&1) printf("%4d ",++lctr);
					quickscroll();
					l=buf+strlen( buf )-1; docr=1;
					if( l>=buf && *l=='\n' ) docr=0;
					fputs(buf,stdout); fflush(stdout);
				}
				fclose (fi);
			} else
				pError(av[i]);
	}
	if ( docr && IsInteractive(Output()) )
		putchar('\n');
	return 0;
}
#endif



char *add_simple_device(char *list,char *dev)
{
	char *new = NULL;

	if (list) {
		if (new = malloc(strlen(dev)+strlen(list)+2)) {		/* null byte + \n */
			strcpy(new,list);
			strcat(new,dev);
			strcat(new,"\n");
			free(list);
		}
		else
			new = list;
	}
	else {
		if (new = malloc(strlen(dev)+2)) {			/* null byte + \n */
			strcpy(new,dev);
			strcat(new,"\n");
		}
	}

	return(new);
}



void
get_drives(char *buf)
{
	struct DosList *dl;
	ULONG flags = LDF_DEVICES|LDF_READ;
	char devname[256];
	char **dev_list=NULL;
	long i,dev_num=0;

	buf[0]=0;
	if (dl=LockDosList(flags)) {
		while (dl=NextDosEntry(dl,flags)) {
			if (dl->dol_Task) {
				BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
				strcat(devname,":");
				add_array_list(&dev_list,&dev_num,devname);
			}
		}
		UnLockDosList(flags);
	}

	QuickSort(dev_list,dev_num);

	for(i=0; i<dev_num; i++) {
		if (IsFileSystem(dev_list[i])) {
			if (buf[0])
				strcat(buf,"\240");
			strcat(buf,dev_list[i]);
		}
	}

	free_array_list(dev_list,dev_num);
}

static char infobuf[100];
static char namebuf[32];  /* AMK: old size was 12, too small for drive names */


/* AMK: find last occurence of a character in a string */
char *strlast(char *s,char c)
{
  char *p=NULL;
  while(*s) {
    if(*s==c)
      p=s;
    s++;
  }
  return(p);
}


char *
drive_name( char *name )
{
	struct DosList *dl;
	struct MsgPort *proc= (struct MsgPort *)DeviceProc( (void*)name );
	ULONG flags = LDF_DEVICES|LDF_READ;
	char devname[256];
	char **dev_list=NULL;
	long i,dev_num=0;

	/* AMK: we want no self-modifying code */
	strncpy( namebuf, name, 31 );   /* AMK: 30 chars device name + ':' */
	namebuf[31] = '\0';             /* AMK: null-terminated */
	if (dl=LockDosList(flags)) {
		while (dl=NextDosEntry(dl,flags)) {
			if (dl->dol_Task) {
				BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
				strcat(devname,":");
				add_array_list(&dev_list,&dev_num,devname);
			}
		}
		UnLockDosList(flags);
	}

	QuickSort(dev_list,dev_num);

	for(i=0; i<dev_num; i++) {
		if (IsFileSystem(dev_list[i])) {
			if ((struct MsgPort *)DeviceProc(dev_list[i])==proc)
				strcpy(namebuf,dev_list[i]);
		}
	}

	free_array_list(dev_list,dev_num);

	return namebuf;
}

int
do_info( void )
{
	struct DosList *dl;
	ULONG flags = LDF_DEVICES|LDF_READ;
	char devname[256];
	char **dev_list=NULL;
	long i,dev_num=0;

	if (options&2)
		puts("Unit     Size  Block  Type   Used   Free Full Errs  Status    Name");
	else
		puts("Unit     Size  Bytes  Used Blk/Byte-Free Full Errs  Status    Name");

	if( ac==1 ) {
		if (dl=LockDosList(flags)) {
			while (dl=NextDosEntry(dl,flags)) {
				if (dl->dol_Task) {
					BtoCStr(devname,dl->dol_Name,254L);  /* 256 - '\0' + ':' */
					strcat(devname,":");
					add_array_list(&dev_list,&dev_num,devname);
				}
			}
			UnLockDosList(flags);
		}

		QuickSort(dev_list,dev_num);

		for(i=0; !dobreak() && i<dev_num; i++) {
			if (IsFileSystem(dev_list[i])) {
				oneinfo(dev_list[i],0);
			}
		}

		free_array_list(dev_list,dev_num);
	}
	else {
		for( i=1; i<ac; i++ )
			oneinfo( drive_name( av[i] ), 0 );
	}

	return 0;
}



/* these defines are new in OS 3.x */
#ifndef ID_FASTDIR_DOS_DISK
#define ID_FASTDIR_DOS_DISK (0x444F5304L)
#endif
#ifndef ID_FASTDIR_FFS_DISK
#define ID_FASTDIR_FFS_DISK (0x444F5305L)
#endif



/* AMK: new mode==6 to suppress output if disk is not present */
char *
oneinfo( char *name, int mode )
{
	struct InfoData *info;
	struct DevProc *devproc;
	struct DeviceList *dl;
	BPTR lock;
	long size, free, freebl, blocks;
	char buf[130], *state, *type;
	char *fmt="%s\240%s\240%d\240%d\240%d\240%s\240%d%%\240%d\240%s\240%s";

	Myprocess->pr_WindowPtr = (APTR)(-1);

	if (!name) name="";

	if (mode<=1 || mode>=5)
		strcpy(infobuf,"");
	else
		strcpy(infobuf,"0");

	info=(struct InfoData *)SAllocMem(sizeof(struct InfoData),MEMF_PUBLIC|MEMF_CLEAR);

	if (devproc=GetDeviceProc(name,NULL)) {
		if (DoPkt(devproc->dvp_Port,ACTION_DISK_INFO,MKBADDR(info),NULL,NULL,NULL,NULL)==DOSTRUE) {
			BOOL go_on = FALSE;
			char *spclfmt;
#if 0
			if (!NameFromLock(lock, buf, 128L)) {
				fprintf(stderr,"csh.oneinfo: NameFromLock() failed\n");
				strcpy(buf,name);
			}
			if (p=strlast(buf,':')) *p = '\0';
			/* AMK: we want the last occurence of ':', not the first;
			        ':' and '/' are legal path name components !!
			        (bug only in RAM: disk)
			if (p=index(buf,':')) *p = '\0';
			*/
#endif
			switch (mode) {
				case 0:
					if (options&1)
						spclfmt = "";
					else
						spclfmt = "%-7s %s\n";
					break;
				case 1:
					spclfmt = "%s\240%s\n";
					break;
				case 2:
				case 3:
				case 4:
					spclfmt = "0";
					break;
				case 5:
					spclfmt = "";
					break;
				default:
					spclfmt = "";
					break;
			}

			switch (info->id_DiskType) {
				case ID_UNREADABLE_DISK:
					sprintf(infobuf,spclfmt,name,"Unreadable disk");
					break;
				case ID_NOT_REALLY_DOS:
					sprintf(infobuf,spclfmt,name,"Not a DOS disk");
					break;
				case ID_KICKSTART_DISK:
					sprintf(infobuf,spclfmt,name,"Kickstart disk");
					break;
				case (0x42555359L):  /* 'BUSY' */
					sprintf(infobuf,spclfmt,name,"Disk is busy");
					break;
				case ID_NO_DISK_PRESENT:
					sprintf(infobuf,spclfmt,name,"No disk present");
					break;
				default:
					sprintf(infobuf,spclfmt,name,"Unknown disk type");
					go_on = TRUE;
					break;
			}

			if (go_on && (lock=Lock(name,ACCESS_READ))) {
				UnLock(lock);
				/* note:  we call Lock() to be sure that the volume is readable */

				switch(info->id_DiskType) {
					case ID_MSDOS_DISK:       type=" MSDOS"; break;
					case ID_DOS_DISK:         type="   OFS"; break;
					case ID_FFS_DISK:         type="   FFS"; break;
					case ID_INTER_DOS_DISK:   type="IN/OFS"; break;
					case ID_INTER_FFS_DISK:   type="  INTL"; break;
					case ID_FASTDIR_DOS_DISK: type="DC/OFS"; break;
					case ID_FASTDIR_FFS_DISK: type="  DCFS"; break;
					default:                  type="   n/a"; break;
				}

				strcpy(buf,"n/a");
				if (dl = (struct DeviceList *)BADDR(info->id_VolumeNode))
					BtoCStr(buf,dl->dl_Name,100L);

				switch(info->id_DiskState) {
					case ID_WRITE_PROTECTED: state="Read Only "; break;
					case ID_VALIDATED:       state="Read/Write"; break;
					case ID_VALIDATING:      state="Validating"; break;
					default:                 state="Unknown   "; break;
				}

#if 0
				size   = (info->id_NumBlocks + 2) * info->id_BytesPerBlock;
#endif
				size   = info->id_NumBlocks * info->id_BytesPerBlock;
				freebl = info->id_NumBlocks - info->id_NumBlocksUsed;
				free   = freebl * info->id_BytesPerBlock;
				blocks = info->id_NumBlocks;
#if 0
				printf("percents: %s... %ld, %ld, %ld, %ld, %ld\n",name,
					((info->id_NumBlocksUsed+2) * 100)/blocks,
					(info->id_NumBlocksUsed * 100)/(blocks+2),
					(((info->id_NumBlocksUsed * 1000)/blocks)+5)/10,
					((((info->id_NumBlocksUsed+2) * 1000)/blocks)+5)/10,
					(((info->id_NumBlocksUsed * 1000)/(blocks+2))+5)/10
				);
#endif

				if (mode==0 && options&2) {
					fmt="%-7s%6s%6d %6s %6s %6s %3d%%%4d  %10s %s\n";
					sprintf(infobuf,fmt,
						name,
						itok( size ),
						info->id_BytesPerBlock,
						type,
						itok( info->id_NumBlocksUsed*info->id_BytesPerBlock ),
						itok( free ),
						(blocks) ? (int)((( (double)info->id_NumBlocksUsed/(double)blocks ) * 1000.0 + 5.0) / 10.0) : 0,
#if 0
						(blocks) ? ((info->id_NumBlocksUsed * 1000)/blocks + 5) / 10 : 0,
#endif
						info->id_NumSoftErrors,
						state,
						buf);
				}
				else if (mode<=1) {
					if (mode==0) fmt="%-7s%6s%6d%7d%7d %6s%4d%%%4d  %s %s\n";
					sprintf(infobuf,fmt,
						name,
						itok( size ),
						info->id_BytesPerBlock,
						info->id_NumBlocksUsed,
						freebl,
						itok( free ),
						(blocks) ? (int)((( (double)info->id_NumBlocksUsed/(double)blocks ) * 1000.0 + 5.0) / 10.0) : 0,
						info->id_NumSoftErrors,
						state,
						buf);
				}
				else if (mode==2) sprintf(infobuf,"%d",free);
				else if (mode==3) sprintf(infobuf,"%d",freebl);
				else if (mode==4) sprintf(infobuf,"%s",itok(free));
				else if (mode==5) sprintf(infobuf,"%s:",buf);
			}
		}
		else
			pError(name);
		FreeDeviceProc(devproc);
	}

#if 0
	else {
		if (mode==1) {
			struct MsgPort *devtask;
			sprintf(infobuf,"%s\240No disk present\n",name);
			if (devtask=(struct MsgPort *)DeviceProc(name)) {
				struct InfoData *infodata;
				infodata = SAllocMem(sizeof(struct InfoData),MEMF_PUBLIC|MEMF_CLEAR);
				if (DoPkt(devtask,ACTION_DISK_INFO,MKBADDR(infodata),NULL,NULL,NULL,NULL)) {
					switch (infodata->id_DiskType) {
					case ID_UNREADABLE_DISK:
						sprintf(infobuf,"%s\240Unreadable disk\n",name);
						break;
					case ID_NOT_REALLY_DOS:
						sprintf(infobuf,"%s\240Not a DOS disk\n",name);
						break;
					case ID_KICKSTART_DISK:
						sprintf(infobuf,"%s\240Kickstart disk\n",name);
						break;
					case (0x42555359L):  /* 'BUSY' */
						sprintf(infobuf,"%s\240Busy\n",name);
						break;
					case ID_NO_DISK_PRESENT:
					default:
						sprintf(infobuf,"%s\240No disk present\n",name);
						break;
					}
				}
				FreeMem(infodata,sizeof(struct InfoData));
			}
		}
		else if (mode==0) {
			if (options&1) {
				sprintf(infobuf,"");
			}
			else {
				struct MsgPort *devtask;
				sprintf(infobuf,"%-7s No disk present\n",name);
				if (devtask=(struct MsgPort *)DeviceProc(name)) {
					struct InfoData *infodata;
					infodata = SAllocMem(sizeof(struct InfoData),MEMF_PUBLIC|MEMF_CLEAR);
					if (DoPkt(devtask,ACTION_DISK_INFO,MKBADDR(infodata),NULL,NULL,NULL,NULL)) {
						switch (infodata->id_DiskType) {
						case ID_UNREADABLE_DISK:
							sprintf(infobuf,"%-7s Unreadable disk\n",name);
							break;
						case ID_NOT_REALLY_DOS:
							sprintf(infobuf,"%-7s Not a DOS disk\n",name);
							break;
						case ID_KICKSTART_DISK:
							sprintf(infobuf,"%-7s Kickstart disk\n",name);
							break;
						case (0x42555359L):  /* 'BUSY' */
							sprintf(infobuf,"%-7s Disk is busy\n",name);
							break;
							case ID_NO_DISK_PRESENT:
						default:
							sprintf(infobuf,"%-7s No disk present\n",name);
							break;
						}
					}
					FreeMem(infodata,sizeof(struct InfoData));
				}
			}
		}
		else if (mode==5) sprintf(infobuf,"");
		else              sprintf(infobuf,"0");
	}
#endif

	if (mode==0) printf("%s",infobuf);

	FreeMem(info,sizeof(struct InfoData));
	Myprocess->pr_WindowPtr = o_noreq ? (APTR) -1L : 0L/*Mywindow*/;

	return infobuf;
}


/* things shared with display_file */

#define DIR_SHORT 0x1
#define DIR_FILES 0x2
#define DIR_DIRS  0x4
#define DIR_NOCOL 0x8
#define DIR_NAMES 0x10
#define DIR_HIDE  0x20
#define DIR_LEN   0x40
#define DIR_TIME  0x80
#define DIR_BACK  0x100
#define DIR_UNIQ  0x200
#define DIR_IDENT 0x400
#define DIR_CLASS 0x800
#define DIR_QUIET 0x1000
#define DIR_AGE   0x2000
#define DIR_VIEW  0x4000
#define DIR_NOTE  0x8000
#define DIR_PATH  0x10000
#define DIR_LFORM 0x20000
#define DIR_BOT   0x40000
#define DIR_TOP   0x80000
#define DIR_LINK  0x100000

static char *lastpath = NULL;
static int filecount, dircount, col, colw, wwidth;
static long bytes, blocks;

/* the args passed to do_dir will never be expanded */

extern expand_err;
extern int w_width;

static struct DateStamp Stamp;
static char *LineBuf, *LinePos, LastWasDir, *LFormat, _LFormat[80], NoTitles;

int
do_dir( void )
{
	int i=1, c, eac, reverse, nump=ac, retcode=0;
	char **eav=NULL, **av1=NULL, **av2=NULL, inter=IsInteractive(Output());
	char linebuf[1024];
	char *fmtstr;
	int (*func)(), ac1, ac2, factor=0;

	LineBuf=LinePos=linebuf;
	LastWasDir=NoTitles=0;
	colw = -1;

	LFormat=_LFormat;

	if( options&DIR_CLASS ) options|=DIR_IDENT;
	if( !(options & (DIR_FILES | DIR_DIRS))) options|= DIR_FILES | DIR_DIRS;

	DateStamp( &Stamp );

	col = filecount = dircount = bytes = blocks = 0L;
	if (lastpath) free(lastpath);
	lastpath=NULL;

	wwidth=77;
	if( inter )
		wwidth=w_width;

	if( options&DIR_SHORT )
		strcpy(LFormat," %-18n%19m");
	else if( options&DIR_PATH )
		strcpy(LFormat," %-50p %7s %d"), NoTitles=1;
	else {
		if ( options&DIR_NOTE )
			strcpy(LFormat,"  %-30n %o");
		else if ( options&DIR_LINK )
			strcpy(LFormat,"  %-30n %L");
		else {
#if 1
			strcpy(LFormat,"  ");
#else
			strcpy(LFormat,"  %-30n ");
#endif
			if( options&DIR_HIDE )
				strcat(LFormat, "%e");
			strcat(LFormat,"%c%I%f ");
			if( options&DIR_VIEW )
				strcat(LFormat,"%10v  ");
			else
				strcat(LFormat,"%8s  ");
#if 0
			if( !(options&DIR_QUIET) )
				strcat(LFormat,options&DIR_VIEW?"%5b ":"%4b ");
#endif
			if( options&DIR_IDENT )
				strcat(LFormat,"%-10k");
			else if( options&DIR_AGE )
				strcat(LFormat,"%a");
			else
				strcat(LFormat,"%d %t");
#if 1
			strcat(LFormat,"  %N");
#endif
		}
	}

#if 1
	/* if option -z, use variable _dirformat or first argument (if no _dirformat) */

	if ( options&DIR_LFORM ) {
		if ( fmtstr=get_var(LEVEL_SET,v_dirformat) ) {
			strncpy(LFormat,fmtstr,79);	/* copy to _LFormat[80] */
			LFormat[79]=0;
		}
		else if (ac>1)
			LFormat=av[i++];
		else {
			show_usage(NULL);
			return 20;
		}
	}
#else
	/* variable _dirformat always used, can be overwritten by -z fmt */

	if ( fmtstr=get_var(LEVEL_SET,v_dirformat) ) {
		strncpy(LFormat,fmtstr,79);	/* copy to _LFormat[80] */
		LFormat[79]=0;
	}

	if ( options&DIR_LFORM ) {
		if (ac>1)
			LFormat=av[i++];
		else {
			show_usage(NULL);
			return 20;
		}
	}
#endif

	if( ac == i) ++nump, av[i]="";
	if( options&DIR_UNIQ) {
		if( ac-i!=2 )  { show_usage(NULL); return 20; }
		i=0, nump=3;
	}

	prepscroll(0);
	for( ; i<nump && !CHECKBREAK(); ++i ) {
		if( options&DIR_UNIQ ) {
			switch( i ) {
				case 0: av1=expand( av[ac-2], &ac1 );
				        av2=expand( av[ac-1], &ac2 );
				        eav=without( av1, ac1, av2, ac2, &eac, 1 );
				        break;
				case 1: printf("\nCommon files\n");
				        eav=and( av1, ac1, av2, ac2, &eac, 1 );
				        break;
				case 2: printf("\n");
				        eav=without( av2, ac2, av1, ac1, &eac, 1 );
				        break;
			}
			col = filecount = dircount = bytes = blocks = 0L;
			if (lastpath) free(lastpath);
			lastpath=NULL;

		/* AMK: enhanced handling of non-matching patterns */
		} else if (!(eav = expand(av[i], &eac))) {
			if (IoError) {
				ierror(av[i],IoError);
				retcode=5;
			}
#if 0
			else {
				if (strlen(av[i])>0)
					fprintf(stderr,"%s: No match.\n",av[i]);
			}
#endif
			continue;
		}

		reverse= ( options&DIR_BACK ) ? 1 : 0;
		func=cmp;
		if( options & DIR_TIME) func   = datecmp_csh;
		if( options & DIR_LEN ) func   = sizecmp;
		if( options & DIR_CLASS)func   = classcmp;
		if( options & DIR_BOT ) factor = -99999999;
		if( options & DIR_TOP ) factor = 99999999;
		DirQuickSort(eav, eac, func, reverse, factor);
		for(c=0; c<eac && !CHECKBREAK(); ++c) {
			if( options & DIR_HIDE ) {
				char *b=FilePart(eav[c]);
				int  l=strlen(b)-5;
				FILEINFO *info =(FILEINFO *)eav[c] - 1;
				if(*b=='.'|| (l>=0 && !strcmp(b+l,".info"))||(info->flags&128))
					continue;
			}
			if (options & DIR_NAMES) {
				FILEINFO *info = (FILEINFO *)eav[c] - 1;
				if(options&(info->size<0 ? DIR_DIRS: DIR_FILES))
					puts(eav[c]);
			} else
				display_file(eav[c]);
		}

		if (col) { quickscroll(); puts(LinePos=LineBuf); col=0; }

		if( LastWasDir )
			printf(o_lolite), LastWasDir=0;

		if (options&DIR_UNIQ || (filecount>1 && i==nump-1)) {
			blocks += filecount-dircount; /* account for dir blocks */
			quickscroll();
			printf(" %ld Blocks, %s Bytes used in %d files\n",
			    blocks, itoa(bytes), filecount);
		}
		if( options&DIR_UNIQ )
			free(eav);
		else
			free_expand (eav);
	}
	if (lastpath) free(lastpath);
	lastpath=NULL;

	if( options&DIR_UNIQ )
		free_expand( av1 ), free_expand( av2 );

	return retcode;
}

static int MultiCol = -1;

static char
pathcomp( char *s1, char *s2 )
{
	char ret, *t, c;

	t=FilePart( s2 ); c = *t; *t=0;
	ret=stricmp( s1, s2 );
	*t=c;
	return ret;
}

static void
display_file( char *filestr )
{
	/* struct InfoData *id=AllocMem( sizeof(struct InfoData), 0); */
	int isadir, len, collen;
	char sc, *base, buf[1024], *hilite;
	FILEINFO *info;
	BPTR thislock;

	base=FilePart(filestr);
	sc = *base;
	*base = 0;
	/* if (thislock==NULL) return; */
	if( !NoTitles ) {
		if( !lastpath || pathcomp( filestr, lastpath)) {
			if(!(thislock=Lock(filestr,SHARED_LOCK)))
				return;
			if (col) { quickscroll(); puts(LinePos=LineBuf); col=0; }
			quickscroll();
			if (!NameFromLock(thislock, buf, 256)) {
				fprintf(stderr,"csh.display_file: NameFromLock() failed\n");
				strcpy(buf,filestr);
			}
			if( LastWasDir )
				printf(o_lolite), LastWasDir=0;
			printf("Directory of %s\n", buf );
			/* Info( thislock, id ); */
			/* itok((id->id_NumBlocks-id->id_NumBlocksUsed)*id->id_BytesPerBlock));*/
			/* FreeMem( id, sizeof(struct InfoData)); */
			lastpath = salloc(256);
			strcpy(lastpath,filestr);
			/*lastpath=filestr;*/
			UnLock(thislock);
		}
	}
	*base    = sc;

	info   = (FILEINFO *)filestr - 1;
	isadir = info->size<0;

	if( !(options & (isadir ? DIR_DIRS : DIR_FILES)))
		return;

	hilite="";
	if (isadir!=LastWasDir && !(options & DIR_NOCOL))
		hilite=isadir ? o_hilite : o_lolite, LastWasDir=isadir;

	lformat(LFormat, buf, info);

	if( MultiCol == -1 ) {
		quickscroll();
		printf("%s%s",hilite,buf);
	} else {
		len=strlen(buf);
		if( col+len>wwidth ) {
			quickscroll();
			puts(LineBuf);
			LinePos=LineBuf; col=0;
		}
		if( MultiCol )
			colw=MultiCol;
		else if( colw == -1 )
			colw=len;
		collen= (len+colw-1)-(len+colw-1)%colw;
		col+=collen;
		LinePos+=sprintf(LinePos,"%s%-*s",hilite,collen,buf);
	}

	if(info->size>0)
		bytes  += info->size;
	blocks += info->blocks;
	filecount++;
}

static char linebuf[1024];
static long dlen, dblocks;

static int
count( long mask, char *s, char *path )
{
	FIB *fib=(FIB*)s-1;
	dlen+=fib->fib_Size;
	dblocks+=fib->fib_NumBlocks+1;
	return 0;
}


/* code contribution by Carsten Heyl, modified by AMK */
void ReadSoftLink(char *path, char *buf, int buflen)
{
	BPTR MyLock;
	BPTR DevLock;
	LONG Err, l;
	UBYTE *bs;
	struct MsgPort *port;

	if (buflen>9)
		strcpy(buf,"<unknown>");
	else
		buf[0] = '\0';	/* just terminate buffer. alternative ? */

	if ( (l=strlen(path)) > 254 )
		return;

	if (!(bs=AllocMem(256,MEMF_CLEAR|MEMF_PUBLIC)))
		return;

	bs[255] = '\0';

	/* build BCPL string */
	bs[0] = l;
	memcpy(&bs[1], path, l+1);

	/* GetDeviceProc or DeviceProc? */
	if (!(port = DeviceProc(path))) {
		FreeMem(bs,256);
		return;
	}

	DevLock = (BPTR)IoErr();

	MyLock = DoPkt(port,ACTION_LOCATE_OBJECT,DevLock,MKBADDR(bs),
			    ACCESS_READ,NULL,NULL);
	Err = IoErr();

	if (!MyLock && (Err==ERROR_IS_SOFT_LINK)) {
#if 0
		ReadLink(port,DevLock,(UBYTE *)path,(UBYTE *)buf,buflen);
#else
		DoPkt(port,ACTION_READ_LINK,DevLock,(LONG)path,(LONG)buf,buflen,NULL);
#endif
	}

	if (MyLock) UnLock(MyLock);
	FreeMem(bs,256);
}


void
lformat( char *s, char *d, FILEINFO *info )
{
	long mi=0;
	char buf[1024], *w, *class;
	DPTR *dp;
	int stat, i, form, sign, cut, size=info->size;
	char *(*func)(int num);

	MultiCol = -1;
	while( *s ) {
		if( *s!='%' ) { *d++ = *s++; continue; }
		sign=1; form=0; cut=0; s++;
		if( *s=='-' ) s++, sign = -1;
		if( *s=='.' ) s++, cut=1;
		while( *s>='0' && *s<='9' ) form=10*form+*s++-'0';
		w=buf; w[0]=0; w[1]=0;
		switch( *s ) {
		case 'p': strcpy(w,(char *)(info+1));             break;
		case 'b': sprintf(w,size>=0 ? "%d":"", info->blocks); break;
		case 's': sprintf(w,size>=0 ? "%d":"<Dir>",size); break;
		case 'i': *w= size>=0 ? '-' : 'd';                break;
		case 'r':
		case 'u':
			if( *s=='r' ) func=itoa; else func=itok;
			strcpy( w,size>=0 ? (*func)(size) : "<Dir>");
			break;
		case 'n':
		case 'q':
			strcpy(w,FilePart((char *)(info+1)));
			if( *s=='q' && size<0 ) strcat(w,"/");
			break;
		case 'l':
			if( info->flags & INFO_COMMENT ) *w='\n';
			break;
		case 'c':
			*w= info->flags & INFO_COMMENT ? 'c' : '-';
			break;
		case 'e':
			*w= info->flags & INFO_INFO ? 'i' : '-';
			break;
		case '+':
			*w= info->flags & INFO_INFO ? '+' : ' ';
			break;
		case 'L':
		case 'N':
			if (*s=='N')
				strcpy(w,FilePart((char *)(info+1)));
			else
				strcpy(w,"");
			if (info->type==ST_SOFTLINK) {
				strcat(w," -> ");
				ReadSoftLink((char *)(info+1),w+strlen(w),256);
			}
			else if (info->type==ST_LINKDIR || info->type==ST_LINKFILE) {
				BPTR lock;
				if (lock=Lock((char *)(info+1),ACCESS_READ)) {
					strcat(w," -> ");
					if (!NameFromLock(lock,w+strlen(w),256)) {
						fprintf(stderr,"csh.lformat: NameFromLock() failed\n");
						strcpy(w+strlen(w),(char *)(info+1));
					}
					UnLock(lock);
				}
			}
			break;
		case 'I':
			switch (info->type) {
				case ST_SOFTLINK:
					*w='S';
					break;
				case ST_LINKDIR:
				case ST_LINKFILE:
					*w='H';
					break;
				case ST_PIPEFILE:
					*w='P';
					break;
				default:
					*w='-';
					break;
			}
			break;
		case 'f': /* standard flags */
			for (i=7; i>=0; i--)
				*w++ = (info->flags^15) & (1L<<i) ? "hsparwed"[7-i] : '-';
			*w=0;
			break;
		case 'F': /* group/other flags */
			for (i=3; i>=0; i--)
				*w++ = (info->flags^15) & (1L<<(i+8)) ? "rwed"[3-i] : '-';
			*w++ = ' ';
			for (i=3; i>=0; i--)
				*w++ = (info->flags^15) & (1L<<(i+12)) ? "rwed"[3-i] : '-';
			*w=0;
			break;
		case 'U': /* user-id */
			sprintf(w,"%d",info->uid);
			break;
		case 'G': /* group-id */
			sprintf(w,"%d",info->gid);
			break;
		case 'a':
			if( Stamp.ds_Days!=0 ) {
				mi =Stamp.ds_Days*1440 + Stamp.ds_Minute;
				mi-=info->date.ds_Days*1440 + info->date.ds_Minute;
			}
			sprintf(w,mi>=0?"%4d days %02d:%02d":"Future    ",
			          mi/1440,mi/60%60,mi%60);
			break;
		case 'o':
			if( dp=dopen( (char *)(info+1), &stat )) {
				strcpy( w, dp->fib->fib_Comment );
				dclose( dp );
			}
			break;
		case 'v':
		case 'w':
			if( *s=='v' ) func=itoa; else func=itok;
			dlen=dblocks=0;
			if( size<0 ) {
				newrecurse( SCAN_DIR|SCAN_FILE|SCAN_RECURSE,
				            (char *)(info+1),count);
				strcpy( w, (*func)(dlen));
				info->size=size=dlen; info->blocks=dblocks;
			} else
				strcpy( w, (*func)(size));
			break;
		case 'k':
			if( *info->class!=1 )
				strcpy(w,info->class);
			else if( class=getclass((char *)(info+1)))
				if( w=index(strncpy(w,class,50),0xA0) )
					*w=0;
			break;
		case 'x':
		case 'd':
			sprintf(w,"%9.9s",dates(&info->date,*s=='x'));
#if 0
/*
 *  the sprint() already relies on the fact that the date string
 *  is exactly 9 characters long, so we don't need to search for
 *  spaces or do other fancy stuff ;-)
 */
			/* strip off after first space */
			if (t=strchr(w,' '))
				*t='\0';
#endif
			break;
		case 't':
			sprintf(w,"%8s", next_word(dates(&info->date,0)));
			break;
		case 'm': MultiCol = form; form = 0;    break;
		case '%': *w = *++s;                    break;
		case  0 : *w = '%';                     break;
		default : *w = '%';  *w++ = *s; *w = 0; break;
		}
		if( cut ) buf[form]=0;
		*d=0; s++;
		d+=sprintf(d,sign<0?"%-*s":"%*s",form,buf);
	}
	if( MultiCol == -1 ) { *d++='\n'; }
	*d=0;
}



extern BOOL nologout;	/* defined in main.c */

int
do_quit( void )
{
	if (Src_stack) {
		Quit = 1;
		return(do_return());
	}
	if (!nologout) {
		if( exists("S:.logout"))
			execute("source S:.logout");
	}
	main_exit(0);
	return 0;
}

int
do_echo( void )
{
	char *args=compile_av(av,1,ac,' ',0);
	fprintf( (options&2)?stderr:stdout, (options&1)?"%s":"%s\n",args );
	free(args);
	return 0;
}


static int
breakcheckd(void)
{
	long sigs = SetSignal(0L,0L);
	int ret=!o_nobreak && (sigs & SIGBREAKF_CTRL_D);
	if (ret)
		fprintf(stderr,"^D\n");
	return ret;
}

static int
breakchecke(void)
{
	long sigs = SetSignal(0L,0L);
	int ret=!o_nobreak && (sigs & SIGBREAKF_CTRL_E);
	if (ret)
		fprintf(stderr,"^E\n");
	return ret;
}


/* gets a line from file, joining lines if they end in '\' */

#define MAXLINE 512

static int
srcgets(char **buf, int *buflen, FILE *file)
{
	char *bufptr = *buf, *p, *new, concat=0, cont;
	int   totlen=0, len;

	do {
		if( totlen+MAXLINE > *buflen ) {
			new=salloc(*buflen *= 2);
			memcpy( new, *buf, 1+bufptr-*buf );
			bufptr+= new-*buf;
			free(*buf);
			*buf=new;
		}
		if (fgets(bufptr, MAXLINE, file)==NULL) {
			if( concat )
				fprintf(stderr,"Source: missing '}'\n");
			else if (bufptr != *buf)
				fprintf(stderr,"Source: file ends in '\\'\n");
			return -1;
		}
		len= strlen( bufptr );
		totlen+= len;

		cont=0;

		p=bufptr+len-1;
		if(  p>=bufptr && *p=='\n') *p--=0;
		if(  p< bufptr   ) ;
		else if( *p=='\\') *p--=0, cont=1;
		else if( *p=='{' ) concat++;
		else if( *p=='}' ) {
			if( concat>0 ) {
				concat--;
				if( concat ) *++p='\n';
			}
		} else if( concat ) *++p='\n';
		bufptr = ++p;
	} while( cont || concat );
	*bufptr=0;
	return totlen;
}



int
do_source( char *str )
{
	FILE *fi;
	char *buf;
	ROOT *root;
	int  retcode, len, bufsize=512+MAXLINE;
	int j;
	char *ptr;

	if (Src_stack == MAXSRC) {
		ierror(NULL,217);
		return -1;
	}

	if ((fi = fopen (av[1], "r")) == 0)
		{ pError(av[1]); return -1;	}

	push_locals(root=(ROOT *)salloc( sizeof(ROOT)));
	buf=salloc(bufsize);

	set_var(LEVEL_SET | LEVEL_LOCAL, v_passed, next_word(next_word(str)));


	/*
	 * now create a bunch of positional parameters , $0, $1 etc
	 */
	j = 0;
	ptr = next_word (str);
	while (*ptr) {
		char *var;
		char p[6];
		char npos[6];

		var = ptr;
		sprintf (p, "%d", j);
		/*
		 * term var str
		 */
		ptr = next_word (ptr);

		/* printf("%s\n" , ptr); */

		if (*ptr)
			*(ptr - 1) = '\0';

		set_var (LEVEL_SET | LEVEL_LOCAL, p, var);
		/*
		 * now set up what should be $# ( number of positionals )
		 */
#if 0
		sprintf (p, "_N");	/* should be "#" but csh barfs :( */
#endif
		sprintf (p, "#");	/* hackin' */
		sprintf (npos, "%d", j++);
		set_var (LEVEL_SET | LEVEL_LOCAL, p, npos);
	}


	Src_pos  [Src_stack] = 0;
	Src_abort[Src_stack] = 0;
	Src_base [Src_stack] = fi;
	Src_if[Src_stack]=If_stack;
	++Src_stack;
	while ((len=srcgets(&buf, &bufsize, fi))>=0&& !dobreak()&& !breakcheckd()){
		Src_pos[Src_stack-1] += len;
		if (Verbose&VERBOSE_SOURCE && !forward_goto)
			if( Verbose&VERBOSE_HILITE )
				fprintf(stderr,"%s)%s%s\n",o_hilite,buf,o_lolite);
			else
				fprintf(stderr,")%s\n",buf);
		retcode=execute(buf);
		if( retcode>=o_failat || Src_abort[Src_stack-1] )
			break;
		retcode=0;
	}
	--Src_stack;
	if( If_stack>Src_if[Src_stack] )
		If_stack=Src_if[Src_stack], disable=If_stack && If_base[If_stack-1];

	if (forward_goto) ierror(NULL,501);
	forward_goto = 0;
	unset_level(LEVEL_LABEL+ Src_stack);
	unset_var(LEVEL_SET, v_gotofwd);
	unset_var(LEVEL_SET, v_passed);
	fclose (fi);

	pop_locals();
	free(buf);
	free(root);

	return retcode;
}

/* set process cwd name and $_cwd, if str != NULL also print it. */

void
set_cwd(void)
{
	char pwd[256];

	if (!NameFromLock(Myprocess->pr_CurrentDir, pwd, 254)) {
		fprintf(stderr,"csh.set_cwd: NameFromLock() failed\n");
		strcpy(pwd,"<unknown>");
	}
	set_var(LEVEL_SET, v_cwd, pwd);
	/* put the current dir name in our CLI task structure */
	CtoBStr(pwd, Mycli->cli_SetName, 254);
}

int
do_pwd( void )
{
	set_cwd();
	puts( get_var( LEVEL_SET, v_cwd ));
	return 0;
}


/*
 * CD
 *
 * CD(str, 0)      -do CD operation.
 *
 */

extern int qcd_flag;

static char lastqcd[256];
static FILE *qcdfile;
static int  NumDirs;

static int
countfunc( long mask, char *file, char *fullpath )
{
	fprintf( qcdfile, "%s\n", fullpath );
	fprintf( stdout, "\r Directories: %d", ++NumDirs );
	fflush ( stdout );
	return 0;
}

int
do_cd(void)
{
	BPTR oldlock, filelock;
	char buf[256], *old, *str=av[1];
	int  i=1, repeat;

	if( options & 1 ) {
		if( !o_csh_qcd ) { fprintf(stderr,"$_qcd unset\n"); return 20; }
		if( !(qcdfile=fopen( o_csh_qcd, "w" )))
			{ fprintf(stderr,"Can't open output\n"); return 20; }
		for( ; i<ac && !dobreak(); i++ ) {
			NumDirs=0;
			printf("%s\n",av[i]);
			newrecurse( SCAN_DIRENTRY | SCAN_RECURSE, av[i], countfunc );
			printf("\n");
		}
		fclose(qcdfile);
		return 0;
	}

	if ( !str || !(*str) ) {	/* old "!*str" causes enforcer hit! */
		printf("%s\n", get_var( LEVEL_SET, v_cwd ));
		return 0;
	}

	if (filelock=Lock(str,ACCESS_READ)) {
		lastqcd[0]=0;
		if (!isdir(str)) { UnLock(filelock); ierror(str,212); return 20; }
	} else {
		repeat= !strncmp( lastqcd, str, 255 );
		strncpy( lastqcd, str, 255 );

		if( !quick_cd( buf, av[i], repeat) )
			{ fprintf(stderr,"Object not found %s\n",str); return 20; }
		if (!(filelock=Lock(buf,ACCESS_READ)))
			{ pError(buf); return 20; }
	}
	if (oldlock=CurrentDir(filelock)) UnLock(oldlock);
	if( !(old=get_var(LEVEL_SET, v_cwd)) )
		old="";
	set_var(LEVEL_SET, v_lcd, old);
	set_cwd();

	return 0;
}

char *
quick_cd( char *buf, char *name, int repeat )
{
	if( !o_csh_qcd || !exists(o_csh_qcd))
		return NULL;
	qcd_flag=repeat ? 2 : 1;
	strcpy(buf,name);
	if( quicksearch( o_csh_qcd, 1, buf)!=2 )
		return NULL;
	return buf;
}


/* AMK: mkdir now builds path to destination directory if neccessary */
int
do_mkdir( void )
{
	int i;
	BPTR lock;
	char *p,c;

	for (i=1; i<ac; ++i) {

		/* Are there any sub-directories to build? */
		if( options&1 && (p=strchr(av[i],'/'))) {
			do {
				c = *p;
				*p = '\0';
				if (lock=CreateDir(av[i]))
					UnLock(lock);
				*p = c;
			} while(p=strchr(++p,'/'));
		}

		/* allow trailing slash */
		if (lastch(av[i]) == '/' )
			av[i][strlen(av[i])-1] = '\0';

		/* Okay, sub-directories should exist, now try normal mkdir. */

		if (lock=CreateDir(av[i]))
			UnLock(lock);
		else
			pError(av[i]);

	}

	return 0;
}

int
do_mv( void )
{
	char *dest, buf[256];
	int dirflag, i, len;

	dirflag=isdir(dest=av[--ac]);
	if (ac>3 && !dirflag) { ierror(dest, 507); return (-1); }
	for (i=1; i<ac; ++i) {
		strcpy(buf, dest);

		/* source: remove trailing slash */
		if ((len=strlen(av[i]))>1 && av[i][len-1]=='/' && av[i][len-2]!=':' && av[i][len-2]!='/')
			av[i][len-1] = '\0';

		/* destination: remove trailing slash */
		if ((len=strlen(buf))>1 && buf[len-1]=='/' && buf[len-2]!=':' && buf[len-2]!='/')
			buf[len-1] = '\0';

		if (dirflag && stricmp(av[i],buf))
			 AddPart(buf, FilePart(av[i]), 255L);

		if (Rename(av[i], buf)==0) {
			pError(av[i]);
			if (!(options&1))
				return -1;
		}
		else {
			clear_archive_bit( buf );
			if (options&2) {
				/*printf("Renaming %s as %s\n",av[i],buf);*/
				printf(" %s...%s\n",av[i],dirflag?"moved":"renamed");
			}
		}
	}
	return 0;
}

static char *searchstring;
static char docr;

#if 0
int
all_args( int (*action)FUNCARG(long,char*,char*), int dirsflag )
{
	int  i;
	long mask= SCAN_FILE;

	if( options&1 )
		mask |= SCAN_RECURSE;
	if( dirsflag )
		mask |= SCAN_DIR;

	for ( i=1; i<ac && !dobreak(); ++i)
		if (isdir(av[i])) {
			if (options & 1)
				newrecurse(mask, av[i], action);
			if (dirsflag)
				(*action)(SCAN_DIR,av[i],av[i]);
		} else
			(*action)(SCAN_FILE,av[i],av[i]);
	if(docr) printf("\n"),docr=0;
	dobreak();
	return 0;
}
#endif

/* this is all_args() but with a user definable range from n to m */
int
all_args_n2m( int (*action)FUNCARG(long,char*,char*), int dirsflag, int aa_from, int aa_to )
{
	int  i;
	long mask= SCAN_FILE;

	if( options&1 )
		mask |= SCAN_RECURSE;
	if( dirsflag )
		mask |= SCAN_DIR;

	for ( i=aa_from; i<aa_to && !dobreak(); ++i)
		if (isdir(av[i])) {
			if (options & 1)
				newrecurse(mask, av[i], action);
			if (dirsflag)
				(*action)(SCAN_DIR,av[i],av[i]);
		} else
			(*action)(SCAN_FILE,av[i],av[i]);
	if(docr) printf("\n"),docr=0;
	dobreak();
	return 0;
}

int
all_args( int (*action)FUNCARG(long,char*,char*), int dirsflag )
{
	return( all_args_n2m(action,dirsflag,1,ac) );
}

#define SEARCH_REC   1
#define SEARCH_CASE  2
#define SEARCH_WILD  4
#define SEARCH_NUM   8
#define SEARCH_EXCL  16
#define SEARCH_QUIET 32
#define SEARCH_VERB  64
#define SEARCH_BIN   128
#define SEARCH_FILE  256
#define SEARCH_ABORT 512
#define SEARCH_LEFT  1024
#define SEARCH_ONLY  2048

/* added by amk, not yet implemented */
#define SEARCH_STYLE 4096	/* alternative style, inspired by GMD */
#define SEARCH_ONCE  8192	/* only show first pattern match per file */

static int abort_search;
static char lowbuf[256], file_name, file_cr;

static int
search_file( long mask, char *s, char *fullpath )
{
	PATTERN *pat;
	FILE *fi;
	char *p, *q;
	int nocasedep, lctr, len, excl=((options & 16) !=0 ), yesno;
	char buf[256], searchit[120], first, left;

	if( abort_search )
		return 0;

	nocasedep=!(options & SEARCH_CASE);
	lctr= docr= file_name= file_cr= 0;
	if (!(options & (SEARCH_QUIET|SEARCH_FILE))) {
		if( options & SEARCH_VERB )
			printf("Examining %s ...\n",fullpath);
		else
			printf("\015Examining %s ...\033[K",fullpath), docr=1;
		fflush( stdout );
	}

	strcpy(searchit,searchstring);
	if (options & SEARCH_WILD) strcat(searchit,"\n");
	len=strlen(searchit);
	if (nocasedep) strupr(searchit);
	first = *searchit;

	if( strcmp("STDIN",s) && !(options&SEARCH_WILD) && !excl ||
	             options&SEARCH_BIN )
		if( quicksearch(s,nocasedep,searchit) )
			return 0;

	if( options&SEARCH_BIN )
		{ fprintf(stderr,"Out of memory\n"); return 20; }

	if(!(pat=compare_preparse( searchit, options&SEARCH_CASE )))
		{ fprintf(stderr,"Invalid pattern\n"); return 20; }

	fi = strcmp("STDIN",s) ?  fopen(s,"r") : stdin;
	if (fi==NULL) { pError(s); return 20; }

	prepscroll(0);

	while (fgets(buf,256,fi) && !dobreak()) {
		lctr++; left=1;
		if (options & SEARCH_WILD)
			yesno=compare_ok(pat, p=buf);
		else {
			if (nocasedep) {
				strcpy(lowbuf,buf);
				strupr(lowbuf);
				p=lowbuf;
			} else
				p=buf;
			q=p;
			while ((p=index(p,first)) && strncmp(p++,searchit,len)) ;
			yesno= (p!=NULL);
			left = --p - q;
		}
		if( yesno ^ excl )
			if(!(options&SEARCH_ONLY)|| !isalphanum(p[-1])&&!isalphanum(p[len]))
				if( found(buf, lctr, 0, s, left ) )
					break;
	}
	compare_free(pat);
	if (fi!=stdin) fclose (fi);
	if( file_cr ) printf("\n");
	return 0;
}

int qcd_flag, qcd_offs;

#define READCHUNK 60000

static int
quicksearch( char *name, int nocasedep, char *pattern )
{
	int i, ptrn=strlen(pattern);
	char ut[256], *buffer, *lend;
	char *uptab=ut, *get, c, *lpos, *lstart;
	int len, lnum, qcd=qcd_flag, repeat=(qcd==2 && qcd_offs!=0), buflen;
	int sofar, got;
	BPTR fh;

#ifdef AZTEC_C
	while(0) while(0) c=c=0, uptab=uptab=ut, get=get=NULL;
#endif

	qcd_flag=0;
	if( !(fh=Open(name,MODE_OLDFILE))) {
		i=(long)IoErr(), docr=0;
		printf("\n");
		ierror(name,i);
		return 1;
	}
	len=filesize( name );
	buflen=len+3;
	if( !(buffer=(void *)AllocMem(buflen,0))) { Close(fh); return 0; }
	sofar=0;
	do {
		got=Read( fh, (char *)buffer+sofar, READCHUNK);
		sofar+=got;
	} while( got==READCHUNK );
	Close( fh);
	if( sofar != len ) {
		FreeMem( buffer, buflen );
		pError(pattern); return 1;
	}
	if(buffer[len-1]!='\n')
		buffer[len++]='\n';

	if( nocasedep )
		strupr( pattern );

	if( !qcd )
		prepscroll(0);

	for( i=0; i<256; i++ ) uptab[i]=i;
	if( nocasedep ) for( i='a'; i<='z'; i++ ) uptab[i]=i-'a'+'A';
retry:
	c = *pattern, buffer[len]=c, buffer[len+1]=c;
	get= (qcd==2) ? buffer+qcd_offs : buffer;
	if( qcd==1 ) qcd_offs=0;

	lpos=lstart=buffer, lnum=1;
	for( ;; ) {
		do ; while( uptab[*get++]!=c );
		if( --get>=buffer + len )
			break;
		for( i=1; i<ptrn; i++ )
			if( uptab[get[i]]!=pattern[i] )
				break;
		if( i==ptrn ) {
			for( ;lpos<get; lpos++ )
				if( *lpos=='\n' )
					lstart=lpos+1, lnum++;
			for( lend=lstart+1; *lend!='\n'; lend++ ) ;
			if( qcd ) {
				if( get[-1]==':' || get[-1]=='/' ||
				      lpos==lstart && lend[-1]==':' ) {
					char *tmp;
					for( tmp=get+ptrn; *tmp&& *tmp!='\n'&& *tmp!='/'; tmp++ );
					if( *tmp!='/' ) {
						*lend=0;
						strncpy(pattern,lstart,79);
						qcd_offs=lend-buffer;
						FreeMem( buffer, buflen );
						return 2;
					}
				} else 
					lend=lpos+1;
			} else {
				*lend=0;
				if(!(options&SEARCH_ONLY) ||
				     !isalphanum(lpos[-1])&&!isalphanum(lpos[ptrn]))
					if(found(lstart, lnum, get-buffer, name, lpos==lstart ))
						break;
				*lend='\n';
			}
			get=lend+1;
		} else
			get++;
	}
	if( repeat )  { repeat=0; qcd_offs=0; goto retry; }
	if( file_cr ) { printf("\n"); quickscroll(); }
	FreeMem( buffer, buflen );
	return 1;
}

static int
found( char *lstart, int lnum, int loffs, char *name, char left )
{
	int fileabort=0;

	if( (options&SEARCH_LEFT) && !left)
		return 0;

	if ( docr )
		{ quickscroll(); printf("\n"); docr=0; }

	if( options&SEARCH_FILE ) {
		file_cr=1;
		if( !file_name )
			printf("%s",name), file_name=1;
		if( options&SEARCH_NUM )
			fileabort=1;
		else
			printf(" %d",lnum);
	} else if( options & SEARCH_BIN ) {
		if (!(options & SEARCH_NUM))
			printf("Byte offset %d\n",loffs);
		else
			printf("%d\n",loffs);
		quickscroll();
	} else {
		if (!(options & SEARCH_NUM))
			printf("%4d ",lnum);
		printf((lstart[strlen(lstart)-1]=='\n')?"%s":"%s\n",lstart);
		quickscroll();
	}
	abort_search= options&SEARCH_ABORT;
	return dobreak() || fileabort || abort_search;
}

int
do_search( void )
{
	if(!IsInteractive(Output())) options |= SEARCH_VERB;
	abort_search=0;
	searchstring=av[--ac];
	all_args(search_file, 0);
	return 0;
}

#if 0
/* do_grep() is just do_search() with a modified all_args() */
int
do_grep( void )
{
	if(!IsInteractive(Output())) options |= SEARCH_VERB;
	abort_search=0;
	searchstring=av[1];
	all_args_n2m(search_file, 0, 2, ac);
	return 0;
}
#endif

static BOOL rm_error_abort = FALSE;
static int rm_file( long mask, char *file, char *fullpath )
{
	if (rm_error_abort)
		return(20);

	if ( *file && file[strlen(file)-1]=='/' ) {
		file[strlen(file)-1]=0;
	}
	if (options&16 || has_wild) {
		fprintf(stdout," %s....",fullpath);
		fflush(stdout);
	}
	if (options&2 || options&4) {
		setProtection(file,0L);
	}
	if (!DeleteFile(file)) {
		pError(file);
		if (options & 8) {
			rm_error_abort = TRUE;
			return 20;
		}
	} else if (options&16 || has_wild)
		fprintf(stdout,"Deleted\n");

	return 0;
}

int
do_rm( void )
{
	rm_error_abort = FALSE;
	all_args( rm_file, 1);
	rm_error_abort = FALSE;
	return 0;
}


int
do_history( void )
{
	HIST *hist;
	int i = H_tail_base;
	int len = av[1] ? strlen(av[1]) : 0;
	char buf[250];

	if( options&2 ) {
		while( safegets(buf,stdin) )
			add_history(buf);
		return 0;
	}

	for (hist = H_tail; hist && !dobreak(); hist = hist->prev, i++)
		if (len == 0 || !strncmp(av[1], hist->line, len))
			if( options&1 )
				printf("%s\n", hist->line);
			else
				printf("%3d %s\n", i, hist->line);
	return 0;
}

int
do_mem( void )
{
	static ULONG clast, flast;
	ULONG cfree, ffree, clarg, flarg, i;
	char *desc="Free", *mem;

	if( options&32 )
		for( i=0; i<10; i++ )
			if(mem=(char*)AllocMem(0x7fffffff,0L))
				FreeMem(mem,0x7fffffff);

	Forbid();
	cfree = AvailMem(MEMF_CHIP);
	ffree = AvailMem(MEMF_FAST);
	clarg = AvailMem(MEMF_CHIP|MEMF_LARGEST);
	flarg = AvailMem(MEMF_FAST|MEMF_LARGEST);
	Permit();

	if( options&8 ) {
		clast=cfree, flast=ffree;
		return 0;
	}
	if( options&16 )
		cfree=clast-cfree, ffree=flast-ffree, desc="Used";
	if( options&4 ) {
		if     ( options & 1 ) printf("%lu\n",cfree);
		else if( options & 2 ) printf("%lu\n",ffree);
		else                   printf("%lu\n",cfree+ffree);
	} else {
#if 1
		if     ( options & 1 ) {
			printf("Free CHIP memory:%10s",itoa(cfree));
			if ( options&16 )
				printf("\n");
			else
				printf("    largest %10s\n",itoa(clarg));
		}
		else if( options & 2 ) {
			printf("Free FAST memory:%10s",itoa(ffree));
			if ( options&16 )
				printf("\n");
			else
				printf("    largest %10s\n",itoa(flarg));
		}
		else {
			if ( options&16 ) {
				if (cfree)
					printf("CHIP memory:%10s\n",itoa(cfree));
				if (ffree)
					printf("FAST memory:%10s\n",itoa(ffree));
			}
			else {
				printf("CHIP memory:%10s",itoa(cfree));
				printf("    largest%10s\n",itoa(clarg));
				printf("FAST memory:%10s",itoa(ffree));
				printf("    largest%10s\n",itoa(flarg));
			}
			printf("Total  %s:%10s\n",desc,itoa(cfree+ffree));
		}
#else
		if     ( options & 1 ) printf("Free CHIP memory:%10s\n",itoa(cfree));
		else if( options & 2 ) printf("Free FAST memory:%10s\n",itoa(ffree));
		else {
			if(ffree) {
				printf("CHIP memory:%10s\n",itoa(cfree));
				printf("FAST memory:%10s\n",itoa(ffree));
			}
			printf("Total  %s:%10s\n",desc,itoa(cfree+ffree));
		}
#endif
	}
	return 0;
}

int
do_forline( void )
{
	char vname[33], buf[256], *cstr;
	int lctr;
	FILE *f;

	strcpy(vname,av[1]);
	if( !strcmp(av[2],"STDIN") )
		f=stdin;
	else 
		if(!(f=fopen(av[2],"r"))) { pError(av[2]); return 20; }

	lctr=0;
	++H_stack;
	cstr = compile_av (av, 3, ac, ' ', 0);
	while (fgets(buf,256,f) && !dobreak() && !breakcheckd()) {
		buf[strlen(buf)-1]='\0';	/* remove CR */
		lctr++;
		set_var(LEVEL_SET | LEVEL_LOCAL, vname, buf);
		sprintf(buf,"%d",lctr);
		set_var(LEVEL_SET | LEVEL_LOCAL, v_linenum, buf);
		exec_command(cstr);
	}
	if( f!=stdin ) fclose(f);
	--H_stack;
	free (cstr);
	if( lctr ) {
		unset_var (LEVEL_SET, vname);
		unset_var (LEVEL_SET, v_linenum);
	}
	return 0;
}

int
do_fornum( void )
{
	char vname[33], buf[16];
	int n1, n2, step, i=1, verbose, runs=0;
	char *cstr;

	verbose=(options & 1);
	strcpy(vname,av[i++]);
	n1=myatoi(av[i++],-32767,32767); if (atoierr) return 20;
	n2=myatoi(av[i++],-32767,32767); if (atoierr) return 20;
	if (options & 2) {
		step=myatoi(av[i++],-32767,32767); if (atoierr) return 20;
	} else
		step=1;
	++H_stack;
	cstr = compile_av (av, i, ac, ' ', 0);
	for (i=n1; (step>=0 ? i<=n2 : i>=n2) && !CHECKBREAK() && !breakcheckd();
	     i+=step, runs++) {
		if (verbose) fprintf(stderr, "fornum: %d\n", i);
		sprintf(buf,"%d",i);
		set_var (LEVEL_SET | LEVEL_LOCAL, vname, buf);
		exec_command(cstr);
	}
	--H_stack;
	free (cstr);
	if( runs )
		unset_var (LEVEL_SET, vname);
	return 0;
}

/*
 * foreach var_name  ( str str str str... str ) commands
 * spacing is important (unfortunately)
 *
 * ac=0    1 2 3 4 5 6 7
 * foreach i ( a b c ) echo $i
 * foreach i ( *.c ) "echo -n "file ->";echo $i"
 */

int
do_foreach( void )
{
	int cstart, cend;
	char *cstr, vname[33];
	char **fav;
	int i=1, verbose;

	verbose=(options & 1);
	strcpy(vname, av[i++]);
	if (*av[i] == '(') i++;
	cstart = i;
	while (i<ac && *av[i] != ')') i++;
	if (i > ac) { fprintf(stderr,"')' expected\n"); return 20; }
	++H_stack;
	cend = i;

	fav = (char **)salloc(sizeof(char *) * (ac));
	for (i = cstart; i < cend; ++i) fav[i] = av[i];

	cstr = compile_av (av, cend + 1, ac, ' ', 0);

	for (i = cstart; i<cend && !CHECKBREAK() && !breakcheckd() && !breakchecke(); ++i) {
		set_var (LEVEL_SET | LEVEL_LOCAL, vname, fav[i]);
		if (verbose) fprintf(stderr, "foreach: %s\n", fav[i]);
		execute(cstr);
	}
	--H_stack;
	free (fav);
	free (cstr);
	if( cstart<cend)
		unset_var (LEVEL_SET, vname);
	return 0;
}



int
do_forever( char *str )
{
	int rcode = 0;
	char *ptr = next_word( str );

	++H_stack;
	for (;;) {
		if (CHECKBREAK() || breakcheckd()) { rcode = 20; break; }
		if (exec_command (ptr) > 0) {
			str = get_var(LEVEL_SET, v_lasterr);
			rcode = (str) ? atoi(str) : 20;
			break;
		}
	}
	--H_stack;
	return rcode;
}



extern struct IntuitionBase *IntuitionBase;

void wait_refresh(struct Window *window)
{
	if (window) {
		if (window->IDCMPFlags & IDCMP_CHANGEWINDOW) {
			struct IntuiMessage *msg;
			BOOL refreshed = FALSE;
			while (!refreshed) {
				if (msg = (struct IntuiMessage *)GetMsg(window->UserPort)) {
					if (msg->Class == IDCMP_CHANGEWINDOW) {
						refreshed = TRUE;
					}
					ReplyMsg((struct Message *)msg);
				}
				else {
					WaitPort(window->UserPort);
				}
			}
		}
	}
}

int do_window( void )
{
	long x = -1, y = -1, w = -1, h = -1, maxwidth, maxheight;

	if(options & 32) { /* -q */
		struct Screen *scrn;
		struct Window *window;
		ULONG ibase_lock;
		char **ibase_list=NULL;
		long i,ibase_num=0;
		char fmt[256];

		newwidth(); /**/	/* get current window width */

		ibase_lock = LockIBase(0);

		for (scrn=IntuitionBase->FirstScreen; scrn; scrn=scrn->NextScreen) {

			struct List *list;
			struct Node *node;
			UBYTE PubNameBuf[MAXPUBSCREENNAME+4] = "";

			list = LockPubScreenList();
			for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
				if (((struct PubScreenNode *)node)->psn_Screen == scrn)
					sprintf(PubNameBuf," [%s]",node->ln_Name);
			}
			UnlockPubScreenList();

			sprintf(fmt,"Screen %c%.*s%c%s (%d,%d,%dx%dx%d):\n",
				scrn->Title ? '\"' : '(',
				(options&64) ? 128 : (((w_width-36-strlen(PubNameBuf)) > 0) ? (w_width-36-strlen(PubNameBuf)) : 30),
				scrn->Title ? scrn->Title : "no title",
				scrn->Title ? '\"' : ')',
				PubNameBuf,
				scrn->LeftEdge,
				scrn->TopEdge,
				scrn->Width,
				scrn->Height,
				scrn->BitMap.Depth
			);
			add_array_list(&ibase_list,&ibase_num,fmt);

			for (window=scrn->FirstWindow; window; window=window->NextWindow) {
				sprintf(fmt,"   win %c%.*s%c (%d,%d,%dx%d)\n",
					window->Title ? '\"' : '(',
					(options&64) ? 128 : w_width-32,
					window->Title ? window->Title : "no title",
					window->Title ? '\"' : ')',
					window->LeftEdge,
					window->TopEdge,
					window->Width,
					window->Height
				);
				add_array_list(&ibase_list,&ibase_num,fmt);
			}

			if (scrn->NextScreen)
				add_array_list(&ibase_list,&ibase_num,"\n");
		}

		UnlockIBase(ibase_lock);

		for(i=0; i<ibase_num; i++)
			printf("%s",ibase_list[i]);

		free_array_list(ibase_list,ibase_num);

		return 0;
	}

	if( o_nowindow || !Mywindow )
		return 20;

	maxwidth = Mywindow->WScreen->Width;
	maxheight= Mywindow->WScreen->Height;

	if( options&1 )
		x=Mywindow->LeftEdge,y=Mywindow->TopEdge,w=Mywindow->MinWidth,h=Mywindow->MinHeight;
	if( options&2 ) x=y=0, w=maxwidth, h=maxheight;
	if( options&4 ) WindowToFront(Mywindow);
	if( options&8 ) WindowToBack(Mywindow);
	if( options&16) ActivateWindow(Mywindow);

	if( ac == 5 ) {
		x = myatoi(av[1],-2,maxwidth-Mywindow->MinWidth); if (atoierr) return 20;
		y = myatoi(av[2],-2,maxheight-Mywindow->MinHeight); if (atoierr) return 20;
		w = myatoi(av[3],-2,maxwidth); if (atoierr) return 20;
		h = myatoi(av[4],-2,maxheight); if (atoierr) return 20;

		if (x == -1) x = 0;
		if (y == -1) y = 0;
		if (w == -1) w = maxwidth;
		if (h == -1) h = maxheight;

		if (x == -2) x = Mywindow->LeftEdge;
		if (y == -2) y = Mywindow->TopEdge;
		if (w == -2) w = Mywindow->Width;
		if (h == -2) h = Mywindow->Height;
	}
	else if ( ac != 1 ) {
		fprintf(stderr,"Usage: window <pos_x> <pos_y> <width> <height>\n");
		fprintf(stderr,"   or  window -<options>\n");
		return 20;
	}

	if( w != -1 ) {
		int i;
#if 0
		if ( x+w>maxwidth || y+h>maxheight ) {
			ierror(NULL, 500);
			return 20;
		}
#endif
		if ( x+w>maxwidth )
			x = maxwidth - w;	/* alternatively: w=maxwidth-x; */
		if ( y+h>maxheight )
			y = maxheight - h;	/* alternatively: h=maxheight-y; */
		if( w<Mywindow->MinWidth )
			w = Mywindow->MinWidth;
		if( h<Mywindow->MinHeight )
			h = Mywindow->MinHeight;

		/* IDCMP_CHANGEWINDOW sucks */

		if (TRUE || options&128) {
			ChangeWindowBox(Mywindow,x,y,w,h);
			Delay(25);
			for( i=0; i<10; i++ ) {
				if(  Mywindow->LeftEdge==x && Mywindow->TopEdge==y &&
				     Mywindow->Width   ==w && Mywindow->Height ==h )
					break;
				Delay(5);
			}
			Delay(25);
		}
		else {
#if 1
			if (Mywindow->IDCMPFlags & IDCMP_CHANGEWINDOW) {
				/* IDCMP_CHANGEWINDOW already recognized */
				ChangeWindowBox(Mywindow,x,y,w,h);
				Delay(250);
				wait_refresh(Mywindow);
			}
			else {
				/* make window recognize IDCMP_CHANGEWINDOW temporarily */
				if (!ModifyIDCMP(Mywindow,Mywindow->IDCMPFlags|IDCMP_CHANGEWINDOW))
					fprintf(stderr,"CSHELL ERROR: cannot install IDCMP_CHANGEWINDOW.\n");
				ChangeWindowBox(Mywindow,x,y,w,h);
				Delay(250);
				wait_refresh(Mywindow);
				ModifyIDCMP(Mywindow,Mywindow->IDCMPFlags&(~IDCMP_CHANGEWINDOW));
			}
#else
			if( Mywindow->LeftEdge!=0 || Mywindow->TopEdge!=0 )
				MoveWindow(Mywindow, -Mywindow->LeftEdge, -Mywindow->TopEdge );
			if( Mywindow->Width!=w || Mywindow->Height!=h )
				SizeWindow(Mywindow, w-Mywindow->Width   , h-Mywindow->Height  );
			if( x || y )
				MoveWindow(Mywindow, x, y );
#endif
		}
	}

	/* Delay(25); */ /* pause 1/2 sec. before trying to print */
	/* Delay(10); */
	/* printf("\014"); */

	return 0;
}



static void
setsystemtime(struct DateStamp *ds)
{
	struct timerequest tr;
	long secs= ds->ds_Days*86400+ds->ds_Minute*60+ds->ds_Tick/TICKS_PER_SECOND;

	if (OpenDevice(TIMERNAME, UNIT_VBLANK,(struct IORequest *)&tr, 0L)) {
		fprintf(stderr,"Clock error: can't open timer device\n");
		return;
	}

	tr.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	tr.tr_node.io_Message.mn_Node.ln_Pri = 0L;
	tr.tr_node.io_Message.mn_Node.ln_Name = NULL;
	tr.tr_node.io_Message.mn_ReplyPort = NULL;
	tr.tr_node.io_Command = TR_SETSYSTIME;
	tr.tr_time.tv_secs = secs;
	tr.tr_time.tv_micro = 0L;
	if (DoIO ((struct IORequest *)&tr))
		fprintf(stderr,"Clock error: can't talk to timer device\n");
	CloseDevice ((struct IORequest *)&tr);
}


static char tday[LEN_DATSTRING+1];	/* not sure, if +1 (null byte) is neccessary */

char *
dates( struct DateStamp *dss, int flags )
{
	static char timestr[2*LEN_DATSTRING+3];
	char tdate[LEN_DATSTRING+1], ttime[LEN_DATSTRING+1];
	struct DateTime dt;
	struct DateStamp *myds;
	char *dp,*tp;
	int slen;

	dt.dat_Format  = FORMAT_DOS;	/* bug in DOS, always localized :-( */
	dt.dat_StrDay  = tday;
	dt.dat_StrDate = tdate;
	dt.dat_StrTime = ttime;
	dt.dat_Flags   = flags ? DTF_SUBST : 0;
	dt.dat_Stamp   = *dss;

	myds = &(dt.dat_Stamp);

	if(  myds->ds_Days<0   || myds->ds_Days>36500 ||
	     myds->ds_Minute<0 || myds->ds_Minute>1440 ||
	     myds->ds_Tick<0   || myds->ds_Tick>3000 || !DateToStr(&dt) )
		strcpy(tdate,"---------"), strcpy(ttime,"--------");

#if 0
	ttime[8] = '\0';
	sprintf(timestr,"%-9s %-8s",tdate,ttime);
#endif

	/* strip off leading spaces from 'tdate' -> result in 'dp' */

	for (dp=tdate; dp && *dp && (*dp==' ' || *dp=='\t'); dp++)
		;

	/* strip off trailing spaces from 'dp' */

	for (slen=strlen(dp)-1; slen>=0 && (dp[slen]==' ' || dp[slen]=='\t'); --slen)
		dp[slen] = '\0';

	/* strip off leading spaces from 'ttime' -> result in 'tp' */

	for (tp=ttime; tp && *tp && (*tp==' ' || *tp=='\t'); tp++)
		;

	/* strip off trailing spaces from 'tp' */

	for (slen=strlen(tp)-1; slen>=0 && (tp[slen]==' ' || tp[slen]=='\t'); --slen)
		tp[slen] = '\0';

	/*
	 * month name > 3 chars? (bug in french locale)
	 * [Worse luck, this also affects german "Donnerstag"!]
	 */

	while (strlen(dp) > 9)
		strdel(dp,6,1);

	sprintf(timestr,"%-9.9s %-8.8s",dp,tp);

	timestr[18] = '\0';	/* protection against bad timestamped files */

	return timestr;
}

/*
 * returns difference in msecs between two TIMEVALS (GMD)
 */

long tv_diff (struct timeval * tv1, struct timeval * tv2)
{
	long val;

	val = ((long) tv2->tv_secs - (long) tv1->tv_secs) * 1000L;
	val += (((long) tv2->tv_micro) - ((long) tv1->tv_micro)) / 1000L;

	return val;
}

/*
 * given a DateStamp structure , returns an updated TIMEVAL (GMD)
 */

void dss2tv (struct DateStamp * t1, struct timeval * tv)
{
	ULONG secs;

	secs = t1->ds_Days * 24 * 60 * 60;
	secs += t1->ds_Minute * 60;
	secs += t1->ds_Tick / TICKS_PER_SECOND;

	tv->tv_secs = secs;
	tv->tv_micro = (t1->ds_Tick % TICKS_PER_SECOND) * (1000000 / TICKS_PER_SECOND);
}

/*
 * tv2dss ; converts a timeval structure to a DateStamp structure (GMD)
 */

void tv2dss (struct timeval * tv, struct DateStamp * dss)
{
	long rem;

	dss->ds_Days = tv->tv_secs / (24 * 60 * 60);
	rem = tv->tv_secs % (24 * 60 * 60);	/* secs in last day */

	dss->ds_Minute = rem / 60;
	rem = rem % 60;		/* secs in last minute */

	rem = (rem * 1000) + (tv->tv_micro / 1000);	/* msecs in last minute */

	dss->ds_Tick = rem / (1000 / TICKS_PER_SECOND);	/* ticks in last minute */
}

/* code for battery-clock by Gary Duncan (GMD) */

int
do_date (void)
{
	static struct DateStamp dss_s; /* set by -s option */
	static struct timeval tv;
	struct DateStamp dss;
	struct DateTime dt;
	int i = 1;

	dt.dat_Format = FORMAT_DOS;
	if (ac == 1) {
		DateStamp(&dss);
		if (options & 4) {  /* its read-battery-clock time; GMD */
			struct timeval tv_batt;
			if (BattClockBase==NULL) {
				fprintf(stderr,"No Battery Clock\n");
				return 0;
			}
			tv_batt.tv_micro = 0;
			tv_batt.tv_secs  = ReadBattClock();
			tv2dss(&tv_batt,&dss);
		}

		if (options & 1) {		/* -s option */
			dss_s = dss;
			dss2tv(&dss_s,&tv);
		}
		else if (options & 2) {		/* -r option */
			long diff;
			struct timeval tv1;

			/*
			 *  if -s not previous done , silently ignore
			 */
			if (dss_s.ds_Days == 0)
				return 0;

			dss2tv(&dss, &tv1);		/* current time */
			diff = tv_diff(&tv,&tv1);	/* diff in msecs */

			printf ("%d.%02d\n", diff / 1000, diff % 1000);
		}
		else
			printf ("%s %s\n", tday, dates (&dss, 0));
	}
	else {
		/* set the time here  */
		DateStamp (&dt.dat_Stamp);
		for (; i < ac; i++) {
			dt.dat_StrDate = NULL;
			dt.dat_StrTime = NULL;
			dt.dat_Flags = DTF_FUTURE;
			if (index (av[i], ':'))
				dt.dat_StrTime = av[i];
			else
				dt.dat_StrDate = av[i];
			if (!StrToDate (&dt))
				ierror (av[i], 500);
		}
		setsystemtime (&(dt.dat_Stamp));
	}
	return 0;
}

