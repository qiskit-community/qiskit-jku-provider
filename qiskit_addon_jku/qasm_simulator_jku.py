# -*- coding: utf-8 -*-
# pylint: disable=unused-import

# Copyright 2017 IBM RESEARCH. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =============================================================================

"""Backend for the JKU C++ simulator."""


import time
import itertools
import operator
import random
import uuid
import logging
import warnings
import platform
import os
import json
import re
import subprocess
from collections import OrderedDict, Counter
import numpy as np

from qiskit.backends import BaseBackend
from qiskit.backends.local.localjob import LocalJob
from qiskit.backends.local._simulatorerror import SimulatorError
from qiskit.qobj import qobj_to_dict
from qiskit.result._utils import result_from_old_style_dict


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
gate ccx a,b,c {h c; cx b,c; tdg c; cx a,c; t c; cx b,c; tdg c; cx a,c; t b; t c; h c; cx a,b; t a; tdg b; cx a,b;}
gate cswap a,b,c {cx c,b; ccx a,b,c; cx c,b;}
gate crz(lambda) a,b {u1(lambda/2) b; cx a,b; u1(-lambda/2) b; cx a,b;}
gate cu1(lambda) a,b {u1(lambda/2) a; cx a,b; u1(-lambda/2) b; cx a,b; u1(lambda/2) b;}
gate cu3(theta,phi,lambda) c, t {u1((lambda-phi)/2) t; cx c,t; u3(-theta/2,0,-(phi+lambda)/2) t; cx c,t; u3(theta/2,phi,0) t;}
gate rzz(theta) a,b {cx a,b; u1(theta) b; cx a,b;}\n"""

#this class handles the actual technical details of converting to and from QISKit style data
class JKUSimulatorWrapper:
    """Converter to and from JKU's simulator"""
    def __init__(self, exe=None, silent = False):
        self.seed = 0
        self.shots = 1
        self.exec = exe
        self.additional_output_data = []
        self.silent = silent

    def set_config(self, config):
        if 'data' in config: #additional output data specifications
            self.additional_output_data = config['data']
        if 'seed' in config and config['seed'] is not None:
            self.seed = config['seed']
        else:
            self.seed = random.getrandbits(32)
            
    def run(self, filename):
        """performs the actual external call to the JKU exe"""
        cmd = [self.exec,
               '--simulate_qasm', filename,
               '--seed', str(self.seed),
               '--shots', str(self.shots),
               '--display_statevector',
              ]
        if 'probabilities' in self.additional_output_data:
            cmd.append('--display_probabilities')
        #print("running command {}".format(" ".join(cmd)))
        if not self.silent:
            print(RUN_MESSAGE)
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode()
        #print("DONE running command {}".format(" ".join(cmd)))
        return output

    #convert one operation from the qobj file to a QASM line in the format JKU can handle
    def convert_operation_to_line(self, op, qubit_names, clbit_names):
        name_string = op['name']
        qubits_string = ", ".join([qubit_names[i] for i in op["qubits"]])
        if 'params' in op and len(op['params']) > 0:
            params_string = "({})".format(", ".join([str(p) for p in op['params']]))
        else:
            params_string = ""
        if (name_string == "measure"): #special syntax
            return "measure {} -> {};".format(qubit_names[op["qubits"][0]], clbit_names[op["clbits"][0]])
        if (name_string == "snapshot"): #some QISKit bug causes snapshot params to be passed as floats
            params_string = "({})".format(", ".join([str(int(p)) for p in op['params']]))
        return "{}{} {};".format(name_string, params_string, qubits_string)
    
    #converts the full qobj circuit (except measurements) to a QASM file JKU can handle
    def convert_qobj_circuit_to_jku_qasm(self, qobj_circuit):
        circuit = qobj_circuit['compiled_circuit']
        qubit_num = circuit['header']['number_of_qubits']
        clbit_num = circuit['header']['number_of_clbits']
        #arbitrary qubit names, to use only in the temp qasm file we pass to JKU's simulator
        qubit_names = ['q[{}]'.format(i) for i in range(qubit_num)]
        clbit_names = ['c[{}]'.format(i) for i in range(clbit_num)]
        qasm_file_lines = []
        qasm_file_lines.append("OPENQASM 2.0;")
        qasm_file_lines.append("include \"qelib1.inc\";")
        qasm_file_lines.append("qreg q[{}];".format(qubit_num))
        qasm_file_lines.append("creg c[{}];".format(qubit_num))
        for op in circuit['operations']:
             qasm_file_lines.append(self.convert_operation_to_line(op, qubit_names, clbit_names))
        qasm_content = "\n".join(qasm_file_lines) + "\n"
        return qasm_content

    #convert the qobj circuit to QASM and save as temp file
    def save_circuit_file(self, filename, qobj_circuit):
        qasm = self.convert_qobj_circuit_to_jku_qasm(qobj_circuit)
        with open(filename, "w") as qasm_file:
            qasm_file.write(qasm)

    #runs the qobj circuit on the JKU exe while performing input/output conversions
    def run_on_qobj_circuit(self, qobj_circuit):
        self.set_config(qobj_circuit['config'])
        #do this before running so we can output warning to the user as soon as possible if needed
        measurement_data = self.compute_measurement_data(qobj_circuit)
        filename = "temp.qasm"
        self.save_circuit_file(filename, qobj_circuit)
        with open("qelib1.inc", "w") as qelib_file:
            qelib_file.write(qelib1)
        self.start_time = time.time()
        run_output = self.run(filename)
        self.end_time = time.time()
        os.remove("temp.qasm")
        os.remove("qelib1.inc")
        output_data = self.parse_output(run_output, measurement_data)
        result_dict = {'status': 'DONE', 'time_taken': self.end_time - self.start_time,
                       'seed': self.seed, 'shots': self.shots,
                       'data': output_data,
                       'success': True}
        if 'name' in qobj_circuit:
            result_dict['name'] = qobj_circuit['name']
        return result_dict

    #parsing the textual JKU output
    def parse_output(self, run_output, measurement_data):
        result = json.loads(run_output)
        qubits = measurement_data['qubits_num']
        translation_table = [0] * 2**qubits #QISKit qubit order is reversed, so we fix accordingly
        for n in range(2**qubits):
            translation_table[n] = int(bin(n)[2:].rjust(qubits,'0')[::-1],2)
        if 'counts' in result:
            result['counts'] = self.convert_counts(result['counts'], measurement_data)
        if 'snapshots' in result:
            for snapshot_key, snapshot_data in result['snapshots'].items():
                result['snapshots'][snapshot_key] = self.convert_snapshot(snapshot_data, translation_table)
        return result
    
    def convert_snapshot(self, snapshot_data, translation_table):
        if 'statevector' in snapshot_data:
            snapshot_data['statevector'] = self.convert_statevector_data(snapshot_data['statevector'], translation_table)
        if 'probabilities' in snapshot_data:
            probs_data = snapshot_data.pop('probabilities')
            if 'probabilities' in self.additional_output_data:
                snapshot_data['probabilities'] = self.convert_probabilities(probs_data, translation_table)
        if 'probabilities_ket' in snapshot_data:
            probs_ket_data = snapshot_data.pop('probabilities_ket')
            if 'probabilities_ket' in self.additional_output_data:
                snapshot_data['probabilities_ket'] = self.convert_probabilities_ket(probs_ket_data)
        return snapshot_data
            
    def convert_statevector_data(self, statevector, translation_table):
        return [complex(statevector[translation_table[i]].replace('i','j')) for i in range(len(translation_table))]
    
    def convert_probabilities(self, probs_data, translation_table):
        return [probs_data[translation_table[i]] for i in range(len(translation_table))]
    
    def convert_probabilities_ket(self, probs_ket_data):
        return dict([(key[::-1], value) for key,value in probs_ket_data.items()])

    def convert_counts(self, counts, measurement_data):
        result = {}
        for qubits, count in counts.items():
            clbits = self.qubits_to_clbits(qubits, measurement_data)[::-1]
            result[clbits] = result.get(clbits, 0) + count
        return result

    #converting the actual measurement results for all qubits to clbits the user expects to see
    def qubits_to_clbits(self, qubits, measurement_data):
        clbits = list('0'*measurement_data['clbits_num'])
        for (qubit, clbit) in measurement_data['mapping'].items():
            clbits[clbit] = qubits[qubit]
        s = "".join(clbits)
        #QISKit expects clbits for different registers to be space-separated, i.e. '01 100'
        clbits_lengths = [x[1] for x in measurement_data['clbits']]
        return " ".join(self.slice_by_lengths(s, clbits_lengths))

    def slice_by_lengths(self, arr, lengths):
        i = 0
        res = []
        for s in lengths:
            res.append(arr[i:i+s])
            i = i + s
        return res

    #finding the data relevant to measurements and clbits in the qobj_circuit
    def compute_measurement_data(self, qobj_circuit):
        #Ignore (and inform the user) any in-circuit measurements
        #Create a mapping of qubit --> classical bit for any end-circuit measurement
        header = qobj_circuit['compiled_circuit']['header']
        measurement_data = {'mapping': {},
                            'clbits': header['clbit_labels'],
                            'clbits_num': header['number_of_clbits'],
                            'qubits_num': header['number_of_qubits']}
        ops = qobj_circuit['compiled_circuit']['operations']
        for op in ops:
            if op["name"] == "measure":
                measurement_data['mapping'][op['qubits'][0]] = op['clbits'][0]
            else:
                if op['qubits'][0] in measurement_data['mapping'].keys() and not op["name"] == 'snapshot':
                    raise RuntimeError("Error: qubit {} was used after being measured. This is currently not supported by JKU".format(op['qubits'][0]))
        return measurement_data

logger = logging.getLogger(__name__)

EXTENSION = '.exe' if platform.system() == 'Windows' else ''

# Add path to compiled qasm simulator
DEFAULT_SIMULATOR_PATHS = [
    # This is the path where Makefile creates the simulator by default
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 '../build/jku_simulator'
                                 + EXTENSION)),
    # This is the path where PIP installs the simulator
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 'jku_simulator' + EXTENSION)),
]

class QasmSimulatorJKU(BaseBackend):
    """Python interface to JKU's simulator"""

    DEFAULT_CONFIGURATION = {
        'name': 'local_statevector_simulator_jku',
        'url': 'http://iic.jku.at/eda/research/quantum_simulation/',
        'simulator': True,
        'local': True,
        'description': 'JKU C++ simulator',
        'coupling_map': 'all-to-all',
        "basis_gates": 'u0,u1,u2,u3,cx,x,y,z,h,s,t,snapshot'
    }

    def __init__(self, configuration=None, silent = False):
        """
        Args:
            configuration (dict): backend configuration
        Raises:
             ImportError: if the JKU simulator is not available.
        """
        super().__init__(configuration or self.DEFAULT_CONFIGURATION.copy())
        if self._configuration.get('exe'):
            paths = [self._configuration.get('exe')]
        else:
            paths = DEFAULT_SIMULATOR_PATHS
            # Ensure that the executable is available.
        try:
            self._configuration['exe'] = next(
                path for path in paths if (os.path.exists(path) and
                                           os.path.getsize(path) > 100))
        except StopIteration:
            raise FileNotFoundError('Simulator executable not found (using %s)' %
                                    self._configuration.get('exe', 'default locations'))

        self.silent = silent

    def run(self, qobj):
        local_job = LocalJob(self._run_job, qobj)
        local_job.submit()
        return local_job

    def _run_job(self, qobj):
        """Run circuits in q_job"""
        result_list = []
        self._validate(qobj)

        qobj_old_format = qobj_to_dict(qobj, version='0.0.1')

        s = JKUSimulatorWrapper(self._configuration['exe'], silent = self.silent)
        #self._shots = qobj['config']['shots']
        s.shots = qobj_old_format['config']['shots']
        start = time.time()
        for circuit in qobj_old_format['circuits']:
            result_list.append(s.run_on_qobj_circuit(circuit))
        end = time.time()
        job_id = str(uuid.uuid4())
        result = {'backend': self._configuration['name'],
                  'id': qobj_old_format['id'],
                  'job_id': job_id,
                  'result': result_list,
                  'status': 'COMPLETED',
                  'success': True,
                  'time_taken': (end - start)}
        return result_from_old_style_dict(
            result,
            [circuit.header.name for circuit in qobj.experiments])

    def _validate(self, qobj):
        #for now, JKU should be ran with shots = 1 and no measurement gates
        #hence, we do not check for those cases as in the default qasm simulator
        return


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
            new_key.insert(0, key[-(index+nbits):-index])
        fcounts[' '.join(new_key)] = value
    return fcounts
