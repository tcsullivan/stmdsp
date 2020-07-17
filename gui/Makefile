CXX = g++-10
CXXFLAGS = --std=c++20 -ggdb -Og \
		   -Wall -Wextra -pedantic \
		   -Wno-deprecated-copy \
		   -Iserial/include \
		   $(shell wx-config --cxxflags)

CXXFILES = $(shell find serial/src -name "*.cc") $(wildcard *.cpp)
OFILES = $(patsubst %.cc, %.o, $(patsubst %.cpp, %.o, $(CXXFILES)))

LIBS = $(shell wx-config --libs)
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

