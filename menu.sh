# This installs an intuition menu with all editing keys

menu -nm Move \
 "Left         CursL",\
 "Right        CursR",\
 "WordLeft   S-CursL",\
 "WordRight  S-CursR",\
 "BegOfLine       ^A",^a\
 "EndOfLine       ^E",^e

menu  Delete \
 "Left         BkSpc",^h\
 "Right          Del",\177\
 "WordLeft        ^W",^[^h\
 "WordRight  ESC-Del",^[\177\
 "To BOL          ^B",^[x^h\
 "To EOL          ^K",^[x\177\
 "Line            ^X",^[d

menu History \
 "Back         CursU",\
 "Forward      CursD",\
 "Start    ESC-CursU",\
 "End      ESC-CursD",\
 "Complete   S-CursU",^[!\
 "Get tail        ^T",^T

menu Complete \
 "One            TAB",^I\
 "Partial      S-TAB",\
 "All        ESC-TAB",^[^I\
 "Show            ^D",^D\
 "QuickCD      ESC-c",^[c\
 "LastCD       ESC-~",^[~

menu Execute \
 "Now         RETURN",^M\
 "+Hist   ESC-RETURN",^[^M\
 "Not             ^N",^N\
 "Exit            ^\\",

menu Misc \
 "Undo            ^U",^U\
 "Repeat          ^R",^R\
 "Retype          ^L",^L\
 "Ins/Ovr      ESC-i",^[i\
 "PrevWord        ^P",^P\
 "SwapChars    ESC-s",^[s

echo "Intuition menus installed."
