
SUBDIRS=$(wildcard phase2[a-d])

.PHONY: $(SUBDIRS) all clean install subdirs

all: $(SUBDIRS)

subdirs: $(SUBDIRS)

clean: $(SUBDIRS)
	rm -f p3/*.o

install: $(SUBDIRS)

tests: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)