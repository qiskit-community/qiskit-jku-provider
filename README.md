# Qiskit JKU Simulator Provider

[![License](https://img.shields.io/github/license/Qiskit/qiskit-jku-provider.svg?style=popout-square)](https://opensource.org/licenses/Apache-2.0)[![Build Status](https://img.shields.io/travis/com/Qiskit/qiskit-jku-provider/master.svg?style=popout-square)](https://travis-ci.com/Qiskit/qiskit-jku-provider)[![](https://img.shields.io/github/release/Qiskit/qiskit-jku-provider.svg?style=popout-square)](https://github.com/Qiskit/qiskit-jku-provider/releases)[![](https://img.shields.io/pypi/dm/qiskit-jku-provider.svg?style=popout-square)](https://pypi.org/project/qiskit-jku-provider/)[![asv](http://img.shields.io/badge/benchmarked%20by-asv-blue.svg?style=flat)](http://your-url-here/)

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
jku_backend = JKU.get_backend('qasm_simulator')

# Simulate the circuit with the JKU Simulator
job = execute(qc, backend=jku_backend)

# Retrieve and display the results
result = job.result()
print(result.get_counts(qc))
```

## Installation from SRC

This provider can also be installed from source. The following steps are necessary:

```
python setup.py build

or 

make dist
```

## Testing

There are also unit tests implemented that can be executed automatically with:

```
make test
```

Furthermore, air speed velocity can be used to benchmark the repository.
It can be invoked with:

```
cd test
asv run
```

## Contribution Guidelines

If you'd like to contribute to the JKU simulator, please take a look at our
[contribution guidelines](.github/CONTRIBUTING.md). This project adheres to Qiskit's [code of conduct](.github/CODE_OF_CONDUCT.md). By participating, you are expect to uphold to this code.

We use [GitHub issues](https://github.com/Qiskit/qiskit-jku-provider/issues) for tracking requests and bugs. Please use our [slack](https://qiskit.slack.com) for discussion and simple questions. To join our Slack community use the [link](https://join.slack.com/t/qiskit/shared_invite/enQtNDc2NjUzMjE4Mzc0LTMwZmE0YTM4ZThiNGJmODkzN2Y2NTNlMDIwYWNjYzA2ZmM1YTRlZGQ3OGM0NjcwMjZkZGE0MTA4MGQ1ZTVmYzk). For questions that are more suited for a forum we use the Qiskit tag in the [Stack Exchange](https://quantumcomputing.stackexchange.com/questions/tagged/qiskit).

## Next Steps

Beyond running the JKU simulator, you may want to experiment and compare with other Qiskit simulators. You'll find the repository of different simulators at [Qiskit-Aer](https://github.com/Qiskit/qiskit-aer), or start with the
[Qiskit-Aer tutorials](https://github.com/Qiskit/qiskit-tutorials/tree/master/qiskit/aer). Once you get the feel, you are welcome to contribute and make the JKU simulator even better. Or you can provide your own simulator and let others examine and contribute to it. For this, see [creating a new provider](https://github.com/Qiskit/qiskit-tutorials/blob/master/qiskit/terra/creating_a_provider.ipynb).

## Authors and Citation

The JKU simulator was created by Alwin Zulehner and Robert Wille from the Johannes Kepler University in Linz, Austria. See [JKU Quantum](http://iic.jku.at/eda/research/quantum/) for more information on the team, or the full [TCAD paper](http://iic.jku.at/files/eda/2018_tcad_advanced_simulation_quantum_computations.pdf) ([bibtex](./jku_tcad.bib)) for a presentation of the underlying concepts and algorithm. 

The adaptation of the JKU simulator to Qiskit was done by Gadi Aleksandrowicz. If you use parts of Qiskit, please cite as per the included [BibTeX file](https://github.com/Qiskit/qiskit/blob/master/Qiskit.bib).

## License

[Apache License 2.0](./LICENSE)
