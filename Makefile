# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.
SHELL := /bin/bash
.PHONY: sim style lint test profile dist

sim:
	cmake . -Bbuild -DSTATIC_LINKING=True
	make -C build

lint:
	pylint -rn qiskit_jku_provider test

style:
	pycodestyle --max-line-length=100 qiskit_jku_provider test

test:
	python3 -m unittest discover

profile:
	python3 -m unittest discover -p "profile*.py" -v

dist:
	for env in py37 py36 py35 py34 ; do \
		source activate $$env ; \
		python setup.py bdist_wheel ; \
	done
