MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = dtmfcmd

dtmfcmd_SOURCES = \
	dtmfcmd.c \
	dsp.c \
	dsp.h \
	control.c \
	control.h \
	sound_info.h \
	sound_info.c \
	general_functions.c \
	general_functions.h


dtmfcmd_CFLAGS = \
	-Wall -Wextra -pedantic \
        -DDATADIR=\"$(datadir)\" \
        -DPKGDATADIR=\"$(pkgdatadir)\" \
        @GLIB_CFLAGS@ 

dtmfcmd_LDFLAGS = \
        -ldl -lm\
        @GLIB_LIBS@ 
