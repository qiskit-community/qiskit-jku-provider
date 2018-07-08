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
from qiskit._result import Result
from qiskit.backends import BaseBackend
from qiskit.backends.local.localjob import LocalJob
from qiskit.backends.local._simulatorerror import SimulatorError

#for sampling shots from the result distribution
#elements_dict should not contain zero-prob entries
#if the elements probabilities sum to less than 1, the last entry is enlarged
#if the elements probabilities sum to more than 1, anything above 1 is skipped
def choose_weighted_random(elements_dict):
    r = random.random()
    total_prob = 0
    for (e,p) in elements_dict.items():
        total_prob += p
        if r <= total_prob:
            return e
    return elements_dict.items()[-1]    
    
#this class handles the actual technical details of converting to and from QISKit style data
class JKUSimulatorWrapper:
    def __init__(self, exe = None):
        self.seed = 0
        self.shots = 1
        self.EXEC = exe
        
    #performs the actual external call to the JKU exe
    def run(self, filename):        
        cmd = [self.EXEC, '--simulate_qasm', filename, '--seed', str(self.seed), '--shots', str(self.shots)]
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode() #TODO: improve, use pipes as in qasm
        return output
       
    #convert one operation from the qobj file to a QASM line in the format JKU can handle
    def convert_operation_to_line(self, op, qubit_names):
        pi = str(np.pi)
        half_pi = str(np.pi / 2)
        gate_names = {'u': 'U', 
                      'u1': 'U', 'u2': 'U', 'u3': 'U',
                      'h': 'U', 'cx': 'CX', 'x': 'U',
                      'y': 'U', 'z': 'U', 's': 'U'}
        gates_to_skip = ['barrier']
        params = op['params'] + [0]*(3-len(op['params'])) if 'params' in op else [0,0,0]
        gate_params = {'u': [params[0], params[1], params[2]],
                       'u1': [0, 0, params[0]],
                       'u2': [half_pi, params[0], params[1]],
                       'u3': [params[0], params[1], params[2]],
                       'h': [half_pi, '0', pi], 
                       'x': [pi, '0', pi], 
                       'y': [pi, half_pi, half_pi], 
                       'z': ['0', '0', pi], 
                       's': ['0', '0', half_pi]
                       }
        gate_name = op['name'].lower()
        if gate_name in gates_to_skip:
            return ""
        if not gate_name in gate_names:
            raise RuntimeError("Error: gate {} is currently not supported by JKU".format(op["name"]))
        new_gate_name = gate_names[gate_name]
        if new_gate_name == 'U':
            new_gate_name = "U({},{},{})".format(*gate_params[gate_name])
        new_gate_inputs = ", ".join([qubit_names[i] for i in op["qubits"]])
        return "{} {};".format(new_gate_name, new_gate_inputs)

    #converts the full qobj circuit (except measurements) to a QASM file JKU can handle
    def convert_qobj_circuit_to_jku_qasm(self, qobj_circuit):
        circuit = qobj_circuit['compiled_circuit']
        qubit_num = circuit['header']['number_of_qubits']
        #arbitrary qubit names, to use only in the temp qasm file we pass to JKU's simulator
        qubit_names = ['q[{}]'.format(i) for i in range(qubit_num)]
        qasm_file_lines = []
        qasm_file_lines.append("OPENQASM 2.0;")
        qasm_file_lines.append("qreg q[{}];".format(qubit_num))
        qasm_file_lines.append("creg s[{}];".format(qubit_num))
        for op in circuit['operations']:
            if op["name"] != "measure":
                qasm_file_lines.append(self.convert_operation_to_line(op, qubit_names))
        qasm_file_lines.append("show_probabilities;\n")
        qasm_content = "\n".join(qasm_file_lines)
        return qasm_content
       
    #convert the qobj circuit to QASM and save as temp file
    def save_circuit_file(self, filename, qobj_circuit):
        qasm = self.convert_qobj_circuit_to_jku_qasm(qobj_circuit)
        with open(filename, "w") as qasm_file:
            qasm_file.write(qasm)

    #runs the qobj circuit on the JKU exe while performing input/output conversions
    def run_on_qobj_circuit(self, qobj_circuit):
        #do this before running so we can output warning to the user as soon as possible if needed
        measurement_data = self.compute_measurement_data(qobj_circuit) 
        filename = "temp.qasm" #TODO: better name
        self.save_circuit_file(filename, qobj_circuit)
        self.start_time = time.time()
        run_output = self.run(filename)
        self.end_time = time.time()
        result = self.parse_output(run_output, measurement_data)
        result_dict = {'status': 'DONE', 'time_taken': self.end_time - self.start_time, 
                       'seed': self.seed, 'shots': self.shots, 
                       'data': {'counts': result['counts']}}
        if 'name' in qobj_circuit:
            result_dict['name'] = qobj_circuit['name']
        return result_dict
    
    #parsing the textual JKU output
    def parse_output(self, run_output, measurement_data):
        #JKU probabilities output is of the form "|0010>: 0.4" for the probabilities, so we grab that with a regexp
        probs = dict(filter(lambda x_p: x_p[1] > 0, 
                map(lambda x_p: (x_p[0], float(x_p[1])), 
                re.findall('\|(\d+)>: (\d+\.?\d*e?-?\d*)', run_output))))
        result = {}
        result['counts'] = self.parse_counts(run_output, measurement_data)
        return result
    
    def parse_counts(self, run_output, measurement_data):
        count_regex = re.compile("'counts': ({[^}]*})", re.DOTALL)
        counts_string = re.search(count_regex, run_output).group(1).replace('\r', '').replace('\n','').replace("'", '"')
        counts = json.loads(counts_string)
        result = {}
        for qubits, count in counts.items():
            clbits = self.qubits_to_clbits(qubits, measurement_data)[::-1]
            result[clbits] = result.get(clbits, 0) + count
        return result
        
        
    def simulate_shots(self, probs, measurement_data):
        sample = self.sample_from_probs(probs, self.shots)
        result = {}
        for qubits, count in sample.items():
            clbits = self.qubits_to_clbits(qubits, measurement_data)[::-1]
            result[clbits] = result.get(clbits, 0) + count
        return result
    
    #converting the actual measurement results for all qubits to the clbits that the user expects to see
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
    
    #given a distribution <probs>, draw <shots> random elements from it
    def sample_from_probs(self, probs, shots):
        result = {}
        for shot in range(shots):
            res = choose_weighted_random(probs)
            if res is None:
                continue #note: this means there are "lost" shots; this should not happen in practice since probs sum to 1
            if not res in result:
                result[res] = 0
            result[res] += 1
        return result        
    
    #finding the data relevant to measurements and clbits in the qobj_circuit
    def compute_measurement_data(self, qobj_circuit):
        #Ignore (and inform the user) any in-circuit measurements
        #Create a mapping of qubit --> classical bit for any end-circuit measurement
        measurement_data = {'mapping': {}, 'clbits': qobj_circuit['compiled_circuit']['header']['clbit_labels'], 'clbits_num': qobj_circuit['compiled_circuit']['header']['number_of_clbits']} #mapping is qubit --> clbit
        ops = qobj_circuit['compiled_circuit']['operations']
        for op in ops:
            if op["name"] == "measure":
                measurement_data['mapping'][op['qubits'][0]] = op['clbits'][0]
            else:
                if op['qubits'][0] in measurement_data['mapping'].keys():
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
        'name': 'local_qasm_simulator_jku',
        'url': 'http://iic.jku.at/eda/research/quantum_simulation/',
        'simulator': True,
        'local': True,
        'description': 'JKU C++ simulator',
        'coupling_map': 'all-to-all',
        "basis_gates": 'u0,u1,u2,u3,cx,x,y,z,h,s,t'
    }

    def __init__(self, configuration=None):
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
        
    def run(self, q_job):
        return LocalJob(self._run_job, q_job)

    def _run_job(self, q_job):
        """Run circuits in q_job"""
        result_list = []
        qobj = q_job.qobj
        self._validate(qobj)
        s = JKUSimulatorWrapper(self._configuration['exe'])
        if 'seed' in qobj['config']:
            s.seed = qobj['config']['seed']
        else:
            s.seed = random.getrandbits(32)
        #self._shots = qobj['config']['shots']
        s.shots = qobj['config']['shots']
        start = time.time()
        for circuit in qobj['circuits']:
            result_list.append(s.run_on_qobj_circuit(circuit))
        end = time.time()
        job_id = str(uuid.uuid4())
        result = {'backend': self._configuration['name'],
                  'id': qobj['id'],
                  'job_id': job_id,
                  'result': result_list,
                  'status': 'COMPLETED',
                  'success': True,
                  'time_taken': (end - start)}
        return Result(result, qobj)

    def _validate(self, qobj):
        if qobj['config']['shots'] == 1:
            warnings.warn('The behavior of getting statevector from simulators '
                          'by setting shots=1 is deprecated and will be removed. '
                          'Use the local_statevector_simulator instead, or place '
                          'explicit snapshot instructions.',
                          DeprecationWarning)
        for circ in qobj['circuits']:
            if 'measure' not in [op['name'] for
                                 op in circ['compiled_circuit']['operations']]:
                logger.warning("no measurements in circuit '%s', "
                               "classical register will remain all zeros.", circ['name'])
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
