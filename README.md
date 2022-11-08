[Make sure to check out the change_log.txt file to see the most
 current work being done!]

As of January 10th, 2022, ScarletDME can be built as a fully native 64 bit
application!  More testing needs to be done, but the 64 bit build appears
to function identically to the 32 bit build.

The 64-bit version is the default for the dev branch, but there is a
qm32 target to build the 32-bit version on the dev branch.

These packages are needed to build ScarletDME as a 32 bit application,
for most Linux distributions:
libgcc.i686
glibc-devel.i686

For proper terminal operation, you'll need to install:
ncurses-devel
ncurses-compat-libs

If you would like to support remote access to your ScarletDME system via
the QMClient API or telnet services, you'll need to install xinetd, or
debug the systemd .socket implementation. See the README.md in the
xinetd.d directory for instructions on the further use of xinetd.

ScarletDME requires a user named "qmsys" and a group named "qmusers".
These will be created for you automatically by the "install" target. But
make sure you add the qmuser group to any user that will be using
ScarletDME.

You should be able to build the system by just typing "make' in the
directory where the Makefile lives. Enter "sudo make install" to install
the result of the first "make" command.

This does not activate ScarletDME - "sudo make qmdev" will start the
server for you as a one-off, you need to do that every boot. Or "sudo
make systemd" will activate the systemd service files so ScarletDME will
start on boot. If you don't run systemd, please modify the makefile to
detect and configure your init system.

If you need to re-install the master system directory, run "sudo make
datafiles", but this should normally never be done, as it is done for
you on initial install, and overwriting the files will destroy your live
system status.

The system has adopted Sphinx for documentation - install Sphinx then
"make docs" to build the html documentation locally. This will create
the document root as docs/build/html/index. To build other formats (pdf,
epub) read the Sphinx documentation, then run the Sphinx makefile from
inside the docs directory. (Make sure you get the right Sphinx - www.sphinx-doc.org)

Code formatting notes:
I'm using Visual Studio code with the Microsoft C/C++ IntelliSense, debugging,
and code browsing extension installed.  

Each time I need to edit a code file, it's reformatted using the clang-format
feature in the extension.  The .clang-format file in this repository is based
upon the Chromium format, but it will not reflow comments, nor will it
sort includes.  The settings can be found in ScarletDME/.clang-format.

Some files have been reformatted, most have not.  However, eventually they all
will be.  There's also instances of K&R-style function parameter declarations
and those will be converted to ANSI-style as I discover them.

There's a mailing list available at https://groups.google.com/g/scarletdme. 
Both developers and regular users are welcome!

[27Feb20] A Discord server is now available (basically a tarted up version of IRC) at
          https://discord.gg/H7MPapC2hK - if this link doesn't work for you,
          please reach out to me (geneb@deltasoft.com) and I'll get you a working
          link.  They do age out periodically. [24Jan22 - the Discord link should
          now be permanent.]

[26Feb20 gwb]

I resumed work on this project because I had an itch I just had to scratch.
That's how most open source software is done apparently. :)

I should note that at the moment, the contents of this git repo builds and
works for *me*.  I'm not guaranteeing it'll work for anyone else. :)

I'm not sure what the future holds for this project, but I would like to get 
a clean, working build of a 64 bit ScarletDME.  I had originally thought that
this was very difficult due to the pre-compiled p-code that the system depended
on.  However, after actually doing some research into what was going on, it 
may be a much less dramatic task than I'd originally thought.  My original 
assumption about the p-code issue was *wildly* incorrect.  That happens when
you just glance at an issue without actually digging into it.

This means that a 64 bit version of ScarletDME isn't that far fetched after
all.  Time well tell I suppose.  I know what needs to be done and how to do 
it, so at least I've got that going for me. :)

I would also like to finish the "re-branding" of OpenQM to ScarletDME.
While some things will forever be "OpenQM-isms" like the names of the binaries,
there are other areas that just need to be changed in order to make the
re-branding effort complete.

I've got a 66 page document that's all of the release notes I could find for
the commercial releases of OpenQM.  That's going to act as a starting point
for improvements and/or bug fixes.  I think.  We'll see.

I'm going to end this document by thanking Martin Phillips for the original
GPL release of OpenQM.  We've not always seen eye to eye on things, but his
contributions to the Multi-Value database industry cannot be understated.
I will always appreciate the gift he's given us all and the value that OpenQM
represents to the Multi-Value database community - regardless of whether or not
they realize it. ;)

-Gene Buckle, February 26th, 2020.
