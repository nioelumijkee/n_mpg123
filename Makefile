################################################################################
lib.name = n_mpg123
cflags = 
ldlibs = -lmpg123
class.sources = n_mpg123.c
sources = \
./include/*
datafiles = \
n_mpg123-help.pd \
n_mpg123-meta.pd \
README.md \
LICENSE.txt

################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
