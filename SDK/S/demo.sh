# This is a comment:
# --------------------------- Main menu ------------------------------

# check if we're running a csh 5.16 or above. $_version contians the current
# version, and the return command returns from a batch file.

if $_version < 516
 echo "Sorry, csh version 5.16 needed";return 0
endif



cls  # clears the screen

# The echo command is used for printing lines to the screen.  Note that a
# trailing quote is not necessary

echo "This is a sample script for csh. It contains quite a few useful
echo "aliases you might want to keep permanently, but its main purpose
echo "is to be used as a tutorial. Look at it and modify it.
echo



# Every csh command accepts ^-codes in its command line, e.g. ^A is 
# the same as CTRL-A, which is ASCII code 1. This can be used to gene-
# rate escape sequences.

echo -n \033\13330m\033\13341m                    # reverse video, no CR
echo -n @center( " C H O I C E S " 76 ) # centered display
echo    \033\1330m
echo
echo "[1] Prime numbers
echo "[2] 'More' text viewer
echo "[3] Labyrinth game
echo "[4] Towers of Hanoi game
echo "[5] Backup/Version control system
echo "[6] Useful aliases
echo "[7] Mansplit, split up your csh.doc



# The input command reads a line from the console and stores it in
# the indicated variable.

echo -n "> "
input choice

goto Start$choice   # Jumps to label Start1 or Start2 and so on





# ------------------------- Prime number -----------------------------
# Interesting about this program is the use of the 'fornum' command
# and the tricks used the generate arrays. The information whether
# 93 is prime or not is stored in variable $prime93 .

label Start1    # entrypoint... here we get from the goto

set max 100     # highest number checked for prime
echo "Computing prime numbers from 1 to "$max
fornum i 2 $max "set prime$i 1

# This is a statement block. Everything will be concatted on one line
# internally, separated by semicolons. Therefore, ending quotes are
# mandatory and comments are forbidden.
fornum i 2 $max {
 exec "set bool $prime"$i
 if $bool
  echo -n $i^i
  fornum -s j $i $max $i "set prime$j 0"
 endif
}
echo

return 0




# ------------------------------ More -------------------------------
# This script does about what the program 'more' does. It is supposed
# to demonstrate the file I/O commands and the file handling functions.

label Start2

# the help screen for 'more', defined as an alias
alias morehelp {
 echo "  space     down one page"
 echo "  enter     down one line"
 echo "  backspace up one page"
 echo "  <         top of file"
 echo "  >         bottom of file"
 echo "  ESC, q    quit"
 echo "  h         help"
}

echo "This is a simple approach to programming a text viewer as a csh script.
echo "Functions:"
echo
morehelp
echo

echo -n "Enter name of text file to view: "
input text
#set text test
if -nf $text;echo "File not found";return 20;endif

# This reads the file into memory. Every line will be one 'word', even
# if it contains blanks. For definition of word, refer to csh.doc.
readfile file $text
# Every use of $file will from now one generate a HUGE command line.
set pos    1
set page   @rpn( @winrows( ) 1 - )
set lines  @flines( file )
set bottom @rpn( $lines $page - 1 + )


label Disp

cls
set screen @subfile( file $pos $page )   # store screenful in $screen
writefile screen
                                           # compute & display percentage
set perc @rpn( $pos $page + 1 - 100 \* $lines / 100 MIN )
echo -n \033\1333m\033\13330m\033\13341m "-- More -- ("$perc"%) "\033\1330m ""


input -r c    # inputs one single keystroke without waiting for CR
set c x$c     # prevents '<' from being misinterpreted in 'if'

if $c = x" ";if $pos = $bottom;unset file;return 0;endif;endif
if $c = x" ";inc pos $page;if $pos > $bottom;set pos $bottom;endif;endif
if $c = x^m ;inc pos   1  ;if $pos > $bottom;set pos $bottom;endif;endif
if $c = x^h ;dec pos $page;if $pos < 1;set pos 1;endif;endif
if $c = x"<";set pos 1;endif       
if $c = x">";set pos $bottom;endif 
if $c = xq  ;unset file;return 0;endif
if $c = x^[ ;unset file;return 0;endif
if $c = xh  ;cls;morehelp;echo "Press any key";input -r a;endif
#if $c = x/  ;set char xf;endif
#if $c = xf
# echo -n "Search string: ";input str;search -fq $text $str | input found;
# set found @subwords( $found 2 1000 );set c xn
#endif
#if $c = xn
# set hop 0
# foreach i ( $found ) "if $i > $pos;if $hop = 0;set hop $i;endif;endif
# if $hop;set pos $hop;endif
#endif

goto Disp





# ---------------------------- Labyrinth -------------------------------
# I always wanted to do an action game as a script :-)
# The laby is stored as one single string, with every line one word.
# Feel free to modify it, the size is detected automatically. Don't
# use auto key repeat when playing, this can cause console trouble.

label Start3

set lab      "#########################"
set lab $lab "#   #     #   #     #   #"
set lab $lab "# # # ### ### ### # ### #"
set lab $lab "# #   # #   #     # #   #"
set lab $lab "# ##### ### ### ### # ###"
set lab $lab "#   #   #   #   # #     #"
set lab $lab "### # ### ### # # ##### #"
set lab $lab "#       #     #   #      "
set lab $lab "#########################"

cls
writefile lab
echo -n ^j"8=up 2=down 4=left 6=right "^j\033\1332A
set x 2
set y 2
set wid @strlen( @word( $lab 1 ) )
set up  @words( $lab )
echo -n \033\133$up\A\033\133B\033\133C.\033\133D
alias test "%a%b set xx @rpn( $x $a + );set yy @rpn( $y $b + );\
 set f @strmid( @word( $lab $yy ) $xx 1 )


date -s

label Loop

input -r c
if $c = 8;test 0 -1;if -n $f = "#";echo -n " "^H\033\133A.^H;dec y;endif;endif
if $c = 2;test 0  1;if -n $f = "#";echo -n " "^H\033\133B.^H;inc y;endif;endif
if $c = 6;test  1 0;if -n $f = "#";echo -n " "^H\033\133C.^H;inc x;endif;endif
if $c = 4;test -1 0;if -n $f = "#";echo -n " "^H\033\133D.^H;dec x;endif;endif
if $x >= $wid;cls;echo Congrats, escaped in `date -r` seconds.;return 0;endif
goto Loop

return 0



# ------------------------------ Towers of hanoi -----------------------------
# This game has been optimized for speed, and is therefore not very nice
# to look at.

label Start4

set height 7 # set to uneven numbers up to 7

set h2 $height;inc h2
cls
unset t
set t1 "9"; set t2 "9"; set t3 "9"   # the three towers
fornum -s i $h2 2 -1 "set t1 $i$t1
set done $t1

#prepare imaging
set im "x";set h @rpn( $height 2 \* 1 + )
fornum -s i 1 $h 2 "set im $im @center( @strleft( XXXXXXXXXXXXXXX $i ) $h )
fornum i $height 10 "set im $im \"               \"

cls
echo
echo "T O W E R S   O F   H A N O I
echo "-----------------------------
echo
echo "This game is not very comfortable, but it works fine. You have to move
echo "a pile of disks from the first of three poles to the third one without
echo "ever putting a larger disk on a smaller one. To move the disk at the 
echo "top of pile one to pile three, first press '1' and then '3'. When you're 
echo "done, the time it took you will be printed, and you have to press 
echo "CTRL-C to end the game. You won't see any output but the stacks while
echo "the game is running.
echo
echo "PS: Tell me if you get below 60 seconds for one go...   *grin*
echo
echo "Press any key when ready...

input -r x
date -s
cls

#display stack
fornum  i 2 $h2 "echo \" \" @word( $im $i )
echo

set m1 1;set m2 17;set m3 33

forever {
 label Disp
 input -r x;input -r y
 exec "set src $t"$x";set dst $t"$y";set rt1 $m"$x";set rt2 $m"$y
 strleft move $src 1
 if a$move""a >= a$dst""b
  echo -n ^g
 else
  strlen up1 $src; strlen up2 $dst; inc up2
  echo -n \233$up1""A\233$rt1""C"                "^m\233$up1""B\
\233$up2""A\233$rt2""C @word( $im $move ) ^m\233$up2""B
  set t$y $move$dst
  strmid t$x $src 2
 endif
 if $t3 = $done;echo ^j^j;date -r;endif
}

return 0




# ---------------------- Backup/Version control -------------------------------
# Now this is a script you'd really want to use regularly. It backs up
# a set of files to a different directory, keeping any number of old
# old versions. Extract this script and call it from your .login or
# whenever you start your C development system. By setting a delay of
# zero you make sure that every time this script a backup is done.

label Start5

set versions 3         # maximum number of versions kept per file
set delay    1         # minimum number of days between backups
set dest     bak       # the destination subdirectory of the backup
set pattern  "*.c *.h" # the pattern for files to back up
set exlude   "x.c y.c" # the files NOT to back up


#----- set up environment -----
if -nd $dest
 echo -n "Directory "$dest" not present... create? y/n: ";input x
 if $x = y;mkdir $dest;else;echo "Aborted...";return 0;endif
endif

if -f $dest/timestamp
 set last @age( $dest/timestamp )
 if $last < $delay
  echo "No backup done... last backup was "$last" day(s) ago."
  return 0
 endif
else
 echo >$dest/timestamp
endif


#----- determine altered files ----
echo "Checking files...."
exec set all $pattern              # wild card expansion happens here

set cp ""                          # the files to copy
foreach i ( $all ) "if -nt $i $dest/$i;set cp $cp $i;endif

exec set exclude $exclude          # wild card expansion happens here
set cp @without( $cp , $exclude )

if $cp = "";echo "All done.";touch $dest/timestamp;return 0;endif
echo "The files to back up:"
echo $cp
# Uncomment the following line if you want a sanity check...
# set cp @confirm( Backup $cp )
if $cp = "";echo "All done.";touch $dest/timestamp;return 0;endif


#------ do the backup -------
alias name "%a set n $dest/$i;if $a;set n $n.$a;endif

foreach i ( $cp ) {
 set v $versions;dec v
 name $v;dec v
 if -f $n;rm $n;endif
 fornum -s j $v 0 -1 "set hi $n;name $j;if -f $n;mv $n $hi;endif"
 cp $i $dest
}

touch $dest/timestamp
echo "All done."

return 0





# ---------------------- The useful aliases -------------------------------
# Read the comments for every single alias to find out which ones you
# like. Copy those into your login.

label Start6

# first a few useful aliases for dir
alias  d   "*a dir @pickopts( $a ) -ze \"%6s %-.12n%m\" @pickargs( $a )
alias  lst "*a ls -t $a"    # sorts by access time
alias  lsl "*a ls -l $a"    # sorts by length
alias  move "cp -m

# sc searches *.c, even 'sc -c main()' works
alias  sc "%a search @pickopts( $a ) *.c @pickargs( $a )

# edf edits a function in CygnusEd if the name starts in the first column:
alias  edf {
%func 
 set b ""
 search -afl *.c $func | inp b
 if $b
  split b file line
  lced $file
  waitforport rexx_ced
  inc line 1
  rxsend rexx_ced "jump to file "$file "jumpto "$line" 0"
 else
  echo Not found
 endif
}

# this aliases suppress wild card expansion for certain commands
alias  zoo     "*a Zoo $a
alias  lharc   "*a Lharc $a
alias  lz      "*a Lz $a
alias  newlist "*a Newlist $a
alias  eval    "*a Eval $a


# this one will show all pictures, the newest first
alias  newpix  "ls -nt | forline i STDIN \"ShowIFF $i

# if you want to run cshell internal commands in the backrund, use rs
alias  rs      "rback csh -c

# some aliases for UNIX compatibility

# this ceates a chmod command that expects the bits first and the files after
alias chmod "%a protect @subwords( $a 2 9999 ) @first( $a )

# the same with grep:
alias grep "%a search @subwords( $a 2 9999 ) @first( $a )

set stk ""
# pushd pushes the current directory on a stack
alias  pushd "%a set stk $_cwd @subwords( $stk 1 10 );\\cd $a;e $stk

# popd  retrieves it from there
alias  popd  "\\cd @first( $stk );set stk @subwords( $stk 2 10 );e $stk

echo "Useful aliases defined. Read this batch file to find out their
echo "names and what they do.

return 0





# ----------------------- Mansplit.sh - splits up csh.doc ------------------
# Splits up csh.doc in small files, one per man entry. So external 'man'
# programs can work with it, which is usually faster then csh's own method.

label Start7

echo Patience...
set dest ram:cshdoc/           # set your destination directory here
if -nd ram:cshdoc;mkdir ram:cshdoc;endif   # ...and here
set mode skip
forline i csh:csh.doc {
 if $mode = write
  if @strleft( $i^a 1 ) < " " 
   echo >>$dest$file $i
  else
   set mode skip
  endif
 endif
 if $mode = skip
  if "    " = @strleft( $i 4 )
   set mode write
   set a @index( $i "(" )
   if $a > 0;dec a 1;strleft i $i $a;endif
   set file $i
   echo >$dest$file $i
   echo $file
  endif
 endif
}
echo "csh.doc split up and written to "$dest

return 0
