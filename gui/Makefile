CXX = g++-10
CXXFLAGS = --std=c++20 -ggdb -O0 \
           -Wall -Wextra -pedantic \
           -Wno-deprecated-copy \
           -Iserial/include \
           $(shell wx-config --cxxflags)

CXXFILES = serial/src/serial.cc \
           serial/src/impl/unix.cc \
           serial/src/impl/list_ports/list_ports_linux.cc \
           $(wildcard *.cpp)
OFILES = $(patsubst %.cc, %.o, $(patsubst %.cpp, %.o, $(CXXFILES)))

LIBS = $(shell wx-config --libs) -lwx_gtk3u_stc-3.1
OUTELF = stmdspgui

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
	@rm -f $(OUTELF) $(OFILES)

