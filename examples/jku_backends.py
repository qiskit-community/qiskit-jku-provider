# -*- coding: utf-8 -*-

# Copyright 2019, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""Usage examples for the JKU Provider"""

from qiskit import ClassicalRegister, QuantumRegister, QuantumCircuit, execute
from qiskit_jku_provider import JKUProvider

JKU = JKUProvider()

jku_backend = JKU.get_backend('qasm_simulator')

print(jku_backend)

# gets the name of the backend.
print(jku_backend.name())

# gets the status of the backend.
print(jku_backend.status())

# returns the provider of the backend
print(jku_backend.provider())

# gets the configuration of the backend.
print(jku_backend.configuration())

# gets the properties of the backend.
print(jku_backend.properties())

# Demonstration of Job
qr = QuantumRegister(2)
cr = ClassicalRegister(2)

qc = QuantumCircuit(qr, cr)
qc.h(qr[0])
qc.cx(qr[0], qr[1])
qc.measure(qr, cr)

job = execute(qc, backend=jku_backend)

# gets the backend the job was run on
backend = job.backend()
print(backend)

# returns the status of the job. Should be running
print(job.status())

# returns the obj that was run
qobj = job.qobj()
print(qobj)

# prints the job id
print(job.job_id())

# cancels the job
print(job.cancel())

# returns the status of the job. Should be canceled
print(job.status())

# runs the qjob on the backend
job2 = backend.run(qobj)

# gets the result of the job. This is a blocker
print(job2.result())
