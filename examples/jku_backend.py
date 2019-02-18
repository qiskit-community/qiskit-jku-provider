# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""
Example use of the JKU backend
"""

import os,sys
from qiskit_jku_provider import JKUProvider
from qiskit import execute, QuantumCircuit

RUNDIR = os.path.dirname(os.path.realpath(sys.argv[0]))

def use_jku_backend():
    q_circuit = QuantumCircuit.from_qasm_file(RUNDIR  + '/ghz.qasm')
    jku_backend = JKUProvider().get_backend('local_statevector_simulator_jku')
    result = execute(q_circuit, backend=jku_backend, shots=100, seed=42).result()
    print("counts: ")
    print(result.get_counts(q_circuit))

if __name__ == "__main__":
    use_jku_backend()