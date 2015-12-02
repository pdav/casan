all: duree enc dec

duree:
	R --save < r_script_duree.r
	mv Rplots.pdf res/duree.pdf

enc:
	R --save < r_script_enc.r
	mv Rplots.pdf res/enc.pdf

dec:
	R --save < r_script_dec.r
	mv Rplots.pdf res/dec.pdf
