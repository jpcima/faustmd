PREFIX = /usr/local
CXX = g++
CXXFLAGS = -O2 -g -Wall -std=c++11
LDFLAGS =

SRCS = main.cpp call_faust.cpp metadata.cpp thirdparty/pugixml.cpp
OBJS = $(SRCS:%.cpp=build/%.o)

all: bin/faustmd

clean:
	rm -rf bin
	rm -rf build

install: bin/faustmd
	install -D -m 755 bin/faustmd $(DESTDIR)$(PREFIX)/bin/faustmd

bin/faustmd: $(OBJS)
	@install -d $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^

build/%.o: %.cpp
	@install -d $(dir $@)
	$(CXX) $(CXXFLAGS) -MD -c -o $@ $<

.PHONY: all clean install

-include $(OBJS:%.o=%.d)
