# -*- coding: utf-8 -*-
# pylint: disable=invalid-name,anomalous-backslash-in-string

# Copyright 2017 IBM RESEARCH. All Rights Reserved.
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
"""
Example use of the JKU backend
"""

import os
from qiskit_addon_jku import JKUProvider
from qiskit.extensions.simulator import snapshot

from qiskit import ClassicalRegister, QuantumRegister, QuantumCircuit, execute
from qiskit.wrapper._wrapper import _DEFAULT_PROVIDER

_DEFAULT_PROVIDER.add_provider(JKUProvider())

def use_jku_backend():
    qubits_num = 3
    qr = QuantumRegister(qubits_num)
    cr = ClassicalRegister(qubits_num)
    qc = QuantumCircuit(qr, cr)
    qc.h(qr[0])
    qc.cx(qr[0], qr[1])
    qc.snapshot(0)
    qc.measure(qr[0], cr[0])
    qc.measure(qr[1], cr[1])
    qc.measure(qr[2], cr[2])
    qc.snapshot(1)
    config = {"data": ['probabilities', 'probabilities_ket']}
    result = execute(qc, backend='local_statevector_simulator_jku', shots=1, config=config).result()
    print(result.get_data())

if __name__ == "__main__":
    use_jku_backend()
