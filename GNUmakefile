include Defs

# Not all programs are in SUBDIRS for the following reasons:

#   - dir_chuck_test and nlmtest are left out of SUBDIRS because they
#     are unused and have never been installed in /usr/local/bin.
#     Their source remains since they may potentially be useful.

#   - nfstest, tclent, and tclnfs are left out of SUBDIRS because they
#     take a whole lot more work to configure but are rarely needed to
#     be rebuilt.

#   - randdir is left out of SUBDIRS because it is unfinished.

# (build only when building for unix)
ifeq ($(UNIX),yes)
SUBDIRS := src
endif

all: build

build build-bin build-man install install-bin install-man clean:
	for x in $(SUBDIRS) ; do $(MAKE) -C $$x $@ ; done

distclean:
	for x in $(SUBDIRS) ; do $(MAKE) -C $$x $@ ; done
	-rm -f config.status config.log config.cache Defs
