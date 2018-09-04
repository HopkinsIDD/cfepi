TEX=pdflatex
R=Rscript
BIB=bibtex
CROP=pdfcrop
.PHONY: all clean figures tikz-figures r-figures

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

figures: tikz-figures r-figures
tikz-figures: figure1.pdf figure2.pdf figure3.pdf figure4.pdf figure5.pdf figure6.pdf
r-figures: intervention-effects-combined-boxplots.pdf intervention-effects-final-size.pdf intervention-effects-peak-time.pdf intervention-effects-raw-boxplots.pdf intervention-effects-relative-risk.pdf

clean:
	rm -f *.aux *.log *.pdf
