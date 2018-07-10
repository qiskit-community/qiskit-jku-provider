# Copyright 2018 IBM RESEARCH. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =============================================================================
.PHONY: style lint test profile

# Ignoring generated ones with .py extension.
lint:
	pylint -rn qiskit_addon_jku test_jku

style:
	pycodestyle --max-line-length=100 qiskit_addon_jku test_jku

# Use the -s (starting directory) flag for "unittest discover" is necessary,
# otherwise the QuantumCircuit header will be modified during the discovery.
test:
	python3 -m unittest discover -s test_jku -v

profile:
	python3 -m unittest discover -p "profile*.py" -v

