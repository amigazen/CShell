#!csh

if $_version < 520
    echo "This script need at least Csh 5.20"
    return 0
endif

# $VER: LhA2LZX.sh 0.95 (5/03/1995)
# Author : Bruno Rohée <rohee@univ-mlv.fr>
# Status : totally FREEWARE. (if you change anything to do something
#          buggy with this please remove my name :-))
#          NO WARRANTY is given.
# Usage :
# Copy this script in CSH: and execute (by typing LhA2LZX) it in the
# directory where you want LhA archive recursively converted to LZX.
# If the LZX file is bigger than the LhA file the script keeps
# the smallest.

# You MUST edit the following variables.
# LhA & LZX should be placed in RAM: to improve speed.

set LHA_COMMAND C:LhA # You can use LX but it have problems with some archives
set LZX_COMMAND C:LZX
set PRIORITY -1

# The directory were you can uncompress archive
# You must have enough place to uncompress your biggest archive
# The directory MUST already exists
# Don't forget the ':' or the '/' at the end of the name

set UNCOMPRESS_DIR T:LhA2LZX_TempDir/

if -nd $UNCOMPRESS_DIR
   echo "The directory "$UNCOMPRESS_DIR" musts exist."
   return 0
endif

# Begining of the real script

pri 0 $PRIORITY

class lharc     offs=2,2D6C68..2D # an usefull definition

foreach i ( * ) {
    # if it's a directory
    if -d $i
        cd $i
        LhA2LZX
        cd /
    # if it's a file do nothing if it's not a LhA and repack it if it is.
    else
        if @dirstr( %k $i ) = "lharc" # if we got a LhA archive

            # unarchive it
            $LHA_COMMAND -a -F -M x $i $UNCOMPRESS_DIR

            # make the new archive
            $LZX_COMMAND -r -e -M500 -3 -F a @strhead( \. $i ) $UNCOMPRESS_DIR\#\?
            #                                                                 ^^^^
            #                                                     This wildcard to have
            #                                           directory names expanded with the '/'

            # delete decompressed files
            rm -pr $UNCOMPRESS_DIR*

# if you want an "ALWAYS LZX" option delete here
# cut here __________

            # delete the bigger archive (keep the best)
            # If LZX is less efficient than LhA
            if @filelen( @strhead( \. $i ).LZX ) > @filelen( $i )
                rm @strhead( \. $i ).LZX
                echo "Lha is better for this file"
            else
                rm $i
                echo "LZX is better for this file"
            endif

# cut here __________
# and add :
#            rm $i

        endif
    endif
}

# let things clear

unset LHA_COMMAND
unset LZX_COMMAND
unset UNCOMPRESS_DIR
unset PRIORITY

pri 0 0

