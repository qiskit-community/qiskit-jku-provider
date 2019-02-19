
# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""
Example use of the JKU Provider and the Qasm Simulator backend for creating the 
state '01 10'.
"""
from qiskit import ClassicalRegister, QuantumRegister, QuantumCircuit, execute
from qiskit_jku_provider import JKUProvider

JKU = JKUProvider()

qreg0 = QuantumRegister(2, 'q0')
creg0 = ClassicalRegister(2, 'c0')
qreg1 = QuantumRegister(2, 'q1')
creg1 = ClassicalRegister(2, 'c1')
circ = QuantumCircuit(qreg0, qreg1)
circ.x(qreg0[1])
circ.x(qreg1[0])

meas = QuantumCircuit(qreg0, qreg1, creg0, creg1)
meas.measure(qreg0, creg0)
meas.measure(qreg1, creg1)

qc = circ + meas

backend_sim = JKU.get_backend('local_statevector_simulator_jku')
job = execute(qc, backend_sim)
result = job.result()
counts = result.get_counts(qc)
print(counts)
