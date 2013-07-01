.SUFFIXES:	.fig .pdf
FILE = talk

XFIG =	#auto-slave.pdf auto-master.pdf handshake.pdf

FIGS = img/icube.png $(XFIG)

all: $(FIGS) $(FILE).pdf 

$(FILE).pdf: $(FILE).tex
	pdflatex $(FILE).tex
	pdflatex $(FILE).tex

#auto-slave.pdf :
#	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf
#auto-master.pdf :
#	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf
#handshake.pdf :
#	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf

test : all
	evince $(FILE).pdf

clean:
	rm -f *.aux *.log *.nav *.out *.snm *.vrb *.toc pdf/*.pdf *.bak
	rm -f $(FILE).pdf
