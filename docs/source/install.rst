Download and Installation
=========================

Obtaining ScarletDME
--------------------

Download the source tree from github with

git clone git://github.com/geneb/ScarletDME.git

Building and installing
-----------------------

As at 16 Jan 2022 the master branch contains the 32-bit version, and the
dev branch contains the 64-bit version.

Type "make" to build ScarletDME. This will build the default target
"qm". The dev branch is 64-bit by default, use the target qm32 for a 32-
bit build.

The following makefile targets are available

*default:* creates a 64-bit build on the dev branch, 32-bit on the
master

**qm32:** creates a 32-bit build on the dev branch

**install:** does an awful lot.

Firstly if they don't exist, it creates the group "qmusers" and adds
root to it. Then it creates the user qmsys and adds that too.

Next if it doesn't find a master qmsys account it copies the template.

After that, it copies/updates all the executables in qmsys/bin and then
links them to /bin. It also updates NEWVOC.

If it detects systemd, it copies the systemd service files.

If it detects xindetd, it copies the xinetd files and updates the
/etc/services file.

**datafiles:** copies all the files required for the master system
account to /usr/qmsys. This should normally never be used, it's done for
you in the install target, and invoking it will trash your live state.

**clean:** deletes many of the files created by the make, leaving a
clean setup that will recompile from scratch if required. You MUST do
this if switching between 32-bit and 64-bit builds.

**distclean:** is a more aggressive version of clean, deleting all? the
files created by the make.

**docs:** If Sphinx is installed, it will build the html docs. See the
Sphinx section for more details. To create anything other than html you
will have to explicitly build it in the docs directory.

**qmdev:** **qmstop:** invoke sudo to start and stop the ScarletDME
service.

**systemd:** Activates the service/socket files so that ScarletDME will
run on boot.

systemd
-------

Set up ScarletDME to start every boot with "systemctl enable
scarletdme.service". To start it manually or for the first time you can
do "systemctl start scarletdme.service"

Adding users
------------

In order for users to be able to use ScarletDME properly they need to be
added to the qmusers group, although this won't cause serious problems
if this is forgotten. The main problem will be if they create an account
it won't be added to the master account list. They may also not have
access to certain system FILEs that are symlinked from their account.
This will be flagged up with "error 3018".

You also need an ACCOUNT, the equivalent of a SQL database, where your
data is stored. While you could create an account in an existing
system directory, that is not wise. Create a directory specifically for
your account, cd into it, and start scarletdme with "qm". It will prompt
to confirm you want to make an ACCOUNT, then copy all the relevant files
over before leaving you at TCL (The Colon Line).

Just remember that while SQL databases conceptually reside in the DBMS,
ScarletDME ACCOUNTs conceptually reside in the filesystem.

Sphinx
------

The documentation is currently being prepped in Sphinx.
https://www.sphinx-doc.org/en/master/

If you don't know your way around Python, don't follow the Sphinx installation
instructions, just use your distro package manager. Otherwise you are likely to
get into a mess with pip and venv.

Once the docs have been built, the root html document is 
docs/build/html/index.html

Open this in your browser to see the result.

All Sphinx documents are .rst (reStructured Text) files. It's fairly simple,
a form of markup. Just read the Sphinx documentation, and look at how the
ScarletDME docs have been made. All source files go in docs/source, and 
the Makefile will copy stuff around as required.
