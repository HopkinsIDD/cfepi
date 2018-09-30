TOPTARGETS := all clean

SUBDIRS := pdfs figures

$(TOPTARGETS): pdfs figures
pdfs: figures
	$(MAKE) -C pdfs $(MAKECMDGOALS)
figures:
	$(MAKE) -C figures $(MAKECMDGOALS)

run_interventions: 
	Rscript R/setup_all_interventions.R

clean_interventions:
	rm output/*.csv

.PHONY: $(TOPTARGETS) $(SUBDIRS) run_interventions clean_interventions
