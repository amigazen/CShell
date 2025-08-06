/* rawcon.c
 *
 * Shell 2.07M  17-Jun-87
 * console handling, command line editing support for Shell
 * using new console packets from 1.2.
 * Written by Steve Drew. (c) 14-Oct-86.
 * 16-Dec-86 Slight mods to rawgets() for Disktrashing.
 *
 * Version 4.01A by Carlo Borreo & Cesare Dieni 17-Feb-90
 * Version 5.00L by Urban Mueller 17-Feb-91
 * Version 5.20L by Andreas M. Kirchwitz (Fri, 13 Mar 1992)
 *
 */

#include "shell.h"

void setrawcon( long flag, int ievent );

#define LINELEN 255

int w_width=80, w_height=24;

#if RAW_CONSOLE

static int myget( void );
static void myunget(int c);
static int get_seq( long *param );

static void back( int n );
static void left( int n );
static void right( int n );
static int  leftwordlen( void );
static int  rightwordlen( void );
static void delete( int i );
static void history( int hnum );
static void display( char *str );
static void redraw( void );
static void abbrev( char *buf, char **eav, int eac );
static void quote( char *buf, char *sep );
static void getword( char *buf );
static void delword( void );
static int  getkey(void);

static char *tyahdptr;
static int unget;

#define SETRAW setrawcon(-1,1);
#define SETCON setrawcon( 0,1);

extern char *MenuCommand[MAXMENUS][MAXMENUITEMS];

#define CTRL  -64
#define SHIFT 512
#define ESC   1024

#define CUP   256
#define CDN   257
#define CRT   258
#define CLT   259
#define TAB   9

static int Curmap;
static USHORT *Keymap[8];
static USHORT DefKeymap0[]={
       CLT,  0, /* CursLt = Move.Left  */
       CRT,  1, /* CursRt = Move.Right */
 SHIFT+CLT,  2, /* SCursLt= Move.WordL */
 SHIFT+CRT,  3, /* SCursRt= Move.WordR */
   ESC+CLT,  4, /* ESC-CLt= Move.SOL   */
   ESC+CRT,  5, /* ESC-CRt= Move.EOL   */
  CTRL+'A',  4, /* ^A     = Move.SOL   */
  CTRL+'E',  5, /* ^E     = Move.EOL   */
  CTRL+'Z',  4, /* ^Z     = Move.SOL   */
         8, 10, /* BackSp = Del.BackSp */
       127, 11, /* Delete = Del.Delete */
   ESC+  8, 12, /* ESC-BkS= Del.WordL  */
   ESC+127, 13, /* ESC-Del= Del.WordR  */
  CTRL+'W', 12, /* ^W     = Del.WordL  */
  CTRL+'B', 14, /* ^B     = Del.SOL    */
  CTRL+'K', 15, /* ^K     = Del.EOL    */
   ESC+'x',513, /* ESC-x  = Setmap 1   */
   ESC+'d', 16, /* ESC-d  = Del.Line   */
  CTRL+'X', 16, /* ^X     = Del.Line   */
       CUP, 20, /* CursUp = Hist.Back  */
       CDN, 21, /* CursDn = Hist.Forw  */
   ESC+CUP, 22, /* ECursUp= Hist.Beg   */
   ESC+CDN, 23, /* ECursDn= Hist.End   */
 SHIFT+CUP, 24, /* SCursUp= Hist.Compl */
   ESC+'!', 24, /* ESC-!  = Hist.Compl */
   ESC+ 13, 25, /* ESC-Ret= Hist.Exec  */
  CTRL+'T', 26, /* ^T     = Hist.Tail  */
 SHIFT+CDN, 27, /* SCursDn= Hist.Clr   */
  CTRL+'P', 28, /* ^P     = Hist.DupWrd*/
       TAB, 30, /* Tab    = Comp.Norm  */
 SHIFT+TAB, 31, /* STab   = Comp.Part  */
   ESC+TAB, 32, /* ESC-TAB= Comp.All   */
   ESC+'*', 32, /* ESC-*  = Comp.All   */
   ESC+'c', 33, /* ESC-c  = Comp.CD    */
   ESC+'~', 34, /* ESC-~  = Comp.LastCD*/
  CTRL+'D', 35, /* CTRL-D = Comp.Dir   */
   ESC+'=', 35, /* ESC-=  = Comp.Dir   */
   ESC+'p', 36, /* ESC-p  = Comp.Prg1  */
   ESC+'P', 37, /* ESC-P  = Comp.PrgAll*/
   ESC+'i', 40, /* ESC-i  = Spec.Insert*/
  CTRL+'L', 43, /* ^L     = Spec.Refr  */
        10, 44, /* Enter  = Spec.Accept*/
        13, 44, /* ^Enter = Spec.Accept*/
  CTRL+'N', 45, /* ^N     = Spec.Next  */
  CTRL+'O', 48, /* ^O     = Spec.EchoO */
 CTRL+'\\', 46, /* ^\     = Spec.EOF   */
       260, 42, /* Help   = Spec.Help  */
       271, 51, /* Menu   = Misc.Menu  */
  CTRL+'U', 52, /* ^U     = Misc.Undo  */
  CTRL+'R', 53, /* ^R     = Misc.Repeat*/
   ESC+'s', 54, /* ESC-s  = Misc.Swap  */
  CTRL+'V', 55, /* CTRL-V = Misc.Quote */
         0,  0
};

static USHORT DefKeymap1[]={
         8, 14,
       127, 15,
         0, 0
};

static int  Pos, Len;
static char *Line, *Prompt;
static char LastDir[128];
static char Undo[256];
static int  LastFn = -99, LastKey = -99;
long   Param[10];



void initmap(void)
{
	if( !Keymap[0] )
		Keymap[0]=DefKeymap0, Keymap[1]=DefKeymap1;
}



char *ffgets(char *line,long length,FILE *fp)
{
	if (fgets(line,length-1,fp)) {
		if (!strchr(line,'\n')) {
			int c;
			do {
				c = fgetc(fp);
			}
			while (c!=EOF && c!='\n');
			strcat(line,"\n");
		}
		return(line);
	}
	else
		return(NULL);
}



char *rawgets( char line[], char prompt[] )
{
	static int HNum = -1;

	int  key, fn, old, hnum = -1, olen=0, inslen=0;
	int  insert, undo_pos=0, undo_len=0;
	int  eac, eactr=0;
	char typeahd[LINELEN], **eav=NULL, *ret=line;
	char *prghash_hit;
	char prghash_pat[LINELEN];

	char *prio_set;
	long oldprio,prio;

	if ( o_noraw || !IsInteractive(Input()) ) {
		if (IsInteractive(Input())) {
			printf("%s",prompt);
			fflush(stdout);
		}
		return( ffgets(line,LINELEN-1,stdin) );
	}

	if (WaitForChar(Input(), 100L) || CHARSWAIT(stdin))	/* don't switch to 1 */
		return( ffgets(line,LINELEN-1,stdin) );

	SETRAW;

	if (prio_set = get_var(LEVEL_SET,v_clipri)) {
		prio=atol(prio_set);
		if (!isnum(prio_set)) {
			printf("error: variable _clipri (%s) is not a number\n",prio_set);
			prio_set = NULL;
		}
		else if (prio<-128 || prio>127) {
			printf("error: variable _clipri (%s) is out of range (-128,127)\n",prio_set);
			prio_set = NULL;
		}
		else {
			oldprio = SetTaskPri((struct Task *)Myprocess,prio);
		}
	}

	tyahdptr=typeahd;
	typeahd[0]=0;
	Flush(Output());

	/*
	 *  If internal variable o_vt100 is set, Cshell sends no longer
	 *  "\033[ p" (special Amiga control sequence to turn on cursor)
	 *  which confused some terminal programs.
	 */

	fprintf(stdout,"\015\017%s%s",
			o_vt100 ? "" : "\033[ p",
			prompt);
	fflush(stdout);

	Line= line; Prompt= prompt;
	line[ Len=Pos=0 ]=0;
	insert= o_insert;

	if( LastFn!=25 )
		LastFn = -1;

	if( HNum>=0 ) {
		history( hnum=HNum );
		HNum = -1;
	}

	while( (key=getkey()) != -1) {
		USHORT *p;
		int    t;
		char   c, *s, *src, *sep;

		for( fn = -1, p=Keymap[Curmap]; *p; p+=2 )
			if( *p==key )
				{ fn=p[1]; break; }
		if( fn == -1 && (key>=261 && key<=270 || key>=261+SHIFT && key<=270+SHIFT))
			fn=50;
		if( fn == -1 && tyahdptr )
			fn = -2;
		if( fn!=52 && fn != -2 ) {
			memcpy( Undo, line, Len+1 );
			undo_pos=Pos; undo_len=Len;
		}

		switch( fn/512 ) {
		case 1:
			fn&=511;
			if( fn<8 && Keymap[fn] ) Curmap=fn;
			fn = -3;
			break;
		case 2:
			key=fn&511, fn = -1;
			break;
		}

		if( fn != -3 )
			Curmap=0;

		if( fn == 53 )
			fn=LastFn, key=LastKey;

		switch( fn ) {
		case -3:                                /* change keymap */
			break;
		case -2:                                /* auto insert   */
		case -1:                                /* insert key    */
			key&=255;
			if (key < 31 || (insert?Len:Pos) >= LINELEN-1 )
				break;
			putchar(key);
			if (Pos < Len && insert) {
				memmove( line+Pos+1, line+Pos, Len-Pos+1);
				printf("%s",line+Pos+1);
				back(Len-Pos);
				Len++;
			}
			line[Pos++] = key;
			if (Len < Pos) Len = Pos;
			line[Len] = 0;
			break;

		case 0:                                 /* cursor left  */
			left(1);
			break;
		case 1:                                 /* cursor right */
			right(1);
			break;
		case 2:                                 /* word left    */
			left( leftwordlen() );
			break;
		case 3:                                 /* word right   */
			right( rightwordlen() );
			break;
		case 4:                                 /* beg of line  */
			left(Pos);
			break;
		case 5:                                 /* end of line  */
			right(Len-Pos);
			break;

		case 10:                                /* backspace    */
			if (Pos==0)
				break;
			left(1);
		case 11:                                /* delete       */
			if (Pos==Len)
				break;
			delete(1);
			break;
		case 12:                                /* bkspc word     */
			if( !leftwordlen() )
				break;
			left( leftwordlen() );
		case 13:                                /* del word       */
			delete( rightwordlen() );
			break;
		case 14:                                /* del to SOL     */
			left( old=Pos );
			delete( old );
			break;
		case 16:                                /* delete line    */
			left( Pos );
		case 15:                                /* delete to EOL  */
			delete( Len-Pos );
			break;

		case 17: /* AMK */                      /* clear screen + refresh */
			fprintf(stdout,"\033c\017");
			fflush(stdout);
			redraw();
			break;

		case 20:                                /* history up   */
			if( hnum>=H_len-1 )
				hnum=H_len-1;
			history( ++hnum );
			break;
		case 21:                                /* history down */
			if( hnum<=0 )
				hnum=0;
			history( --hnum );
			break;
		case 22:                                /* beg of hist */
			history( hnum=H_len-1 );
			break;
		case 23:                                /* end of hist */
			history( hnum=0 );
			break;
		case 24:                                /* complete hist */
			if( LastFn!=fn )
				hnum = -1, olen=Len;
			line[Len=olen]=0;
			if((hnum=find_history( line, ++hnum )) == -1)
				display(line);
			else
				history(hnum);
			break;
		case 25:                                /* exec hist  */
			HNum= hnum;
			LastFn= fn;
			goto done;
		case 26:                                /* tail of prev */
			if( H_head && (s=H_head->line) && (s=index(s,' ')))
				tyahdptr=s;
			break;
		case 27:                                /* bottom   */
			history( hnum = -1 );
			break;
		case 28:                                /* duplicate word */
			for( s=line+Pos,t=0; s>line && s[-1]==' '; s--,t++ ) ;
			left(t);
			*(tyahdptr=typeahd)=' ';
			getword( typeahd+(t?0:1) );
			right(t);
			break;
		case 29: /* AMK */                      /* last word of prev */
			if( H_head && (s=H_head->line)) {
				int l = strlen(s) - 1;
				while (l>=0 && s[l]==' ')	/* skip spaces */
					--l;
				while (l>=0 && s[l]!=' ')	/* find next space */
					--l;
				if (l>=0)
					tyahdptr = &s[l];
			}
			break;

		case 32:                                /* complete all  */
		case 35: LastFn = -1;                     /* show files    */
		case 30:                                /* complete      */
		case 31:                                /* complete part */
		case 33:                                /* qcd           */
			sep=" ";
			if( fn!=LastFn ) {
				getword( typeahd );
				filemap( typeahd, 1 );
				if( eav    ) free_expand( eav ), eav=NULL;
				eac=0;
				breakreset();
				if( fn==35 ) {
					int tmp=o_scroll;
					int old_Pos=Pos;
					right(Len-Pos);
					putchar('\n');
					if (isdir( typeahd )) {
						/* show contents of directory */
						memmove( typeahd+8, typeahd, strlen(typeahd)+1);
						/*
						   we need to escape "dir" here in
						   such a way that it works even if
						   a user aliased "dir" to something
						   completely else
						*/
						memcpy ( typeahd, "\\dir -s ", 8);
					}
					else {
						/* show matching files */
						memmove( typeahd+8, typeahd, strlen(typeahd)+1);
						/*
						   we need to escape "dir" here in
						   such a way that it works even if
						   a user aliased "dir" to something
						   completely else
						*/
						memcpy ( typeahd, "\\dir -s ", 8);
						strcat ( typeahd, "*");
					}
					o_scroll=0;
					execute( typeahd );
					o_scroll=tmp;
					printf("\033[J%s%s",Prompt,Line);
					back(Len-old_Pos);
					Pos = old_Pos;
					break;
				}
				if( fn!=33 ) {
					if (strlen(o_complete)>0)
						strcat( typeahd, o_complete);
					else
						strcat( typeahd, "*");
					if( !index(typeahd,'&' ))
						eav =expand(typeahd,&eac);
					if( eac==0 ) {
						putchar(7);
						break;
					}
				}
				if( fn==30 )
					s=eav[ eactr=0 ];
				else if( fn==31 )
					abbrev(s=typeahd,eav,eac), eactr = -1, sep= eac==1?" ":"";
				else if( fn==32 ) {
					strncpy(s=typeahd, src=compile_av(eav,0,eac,' ',1),LINELEN);
					typeahd[LINELEN-1]=0;
					free(src);
				} else if( fn==33 ) {     /* 33 */
					strncpy(LastDir,typeahd,128);
					if( !quick_cd( s=typeahd+128, LastDir, 0)) {
						putchar(7);
						break;
					}
				}
				delword();
			} else {
				if( fn==33 )
					quick_cd( s=typeahd+128, LastDir, 1);
				else { /* 30,31 */
					if( eac==0 ) {
						putchar(7);
						break;
					}
					s=eav[eactr = ++eactr % eac];
				}
				left(inslen);
				delete(inslen);
			}
			strcpy( tyahdptr=typeahd, s );
			if( fn!=32 )
				quote( typeahd, sep );
			inslen=strlen(typeahd);
			break;
		case 34:                      /* last CD */
			strncpy(typeahd,get_var( LEVEL_SET, v_lcd ),230);
			appendslash(tyahdptr=typeahd);
			break;
		case 36:
			if( fn!=LastFn ) {
				getword( typeahd );
				filemap( typeahd, 1 );
				strcpy(prghash_pat,typeahd);
				if (prghash_hit=get_rehash_prog(NULL,prghash_pat)) {
					delword();
					strcpy(tyahdptr=typeahd,prghash_hit);
					/*quote(typeahd," ");*/
					inslen=strlen(typeahd);
				}
				else
					putchar(7);
			}
			else {
				if (prghash_hit) {
					if (prghash_hit=get_rehash_prog(prghash_hit,prghash_pat)) {
						left(inslen);
						delete(inslen);
						strcpy(tyahdptr=typeahd,prghash_hit);
						/*quote(typeahd," ");*/
						inslen=strlen(typeahd);
					}
					else
						putchar(7);
				}
				else
					putchar(7);
			}
			break;
		case 37:
			{
			char *firsthit;
			int tmp=o_scroll;
			int old_Pos=Pos;
			int cnt = 0;

			getword( typeahd );
			filemap( typeahd, 1 );

			prghash_hit = NULL;
			if (firsthit=get_rehash_prog(NULL,typeahd)) {
				right(Len-Pos);
				putchar('\n');
				o_scroll=0;
				printf(" %d. %s\n",++cnt,firsthit);
				prghash_hit = firsthit;
				breakreset();
				while ( !dobreak() && (prghash_hit=get_rehash_prog(prghash_hit,typeahd)) && prghash_hit!=firsthit ) {
					printf(" %d. %s\n",++cnt,prghash_hit);
				}
				breakreset();
				o_scroll=tmp;
				printf("\033[J%s%s",Prompt,Line);
				back(Len-old_Pos);
				Pos = old_Pos;
			}
			else
				putchar(7);

			break;
			}

		case 40:                      /* ins/ovr */
			insert ^= 1;
			break;
		case 41:                      /* quit    */
			strcpy(line,"quit");
			goto done;
		case 42:                      /* help    */
			strcpy(line,"help");
			goto done;
		case 43:                      /* refresh */
			redraw();
			break;
		case 44:                      /* exec    */
			goto done;
		case 45:                      /* leave   */
			add_history( line );
			right(Len-Pos);
			putchar('\n');
			line[Len=Pos=0]=0;
			update_sys_var(v_prompt);
			update_sys_var(v_titlebar);
			redraw();
			hnum = -1;
			break;
		case 46:                      /* EOF */
			ret=NULL;
			goto done;
		case 47:                      /* NOP */
			break;
		case 48:                      /* ^O  */
			/*putchar( CTRL+'O' );*/
			putchar( '\017' );
			redraw();
			break;
		case 49:                      /* ^G  */
			/*putchar( CTRL+'G' );*/
			putchar( '\007' );
			break;

		case 50:                      /* FKey */
			sprintf(typeahd,"%c%d",Param[0]>=10?'F':'f',Param[0]%10+1);
			if (s = get_var(LEVEL_SET, typeahd)) {
				tyahdptr = strcpy(typeahd,s);
				a0tospace( tyahdptr );
			}
			break;
		case 51:                      /* RawRaw */
			if( Param[0]==10 ) {   /* P0=class P2=code */
				int num=MENUNUM( Param[2] ), item=ITEMNUM( Param[2] );
				tyahdptr="";
				if( num>=0 && num<MAXMENUS && item>=0 && item<MAXMENUITEMS )
					tyahdptr=MenuCommand[num][item];
			}
			if( Param[0]==11 ) {
				strcpy(line,"quit");
				goto done;
			}
			break;
		case 52:                      /* Undo   */
			back(Pos);
			t=Len; Len=undo_len; undo_len=t;
			t=Pos; Pos=undo_pos; undo_pos=t;
			swapmem( Undo, line, MAX( Len, undo_len)+1 );
			printf("\033[J%s",line);
			back(Len-Pos);
			break;
		case 53:                      /* repeat */
			break;
		case 54:                      /* swap   */
			if( (t=Pos==Len?Pos-1:Pos)>0 )
				c=line[t-1], line[t-1]=line[t], line[t]=c;
			redraw();
			break;
		case 55:                      /* quote char */
			{
			int qkey = getkey();
			if (qkey>=0 && qkey<=255)
				sprintf(tyahdptr=typeahd,"\\%03.3o",qkey);
			}
			break;
		}
		if( fn != -2 )
			LastFn=fn, LastKey=key;
	}
	ret=NULL;
done:
	if( ret )
		printf("\033[%dC\n", Len-Pos );
	newwidth();
	if( eav ) free_expand(eav);
	SETCON;

	if (prio_set)
		SetTaskPri((struct Task *)Myprocess,oldprio);

	return ret;
}

static int
getkey(void)
{
	int esc=0, key, c;
	long *par;

	key = -1;

rbabelhack:
	if ( (c=myget()) == 27 ) {		/* ESC */
		esc=ESC;
		if ( (c=myget()) == '[')	/* CSI */
			c=155, esc=0;
	}

	/* siehe RKM:Devs, Seite 399 fuer einen GUTEN ISO-Parser */
	if (c == -1)
		return -1;

	if (c == 155) {				/* CSI */
		switch ( c=myget() ) {
		case 'A': key=256;       break; /* CursUp */
		case 'B': key=257;       break; /* CursDn */
		case 'C': key=258;       break; /* CursRt */
		case 'D': key=259;       break; /* CursLt */
		case 'T': key=256+SHIFT; break; /* SCursUp */
		case 'S': key=257+SHIFT; break; /* SCursDn */
		case ' ':
			switch( myget() ) {
			case '@': key=258+SHIFT; break; /* SCursRt */
			case 'A': key=259+SHIFT; break; /* SCursLt */
			}
			break;
		case 'Z': key= 9+SHIFT;       break; /* STab    */
		case '?': key= 260; myget();  break; /* Help    */
		case -1 : return -99;
		default :
			if( c>='0' && c<='9' ) {
				myunget(c);
				par=Param;
				do {
					for( *par=0; (c=myget())>='0' && c<='9';  )
						*par=10* *par + c-'0';
					par++;
				} while( c==';' );
				if( c=='~' ) {
					key=Param[0]+261;
					if( key>270 ) key+=SHIFT-10;
				}
				else if ( c=='|' ) key=271;
				else {
					while ( c>=' ' && c<='?' && c != -1 )
						c=myget();
					/* now: c>='@' && c<='~' */
				}
			} else {
				key = c;
			}
		}
	}
	else
		key = c;	/* normal char (no ESC, no CSI) */

	if (key == -1 && c != -1)	/* really EOF ? */
		goto rbabelhack;

	return key+esc;
}

static void
back(int n)
{
	if( n>0 ) printf("\033[%dD",n);
}

static void
left(int n)
{
	if( n>Pos ) n=Pos;
	if( n<=0  ) return;
	back(n);
	Pos-=n;
}

static void
right(int n)
{
	if( n+Pos>Len ) n=Len-Pos;
	if( n<=0      ) return;
	printf("\033[%dC",n);
	Pos+=n;
}

static int
leftwordlen( void )
{
	char  *ptr= Line+Pos;
	while( ptr>Line && *(ptr-1) == ' ' ) ptr--;
	while( ptr>Line && *(ptr-1) != ' ' ) ptr--;
	return (Line+Pos)-ptr;
}

static int
rightwordlen( void )
{
	char  *ptr= Line+Pos;
	while( ptr<Line+Len && *ptr != ' ' ) ptr++;
	while( ptr<Line+Len && *ptr == ' ' ) ptr++;
	return ptr-(Line+Pos);
}

static void
delete( int cnt )
{
	if( !cnt ) return;
	memmove( Line+Pos, Line+Pos+cnt, Len-Pos-cnt );
	memset ( Line+Len-cnt, ' ', cnt );
	printf("%s",Line+Pos);
	back(Len-Pos);
	Line[ Len-=cnt ]=0;
}

static void
history( int hnum )
{
	HIST *hist;

	for( hist=H_head; hist && hnum--; hist=hist->next) ;
	display( hist ? hist->line : "");
}

static void 
display( char *str )
{
	left(Pos);
	strcpy(Line,str);
	printf("\033[J%s",str);
	Pos= Len= strlen(str);
}

static void
redraw( void )
{
	back(Pos);
	printf("\015\033[J%s%s",Prompt,Line);
	back(Len-Pos);
}

static int wleft, wdel;

static void
getword( char *buf )
{
	char *beg, *end, *l=Line;

	for( end=l+Pos; *end  && *end!=' '; ++end ) ;
	for( beg=l+Pos; beg>l && !index(" <>;",*(beg-1)) ; beg-- ) ;
	memcpy( buf, beg, end-beg );
	buf[end-beg]=0;
	wleft= (l+Pos)-beg;
	wdel = end-beg;
}

static void
delword()
{
	left( wleft);
	delete( wdel );
}



static void
abbrev( char *buf, char **eav, int eac )
{
	int i, j, radlen=9999;

	/*if( eac>1 ) putchar( CTRL+'G' );*/
	if( eac>1 ) putchar( '\007' );	/* ^G */

	strcpy( buf, eav[0] );
	for( i=0; i<eac; i++ ) {
		if ( (j=strlen(eav[i])) < radlen ) radlen=j;
		for( j=0; j<radlen && tolower(eav[0][j])==tolower(eav[i][j]); j++ ) ;
		if ( j<radlen ) radlen=j;
	}
	buf[radlen]=0;
}



static int must_be_quoted(char *s)
{
	if (!*s)
		return 1;
	for ( ; *s; s++)
		if (ISSPACE(*s) || *s==';' || *s=='|' || *s=='#' || *s=='^'
		     || *s=='?' || *s=='*' || *s=='&' || *s=='$' || *s=='!'
		     || *s=='~' || *s=='`' || *s=='\'')
		   return 1;
	return 0;
}

static void quote( char *buf, char *sep )
{
	int len=strlen(buf), dir=isdir(buf);

	if (must_be_quoted(buf)) {
		memmove(buf+1,buf,len);
		buf[0]=buf[++len]='\"';
		buf[++len]=0;
	}
	strcat(buf,dir?"/":sep);
}



void
setrawcon( long flag, int ievent ) /* -1L=RAW:, 0L=CON: */
{
	struct FileHandle *fh;
	static char menuon, button;

	if( !o_vt100 && !o_nowindow && ievent && flag==0 && menuon)
		printf("\033[10}"), menuon=0;

	if (fh = BADDR( Input() )) {
		if ( fh->fh_Type )
			DoPkt( (void *)fh->fh_Type, ACTION_SCREEN_MODE, flag,  NULL,NULL,NULL,NULL );
			/*DoPkt( (void *)Myprocess->pr_ConsoleTask, ACTION_SCREEN_MODE, flag,  NULL,NULL,NULL,NULL );*/
	}

	if( !o_vt100 && !o_nowindow && ievent && flag == -1 ) {
		if( !menuon )
			printf("\033[10{"), menuon=1;
		if( !button )
			printf("\033[11{"), button=1;
	}

	fflush(stdout);
}


static int row, height, cnt;
static char scrollstr[10];
static int noquick=1;


extern BPTR OldCin;

static int FromTee;

static UBYTE
mygetchar(void)
{
	UBYTE c;
	Read( Input(), &c, 1 );
	return c;
}

#if 0
void
prepscroll( int fromtee )
{
	BPTR truecin=0;
	long param[8];

	row=height=0;
	FromTee=fromtee;

	if(( noquick=!o_scroll || o_noraw || o_nofastscr || o_vt100 ))
		return;

	if(( noquick=!IsInteractive(Myprocess->pr_COS) && !fromtee ))
		return;

	if( !IsInteractive(Myprocess->pr_CIS)) {
		truecin=Myprocess->pr_CIS;

		if( noquick=!IsInteractive(OldCin) )
			return;

		Myprocess->pr_CIS = DEVTAB(stdin) = OldCin;
	}

	if( !CHARSWAIT(stdin) ) {
		SETRAW;
		Write(OldCin,"\033[ q",4);
		get_seq( param );
		height=param[2];
		while( mygetchar()!='r') ;

		Write(OldCin,"\033[6n",4);
		get_seq( param );
		row=param[0];

		SETCON;

		cnt= height-row+1;
		noquick= height<o_minrows;
	}

	sprintf(scrollstr,"\033[%cS\033[%cA", o_scroll+'0', o_scroll+'0');

	if( truecin )
		Myprocess->pr_CIS = DEVTAB(stdin) = truecin;
}
#else
/* old prepscroll() with isconsole() instead of just IsInteractive() */
void
prepscroll( int fromtee )
{
	BPTR truecin=0;
	long param[8];

	row=height=0;
	FromTee=fromtee;

	if(( noquick=!o_scroll || o_noraw || o_nofastscr || o_vt100))
		return;

	if(( noquick=!isconsole(Myprocess->pr_COS) && !fromtee ))
		return;

	if( !isconsole(Myprocess->pr_CIS)) {
		truecin=Myprocess->pr_CIS;

		if( noquick=!isconsole(OldCin) )
			return;

		Myprocess->pr_CIS = DEVTAB(stdin) = OldCin;
	}

	if( !CHARSWAIT(stdin) ) {
		SETRAW;
		Write(OldCin,"\033[ q",4);
		get_seq( param );
		height=param[2];
		while( mygetchar()!='r') ;

		Write(OldCin,"\033[6n",4);
		get_seq( param );
		row=param[0];

		SETCON;

		cnt= height-row+1;
		noquick= height<o_minrows;
	}

	sprintf(scrollstr,"\033[%cS\033[%cA", o_scroll+'0', o_scroll+'0');

	if( truecin )
		Myprocess->pr_CIS = DEVTAB(stdin) = truecin;
}
#endif

static int
get_seq( long *param )
{
	int c;

	while( (c=mygetchar())!=155 ) ;
	do {
		*param=0;
		while( (c=mygetchar())>='0' && c<='9' )
			*param=10* *param + c-'0';
		param++;
	} while( c==';' );

	return c;
}


void
quickscroll( void )
{
	if( noquick ) return;

	if( --cnt<=0 ) {
		cnt=o_scroll;
		fprintf( FromTee ? stderr : stdout, "%s",scrollstr);
	}
}

int
do_keymap( void )
{
	int i, n, len;
	USHORT *tmp, *put, *get, *map;
	char   *ind;

#if 0
	if( ac==1 ) {
		for( get=Keymap[0]; *get; get+=2 )
			printf("%4d %4d\n",get[0],get[1]);
		return 0;
	}
#endif

	n=myatoi(av[1],0,7);
	if( atoierr ) return 20;

	map=Keymap[n]; len=0;
	if( map )
		for( len=0; map[2*len]; len++ ) ;

	put=tmp=salloc((len+ac)*2*sizeof(USHORT));
	for( i=2; i<ac; i++ ) {
		if( !(ind=index(av[i],'='))) {
			ierror( av[i],500);
			free( tmp );
			return 20;
		}
		*put++=atoi(av[i]);
		*put++=atoi(ind+1);
	}

	for( i=0; i<len; i++ ) {
		for( get=tmp; get<put; get+=2 )
			if( *get==map[2*i] )
				break;
		if( get==put ) {
			*put++=map[2*i];
			*put++=map[2*i+1];
		}
	}
	*put++=0;
	*put  =0;

	if( map && map!=DefKeymap0 && map!=DefKeymap1 )
		free( map );
	Keymap[n]=tmp;
	Curmap=0;

	return 0;
}

static int
myget( void )
{
	int c;
	UBYTE uc;

	if( unget )
		c=unget, unget=0;
	else if( tyahdptr && *tyahdptr ) {
		c = *tyahdptr++;
	} else {
		tyahdptr=NULL;
		fflush(stdout);
		/* replacement for c=getchar() */
		if (Read( Input(), &uc, 1) != 1)
			c = -1;
		else
			c = uc;
	}

	return c;
}

static void
myunget(int c)
{
	unget=c;
}

int
newwidth( void )
{
	get_WindowBounds_def(&w_width,&w_height);	/* 80 x 24 */
	if (!get_WindowBounds_env(&w_width,&w_height)) {
		if ( !o_nowindow && Mywindow ) {
			w_width =(Mywindow->Width - Mywindow->BorderLeft - Mywindow->BorderRight)/
			          Mywindow->RPort->TxWidth;
			w_height=(Mywindow->Height- Mywindow->BorderTop  - Mywindow->BorderBottom)/
			          Mywindow->RPort->TxHeight;
		}
		else if ( !o_noraw && !o_vt100 ) {
			get_WindowBounds_Output(&w_width,&w_height,o_timeout);
		}
	}
	/* kprintf("newwidth:  %ld × %ld  (timeout %ld)\n",w_width,w_height,o_timeout); */
	if( w_width<1 ) w_width=1;	/* crash after resizing was reported */
	return w_width;
}



#else

void prepscroll(int fromtee) {}
void quickscroll(void) {}
int  do_keymap( void ) {}
void setrawcon( long flag, int ievent ) {}
int  newwidth( void ) {}
void initmap(void) {}

#endif

extern struct MsgPort *Console;

#if 0
int isconsole( BPTR fh )
{
	return( IsInteractive(fh) );
}
#else
int isconsole( BPTR fh )
{
	return ((struct FileHandle *)(4*fh))->fh_Type==Console &&
		IsInteractive(fh);
}
#endif

