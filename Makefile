######################### -*- Mode: Makefile-Gmake -*- ########################
## Copyright (C) 2024, Mats Bergstrom
## 
## File name       : Makefile
## Description     : for normp
## 
## Author          : Mats Bergstrom
## Created On      : Sun Mar 10 14:15:21 2024
## 
## Last Modified By: Mats Bergstrom
## Last Modified On: Thu Mar 14 19:04:12 2024
## Update Count    : 5
###############################################################################


CC		= gcc
CFLAGS		= -Wall -pedantic-errors -g
CPPFLAGS	= -Icfgf
LDLIBS		= -lmosquitto -lcfgf
LDFLAGS		= -Lcfgf

BINDIR		= /usr/local/bin
ETCDIR		= /usr/local/etc
SYSTEMD_DIR 	= /lib/systemd/system

BINARIES 	= normp
SYSTEMD_FILES 	= normp.service
ETC_FILES 	= normp.cfg

CFGFGIT		= https://github.com/mats-bergstrom/cfgf.git


all: cfgf normp

normp: normp.o

normp.o: normp.c



.PHONY: cfgf clean uninstall install

cfgf:
	if [ ! -d cfgf ] ; then git clone $(CFGFGIT) ; fi
	cd cfgf && make

really-clean:
	if [ -d cfgf ] ; then rm -rf cfgf ; fi

clean:
	rm -f *.o normp *~ *.log .*~
	if [ -d cfgf ] ; then cd cfgf && make clean ; fi

uninstall:
	cd $(SYSTEMD_DIR); rm $(SYSTEMD_FILES)
	cd $(BINDIR); rm $(BINARIES)
	cd $(ETCDIR); rm $(ETC_FILES)

install:
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR); fi
	if [ ! -d $(ETCDIR) ] ; then mkdir $(ETCDIR); fi
	cp $(BINARIES) $(BINDIR)
	cp $(ETC_FILES) $(ETCDIR)
	cp $(SYSTEMD_FILES) $(SYSTEMD_DIR)
