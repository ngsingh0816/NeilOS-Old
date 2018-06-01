
# Makefile for OS project
# To build, first `make dep`, them `make`. Everything should be automatic.
# Will compile all *.cpp and *.S files in current and sub directories.

# Flags to use when compiling, preprocessing, assembling, and linking
CFLAGS+=-Wall -fPIC -g -ffast-math -funsafe-math-optimizations -DLIBRARY_NEILOS #-O3
CC=/Users/Neil/NeilOS/cross/bin/i686-neilos-gcc
CXX=/Users/Neil/NeilOS/cross/bin/i686-neilos-g++
AR=/Users/Neil/NeilOS/cross/bin/i686-neilos-ar
OBJDIR=build
FOLDERS=Core GUI

#If you have any .h files in another directory, add -I<dir> to this line

# This generates the list of source files
SRC=$(shell find . -name '*.S') $(shell find . -name '*.cpp')

# This generates the list of .o files.
OBJS=$(addprefix $(OBJDIR)/, $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRC))))

print-%  : ; @echo $* = $($*)

libNeilOS: Makefile $(OBJS)
	mkdir -p output
	# Create shared library
	$(CXX) -shared -o output/libNeilOS.so -lpthread -lgraphics -Wl,--whole-archive -lmpg123 -lz -lpng -ljpeg -lgif\
		-ltiff -lfreetype -Wl,--no-whole-archive $(OBJS)
	cp -rf output/libNeilOS.so /Users/Neil/NeilOS/sysroot/usr/lib/
	$(foreach var, $(FOLDERS), \
		mkdir -p /Users/Neil/NeilOS/sysroot/usr/include/NeilOS/$(var)/; \
	find ./$(var)/ -name \*.h -exec cp -prf {} /Users/Neil/NeilOS/sysroot/usr/include/NeilOS/$(var)/ ';' ;)
	# Copy over NeilOS.h
	cp NeilOS.h /Users/Neil/NeilOS/sysroot/usr/include/NeilOS/
	cp -r Core/Events/ /Users/Neil/NeilOS/programs/WindowServer/Events/

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
