include Defs

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
