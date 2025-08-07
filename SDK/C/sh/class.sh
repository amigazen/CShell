if $_version < 516
 echo "Csh 5.16 required"
 return 0
endif

#
# If started from ToolManager: Set the path where all these tools reside.
# Enter your search path here. You might want to use the 'path' command,
# but 'set _path' does not complain about nonexisting assigns.
#

if $_passed = toolmanager
 set _path csh:,s:,sys:utilities,sys:system,tools:
endif

#
# The file class definitions
#

class -n
class xpk       offs=0,58504b46
class dms       offs=0,444D5321
class warp      offs=0,57617270
class zoom      offs=0,5A4F4F4D
class lharc     offs=2,2D6C68..2D
class zoo       offs=0,5A4F4F20
class arc       offs=0,1a08
class compress  offs=0,1f9d
class anim      offs=0,464f524d........414e494d
class iff_8svx  offs=0,464f524d........38535658
class sculpt_sc offs=0,464f524d........53433344
class sculpt_ta offs=0,464f524d........54414b45
class imagine   offs=0,464f524d........54444444
class icon      offs=0,e3100001 offs=0,f34c0012
class gif       offs=0,474946
class zip       offs=0,504b0304
class gzip      offs=0,1f8b0808
class ppacked   offs=0,50503230
class imploded  offs=0,494d5021
class dimp      offs=0,44494d50
class manx_dbg  offs=0,35d200
class manx_lib  offs=0,636a
class manx_obj  offs=0,434a0000
class object    offs=0,000003e70000
class sas_lib   offs=0,000003fa0000
class font      offs=0,0f0000
class ps_doc    offs=0,072319
class ps_font   offs=0,236900 offs=0,2520434f50595249
class compufont offs=0,00440001 offs=00016e offs=0000001400000d
class compucach offs=0,3c466f6e74204361
class jpeg      offs=6,4a464946
class guide     offs=0,4064617461626173 offs=0,4044415441424153

# Windows Audio Visual (WAV) one-shot sound files
class wav offs=0,52494646........57415645

# MED/OctaMED music
class med offs=0,4D4D4430 offs=0,4D4D4431

#
# Additional class definitions plus action definitions.
# Change lines after 'act' to reflect your personal installation.
#

class lharc     suff=.lha
class lharc     suff=.lzh act exec="LhA e" extr="LhA e" view="LhA v" add="LhA a" edit=lharca
class dms       suff=.dms act exec="Dms write" extr="Dms write" view="Dms view"
class warp      suff=.wrp act exec="Warp write 0 79" extr="Warp write 0 79"
class zoom      suff=.zom act exec="Zoom" extr="Zoom"
class zoo       suff=.zoo act exec="zoo e//" extr="zoo e//" view="zoo v" add="zoo a"
class arc       suff=.arc act exec="arc e" extr="arc e" view="arc -v"
class zip       suff=.zip act exec="unzip" extr="unzip" view="unzip l"
class gzip      suff=.z   act exec="gunzip" extr="gunzip" view="gzip -l"
class ppacked   suff=.pp  act exec="ppmore" view=ppmore
class imploded  suff=.im  act exec="fimp"  extr="fimp"
class lhwarp    suff=.lhw act exec="lhwarp write 0"
class uuencoded suff=.uu  act exec="uudecode" extr="uudecode"
class modula    suff=.mod suff=.ref suff=.sym suff=.obj
class anim      suff=.anim act exec=showanim view=showanim
class gl_anim   suff=.gl   act exec="gl -g" view="gl -g"
class gif       suff=.gif  act exec=turbogif  view=turbogif extr=shamsharp
class jpeg      suff=.jpg
class ftm       offs=0,46544d4e act exec="PlayFTM >nil:" view="PlayFTM >nil:"
class ilbm      offs=0,464F524D........494C424D act exec=M view=M edit=dpaint
class text      offs=0,464F524D........46545854 act edit=excellence
class prog      offs=0,000003f300000000
class module    name="mod.*" act view="Experiment\\ IV" exec="Experiment\\ IV"
class object    suff=.o
class include   suff=.h  is=ascii
class c_source  suff=.c  is=ascii
class script    suff=.sh is=ascii
class guide     suff=.guide suff=.hyper is=ascii act view=amigaguide exec=amigaguide
class ascii     suff=.doc suff=.txt chars act view=muchmore exec=more edit=ced
class "?"       default action view=more edit=ced

# Class definitions for Amiga E...

alias compe %n EC " @strhead( \\. $n )"
class e_source  suff=.e is=ascii act exec=compe

# (The 'compe' alias is needed because E will not compile the program if you
# add the '.e' to the end of the filename.  Sort of an irritating twist,
# really).

alias v    "%n action view $n"
alias ed   "%n action edit $n"
alias xt   "action extr"

