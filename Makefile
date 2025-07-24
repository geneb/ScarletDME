# ScarletDME Makefile
#
#   You are allowed to modify this file, so long as the derived file can
#   be distributed under the same license as this file.  You are allowed to copy
#   and distribute verbatim copies of this document so long as you follow
#   the licensing agreements contained within this document.
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 3 of the License, or (at your option)
#   any later version.
#   This program is distributed in the hope that it will be useful, but WITHOUT
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
#   more details.
#   You should have received a copy of the GNU General Public License along with
#   this program; if not, write to the Free Software Foundation, Inc.,
#   59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# Note - for some reason this makefile had the entire text of the GPLv3 license
#        attached at the end. I've removed it as part of doing updates to this
#        Makefile.  The GPL v3 License can be found here: 
#        https://www.gnu.org/licenses/gpl-3.0.txt
#
# Changelog
# ---------
# 05Nov22 awy Fix systemd install for unified /usr
# 13Mar22 awy Update install target to create master account if required,
#             and update NEWVOC
#             $COMO is created on first use so remove it from master account.
# 16Jan22 awy Adding qm32 target to build 32-bit if required. BUILD64 now obsolete.
# 15Jan22 awy Adding code to create qmsys and qmuser if they don't exist.
#             Adding scarletdme.service file to install target
#
# 12Jan22 gwb Fixed a typo that resulted in the $IPC directory being a mess and 
#             not having the dynamic files it needed.
#             Cleaned up the datafiles installation process to make sure the things
#             that needed to get copied were.
#             Ensured correct permissions were being correctly set for the all the 
#             files within the subdirectories of the installation location.
#             Incorrect permissions were causing issues with creating an account on
#             the very first login to the system.
#
# 09Jan22 gwb Added the ability to more easily select a 32 vs 64 bit build target.
#             Setting BUILD64 := Y will result in a 64 bit build.
#             There was a bug in the command that creates the symbolic link to
#             the qm binary.  "@ln" was invalid, it should be "ln".
#
# 09Jan22 gwb Added $COMO directory to the list of things to be copied on install.
#             Added the $FORMS directory to the list of things to be copied.
#             Fixed dumb typo in a Makefile comment.
# 20Feb20 gwb Added -m32 $(ARCH) flag to ensure we're compiling with 32 bit 
#             libraries.
#             I also inverted the timeline of these comments in order to make
#             the format similar to the change logs in the source code files.
# 22Jun12 gwb Added individual source files to trigger recompile if revstamp.h
#			  has been changed.
# 20Jun12 gwb Added -Wl,--no-as-needed flags in order to enable proper linking
#             under Ubuntu 12.04 LTS
#             Added install target - copies binaries to $(INSTROOT)/bin
#             Added datafiles target - copies all the system data files to
#             $(INSTROOT) - note that this will overwrite an existing 
#             installation!!!
# 29Aug09 dat Added optional qmdev target wraps qm target, but stops and starts
#             server
# 31Nov08 dat Amended to function with 2-6-6 -Diccon Tesson
# =====================================================================================
# Maintainer List
# ---------------
# dat - Diccon Tesson
# gwb - Gene Buckle (geneb@deltasoft.com)
# awy - Anthony (Wol) Youngman

# default target builds 64-bit, build qm32 target for a 32-bit build

# MAIN     := ./
MAIN     := $(shell pwd)/
GPLSRC   := $(MAIN)gplsrc/
GPLDOTSRC := $(MAIN)gpl.src
GPLOBJ   := $(MAIN)gplobj/
GPLBIN   := $(MAIN)bin/
TERMINFO := $(MAIN)qmsys/terminfo/
VPATH    := $(GPLOBJ):$(GPLBIN):$(GPLSRC)

ifneq ($(wildcard /usr/lib/systemd/system/.),)
	SYSTEMDPATH := /usr/lib/systemd/system
else
	SYSTEMDPATH := /lib/systemd/system
endif

OSNAME   := $(shell uname -s)

COMP     := gcc

ifeq (Darwin,$(OSNAME))
	ARCH :=
	BITSIZE := 64
	C_FLAGS  := -Wall -Wformat=2 -Wno-format-nonliteral -DLINUX -D_FILE_OFFSET_BITS=64 -I$(GPLSRC) -DGPL -g $(ARCH)
	L_FLAGS  := -lm -ldl
	INSTROOT := /opt/qmsys
	SONAME_OPT := -install_name
else
	L_FLAGS  := -Wl,--no-as-needed -lm -lcrypt -ldl
	INSTROOT := /usr/qmsys
	SONAME_OPT := -soname
endif

# The -Wno-format-nonliteral flag prevents the compiler warning us about being unable to check the format
# strings the system uses for error message text.
# C_FLAGS  := -Wall -Wformat=2 -Wno-format-nonliteral -DLINUX -D_FILE_OFFSET_BITS=64 -I$(GPLSRC) -DGPL -g $(ARCH)


RM       := rm

QMHDRS   := $(wildcard *.h)
QMSRCS   := $(shell cat $(GPLDOTSRC))
QMTEMP   := $(addsuffix .o,$(QMSRCS))
QMOBJS   := $(QMTEMP)
QMOBJSD  := $(addprefix $(GPLOBJ),$(QMTEMP))
TEMPSRCS := $(wildcard *.c)
SRCS     := $(TEMPSRCS:qmclient.c=)
OBJS     := $(SRCS:.c=.o)
DIROBJS  := $(addprefix $(GPLOBJ),$(OBJS))
QMSYS   := $(shell cat /etc/passwd | grep qmsys)
QMUSERS := $(shell cat /etc/group | grep qmusers)

qm: ARCH :=
qm: BITSIZE := 64
qm: C_FLAGS  := -Wall -Wformat=2 -Wno-format-nonliteral -DLINUX -D_FILE_OFFSET_BITS=64 -I$(GPLSRC) -DGPL -g $(ARCH) -fPIE
qm: $(QMOBJS) qmclilib.so qmtic qmfix qmconv qmidx qmlnxd
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(ARCH) $(L_FLAGS) $(QMOBJSD) -o $(GPLBIN)qm

qm32: ARCH := -m32
qm32: BITSIZE := 32
qm32: C_FLAGS  := -Wall -Wformat=2 -Wno-format-nonliteral -DLINUX -D_FILE_OFFSET_BITS=64 -I$(GPLSRC) -DGPL -g $(ARCH)
qm32: $(QMOBJS) qmclilib.so qmtic qmfix qmconv qmidx qmlnxd
	@echo Linking $@
	@$(COMP) $(ARCH) $(L_FLAGS) $(QMOBJSD) -o $(GPLBIN)qm

qmclilib.so: qmclilib.o
	@echo Linking $@
	@$(COMP) -shared -Wl,$(SONAME_OPT),qmclilib.so -lc $(ARCH) $(GPLOBJ)qmclilib.o -o $(GPLBIN)qmclilib.so
	@$(COMP) -shared -Wl,$(SONAME_OPT),libqmcli.so -lc $(ARCH) $(GPLOBJ)qmclilib.o -o $(GPLBIN)libqmcli.so

qmtic: qmtic.o inipath.o
	@echo Linking $@
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmtic.o $(GPLOBJ)inipath.o -o $(GPLBIN)qmtic

qmfix: qmfix.o ctype.o linuxlb.o dh_hash.o inipath.o
	@echo Linking $@
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmfix.o $(GPLOBJ)ctype.o $(GPLOBJ)linuxlb.o $(GPLOBJ)dh_hash.o $(GPLOBJ)inipath.o -o $(GPLBIN)qmfix

qmconv: qmconv.o ctype.o linuxlb.o dh_hash.o
	@echo Linking $@
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmconv.o $(GPLOBJ)ctype.o $(GPLOBJ)linuxlb.o $(GPLOBJ)dh_hash.o -o $(GPLBIN)qmconv

qmidx: qmidx.o
	@echo Linking $@
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmidx.o -o $(GPLBIN)qmidx

qmlnxd: qmlnxd.o qmsem.o
	@echo Linking $@
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmlnxd.o $(GPLOBJ)qmsem.o -o $(GPLBIN)qmlnxd

qmclilib.o: qmclilib.c revstamp.h
	@echo Compiling $@ with -fPIC
	@$(COMP) $(C_FLAGS) -fPIC -c $(GPLSRC)qmclilib.c -o $(GPLOBJ)qmclilib.o

# We need to make sure that anything that includes revstamp.h gets built if revstamp.h 
# changes.

config.o: config.c config.h qm.h revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)config.o

kernel.o: kernel.c qm.h revstamp.h header.h tio.h debug.h keys.h syscom.h config.h \
	options.h dh_int.h locks.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)kernel.o

op_kernel.o: op_kernel.c qm.h revstamp.h header.h tio.h debug.h keys.h syscom.h \
	config.h options.h dh_int.h locks.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)op_kernel.o

op_sys.o: op_sys.c qm.h header.h tio.h syscom.h dh_int.h revstamp.h config.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)op_sys.o

pdump.o: pdump.c qm.h header.h syscom.h config.h revstamp.h locks.h dh_int.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)pdump.o

qm.o:	qm.c qm.h revstamp.h header.h debug.h dh_int.h tio.h config.h options.h \
	locks.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qm.o 

qmclient.o: qmclient.c qmdefs.h revstamp.h qmclient.h err.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmclient.o

qmconv.o: qmconv.c qm.h dh_int.h header.h revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmconv.o

qmfix.o: qmfix.c qm.h dh_int.h revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmfix.o

qmidx.o: qmidx.c qm.h dh_int.h revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmidx.o

qmtic.o: qmtic.c ti_names.h revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmtic.o

sysdump.o: sysdump.c qm.h locks.h revstamp.h config.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)sysdump.o

sysseg.o: sysseg.c qm.h locks.h config.h revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)sysseg.o

.c.o:
	@echo Compiling $@, $(BITSIZE) bit target.
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)$@

.PHONY: clean distclean install datafiles docs systemd qmdev qmstop

install:  

ifeq ($(QMUSERS),)
	@echo Creating qm system user and group
	@groupadd --system qmusers
	@usermod -a -G qmusers root
ifeq ($(QMSYS),)
	@useradd --system qmsys --gid qmusers
endif
endif

	@echo Compiling terminfo library
	@test -d qmsys/terminfo && rm -Rf qmsys/terminfo
	@mkdir qmsys/terminfo
	cd qmsys && $(GPLBIN)qmtic -pterminfo $(MAIN)terminfo.src

	@echo Installing to $(INSTROOT)
ifeq ($(wildcard $(INSTROOT)/.),)
#	qmsys doesn't exist, so copy it to the live location
	cp -R qmsys $(INSTROOT)
	chown -R qmsys:qmusers $(INSTROOT)
	chmod -R 664 $(INSTROOT)
	find $(INSTROOT) -type d -print0 | xargs -0 chmod 775
else
#	else update everything that's changed, eg NEWVOC, MESSAGES, all that sort of stuff.

#	copy the contents of NEWVOC so the account will upgrade
	@rm $(INSTROOT)/NEWVOC/*
	@cp qmsys/NEWVOC/* $(INSTROOT)/NEWVOC
	@chown qmsys:qmusers $(INSTROOT)/NEWVOC/*
	@chmod 664 $(INSTROOT)/NEWVOC/*

#	copy the contents of MESSAGES so the account will upgrade
	@rm $(INSTROOT)/MESSAGES/*
	@cp qmsys/MESSAGES/* $(INSTROOT)/MESSAGES
	@chown qmsys:qmusers $(INSTROOT)/MESSAGES/*
	@chmod 664 $(INSTROOT)/MESSAGES/*

#	copy the contents of terminfo so the account will upgrade
	@rm -Rf $(INSTROOT)/terminfo/*
	@cp -R qmsys/terminfo/* $(INSTROOT)/terminfo

	@chown -R qmsys:qmusers $(INSTROOT)/terminfo
	@find $(INSTROOT)/terminfo -type d -print0 | xargs -0 chmod 775
	@find $(INSTROOT)/terminfo -type f -print0 | xargs -0 chmod 664

#	@chown qmsys:qmusers $(INSTROOT)/terminfo/*
#	@chmod 664 $(INSTROOT)/terminfo/*
#	make sure all directories are readable
#	find $(INSTROOT) -type d -print0 | xargs -0 chmod 775


endif
#       copy bin files and make them executable
	@test -d $(INSTROOT)/bin || mkdir $(INSTROOT)/bin
#	copy the contents of bin so the account will upgrade - -f so it works on an empty directory
	@rm -f $(INSTROOT)/bin/*
	@cp bin/* $(INSTROOT)/bin
	chown qmsys:qmusers $(INSTROOT)/bin $(INSTROOT)/bin/*
	chmod 775 $(INSTROOT)/bin $(INSTROOT)/bin/*

	@echo Writing scarlet.conf file
	@cp $(main)scarlet.conf /etc/scarlet.conf
	@chmod 644 /etc/scarlet.conf

#	Create symbolic link if it does not exist
	@test -f /usr/bin/qm || ln -s /usr/qmsys/bin/qm /usr/bin/qm

#	Install systemd configuration file if needed.
ifneq ($(wildcard $(SYSTEMDPATH)/.),)
	@echo Installing scarletdme.service for systemd.
	@cp usr/lib/systemd/system/* $(SYSTEMDPATH)
	@chown root:root $(SYSTEMDPATH)/scarletdme.service
	@chown root:root $(SYSTEMDPATH)/scarletdmeclient.socket
	@chown root:root $(SYSTEMDPATH)/scarletdmeclient@.service
	@chown root:root $(SYSTEMDPATH)/scarletdmeserver.socket
	@chown root:root $(SYSTEMDPATH)/scarletdmeserver@.service
	@chmod 644 $(SYSTEMDPATH)/scarletdme.service
	@chmod 644 $(SYSTEMDPATH)/scarletdmeclient.socket
	@chmod 644 $(SYSTEMDPATH)/scarletdmeclient@.service
	@chmod 644 $(SYSTEMDPATH)/scarletdmeserver.socket
	@chmod 644 $(SYSTEMDPATH)/scarletdmeserver@.service
endif

#	Install xinetd files if required
ifneq ($(wildcard /etc/xinetd.d/.),)
	@echo Installing xinetd files
	@cp etc/xinetd.d/qmclient /etc/xinetd.d
	@cp etc/xinetd.d/qmserver /etc/xinetd.d
ifneq ($(wildcard /etc/services),)
ifeq ($(shell cat /etc/services | grep qmclient),)
	@cat etc/xinetd.d/services >> /etc/services
endif
endif
endif

systemd:
	@systemctl enable scarletdme.service
	@systemctl enable scarletdmeclient.socket
	@systemctl enable scarletdmeserver.socket

datafiles:
# you should never need this target ...
ifeq ($(wildcard $(INSTROOT)/.),)
	cp -R qmsys $(INSTROOT)
else
	cp -R qmsys/* $(INSTROOT)
endif
	chown -R qmsys:qmusers $(INSTROOT)
	chmod -R 664 $(INSTROOT)
	find $(INSTROOT) -type d -print0 | xargs -0 chmod 775

	@echo Data file copy completed!
clean:
	@$(RM) $(GPLOBJ)*.o
#	@$(RM) $(GPLOBJ)*.so
# no .so files are currently dropped into GPLOBJ...

distclean: clean
	@$(RM) $(GPLOBJ)*.o
	@$(RM) $(GPLBIN)*
	@$(RM) $(GPLSRC)terminfo

docs:
	$(MAKE) -C docs html

# Additional stop and start for developers, wraps qm build
qmdev: qmstop qm
	@echo Attempting to start server
	qm -start
qmstop:
	@echo Attempting to stop server
	qm -stop

