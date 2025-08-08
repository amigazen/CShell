# $VER: SplitAminet 1.2 (17.2.95)
#
# CSh script to create a split-up Aminet index file...
#
# Just cos its getting bloody huge these days...
#
# Written by Bil Irving, February 1995.
#
#	Internet:	mcscs3wji@zippy.dct.ac.uk
#	FidoNet:	2:259/24.100
#
# It's all done with 'search'.... :)
#
# Notes: This is going to take quite a long time to execute, because of the
#        way search works - it'll scan the entire (currently over 1MB) index
#        file for each directory entry. There's nothing I can do about this
#        really. However, it works very well - and once done can save you
#        loads in memory space on looking up a particular directory. One day
#        I might re-write the whole thing in Amiga E to make it smarter...
#        but until then...
#
# Revision history:
#
#	1.0		-	Used SAS/C 'grep' to scan the files... this sucked. :)
#				*Internal release*
#
#	1.1		-	Ditched 'grep' to go completely CSh, using 'search'. :-D
#				*First released version!*
#
#   1.2     -   Hmmm. V1.1 didn't make it onto Aminet, I suspect because I
#               was working with an old index file and the directories have
#               been updated (I only get a new index file once a month on
#               average). Anyway, this version now copes with the current
#               (as of the above version date string) directory structure
#               at ftp.luth.se.

# Set these variables to suit your system.

set aminet "COMMS:FileLists/Aminet.txt"		# Name of your master index file
set tmpdir "ram:tmp"		# Do you want to change this? If so, do so..
set destdir "COMMS:Aminet"	# This is where your split files will go
set grepopts "-nq"

# And thats it. What's below isn't worth looking at, unless you fancy a bit
# of S.M. ;)

if -n -f $tmpdir; mkdir $tmpdir; endif
echo "Processing index file... please wait a year... :("
echo " biz/dbase"
search >$tmpdir/biz.dbase.txt $grepopts $aminet biz/dbase
echo " biz/demo"
search >$tmpdir/biz.demo.txt $grepopts $aminet biz/demo
echo " biz/misc"
search >$tmpdir/biz.misc.txt $grepopts $aminet biz/misc
echo " biz/patch"
search >$tmpdir/biz.patch.txt $grepopts $aminet biz/patch
echo " comm/bbs"
search >$tmpdir/comm.bbs.txt $grepopts $aminet comm/bbs
echo " comm/cnet"
search >$tmpdir/comm.cnet.txt $grepopts $aminet comm/cnet
echo " comm/dlg"
search >$tmpdir/comm.dlg.txt $grepopts $aminet comm/dlg
echo " comm/envoy"
search >$tmpdir/comm.envoy.txt $grepopts $aminet comm/envoy
echo " comm/fido"
search >$tmpdir/comm.fido.txt $grepopts $aminet comm/fido
echo " comm/mail"
search >$tmpdir/comm.mail.txt $grepopts $aminet comm/mail
echo " comm/misc"
search >$tmpdir/comm.misc.txt $grepopts $aminet comm/misc
echo " comm/net"
search >$tmpdir/comm.net.txt $grepopts $aminet comm/net
echo " comm/news"
search >$tmpdir/comm.news.txt $grepopts $aminet comm/news
echo " comm/tcp"
search >$tmpdir/comm.tcp.txt $grepopts $aminet comm/tcp
echo " comm/term"
search >$tmpdir/comm.term.txt $grepopts $aminet comm/term
echo " comm/ums"
search >$tmpdir/comm.ums.txt $grepopts $aminet comm/ums
echo " comm/uucp"
search >$tmpdir/comm.uucp.txt $grepopts $aminet comm/uucp
echo " demo/aga"
search >$tmpdir/demo.aga.txt $grepopts $aminet demo/aga
echo " demo/euro"
search >$tmpdir/demo.euro.txt $grepopts $aminet demo/euro
echo " demo/intro"
search >$tmpdir/demo.intro.txt $grepopts $aminet demo/intro
echo " demo/mag"
search >$tmpdir/demo.mag.txt $grepopts $aminet demo/mag
echo " demo/mega"
search >$tmpdir/demo.mega.txt $grepopts $aminet demo/mega
echo " demo/par92"
search >$tmpdir/demo.par92.txt $grepopts $aminet demo/par92
echo " demo/par94"
search >$tmpdir/demo.par94.txt $grepopts $aminet demo/par94
echo " demo/slide"
search >$tmpdir/demo.slide.txt $grepopts $aminet demo/slide
echo " demo/sound"
search >$tmpdir/demo.sound.txt $grepopts $aminet demo/sound
echo " demo/tg93"
search >$tmpdir/demo.tg93.txt $grepopts $aminet demo/tg93
echo " dev/amos"
search >$tmpdir/dev.amos.txt $grepopts $aminet dev/amos
echo " dev/asm"
search >$tmpdir/dev.asm.txt $grepopts $aminet dev/asm
echo " dev/basic"
search >$tmpdir/dev.basic.txt $grepopts $aminet dev/basic
echo " dev/c"
search >$tmpdir/dev.c.txt $grepopts $aminet dev/c
echo " dev/cross"
search >$tmpdir/dev.cross.txt $grepopts $aminet dev/cross
echo " dev/debug"
search >$tmpdir/dev.debug.txt $grepopts $aminet dev/debug
echo " dev/e"
search >$tmpdir/dev.e.txt $grepopts $aminet dev/e
echo " dev/gcc"
search >$tmpdir/dev.gcc.txt $grepopts $aminet dev/gcc
echo " dev/gui"
search >$tmpdir/dev.gui.txt $grepopts $aminet dev/gui
echo " dev/lang"
search >$tmpdir/dev.lang.txt $grepopts $aminet dev/lang
echo " dev/m2"
search >$tmpdir/dev.m2.txt $grepopts $aminet dev/m2
echo " dev/misc"
search >$tmpdir/dev.misc.txt $grepopts $aminet dev/misc
echo " dev/moni"
search >$tmpdir/dev.moni.txt $grepopts $aminet dev/moni
echo " dev/obero"
search >$tmpdir/dev.obero.txt $grepopts $aminet dev/obero
echo " dev/src"
search >$tmpdir/dev.src.txt $grepopts $aminet dev/src
echo " disk/bakup"
search >$tmpdir/disk.bakup.txt $grepopts $aminet disk/bakup
echo " disk/cache"
search >$tmpdir/disk.cache.txt $grepopts $aminet disk/cache
echo " disk/cdrom"
search >$tmpdir/disk.cdrom.txt $grepopts $aminet disk/cdrom
echo " disk/misc"
search >$tmpdir/disk.misc.txt $grepopts $aminet disk/misc
echo " disk/moni"
search >$tmpdir/disk.moni.txt $grepopts $aminet disk/moni
echo " disk/optim"
search >$tmpdir/disk.optim.txt $grepopts $aminet disk/optim
echo " disk/salv"
search >$tmpdir/disk.salv.txt $grepopts $aminet disk/salv
echo " docs/anno"
search >$tmpdir/docs.anno.txt $grepopts $aminet docs/anno
echo " docs/help"
search >$tmpdir/docs.help.txt $grepopts $aminet docs/help
echo " docs/hyper"
search >$tmpdir/docs.hyper.txt $grepopts $aminet docs/hyper
echo " docs/lists"
search >$tmpdir/docs.lists.txt $grepopts $aminet docs/lists
echo " docs/mags"
search >$tmpdir/docs.mags.txt $grepopts $aminet docs/mags
echo " docs/misc"
search >$tmpdir/docs.misc.txt $grepopts $aminet docs/misc
echo " docs/rview"
search >$tmpdir/docs.rview.txt $grepopts $aminet docs/rview
echo " fish/docs"
search >$tmpdir/fish.docs.txt $grepopts $aminet fish/docs
echo " game/2play"
search >$tmpdir/game.2play.txt $grepopts $aminet game/2play
echo " game/board"
search >$tmpdir/game.board.txt $grepopts $aminet game/board
echo " game/demo"
search >$tmpdir/game.demo.txt $grepopts $aminet game/demo
echo " game/gag"
search >$tmpdir/game.gag.txt $grepopts $aminet game/gag
echo " game/hint"
search >$tmpdir/game.hint.txt $grepopts $aminet game/hint
echo " game/jump"
search >$tmpdir/game.jump.txt $grepopts $aminet game/jump
echo " game/misc"
search >$tmpdir/game.misc.txt $grepopts $aminet game/misc
echo " game/patch"
search >$tmpdir/game.patch.txt $grepopts $aminet game/patch
echo " game/role"
search >$tmpdir/game.role.txt $grepopts $aminet game/role
echo " game/shoot"
search >$tmpdir/game.shoot.txt $grepopts $aminet game/shoot
echo " game/think"
search >$tmpdir/game.think.txt $grepopts $aminet game/think
echo " game/wb"
search >$tmpdir/game.wb.txt $grepopts $aminet game/wb
echo " gfx/3d"
search >$tmpdir/gfx.3d.txt $grepopts $aminet gfx/3d
echo " gfx/3dobj"
search >$tmpdir/gfx.3dobj.txt $grepopts $aminet gfx/3dobj
echo " gfx/aga"
search >$tmpdir/gfx.aga.txt $grepopts $aminet gfx/aga
echo " gfx/anim"
search >$tmpdir/gfx.anim.txt $grepopts $aminet gfx/anim
echo " gfx/board"
search >$tmpdir/gfx.board.txt $grepopts $aminet gfx/board
echo " gfx/conv"
search >$tmpdir/gfx.conv.txt $grepopts $aminet gfx/conv
echo " gfx/edit"
search >$tmpdir/gfx.edit.txt $grepopts $aminet gfx/edit
echo " gfx/fract"
search >$tmpdir/gfx.fract.txt $grepopts $aminet gfx/fract
echo " gfx/misc"
search >$tmpdir/gfx.misc.txt $grepopts $aminet gfx/misc
echo " gfx/opal"
search >$tmpdir/gfx.opal.txt $grepopts $aminet gfx/opal
echo " gfx/pbm"
search >$tmpdir/gfx.pbm.txt $grepopts $aminet gfx/pbm
echo " gfx/show"
search >$tmpdir/gfx.show.txt $grepopts $aminet gfx/show
echo " gfx/x11"
search >$tmpdir/gfx.x11.txt $grepopts $aminet gfx/x11
echo " hard/drivr"
search >$tmpdir/hard.drivr.txt $grepopts $aminet hard/drivr
echo " hard/hack"
search >$tmpdir/hard.hack.txt $grepopts $aminet hard/hack
echo " hard/misc"
search >$tmpdir/hard.misc.txt $grepopts $aminet hard/misc
echo " misc/amag"
search >$tmpdir/misc.amag.txt $grepopts $aminet misc/amag
echo " misc/antiq"
search >$tmpdir/misc.antiq.txt $grepopts $aminet misc/antiq
echo " misc/edu"
search >$tmpdir/misc.edu.txt $grepopts $aminet misc/edu
echo " misc/emu"
search >$tmpdir/misc.emu.txt $grepopts $aminet misc/emu
echo " misc/math"
search >$tmpdir/misc.math.txt $grepopts $aminet misc/math
echo " misc/misc"
search >$tmpdir/misc.misc.txt $grepopts $aminet misc/misc
echo " misc/sci"
search >$tmpdir/misc.sci.txt $grepopts $aminet misc/sci
echo " misc/unix"
search >$tmpdir/misc.unix.txt $grepopts $aminet misc/unix
echo " mods/8voic"
search >$tmpdir/mods.8voic.txt $grepopts $aminet mods/8voic
echo " mods/atmos"
search >$tmpdir/mods.atmos.txt $grepopts $aminet mods/atmos
echo " mods/boing"
search >$tmpdir/mods.boing.txt $grepopts $aminet mods/boing
echo " mods/chart"
search >$tmpdir/mods.chart.txt $grepopts $aminet mods/chart
echo " mods/chip"
search >$tmpdir/mods.chip.txt $grepopts $aminet mods/chip
echo " mods/demo"
search >$tmpdir/mods.demo.txt $grepopts $aminet mods/demo
echo " mods/dream"
search >$tmpdir/mods.dream.txt $grepopts $aminet mods/dream
echo " mods/ephnx"
search >$tmpdir/mods.ephnx.txt $grepopts $aminet mods/ephnx
echo " mods/fant"
search >$tmpdir/mods.fant.txt $grepopts $aminet mods/fant
echo " mods/funet"
search >$tmpdir/mods.funet.txt $grepopts $aminet mods/funet
echo " mods/hje"
search >$tmpdir/mods.hje.txt $grepopts $aminet mods/hje
echo " mods/jazz"
search >$tmpdir/mods.jazz.txt $grepopts $aminet mods/jazz
echo " mods/jogei"
search >$tmpdir/mods.jogei.txt $grepopts $aminet mods/jogei
echo " mods/med"
search >$tmpdir/mods.med.txt $grepopts $aminet mods/med
echo " mods/misc"
search >$tmpdir/mods.misc.txt $grepopts $aminet mods/misc
echo " mods/otis"
search >$tmpdir/mods.otis.txt $grepopts $aminet mods/otis
echo " mods/piano"
search >$tmpdir/mods.piano.txt $grepopts $aminet mods/piano
echo " mods/pop"
search >$tmpdir/mods.pop.txt $grepopts $aminet mods/pop
echo " mods/pro"
search >$tmpdir/mods.pro.txt $grepopts $aminet mods/pro
echo " mods/rated"
search >$tmpdir/mods.rated.txt $grepopts $aminet mods/rated
echo " mods/rock"
search >$tmpdir/mods.rock.txt $grepopts $aminet mods/rock
echo " mods/sets"
search >$tmpdir/mods.sets.txt $grepopts $aminet mods/sets
echo " mods/sidew"
search >$tmpdir/mods.sidew.txt $grepopts $aminet mods/sidew
echo " mods/smpl"
search >$tmpdir/mods.sampl.txt $grepopts $aminet mods/smpl
echo " mods/spark"
search >$tmpdir/mods.spark.txt $grepopts $aminet mods/spark
echo " mods/techn"
search >$tmpdir/mods.techn.txt $grepopts $aminet mods/techn
echo " mods/u4ia"
search >$tmpdir/mods.u4ia.txt $grepopts $aminet mods/u4ia
echo " mods/voice"
search >$tmpdir/mods.voice.txt $grepopts $aminet mods/voice
echo " mus/edit"
search >$tmpdir/mus.edit.txt $grepopts $aminet mus/edit
echo " mus/midi"
search >$tmpdir/mus.midi.txt $grepopts $aminet mus/midi
echo " mus/misc"
search >$tmpdir/mus.misc.txt $grepopts $aminet mus/misc
echo " mus/play"
search >$tmpdir/mus.play.txt $grepopts $aminet mus/play
echo " os20/cdity"
search >$tmpdir/os20.cdity.txt $grepopts $aminet os20/cdity
echo " os20/util"
search >$tmpdir/os20.util.txt $grepopts $aminet os20/util
echo " os20/wb"
search >$tmpdir/os20.wb.txt $grepopts $aminet os20/wb
echo " os30/gfx"
search >$tmpdir/os30.gfx.txt $grepopts $aminet os30/gfx
echo " os30/util"
search >$tmpdir/os30.util.txt $grepopts $aminet os30/util
echo " os30/wb"
search >$tmpdir/os30.wb.txt $grepopts $aminet os30/wb
echo " pix/anim"
search >$tmpdir/pix.anim.txt $grepopts $aminet pix/anim
echo " pix/bill"
search >$tmpdir/pix.bill.txt $grepopts $aminet pix/bill
echo " pix/eric"
search >$tmpdir/pix.eric.txt $grepopts $aminet pix/eric
echo " pix/guard"
search >$tmpdir/pix.guard.txt $grepopts $aminet pix/guard
echo " pix/icon"
search >$tmpdir/pix.icon.txt $grepopts $aminet pix/icon
echo " pix/illu"
search >$tmpdir/pix.illu.txt $grepopts $aminet pix/illu
echo " pix/imagi"
search >$tmpdir/pix.imagi.txt $grepopts $aminet pix/imagi
echo " pix/irc"
search >$tmpdir/pix.irc.txt $grepopts $aminet pix/irc
echo " pix/misc"
search >$tmpdir/pix.misc.txt $grepopts $aminet pix/misc
echo " pix/real3"
search >$tmpdir/pix.real3.txt $grepopts $aminet pix/real3
echo " pix/trace"
search >$tmpdir/pix.trace.txt $grepopts $aminet pix/trace
echo " pix/wb"
search >$tmpdir/pix.wb.txt $grepopts $aminet pix/wb
echo " text/anno"
search >$tmpdir/text.anno.txt $grepopts $aminet text/anno
echo " text/dtp"
search >$tmpdir/text.dtp.txt $grepopts $aminet text/dtp
echo " text/edit"
search >$tmpdir/text.edit.txt $grepopts $aminet text/edit
echo " text/font"
search >$tmpdir/text.font.txt $grepopts $aminet text/font
echo " text/hyper"
search >$tmpdir/text.hyper.txt $grepopts $aminet text/hyper
echo " text/misc"
search >$tmpdir/text.misc.txt $grepopts $aminet text/misc
echo " text/print"
search >$tmpdir/text.print.txt $grepopts $aminet text/print
echo " text/show"
search >$tmpdir/text.show.txt $grepopts $aminet text/show
echo " text/tex"
search >$tmpdir/text.tex.txt $grepopts $aminet text/tex
echo " util/app"
search >$tmpdir/util.app.txt $grepopts $aminet util/app
echo " util/arc"
search >$tmpdir/util.arc.txt $grepopts $aminet util/arc
echo " util/batch"
search >$tmpdir/util.batch.txt $grepopts $aminet util/batch
echo " util/blank"
search >$tmpdir/util.blank.txt $grepopts $aminet util/blank
echo " util/boot"
search >$tmpdir/util.boot.txt $grepopts $aminet util/boot
echo " util/cdity"
search >$tmpdir/util.cdity.txt $grepopts $aminet util/cdity
echo " util/cli"
search >$tmpdir/util.cli.txt $grepopts $aminet util/cli
echo " util/conv"
search >$tmpdir/util.conv.txt $grepopts $aminet util/conv
echo " util/crypt"
search >$tmpdir/util.crypt.txt $grepopts $aminet util/crypt
echo " util/dir"
search >$tmpdir/util.dir.txt $grepopts $aminet util/dir
echo " util/dtype"
search >$tmpdir/util.dtype.txt $grepopts $aminet util/dtype
echo " util/gnu"
search >$tmpdir/util.gnu.txt $grepopts $aminet util/gnu
echo " util/libs"
search >$tmpdir/util.libs.txt $grepopts $aminet util/libs
echo " util/misc"
search >$tmpdir/util.misc.txt $grepopts $aminet util/misc
echo " util/moni"
search >$tmpdir/util.moni.txt $grepopts $aminet util/moni
echo " util/mouse"
search >$tmpdir/util.mouse.txt $grepopts $aminet util/mouse
echo " util/pack"
search >$tmpdir/util.pack.txt $grepopts $aminet util/pack
echo " util/rexx"
search >$tmpdir/util.rexx.txt $grepopts $aminet util/rexx
echo " util/shell"
search >$tmpdir/util.shell.txt $grepopts $aminet util/shell
echo " util/time"
search >$tmpdir/util.time.txt $grepopts $aminet util/time
echo " util/virus"
search >$tmpdir/util.virus.txt $grepopts $aminet util/virus
echo " util/wb"
search >$tmpdir/util.wb.txt $grepopts $aminet util/wb

# Now we're finished, we can tidy up by copying the new files across &
# deleting the old monolithic one...

if -n -f $destdir; mkdir $destdir; endif
echo "Copying files across..."
copy $tmpdir/*.txt $destdir
delete -r $tmpdir
echo -n "About to delete "$aminet"! Ok? : "
input vroom
if $vroom = "y";delete $aminet;endif
unset aminet
unset grepopts
unset destdir
unset tmpdir
echo "Finished! And I *really* don't want to know how long that took! :)"
