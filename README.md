# Qiskit JKU simulator provieder

[![License](https://img.shields.io/github/license/Qiskit/qiskit-terra.svg?style=popout-square)](https://opensource.org/licenses/Apache-2.0)

This module contains a [Qiskit](https://www.qiskit.org/) provider for the JKU simulator.

The purpose of the JKU simulator is to simulate quantum circuits on a classical computer. 
In the JKU simulator this is done using the QMDD data structure for improving runtime and memory consumption
in situations where the simulated quantum state contains redundancies.  

The [JKU simulator](http://iic.jku.at/eda/research/quantum_simulation/) was written by Alwin Zulehner and Robert Wille from Johannes Kepler University in Linz.

This provider allows users of Qiskit to use the JKU simulator as a backend. 

## Installation

Install this module via the PIP tool:

```
pip install qiskit_jku_provider
```

PIP will handle all dependencies automatically. The latest version of the JKU simulator itself will also be installed.

## Usage

We recommend to follow the [usage example](examples/jku_backend.py). More general information and education on running quantum simulation can be found in the [Qiskit instructions page](https://github.com/Qiskit/qiskit-terra) and the QISKit tutorials.

After installing both **qiskit-jku-provider** and **qiskit** itself, the simulator can be used in Qiskit code by importing the provider

  ```python
   >>> from qiskit_jku_provider import JKUProvider
  ```

And loading the backend

  ```python
   >>> jku_backend = JKUProvider().get_backend('local_statevector_simulator_jku')
  ```
    
## License

This project uses the [Apache License Version 2.0 software license](https://www.apache.org/licenses/LICENSE-2.0).
