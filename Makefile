# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

.PHONY: sim style lint test profile

sim:
	cmake . -Bbuild
	make -C build
# Ignoring generated ones with .py extension.
lint:
	pylint -rn qiskit/backends/jku test_jku

style:
	pycodestyle --max-line-length=100 qiskit/backends/jku test_jku

# Use the -s (starting directory) flag for "unittest discover" is necessary,
# otherwise the QuantumCircuit header will be modified during the discovery.
test:
	python3 -m unittest discover -s test_jku -v

profile:
	python3 -m unittest discover -p "profile*.py" -v

