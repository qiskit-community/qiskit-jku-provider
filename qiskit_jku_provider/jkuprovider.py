# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""Provider for the local JKU backend."""

import logging

from qiskit.providers import BaseProvider
from .qasm_simulator_jku import QasmSimulator

logger = logging.getLogger(__name__)


class JKUProvider(BaseProvider):
    """Provider for the local JKU backend."""

    def __init__(self, *args, **kwargs):
        super().__init__(args, kwargs)

        # Populate the list of local JKU backends.
        self.backends_list = {'qasm_simulator': QasmSimulator(provider=self)}

    def get_backend(self, name, **kwargs):
        return self.backends_list[name]

    def available_backends(self, filters=None):
        # pylint: disable=arguments-differ
        backends = self.backends_list

        filters = filters or {}
        for key, value in filters.items():
            backends = {name: instance for name, instance in backends.items()
                        if instance.configuration().get(key) == value}

        return list(backends.values())

    def backends(self, name=None, **kwargs):
        return list(self.backends_list.values())

    def __str__(self):
        return 'JKUProvider'
