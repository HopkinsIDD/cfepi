TEX=pdflatex
R=Rscript
BIB=bibtex
CROP=pdfcrop
PDFTK=pdftk
.PHONY: all clean figures tikz-figures r-figures figure7-burst peg_steps-burst epidemic_uncontrolled-burst epidemic_controlled-burst

all: figures

handout.pdf: figures
	$(TEX) ../tex/handout.tex

view: main.pdf
	open main.pdf

figure1.pdf:
	$(TEX) ../tex/figure1.tex 
figure2.pdf:
	$(TEX) ../tex/figure2.tex 
figure3.pdf:
	$(TEX) ../tex/figure3.tex 
figure4.pdf:
	$(TEX) ../tex/figure4.tex 
peg_steps.pdf:
	$(TEX) ../tex/peg_steps.tex 
peg_steps-burst: peg_steps.pdf
	$(PDFTK) peg_steps.pdf burst output peg_steps_pg_%01d.pdf
peg_steps_pg_1.pdf: peg_steps-burst
peg_steps_pg_2.pdf: peg_steps-burst
peg_steps_pg_3.pdf: peg_steps-burst
peg_steps_pg_4.pdf: peg_steps-burst
peg_steps_pg_5.pdf: peg_steps-burst
peg_create.pdf: peg_steps-burst
	$(TEX) ../tex/peg_create.tex
#	$(TEX) peg_create-precrop.pdf peg_create.pdf
figure7.pdf:
	$(TEX) ../tex/figure7.tex 
figure7-burst: figure7.pdf
	$(PDFTK) figure7.pdf burst output figure7_pg_%01d.pdf
figure7_pg_1.pdf: figure7-burst
figure7_pg_2.pdf: figure7-burst
figure7_pg_3.pdf: figure7-burst
figure7_pg_4.pdf: figure7-burst
figure7_pg_5.pdf: figure7-burst
figure8.pdf: figure7_pg_1.pdf figure7_pg_2.pdf figure7_pg_3.pdf figure7_pg_4.pdf figure7_pg_5.pdf
	$(TEX) ../tex/figure8.tex 
epidemic_uncontrolled.pdf:
	$(TEX) ../tex/epidemic_uncontrolled.tex 
epidemic_uncontrolled-burst: epidemic_uncontrolled.pdf
	$(PDFTK) epidemic_uncontrolled.pdf burst output epidemic_uncontrolled_pg_%01d.pdf
epidemic_uncontrolled_pg_1.pdf: epidemic_uncontrolled-burst
epidemic_uncontrolled_pg_2.pdf: epidemic_uncontrolled-burst
epidemic_uncontrolled_pg_3.pdf: epidemic_uncontrolled-burst
epidemic_uncontrolled_pg_4.pdf: epidemic_uncontrolled-burst
epidemic_uncontrolled_pg_5.pdf: epidemic_uncontrolled-burst
epidemic_controlled.pdf:
	$(TEX) ../tex/epidemic_controlled.tex 
epidemic_controlled-burst: epidemic_controlled.pdf
	$(PDFTK) epidemic_controlled.pdf burst output epidemic_controlled_pg_%01d.pdf
epidemic_controlled_pg_1.pdf: epidemic_controlled-burst
epidemic_controlled_pg_2.pdf: epidemic_controlled-burst
epidemic_controlled_pg_3.pdf: epidemic_controlled-burst
epidemic_controlled_pg_4.pdf: epidemic_controlled-burst
epidemic_controlled_pg_5.pdf: epidemic_controlled-burst
epidemic_both.pdf: epidemic_uncontrolled-burst epidemic_controlled-burst
	$(TEX) ../tex/epidemic_both.tex 
figure12.pdf:
	$(TEX) ../tex/figure12.tex 
event-anatomy.pdf:
	$(TEX) ../tex/event-anatomy.tex 
event-anatomy-simplified.pdf:
	$(TEX) ../tex/event-anatomy-simplified.tex 

intervention-effects-combined-boxplots.pdf:
	$(R) ../R/make_figures.R
intervention-effects-final-size.pdf:
	$(R) ../R/make_figures.R
intervention-effects-peak-time.pdf:
	$(R) ../R/make_figures.R
intervention-effects-raw-boxplots.pdf:
	$(R) ../R/make_figures.R
intervention-effects-relative-risk.pdf:
	$(R) ../R/make_figures.R
intervention-effects-time-series-recovered.pdf:
	$(R) ../R/make_figures.R
intervention-effects-time-series-recovered-switched.pdf:
	$(R) ../R/make_figures.R
intervention-effects-time-series-susceptible.pdf:
	$(R) ../R/make_figures.R
intervention-effects-time-series-susceptible-switched.pdf:
	$(R) ../R/make_figures.R
intervention-effects-time-series-cases-averted.pdf:
	$(R) ../R/make_figures.R
intervention-effects-time-series-cases-averted-switched.pdf:
	$(R) ../R/make_figures.R

intervention-effects-combined-boxplots-cropped.pdf: intervention-effects-combined-boxplots.pdf
	$(CROP) intervention-effects-combined-boxplots.pdf intervention-effects-combined-boxplots-cropped.pdf
intervention-effects-final-size-cropped.pdf: intervention-effects-final-size.pdf
	$(CROP) intervention-effects-final-size.pdf intervention-effects-final-size-cropped.pdf
intervention-effects-peak-time-cropped.pdf: intervention-effects-peak-time.pdf
	$(CROP) intervention-effects-peak-time.pdf intervention-effects-peak-time-cropped.pdf
intervention-effects-raw-boxplots-cropped.pdf: intervention-effects-raw-boxplots.pdf
	$(CROP) intervention-effects-raw-boxplots.pdf intervention-effects-raw-boxplots-cropped.pdf
intervention-effects-relative-risk-cropped.pdf: intervention-effects-relative-risk.pdf
	$(CROP) intervention-effects-relative-risk.pdf intervention-effects-relative-risk-cropped.pdf
intervention-effects-time-series-recovered-cropped.pdf: intervention-effects-time-series-recovered.pdf
	$(CROP) intervention-effects-time-series-recovered.pdf intervention-effects-time-series-recovered-cropped.pdf
intervention-effects-time-series-recovered-switched-cropped.pdf: intervention-effects-time-series-recovered-switched.pdf
	$(CROP) intervention-effects-time-series-recovered-switched.pdf intervention-effects-time-series-recovered-switched-cropped.pdf
intervention-effects-time-series-susceptible-cropped.pdf: intervention-effects-time-series-susceptible.pdf
	$(CROP) intervention-effects-time-series-susceptible.pdf intervention-effects-time-series-susceptible-cropped.pdf
intervention-effects-time-series-susceptible-switched-cropped.pdf: intervention-effects-time-series-susceptible-switched.pdf
	$(CROP) intervention-effects-time-series-susceptible-switched.pdf intervention-effects-time-series-susceptible-switched-cropped.pdf
intervention-effects-time-series-cases-averted-cropped.pdf: intervention-effects-time-series-cases-averted.pdf
	$(CROP) intervention-effects-time-series-cases-averted.pdf intervention-effects-time-series-cases-averted-cropped.pdf
intervention-effects-time-series-cases-averted-switched-cropped.pdf: intervention-effects-time-series-cases-averted-switched.pdf
	$(CROP) intervention-effects-time-series-cases-averted-switched.pdf intervention-effects-time-series-cases-averted-switched-cropped.pdf

figures: tikz-figures r-figures
tikz-figures: event-anatomy.pdf peg_create.pdf epidemic_both.pdf event-anatomy-simplified.pdf figure12.pdf
r-figures: intervention-effects-time-series-cases-averted-switched-cropped.pdf

clean:
	rm -f *.aux *.log *.pdf intervention-effects-time-series-susceptible *.txt
