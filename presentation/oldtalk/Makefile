.SUFFIXES:	.fig .pdf

XFIG =	concept.pdf stack.pdf \
	rest-princ0.pdf rest-princ1.pdf rest-princ2.pdf \
	rest-princ3.pdf rest-princ4.pdf rest-princ5.pdf \
	rest-princ6.pdf rest-princ7.pdf \
	rest-princ.pdf \
	rest-sens.pdf \
	arch-req.pdf arch-stack.pdf \
	core-arch.pdf core-stack.pdf \
	coap-fmt.pdf \
	coap-req-con.pdf coap-req-tok.pdf \
	coap-req-non.pdf coap-req-blk.pdf \
	coap-req-obs.pdf \
	sos-arch11.pdf sos-arch12.pdf sos-arch13.pdf \
	sos-arch21.pdf sos-arch22.pdf sos-arch23.pdf sos-arch24.pdf \
	sos-arch25.pdf sos-arch26.pdf sos-arch27.pdf \

XFIG =	auto-slave.pdf auto-master.pdf handshake.pdf

FIGS = icube.png $(XFIG) delicious.png power-outlet2.png tomatoes.png

GENFIGS = rest-princ0.fig rest-princ1.fig rest-princ2.fig \
	  rest-princ3.fig rest-princ4.fig rest-princ5.fig \
	  rest-princ6.fig rest-princ7.fig \
	  sos-arch11.fig sos-arch12.fig sos-arch13.fig \
	  sos-arch21.fig sos-arch22.fig sos-arch23.fig sos-arch24.fig \
	  sos-arch25.fig sos-arch26.fig sos-arch27.fig
GENFIGS =

ALLDEVS = thermometer.jpg bathroom-scale.jpg lightbulb.jpg power-outlet.jpg

.fig.pdf:
	fig2dev -L pdf $*.fig > $*.pdf

all: talk.pdf

talk.pdf: talk.tex $(FIGS)
	pdflatex talk
	pdflatex talk

concept.pdf	: iphone.png $(ALLDEVS)
stack.pdf	:
rest-princ.pdf $(GENFIGS) \
		: iphone.png cray.jpg disk.jpg
rest-sens.pdf	: iphone.png bathroom-scale.jpg lightbulb.jpg
arch-req.pdf	: iphone.png $(ALLDEVS) rpi.jpg
arch-stack.pdf	: iphone.png lightbulb.jpg rpi.jpg
core-arch.pdf	: iphone.png $(ALLDEVS) rpi.jpg
core-stack.pdf	:
coap-fmt.pdf	:
coap-req-con.pdf coap-req-tok.pdf coap-req-non.pdf \
                 coap-req-blk.pdf coap-req-obs.pdf \
		: iphone.png lightbulb.jpg
sos-arch.pdf	:

rest-princ0.fig: rest-princ.fig
	figlayers 70-74             99 < rest-princ.fig > rest-princ0.fig
rest-princ1.fig: rest-princ.fig
	figlayers             45-54 99 < rest-princ.fig > rest-princ1.fig
rest-princ2.fig: rest-princ.fig
	figlayers 1-4         45-54 99 < rest-princ.fig > rest-princ2.fig
rest-princ3.fig: rest-princ.fig
	figlayers 5-9         45-54 99 < rest-princ.fig > rest-princ3.fig
rest-princ4.fig: rest-princ.fig
	figlayers 10-14       45-54 99 < rest-princ.fig > rest-princ4.fig
rest-princ5.fig: rest-princ.fig
	figlayers 15-19       45-54 99 < rest-princ.fig > rest-princ5.fig
rest-princ6.fig: rest-princ.fig
	figlayers 20-24       45-54 99 < rest-princ.fig > rest-princ6.fig
rest-princ7.fig: rest-princ.fig
	figlayers 25-29       45-54 99 < rest-princ.fig > rest-princ7.fig

sos-arch11.fig: sos-arch.fig
	figlayers             45-54 60-64 99 < sos-arch.fig > sos-arch11.fig
sos-arch12.fig: sos-arch.fig
	figlayers 10          45-54 60-64 99 < sos-arch.fig > sos-arch12.fig
sos-arch13.fig: sos-arch.fig
	figlayers 10 11       45-54 60-64 99 < sos-arch.fig > sos-arch13.fig

sos-arch21.fig: sos-arch.fig
	figlayers                   60-64 99 < sos-arch.fig > sos-arch21.fig
sos-arch22.fig: sos-arch.fig
	figlayers 20-20             60-64 99 < sos-arch.fig > sos-arch22.fig
sos-arch23.fig: sos-arch.fig
	figlayers 20-21             60-64 99 < sos-arch.fig > sos-arch23.fig
sos-arch24.fig: sos-arch.fig
	figlayers 20-22             60-64 99 < sos-arch.fig > sos-arch24.fig
sos-arch25.fig: sos-arch.fig
	figlayers 20-23             60-64 99 < sos-arch.fig > sos-arch25.fig
sos-arch26.fig: sos-arch.fig
	figlayers 20-24             60-64 99 < sos-arch.fig > sos-arch26.fig
sos-arch27.fig: sos-arch.fig
	figlayers 20-25             60-64 99 < sos-arch.fig > sos-arch27.fig

clean:
	rm -f *.aux *.log *.nav *.out *.snm *.vrb *.toc *.pdf *.bak
	rm -f $(XFIG) $(GENFIGS)
