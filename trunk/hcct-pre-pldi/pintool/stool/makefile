##############################################################
##
## PIN tool makefile
##
##############################################################

PIN_HOME ?= ../pin

USER_LIBS = -lglib-2.0
USER_INCLUDES = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
OBJ = STool_PIN.o TraceWriter.o TraceReader.o

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

