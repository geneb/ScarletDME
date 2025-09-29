# ScarletDME

A multivalue database management system built from OpenQM 2.6-6.
Thank you to Martin Phillips for the original GPL release of OpenQM.

## Building from Source

Building ScarletDME requires gcc and make.

```
git clone https://github.com/geneb/ScarletDME.git
cd ScarletDME
make
sudo make install
```

The installation will create the qmusers group and a qmsys user, which is required for ScarletDME to work.

To update a specific user to use ScarletDME, you will need to add them to the
qmusers group:

```
sudo usermod -aG qmusers username
```

Note that you'll need to add your account to the qmusers account before you
can use ScarletDME.  If you do this and get a $LOGIN error, you can just log
out of your Linux terminal session and log back in so that the current shell 
picks up your new group assignment.

You can start ScarletDME by issuing one of the following commands:

(non-systemd method)
```
sudo qm -start
```
(systemd method)
```
sudo systemctl start scarletdme
```
Note that if you would like ScarletDME to automatically start up on system boot,
you'll also need to issue the command:
```
sudo systemctl enable scarletdme
```

## Community

The community for multivalue is small and so here are some places specific
to ScarletDME. 

[ScarletDME Discord](https://discord.gg/H7MPapC2hK)

[ScarletDME Google Group](https://groups.google.com/g/scarletdme/)

There are also general multivalue communites online.

[Pick Google Group](https://groups.google.com/g/mvdbms)

[OpenQM Google Group](https://groups.google.com/g/openqm)

[Rocket Forums](https://community.rocketsoftware.com/forums/multivalue)

# For Developers

If you would like to run the most current "development" edition of ScarletDME, you'll need to get a copy of the 'dev' branch:
```
git checkout dev
```
This working branch will have the most up to date changes, but may not be strictly speaking, "stable".  If you favor stability
over bleeding edge work, please stick with the 'master' branch.

ScarletDME now favors 64 bit platforms.  The 32 bit code has been
"retired" to the "master32" and "Release32" branches.  No pull requests will
be accepted for the 32 bit branches.

# Client Access
Client access via the QMClient API or telnet can be handled via systemd.
The default 'make install' process will install the included systemd scripts
to allow ScarletDME client services to run.  If you don't use systemd, see the 
README.md in the xinetd.d directory for instructions on the further use of xinetd.

# Build/Install Notes
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

# Documentation
The system has adopted Sphinx for documentation - install Sphinx then
"make docs" to build the html documentation locally. This will create
the document root as docs/build/html/index. To build other formats (pdf,
epub) read the Sphinx documentation, then run the Sphinx makefile from
inside the docs directory. (Make sure you get the right Sphinx - www.sphinx-doc.org)

Got a pull request for us?  GREAT!  However, all pull requests must be tied to 
a git Issue # in order to help keep things a bit better documented and much
more organized!  Make sure your commit message includes the Issue # you created.

# Code Formatting
We mostly use Visual Studio code with the Microsoft C/C++ IntelliSense, debugging,
and code browsing extension installed.

Each time I need to edit a code file, it's reformatted using the clang-format
feature in the extension.  The .clang-format file in this repository is based
upon the Chromium format, but it will not reflow comments, nor will it
sort includes.  The settings can be found in ScarletDME/.clang-format.

Some files have been reformatted, most have not.  However, eventually they all
will be.

There's a mailing list available at https://groups.google.com/g/scarletdme. 
Both developers and regular users are welcome!

# Future Work

I would like to finish the "re-branding" of OpenQM to ScarletDME.
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

Many thanks to @Krowemoh for providing a fantastic starting point for updating this README!
