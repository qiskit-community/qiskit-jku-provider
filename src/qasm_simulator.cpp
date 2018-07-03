/*
 * qasm_simulator.cpp
 *
 *  Created on: Jul 3, 2018
 *      Author: zulehner
 */

#include "qasm_simulator.h"

QasmSimulator::QasmSimulator(std::string filename) {
	in = new std::ifstream (filename, std::ifstream::in);
	this->scanner = new QASM_scanner(*this->in);
	this->fname = filename;
}

QasmSimulator::QasmSimulator() {
	std::stringstream* in = new std::stringstream();
	(*in) << std::cin.rdbuf();
	this->in = in;
	this->scanner = new QASM_scanner(*this->in);
	this->fname = "";
}

QasmSimulator::~QasmSimulator() {
	delete scanner;
	delete in;
}

void QasmSimulator::scan() {
	t = la;
	la = scanner->next();
	sym = la.kind;
}

void QasmSimulator::check(Token::Kind expected) {
	if (sym == expected) {
		scan();
	} else {
		std::cerr << "ERROR while parsing QASM file" << std::endl;
		std::cerr << "expected";
	}
}

int QasmSimulator::QASMargument() {
	check(Token::Kind::identifier);
	std::string s = t.str;
	if(sym == Token::Kind::lbrack) {
		scan();
		check(Token::Kind::nninteger);
		int offset = t.val;
		check(Token::Kind::rbrack);
		return qregs[s].first+offset;
	}
	if(qregs[s].second != 1) {
		std::cerr << "Argument has to be a single qubit" << std::endl;
	}
	return qregs[s].first;
}

mpreal QasmSimulator::QASMexponentiation() {
	mpreal x;

	if(sym == Token::Kind::real) {
		scan();
		return mpreal(t.val_real);
	} else if(sym == Token::Kind::nninteger) {
		scan();
		return mpreal(t.val);
	} else if(sym == Token::Kind::pi) {
		scan();
		return mpfr::const_pi();
	} else if(sym == Token::Kind::lpar) {
		scan();
		x = QASMexp();
		check(Token::Kind::rpar);
		return x;
	} else if(unaryops.find(sym) != unaryops.end()) {
		Token::Kind op = sym;
		scan();
		check(Token::Kind::lpar);
		x = QASMexp();
		check(Token::Kind::rpar);
		if(op == Token::Kind::sin) {
			return sin(x);
		} else if(op == Token::Kind::cos) {
			return cos(x);
		} else if(op == Token::Kind::tan) {
			return tan(x);
		} else if(op == Token::Kind::exp) {
			return exp(x);
		} else if(op == Token::Kind::ln) {
			return log(x);
		} else if(op == Token::Kind::sqrt) {
			return sqrt(x);
		}
	} else {
		std::cerr << "Invalid Expression" << std::endl;
	}
	return 0;
}

mpreal QasmSimulator::QASMfactor() {
	mpreal x,y;
	x = QASMexponentiation();
	while (sym == Token::Kind::power) {
		scan();
		y = QASMexponentiation();
		x = pow(x, y);
	}

	return x;
}

mpreal QasmSimulator::QASMterm() {
	mpreal x = QASMfactor();
	mpreal y;

	while(sym == Token::Kind::times || sym == Token::Kind::div) {
		Token::Kind op = sym;
		scan();
		y = QASMfactor();
		if(op == Token::Kind::times) {
			x *= y;
		} else {
			x /= y;
		}
	}
	return x;
}

mpreal QasmSimulator::QASMexp() {
	mpreal x;
	if(sym == Token::Kind::minus) {
		scan();
		x = -QASMterm();
	} else {
		x = QASMterm();
	}

	while(sym == Token::Kind::plus || sym == Token::Kind::minus) {
		Token::Kind op = sym;
		scan();
		mpreal y = QASMterm();
		if(op == Token::Kind::plus) {
			x += y;
		} else {
			x -= y;
		}
	}
	return x;
}

QMDDedge QasmSimulator::gateQASM() {
	if(sym == Token::Kind::ugate) {
		scan();
		check(Token::Kind::lpar);
		mpreal theta = QASMexp();
		check(Token::Kind::comma);
		mpreal phi = QASMexp();
		check(Token::Kind::comma);
		mpreal lambda = QASMexp();
		check(Token::Kind::rpar);
		int target = QASMargument();
		check(Token::Kind::semicolon);

		tmp_matrix[0][0] = Cmake(cos(-(phi+lambda)/2)*cos(theta/2), sin(-(phi+lambda)/2)*cos(theta/2));
		tmp_matrix[0][1] = Cmake(-cos(-(phi-lambda)/2)*sin(theta/2), -sin(-(phi-lambda)/2)*sin(theta/2));
		tmp_matrix[1][0] = Cmake(cos((phi-lambda)/2)*sin(theta/2), sin((phi-lambda)/2)*sin(theta/2));
		tmp_matrix[1][1] = Cmake(cos((phi+lambda)/2)*cos(theta/2), sin((phi+lambda)/2)*cos(theta/2));

		line[nqubits-1-target] = 2;
		QMDDedge f = QMDDmvlgate(tmp_matrix, nqubits, line);


		line[nqubits-1-target] = -1;
		return f;
	} else if(sym == Token::Kind::cxgate) {
		scan();
		int control = QASMargument();
		check(Token::Kind::comma);
		int target = QASMargument();
		check(Token::Kind::semicolon);
		line[nqubits-1-control] = 1;
		line[nqubits-1-target] = 2;
		QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
		line[nqubits-1-control] = -1;
		line[nqubits-1-target] = -1;
		return f;
	} else if(sym == Token::Kind::identifier) {
		//TODO: implement custom gates
	}
	return QMDDzero;
}

void QasmSimulator::Reset() {
	Simulator::Reset();
	qregs.clear();
	cregs.clear();
	delete scanner;
	in->clear();
	in->seekg(0, in->beg);
	this->scanner = new QASM_scanner(*this->in);
}

void QasmSimulator::Simulate(int shots) {
	if(shots < 1) {
		std::cerr << "Shots have to be greater than 0!" << std::endl;
	}

	std::map<std::string, int> result;

	Simulate();
	if(!intermediate_measurement) {
		for(int i = 0; i < shots; i++) {
			MeasureAll(false);
			std::stringstream s;
			for(int i=circ.n-1;i >=0; i--) {
				s << measurements[i];
			}
			if(result.find(s.str()) != result.end()) {
				result[s.str()]++;
			} else {
				result[s.str()] = 1;
			}
		}
	} else {
		MeasureAll(false);
		std::stringstream s;
		for(int i=circ.n-1;i >=0; i--) {
			s << measurements[i];
		}
		result[s.str()] = 1;
		for(int i = 1; i < shots; i++) {
			Reset();
			Simulate();
			MeasureAll(false);
			std::stringstream s;
			for(int j=circ.n-1;j >=0; j--) {
				s << measurements[j];
			}
			if(result.find(s.str()) != result.end()) {
				result[s.str()]++;
			} else {
				result[s.str()] = 1;
			}
		}
	}

	std::cout << "{" << std::endl << "'counts': {" << std::endl;
	auto it = result.begin();
	std::cout << "  '" << it->first << "': " << it->second;
	for(it++; it != result.end(); it++) {
		std::cout << ",\n  '" << it->first << "': " << it->second;
	}
	std::cout << "}\n}" << std::endl;
}

void QasmSimulator::Simulate() {

	scan();
	check(Token::Kind::openqasm);
	check(Token::Kind::real);
	check(Token::Kind::semicolon);

	QMDDedge f, tmp;

	do {
		if(sym == Token::Kind::qreg) {

			scan();
			check(Token::Kind::identifier);
			std::string s = t.str;
			check(Token::Kind::lbrack);
			check(Token::Kind::nninteger);
			int n = t.val;
			check(Token::Kind::rbrack);
			check(Token::Kind::semicolon);
			//check whether it already exists

			qregs[s] = std::make_pair(nqubits, n);
			AddVariables(n, s);
		} else if(sym == Token::Kind::creg) {
			scan();
			check(Token::Kind::identifier);
			std::string s = t.str;
			check(Token::Kind::lbrack);
			check(Token::Kind::nninteger);
			int n = t.val;
			check(Token::Kind::rbrack);
			check(Token::Kind::semicolon);
			//TODO: implement
		} else if(sym == Token::Kind::ugate || sym == Token::Kind::cxgate || sym == Token::Kind::identifier) {
			f = gateQASM();

			ApplyGate(f);
		} else if(sym == Token::Kind::probabilities) {
			std::cout << "Probabilities of the states |";
			for(int i=nqubits-1; i>=0; i--) {
				std::cout << circ.line[i].variable << " ";
			}
			std::cout << ">:" << std::endl;
			for(int i=0; i<(1<<nqubits);i++) {
				uint64_t res = GetElementOfVector(circ.e,i);
				std::cout << "  |";
				for(int j=nqubits-1; j >= 0; j--) {
					std::cout << ((i >> j) & 1);
				}
				std::cout << ">: " << Cmag[res & 0x7FFFFFFF7FFFFFFFull]*Cmag[res & 0x7FFFFFFF7FFFFFFFull];
				std::cout << std::endl;
			}
			scan();
			check(Token::Kind::semicolon);
		} else {
            std::cerr << "ERROR: unexpected statement!" << std::endl;
		}
	} while (sym != Token::Kind::eof);
}
