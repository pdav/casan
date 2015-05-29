PEP = pep8 --ignore E211
PYLINT = pylint --disable=C0326,I \
	--variable-rgx=[a-z_][a-z0-9_]{0,30}$$ \
	--argument-rgx=[a-z_][a-z0-9_]{0,30}$$

all: README.pdf

check:
	$(PEP) *.py

pylint:
	$(PYLINT) *.py

README.pdf: README.md
	pandoc --number-sections -o README.pdf README.md

clean:
	rm -rf __pycache__
	rm -f README.pdf
