##############################################################
##
## PIN tool makefile
##
##############################################################

PIN_HOME ?= ../pin

USER_LPATHS =
USER_LIBS = -lglib-2.0
USER_INCLUDES = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include

TARGET_COMPILER?=gnu

ifeq ($(TARGET_COMPILER),gnu)
    include $(PIN_HOME)/source/tools/makefile.gnu.config
    CXXFLAGS ?= -Wall -Wno-unknown-pragmas $(DBG) $(OPT) $(USER_INCLUDES)
    PIN=$(PIN_HOME)/pin
endif

PIN_KIT=$(PIN_HOME)

KIT=1

OBJS = $(OBJDIR)empty.o

ifeq ($(TARGET),ia32)
   # can't handle 64 bit syscalls
   TOOL_NAME = empty
endif

TOOL = $(OBJDIR)$(TOOL_NAME)$(PINTOOL_SUFFIX)


##############################################################
#
# build rules
#
##############################################################

all: $(OBJDIR) $(TOOL)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<

$(TOOL) : $(OBJS) $(PIN_LIBNAMES)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) $(OBJS) -o $@  $(DBG) ${PIN_LPATHS} $(USER_LPATHS) $(PIN_LIBS) $(USER_LIBS)

clean:
	-rm -rf $(OBJDIR)

