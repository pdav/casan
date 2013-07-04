.SUFFIXES:	.fig .pdf
FILE = talk

XFIG =	memory0.pdf memory1.pdf arduino0.pdf arduino1.pdf arduino2.pdf

GENFIGS = fig/memory0.fig fig/memory1.fig \
	fig/arduino0.fig fig/arduino1.fig fig/arduino2.fig

FIGS = $(GENFIGS) img/icube.png $(XFIG)
TEX = content/arduino.tex \
	content/components.tex \
	content/usage.tex

all: $(FILE).pdf 

$(FILE).pdf: $(FILE).tex $(TEX) $(FIGS) 
	pdflatex $(FILE).tex
	pdflatex $(FILE).tex

fig/memory0.fig: fig/memory.fig
	figlayers             10-11 99 < fig/memory.fig > fig/memory0.fig
fig/memory1.fig: fig/memory.fig
	figlayers             10-12 99 < fig/memory.fig > fig/memory1.fig

fig/arduino0.fig: fig/arduino.fig
	figlayers             10 99 < fig/arduino.fig > fig/arduino0.fig
fig/arduino1.fig: fig/arduino.fig
	figlayers             10-20 99 < fig/arduino.fig > fig/arduino1.fig
fig/arduino2.fig: fig/arduino.fig
	figlayers             10-30 99 < fig/arduino.fig > fig/arduino2.fig

memory0.pdf :
	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf
memory1.pdf :
	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf

arduino0.pdf :
	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf
arduino1.pdf :
	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf
arduino2.pdf :
	fig2dev -L pdf fig/$*.fig > pdf/$*.pdf

test : all
	evince $(FILE).pdf

clean:
	rm -f *.aux *.log *.nav *.out *.snm *.vrb *.toc pdf/*.pdf *.bak
	rm -f $(FILE).pdf $(GENFIGS)
	rm -f fig/*.bak
