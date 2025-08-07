# set of aliases for transparent usage of ObjectArchive:
# cd - changes current dir, no matter if target is a plain dir, lha or tar
#      archive. Previous current dir is pushed to the stack, if you've
#      entered the archive.
# b  - changes the current directory to the one on the top of the stack,
#      and remove this one from the stack. If stack is empty, changes dir
#      to parent.
# st - shows contents of the stack.
# push - pushes current directory to the stack
# pop - discards the directory from the top of the stack

# zestaw aliasów do wspóîpracy z ObjArchive:
# cd - zmiana katalogu niezaleûnie od tego czy argument jest plikiem .lha
#      .tar czy teû katalogiem. poprzedni bierzâcy katalog wëdruje na stos,
#      jeôli nastâpiîo wejôcie do archiwum.
# b  - zmiana katalogu na najwyûszy na stosie i zdjecie go ze stosu. Po
#      wyczerpaniu stosu, zmiana katalogu na parent.
# st - wyôwietla zawartoôê stosu.
# push - odkîada na stosie bieûâcy katalog
# pop - zdejmuje ze stosu ostatnio odîoûony katalog

alias cd "%dir\
  local arcfile;\
  if -nd $dir;\
    if -e $dir;\
      if $dir = @match( $dir \"*.tar.(gz|z)\" );\
        set arcfile @basename( @nameroot( @nameroot( $dir ) ) )\":\";\
      else;\
        set arcfile @basename( @nameroot( $dir ) )\":\";\
      endif;\
      mountarchive $dir;\
      cd $arcfile;\
      unmount $arcfile;\
      unset arcfile;\
      if -v _cd;\
        set _cd $_cwd $_cd;\
      else;\
        set _cd $_cwd;\
      endif;\
    else;\
      echo $dir is not a directory or archive;\
    endif;\
  else;\
    cd $dir;\
  endif"

alias b "\
  local __cd;\
  if -v _cd;\
    if -r @words( $_cd ) 1 == ;\
      set __cd $_cd;\
      unset _cd;\
    else;\
      set __cd @word( $_cd 1 );\
      set _cd @delword( $_cd 1 );\
    endif;\
  endif;\
  if -v __cd;\
    if -d $__cd;\
      cd $__cd;\
    else;\
      echo No dir: $__cd;\
    endif;\
  else;\
    cd ..;\
  endif"

alias push "\
  if -v _cd;\
    set _cd $_cwd $_cd;\
  else;\
    set _cd $_cwd;\
  endif"

alias pop "\
  if -v _cd;\
    if -nr @words( $_cd ) 1 == ;\
      set _cd @delword( $_cd 1 );\
    else;\
      unset _cd;\
    endif;\
  else;\
    echo \"<stack empty>\";\
  endif"

alias st\
  "if -v _cd;\
    foreach elt ( $_cd ) {\
      echo $elt\
    };\
  else;\
    echo \"<stack empty>\";\
  endif"

