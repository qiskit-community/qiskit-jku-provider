# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""
Example use of the JKU backend
"""

import os
from qiskit_jku_provider import JKUProvider
from qiskit.extensions.simulator import snapshot

from qiskit import ClassicalRegister, QuantumRegister, QuantumCircuit, execute

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
    jku_backend = JKUProvider().get_backend('local_statevector_simulator_jku', )
    result = execute(qc, backend=jku_backend, shots=1, config=config, seed=42).result()
    print(result)

if __name__ == "__main__":
    use_jku_backend()
