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

	for(auto it = compoundGates.begin(); it != compoundGates.end(); it++) {
		for(auto it2 = it->second.gates.begin(); it2 != it->second.gates.end(); it2++) {
			delete *it2;
		}
	}
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
		std::cerr << "ERROR while parsing QASM file: expected '" << Token::KindNames[expected] << "' but found '" << Token::KindNames[sym] << "' in line " << la.line << ", column " << la.col << std::endl;
	}
}

std::pair<int, int> QasmSimulator::QASMargumentQreg() {
	check(Token::Kind::identifier);
	std::string s = t.str;
	if(qregs.find(s) == qregs.end()) {
		std::cerr << "Argument is not a qreg: " << s << std::endl;
	}

	if(sym == Token::Kind::lbrack) {
		scan();
		check(Token::Kind::nninteger);
		int offset = t.val;
		check(Token::Kind::rbrack);
		return std::make_pair(qregs[s].first+offset, 1);
	}
	return std::make_pair(qregs[s].first, qregs[s].second);
}

std::pair<std::string, int> QasmSimulator::QASMargumentCreg() {
	check(Token::Kind::identifier);
	std::string s = t.str;
	if(cregs.find(s) == cregs.end()) {
		std::cerr << "Argument is not a creg: " << s << std::endl;
	}

	int index = -1;
	if(sym == Token::Kind::lbrack) {
		scan();
		check(Token::Kind::nninteger);
		index = t.val;
		if(index < 0 || index >= cregs[s].first) {
			std::cerr << "Index of creg " << s << " is out of bounds: " << index << std::endl;
		}
		check(Token::Kind::rbrack);
	}

	return std::make_pair(s, index);
}


QasmSimulator::Expr* QasmSimulator::QASMexponentiation() {
	Expr* x;

	if(sym == Token::Kind::real) {
		scan();
		return new Expr(Expr::Kind::number, NULL, NULL, t.val_real, "");
		//return mpreal(t.val_real);
	} else if(sym == Token::Kind::nninteger) {
		scan();
		return new Expr(Expr::Kind::number, NULL, NULL, t.val, "");
		//return mpreal(t.val);
	} else if(sym == Token::Kind::pi) {
		scan();
		return new Expr(Expr::Kind::number, NULL, NULL, mpfr::const_pi(), "");
		//return mpfr::const_pi();
	} else if(sym == Token::Kind::identifier) {
		scan();
		return new Expr(Expr::Kind::id, NULL, NULL, 0, t.str);
		//return it->second;
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
		if(x->kind == Expr::Kind::number) {
			if(op == Token::Kind::sin) {
				x->num = sin(x->num);
			} else if(op == Token::Kind::cos) {
				x->num = cos(x->num);
			} else if(op == Token::Kind::tan) {
				x->num = tan(x->num);
			} else if(op == Token::Kind::exp) {
				x->num = exp(x->num);
			} else if(op == Token::Kind::ln) {
				x->num = log(x->num);
			} else if(op == Token::Kind::sqrt) {
				x->num = sqrt(x->num);
			}
			return x;
		} else {
			if(op == Token::Kind::sin) {
				return new Expr(Expr::Kind::sin, x, NULL, 0, "");
			} else if(op == Token::Kind::cos) {
				return new Expr(Expr::Kind::cos, x, NULL, 0, "");
			} else if(op == Token::Kind::tan) {
				return new Expr(Expr::Kind::tan, x, NULL, 0, "");
			} else if(op == Token::Kind::exp) {
				return new Expr(Expr::Kind::exp, x, NULL, 0, "");
			} else if(op == Token::Kind::ln) {
				return new Expr(Expr::Kind::ln, x, NULL, 0, "");
			} else if(op == Token::Kind::sqrt) {
				return new Expr(Expr::Kind::sqrt, x, NULL, 0, "");
			}
		}
	} else {
		std::cerr << "Invalid Expression" << std::endl;
	}
	return NULL;
}

QasmSimulator::Expr* QasmSimulator::QASMfactor() {
	Expr* x;
	Expr* y;
	x = QASMexponentiation();
	while (sym == Token::Kind::power) {
		scan();
		y = QASMexponentiation();
		if(x->kind == Expr::Kind::number && y->kind == Expr::Kind::number) {
			x->num = pow(x->num, y->num);
			delete y;
		} else {
			x = new Expr(Expr::Kind::power, x, y, 0, "");
		}
	}

	return x;
}

QasmSimulator::Expr* QasmSimulator::QASMterm() {
	QasmSimulator::Expr* x = QASMfactor();
	QasmSimulator::Expr* y;

	while(sym == Token::Kind::times || sym == Token::Kind::div) {
		Token::Kind op = sym;
		scan();
		y = QASMfactor();
		if(op == Token::Kind::times) {
			if(x->kind == Expr::Kind::number && y->kind == Expr::Kind::number) {
				x->num = x->num * y->num;
				delete y;
			} else {
				x = new Expr(Expr::Kind::times, x, y, 0, "");
			}
		} else {
			if(x->kind == Expr::Kind::number && y->kind == Expr::Kind::number) {
				x->num = x->num / y->num;
				delete y;
			} else {
				x = new Expr(Expr::Kind::div, x, y, 0, "");
			}
		}
	}
	return x;
}

QasmSimulator::Expr* QasmSimulator::QASMexp() {
	Expr* x;
	Expr* y;
	if(sym == Token::Kind::minus) {
		scan();
		x = QASMterm();
		if(x->kind == Expr::Kind::number) {
			x->num = -x->num;
		}
	} else {
		x = QASMterm();
	}

	while(sym == Token::Kind::plus || sym == Token::Kind::minus) {
		Token::Kind op = sym;
		scan();
		y = QASMterm();
		if(op == Token::Kind::plus) {
			if(x->kind == Expr::Kind::number && y->kind == Expr::Kind::number) {
				x->num = x->num + y->num;
				delete y;
			} else {
				x = new Expr(Expr::Kind::plus, x, y, 0, "");
			}
		} else {
			if(x->kind == Expr::Kind::number && y->kind == Expr::Kind::number) {
				x->num = x->num - y->num;
				delete y;
			} else {
				x = new Expr(Expr::Kind::minus, x, y, 0, "");
			}
		}
	}
	return x;
}

void QasmSimulator::QASMexpList(std::vector<Expr*>& expressions) {
	Expr* x = QASMexp();
	expressions.push_back(x);
	while(sym == Token::Kind::comma) {
		scan();
		expressions.push_back(QASMexp());
	}
}

void QasmSimulator::QASMargsList(std::vector<std::pair<int, int> >& arguments) {
	arguments.push_back(QASMargumentQreg());
	while(sym == Token::Kind::comma) {
		scan();
		arguments.push_back(QASMargumentQreg());
	}
}

void QasmSimulator::QASMgate() {
	if(sym == Token::Kind::ugate) {
		scan();
		check(Token::Kind::lpar);
		Expr* theta = QASMexp();
		check(Token::Kind::comma);
		Expr* phi = QASMexp();
		check(Token::Kind::comma);
		Expr* lambda = QASMexp();
		check(Token::Kind::rpar);
		std::pair<int, int> target = QASMargumentQreg();
		check(Token::Kind::semicolon);

		for(int i = 0; i < target.second; i++) {
			tmp_matrix[0][0] = Cmake(cos(-(phi->num+lambda->num)/2)*cos(theta->num/2), sin(-(phi->num+lambda->num)/2)*cos(theta->num/2));
			tmp_matrix[0][1] = Cmake(-cos(-(phi->num-lambda->num)/2)*sin(theta->num/2), -sin(-(phi->num-lambda->num)/2)*sin(theta->num/2));
			tmp_matrix[1][0] = Cmake(cos((phi->num-lambda->num)/2)*sin(theta->num/2), sin((phi->num-lambda->num)/2)*sin(theta->num/2));
			tmp_matrix[1][1] = Cmake(cos((phi->num+lambda->num)/2)*cos(theta->num/2), sin((phi->num+lambda->num)/2)*cos(theta->num/2));

			line[nqubits-1-(target.first+i)] = 2;

			QMDDedge f = QMDDmvlgate(tmp_matrix, nqubits, line);
			line[nqubits-1-(target.first+i)] = -1;

			ApplyGate(f);
		}
		delete theta;
		delete phi;
		delete lambda;

#if VERBOSE
		std::cout << "Applied gate: U" << std::endl;
#endif

	} else if(sym == Token::Kind::cxgate) {
		scan();
		std::pair<int, int> control = QASMargumentQreg();
		check(Token::Kind::comma);
		std::pair<int, int> target = QASMargumentQreg();
		check(Token::Kind::semicolon);

		if(control.second == target.second) {
			for(int i = 0; i < target.second; i++) {
				line[nqubits-1-(control.first+i)] = 1;
				line[nqubits-1-(target.first+i)] = 2;
				QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
				line[nqubits-1-(control.first+i)] = -1;
				line[nqubits-1-(target.first+i)] = -1;
				ApplyGate(f);
			}
		} else if(control.second == 1) {
			for(int i = 0; i < target.second; i++) {
				line[nqubits-1-control.first] = 1;
				line[nqubits-1-(target.first+i)] = 2;
				QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
				line[nqubits-1-control.first] = -1;
				line[nqubits-1-(target.first+i)] = -1;
				ApplyGate(f);
			}
		} else if(target.second == 1) {
			for(int i = 0; i < target.second; i++) {
				line[nqubits-1-(control.first+i)] = 1;
				line[nqubits-1-target.first] = 2;
				QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
				line[nqubits-1-(control.first+i)] = -1;
				line[nqubits-1-target.first] = -1;
				ApplyGate(f);
			}
		} else {
			std::cerr << "Register size does not match for CX gate!" << std::endl;
		}
#if VERBOSE
		std::cout << "Applied gate: CX" << std::endl;
#endif

	} else if(sym == Token::Kind::identifier) {
		scan();
		auto gateIt = compoundGates.find(t.str);
		if(gateIt != compoundGates.end()) {
			std::string gate_name = t.str;

			std::vector<Expr*> parameters;
			std::vector<std::pair<int, int> > arguments;
			if(sym == Token::Kind::lpar) {
				scan();
				if(sym != Token::Kind::rpar) {
					QASMexpList(parameters);
				}
				check(Token::Kind::rpar);
			}
			QASMargsList(arguments);
			check(Token::Kind::semicolon);

			std::map<std::string, std::pair<int, int> > argsMap;
			std::map<std::string, Expr*> paramsMap;
			int size = 1;
			int i = 0;
			for(auto it = arguments.begin(); it != arguments.end(); it++) {
				argsMap[gateIt->second.argumentNames[i]] = *it;
				i++;
				if(it->second > 1 && size != 1 && it->second != size) {
					std::cerr << "Register sizes do not match!" << std::endl;
				}
				if(it->second > 1) {
					size = it->second;
				}
			}
			for(int i = 0; i < parameters.size(); i++) {
				paramsMap[gateIt->second.parameterNames[i]] = parameters[i];
			}

			for(auto it = gateIt->second.gates.begin(); it != gateIt->second.gates.end(); it++) {
				if(Ugate* u = dynamic_cast<Ugate*>(*it)) {
					Expr* theta = RewriteExpr(u->theta, paramsMap);
					Expr* phi = RewriteExpr(u->phi, paramsMap);
					Expr* lambda = RewriteExpr(u->lambda, paramsMap);

					for(int i = 0; i < argsMap[u->target].second; i++) {
						tmp_matrix[0][0] = Cmake(cos(-(phi->num+lambda->num)/2)*cos(theta->num/2), sin(-(phi->num+lambda->num)/2)*cos(theta->num/2));
						tmp_matrix[0][1] = Cmake(-cos(-(phi->num-lambda->num)/2)*sin(theta->num/2), -sin(-(phi->num-lambda->num)/2)*sin(theta->num/2));
						tmp_matrix[1][0] = Cmake(cos((phi->num-lambda->num)/2)*sin(theta->num/2), sin((phi->num-lambda->num)/2)*sin(theta->num/2));
						tmp_matrix[1][1] = Cmake(cos((phi->num+lambda->num)/2)*cos(theta->num/2), sin((phi->num+lambda->num)/2)*cos(theta->num/2));

						line[nqubits-1-(argsMap[u->target].first+i)] = 2;
						QMDDedge f = QMDDmvlgate(tmp_matrix, nqubits, line);
						line[nqubits-1-(argsMap[u->target].first+i)] = -1;

						ApplyGate(f);
					}
					delete theta;
					delete phi;
					delete lambda;
				} else if(CXgate* cx = dynamic_cast<CXgate*>(*it)) {
					if(argsMap[cx->control].second == argsMap[cx->target].second) {
						for(int i = 0; i < argsMap[cx->target].second; i++) {
							line[nqubits-1-(argsMap[cx->control].first+i)] = 1;
							line[nqubits-1-(argsMap[cx->target].first+i)] = 2;
							QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
							line[nqubits-1-(argsMap[cx->control].first+i)] = -1;
							line[nqubits-1-(argsMap[cx->target].first+i)] = -1;
							ApplyGate(f);
						}
					} else if(argsMap[cx->control].second == 1) {
						for(int i = 0; i < argsMap[cx->target].second; i++) {
							line[nqubits-1-argsMap[cx->control].first] = 1;
							line[nqubits-1-(argsMap[cx->target].first+i)] = 2;
							QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
							line[nqubits-1-argsMap[cx->control].first] = -1;
							line[nqubits-1-(argsMap[cx->target].first+i)] = -1;
							ApplyGate(f);
						}
					} else if(argsMap[cx->target].second == 1) {
						for(int i = 0; i < argsMap[cx->target].second; i++) {
							line[nqubits-1-(argsMap[cx->control].first+i)] = 1;
							line[nqubits-1-argsMap[cx->target].first] = 2;
							QMDDedge f = QMDDmvlgate(Nm, nqubits, line);
							line[nqubits-1-(argsMap[cx->control].first+i)] = -1;
							line[nqubits-1-argsMap[cx->target].first] = -1;
							ApplyGate(f);
						}
					} else {
						std::cerr << "Register size does not match for CX gate!" << std::endl;
					}

				}
			}
#if VERBOSE
		std::cout << "Applied gate: " << gate_name << std::endl;
#endif

		} else {
			std::cerr << "Undefined gate: " << t.str << std::endl;
		}
	}
}

void QasmSimulator::Reset() {
	Simulator::Reset();
	qregs.clear();

	for(auto it = cregs.begin(); it != cregs.end(); it++) {
		delete it->second.second;
	}
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

void QasmSimulator::QASMidList(std::vector<std::string>& identifiers) {
	check(Token::Kind::identifier);
	identifiers.push_back(t.str);
	while(sym == Token::Kind::comma) {
		scan();
		check(Token::Kind::identifier);
		identifiers.push_back(t.str);
	}
}

QasmSimulator::Expr* QasmSimulator::RewriteExpr(Expr* expr, std::map<std::string, Expr*>& exprMap) {
	if(expr == NULL) {
		return NULL;
	}
	Expr* op1 = RewriteExpr(expr->op1, exprMap);
	Expr* op2 = RewriteExpr(expr->op2, exprMap);

	if(expr->kind == Expr::Kind::number) {
		return new Expr(expr->kind, op1, op2, expr->num, expr->id);
	} else if(expr->kind == Expr::Kind::plus) {
		if(op1->kind == Expr::Kind::number && op2->kind == Expr::Kind::number) {
			op1->num = op1->num + op2->num;
			delete op2;
			return op1;
		}
	} else if(expr->kind == Expr::Kind::minus) {
		if(op1->kind == Expr::Kind::number && op2->kind == Expr::Kind::number) {
			op1->num = op1->num - op2->num;
			delete op2;
			return op1;
		}
	} else if(expr->kind == Expr::Kind::sign) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = -op1->num;
			return op1;
		}
	} else if(expr->kind == Expr::Kind::times) {
		if(op1->kind == Expr::Kind::number && op2->kind == Expr::Kind::number) {
			op1->num = op1->num * op2->num;
			delete op2;
			return op1;
		}
	} else if(expr->kind == Expr::Kind::div) {
		if(op1->kind == Expr::Kind::number && op2->kind == Expr::Kind::number) {
			op1->num = op1->num / op2->num;
			delete op2;
			return op1;
		}
	} else if(expr->kind == Expr::Kind::power) {
		if(op1->kind == Expr::Kind::number && op2->kind == Expr::Kind::number) {
			op1->num = pow(op1->num,op2->num);
			delete op2;
			return op1;
		}
	} else if(expr->kind == Expr::Kind::sin) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = sin(op1->num);
			return op1;
		}
	} else if(expr->kind == Expr::Kind::cos) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = cos(op1->num);
			return op1;
		}
	} else if(expr->kind == Expr::Kind::tan) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = tan(op1->num);
			return op1;
		}
	} else if(expr->kind == Expr::Kind::exp) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = exp(op1->num);
			return op1;
		}
	} else if(expr->kind == Expr::Kind::ln) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = log(op1->num);
			return op1;
		}
	} else if(expr->kind == Expr::Kind::sqrt) {
		if(op1->kind == Expr::Kind::number) {
			op1->num = sqrt(op1->num);
			return op1;
		}
	} else if(expr->kind == Expr::Kind::id) {
		return new Expr(*exprMap[expr->id]);
	}

	return new Expr(expr->kind, op1, op2, expr->num, expr->id);
}

void QasmSimulator::QASMgateDecl() {
	check(Token::Kind::gate);
	check(Token::Kind::identifier);

	CompoundGate gate;
	std::string gateName = t.str;
	if(sym == Token::Kind::lpar) {
		scan();
		if(sym != Token::Kind::rpar) {
			QASMidList(gate.parameterNames);
		}
		check(Token::Kind::rpar);
	}
	QASMidList(gate.argumentNames);
	check(Token::Kind::lbrace);


	while(sym != Token::Kind::rbrace) {
		if(sym == Token::Kind::ugate) {
			scan();
			check(Token::Kind::lpar);
			Expr* theta = QASMexp();
			check(Token::Kind::comma);
			Expr* phi = QASMexp();
			check(Token::Kind::comma);
			Expr* lambda = QASMexp();
			check(Token::Kind::rpar);
			check(Token::Kind::identifier);

			gate.gates.push_back(new Ugate(theta, phi, lambda, t.str));
			check(Token::Kind::semicolon);
		} else if(sym == Token::Kind::cxgate) {
			scan();
			check(Token::Kind::identifier);
			std::string control = t.str;
			check(Token::Kind::comma);
			check(Token::Kind::identifier);
			gate.gates.push_back(new CXgate(control, t.str));
			check(Token::Kind::semicolon);

		} else if(sym == Token::Kind::identifier) {
			scan();
			std::string name = t.str;

			std::vector<Expr* > parameters;
			std::vector<std::string> arguments;
			if(sym == Token::Kind::lpar) {
				scan();
				if(sym != Token::Kind::rpar) {
					QASMexpList(parameters);
				}
				check(Token::Kind::rpar);
			}
			QASMidList(arguments);
			check(Token::Kind::semicolon);

			CompoundGate g = compoundGates[name];
			std::map<std::string, std::string> argsMap;
			for(int i = 0; i < arguments.size(); i++) {
				argsMap[g.argumentNames[i]] = arguments[i];
			}

			std::map<std::string, Expr*> paramsMap;
			for(int i = 0; i < parameters.size(); i++) {
				paramsMap[g.parameterNames[i]] = parameters[i];
			}

			for(auto it = g.gates.begin(); it != g.gates.end(); it++) {
				if(Ugate* u = dynamic_cast<Ugate*>(*it)) {
					gate.gates.push_back(new Ugate(RewriteExpr(u->theta, paramsMap), RewriteExpr(u->phi, paramsMap), RewriteExpr(u->lambda, paramsMap), argsMap[u->target]));
				} else if(CXgate* cx = dynamic_cast<CXgate*>(*it)) {
					gate.gates.push_back(new CXgate(argsMap[cx->control], argsMap[cx->target]));
				} else {
					std::cerr << "Unexpected gate!" << std::endl;
				}
			}

			for(auto it = parameters.begin(); it != parameters.end(); it++) {
				delete *it;
			}
		} else {
			std::cerr << "Error in gate declaration!" << std::endl;
		}
	}

#if VERBOSE & 0
	std::cout << "Declared gate \"" << gateName << "\":" << std::endl;
	for(auto it = gate.gates.begin(); it != gate.gates.end(); it++) {
		if(Ugate* u = dynamic_cast<Ugate*>(*it)) {
			std::cout << "  U(";
			printExpr(u->theta);
			std::cout << ", ";
			printExpr(u->phi);
			std::cout << ", ";
			printExpr(u->lambda);
			std::cout << ") "<< u->target << ";" << std::endl;
		} else if(CXgate* cx = dynamic_cast<CXgate*>(*it)) {
			std::cout << "  CX " << cx->control << ", " << cx->target << ";" << std::endl;
		} else {
			std::cout << "other gate" << std::endl;
		}
	}
#endif

	compoundGates[gateName] = gate;

	check(Token::Kind::rbrace);
}

void QasmSimulator::printExpr(Expr* expr) {
	if(expr->kind == Expr::Kind::number) {
		std::cout << expr->num;
	} else if(expr->kind == Expr::Kind::plus) {
		printExpr(expr->op1);
		std::cout << " + ";
		printExpr(expr->op2);
	} else if(expr->kind == Expr::Kind::minus) {
		printExpr(expr->op1);
		std::cout << " - ";
		printExpr(expr->op2);
	} else if(expr->kind == Expr::Kind::sign) {
		std::cout << "( - ";
		printExpr(expr->op1);
		std::cout << " )";
	} else if(expr->kind == Expr::Kind::times) {
		printExpr(expr->op1);
		std::cout << " * ";
		printExpr(expr->op2);
	} else if(expr->kind == Expr::Kind::div) {
		printExpr(expr->op1);
		std::cout << " / ";
		printExpr(expr->op2);
	} else if(expr->kind == Expr::Kind::power) {
		printExpr(expr->op1);
		std::cout << " ^ ";
		printExpr(expr->op2);
	} else if(expr->kind == Expr::Kind::sin) {
		std::cout << "sin(";
		printExpr(expr->op1);
		std::cout << ")";
	} else if(expr->kind == Expr::Kind::cos) {
		std::cout << "cos(";
		printExpr(expr->op1);
		std::cout << ")";
	} else if(expr->kind == Expr::Kind::tan) {
		std::cout << "tan(";
		printExpr(expr->op1);
		std::cout << ")";
	} else if(expr->kind == Expr::Kind::exp) {
		std::cout << "exp(";
		printExpr(expr->op1);
		std::cout << ")";
	} else if(expr->kind == Expr::Kind::ln) {
		std::cout << "ln(";
		printExpr(expr->op1);
		std::cout << ")";
	} else if(expr->kind == Expr::Kind::sqrt) {
		std::cout << "sqrt(";
		printExpr(expr->op1);
		std::cout << ")";
	} else if(expr->kind == Expr::Kind::id) {
		std::cout << expr->id;
	}
}

void QasmSimulator::Simulate() {

	scan();
	check(Token::Kind::openqasm);
	check(Token::Kind::real);
	check(Token::Kind::semicolon);

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
			int* reg = new int[n];

			//Initialize cregs with 0
			for(int i=0; i<n; i++) {
				reg[i] = 0;
			}
			cregs[s] = std::make_pair(n, reg);

		} else if(sym == Token::Kind::ugate || sym == Token::Kind::cxgate || sym == Token::Kind::identifier) {
			QASMgate();
		} else if(sym == Token::Kind::gate) {
			QASMgateDecl();
		} else if(sym == Token::Kind::include) {
			scan();
			check(Token::Kind::string);
			std::string fname = t.str;
			scanner->addFileInput(fname);
			check(Token::Kind::semicolon);
		} else if(sym == Token::Kind::measure) {
			scan();
			std::pair<int, int> qreg = QASMargumentQreg();

			check(Token::Kind::minus);
			check(Token::Kind::gt);
			std::pair<std::string, int> creg = QASMargumentCreg();
			check(Token::Kind::semicolon);

			int creg_size = (creg.second == -1) ? cregs[creg.first].first : 1;

			if(qreg.second == creg_size) {
				if(creg_size == 1) {
					cregs[creg.first].second[creg.second] = MeasureOne(nqubits-1-(qreg.first));
				} else {
					for(int i = 0; i < creg_size; i++) {
						cregs[creg.first].second[i] = MeasureOne(nqubits-1-(qreg.first+i));
					}
				}
			} else {
				std::cerr << "Mismatch of qreg and creg size in measurement" << std::endl;
			}
			intermediate_measurement = true;

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
            exit(1);
		}
	} while (sym != Token::Kind::eof);
}
