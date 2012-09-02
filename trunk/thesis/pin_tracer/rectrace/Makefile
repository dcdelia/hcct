##
## This is a sample makefile for building Pin tools outside
## of the Pin environment.  This makefile is suitable for
## building with the Pin kit, not a Pin source development tree.
##
## To build the tool, execute the make command:
##
##      make
## or
##      make PIN_HOME=<top-level directory where Pin was installed>
##
## After building your tool, you would invoke Pin like this:
## 
##      $PIN_HOME/pin -t MyPinTool -- /bin/ls
##
##############################################################
#
# User-specific configuration
#
##############################################################


# 1. Change PIN_HOME to point to the top-level directory where
#    Pin was installed. This can also be set on the command line,
#    or as an environment variable.
#
PIN_HOME ?= ../pin

STOOLDIR = ../stool
STOOLHDR = $(STOOLDIR)/STool.h $(STOOLDIR)/TraceWriter.h
STOOLSRC = $(STOOLDIR)/STool_PIN.cpp $(STOOLDIR)/TraceWriter.cpp
STOOLOBJ = $(STOOLDIR)/STool_PIN.o $(STOOLDIR)/TraceWriter.o
USER_INCLUDES = -I$(STOOLDIR)


##############################################################
#
# set up and include *.config files
#
##############################################################

PIN_KIT=$(PIN_HOME)
KIT=1

TARGET_COMPILER?=gnu

ifeq ($(TARGET_COMPILER),gnu)
    include $(PIN_HOME)/source/tools/makefile.gnu.config
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) $(USER_INCLUDES)
    PIN=$(PIN_HOME)/pin
endif


##############################################################
#
# Tools - you may wish to add your tool name to TOOL_ROOTS
#
##############################################################


TOOL_ROOTS = rectrace

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))


##############################################################
#
# build rules
#
##############################################################

GLIB_CFLAGS = `pkg-config --cflags glib-2.0`
GLIB_LIBS = `pkg-config --libs glib-2.0`

LFS_CFLAGS = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

all: tools timer
tools: $(STOOLOBJ) $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TOOL_ROOTS:%=%.test)

timer: external_timer.c
	gcc -o external_timer external_timer.c
	
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(STOOLOBJ): $(STOOLSRC) $(STOOLHDR)
	cd $(STOOLDIR);$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) $(GLIB_CFLAGS) $(LFS_CFLAGS) $(STOOLSRC)

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o  $(STOOLOBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(STOOLOBJ) $(PIN_LIBS) $(DBG) $(GLIB_LIBS)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed makefile.copy external_timer
clean-logs:
	-rm -rf *.trace *.trace.* pin.log

