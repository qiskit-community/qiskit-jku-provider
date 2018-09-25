# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""
Example use of the JKU backend
"""

import os,sys
from qiskit.backends.jku import JKUProvider

from qiskit import execute, load_qasm_file
from qiskit.wrapper._wrapper import _DEFAULT_PROVIDER

_DEFAULT_PROVIDER.add_provider(JKUProvider())
RUNDIR = os.path.dirname(os.path.realpath(sys.argv[0]))

def use_jku_backend():
    q_circuit = load_qasm_file(RUNDIR  + '/ghz.qasm')
   
    result = execute(q_circuit, backend='local_statevector_simulator_jku', shots=100).result()
    print("counts: ")
    print(result.get_counts(q_circuit))

if __name__ == "__main__":
    use_jku_backend()
