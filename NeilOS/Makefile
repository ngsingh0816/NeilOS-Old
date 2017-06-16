
# Makefile for OS project
# To build, first `make dep`, them `make`. Everything should be automatic.
# Will compile all *.c and *.S files in current and sub directories.

# Flags to use when compiling, preprocessing, assembling, and linking
CFLAGS+=-Wall -fno-builtin -fno-stack-protector -nostdlib
ASFLAGS+=
LDFLAGS+=-nostdlib -static
CC=gcc
OBJDIR=build

#If you have any .h files in another directory, add -I<dir> to this line
CFLAGS+=-nostdinc -g -Ikernel

# This generates the list of source files
SRC=$(shell find . -name *.S) $(shell find . -name *.c)

# This generates the list of .o files. The order matters, boot.o must be first
OBJS=$(OBJDIR)/./kernel/boot/boot.o
OBJS+=$(addprefix $(OBJDIR)/, $(filter-out ./kernel/boot/boot.o,$(patsubst %.S,%.o,$(filter %.S,$(SRC)))))
OBJS+=$(addprefix $(OBJDIR)/, $(patsubst %.c,%.o,$(filter %.c,$(SRC))))

print-%  : ; @echo $* = $($*)

bootimg: Makefile $(OBJS)
	rm -f bootimg
	# Loads the kernel at 4MB
	$(CC) $(LDFLAGS) $(OBJS) -Ttext=0x400000 -o bootimg
	#sudo ./debug.sh

dep: Makefile.dep

$(OBJDIR):
	mkdir $(OBJDIR)

Makefile.dep: $(SRC) $(OBJDIR)
	rm -f Makefile.dep;

	$(foreach var, $(SRC), \
		mkdir -p $(OBJDIR)/$(dir $(var)); \
		$(CC) -MM -MT "$(OBJDIR)/$(basename $(var)).o" $(CFLAGS) $(var) >> $@; \
		printf "\t$(CC) $(CFLAGS) -c -o "$(OBJDIR)/$(basename $(var)).o" $(var)\n" >> $@;)

.PHONY: clean
clean:
	find . -type f -name '*.o' -exec rm {} +
	rm -f Makefile.dep
	rm -f bootimg

ifneq ($(MAKECMDGOALS),dep)
ifneq ($(MAKECMDGOALS),clean)
include Makefile.dep
endif
endif
