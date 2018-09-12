TEX=pdflatex
R=Rscript
BIB=bibtex
CROP=pdfcrop
PDFTK=pdftk
.PHONY: all clean figures tikz-figures r-figures figure7-burst

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
figure5.pdf:
	$(TEX) ../tex/figure5.tex 
figure6-precrop.pdf: figure5.pdf
	$(TEX) ../tex/figure6-precrop.tex 
figure6.pdf: figure6-precrop.pdf
	$(CROP) figure6-precrop.pdf figure6.pdf
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
figure9.pdf:
	$(TEX) ../tex/figure9.tex 
figure10-precrop.pdf: figure9.pdf
	$(TEX) ../tex/figure10-precrop.tex 
figure10.pdf: figure10-precrop.pdf
	$(CROP) figure10-precrop.pdf figure10.pdf

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

figures: tikz-figures r-figures
tikz-figures: figure1.pdf figure2.pdf figure3.pdf figure4.pdf figure6.pdf figure8.pdf figure10.pdf
r-figures: intervention-effects-combined-boxplots-cropped.pdf intervention-effects-final-size-cropped.pdf intervention-effects-peak-time-cropped.pdf intervention-effects-raw-boxplots-cropped.pdf intervention-effects-relative-risk-cropped.pdf intervention-effects-time-series-recovered-cropped.pdf intervention-effects-time-series-recovered-switched-cropped.pdf intervention-effects-time-series-susceptible-cropped.pdf intervention-effects-time-series-susceptible-switched-cropped.pdf

clean:
	rm -f *.aux *.log *.pdf intervention-effects-time-series-susceptible *.txt
