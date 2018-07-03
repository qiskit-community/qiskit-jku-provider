//============================================================================
// Name        : qmdd_simulator.cpp
// Author      : 
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <QMDDcore.h>
#include <QMDDpackage.h>
#include <QMDDcomplex.h>
#include <stdlib.h>
#include <set>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <vector>
#include <gmp.h>
#include <mpreal.h>
#include <queue>
#include <chrono>

#include <QASM_scanner.hpp>

using mpfr::mpreal;


int nqubits;
int nq;
int* line;
int *measurements;
QMDDrevlibDescription circ;
int gatecount = 0;
int max_gates = 0x7FFFFFFF;
mpreal epsilon;
QMDDedge gates;


std::unordered_map<QMDDnodeptr, mpreal> probs;

mpreal assignProbs(QMDDedge& e) {
	std::unordered_map<uint64_t, mpreal>::iterator it2;
	std::unordered_map<QMDDnodeptr, mpreal>::iterator it = probs.find(e.p);
	if(it != probs.end()) {
		it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);
		return it2->second * it2->second * it->second;
	}
	mpreal sum;
	if(QMDDterminal(e)) {
		sum = mpreal(1);
	} else {
		sum = assignProbs(e.p->e[0]) + assignProbs(e.p->e[1]) + assignProbs(e.p->e[2]) + assignProbs(e.p->e[3]);
	}

	probs.insert(std::pair<QMDDnodeptr, mpreal>(e.p, sum));

	it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);

	return it2->second * it2->second * sum;
}



using namespace std;
void QMDDmeasureAll() {
	QMDDedge e;

	std::unordered_map<QMDDnodeptr, mpreal>::iterator it;
	std::unordered_map<uint64_t, mpreal>::iterator it2;

	probs.clear();
	e = circ.e;

	mpreal p,p0,p1,tmp,w;
	p = assignProbs(e);

	if(abs(p -1) > epsilon) {
		cerr << "Numerical error occurred during simulation: |alpha0|^2 + |alpha1|^2 = " << p<< ", but should be 1!"<<endl;
		exit(1);
	}
	QMDDedge cur = e;
	QMDDedge edges[4];
	QMDDedge prev = QMDDone;
	int prev_index = 0;
	for(int i = QMDDinvorder[e.p->v]; i >= 0;--i) {


		cout << "  -- measure qubit " << circ.line[cur.p->v].variable << ": " << flush;
		it = probs.find(cur.p->e[0].p);
		it2 = Cmag.find(cur.p->e[0].w & 0x7FFFFFFF7FFFFFFFull);
		p0 = it->second * it2->second * it2->second;
		it2 = Cmag.find(cur.p->e[1].w & 0x7FFFFFFF7FFFFFFFull);
		it = probs.find(cur.p->e[1].p);
		p0 += it->second * it2->second * it2->second;

		it = probs.find(cur.p->e[2].p);
		it2 = Cmag.find(cur.p->e[2].w & 0x7FFFFFFF7FFFFFFFull);
		p1 = it->second * it2->second * it2->second;
		it = probs.find(cur.p->e[3].p);
		it2 = Cmag.find(cur.p->e[3].w & 0x7FFFFFFF7FFFFFFFull);
		p1 += it->second * it2->second * it2->second;

		it2 = Cmag.find(cur.w & 0x7FFFFFFF7FFFFFFFull);

		mpreal tmp = it2->second * it2->second;
		p0 *= tmp;
		p1 *= tmp;

		if(abs(p0+p1-1) > epsilon) {
			cerr << "Numerical error occurred during simulation: |alpha0|^2 + |alpha1|^2 = " << p0+p1 << ", but should be 1!"<<endl;
			exit(1);
		}

		cout << "p0 = " << p0 << ", p1 = " << p1 << flush;
		mpreal n = mpreal(rand()) / RAND_MAX;

		if(n < p0) {
			cout << " -> measure 0" << endl;
			measurements[cur.p->v] = 0;
			edges[0]=cur.p->e[0];
			edges[1]=cur.p->e[1];
			edges[2]=edges[3]=QMDDzero;

			if(i == QMDDinvorder[e.p->v]) {
				it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);
				w = it2->second;
				e = QMDDmakeNonterminal(cur.p->v, edges);
				it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);
				w *= it2->second;
				e.w = 0x100000000ull;
				cur = e;
			} else {
				it2 = Cmag.find(cur.w & 0x7FFFFFFF7FFFFFFFull);
				w = it2->second;
				cur = QMDDmakeNonterminal(cur.p->v, edges);
				it2 = Cmag.find(cur.w & 0x7FFFFFFF7FFFFFFFull);
				w *= it2->second;
				cur.w = 0x100000000ull;
				prev.p->e[prev_index] = cur;
			}

			if(cur.p->e[0].w != 0x0ull) {
				prev_index = 0;
			} else {
				prev_index = 1;
			}
			prev=cur;
			cur = prev.p->e[prev_index];
			p0 = w / sqrt(p0);
			mpreal zero = mpreal(0);
			uint64_t a = Cmake(p0,zero);

			cur.w = Cmul(cur.w, a);
		} else {
			cout << " -> measure 1" << endl;
			measurements[cur.p->v] = 1;
			edges[2]=cur.p->e[2];
			edges[3]=cur.p->e[3];
			edges[0]=edges[1]=QMDDzero;

			if(i == QMDDinvorder[e.p->v]) {
				it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);
				w = it2->second;
				e = QMDDmakeNonterminal(cur.p->v, edges);
				it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);
				w *= it2->second;
				e.w = 0x100000000ull;
				cur = e;
			} else {
				it2 = Cmag.find(cur.w & 0x7FFFFFFF7FFFFFFFull);
				w = it2->second;
				cur = QMDDmakeNonterminal(cur.p->v, edges);
				it2 = Cmag.find(cur.w & 0x7FFFFFFF7FFFFFFFull);
				w *= it2->second;
				cur.w = 0x100000000ull;
				prev.p->e[prev_index] = cur;
			}

			if(cur.p->e[2].w != 0x0ull) {
				prev_index = 2;
			} else {
				prev_index = 3;
			}

			prev=cur;
			cur = prev.p->e[prev_index];

			p1 = w / sqrt(p1);
			cur.w = Cmul(cur.w, Cmake(p1, mpreal(0)));
		}
	}

	cout << endl;
}


std::map<QMDDnodeptr,mpreal> probsMone;
std::set<QMDDnodeptr> visited_nodes2;

std::pair<mpreal, mpreal> assignProbsMone(QMDDedge e, int index) {
	probs.clear();
	assignProbs(e);
	std::queue<QMDDnodeptr> q;
	mpreal pzero, pone;
	pzero = pone = mpreal(0);
	mpreal prob;

	probsMone.clear();
	visited_nodes2.clear();

	visited_nodes2.insert(e.p);
	auto it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);
	probsMone[e.p] = it2->second * it2->second;
	mpreal tmp1;
	q.push(e.p);

	while(q.front()->v != index) {
		QMDDnodeptr ptr = q.front();
		q.pop();
		mpreal prob = probsMone[ptr];

		if(ptr->e[0].w != COMPLEX_ZERO) {
			auto it2 = Cmag.find(ptr->e[0].w & 0x7FFFFFFF7FFFFFFFull);
			tmp1 = prob * it2->second * it2->second;

			if(visited_nodes2.find(ptr->e[0].p) != visited_nodes2.end()) {
				probsMone[ptr->e[0].p] = probsMone[ptr->e[0].p] + tmp1;
			} else {
				probsMone[ptr->e[0].p] = tmp1;
				visited_nodes2.insert(ptr->e[0].p);
				q.push(ptr->e[0].p);
			}
		}

		if(ptr->e[2].w != COMPLEX_ZERO) {
			auto it2 = Cmag.find(ptr->e[2].w & 0x7FFFFFFF7FFFFFFFull);
			tmp1 = prob * it2->second * it2->second;

			if(visited_nodes2.find(ptr->e[2].p) != visited_nodes2.end()) {
				probsMone[ptr->e[2].p] = probsMone[ptr->e[2].p] + tmp1;
			} else {
				probsMone[ptr->e[2].p] = tmp1;
				visited_nodes2.insert(ptr->e[2].p);
				q.push(ptr->e[2].p);
			}
		}
	}

	while(q.size() != 0) {
		QMDDnodeptr ptr = q.front();
		q.pop();

		if(ptr->e[0].w != COMPLEX_ZERO) {
			auto it2 = Cmag.find(ptr->e[0].w & 0x7FFFFFFF7FFFFFFFull);
			tmp1 = probsMone[ptr] * probs[ptr->e[0].p] * it2->second * it2->second;
			pzero = pzero + tmp1;
		}

		if(ptr->e[2].w != COMPLEX_ZERO) {
			auto it2 = Cmag.find(ptr->e[2].w & 0x7FFFFFFF7FFFFFFFull);
			tmp1 = probsMone[ptr] * probs[ptr->e[2].p] * it2->second * it2->second;
			pone = pone + tmp1;
		}
	}


	probs.clear();
	probsMone.clear();
	visited_nodes2.clear();

	return std::make_pair(pzero, pone);
}


int QMDDmeasureOne(int index) {
	std::pair<mpreal, mpreal> probs = assignProbsMone(circ.e, index);

	QMDDedge e = circ.e;
	std::cout << "  -- measure qubit " << circ.line[e.p->v].variable << ": " << std::flush;

	mpreal sum = probs.first + probs.second;
	mpreal norm_factor;

	if(abs(sum - 1) > epsilon) {
		std::cerr << "Numerical error occurred during simulation: |alpha0|^2 + |alpha1|^2 = " << sum << ", but should be 1!"<< std::endl;
		exit(1);
	}

	std::cout << "p0 = " << probs.first << ", p1 = " << probs.second << std::flush;

	mpreal n = (mpreal(rand()) / RAND_MAX);

	for(int i=0; i<circ.n; i++) {
		line[i] = -1;
	}
	line[index] = 2;

	QMDD_matrix measure_m;
	measure_m[0][0] = measure_m[0][1] = measure_m[1][0] = measure_m[1][1] = COMPLEX_ZERO;

	int measurement;

	if(n < probs.first) {
		std::cout << " -> measure 0" << std::endl;
		measure_m[0][0] = COMPLEX_ONE;
		measurement = 0;
		norm_factor = probs.first;
	} else {
		std::cout << " -> measure 1" << std::endl;
		measure_m[1][1] = COMPLEX_ONE;
		measurement = 1;
		norm_factor = probs.second;
	}

	QMDDedge f = QMDDmvlgate(measure_m, circ.n, line);
	e = QMDDmultiply(f,e);
	QMDDdecref(circ.e);
	QMDDincref(e);
	circ.e = e;
	circ.e.w = e.w = Cmul(e.w, Cmake(sqrt(mpreal(1)/mpreal(norm_factor)), mpreal(0)));

	return measurement;
}



int max_active;



unsigned int complex_limit = 10000;


set<QMDDnodeptr> visited;

void countNodes(QMDDnodeptr p) {
	if(p == QMDDone.p) {
		return;
	}
	if(visited.find(p) != visited.end()) {
		return;
	}
	visited.insert(p);
	countNodes(p->e[0].p);
	countNodes(p->e[1].p);
	countNodes(p->e[2].p);
	countNodes(p->e[3].p);
}


uint64_t getElementofVector(QMDDedge e, unsigned long element) {
	if(QMDDterminal(e)) {
		return 0;
	}
	uint64_t l = COMPLEX_ONE;
	do {
		l = Cmul(l, e.w);
		//cout << "variable q" << QMDDinvorder[e.p->v] << endl;
		long tmp = (element >> QMDDinvorder[e.p->v]) & 1;
		e = e.p->e[2*tmp];
		//element = element % (int)pow(MAXRADIX, QMDDinvorder[e.p->v]+1);
	} while(!QMDDterminal(e));
	l = Cmul(l, e.w);

	return l;
}

//QMDDedge dummy_edge;
vector<QMDDedge> stack;

Token la,t;
Token::Kind sym;

QASM_scanner* scanner;

void scan() {
	t = la;
	la = scanner->next();
	sym = la.kind;
}

void check(Token::Kind expected) {
	if (sym == expected) {
		scan();
	} else {
		cerr << "ERROR while parsing QASM file" << endl;
	}
}

map<QMDDnodeptr, QMDDedge> dag_edges;

QMDDedge addVariables(QMDDedge e, QMDDedge t, int add) {

	if(e.p == QMDDtnode) {
		if(e.w == 0) {
			return QMDDzero;
		}
		t.w = Cmul(e.w, t.w);
		return t;
	}

	map<QMDDnodeptr, QMDDedge>::iterator it = dag_edges.find(e.p);
	if(it != dag_edges.end()) {
		QMDDedge e2 = it->second;
		e2.w = Cmul(e.w, e2.w);
		return e2;
	}

	QMDDedge edges[MAXRADIX*MAXRADIX];

	for(int i=0; i<MAXRADIX*MAXRADIX; i++) {
		edges[i] = addVariables(e.p->e[i], t, add);
	}

	QMDDedge e2 = QMDDmakeNonterminal(e.p->v+add, edges);
	dag_edges[e.p] = e2;
	e2.w = Cmul(e.w, e2.w);
	return e2;
}

void printSVG(QMDDedge e, int n, char* filename) {
	QMDDrevlibDescription circ;
	circ.n = n;
	for(int i=0; i<n; i++) {
		snprintf(circ.line[i].variable, 10, "x%d",i);
	}
	QMDDdotExport(e, 10000, filename, circ, 1);
}

map<string, pair<int ,int> > qregs;
map<string, int> cregs;


int QASMargument() {
	check(Token::Kind::identifier);
	string s = t.str;
	if(sym == Token::Kind::lbrack) {
		scan();
		check(Token::Kind::nninteger);
		int offset = t.val;
		check(Token::Kind::rbrack);
		return qregs[s].first+offset;
	}
	if(qregs[s].second != 1) {
		cerr << "Argument has to be a single qubit" << endl;
	}
	return qregs[s].first;
}

QMDD_matrix tmp_matrix;

set<Token::Kind> unaryops {Token::Kind::sin,Token::Kind::cos,Token::Kind::tan,Token::Kind::exp,Token::Kind::ln,Token::Kind::sqrt};


mpreal QASMexp();

mpreal QASMexponentiation() {
	mpreal x;

	if(sym == Token::Kind::real) {
		scan();
		return mpreal(t.val_real);
	} else if(sym == Token::Kind::nninteger) {
		scan();
		return mpreal(t.val);
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
		std::cerr << "Invalid Expression" << endl;
	}
	return 0;
}

mpreal QASMfactor() {
	mpreal x,y;
	x = QASMexponentiation();
	while (sym == Token::Kind::power) {
		scan();
		y = QASMexponentiation();
		x = pow(x, y);
	}
	return x;
}

mpreal QASMterm() {
	mpreal x,y;
	x = QASMfactor();

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


mpreal QASMexp() {
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

QMDDedge gateQASM() {
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

void simulateQASM(string fname)
{

	std::ifstream ifs (fname, std::ifstream::in);
	scanner = new QASM_scanner(ifs);

	scan();
	check(Token::Kind::openqasm);
	check(Token::Kind::real);
	check(Token::Kind::semicolon);

	nqubits = 0;

	line = new int[MAXN];
	for(int i = 0; i < MAXN; i++) {
		line[i] = -1;
	}

	max_active = 0;
	gatecount = 0;

	QMDDedge f, tmp;
	QMDDedge e = QMDDone;
	QMDDincref(e);

	do {
		if(sym == Token::Kind::qreg) {

			scan();
			check(Token::Kind::identifier);
			string s = t.str;
			check(Token::Kind::lbrack);
			check(Token::Kind::nninteger);
			int n = t.val;
			check(Token::Kind::rbrack);
			check(Token::Kind::semicolon);
			//check whether it already exists
			qregs["s"] = make_pair(nqubits, n);

			f = QMDDone;
			QMDDedge edges[4];
			edges[1]=edges[2]=edges[3]=QMDDzero;

			for(int p=0;p<n;p++) {
				edges[0] = f;
				f = QMDDmakeNonterminal(p, edges);
			}
			if(e.p != QMDDzero.p) {
				f = addVariables(e, f, n);
				dag_edges.clear();
			}
			QMDDincref(f);
			QMDDdecref(e);
			e = f;

			nqubits += n;
			circ.n = nqubits;
			for(auto it = qregs.begin(); it != qregs.end(); it++) {
				for(int i=it->second.second-1; i >=0 ; i--) {
					snprintf(circ.line[nqubits - 1 - (it->second.first + i)].variable, 10 , "%s[%d]",it->first.c_str(), i);
				}
			}

		} else if(sym == Token::Kind::creg) {
			scan();
			check(Token::Kind::identifier);
			string s = t.str;
			check(Token::Kind::lbrack);
			check(Token::Kind::nninteger);
			int n = t.val;
			check(Token::Kind::rbrack);
			check(Token::Kind::semicolon);
			//TODO: implement
		} else if(sym == Token::Kind::ugate || sym == Token::Kind::cxgate || sym == Token::Kind::identifier) {
			f = gateQASM();

			tmp = e;
			e = QMDDmultiply(f, e);
			QMDDincref(e);
			QMDDdecref(tmp);

			if(ActiveNodeCount > max_active) {
				max_active = ActiveNodeCount;
			}
			gatecount++;

			if(Ctable.size() > complex_limit) {
				vector<QMDDedge> v;
				v.push_back(e);
				for(auto it = stack.begin(); it != stack.end(); it++) {
					v.push_back(*it);
				}
				cleanCtable(v);
				v.clear();
				if(Ctable.size() > complex_limit/2) {
					complex_limit = 2 * Ctable.size();
				}
			}
			QMDDgarbageCollect();
		} else if(sym == Token::Kind::probabilities) {
			cout << "Probabilities of the states |";
			for(int i=nqubits-1; i>=0; i--) {
				cout << circ.line[i].variable << " ";
			}
			cout << ">:" << endl;
			for(int i=0; i<(1<<nqubits);i++) {
				uint64_t res = getElementofVector(e,i);
				cout << "  |";
				for(int j=nqubits-1; j >= 0; j--) {
					cout << ((i >> j) & 1);
				}
				cout << ">: " << Cmag[res & 0x7FFFFFFF7FFFFFFFull]*Cmag[res & 0x7FFFFFFF7FFFFFFFull];
				cout << endl;
			}
			scan();
			check(Token::Kind::semicolon);
		} else {
            cerr << "ERROR: unexpected statement!" << endl; 
		}
	} while (sym != Token::Kind::eof);

	delete scanner;
}


void QMDDsimulate(string fname)
{
	FILE *infile;

	int i;

	QMDDedge e,f,olde;

	// get name of input file, open it and attach it to file (a global)
	infile=openTextFile((char*)fname.c_str(),'r');
	if(infile == NULL) {
		cerr << "Error: failed to open circuit file!"<< endl;
		exit(3);
	}

	//Read header from infile
	circ=QMDDrevlibHeader(infile);


    cout << "Simulate " << fname << " (requires " << circ.n << " qubits):" << endl;

	circ.ngate=circ.cgate=circ.tgate=circ.fgate=circ.pgate=circ.vgate=0;
	circ.qcost=circ.ngates=0;

	e = QMDDone;
	QMDDedge edges[4];
	edges[1]=edges[3]=QMDDzero;

	for(int p=0;p<circ.n;p++) {
		if(circ.line[p].ancillary=='0') {
			edges[0] = e;
			edges[2] = QMDDzero;
		} else if(circ.line[p].ancillary=='1') {
			edges[0] = QMDDzero;
			edges[2] = e;
		} else {
			cerr << "Error: the input vector is not correctly specified!" << endl;
			exit(2);
		}
		e = QMDDmakeNonterminal(p, edges);
	}

	QMDDincref(e);

	cout << "Start simulation (input vector = ";
	for(int i=circ.n-1; i >= 0; --i) {
		cout << circ.line[i].ancillary;
	}
	cout << "):" << endl;

	line = new int[circ.n];

	auto t1 = chrono::high_resolution_clock::now();

	int gate_size = 1;
	while(1) // read gates
	{
		if(gatecount >= max_gates) {
			break;
		}
		f=QMDDreadGate(infile,&circ);

		if(f.p==NULL) {
			if(f.w == COMPLEX_M_ONE) {
				char buf[128];
				sprintf(buf, "/tmp/vector_after_gate%d.dot", gatecount);
				QMDDdotExport(e, 0, buf, circ, 1);

			} else if(f.w == 3) {
				//push
				QMDDincref(e);
				stack.push_back(e);
			} else if(f.w == 4) {
				//pop
				if(stack.size() == 0) {
					cerr << "ERROR: cannot pop element from empty stack!" << endl;
					exit(7);
				}
				QMDDdecref(e);
				e = stack.back();
				stack.pop_back();
			} else if(f.w >= 10) {

				QMDDdecref(e);
				//TODO:
				QMDDmeasureOne(f.w-10);

				QMDDincref(e);
				vector<QMDDedge> v;
				v.push_back(e);

				for(auto it = stack.begin(); it != stack.end(); it++) {
					v.push_back(*it);
				}
				cleanCtable(v);
				v.clear();
				QMDDgarbageCollect();
			} else {
				break;
			}
			continue;
		}

	olde = e;
	e = QMDDmultiply(f, e);
	QMDDincref(e);
	QMDDdecref(olde);

		if(ActiveNodeCount > max_active) {
			max_active = ActiveNodeCount;
		}
		gatecount++;

		cout << ActiveNodeCount << endl;


		if(Ctable.size() > complex_limit) {
			vector<QMDDedge> v;
			v.push_back(e);
			for(auto it = stack.begin(); it != stack.end(); it++) {
				v.push_back(*it);
			}
			cleanCtable(v);
			v.clear();
			if(Ctable.size() > complex_limit/2) {
				complex_limit = 2 * Ctable.size();
			}
		}
		QMDDgarbageCollect();
	}

	for(i=0;i<circ.n;i++) circ.outperm[i]=i;

	skip2eof(infile); // skip rest of input file

    circ.e=e;

	i=0;
	if(circ.ngate) circ.kind[i++]='N';
	if(circ.cgate) circ.kind[i++]='C';
	if(circ.tgate) circ.kind[i++]='T';
	if(circ.fgate) circ.kind[i++]='F';
	if(circ.pgate) circ.kind[i++]='P';
	if(circ.vgate) circ.kind[i++]='V';
	circ.kind[i]=0;

	i=0;
	if(circ.nancillary>0) circ.dc[i++]='C';
	if(circ.ngarbage>0) circ.dc[i++]='G';
	circ.dc[i]=0;

	fclose(infile);

	measurements = new int[circ.n];

	QMDDmeasureAll();

	cout << "Measured: ";
	for(int i = circ.n-1; i >= 0; --i) {
		cout << measurements[i];
	}
	cout << " (starting from the most significant bit)" << endl;

	delete[] measurements;
}


void apply_gate(QMDD_matrix& m, int count, int target, int div = 0, int silent = 0, string indent="") {
	gatecount++;


	QMDDedge e,f;
	f = QMDDmvlgate(m, circ.n, line);

	e = QMDDmultiply(f, circ.e);
	QMDDincref(e);
	QMDDdecref(circ.e);
	circ.e = e;

	QMDDgarbageCollect();

	if(ActiveNodeCount > max_active) {
		max_active = ActiveNodeCount;
	}

	if(Ctable.size() > complex_limit) {
		vector<QMDDedge> v;
		v.push_back(circ.e);

		cleanCtable(v);
		v.clear();
		if(complex_limit < 2*Ctable.size()) {
			complex_limit *= 2;
		}
	}
}

int inverse_mod(int a, int n) {
    int t = 0;
	int newt = 1;
    int r = n;
    int newr = a;
    int h;
    while(newr != 0) {
        int quotient = r / newr;
        h = t;
        t = newt;
        newt = h - quotient * newt;
        h = r;
        r = newr;
        newr = h - quotient * newr;
    }
    if(r > 1) {
    	cout << "ERROR: a is not invertible" << endl;
    }
    if(t < 0) {
    	t = t + n;
    }
    return t;
}

mpreal q_r, q_i;

void add_phi(int n, int c1, int c2) {

	int q;
	int controls = 0;
	if(c1 >= 0) controls++;
	if(c2 >= 0) controls++;
	for(int i = nqubits; i>= 0; --i) {
		q = 1;
		int fac = 0;
		for(int j = i; j >= 0; --j) {
			if((n >> j)&1) {
				fac |= 1;
			}
			q*=2;
			fac = fac << 1;
		}
		if(c1 >= 0) {
			line[nq-c1] = 1;
		}
		if(c2 >= 0) {
			line[nq-c2] = 1;
		}
		line[nq-(1+2*nqubits-i)] = 2;
		q_r = QMDDcos(fac, q);
		q_i = QMDDsin(fac, q);

		//Cfree(Qm[1][1]);
		Qm[1][1]=Cmake(q_r,q_i);

		//cout << "U = [1 0; 0 exp(pi*i*" << fac << "/" << q << ")];" << endl;
		apply_gate(Qm, controls+1, nq-(1+2*nqubits-i), q);

		line[nq-(1+2*nqubits-i)] = -1;
		if(c1 >= 0) {
			line[nq-c1] = -1;
		}
		if(c2 >= 0) {
			line[nq-c2] = -1;
		}
	}
}

void add_phi_inv(int n, int c1, int c2) {
	int q;
	int controls = 0;
	if(c1 >= 0) controls++;
	if(c2 >= 0) controls++;
	for(int i = nqubits; i>= 0; --i) {
		q = 1;
		int fac = 0;
		for(int j = i; j >= 0; --j) {
			if((n >> j)&1) {
				fac |= 1;
			}
			fac = fac << 1;
			q*=2;
		}
		if(c1 >= 0) {
			line[nq-c1] = 1;
		}
		if(c2 >= 0) {
			line[nq-c2] = 1;
		}

		line[nq-(1+2*nqubits-i)] = 2;

		q_r = QMDDcos(fac, -q);
		q_i = QMDDsin(fac, -q);

		//Cfree(Qm[1][1]);
		Qm[1][1]=Cmake(q_r,q_i);
		//cout << "U = [1 0; 0 exp(-pi*i*" << fac << "/" << q << ")];" << endl;
		apply_gate(Qm, controls+1, nq-(1+2*nqubits-i), -q);

		line[nq-(1+2*nqubits-i)] = -1;
		if(c1 >= 0) {
			line[nq-c1] = -1;
		}
		if(c2 >= 0) {
			line[nq-c2] = -1;
		}
	}
}

void qft() {
	int q;
	for(int i = nqubits+1; i < 2*nqubits+2; i++) {
		line[nq-i] = 2;
		apply_gate(Hm, 1, nq-i);
		line[nq-i] = -1;

		q = 2;
		for(int j = i+1; j < 2*nqubits+2; j++) {
			line[nq-j] = 1;
			line[nq-i] = 2;

			q_r = QMDDcos(1, q);
			q_i = QMDDsin(1, q);

			//Cfree(Qm[1][1]);
			Qm[1][1]=Cmake(q_r, q_i);
			//cout << "U = [1 0; 0 exp(pi*i/" << q << ")];" << endl;
			apply_gate(Qm, 2, nq-i, q);

			line[nq-j] = -1;
			line[nq-i] = -1;
			q *= 2;
		}
	}
}

void qft_inv() {
	int q, qq;
	for(int i = 2*nqubits+1; i >= nqubits+1; i--) {
		q = 2;
		for(int j = i+1; j < 2*nqubits+2; j++) {
			qq = q;
			line[nq-j] = 1;
			line[nq-i] = 2;

			q_r = QMDDcos(1, -qq);
			q_i = QMDDsin(1, -qq);

			//Cfree(Qm[1][1]);
			Qm[1][1]=Cmake(q_r, q_i);
			//cout << "U = [1 0; 0 exp(-pi*i/" << qq << ")];" << endl;

			apply_gate(Qm, 2, nq-i, -qq);

			line[nq-j] = -1;
			line[nq-i] = -1;
			q *= 2;
		}
		line[nq-i] = 2;
		apply_gate(Hm, 1, nq-i);
		line[nq-i] = -1;
	}
}

void mod_add_phi(int a, int N, int c1, int c2) {
	add_phi(a, c1, c2);
	add_phi_inv(N,-1,-1);
	qft_inv();

	line[nq-(nqubits+1)] = 1;
	line[nq-(2*nqubits+2)] = 2;

	apply_gate(Nm, 2, nq-(2*nqubits+2));

	line[nq-(nqubits+1)] = -1;
	line[nq-(2*nqubits+2)] = -1;

	qft();
	add_phi(N, 2*nqubits+2, -1);
	add_phi_inv(a,c1,c2);
	qft_inv();

	line[nq-(nqubits+1)] = 0;
	line[nq-(2*nqubits+2)] = 2;
	apply_gate(Nm, 2, nq-(2*nqubits+2));
	line[nq-(nqubits+1)] = -1;
	line[nq-(2*nqubits+2)] = -1;

	qft();
	add_phi(a, c1, c2);
}

void mod_add_phi_inv(int a, int N, int c1, int c2) {
	add_phi_inv(a, c1, c2);
	qft_inv();

	line[nq-(nqubits+1)] = 0;
	line[nq-(2*nqubits+2)] = 2;

	apply_gate(Nm, 2, nq-(2*nqubits+2));

	line[nq-(nqubits+1)] = -1;
	line[nq-(2*nqubits+2)] = -1;
	qft();
	add_phi(a,c1,c2);
	add_phi_inv(N, 2*nqubits+2, -1);
	qft_inv();
	line[nq-(nqubits+1)] = 1;
	line[nq-(2*nqubits+2)] = 2;

	apply_gate(Nm,2,  nq-(2*nqubits+2));

	line[nq-(nqubits+1)] = -1;
	line[nq-(2*nqubits+2)] = -1;

	qft();
	add_phi(N,-1,-1);
	add_phi_inv(a, c1, c2);
}

void cmult(int a, int N, int c) {
	qft();

	int t = a;
	for(int i = nqubits; i >= 1; i--) {
		mod_add_phi(t, N, i, c);
		t = (2*t) % N;
	}
	qft_inv();
}

void cmult_inv(int a, int N, int c) {
	qft();
	int t = a;
	for(int i = nqubits; i >= 1; i--) {
		mod_add_phi_inv(t, N, i, c);
		t = (2*t) % N;
	}
	qft_inv();
}

void u_a(int a, int N, int c) {
	cmult(a, N, c);
	for(int i = 0; i < nqubits; i++) {
		line[nq-(nqubits+2+i)] = 1;
		line[nq-(i+1)] = 2;
		apply_gate(Nm, 2, nq-(i+1));

		line[nq-(nqubits+2+i)] = 2;
		line[nq-(i+1)] = 1;
		line[nq-c] = 1;
		apply_gate(Nm, 3, nq-(nqubits+2+i));
		line[nq-(nqubits+2+i)] = 1;
		line[nq-(i+1)] = 2;
		line[nq-c] = -1;

		apply_gate(Nm, 2, nq-(i+1));

		line[nq-(nqubits+2+i)] = -1;
		line[nq-(i+1)] = -1;
	}


	cmult_inv(inverse_mod(a, N), N, c);
}

unsigned long gcd(unsigned long a, unsigned long b) {
	unsigned long c;
    while ( a != 0 ) {
    	c = a; a = b%a;  b = c;
    }
    return b;
}

unsigned long modpow(unsigned long base, unsigned long exp, unsigned long modulus) {
  base %= modulus;
  unsigned long result = 1;
  while (exp > 0) {
    if (exp & 1) result = (result * base) % modulus;
    base = (base * base) % modulus;
    exp >>= 1;
  }
  return result;
}

void shor(unsigned long n, int a) {
    cout << "Simulate Shor's algorithm for n=" << n;

    //Prepare initial state of the circuit
    circ.nancillary=circ.ngarbage=0;

    circ.n=ceil(log2(n))*2+3;
    cout << " (requires " << circ.n << " qubits):" << endl;

    for(int p=circ.n-1;p>=0;p--) {
    	snprintf(circ.line[p].variable, MAXSTRLEN, "x%d", p);
    	snprintf(circ.line[p].input, MAXSTRLEN, "i%d", p);
    	snprintf(circ.line[p].output, MAXSTRLEN, "o%d", p);
        circ.line[p].ancillary = (p==circ.n-1-ceil(log2(n))) ? '1' : '0';
        circ.line[p].garbage='-';
    }

    for(int i=0;i<circ.n;i++) {
    	circ.inperm[i]=i;
    }

	QMDDedge e = QMDDone;
	QMDDedge edges[4];
	edges[1]=edges[3]=QMDDzero;

	for(int p=0;p<circ.n;p++) {
		if(circ.line[p].ancillary=='0') {
			edges[0] = e;
			edges[2] = QMDDzero;
		} else if(circ.line[p].ancillary=='1') {
			edges[0] = QMDDzero;
			edges[2] = e;
		} else {
			edges[0]=edges[2]=e;
		}
		e = QMDDmakeNonterminal(p, edges);
	}


	QMDDincref(e);
	circ.e = e;

	nqubits = ceil(log2(n));
	nq = nqubits*2+2;

	line = new int[2*nqubits+3];
	for(int j=0;j<2*nqubits+3;j++) {
		line[j] = -1;
	}

	if(a != -1 && gcd(a,n) != 1) {
		cout << "Warning: gcd(a,n) != 1 --> choosing a new value for a" << endl;
		a = -1;
	}

	if(a == -1) {
		do {
			a = rand() % n;
		} while (gcd(a, n) != 1 || a == 1);
	}

	cout << "Find a coprime to N: " << a << endl;

	measurements = new int[2*nqubits];

	int* as = new int[2*nqubits];
	as[2*nqubits-1] = a;
	unsigned long new_a = a;
	for(int i = 2*nqubits-2; i >= 0; i--) {
		new_a = new_a * new_a;
		new_a = new_a % n;
		as[i] = new_a;
	}

	//Start Shor's algorithm
	for(int i = 0; i < 2*nqubits; i++) {

		line[nq] = 2;
		apply_gate(Hm, 1, nq);
		line[nq] = -1;

		u_a(as[i], n, 0);

		line[nq] = 2;
		int q = 2;

		for(int j =i-1; j >= 0; j--) {

			if(measurements[j] == 1) {
				q_r = QMDDcos(1, -q);
				q_i = QMDDsin(1, -q);

				Qm[1][1]=Cmake(q_r, q_i);
				apply_gate(Qm, 1, nq, -q, 0, "\t");
			}
			q = q << 1;
		}

		apply_gate(Hm, 1, nq);


		measurements[i] = QMDDmeasureOne(nq);

		QMDDgarbageCollect();
		vector<QMDDedge> v;
		v.push_back(circ.e);

		cleanCtable(v);

		v.clear();

		if(measurements[i] == 1) {
			apply_gate(Nm, 1, nq, 0, 0, "\t");
		}
		line[nq] = -1;
	}


	unsigned long res = 0;
	cout << "measurement: ";
	for(int i=0; i<2*nqubits; i++) {
		cout << measurements[2*nqubits-1-i];
		res = (res << 1) + measurements[2*nqubits-1-i];
	}

	cout << " = " << res << endl;
	unsigned long denom = 1 << (2*nqubits);


	//Post processing (non-quantum part)

	if(res == 0) {
		cout << "Factorization failed (measured 0)!" << endl;
	} else {
	cout << "Continued fraction expansion of " << res << "/" << denom << " = " << flush;
	vector<int> cf;

	unsigned long old_res = res;
	unsigned long old_denom = denom;
	while(res != 0) {
		cf.push_back(denom/res);
		unsigned long tmp = denom%res;
		denom = res;
		res = tmp;
	}

	for(unsigned int i = 0; i < cf.size(); i++) {
		cout << cf[i] << " ";
	} cout << endl;

	int success = 0;
	for(unsigned int i=0; i < cf.size(); i++) {
		//determine candidate
		unsigned long denominator = cf[i];
		unsigned long numerator = 1;

		for(int j=i-1; j >= 0; j--) {
			unsigned long tmp = numerator + cf[j]*denominator;
			numerator = denominator;
			denominator = tmp;
		}
		cout << "  Candidate " << numerator << "/" << denominator << ": ";
		if(denominator > n) {
			cout << " denominator too large (greater than " << n << ")!" << endl;
			success = 0;
			cout << "Factorization failed!" << endl;break;
		} else {
			double delta = (double)old_res / (double)old_denom - (double)numerator / (double) denominator;
			if(fabs(delta) < 1.0/(2.0*old_denom)) {
				if(modpow(a, denominator, n) == 1) {
					cout << "found period = " << denominator << endl;
					if(denominator & 1) {
						cout << "Factorization failed (period is odd)!" << endl;
					} else {
						cout << "Factorization succeeded! Non-trivial factors are: " << endl;
						unsigned long f1, f2;
						f1 = modpow(a, denominator>>1, n);
						f2 = (f1+1)%n;
						f1 = (f1 == 0) ? n-1 : f1-1;
						f1 = gcd(f1, n);
						f2 = gcd(f2, n);
						cout << "  -- gcd(" << n << "^(" << denominator << "/2)-1" << "," << n << ") = " << f1 << endl;
						cout << "  -- gcd(" << n << "^(" << denominator << "/2)+1" << "," << n << ") = " << f2 << endl;
					}

					break;
				} else {
					cout << "failed" << endl;
				}
			} else {
				cout << "delta is too big (" << delta << ")"<<endl;
			}
		}
	}
	}
	delete[] measurements;
	delete[] line;

}

QMDDedge randomOracle(unsigned long n, unsigned long* expected) {
	cout << "generate random " << n << "-bit oracle: " << flush;
	*expected = 0;
	for(unsigned int p=0;p < n;p++) {
		if(rand() %2 == 0) {
			cout << "0";
			line[n-p] = 0;
			*expected = (*expected) << 1;
		} else {
			cout << "1";
			line[n-p] = 1;
			*expected = ((*expected) << 1) | 1;
		}
	}

	cout << " = " << *expected << endl;

	line[0] = 2;
	QMDDedge f = QMDDmvlgate(Nm, n+1, line);
	for(unsigned int i=0; i<n+1; i++) {
		line[i] = -1;
	}

	return f;
}

QMDDedge groverIteration(QMDDedge oracle) {

	QMDDedge e = circ.e;
	circ.e = oracle;
	QMDDincref(circ.e);

	for(int i =0; i < circ.n-1; i++) {
		line[circ.n-1-i] = 2;
		apply_gate(Hm, 1, circ.n-1-i);
		line[circ.n-1-i] = -1;
	}


	for(int i =0; i < circ.n-1; i++) {
		line[circ.n-1-i] = 2;
		apply_gate(Nm, 1, circ.n-1-i);
		line[circ.n-1-i] = -1;
	}


	line[1] = 2;
	apply_gate(Hm, 1, 1);
	line[1] = -1;

	for(int i=0; i < circ.n-2; i++) {
		line[circ.n-1-i] = 1;
	}
	line[1] = 2;
	apply_gate(Nm, circ.n-1, 1);
	for(int i=0; i < circ.n; i++) {
		line[i] = -1;
	}
	line[1] = 2;
	apply_gate(Hm, 1, 1);

	line[1] = -1;
	for(int i =0; i < circ.n-1; i++) {
		line[circ.n-1-i] = 2;
		apply_gate(Nm, 1, circ.n-1-i);
		line[circ.n-1-i] = -1;
	}
	for(int i =0; i < circ.n-1; i++) {
		line[circ.n-1-i] = 2;
		apply_gate(Hm, 1, circ.n-1-i);
		line[circ.n-1-i] = -1;
	}

	QMDDedge t = circ.e;
	circ.e = e;
	QMDDdecref(t);



	return t;
}

void grover(unsigned long n) {

	cout << "Simulate Grover's search algorithm for " << n << "-Bit number ";

    circ.nancillary=circ.ngarbage=0;

    circ.n=n+1;
    cout << " (requires " << circ.n << " qubits):" << endl;

    for(int p=circ.n-1;p>=0;p--) {
    	snprintf(circ.line[p].variable, MAXSTRLEN, "x%d", p);
    	snprintf(circ.line[p].input, MAXSTRLEN, "i%d", p);
    	snprintf(circ.line[p].output, MAXSTRLEN, "o%d", p);
        circ.line[p].ancillary = (p==0) ? '1' : '0';
        circ.line[p].garbage='-';
    }

    for(int i=0;i<circ.n;i++) {
    	circ.inperm[i]=i;
    }

	QMDDedge e = QMDDone;
	QMDDedge edges[4];
	edges[1]=edges[3]=QMDDzero;

	for(int p=0;p<circ.n;p++) {
		if(circ.line[p].ancillary=='0') {
			edges[0] = e;
			edges[2] = QMDDzero;
		} else if(circ.line[p].ancillary=='1') {
			edges[0] = QMDDzero;
			edges[2] = e;
		} else {
			edges[0]=edges[2]=e;
		}
		e = QMDDmakeNonterminal(p, edges);
	}
	QMDDincref(e);
	circ.e = e;

	line = new int[circ.n];
	measurements = new int[circ.n];
	for(int j=0;j<circ.n;j++) {
		line[j] = -1;
	}

	unsigned long iterations;
	if((circ.n-1)%2 == 0) {
		iterations = pow(2, (circ.n-1)/2);
	} else {
		iterations = floor(pow(2, (circ.n-2)/2) * sqrt(2));
	}

	unsigned long expected;
	QMDDedge t = randomOracle(n, &expected);
	QMDDincref(t);

	for(int i =0; i < circ.n; i++) {
		line[circ.n-1-i] = 2;
		apply_gate(Hm, 1, circ.n-1-i);
		line[circ.n-1-i] = -1;
	}

	QMDDedge gi = groverIteration(t);

	QMDDincref(gi);
	QMDDdecref(t);
	QMDDdecref(circ.e);
	cout << "oracle has " << ActiveNodeCount << " nodes" << endl;
	QMDDincref(circ.e);

	cout << "perform " << iterations << " Grover iterations" << endl;

	for(unsigned int i = 0; i < iterations; i++) {
		t = QMDDmultiply(gi, circ.e);
		QMDDincref(t);
		QMDDdecref(circ.e);
		if(ActiveNodeCount > max_active) {
			max_active = ActiveNodeCount;
		}
		circ.e = t;
		QMDDgarbageCollect();
		if((i+1) %1000 == 0) {
			cout << "  -- performed " << (i+1) << " of " << iterations << " grover iterations" << endl;
		}
//			if((i+1) %10 == 0) {
//			cout << "  -- performed " << (i+1) << " of " << iterations << " grover iterations" << endl;
//			cout << "  size of ctable: " << Ctable.size() << endl;
			vector<QMDDedge> v;

			v.push_back(circ.e);
			v.push_back(gi);
			cleanCtable(v);
			v.clear();
//		}
	}


	QMDDmeasureAll();
	cout << "Measured: ";
    unsigned long measured = 0;
	for(int i = circ.n-1; i > 0; --i) {
		cout << measurements[i];
		measured = (measured << 1) | measurements[i];
	}
	cout << " = " << measured << " (expected " << expected << ")" << endl;

	gatecount = (1+(circ.n-1)*4+3)*iterations + circ.n;
	delete[] measurements;
}


int main(int argc, char** argv) {

	namespace po = boost::program_options;
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
		("seed", po::value<unsigned long>(), "seed for random number generator")
	    ("simulate_circuit", po::value<string>(), "simulate a quantum circuit given in .real format")
	    ("simulate_qasm", po::value<string>(), "simulate a quantum circuit given in QPENQASM 2.0 format")
		("simulate_shor", po::value<unsigned long>(), "simulate Shor's algorithm for <n>")
		("simulate_grover", po::value<unsigned long>(), "simulate Groover's algorithm with oracle <n>")
		("a", po::value<unsigned long>(), "value of a (coprime to <n>) for Shors algorithm")
		("limit", po::value<unsigned long>(), "max number of gates to be simulated")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, description), vm);
	po::notify(vm);

	if (vm.count("help")) {
	    cout << description << "\n";
	    return 1;
	}



	max_active = 0;
	gatecount = 0;
	epsilon = mpreal(0.01);

	unsigned long seed = time(NULL);
	if (vm.count("seed")) {
		seed = vm["seed"].as<unsigned long>();
	}


	//seed = 1507544248ul;
//	cout << "Seed for random number generator is " << seed << endl;
	srand(seed);

	QMDDinit(0);

    //struct timeval t1, t2;
    //double elapsedTime;


/*    QMDD_matrix mm;
    mm[0][0] = CmakeZero();
    mm[1][1] = CmakeZero();
    mm[0][1] = Cmake(1, 1);
    mm[1][0] = Cmake(0,2);

   / int tmp_line[1];
    tmp_line[0] = 2;
    QMDDedge tmp = QMDDmvlgate(mm, 1, tmp_line);
    QMDDmatrixPrint2(tmp);
    QMDDmatrixPrint2(QMDDconjugateTranspose(tmp));
*/
	    // start timer
    auto t1 = chrono::high_resolution_clock::now();
	//gettimeofday(&t1, NULL);

	if (vm.count("limit")) {
			max_gates = vm["limit"].as<unsigned long>();
	}

	if (vm.count("simulate_circuit")) {
		QMDDsimulate(vm["simulate_circuit"].as<string>());
	} else if (vm.count("simulate_qasm")) {
		simulateQASM(vm["simulate_qasm"].as<string>());
	} else if (vm.count("simulate_shor")) {
		int a = -1;
		if (vm.count("a")) {
				a = vm["a"].as<unsigned long>();
			}
		shor(vm["simulate_shor"].as<unsigned long>(), a);
	} else if (vm.count("simulate_grover")) {
	    grover(vm["simulate_grover"].as<unsigned long>());
	} else {
		cout << description << "\n";
	    return 1;
	}

	auto t2 = chrono::high_resolution_clock::now();
	chrono::duration<float> diff = t2-t1;
	//gettimeofday(&t2, NULL);


	// compute and print the elapsed time in millisec
    //elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    //elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;

    cout << endl << "SIMULATION STATS: " << endl;
    cout << "  Number of applied gates: " << gatecount << endl;
    cout << "  Simulation time: " << diff.count() << " seconds" << endl;
    cout << "  Maximal size of DD (number of nodes) during simulation: " << max_active << endl;


	return 0;
}
