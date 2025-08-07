# Changelog

All notable changes to CShell will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [5.60] - Public Release

### Added
- $STACK: cookie for NDK3.2
- Revised version number to 5.60
- Added ObjArc contribution

### Fixed
- Build failure caused by missing changes in 5.50a (see the next item)
- Revised smakefile to utilise SAS/C 6.58 features
- Changed location of .cshrc to SDK:S/ for ToolKit compatibility
- Revised vers command output

## [5.50a] - Public Release

### Added
- New built-in variable: `_warn`. Bit-field that enables/disables the various "warning levels" of Cshell when executing programs (e.g., unreleased signals, priority changes, wrong nesting count for Enable/Disable)

### Fixed
- Corrected "demo.sh". Didn't work due to some changes in previous versions.

## [5.50] - Public Release

### Added
- Cshell now warns if an executed program changed the priority of the current task or the signal state shows unreleased signals.

## [5.49] - Internal Beta Release

### Fixed
- Fixed minor bug when files for redirection were given but no command to execute (doesn't make any sense, but could happen due to typos)
- Made fast successive calls to built-in function `@rnd(0)` more accurate, seed is more "random" now

### Added
- New built-in function: `@mktemp`. Returns a unique temporary file name of form `T:tmpXXXXXXXX`, or just `tmpXXXXXXXX` if T: does not exist. *(Thanks to Gary Duncan for source code)*
- Built-in function `@rnd`, when called with seed=0 to build seed value from current time-stamp, now always builds unique seed value on every call
- Built-in command now handles binary files properly, so it's possible to use "cat file1 file2 >file3" to join files like under UNIX. But note that the built-in command "join" is much faster

### Changed
- Minor enhancements

## [5.48] - Public Release

### Added
- Built-in command "version" now behaves more like Commodore's "version" command. Four new options. See manual for details

### Fixed
- Fixed bug in built-in command "dir", custom output format printed date *and* time for "%d" instead of only the date ("%t" is for time). This bug was a side effect of the locale kludge in Cshell 5.44
- Fixed compatibility problem with the new redirection (introduced in Cshell 5.46) that caused problems with some handlers, e.g. NULL:, which cannot handle MODE_READWRITE but only MODE_NEWFILE. Cshell now tries MODE_READWRITE first and falls back to MODE_NEWFILE if R/W fails

## [5.47] - Public Release

### Changed
- Replaced "diskchange" with old code from last public release (only of importance for beta testers)

## [5.46] - Internal Beta Release

### Fixed
- Rewrote built-in function "diskchange" because of some handlers that have problems with Inhibit() and ACTION_INHIBIT
- Made "echo mem | csh" work again

### Added
- Redirected output to a file can now be examined by other programs while Cshell is still filling up the file (files for redirection are now opened "shared" instead of "exclusively") *(Thanks to Gary Duncan for source code)*
- Built-in command "copy" now has (optional) new output format which prints full path of files being copied. Use option "-x" to try it out *(Thanks to Gary Duncan for source code)*

### Changed
- Some minor cosmetic changes

## [5.45] - Internal Beta Release

### Added
- Built-in command "window" resizes/moves window much faster now
- Supports "muSetProtection" if MultiUser is installed
- New option "-t" for built-in command "sleep": wait ticks

### Fixed
- Fixed fatal bug if non-interactive input line was too long
- Fixed bug in built-in commands "chown" and "chgrp" which could crash under OS 2.x
- Built-in commands "chown" and "chgrp" now accept also user and group names (as well as numerical IDs). Does only work with "MultiUser" *(Thanks to Carsten Pluntke <Carsten_Pluntke@ouzonix.bo.open.de>)*
- Updated built-in error message texts to AmigaOS 3.1

## [5.44] - Not Released

### Fixed
- Skipped 5.43 because of unauthorized version of Cshell on AmiNet by Martin Berndt <m_berndt@wanderer.gun.de>
- Date/time strings now left-justified (right-justified before), this fixes some problems with localized date strings
- Improved code for built-in command "window" to minimize move operations (the "hopping" windows effect)

### Changed
- Added extensive disclaimer to documentation
- Changed "smakefile" slightly
- Changed output format of built-in command "mem" again

## [5.42] - Public Release

### Added
- New built-in variable `_promptdep` (prompt path depth) and new placeholder "%P" for built-in variable `_prompt`. Like "%p", "%P" displays the current path. With `_promptdep` the user sets the maximum number of directories (path parts) displayed for %P (to keep prompt smart and short). Default is 3
- New built-in variable `_complete` (DOS pattern). What files should match on filename completion. Default is "*". For example, if you don't want to see files ending with ".bak", set it to "~(*.bak)"

### Fixed
- Fixed timer bug in `_prompt` (%e) if start and end time of a program were not at the same day (wrong execution time was displayed)

### Changed
- Minor changes in documentation
- Built-in command "window" doesn't clear screen anymore. If you want to clear screen, use "echo -n ^[c^O" or "echo -n ^L"

## [5.41] - Internal Beta Release

### Fixed
- Fixed various bugs in built-in command "window". The changes for 5.40 caused an unnecessary usage message when using options -f, -b and -a. And options -l and -s didn't work at all. The checking for maximum window dimensions are not hardcoded anymore but dynamically adjusted to the screen dimensions
- Finally found (and fixed) rounding bug in "itok()", caused display of wrong size for memory or harddisk around 1 GB and 1 TB etc. Numbers were rounded down to "0 GB" and "0 TB" instead of rounded up to "1 GB" and "1 TB" *(Thanks to Andreas 'Leguan' Geist)*

### Changed
- Changed output format of built-in command "mem" (for option -r) a little bit
- Changed shortcut for "Kilobytes" from "KB" (or "K") to "kB" (or "k"). You may like it or not - but only this way it's consistent. They say one should use "K" instead of "k" because it has something to do with 1024 and not 1000. But then, why don't we use "m" instead of "M" and "g" instead of "G"?

## [5.40] - Public Release

### Added
- New placeholders "-1" and "-2" for built-in command "window"
- Increased maximum value for window dimensions from 1023 to 32767 for built-in command "window"
- Environment variables LINES and COLUMNS override window bounds from Amiga console.device (if env vars are set, no CSI sequence is sent)
- Built-in command "mem" now shows size of largest available memory block

### Fixed
- Fixed error message for built-in command "copy": if no special error message was available, always the string "(no mem)" was output

## [5.39] - Public Release

### Added
- New built-in variable `_timeout` (in microseconds) sets maximum response time for terminal to answer WINDOW STATUS REQUEST (for window bounds). Defaults to 1 (for local usage), must be set to higher value for remote connections. Only used if window pointer is not available
- Not only "*" and "?" but also "[" and "]" recognized as AmigaDOS pattern (that means, to use "[" and "]" you must quote (") or escape (\) them!)
- New flag for command abbreviation (`$_abbrev`): 8, search DOS path-list if command wasn't found in Cshell's internal program hash list (see "rehash" command)

### Fixed
- Removed command line length limitation (140 chars) for ARexx scripts that ends with ".rexx" but are started without the trailing ".rexx"
- Removed command line length limitation (518 chars) for ARexx scripts and external shells (#! in first line), this was a limitation in AmigaOS' System() function. DOS scripts still have this limitation, because you cannot RunCommand() "execute"
- Much more workarounds for serious bugs in DateToStr() and Locale
- Fixed bug: making an assign to an executable and calling the executable by its assign crashed machine
- Fixed bug: built-in command "cp" sometimes used already freed memory for generating error messages (resulted in some strange error messages)

## [5.38] - Internal Beta Release

### Added
- CTRL-D now shows matching files if current word is not a directory (if directory then shows contents of directory -- as usual). In its current implementation this may have unexpected side effects if current word is already a pattern
- Class definition for AmigaE in class.sh *(Thanks to Joseph E. Van_Riper III)*
- New control-code for line-editing: "^V" (ctrl-v) quotes next char
- New built-in variable `_kick` holds version number of Operating System
- New built-in variable `_mappath` (see manual), enables pathname-mapping for commands if script starts with "#!" or ";!" in first line. Converts Unix paths like "/usr/..." to "usr:..."
- New option "-w" for Cshell, don't use window pointer (useful for KingCON)
- New option "-V" for Cshell, send only VT100 compatible control sequences

### Fixed
- Now internal timer (e.g., %e in the titlebar) not set to zero when a null command is encountered (same for return code, %x in titlebar)
- Built-in command "assign" now prints volume name if assign points to an unmounted volume (e.g., a removed floppy disk) and doesn't pop up a requester "Please replace volume ..."
- It was a stupid idea to force redirecting of all Cshell-related system requesters to CSH's screen, because requester windows inherit the window title of their "initiator". They appear now again on your default public screen
- Fixed serious bug (crashed machine) with redirection and launching programs into background (files closed twice). Known bug: it's still not possible to run pipes into background...
- When running programs into background (run, rback, &), internal commands and aliases are recognized and executed with "csh -c". Aliases WON'T be resolved on this level so they must be declared in .cshrc to run them into background
- Execution of Rexx-Scripts (without trailing ".rexx") and any other program with "#! my_prog" or ";! my_prog" in first line of script now possible also from DOS search path and not only `$_path`
- Internal variable "o_vt100" now used (if option -t or -V is set), don't send control sequences that are not VT100 compatible (e.g., special Amiga control sequences)

### Changed
- The idea of always using the variable `_dirformat` for "dir" wasn't a good idea. So, `_dirformat` is only used if option -z is given (when `_dirformat` is unset then use first argument as format string)

## [5.37] - Public Release

### Added
- Built-in command "chmod" now supports "a" for all bits (ugo), and if no ownership-bit is specified "chmod" really modifies only the user-bits (as stated in the doc) and not all bits
- Built-in commands "chown/chgrp" now also run under AmigaOS 2.0+ (direct DOS packets), not only under AmigaOS 3.0+ (SetOwner) *(Thanks to Carsten Heyl)*
- Built-in command "dir" now uses contents of variable `_dirformat` (if set) automatically as default output format (option -z not necessary for `_dirformat` anymore). Option -z (followed by an argument with format string) can now be used again for another custom format for current call as in Cshell 5.19 and before *(Thanks to Laurent Faillie for his letter)*
- New option "-c" for built-in command "qsort": be case-sensitive

### Fixed
- Built-in command "mkdir" now strips off trailing slash (if any)
- No requester "please insert volume ..." when you press return and your current directory is on an "unmounted" volume (e.g., a removed floppy disk)
- Built-in command "protect" was broken in beta-version 5.36
- Source code now "indent" clean (some warnings on first run, but no errors)

## [5.36] - Internal Beta Release

### Added
- Now @rnd() accepts optional seed parameter
- Now @age() returns null-string if file was not found (instead of 99999) *(Thanks to Gary Duncan)*
- New function @age_mins() returns age of file in minutes *(Thanks to Gary Duncan)*
- Variable `_abbrev` now handled somewhat different: it enables/disables the different modes of command-abbreviation. See manual for details!
- Added class "gzip" to csh:class.sh
- Added support for MultiUser. %U in prompt/titlebar shows current user of CSH. Out-comment "#define MULTIUSER_SUPPORT 1" in shell.h to compile without MultiUser stuff *(Thanks to Magnus Lilja)*
- New built-in commands "chown" and "chgrp"
- Built-in command "chmod" now supports User/Group/Other bits
- New placeholders for custom output format (option "-z"), "%F" shows group/other bits, %U shows user-id, %G shows group-id

### Fixed
- Now output in "path -r" only if at least 1 memory block could not be freed
- Fixed layout bug in "rm -r <wildcard>" if one or more items could not be removed
- Fixed serious bug in "rehash", access to already freed memory under certain circumstances (always think twice before using "char ***" ;-)
- Manual entry for built-in command "relabel"
- Error checking code for all NameFromLock() calls
- Now 100% Commodore-compatible version string
- Fixed bogus output for NULL pointers in pattern-matching-routine
- Fixed bug in "rehash", recognition of executables was broken
- Now multiple drive names are allowed for "diskchange"
- Now requesters appear on same screen as CSH's window
- Output of "dir -k" and "dir -i" (show classes) looks much nicer now

## [5.35] - Public Release

### Added
- Now CSH executes "s:.logout" on exit. New option "-L" (noLogout) disables sourcing of "s:.logout"
- Built-in command "window -q" (query) shows public screen names
- New/Changed options for built-in command "strings" (not compatible with previous versions!)
- More sophisticated memory-cleanup for built-in command "path -r" (reset)
- New option "-M" for CSH, don't clear menus (especially for KingCON)
- Support for soft-links in built-in command "dir" *(Thanks to Carsten Heyl)*

### Fixed
- Fixed option "-n" in built-in command "dir", never worked before
- Some minor layout changes in built-in command "help"

## [5.34] - Internal Beta Release

### Added
- New section SCROLLING in the manual (at the end). Read it!
- New variable `_prghash` holds filename where program hash list is loaded from and saved to
- Now CSH also finds programs in resident list if they start with "C:" (even if there is no corresponding program in C: directory). This is for compatibility reasons (Commodore Shell)
- Totally new meaning of variable `_nomatch`. It's now unset by default and if you set it then CSH will *not* abort if all patterns does not match
- Added option "-m" to CSH, sets `_nomatch` variable (for compatibility reasons)
- New option -a for "copy" command, don't clear archive bit *(Gary Duncan)*
- Rewritten code for "window -q", no longer crashes system
- New option "-w" for built-in command "window" (ignore window width for option -q, so that very long window/screen titles are printed completely)
- Input/output redirection with "<>", the file must be either "NIL:" or an interactive file
- New variable `_cquote` enables Commodore-style handling of quotation marks (e.g., for use with ReadArgs); variable is unset by default (UNIX-style handling of quotes)
- Increased some (very small) buffers in "ls" code (could crash machine), line limit is now around 1000 chars per line. Had to increase internal stack size to 17500

### Fixed
- In previous versions CSH aborted the commandline if one pattern did not match. Now CSH only aborts if all patterns does not match
- Fixed bug in date command (s/r options) *(Gary Duncan)*

## [5.33] - Internal Beta Release

### Added
- New built-in command "rehash", buffers programs in DOS search path and offers program name completion (ESC-p, ESC-P). Two new "completion functions" for keymaps:
  - 36, CompPrg1 - Insert first matching program (or cycle)
  - 37, CompPrgAll - Shows all matching programs
- Kludge for running DOS scripts from devices with spaces in their name
- Built-in command "date" can now read battery-backed up clock directly *(Thanks to Gary Duncan)*

### Fixed
- Fixed input-stream problem for DOS scripts
- Fixed Enforcer hit in "set" command (e.g., "set a ="), just curious: this Enforcer hit was misinterpreted by an user to be a bug in `_every` variable and accidentally I found another bug there which caused CSH to ignore `_every` if the previous shell command was aborted (^C)
- Fixed Enforcer hit: "unset _path"

## [5.32] - Internal Beta Release

### Added
- New "edit function" for keymaps: 29, insert last word of previous line (e.g. "keymap 0 12=29" assigns that function to CTRL+L)
- Checking of requested stack size (built-in command "stack")
- Built-in command "mem" now calls AllocMem(0x7fffffff,0) ten times to flush unneeded memory (similar to "avail flush")

### Fixed
- Removed "No match" output if "dir" was used on empty directories. This was a feature, not a bug. But misunderstood by most users
- Option "-q" of built-in command "rm/delete" didn't work in previous versions. Now aborts as documented. This does *not* affect non-matching wildcards, use `_nomatch` instead
- Fixed possible rounding bug in "itok()", caused display of wrong size for harddisk partitions around 1 GB etc.

## [5.31] - The Essential

### Added
- ARP free
- OS 2.0+ only
- Slightly changed pattern matching (pattern.library, dos.library)
- Better support for DOS' command search path and resident list
- Supports "PROGDIR:"
- Supports AmigaOS-scripts
- Unix-like automagic execution of ".login" and ".cshrc"
- More built-in commands support ^C (ctrl-c)
- More reliable support of WILDSTAR-Flag
- Enhanced commands (new options, bug fixes, changed behaviour):
  - dir, path, info, rename, delete, assign, touch, ps, mkdir, addbuffers, cd, stack, help, strings, menu, resident, head, tail, set
- New commands:
  - ln/makelink, chmod
- New (or modified) functions:
  - @stricmp, @filedate, @filenote, @hextodec, @confirm, @ask
- New variables:
  - _clipri, _dirformat, _nomatch
- New prompt variable(s)
- New "edit function" for keymaps
- Detects if current directory was changed by other programs
- Uses ASL file-requester
- Arguments in scripts passed via $0, $1, $2 etc., number of arguments in $#
- Enhanced quoting mechanism for filename completion
- Lots of internal changes
- Lots of other bug fixes
- And lots of changes and new features I forgot to write down ;-)


