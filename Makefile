
.PHONY: all clean distclean

OBJS = cmd.o command.o pixel_dtb.o protocol.o settings.o psi46test.o datastream.o analyzer.o profiler.o rpc.o rpc_calls.o usb.o rpc_error.o rs232.o iseg.o
# chipdatabase.o color.o defectlist.o error.o histo.o pixelmap.o prober.o ps.o scanner.o
# test_dig.o
# plot.o

ROOTCFLAGS = $(shell $(ROOTSYS)/bin/root-config --cflags)
ROOTGLIBS  = $(shell $(ROOTSYS)/bin/root-config --glibs) # with Gui

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CXXFLAGS = -g -Os -Wall $(ROOTCFLAGS) -I/usr/local/include -I/usr/X11/include
LDFLAGS = -lftd2xx -lreadline -L/usr/local/lib -L/usr/X11/lib -lX11 $(ROOTGLIBS)
endif

# root g++ flags = -pthread -std=c++11 -Wno-deprecated-declarations -m64 -I/home/pitzl/ROOT/root/include

ifeq ($(UNAME), Linux)
CXXFLAGS = -g -Os -Wall $(ROOTCFLAGS) -I/usr/local/include -I/usr/X11/include
LDFLAGS = -lftd2xx -lreadline -L/usr/local/lib -L/usr/X11/lib -lX11 -pthread -lrt $(ROOTGLIBS)
endif


# PATTERN RULES:

obj/%.o : %.cpp
	@mkdir -p obj/linux
	@echo 'root' $(CXX) 'flags =' $(ROOTCFLAGS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

#obj/%.d : %.cpp obj
#	@mkdir -p obj/linux
#	$(shell $(CXX) -MM $(CXXFLAGS) $< | awk -F: '{if (NF > 1) print "obj/"$$0; else print $0}' > $@)


# TARGETS:

all: bin/psi46test
	@true

obj:
	@mkdir -p obj/linux

bin:
	@mkdir -p bin

bin/psi46test: $(addprefix obj/,$(OBJS)) bin rpc_calls.cpp
	$(CXX) -o $@ $(addprefix obj/,$(OBJS)) $(LDFLAGS)
	@echo 'done: bin/psi46test'

o2r: o2r.cc
	g++ $(ROOTCFLAGS) o2r.cc \
	-Wall -O2 -o o2r $(ROOTLIBS)
	@echo 'done: o2r'

clean:
	rm -rf obj

distclean: clean
	rm -rf bin


# DEPENDENCIES:

#-include $(addprefix obj/,$(OBJS:.o=.d))
