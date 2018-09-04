TEX=pdflatex
R=Rscript
BIB=bibtex
.PHONY: all clean view figures

all: main.pdf outline.pdf

view: main.pdf
	open main.pdf

main.pdf:
	$(TEX) ../tex/main.tex
	$(BIB) main
	$(TEX) ../tex/main.tex
	$(TEX) ../tex/main.tex
	$(TEX) ../tex/main.tex
outline.pdf:
	$(TEX) ../tex/outline.tex

clean:
	rm -f main.log main.pdf main.aux outline.log outline.pdf outline.aux main.bbl main.blg main.out outline.out outline.toc
