TEX=pdflatex
R=Rscript
BIB=bibtex
.PHONY: all clean view figures

all: main.pdf outline.pdf

view: main.pdf
	open main.pdf

main.pdf: figures main.tex
	$(TEX) tex/main.tex
outline.pdf: figures outline.tex
	$(TEX) tex/outline.tex

figures:
	$(R) figures/make_figures.R

main.tex:

outline.tex:

clean:
	rm -f main.log main.pdf main.aux outline.log outline.pdf outline.aux tex/main.log tex/main.synctex.gz tex/main.pdf tex/main.aux tex/outline.log tex/outline.synctex.gz tex/outline.pdf tex/outline.aux
