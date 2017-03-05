COVERAGE=coverage
PREFIX=/usr/local
export PREFIX
PYTHON=python

all: build

build:
	$(PYTHON) setup.py build

check: test

clean:
	-$(PYTHON) setup.py clean --all
	find . -not \( -path '*/.hg/*' -o -path '*/.git/*' \) \
		\( -name '*.py[cdo]' -o -name '*.err' -o \
		-name '*,cover' -o -name __pycache__ \) -prune \
		-exec rm -rf '{}' ';'
	rm -rf dist build htmlcov
	rm -f MANIFEST .coverage cram.xml

dist:
	TAR_OPTIONS="--owner=root --group=root --mode=u+w,go-w,a+rX-s" \
		$(PYTHON) setup.py -q sdist

install: build
	$(PYTHON) setup.py install --prefix="$(PREFIX)" --force

quicktest:
	PYTHON=$(PYTHON) PYTHONPATH=`pwd` scripts/cram $(TESTOPTS) tests

test:
	$(COVERAGE) erase
	COVERAGE=$(COVERAGE) PYTHON=$(PYTHON) PYTHONPATH=`pwd` scripts/cram \
		$(TESTOPTS) tests
	$(COVERAGE) report --fail-under=100

.PHONY: all build check clean install dist quicktest test
