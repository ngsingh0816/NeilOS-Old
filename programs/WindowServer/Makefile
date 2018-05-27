
# Makefile for OS project
# To build, first `make dep`, them `make`. Everything should be automatic.
# Will compile all *.cpp and *.S files in current and sub directories.

# Flags to use when compiling, preprocessing, assembling, and linking
CFLAGS+=-Wall -g -ffast-math -funsafe-math-optimizations #-O3
CC=/Users/Neil/NeilOS/cross/bin/i686-neilos-gcc
CXX=/Users/Neil/NeilOS/cross/bin/i686-neilos-g++
AR=/Users/Neil/NeilOS/cross/bin/i686-neilos-ar
OBJDIR=build

#If you have any .h files in another directory, add -I<dir> to this line

# This generates the list of source files
SRC=$(shell find . -name '*.S') $(shell find . -name '*.cpp') $(shell find . -name '*.c')

# This generates the list of .o files.
OBJS=$(addprefix $(OBJDIR)/, $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRC))))

print-%  : ; @echo $* = $($*)

WindowServer: Makefile $(OBJS)
	mkdir -p output
	$(CXX) -o output/WindowServer -lpthread -lgraphics -lNeilOS $(OBJS)
	cp -rf output/WindowServer /Users/Neil/NeilOS/sysroot/system/apps/WindowServer

dep: Makefile.dep

$(OBJDIR):
	mkdir $(OBJDIR)

Makefile.dep: $(SRC) $(OBJDIR)
	rm -f Makefile.dep;

	$(foreach var, $(SRC), \
		mkdir -p $(OBJDIR)/$(dir $(var)); \
		$(CXX) -MM -MT "$(OBJDIR)/$(basename $(var)).o" $(CFLAGS) $(var) >> $@; \
		printf "\t$(CXX) $(CFLAGS) -c -o "$(OBJDIR)/$(basename $(var)).o" $(var)\n" >> $@;)

.PHONY: clean
clean:
	find . -type f -name '*.o' -exec rm {} +
	rm -f Makefile.dep
	rm -rf output

ifneq ($(MAKECMDGOALS),dep)
ifneq ($(MAKECMDGOALS),clean)
include Makefile.dep
endif
endif
