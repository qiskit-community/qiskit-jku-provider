//============================================================================
// Name        : qmdd_simulator.cpp
// Author      : 
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <set>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <vector>
#include <gmp.h>
#include <mpreal.h>
#include <queue>
#include <chrono>
#include <QMDDcore.h>
#include <QMDDpackage.h>
#include <QMDDcomplex.h>

#include <simulator.h>
#include <qasm_simulator.h>
#include <QASM_scanner.hpp>

using mpfr::mpreal;

using namespace std;


int main(int argc, char** argv) {

	namespace po = boost::program_options;
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
		("seed", po::value<unsigned long>(), "seed for random number generator")
	    ("simulate_qasm", po::value<string>()->implicit_value(""), "simulate a quantum circuit given in QPENQASM 2.0 format (if no file is given, the circuit is read from stdin)")
		("shots", po::value<unsigned int>(), "number of shots")
		("ps", "print simulation stats (applied gates, sim. time, and maximal size of the DD)")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, description), vm);
	po::notify(vm);

	if (vm.count("help")) {
	    cout << description << "\n";
	    return 1;
	}

	unsigned long seed = time(NULL);
	if (vm.count("seed")) {
		seed = vm["seed"].as<unsigned long>();
	}

	srand(seed);

	QMDDinit(0);

	Simulator* simulator;

	if (vm.count("simulate_qasm")) {
		string fname = vm["simulate_qasm"].as<string>();
		if(fname == "") {
			simulator = new QasmSimulator();
		} else {
			simulator = new QasmSimulator(fname);
		}
	} else {
		cout << description << "\n";
	    return 1;
	}

    auto t1 = chrono::high_resolution_clock::now();

	if(vm.count("shots")) {
		simulator->Simulate(vm["shots"].as<unsigned int>());
	} else {
		simulator->Simulate(1);
	}

	auto t2 = chrono::high_resolution_clock::now();
	chrono::duration<float> diff = t2-t1;

	delete simulator;

	if (vm.count("ps")) {
		cout << endl << "SIMULATION STATS: " << endl;
		cout << "  Number of applied gates: " << simulator->GetGatecount() << endl;
		cout << "  Simulation time: " << diff.count() << " seconds" << endl;
		cout << "  Maximal size of DD (number of nodes) during simulation: " << simulator->GetMaxActive() << endl;
	}

	return 0;
}
