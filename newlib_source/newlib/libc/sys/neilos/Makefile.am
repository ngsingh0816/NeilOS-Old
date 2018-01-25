AUTOMAKE_OPTIONS = cygnus
INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)
AM_CCASFLAGS = $(INCLUDES)
 
noinst_LIBRARIES = lib.a
 
if MAY_SUPPLY_SYSCALLS
extra_objs = syscalls.o sysfile.o sysfs.o sysgraphics.o sysmem.o sysmisc.o sysproc.o syssched.o syssignal.o syssockets.o systermios.o systime.o
else
extra_objs =
endif
 
lib_a_SOURCES =
lib_a_LIBADD = $(extra_objs)
EXTRA_lib_a_SOURCES = crt0.c syscalls.S sysfile.c sysfs.c sysgraphics.c sysmem.c sysmisc.c sysproc.c syssched.c syssignal.c syssockets.o systermios.c systime.c
lib_a_DEPENDENCIES = $(extra_objs)
lib_a_CCASFLAGS = $(AM_CCASFLAGS)
lib_a_CFLAGS = $(AM_CFLAGS)
 
if MAY_SUPPLY_SYSCALLS
all: crt0.o
endif
 
ACLOCAL_AMFLAGS = -I ../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(tooldir)/include/net; \
	for i in $(srcdir)/include/net/*.h; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/net/`basename $$i`; \
	done; \
	$(mkinstalldirs) $(DESTDIR)$(tooldir)/include/netinet; \
	for i in $(srcdir)/include/netinet/*.h; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/netinet/`basename $$i`; \
	done; \
	$(mkinstalldirs) $(DESTDIR)$(tooldir)/include/netinet6; \
	for i in $(srcdir)/include/netinet6/*.h; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/netinet6/`basename $$i`; \
	done;
