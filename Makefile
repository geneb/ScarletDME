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

MAIN     := ./
GPLSRC   := $(MAIN)gplsrc/
GPLDOTSRC := $(MAIN)gpl.src
GPLOBJ   := $(MAIN)gplobj/
GPLBIN   := $(MAIN)bin/
TERMINFO := $(MAIN)terminfo/
VPATH    := $(GPLOBJ):$(GPLBIN):$(GPLSRC)

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
qm: C_FLAGS  := -Wall -Wformat=2 -Wno-format-nonliteral -DLINUX -D_FILE_OFFSET_BITS=64 -I$(GPLSRC) -DGPL -g $(ARCH)
qm: $(QMOBJS) qmclilib.so qmtic qmfix qmconv qmidx qmlnxd terminfo
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(ARCH) $(L_FLAGS) $(QMOBJSD) -o $(GPLBIN)qm

qm32: ARCH := -m32
qm32: BITSIZE := 32
qm32: C_FLAGS  := -Wall -Wformat=2 -Wno-format-nonliteral -DLINUX -D_FILE_OFFSET_BITS=64 -I$(GPLSRC) -DGPL -g $(ARCH)
qm32: $(QMOBJS) qmclilib.so qmtic qmfix qmconv qmidx qmlnxd terminfo
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(ARCH) $(L_FLAGS) $(QMOBJSD) -o $(GPLBIN)qm

qmclilib.so: qmclilib.o
	@echo Linking $@
	@$(COMP) -shared -Wl,$(SONAME_OPT),qmclilib.so -lc $(ARCH) $(GPLOBJ)qmclilib.o -o $(GPLBIN)qmclilib.so
	@$(COMP) -shared -Wl,$(SONAME_OPT),libqmcli.so -lc $(ARCH) $(GPLOBJ)qmclilib.o -o $(GPLBIN)libqmcli.so


qmtic: qmtic.o inipath.o
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmtic.o $(GPLOBJ)inipath.o -o $(GPLBIN)qmtic

qmfix: qmfix.o ctype.o linuxlb.o dh_hash.o inipath.o
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmfix.o $(GPLOBJ)ctype.o $(GPLOBJ)linuxlb.o $(GPLOBJ)dh_hash.o $(GPLOBJ)inipath.o -o $(GPLBIN)qmfix

qmconv: qmconv.o ctype.o linuxlb.o dh_hash.o
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmconv.o $(GPLOBJ)ctype.o $(GPLOBJ)linuxlb.o $(GPLOBJ)dh_hash.o -o $(GPLBIN)qmconv

qmidx: qmidx.o
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmidx.o -o $(GPLBIN)qmidx

qmlnxd: qmlnxd.o qmsem.o
	@echo Linking $@
	@cd $(GPLOBJ)
	@$(COMP) $(C_FLAGS) -lc $(GPLOBJ)qmlnxd.o $(GPLOBJ)qmsem.o -o $(GPLBIN)qmlnxd

terminfo:
	@echo Compiling terminfo library
	@cd $(GPLSRC)
	@mkdir $(TERMINFO)
	@$(GPLBIN)qmtic -pterminfo $(MAIN)terminfo.src

qmclilib.o: qmclilib.c revstamp.h
	@echo Compiling $@ with -fPIC
	@$(COMP) $(C_FLAGS) -fPIC -c $(GPLSRC)qmclilib.c -o $(GPLOBJ)qmclilib.o

# We need to make sure that anything that includes revstamp.h gets built if revstamp.h 
# changes.

config.o: config.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)config.o

kernel.o: kernel.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)kernel.o

op_kernel.o: op_kernel.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)op_kernel.o

op_sys.o: op_sys.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)op_sys.o

pdump.o: pdump.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)pdump.o

qm.o:	qm.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qm.o 

qmclient.o: qmclient.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmclient.o

qmconv.o: qmconv.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmconv.o

qmfix.o: qmfix.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmfix.o

qmidx.o: qmidx.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmidx.o

qmtic.o: qmtic.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)qmtic.o

sysdump.o: sysdump.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)sysdump.o

sysseg.o: sysseg.c revstamp.h
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)sysseg.o

.c.o:
	@echo Compiling $@, $(BITSIZE) bit target.
	@$(COMP) $(C_FLAGS) -c $< -o $(GPLOBJ)$@

.PHONY: clean distclean install datafiles

install:  

ifeq ($(QMUSERS),)
	@echo Creating qm system user and group
	@groupadd --system qmusers
	@usermod -a -G qmusers root
	ifeq ($(QMSYS),)
		@useradd --system qmsys --gid qmusers
	endif
endif

	@echo Installing to $(INSTROOT)
	@test -d $(INSTROOT) || mkdir $(INSTROOT)
	@test -d $(INSTROOT)/bin || mkdir $(INSTROOT)/bin
	@for qm_prog in $(GPLBIN)*; do \
	  install -m 775 -o qmsys -g qmusers $$qm_prog $(INSTROOT)/bin; \
	done

	@echo Writing scarlet.conf file
	@cp $(main)scarlet.conf /etc/scarlet.conf
	@chmod 644 /etc/scarlet.conf

#	Create symbolic link if it does not exist
	@test -f /usr/bin/qm || ln -s /usr/qmsys/bin/qm /usr/bin/qm

#	Install systemd configuration file if needed.
	@echo Installing scarletdme.service for systemd.
	@test -d /etc/systemd/system && \
			cp etc/systemd/system/scarletdme.service /etc/systemd/system
	@chown root.root /etc/systemd/system/scarletdme.service
	@chmod 755 /etc/systemd/system/scarletdme.service


datafiles:
	@echo Installing data files...
	@cp -r $(MAIN)\$$COMO/ $(INSTROOT)/\$$COMO
	@chmod 775 $(INSTROOT)/\$$COMO/*
	@cp -r $(MAIN)\$$FORMS/ $(INSTROOT)/\$$FORMS
	@chmod 775 $(INSTROOT)/\$$FORMS/*
	@cp -r $(MAIN)\$$HOLD.DIC/ $(INSTROOT)/\$$HOLD.DIC
	@cp -r $(MAIN)\$$HOLD/ $(INSTROOT)/\$$HOLD
	@chmod 665 $(INSTROOT)/\$$HOLD.DIC/*
	@cp -r $(MAIN)\$$IPC/ $(INSTROOT)/\$$IPC
	@chmod 665 $(INSTROOT)/\$$IPC/*
	@cp -r $(MAIN)\$$LOGINS/ $(INSTROOT)/\$$LOGINS
	@chmod 665 $(INSTROOT)/\$$LOGINS/*
	@cp -r $(MAIN)\$$MAP/ $(INSTROOT)/\$$MAP
	@chmod 665 $(INSTROOT)/\$$MAP/*
	@cp -r $(MAIN)\$$MAP.DIC/ $(INSTROOT)/\$$MAP.DIC
	@chmod 665 $(INSTROOT)/\$$MAP.DIC/*
	@cp -r $(MAIN)\$$SCREENS/ $(INSTROOT)/\$$SCREENS
	@chmod 665 $(INSTROOT)/\$$SCREENS/*
	@cp -r $(MAIN)\$$SCREENS.DIC/ $(INSTROOT)/\$$SCREENS.DIC
	@chmod 665 $(INSTROOT)/\$$SCREENS.DIC/*
	@cp -r $(MAIN)\$$SVLISTS/ $(INSTROOT)/\$$SVLISTS
#
	@cp -r $(MAIN)ACCOUNTS/ $(INSTROOT)/ACCOUNTS
	@chmod 665 $(INSTROOT)/ACCOUNTS/*
	@cp -r $(MAIN)ACCOUNTS.DIC/ $(INSTROOT)/ACCOUNTS.DIC
	@chmod 665 $(INSTROOT)/ACCOUNTS.DIC/*
	@cp -r $(MAIN)BP/ $(INSTROOT)/BP
	@chmod 665 $(INSTROOT)/BP/*
	@cp -r $(MAIN)BP.OUT/ $(INSTROOT)/BP.OUT
	@chmod 665 $(INSTROOT)/BP.OUT/*
	@cp -r $(MAIN)cat/ $(INSTROOT)/cat
	@cp -r $(MAIN)DICT.DIC/ $(INSTROOT)/DICT.DIC
	@chmod 665 $(INSTROOT)/DICT.DIC/*
	@cp -r $(MAIN)DIR_DICT/ $(INSTROOT)/DIR_DICT
	@chmod 665 $(INSTROOT)/DIR_DICT/*
	@cp -r $(MAIN)ERRMSG/ $(INSTROOT)/ERRMSG
	@chmod 665 $(INSTROOT)/ERRMSG/*
	@cp -r $(MAIN)ERRMSG.DIC/ $(INSTROOT)/ERRMSG.DIC
	@chmod 665 $(INSTROOT)/ERRMSG.DIC/*
	@cp -r $(MAIN)gcat/ $(INSTROOT)/gcat
	@chmod 665 $(INSTROOT)/gcat/*
	@cp -r $(MAIN)GPL.BP/ $(INSTROOT)/GPL.BP
	@chmod 665 $(INSTROOT)/GPL.BP/*	
	@cp -r $(MAIN)GPL.BP.OUT/ $(INSTROOT)/GPL.BP.OUT
	@chmod 665 $(INSTROOT)/GPL.BP.OUT/*	
	@cp -r $(MAIN)MESSAGES/ $(INSTROOT)/MESSAGES
	@chmod 665 $(INSTROOT)/MESSAGES/*	
	@cp -r $(MAIN)NEWVOC/ $(INSTROOT)/NEWVOC
	@chmod 665 $(INSTROOT)/NEWVOC/*	
	@cp -r $(MAIN)prt/ $(INSTROOT)/prt
	@cp -r $(MAIN)QM.VOCLIB/ $(INSTROOT)/QM.VOCLIB
	@chmod 665 $(INSTROOT)/QM.VOCLIB/*	
	@cp -r $(MAIN)SYSCOM/ $(INSTROOT)/SYSCOM
	@chmod 665 $(INSTROOT)/SYSCOM/*
	@cp -r $(MAIN)terminfo/ $(INSTROOT)/terminfo
	@cp -r $(MAIN)TEST/ $(INSTROOT)/TEST
	@cp -r $(MAIN)tools/ $(INSTROOT)/tools
	@cp -r $(MAIN)VOC/ $(INSTROOT)/VOC
	@chmod 665 $(INSTROOT)/VOC/*
	@cp -r $(MAIN)VOC.DIC/ $(INSTROOT)/VOC.DIC
	@chmod 665 $(INSTROOT)/VOC.DIC/*
	@chown -R qmsys.qmusers $(INSTROOT)
	@chmod 775 $(INSTROOT)
	@chmod 775 $(INSTROOT)/*
	@echo Data file copy completed!
clean:
	@$(RM) $(GPLOBJ)*.o
#	@$(RM) $(GPLOBJ)*.so
# no .so files are currently dropped into GPLOBJ...

distclean: clean
	@$(RM) $(GPLOBJ)*.o
	@$(RM) $(GPLBIN)*
	@$(RM) $(GPLSRC)terminfo

# Additional stop and start for developers, wraps qm build
# Unfortunatly this requires root/sudo access -dat
qmdev: qmstop qm
	@echo Attempting to start server
	sudo bin/qm -start
qmstop:
	@echo Attempting to stop server
	sudo qm -stop

