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
#include <stack>

class QasmSimulator : public Simulator {
public:
	QasmSimulator();
	QasmSimulator(std::string fname);
	virtual ~QasmSimulator();

	void Simulate();
	void Simulate(int shots);
	void Reset();

private:
	class Expr {
	public:
		enum class Kind {number, plus, minus, sign, times, sin, cos, tan, exp, ln, sqrt, div, power, id};
		mpreal num;
		Kind kind;
		Expr* op1 = NULL;
		Expr* op2 = NULL;
		std::string id;

		Expr(Kind kind,  Expr* op1, Expr* op2, mpreal num, std::string id) {
			this->kind = kind;
			this->op1 = op1;
			this->op2 = op2;
			this->num = num;
			this->id = id;
		}

		Expr(const Expr& expr) {
			this->kind = expr.kind;
			this->num = expr.num;
			this->id = expr.id;
			if(expr.op1 != NULL) {
				this->op1 = new Expr(*expr.op1);
			}
			if(expr.op2 != NULL) {
				this->op2 = new Expr(*expr.op2);
			}
		}

		~Expr() {
			if(op1 != NULL) {
				delete op1;
			}
			if(op2 != NULL) {
				delete op2;
			}
		}
	};

	class BasisGate {
	public:
		virtual ~BasisGate() = default;
	};

	class Ugate : public BasisGate {
	public:
		Expr* theta;
		Expr* phi;
		Expr* lambda;
		std::string target;

		Ugate(Expr* theta, Expr* phi, Expr* lambda, std::string target) {
			this->theta = theta;
			this->phi = phi;
			this->lambda = lambda;
			this->target = target;
		}

		~Ugate() {
			if(theta != NULL) {
				delete theta;
			}
			if(phi != NULL) {
				delete phi;
			}
			if(lambda != NULL) {
				delete lambda;
			}
		}
	};

	class CXgate : public BasisGate {
	public:
		std::string control;
		std::string target;

		CXgate(std::string control, std::string target) {
			this->control = control;
			this->target = target;
		}
	};

	class CompoundGate {
	public:
		std::vector<std::string> parameterNames;
		std::vector<std::string> argumentNames;
		std::vector<BasisGate*> gates;
		bool opaque;
	};


	void scan();
	void check(Token::Kind expected);

	Token la,t;
	Token::Kind sym = Token::Kind::none;

	std::string fname;
  	std::istream* in;
	QASM_scanner* scanner;
	std::map<std::string, std::pair<int ,int> > qregs;
	std::map<std::string, int> cregs;
	std::pair<int, int> QASMargument();
	Expr* QASMexponentiation();
	Expr* QASMfactor();
	Expr* QASMterm();
	QasmSimulator::Expr* QASMexp();
	void QASMgateDecl();
	void QASMgate();
	void QASMexpList(std::vector<Expr*>& expressions);
	void QASMidList(std::vector<std::string>& identifiers);
	void QASMargsList(std::vector<std::pair<int, int> >& arguments);
	std::set<Token::Kind> unaryops {Token::Kind::sin,Token::Kind::cos,Token::Kind::tan,Token::Kind::exp,Token::Kind::ln,Token::Kind::sqrt};

	QMDD_matrix tmp_matrix;

	std::map<std::string, CompoundGate> compoundGates;
	Expr* RewriteExpr(Expr* expr, std::map<std::string, Expr*>& exprMap);
	void printExpr(Expr* expr);
};

#endif /* QASM_SIMULATOR_H_ */
