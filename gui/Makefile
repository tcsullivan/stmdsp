UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  RM = rm -f
else
  RM = del
endif

CXX = g++-10
CXXFLAGS = --std=c++20 -ggdb -O0 \
           -Wall -Wextra -pedantic \
           -Wno-deprecated-copy \
           -Iserial/include -IMETL/include -IMETL/dependencies/PEGTL/include

ifeq ($(UNAME), Linux)
CXXFLAGS += $(shell wx-config --cxxflags)
else
CXXFLAGS += -IC:\wx\include -DSTMDSP_WIN32 -Wa,-mbig-obj
endif

ifeq ($(UNAME), Linux)
CXXFILES = serial/src/serial.cc \
           serial/src/impl/unix.cc \
           serial/src/impl/list_ports/list_ports_linux.cc \
           $(wildcard *.cpp)
else
CXXFILES = serial/src/serial.cc \
           serial/src/impl/win.cc \
           serial/src/impl/list_ports/list_ports_win.cc \
           $(wildcard *.cpp)
endif
OFILES = $(patsubst %.cc, %.o, $(patsubst %.cpp, %.o, $(CXXFILES)))

ifeq ($(UNAME), Linux)
LIBS = $(shell wx-config --libs) -lwx_gtk3u_stc-3.1
else
LIBS = -lSetupAPI \
       -LC:\wx\lib\gcc810_x64_dll -lwxbase31u -lwxmsw31u_core -lwxmsw31u_stc
endif

OUTELF = stmdspgui

ifeq ($(UNAME), Linux)
CLEANFILES = $(OUTELF) $(OFILES)
else
CLEANFILES = $(subst /,\\,$(OUTELF)) $(subst /,\\,$(OFILES))
endif

all: $(OUTELF)

$(OUTELF): $(OFILES)
	@echo "  CXX    " $(OUTELF)
	@$(CXX) $(CXXFLAGS) $(OFILES) $(LIBS) -o $(OUTELF)

.cc.o:
	@echo "  CXX    " $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

.cpp.o:
	@echo "  CXX    " $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "  CLEAN"
	@$(RM) $(CLEANFILES)

