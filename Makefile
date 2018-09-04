TOPTARGETS := all clean

SUBDIRS := pdfs figures

$(TOPTARGETS): pdfs figures
pdfs: figures
	$(MAKE) -C pdfs $(MAKECMDGOALS)
figures:
	$(MAKE) -C figures $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
