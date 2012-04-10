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
STOOLHDR = $(STOOLDIR)/STool.h
STOOLSRC = $(STOOLDIR)/STool_PIN.cpp
STOOLOBJ = $(STOOLDIR)/STool_PIN.o
USER_INCLUDES = -I$(STOOLDIR)


##############################################################
#
# set up and include *.config files
#
##############################################################

PIN_KIT=$(PIN_HOME)
KIT=1
TESTAPP=$(OBJDIR)cp-pin.exe

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

ifeq ($(TARGET_COMPILER),gnu)
    include $(PIN_HOME)/source/tools/makefile.gnu.config
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) $(USER_INCLUDES)
    PIN=$(PIN_HOME)/pin
endif

ifeq ($(TARGET_COMPILER),ms)
    include $(PIN_HOME)/source/tools/makefile.ms.config
    DBG?=
    PIN=$(PIN_HOME)/pin.bat
endif


##############################################################
#
# Tools - you may wish to add your tool name to TOOL_ROOTS
#
##############################################################


TOOL_ROOTS = hcct

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))


##############################################################
#
# build rules
#
##############################################################

all: tools
tools: $(STOOLOBJ) $(OBJDIR) $(TOOLS) 

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : hcct.cpp streaming.h ss.c
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}hcct.o hcct.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}ss.o ss.c

$(TOOLS): $(PIN_LIBNAMES) 

$(STOOLOBJ): $(STOOLSRC) $(STOOLHDR)
	$(CXX) -c -o $(STOOLOBJ) $(CXXFLAGS) $(PIN_CXXFLAGS) $(STOOLSRC)

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o  $(STOOLOBJ)
	${PIN_LD} -lrt $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ hcct.o ss.o ${PIN_LPATHS} $(STOOLOBJ) $(PIN_LIBS) $(DBG) $(USER_LIBS)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.o
	
cleanlogs:
	-rm -rf *.out* *.stdata* *.dump* pin.log