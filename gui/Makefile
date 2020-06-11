CXX = clang++-10
CXXFLAGS = --std=c++20 -Wall -Wextra -pedantic

CXXFILES = $(wildcard *.cpp)

OUTELF = stmdspgui

all: $(CXXFILES)
	@echo "  CXX    " $(CXXFILES)
	@$(CXX) $(CXXFLAGS) $(CXXFILES) -o $(OUTELF)

clean:
	@echo "  CLEAN"
	@rm -f $(OUTELF)

