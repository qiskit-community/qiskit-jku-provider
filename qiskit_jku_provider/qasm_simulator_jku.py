# -*- coding: utf-8 -*-

# Copyright 2018, IBM.
#
# This source code is licensed under the Apache License, Version 2.0 found in
# the LICENSE.txt file in the root directory of this source tree.

"""Backend for the JKU C++ simulator."""

import itertools
import json
import logging
import operator
import os
import platform
import random
import subprocess
import time
import uuid
import tempfile

from qiskit.providers import BaseBackend
from qiskit.providers.models import BackendConfiguration, BackendStatus
from qiskit.result import Result

from .jkujob import JKUJob
from .jkusimulatorerror import JKUSimulatorError

RUN_MESSAGE = """DD-based simulator by JKU Linz, Austria
Developer: Alwin Zulehner, Robert Wille
For more information, please visit http://iic.jku.at/eda/research/quantum_simulation"""

qelib1 = """gate u3(theta,phi,lambda) q { U(theta,phi,lambda) q; }
gate u2(phi,lambda) q { U(pi/2,phi,lambda) q; }
gate u1(lambda) q { U(0,0,lambda) q; }
gate cx c,t { CX c,t; }
gate id a { U(0,0,0) a; }
gate u0(gamma) q { U(0,0,0) q; }
gate x a { u3(pi,0,pi) a; }
gate y a { u3(pi,pi/2,pi/2) a; }
gate z a { u1(pi) a; }
gate h a { u2(0,pi) a; }
gate s a { u1(pi/2) a; }
gate sdg a { u1(-pi/2) a; }
gate t a { u1(pi/4) a; }
gate tdg a { u1(-pi/4) a; }
gate rx(theta) a { u3(theta, -pi/2,pi/2) a; }
gate ry(theta) a { u3(theta,0,0) a; }
gate rz(phi) a { u1(phi) a; }
gate cz a,b { h b; cx a,b; h b; }
gate cy a,b { sdg b; cx a,b; s b; }
gate swap a,b { cx a,b; cx b,a; cx a,b; }
gate ch a,b {h b; sdg b; cx a,b; h b; t b; cx a,b; t b; h b; s b; x b; s a;}
gate ccx a,b,c {h c; cx b,c; tdg c; cx a,c; t c; cx b,c; tdg c; cx a,c; t b; t c; h c; cx a,b; \
t a; tdg b; cx a,b;}
gate cswap a,b,c {cx c,b; ccx a,b,c; cx c,b;}
gate crz(lambda) a,b {u1(lambda/2) b; cx a,b; u1(-lambda/2) b; cx a,b;}
gate cu1(lambda) a,b {u1(lambda/2) a; cx a,b; u1(-lambda/2) b; cx a,b; u1(lambda/2) b;}
gate cu3(theta,phi,lambda) c,t {u1((lambda-phi)/2) t; cx c,t; u3(-theta/2,0,-(phi+lambda)/2) t; \
cx c,t; u3(theta/2,phi,0) t;}
gate rzz(theta) a,b {cx a,b; u1(theta) b; cx a,b;}\n"""


# this class handles the actual technical details of converting to and from QISKit style data
class JKUSimulatorWrapper:
    """Converter to and from JKU's simulator"""

    def __init__(self, exe=None, silent=False):
        self.seed = 0
        self.shots = 1
        self.exec = exe
        self.additional_output_data = []
        self.silent = silent

    def set_config(self, global_config, experiment_config):
        if hasattr(experiment_config, 'data'):  # additional output data specifications
            self.additional_output_data = experiment_config.data
        if hasattr(global_config, 'seed'):
            self.seed = global_config.seed
        else:
            self.seed = random.getrandbits(32)

    def run(self, qasm):
        """performs the actual external call to the JKU exe"""

        cmd = [self.exec,
               '--simulate_qasm',
               '--seed={}'.format(self.seed),
               '--shots={}'.format(self.shots),
               '--display_statevector',
               ]
        if 'probabilities' in self.additional_output_data:
            cmd.append('--display_probabilities')
        if not self.silent:
            print(RUN_MESSAGE)
        output = subprocess.check_output(cmd, input=qasm, stderr=subprocess.STDOUT,
                                         universal_newlines=True)
        return output

    def convert_operation_to_line(self, op, qubit_names, clbit_names):
        """convert one operation from the qobj file to a QASM line in the format JKU can handle"""
        name_string = op.name
        qubits_string = ", ".join([qubit_names[i] for i in op.qubits])
        if hasattr(op, 'params') and len(op.params) > 0:
            params_string = "({})".format(", ".join([str(p) for p in op.params]))
        else:
            params_string = ""
        if name_string == "measure":  # special syntax
            return "measure {} -> {};".format(qubit_names[op.qubits[0]], clbit_names[op.memory[0]])
        # some QISKit bug causes snapshot params to be passed as floats
        if name_string == "snapshot":
            params_string = "({})".format(", ".join([str(int(p)) for p in op.params]))
            for p in op.params:
                self.max_snapshot_index = max(self.max_snapshot_index, int(p))
        return "{}{} {};".format(name_string, params_string, qubits_string)

    def add_final_snapshot(self, qasm_file_lines, qubit_names):
        """Adds a final snapshot at the end of the file, e.g. to get the final statevector"""
        self.max_snapshot_index = self.max_snapshot_index + 1
        qubits = ", ".join(qubit_names)
        qasm_file_lines.append("snapshot({}) {};".format(self.max_snapshot_index, qubits))

    # converts the full qobj circuit (except measurements) to a QASM file JKU can handle
    def convert_qobj_circuit_to_jku_qasm(self, experiment, qelib_inc_name = "qelib1.inc"):
        instructions = experiment.instructions
        qubit_num = len(experiment.header.qubit_labels)
        clbit_num = len(experiment.header.clbit_labels)
        # arbitrary qubit names, to use only in the temp qasm file we pass to JKU's simulator
        qubit_names = ['q[{}]'.format(i) for i in range(qubit_num)]
        clbit_names = ['c[{}]'.format(i) for i in range(clbit_num)]
        qasm_file_lines = ["OPENQASM 2.0;",
                           "include \"{}\";".format(qelib_inc_name),
                           "qreg q[{}];".format(qubit_num),
                           "creg c[{}];".format(qubit_num)
                           ]
        self.max_snapshot_index = 0
        for op in instructions:
            qasm_file_lines.append(self.convert_operation_to_line(op, qubit_names, clbit_names))
        self.add_final_snapshot(qasm_file_lines, qubit_names)
        qasm_content = "\n".join(qasm_file_lines) + "\n"
        return qasm_content

    def final_statevector(self, run_output):
        """
        Retrieves the statevector at the end of the computation
        """
        return run_output["snapshots"][str(self.max_snapshot_index)]["statevector"]

    # runs the qobj circuit on the JKU exe while performing input/output conversions
    def run_experiment(self, config, experiment):
        self.set_config(config, experiment.config)
        # do this before running so we can output warning to the user as soon as possible if needed
        measurement_data = self.compute_measurement_data(experiment)
        
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as qelib_file:
            qelib_inc_name = qelib_file.name
            qelib_file.write(qelib1)
            
        self.start_time = time.time()
        qasm = self.convert_qobj_circuit_to_jku_qasm(experiment, qelib_inc_name)
        run_output = self.run(qasm)
        self.end_time = time.time()
        if os.path.exists(qelib_inc_name):
            os.unlink(qelib_inc_name)
        output_data = self.parse_output(run_output, measurement_data)
        result_dict = {'header': {'name': experiment.header.name,
                                  'memory_slots': experiment.config.memory_slots,
                                  'creg_sizes': experiment.header.creg_sizes
                                  },
                       'status': 'DONE', 'time_taken': self.end_time - self.start_time,
                       'seed': self.seed, 'shots': self.shots,
                       'data': output_data,
                       'success': True
                       }
        return result_dict

    # parsing the textual JKU output
    def parse_output(self, run_output, measurement_data):
        result = json.loads(run_output)
        qubits = measurement_data['qubits_num']
        # QISKit qubit order is reversed, so we fix accordingly
        translation_table = [0] * 2 ** qubits
        for n in range(2 ** qubits):
            translation_table[n] = int(bin(n)[2:].rjust(qubits, '0')[::-1], 2)
        if 'counts' in result:
            result['counts'] = self.convert_counts(result['counts'], measurement_data)
        if 'snapshots' in result:
            for snapshot_key, snapshot_data in result['snapshots'].items():
                result['snapshots'][snapshot_key] = self.convert_snapshot(snapshot_data,
                                                                          translation_table)
        result['statevector'] = self.final_statevector(result)
        return result

    def convert_snapshot(self, snapshot_data, translation_table):
        if 'statevector' in snapshot_data:
            snapshot_data['statevector'] = self.convert_statevector_data(
                snapshot_data['statevector'], translation_table)
        if 'probabilities' in snapshot_data:
            probs_data = snapshot_data.pop('probabilities')
            if 'probabilities' in self.additional_output_data:
                snapshot_data['probabilities'] = self.convert_probabilities(probs_data,
                                                                            translation_table)
        if 'probabilities_ket' in snapshot_data:
            probs_ket_data = snapshot_data.pop('probabilities_ket')
            if 'probabilities_ket' in self.additional_output_data:
                snapshot_data['probabilities_ket'] = self.convert_probabilities_ket(probs_ket_data)
        return snapshot_data

    def to_qiskit_complex(self, num_string):
        num = complex(num_string.replace('i', 'j'))  # first obtain an actual number
        return [num.real, num.imag]

    def convert_statevector_data(self, statevector, translation_table):
        return [self.to_qiskit_complex(statevector[translation_table[i]])
                for i in range(len(translation_table))]

    def convert_probabilities(self, probs_data, translation_table):
        return [probs_data[translation_table[i]] for i in range(len(translation_table))]

    def convert_probabilities_ket(self, probs_ket_data):
        return dict([(key[::-1], value) for key, value in probs_ket_data.items()])

    def convert_counts(self, counts, measurement_data):
        result = {}
        for qubits, count in counts.items():
            clbits = self.qubits_to_clbits(qubits, measurement_data)
            if clbits is not None:
                result[clbits] = result.get(clbits, 0) + count
        return result

    # converting the actual measurement results for all qubits to clbits the user expects to see
    def qubits_to_clbits(self, qubits, measurement_data):
        clbits = list('0' * measurement_data['clbits_num'])
        for (qubit, clbit) in measurement_data['mapping'].items():
            clbits[clbit] = qubits[qubit]
        s = "".join(clbits)[::-1]
        if s == '':
            return None
        return hex(int(s, 2))

    # finding the data relevant to measurements and clbits in the qobj_circuit
    def compute_measurement_data(self, experiment):
        # Ignore (and inform the user) any in-circuit measurements
        # Create a mapping of qubit --> classical bit for any end-circuit measurement
        header = experiment.header
        measurement_data = {'mapping': {},
                            'clbits': header.creg_sizes,
                            'clbits_num': len(header.clbit_labels),
                            'qubits_num': len(header.qubit_labels)}
        ops = experiment.instructions
        for op in ops:
            if op.name == "measure":
                measurement_data['mapping'][op.qubits[0]] = op.memory[0]
            else:
                for qubit in op.qubits:
                    if qubit in measurement_data['mapping'].keys() and not op.name == 'snapshot':
                        raise JKUSimulatorError(
                            "Error: qubit {} was used after being measured. This is currently "
                            "not supported by JKU".format(qubit))
        return measurement_data


logger = logging.getLogger(__name__)

EXTENSION = '.exe' if platform.system() == 'Windows' else ''

# Add path to compiled qasm simulator
DEFAULT_SIMULATOR_PATHS = [
    # This is the path where Makefile creates the simulator by default
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 '../build/lib/qiskit_jku_provider/jku_simulator'
                                 + EXTENSION)),
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 '../build/jku_simulator'
                                 + EXTENSION)),
    # This is the path where PIP installs the simulator
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 'jku_simulator' + EXTENSION)),
]

VERSION_PATHS = [
    os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "VERSION.txt")),
    os.path.abspath(os.path.join(os.path.dirname(__file__), "VERSION.txt")),
]

for version_path in VERSION_PATHS:
    if os.path.exists(version_path):
        with open(version_path, "r") as version_file:
            VERSION = version_file.read().strip()


class QasmSimulator(BaseBackend):
    """Python interface to JKU's simulator"""

    DEFAULT_CONFIGURATION = {
        'backend_name': 'qasm_simulator',
        'backend_version': VERSION,
        'url': 'https://github.com/Qiskit/qiskit-jku-provider',
        'simulator': True,
        'local': True,
        'description': 'JKU C++ simulator',
        'basis_gates': ['u0', 'u1', 'u2', 'u3', 'cx', 'x', 'y', 'z', 'h', 's', 't', 'snapshot'],
        'memory': False,
        'n_qubits': 30,
        'conditional': False,
        'max_shots': 100000,
        'coupling_map': None,
        'open_pulse': False,
        'gates': [
            {
                'name': 'TODO',
                'parameters': [],
                'qasm_def': 'TODO'
            }
        ]
    }

    def __init__(self, configuration=None, provider=None, silent=False):
        """
        Args:
            configuration (dict): backend configuration
        Raises:
             ImportError: if the JKU simulator is not available.
        """
        super().__init__(configuration=(configuration or
                                        BackendConfiguration.from_dict(self.DEFAULT_CONFIGURATION)),
                         provider=provider)

        paths = DEFAULT_SIMULATOR_PATHS
        # Ensure that the executable is available.
        try:
            self.executable = next(
                path for path in paths if (os.path.exists(path) and
                                           os.path.getsize(path) > 100))
        except StopIteration:
            print(paths)
            raise FileNotFoundError('Simulator executable not found')

        self.silent = silent

    def run(self, qobj):
        job_id = str(uuid.uuid4())
        local_job = JKUJob(self, job_id, self._run_job, qobj)
        local_job.submit()
        return local_job

    def _run_job(self, job_id, qobj):
        """Run circuits in q_job"""
        result_list = []
        self._validate(qobj)

        s = JKUSimulatorWrapper(self.executable, silent=self.silent)
        s.shots = qobj.config.shots
        start = time.time()
        for experiment in qobj.experiments:
            result_list.append(s.run_experiment(qobj.config, experiment))
        end = time.time()
        result = {'backend_name': self._configuration.backend_name,
                  'backend_version': VERSION,
                  'qobj_id': qobj.qobj_id,
                  'job_id': job_id,
                  'results': result_list,
                  'status': 'COMPLETED',
                  'success': True,
                  'time_taken': (end - start)}
        return Result.from_dict(result)

    def _validate(self, qobj):
        # for now, JKU should be ran with shots = 1 and no measurement gates
        # hence, we do not check for those cases as in the default qasm simulator
        return

    def status(self):
        """Return backend status.

        Returns:
            BackendStatus: the status of the backend.
        """
        return BackendStatus(backend_name=self.name(),
                             backend_version=self.configuration().backend_version,
                             operational=True,
                             pending_jobs=0,
                             status_msg='')


def _get_register_specs(bit_labels):
    """
    Get the number and size of unique registers from bit_labels list with an
    iterator of register_name:size pairs.

    Args:
        bit_labels (list): this list is of the form::

            [['reg1', 0], ['reg1', 1], ['reg2', 0]]

            which indicates a register named "reg1" of size 2
            and a register named "reg2" of size 1. This is the
            format of classic and quantum bit labels in qobj
            header.
    Yields:
        tuple: pairs of (register_name, size)
    """
    iterator = itertools.groupby(bit_labels, operator.itemgetter(0))
    for register_name, sub_it in iterator:
        yield register_name, max(ind[1] for ind in sub_it) + 1


def _format_result(counts, cl_reg_index, cl_reg_nbits):
    """Format the result bit string.

    This formats the result bit strings such that spaces are inserted
    at register divisions.

    Args:
        counts (dict): dictionary of counts e.g. {'1111': 1000, '0000':5}
        cl_reg_index (list): starting bit index of classical register
        cl_reg_nbits (list): total amount of bits in classical register
    Returns:
        dict: spaces inserted into dictionary keys at register boundaries.
    """
    fcounts = {}
    for key, value in counts.items():
        new_key = [key[-cl_reg_nbits[0]:]]
        for index, nbits in zip(cl_reg_index[1:],
                                cl_reg_nbits[1:]):
            new_key.insert(0, key[-(index + nbits):-index])
        fcounts[' '.join(new_key)] = value
    return fcounts
