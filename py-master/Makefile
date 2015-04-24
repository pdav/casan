PEP = pep8 --ignore E211
PYLINT = pylint --disable=C0326,I \
	--variable-rgx=[a-z_][a-z0-9_]{0,30}$$ \
	--argument-rgx=[a-z_][a-z0-9_]{0,30}$$

check:
	$(PEP) *.py

pylint:
	$(PYLINT) *.py

clean:
	rm -rf __pycache__
