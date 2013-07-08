.SUFFIXES:	.fig .pdf
FILE = talk

XFIG =	memory0.pdf memory1.pdf arduino0.pdf arduino1.pdf arduino2.pdf

GENFIGS = fig/memory0.fig fig/memory1.fig \
	fig/arduino0.fig fig/arduino1.fig fig/arduino2.fig

IMG = img/arduino_atmel_mc.jpg \
	img/arduino_gnd_5v_3v3.jpg \
	img/arduino.jpg \
	img/arduino_leds.jpg \
	img/arduino_pins.jpg \
	img/arduino_power_connector.jpg \
	img/arduino_reset_button.jpg \
	img/arduino_usb.jpg \
	img/components_breadboard.jpg \
	img/components.jpg \
	img/components_leds.jpg \
	img/components_light_sensor.jpg \
	img/components_potentiometers.jpg \
	img/components_pushbuttons.jpg \
	img/components_resistors.jpg \
	img/components_wires.jpg \
	img/drone.JPG \
	img/due.jpg \
	img/icube.png \
	img/ide_examples.png \
	img/ide_serial.png \
	img/ide_webserver_buttons.png \
	img/ide_webserver.png \
	img/leds.jpeg \
	img/lilypad.JPG \
	img/rover.jpg \
	img/stacked-shields.jpg \
	img/TinyDuino.jpg

FIGS = $(GENFIGS) $(XFIG) $(IMG)
TEX = content/arduino.tex \
	content/components.tex \
	content/usage.tex \
	content/implementation.tex

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
