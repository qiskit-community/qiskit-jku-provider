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
def choose_weighted_random(elements_dict):
    r = random.random()
    total_prob = 0
    for (e,p) in elements_dict.items():
        total_prob += p
        if r <= total_prob:
            return e
    return None    
    
class JKUSimulatorWrapper:
    def __init__(self, exe = None):
        self.seed = 0
        self.EXEC = exe
        
    def run(self, filename):        
        cmd = [self.EXEC, '--simulate_qasm', filename, '--seed', str(self.seed)]
        print(" ".join(cmd))
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode() #TODO: improve, use pipes as in qasm
        return output
            
    def convert_operation_to_line(self, op, qubit_names):
        pi = str(np.pi)
        half_pi = str(np.pi / 2)
        gate_names = {'U': 'U', 'h': 'U', 'cx': 'CX', 'x': 'U', 'y': 'U', 'z': 'U', 's': 'U'}
        gate_params = {'U': op.get('params'),
                       'h': [half_pi, '0', pi], 
                       'x': [pi, '0', pi], 
                       'y': [pi, half_pi, half_pi], 
                       'z': ['0', '0', pi], 
                       's': ['0', '0', half_pi]
                       }
        gate_name = op.get('name')
        if not gate_name in gate_names:
            #should be error
            warnings.warn("Warning: gate {} is currently not supported by JKU and will be ignored".format(op["name"]))
            return ""
        new_gate_name = gate_names[gate_name]
        if new_gate_name == 'U':
            new_gate_name = "U({},{},{})".format(*gate_params[gate_name])
        new_gate_inputs = ", ".join([qubit_names[i] for i in op["qubits"]])
        return "{} {};".format(new_gate_name, new_gate_inputs)

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
       
    def save_circuit_file(self, filename, qobj_circuit):
        qasm = self.convert_qobj_circuit_to_jku_qasm(qobj_circuit)
        with open(filename, "w") as qasm_file:
            qasm_file.write(qasm)

    def run_on_qobj_circuit(self, qobj_circuit):
        measurement_data = self.compute_measurement_data(qobj_circuit) #do this before running so we can output warning to the user as soon as possible if needed
        filename = "temp.qasm" #TODO: better name
        self.save_circuit_file(filename, qobj_circuit)
        self.start_time = time.time()
        run_output = self.run(filename)
        self.end_time = time.time()
        result = self.parse_output(run_output, measurement_data)
        result_dict = {'status': 'DONE', 'time_taken': self.end_time - self.start_time, 'seed': self.seed, 'shots': self.shots, 'data': {'counts': result['counts']}}
        if 'name' in qobj_circuit:
            result_dict['name'] = qobj_circuit['name']
        return result_dict
    
    def parse_output(self, output, measurement_data):
        #JKU doesn't support shots yet. So our current support is based on getting probabilities and sampling by hand
        probs = dict(filter(lambda x_p: x_p[1] > 0,
                map(lambda x_p: (x_p[0], float(x_p[1])), 
                re.findall('\|(\d+)>: (\d+\.?\d*)', output))))
                
        sample = self.sample_from_probs(probs, self.shots)
        result = {}
        
        result['counts'] = dict(map(lambda x_count: (self.qubits_to_clbits(x_count[0], measurement_data)[::-1], x_count[1]) , sample.items()))
        return result
    
    def qubits_to_clbits(self, qubits, measurement_data):
        clbits = list('0'*len(qubits))
        for (qubit, clbit) in measurement_data.items():
            clbits[clbit] = qubits[qubit]
        return "".join(clbits)
        
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
    
    def compute_measurement_data(self, qobj_circuit):
        #Ignore (and inform the user) any in-circuit measurements
        #Create a mapping of qubit --> classical bit for any end-circuit measurement
        measurement_data = {} #qubit --> clbit
        ops = qobj_circuit['compiled_circuit']['operations'][::-1] #reverse order
        found_non_measure_gate = False
        found_mid_circuit_measurement = False
        for op in ops:
            if op["name"] == "measure":
                if found_non_measure_gate:
                    found_mid_circuit_measurement = True
                    #we should skip this measurement, but right now QISKit optimizes circuits by pushing measurements from the end backward, so we can't ignore this either
                    #continue
                measurement_data[op['qubits'][0]] = op['clbits'][0]
            else:
                found_non_measure_gate = True
        #this warning is meaningless as long as QISKit pushed measurement gates backwards
        #if found_mid_circuit_measurement:
            #warnings.warn("Measurements gates in mid-circuit are currently not supported by the JKU simulator backend and will be ignored")
        return measurement_data
        
            
logger = logging.getLogger(__name__)

EXTENSION = '.exe' if platform.system() == 'Windows' else ''

# Add path to compiled qasm simulator
DEFAULT_SIMULATOR_PATHS = [
    # This is the path where Makefile creates the simulator by default
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 '../out/quantum_simulator'
                                 + EXTENSION)),
    # This is the path where PIP installs the simulator
    os.path.abspath(os.path.join(os.path.dirname(__file__),
                                 'quantum_simulator' + EXTENSION)),
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
        'basis_gates': 'h,s,t,cx,id',
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
        
        #logger.info('JKU C++ simulator unavailable.')
        #raise ImportError('JKU C++ simulator unavailable.')

        # Define the attributes inside __init__.
        #self._number_of_qubits = 0
        #self._number_of_clbits = 0
        #self._statevector = 0
        #self._classical_state = 0
        #self._seed = None
        #self._shots = 0
        #self._sim = None

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
