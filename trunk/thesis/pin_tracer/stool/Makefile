##############################################################
##
## PIN tool makefile
##
##############################################################

PIN_HOME ?= ../pin

GLIB_CFLAGS = `pkg-config --cflags glib-2.0`
GLIB_LIBS = `pkg-config --libs glib-2.0`

USER_LIBS = $(GLIB_LIBS)
USER_INCLUDES = $(GLIB_CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
OBJ = STool_PIN.o TraceWriter.o

TARGET_COMPILER?=gnu

ifeq ($(TARGET_COMPILER),gnu)
    include $(PIN_HOME)/source/tools/makefile.gnu.config
    CXXFLAGS ?= -Wall -Wno-unknown-pragmas $(DBG) $(OPT) $(USER_INCLUDES)
endif

PIN_KIT=$(PIN_HOME)

KIT=1


##############################################################
#
# build rules
#
##############################################################

all: libstool.a

libstool.a: $(OBJ)
	ar -cr libstool.a $(OBJ)

%_PIN.o : %_PIN.cpp %.h
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) $<

%.o : %.cpp %.h
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) $<

clean:
	-rm -rf *.o *.a

