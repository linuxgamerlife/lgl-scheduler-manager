NAME := lgl-scheduler-manager
SPEC := packaging/$(NAME).spec
VERSION := $(shell awk '/^Version:/ {print $$2; exit}' $(SPEC))
ARCHIVE := result/$(NAME)-$(VERSION).tar.gz

.PHONY: archive srpm clean

archive:
	mkdir -p result
	rm -f $(ARCHIVE)
	tar --sort=name --mtime='UTC 2026-10-21' --owner=0 --group=0 --numeric-owner \
		--exclude=.git --exclude=legacy/qt --exclude=rpmbuild --exclude=result \
		--exclude='build*' --transform='s,^,$(NAME)-$(VERSION)/,' \
		-czf $(ARCHIVE) .

srpm: archive
	rpmbuild -bs $(SPEC) --define '_sourcedir $(CURDIR)/result' \
		--define '_srcrpmdir $(CURDIR)/result'

clean:
	rm -rf result
