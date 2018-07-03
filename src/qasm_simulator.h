/*
 * qasm_simulator.h
 *
 *  Created on: Jul 3, 2018
 *      Author: zulehner
 */

#ifndef QASM_SIMULATOR_H_
#define QASM_SIMULATOR_H_

#include <simulator.h>
#include <QASM_scanner.hpp>
#include <QASM_token.hpp>

class QasmSimulator : public Simulator {
public:
	QasmSimulator();
	QasmSimulator(std::string fname);
	virtual ~QasmSimulator();

	void Simulate();
	void Simulate(int shots);
	void Reset();

private:
	void scan();
	void check(Token::Kind expected);

	Token la,t;
	Token::Kind sym = Token::Kind::none;

	std::string fname;
  	std::istream* in;
	QASM_scanner* scanner;
	std::map<std::string, std::pair<int ,int> > qregs;
	std::map<std::string, int> cregs;
	int QASMargument();
	mpreal QASMexponentiation();
	mpreal QASMfactor();
	mpreal QASMterm();
	mpreal QASMexp();
	QMDDedge gateQASM();
	std::set<Token::Kind> unaryops {Token::Kind::sin,Token::Kind::cos,Token::Kind::tan,Token::Kind::exp,Token::Kind::ln,Token::Kind::sqrt};

	QMDD_matrix tmp_matrix;

};

#endif /* QASM_SIMULATOR_H_ */
