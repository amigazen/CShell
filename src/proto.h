/* main.c */
int main(int argc, char **argv);
void main_exit(int n);
int breakcheck(void);
int dobreak(void);
long Chk_Abort(void);
void _wb_parse(void);
int do_howmany(void);
void breakreset(void);

/* comm1.c */
int do_sleep(void);
int do_chgrp(void);
int do_chmod(void);
int do_chown(void);
int do_protect(void);
int do_filenote(void);
int do_cat(void);
char *add_simple_device(char *,char *);
void get_drives(char *);
char *drive_name(char *);
char *oneinfo( char *,int);
char *strlast(char *,char);
int do_info(void);
int do_dir(void);
void lformat( char *s, char *d, struct file_info *info );
int do_quit(void);
int do_echo(void);
int do_source(char *str );
int do_pwd(void);
int do_cd(void);
char *quick_cd( char *buf, char *name, int repeat );
int do_mkdir(void);
int do_mv(void);
int all_args( int (*action)FUNCARG(long,char*,char*), int dirs );
int all_args_n2m( int (*action)FUNCARG(long,char*,char*), int dirs, int aa_from, int aa_to );
int do_search(void);
int do_rm(void);
int do_history(void);
int do_mem(void);
int do_forline(void);
int do_fornum(void);
int do_foreach(void);
int do_forever(char *str);
int do_window(void);
char *dates(struct DateStamp *dss, int flags);
int do_date(void);
void set_cwd(void);

/* comm2.c */
int do_abortline(void);
int do_return(void);
int do_strhead(void);
int do_strtail(void);
int do_if(char *garbage, int com);
int do_label(void);
int do_goto(void);
int do_inc(char *garbage, int com);
int do_input(void);
int do_version(void);
int do_ps(void);
int do_copy(void);
int do_touch(void);
int do_addbuffers(void);
int do_relabel(void);
int do_diskchange(void);
int dofunc(int id, char **av, int ac);
int do_error( void );

/* comm3.c */
int do_ln(void);
int do_tee(void);
int do_head(char *garbage, int com);
void man(FILE *f, char *s);
int do_man(void);
int do_assign(void);
char **expand_devs(void);
int do_join(void);
int do_strings(void);
int do_open(void);
int do_close(void);
void myclose(int n);
int do_fileslist(void);
long extOpen(char *name, long mode);
void extClose(long fh);
int do_basename(void);
int do_tackon(void);
int do_resident(void);
int loadres(char *s);
int do_truerun(char *avline, int backflag);
int exists(char *name);
int mounted( char *str );
int do_aset(void);
int do_htype(void);
int do_stack(void);
int do_fault(void);
int eval_rpn(char **av, int ac, int flag);
int do_rpn(char *garbage, int ifflag);

/* rehash stuff */
char *get_rehash_prog(char *,char *);
void resolve_multiassign(char *,char ***,long *);
int do_rehash(void);

int do_path(void);
int do_pri(void);
int do_strleft(void);
int do_strright(void);
int do_strmid(void);
int do_strlen(void);
int myatoi(char *s, int mmin, int mmax);
int unlatoi(char *s);
int posatoi(char *s);
int do_fltlower(void);
int do_fltupper(void);
int do_linecnt(void);
int do_uniq(void);
int do_rxsend(char *avline);
int do_rxrec(void);
int do_rxreturn(void);
int do_waitport(void);
int do_ascii(void);
void appendslash(char *path);
int do_whereis(void);
int do_usage(void);
int do_menu(void);
void remove_menu(void);
void set_menu(void);
int do_getenv(void);
int do_setenv(void);
char **read_file(FILE *file, int *ac);
void free_file(char **ptr);
int do_qsort(void);
int do_truncate(void);
int do_split(void);
int do_action( char *argline );
int do_class( char *avline );
int do_readfile( void );
int do_writefile( void );

/* execom.c */
int isnum(char *);
void *mymalloc(int len);
int exec_command(char *base);
#ifndef isalphanum
int isalphanum(char c);
#endif
char *exec_function(char *str, char **fav, int fac);
int do_help(void);
void exec_every(void);
void show_usage(char *str);
int do_exec(char *str);
char *a0tospace(char *str);
int execute( char *str );
char *find_internal(char *str);
int get_opt(char **av, int *ac, int ccno, int *err);
int hasspace( char *s );
int myfgets( char *buf, FILE *in );
char lastch(char *);
ULONG BtoCStr(char *, BSTR, LONG);
ULONG CtoBStr(char *, BSTR, LONG);


/* sub.c */
int issep( char s );
char *getclass(char *file);
void seterr(int err);
char *next_word(char *str);
char *compile_av(char **av, int start, int end, char delim, int quote);
void Free(void *ptr);
void add_history(char *str);
char *get_history(char *ptr);
int find_history(char *ptr,int min);
void replace_head(char *str);
void pError(char *str);
int ierror(char *str, int err);
char *ioerror(int num);
struct dirptr *dopen(char *name, int *stat);
int dclose(struct dirptr *dp);
int isdir(char *file);
void free_expand(char **av);
char **expand(char *base, int *pac);
int newrecurse(int mask, char *name, int (*action)FUNCARG(long,char*,char*));
char *strdel(char *,int,int);
char *strupr(char *s);
char *strlwr(char *s);
struct pattern *compare_preparse( char *wild, int casedep );
int compare_ok( struct pattern *pat, char *name);
void compare_free( struct pattern *pat );
int compare_strings( char *pat, char *str, int casedep );
void expand_all(char *name, FILE *file);
int cmp(struct file_info *s1, struct file_info *s2);
int cmp_case(struct file_info *s1, struct file_info *s2);
int sizecmp(struct file_info *s1, struct file_info *s2);
int datecmp_csh(struct file_info *s1, struct file_info *s2);
int classcmp(struct file_info *s1, struct file_info *s2);
int numcmp( struct file_info *s1, struct file_info *s2 );
void QuickSort(char **av, int n);
void DirQuickSort(char **av,int n,int (*func)(struct file_info *,struct file_info *), int rev, int fac);
int filesize(char *name);
char **and(char **av1, int ac1, char **av2, int ac2, int *ac, int base);
char **without(char **av1, int ac1, char **av2, int ac2, int *ac, int base);
char **or(char **av1, int ac1, char **av2, int ac2, int *ac, int base);
void clear_archive_bit(char *name);
char *itoa( int i );
char *itok( int i );
char *getaction( char *class, char *action );
int doaction( char *file, char *action, char *args );
void *salloc( int len );
void *SAllocMem( long size, long req );
void setioerror( int err );
char *filemap( char *buf, int lcd );
char *safegets( char *buf, FILE *in );
BOOL add_array_list(char ***,long *,char *);
void free_array_list(char **,long);
BOOL SetOwner37(UBYTE *,LONG);
long setProtection(char *,long);

/* set.c */
void init_mbase(void);
void set_var(int level, char *name, char *str);
void set_var_mem( int level, char *name, char *str );
void update_sys_var( char *name );
void set_var_n(int level, char *name, char *str, int n);
void *get_var(int level, void *name);
void unset_level(int level);
void unset_var(int level, char *name);
int do_unset_var(char *str, int level);
int do_set_var(char *command, int level);
void push_locals( struct VRoot *newroot );
void pop_locals( void );
int do_local(void);

/* rawcon.c */
int newwidth(void);
void initmap(void);
char *rawgets(char line[], char prompt[]);
void prepscroll(int fromtee);
void quickscroll(void);
void setrawcon( long flag, int ievent );
int isconsole( BPTR fh );
int do_keymap( void );

/* run.c */
int do_run(char *str, int exec );
char *dofind(char *cmd, char *ext, char *buf, char *path);

int setenv(char *var, char *val);

