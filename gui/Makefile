CXX = g++-10
CXXFLAGS = --std=c++20 -ggdb -Og \
		   -Wall -Wextra -pedantic \
		   -Wno-deprecated-copy \
		   $(shell wx-config --cxxflags)

CXXFILES = $(wildcard *.cpp)

LIBS = $(shell wx-config --libs)
OUTELF = stmdspgui

all: $(CXXFILES)
	@echo "  CXX    " $(CXXFILES)
	@$(CXX) $(CXXFLAGS) $(CXXFILES) $(LIBS) -o $(OUTELF)

clean:
	@echo "  CLEAN"
	@rm -f $(OUTELF)

