# Qiskit JKU Simulator Provider

[![License](https://img.shields.io/github/license/Qiskit/qiskit-jku-provider.svg?style=popout-square)](https://opensource.org/licenses/Apache-2.0)[![Build Status](https://img.shields.io/travis/com/Qiskit/qiskit-jku-provider/master.svg?style=popout-square)](https://travis-ci.com/Qiskit/qiskit-jku-provider)[![](https://img.shields.io/github/release/Qiskit/qiskit-jku-provider.svg?style=popout-square)](https://github.com/Qiskit/qiskit-jku-provider/releases)[![](https://img.shields.io/pypi/dm/qiskit-jku-provider.svg?style=popout-square)](https://pypi.org/project/qiskit-jku-provider/)

## Simulate redundant quantum states efficiently

This module contains the *Qiskit JKU Simulator Provider*, which gives access to the [JKU Simulator](http://iic.jku.at/eda/research/quantum_simulation/) as a backend. Together, they form a provider/backend pair in the [Qiskit backend interface framework](https://qiskit.org/documentation/advanced_use_of_ibm_q_devices.html).

The JKU Simulator improves the runtime and memory consumption of classically simulated quantum circuits with redundant quantum states by using specially designed decision-diagram data structures. The JKU simulator was written by Alwin Zulehner and Robert Wille from Johannes Kepler Universität Linz.

## Installation

Install [Qiskit](https://qiskit.org/) and this module with pip, the package installer for Python:

```
pip install qiskit
pip install qiskit-jku-provider
```

pip will handle all dependencies automatically.

## Getting Started

After installing `qiskit` and `qiskit-jku-provider`, the simulator can be used as demonstrated below.

```python
# Import tools
from qiskit import QuantumCircuit, QuantumRegister, ClassicalRegister, execute
from qiskit_jku_provider import JKUProvider

# Create an instance of the JKU Provider
JKU = JKUProvider()

# Create a quantum circuit
qr = QuantumRegister(2)
cr = ClassicalRegister(2)
qc = QuantumCircuit(qr, cr)

# Add quantum gates
qc.h(qr[0])
qc.cx(qr[0], qr[1])
qc.measure(qr, cr)

# Get the JKU backend from the JKU provider
jku_backend = JKU.get_backend('local_statevector_simulator_jku')

# Simulate the circuit with the JKU Simulator
job = execute(qc, backend=jku_backend)

# Retrieve and display the results
result = job.result()
print(result.get_counts(qc))
```

## License

This project uses the [Apache License Version 2.0 software license](https://www.apache.org/licenses/LICENSE-2.0).
