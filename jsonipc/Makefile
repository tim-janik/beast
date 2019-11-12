all: testjsonipc

CXX	 = ccache clang++
#CXX	 = ccache g++ -Wno-nonnull-compare -Wno-address
CXXFLAGS = -std=gnu++14 -Wall -O0 -g
OBJECTS  = testjsonipc.o
DEFS     = -DSTANDALONE
DEPS     = jsonipc.hh Makefile

INCLUDES = -I../external/

$(OBJECTS): %.o: %.cc $(DEPS)
	$(CXX) $(DEFS) $(CXXFLAGS) $(INCLUDES) -c $<

testjsonipc: $(OBJECTS) Makefile
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -lboost_system

check: testjsonipc
	./testjsonipc

clean:
	rm -f $(OBJECTS) testjsonipc
