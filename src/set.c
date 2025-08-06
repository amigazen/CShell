
/*
 * SET.C
 *
 * (c)1986 Matthew Dillon     9 October 1986
 *
 * Version 2.07M by Steve Drew 10-Sep-87
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 *
 */

#include "shell.h"

static char *prompt_string( char *str, char *t );
static void set_sys_var( char *name, char *val );

#define MAXLEVELS (4+MAXSRC)

static ROOT _Mbase[MAXLEVELS], *Mbase[MAXLEVELS], *Topbase[MAXLEVELS];

void
init_mbase(void)
{
	int  i;
	char *c;

	for( i=0; i<MAXLEVELS; i++ )
		Topbase[i]=Mbase[i] = &_Mbase[i];

#ifdef isalphanum
	for( c=isalph+'0'; c<=isalph+'9'; *c++=1 ) ;
	for( c=isalph+'A'; c<=isalph+'Z'; *c++=1 ) ;
	for( c=isalph+'a'; c<=isalph+'z'; *c++=1 ) ;
	isalph['_']=1;
#endif
}



void
set_var( int level, char *name, char *str )
{
	char *val;

	if(!str) str="";
	if(!(val=malloc(strlen(str)+1))) {
		ierror(NULL,512);
		return;
	}
	strcpy( val, str );

	set_var_mem( level, name, val );
}



void
set_var_mem( int level, char *name, char *str ) /* does not make a copy */
{
	NODE **first, *node;
	ROOT *root, *nul=0;
	int  local= level & LEVEL_LOCAL;
	char *t, c;

	for( t=name; isalphanum(*t); t++ ) ;
	c = *t; *t=0;

	level &=~ LEVEL_LOCAL;

	for( root= Mbase[level]; root; root= local? nul : root->parent ) {
		first= & root->first[*name & MAXHASH-1];
		for( node = *first; node; node=node->next ) {
			if( !strcmp( node->name, name) ) {
				free( node->text );
				goto copy;
			}
		}
	}

	if(!(node=malloc(sizeof(NODE) + strlen(name)))) {
		ierror(NULL,512);
		return;
	}
	node->next = *first;
	*first=node;
	strcpy( node->name, name );

copy:
	node->text=str;
	if( *name=='_' )
		set_sys_var( name, node->text );

	*t=c;
}

static char VarBuf[256];

void *
get_var( int level, void *varname )
{
	NODE *node;
	ROOT *root;
	char *t, c, *res=NULL;
	int  f = *(char *)varname & MAXHASH-1;

	for ( t= varname; (signed char)*t>0; t++ ) ;
	c = *t; *t=0;

	for( root= Mbase[level]; root; root=root->parent )
		for( node=root->first[f]; node; node=node->next )
			if( !strcmp(node->name,varname) )
				{ res=node->text; goto done; }

	/* AMK: OS20-GetVar replaces ARP-Getenv */
	if(level==LEVEL_SET && *(char*)varname!='_' && GetVar(varname,VarBuf,256,GVF_BINARY_VAR)>=0L)
		res=VarBuf;

done:
	*t=c;
	return res;
}

void
unset_level( int level )
{
	NODE *node;
	int i;

	for( i=0; i<MAXHASH; i++ ) {
		for( node=Mbase[level]->first[i]; node; node=node->next ) {
			if( *node->name=='_' && level==LEVEL_SET ) {
				Mbase[level]->first[i]= node->next;
				set_sys_var( node->name, get_var( LEVEL_SET, node->name ));
			}
			Free ( node->text );
			Free ( node );
		}
		Mbase[level]->first[i] = NULL;
	}
}

void
unset_var( int level, char *name )
{
	ROOT *root;
	NODE **first, *node, *prev;
	char *t, c;
	int  f = *name & MAXHASH-1;;

	for( t=name; isalphanum(*t); t++ ) ;
	c = *t; *t=0;

	for( root= Mbase[level]; root; root=root->parent ) {
		first= & root->first[f];
		for( node = *first, prev=NULL; node; prev=node, node=node->next ) {
			if( !strcmp( node->name, name) ) {
				if( prev ) prev->next=node->next; else *first=node->next;
				Free( node->text );
				Free( node );
				if( *name=='_' )
					set_sys_var( name, NULL );
				goto done;
			}
		}
	}
done:
	*t=c;
}


void
set_var_n( int level, char *name, char *str, int n )
{
	char c, len=strlen(str);

	if( n>len )
		n=len;

	if( n>=0 ) {
		c=str[n]; str[n]=0;
		set_var( level, name, str );
		str[n]=c;
	} else
		set_var( level, name, "" );
}

int
do_unset_var( char *str, int level )
{
	int i;

	for (i = 1; i < ac; ++i)
		unset_var (level, av[i]);
	return 0;
}

/*
 * print given string, print control chars quoted (e.g. '^L' as '^' 'L')
 *
 * 05/18/93 ch
 */
void printf_ctrl(char *s)
{
	int c,i=0;
	char *p;
	if (p = malloc(strlen(s)*2 + 1)) {
		while(c=(*s++)) {
			if(c<32) {
				p[i++] = '^';
				p[i++] = 'A'-1+c;
			}
			else
				p[i++] = c;
		}
		p[i] = 0;
		puts(p);
		free(p);
	}
	else
		puts(s);
}

int
do_set_var( char *command, int level )
{
	ROOT *root;
	NODE *node;
	char *str, *val;
	int  i=2, j= *av[0]=='a' ? ' ' : 0xA0;

	if( ac>2 ) {
		if( *av[i]=='=' && !av[i][1] && ac>3) i++;
		val= compile_av( av, *av[i] ? i : i+1, ac, j, 0 );
		set_var_mem(level, av[1], val);
	} else if( ac==2 ) {
		if (str=get_var(level,av[1])) {
			printf("%-10s ",av[1]);
			printf_ctrl(str);
			/*putchar('\n');*/
		}
	} else if( ac==1 ) {
		if( level& LEVEL_LOCAL )
			root= Mbase[ level&~LEVEL_LOCAL];
		else
			root= Topbase[ level ];
		for( i=0; i<MAXHASH && !breakcheck(); i++ )
			for( node=root->first[i]; node && !dobreak(); node=node->next ) {
				printf("%s%-10s ",o_lolite,node->name);
				printf_ctrl(node->text);
				/*putchar('\n');*/
				/*quickscroll();*/
			}
	}

	return 0;
}


extern char shellvers[];
extern char shellctr[];
extern long ExecTimer, ExecRC;
#define MAX_PITEM 3

static char *
prompt_string( char *str, char *t )
{
	struct DateStamp dss;
	char *u,*dev,buf[10];
	static char *mypath = NULL;
	static int mypathlen = 0;

#ifdef MULTIUSER_SUPPORT
	struct muUserInfo *user_info; /* LILJA: Added for multiuser-support */
#endif

	DateStamp(&dss);

	if( !str ) {
		*t=0;
		return t;
	}

	while (*str) {
		if (*str!='%') {
			*t++ = *str++;
			continue;
		}
		str+=2;
		switch( str[-1] ) {
		case 'P':
			if (u=get_var(LEVEL_SET, "_cwd")) {

				/* allocate bigger buffer? */
				if (strlen(u)+1 > mypathlen) {
					if (mypath) free(mypath);
					mypathlen = strlen(u)+1;
					if (!(mypath = malloc(mypathlen)))
						mypathlen = 0;
				}

				if (mypath) {
					strcpy(mypath,u);

					/* find last ':' in path */
					if (u=strlast(mypath,':')) {
						int i,max_pitem=o_promptdep,cnt=0;

						/* NOTICE: "u" points to ':', NOT to char after ':' */

						/* is "_promptdep" valid? */
						if (o_promptdep < 1 || o_promptdep > 65535) {
							fprintf(stderr,"csh: 1 <= _promptdep <= 65535\n");
							max_pitem = MAX_PITEM;
						}

						/* count occurences of '/' (add 1 for no. of path parts) */
						/* (notice, that "u" still holds the leading ':') */
						for (i=strlen(u)-1; i>0 && cnt<max_pitem; i--) {
							if(u[i]=='/') cnt++;
						}

						/* too much path parts? */
						if (cnt>=max_pitem) {
							/* strip only if remaining path is > 3 chars */
							if (i>3) {
								u[1] = u[2] = u[3] = '.';
								strdel(u,4,i-3);
							}
						}
						t+=sprintf(t,"%s",mypath);
					}
					else
						t+=sprintf(t,"%s",mypath);
				}
				else
					t+=sprintf(t,"%s", u);
			}
			else
				t+=sprintf(t,"%s", "(no _cwd)");
			break;
		case 'p': t+=sprintf(t,"%s", get_var(LEVEL_SET, "_cwd"));  break;
		case 'm': t+=sprintf(t,"%d", (AvailMem( 0 )+512)/1024);    break;
		case 't': t+=sprintf(t,"%s", next_word(dates(&dss,0)));    break;
		case 'c': t+=sprintf(t,"%s", o_hilite);                    break;
		case 'v': t+=sprintf(t,"%s", shellvers );                  break;
		case 'n': t+=sprintf(t,"%s", get_var(LEVEL_SET,"_clinumber"));break;
		case 'h': t+=sprintf(t,"%d", H_num);                       break;
		case 'd':    sprintf(t,"%s", dates(&dss,0));if(u=index(t,' '))t=u;break;
		case 'f': t+=sprintf(t,"%s", oneinfo(get_var(LEVEL_SET,v_cwd),4));break;
		/* AMK: OS20-GetVar replaces ARP-Getenv */
		case 's': if (GetVar(shellctr,buf,10,GVF_GLOBAL_ONLY|GVF_BINARY_VAR)<0L)
		            u = NULL;
		          else
		            u = buf;
		          t+=sprintf(t,"%s", u?buf:"?");break;
		case 'V': if (u=get_var(LEVEL_SET, "_cwd")) {
				if (dev = strdup(u)) {
					if (u=strlast(dev,':')) {
						*u = '\0';
						t+=sprintf(t,"%s",dev);
					}
					free(dev);
				}
			  }
			  break;
		case 'x': t+=sprintf(t,"%2d",ExecRC);                             break;
		case 'r': t+=sprintf(t,"%d", (signed char)
		                               Myprocess->pr_Task.tc_Node.ln_Pri);break;
		case 'e': t+=sprintf(t,"%d:%02d.%02d",ExecTimer/6000,ExecTimer/100%60,
		                                     ExecTimer%100);              break;
#ifdef MULTIUSER_SUPPORT
		/* LILJA: Added for multiuser-support */
		case 'U':
			if (muBase) {
				if(user_info=muAllocUserInfo()) {
					user_info->uid=muGetTaskOwner(NULL)>>16;
					t+=sprintf(t,"%s",muGetUserInfo(user_info,muKeyType_uid)?user_info->UserID:"<unknown>");
					muFreeUserInfo(user_info);
				}
				else
					t+=sprintf(t,"<unknown>");
			}
			else
				t+=sprintf(t,"<unknown>");
			break;
#endif
		default : *t++=str[-2]; *t++=str[-1]; break;
		}
	}
	*t=0;
	return t;
}

void
push_locals( ROOT *newroot )
{
	int i;
	NODE **nodeptr;

	newroot->parent=Mbase[ LEVEL_SET ];
	Mbase[ LEVEL_SET ]=newroot;

	nodeptr=newroot->first;
	for( i=MAXHASH; i>0; --i )
		*nodeptr++=NULL;
}

void
pop_locals( void )
{
	unset_level( LEVEL_SET );
	Mbase[ LEVEL_SET ]=Mbase[ LEVEL_SET ]->parent;
}

int
do_local(void)
{
	int i;

	if( ac==1 )
		do_set_var( "", LEVEL_SET | LEVEL_LOCAL);
	else 
		for( i=1; i<ac; i++ )
			set_var( LEVEL_SET | LEVEL_LOCAL, av[i], "");
	return 0;
}


char truetitle[200];

char *o_rback="rback", *o_complete="*";
char o_hilite[24], o_lolite[8], *o_csh_qcd, o_nobreak;
char o_minrows, o_scroll, o_nowindow, o_noraw, o_vt100, o_nofastscr;
char o_bground, o_resident, o_pipe[16]="T:", o_datefmt, o_nomatch=0;
char o_abbrev=5, o_insert=1, *o_every, o_cquote=0, o_mappath=0;
long o_noreq, o_failat=20, o_timeout=GWB_TIMEOUT_LOCAL, o_warn=29;
long o_promptdep=MAX_PITEM;

extern char trueprompt[100];

#define ISVAR(x) ( !strcmp( name, x ) )

static void
set_sys_var( char *name, char *val )
{
	char *put, col;

	/*
	 *  assumption is that "val" is not dynamic but a pointer
	 *  to an internally allocated string for that variable and
	 *  is valid until next change ...
	 */

	if     (ISVAR(v_debug    )) debug     = val!=NULL;
	else if(ISVAR(v_nobreak  )) o_nobreak = val!=NULL;
	else if(ISVAR(v_insert   )) o_insert  = val!=NULL;
	else if(ISVAR(v_nomatch  )) o_nomatch = val!=NULL;
	else if(ISVAR(v_mappath  )) o_mappath = val!=NULL;
	else if(ISVAR(v_cquote   )) o_cquote  = val!=NULL;
	else if(ISVAR(v_every    )) o_every   = val;
	else if(ISVAR(v_failat   )) o_failat  = val ? atoi(val) : 20;
	else if(ISVAR(v_warn     )) o_warn    = val ? atoi(val) : 29;
	else if(ISVAR(v_timeout  )) o_timeout = val ? atoi(val) : 1;
	else if(ISVAR(v_rback    )) o_rback   = val ? val : "rback";
	else if(ISVAR(v_abbrev   )) o_abbrev  = val ? atoi(val) : 5;
	else if(ISVAR(v_promptdep)) o_promptdep = val ? atoi(val) : -1;
	else if(ISVAR(v_complete )) o_complete= val ? val : "*";
#if 0
	else if(ISVAR(v_abbrev   )) o_abbrev  = val?*val!='n':1;
#endif
	else if(ISVAR(v_datefmt  )) o_datefmt = val && !strcmp(val,"subst");
	else if(ISVAR(v_qcd      )) o_csh_qcd = val;
	else if(ISVAR(v_noreq    )) Myprocess->pr_WindowPtr = (o_noreq=(val?1:0))? (APTR) -1L : 0L/*Mywindow*/;
	else if(ISVAR(v_hist     )) S_histlen = val ? atoi(val) : 0;
	else if(ISVAR(v_titlebar )) update_sys_var( v_titlebar );
	else if(ISVAR(v_pipe     )) appendslash(strcpy(o_pipe,val?val:"t:"));
	else if(ISVAR(v_verbose  )) {
		Verbose=0;
		if( val ) {
			if( index(val,'s' )) Verbose|= VERBOSE_SOURCE;
			if( index(val,'a' )) Verbose|= VERBOSE_ALIAS;
			if( index(val,'h' )) Verbose|= VERBOSE_HILITE;
		}
	} else if( ISVAR(v_hilite)) {
		o_hilite[0]=o_lolite[0]=0;
		if( !val )
			val="";
		put= o_hilite;
		while( val && *val ) {
			switch( *val++ ) {
			case 'b': put+=sprintf( put, "\033[1m" ); break;
			case 'i': put+=sprintf( put, "\033[3m" ); break;
			case 'u': put+=sprintf( put, "\033[4m" ); break;
			case 'r': put+=sprintf( put, "\033[7m" ); break;
			case 'c': put+=strlen(put);
			          if( *val>='0' && *val<='9' ) {
			             col = *val++;
			             if( *val==',' && val[1]>='0' && val[1]<='9' ) {
			                 put+=sprintf( put,"\033[3%cm\033[4%cm",col,val[1]);
			                 val+=2;
			             } else
			                 put+=sprintf( put,"\033[3%cm",col );
			          }
			          break;
			}
		}
		*put=0;
		if( *o_hilite )
			strcpy(o_lolite,"\033[m");
		strcpy(prompt_string(put,trueprompt),o_lolite);
	} else if( ISVAR( v_scroll )) {
		o_scroll=0;
		if( val ) {
			o_scroll=atoi( val );
			if( o_scroll<2 ) o_scroll=0;
			if( o_scroll>8 ) o_scroll=8;
		}
	} else if( ISVAR(v_minrows)) {
		o_minrows=34;
		if( val ) {
			o_minrows=atoi( val );
			if( o_minrows<8 )   o_minrows=8;
			if( o_minrows>100 ) o_minrows=100, o_scroll=0;
		}
	}
}


extern BOOL nowintitle;  /* defined in main.c */

void
update_sys_var( char *name )
{
	char c=name[1], *str, buf[250];

	if( c==v_prompt[1] ) {
		if( (str=get_var(LEVEL_SET,v_prompt) ) ==NULL) str="$ ";
		strcpy(prompt_string(str,trueprompt),o_lolite);
	}
	if( c==v_titlebar[1] && !o_nowindow && Mywindow ) {
		prompt_string( get_var(LEVEL_SET, v_titlebar), buf);
		if (strcmp((char*)Mywindow->Title, buf)) {
			strcpy(truetitle,buf);
			if (!nowintitle)
				SetWindowTitles(Mywindow, truetitle, (char *)-1);
		}
	}
}

