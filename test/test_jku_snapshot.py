# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

# pylint: disable=missing-docstring,broad-except

import unittest
from qiskit import QuantumCircuit, QuantumRegister
from qiskit import execute

from .common import QiskitTestCase
from qiskit_jku_provider import QasmSimulator


class JKUSnapshotTest(QiskitTestCase):
    """Test JKU's statevector return capatbilities."""

    def setUp(self):
        super().setUp()
        self.backend = QasmSimulator(silent=True)
        qr = QuantumRegister(2)
        self.q_circuit = QuantumCircuit(qr)
        self.q_circuit.h(qr[0])
        self.q_circuit.cx(qr[0], qr[1])

    def test_statevector_output(self):
        """Test final state vector for single circuit run."""
        result = execute(self.q_circuit, backend=self.backend).result()
        self.assertEqual(result.success, True)
        actual = result.get_statevector(self.q_circuit)

        # state is 1/sqrt(2)|00> + 1/sqrt(2)|11>, up to a global phase
        self.assertAlmostEqual((abs(actual[0]))**2, 1/2, places=5)
        self.assertEqual(actual[1], 0)
        self.assertEqual(actual[2], 0)
        self.assertAlmostEqual((abs(actual[3]))**2, 1/2, places=5)


if __name__ == '__main__':
    unittest.main()
