# CShell

This is CShell, a csh-like command shell for Amiga.

CShell has been modernized and integrated into ToolKit from amigazen project, to provide developers with a Unix-like command shell for both interactive operations using familiar commands such as ls, rm and mkdir, and an environment that can execute csh style command scripts.

## [amigazen project](http://www.amigazen.com)

*A web, suddenly*

*Forty years meditation*

*Minds awaken, free*

**amigazen project** uses modern software development tools and methods to update and rerelease classic Amiga open source software. Our upcoming releases include a new AWeb, and a new Amiga Python 2.

Key to our approach is ensuring every project can be built with the same common set of development tools and configurations, so we created the ToolKit project to provide a standard configuration for Amiga development. All *amigazen project* releases will be guaranteed to build against the ToolKit standard so that anyone can download and begin contributing straightaway without having to tailor the toolchain for their own setup.

The original authors of the *CShell* software are not affiliated with the amigazen project. This software is redistributed on terms described in the documentation, particularly the files COPYING or LICENSE.md

Our philosophy is based on openness:

*Open* to anyone and everyone	- *Open* source and free for all	- *Open* your mind and create!

PRs for all of our projects are gratefully received at [GitHub](https://github.com/amigazen/). While our focus now is on classic 68k software, we do intend that all amigazen project releases can be ported to other Amiga-like systems including AROS and MorphOS where feasible.

All amigazen projects have been updated:
- To build against the latest NDK3.2
- Using the latest good version of at least one standard Amiga compiler - either SAS/C (6.58), DICE, VBCC or GCC
- And will build straight out of the box, for anyone who has ToolKit setup correctly

## About ToolKit

**ToolKit** exists to solve the problem that most Amiga software was written in the 1980s and 90s, by individuals working alone, each with their own preferred setup for where their dev tools are run from, where their include files, static libs and other toolchain artifacts could be found, which versions they used and which custom modifications they made. Open source collaboration did not exist then as we know it now. 

**ToolKit** from amigazen project is a work in progress to make a standardised installation of not just the Native Developer Kit, but the compilers, build tools and third party components needed to be able to consistently build projects in collaboration with others, without each contributor having to change build files to work with their particular toolchain configuration. 

All *amigazen project* releases will release in a ready to build configuration according to the ToolKit standard.

Each component of **ToolKit** is open source and like *csh* here will have it's own github repo, while ToolKit itself will eventually be released as an easy to install package containing the redistributable components, as well as scripts to easily install the parts that are not freely redistributable from archive.

## Requirements

- Amiga or Amiga-compatible computer with latest operating system software
- SAS/C 6.58 setup according to ToolKit standard
- NDK3.2

## Installation

- copy SDK/C/csh to SDK:C/ this being the standard location for ToolKit commands
- make sure SDK: exists as an assign to your ToolKit files root, and that SDK:C/ is in your path
- the ToolKit sdk-startup script will do this for you if you have it installed
- csh can also be used standalone, in which case simply place the *csh* binary somewhere in your path

## Changelog

See (CHANGELOG.md)

## Contact 

- At GitHub https://github.com/amigazen/CShell
- on the web at http://www.amigazen.com/toolkit/ (Amiga browser compatible)
- or email toolkit@amigazen.com

## [Aminet.readme](https://www.aminet.net/package/util/shell/csh550)

***N.B. this readme contents dates to 1996! contact details may no longer be relevant!***

DESCRIPTION

     C-Shell is a replacement for the AmigaDOS command line interface.
     Many builtin Unix-like commands, very fast script language, file-
     name completion, command-name completion, comfortable command line
     editing, pattern matching, AUX: mode, object oriented file classes,
     abbreviation of internal and external commands.  Supports multiple
     users.

     C-Shell is easy to install and to use.  Online help for all
     commands, functions and various subjects.  ARP-free!

NEW FEATURES

     Changes since version 5.48 (summary):
      - Warns if an executed program changed the priority of the
         current task or the signal state shows unreleased signals.
      - Fast successive calls to built-in function @rnd(0) more accurate,
         seed is more "random" now.
      - Built-in function @rnd, when called with seed=0 to build seed
         value from current time-stamp, now always builds unique seed
         value on every call.
      - New built-in function: @mktemp. Returns a unique temporary file
         name of form T:tmpXXXXXXXX, or just tmpXXXXXXXX if T: does not
         exist. [thanks to Gary Duncan for source code]
      - Built-in command now handles binary files properly, so it's
         possible to use "cat file1 file2 >file3" to join files like
         under UNIX.  But note, that the built-in command "join" is
         much more faster.
      - Some minor enhancements and bug fixes.

     Changes since version 5.42 (summary):
      - Redirected output to a file can now be examined by other programs
         while Cshell is still filling up the file. (files for redirection
         are now opened "shared" instead of "exclusivly")
      - Enhanced pipe functionality. Made "echo mem | csh" work again.
      - Built-in command "copy" now has (optional) new output format which
         prints full path of files being copied. (just try option "-x")
      - Built-in commands "chown" and "chgrp" now accept also user and group
         names (as well as numerical IDs). Does only work with "MultiUser".
      - Built-in command "window" resizes/moves window much more faster now.
      - Built-in command "version" now behaves more like Commodore's
         command "version".

     See file "HISTORY" in archive csh550.lha for complete listing
     of changes and new features.

SPECIAL REQUIREMENTS

     AmigaOS 2.0 (or higher)

## Acknowledgements

*Amiga* is a trademark of **Amiga Inc**. 