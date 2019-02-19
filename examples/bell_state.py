# -*- coding: utf-8 -*-

# Copyright 2019, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""
Example use of the JKU Provider and the Qasm Simulator backend for creating a 
Bell state
"""
from qiskit import ClassicalRegister, QuantumRegister, QuantumCircuit, execute
from qiskit_jku_provider import JKUProvider

JKU = JKUProvider()

qubits_num = 2
qr = QuantumRegister(qubits_num)
cr = ClassicalRegister(qubits_num)

qc = QuantumCircuit(qr, cr)
qc.h(qr[0])
qc.cx(qr[0], qr[1])
qc.measure(qr, cr)

jku_backend = JKU.get_backend('qasm_simulator')
job = execute(qc, backend=jku_backend, shots=1000, seed=42)
result = job.result()
print(result.get_counts())
