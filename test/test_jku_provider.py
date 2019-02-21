# -*- coding: utf-8 -*-

# Copyright 2019, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""Test JKU backend."""

from qiskit_jku_provider import JKUProvider

from .common import QiskitTestCase


class JKUProviderTestCase(QiskitTestCase):
    """Tests for the JKU Provider."""

    def setUp(self):
        self.provider = JKUProvider()
        self.backend_name = 'qasm_simulator'

    def test_backends(self):
        """Test the provider has backends."""
        backends = self.provider.backends()
        self.assertTrue(len(backends) > 0)

    def test_get_backend(self):
        """Test getting a backend from the provider."""
        backend = self.provider.get_backend(name=self.backend_name)
        self.assertEqual(backend.name(), self.backend_name)
