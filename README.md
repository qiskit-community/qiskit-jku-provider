# Qiskit JKU simulator provider

[![License](https://img.shields.io/github/license/Qiskit/qiskit-jku-provider.svg?style=popout-square)](https://opensource.org/licenses/Apache-2.0)[![Build Status](https://img.shields.io/travis/com/Qiskit/qiskit-jku-provider/master.svg?style=popout-square)](https://travis-ci.com/Qiskit/qiskit-jku-provider)[![](https://img.shields.io/github/release/Qiskit/qiskit-jku-provider.svg?style=popout-square)](https://github.com/Qiskit/qiskit-jku-provider/releases)[![](https://img.shields.io/pypi/dm/qiskit-jku-provider.svg?style=popout-square)](https://pypi.org/project/qiskit-jku-provider/)


This module contains a [Qiskit](https://www.qiskit.org/) provider for the JKU simulator.

The purpose of the JKU simulator is to simulate quantum circuits on a classical computer. 
In the JKU simulator this is done using a specialized decision-diagrams data structure for improving runtime and memory consumption
in situations where the simulated quantum state contains redundancies.  

The [JKU simulator](http://iic.jku.at/eda/research/quantum_simulation/) was written by Alwin Zulehner and Robert Wille from Johannes Kepler University in Linz.

This provider allows users of Qiskit to use the JKU simulator as a backend. 

## Installation

Install this module via the PIP tool:

```
pip install qiskit
pip install qiskit_jku_provider
```

PIP will handle all dependencies automatically. The latest version of the JKU simulator itself will also be installed.

## Usage

After installing both **qiskit** and **qiskit-jku-provider**, the simulator can be used with the following example 

```python
from qiskit import QuantumCircuit, QuantumRegister, ClassicalRegister, execute
from qiskit_jku_provider import JKUProvider

JKU = JKUProvider()

qr = QuantumRegister(2)
cr = ClassicalRegister(2)

qc = QuantumCircuit(qr, cr)
qc.h(qr[0])
qc.cx(qr[0], qr[1])
qc.measure(qr, cr)

jku_backend = JKU.get_backend('local_statevector_simulator_jku')

job = execute(qc, backend=jku_backend)

result = job.result()

print(result.get_counts(qc))
```
    
## License

This project uses the [Apache License Version 2.0 software license](https://www.apache.org/licenses/LICENSE-2.0).
