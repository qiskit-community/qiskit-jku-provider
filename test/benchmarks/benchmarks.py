# -*- coding: utf-8 -*-

# Copyright 2019, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

# pylint: disable=missing-docstring,redefined-builtin

import os

from qiskit              import QuantumCircuit
from qiskit              import execute
from qiskit_jku_provider import QasmSimulator




class QasmSimulatorJKUBenchmarkSuite:
    """Runs the Basic qasm_simulator tests from Terra on JKU."""
    params = ["grover_12.qasm", "sym9_193.qasm", "plus63mod4096_163.qasm", "z4_268.qasm", "clip_206.qasm"]
    timeout = 1800
    
    def setup(self, filename):
        self.seed         = 88
        self.backend      = QasmSimulator(silent=True)
        full_file         = os.path.join(os.path.dirname(__file__), 'qasms', filename)
        self.circuit      = QuantumCircuit.from_qasm_file(full_file)
        self.circuit.name = filename
           
    def time_qasm_simulator_single_shot(self, parameter):
        """Test single shot run."""
        result = execute(self.circuit, self.backend, seed_transpiler=34342, shots=1).result()
        assert(result.success)

    def time_qasm_simulator(self, parameter):
        """Test data counts output for single circuit run against reference."""
        shots = 1024
        result = execute(self.circuit, self.backend, seed_transpiler=34342, shots=shots).result()
        assert(result.success)