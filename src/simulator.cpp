/*
 * simulator.cpp
 *
 *  Created on: Jul 3, 2018
 *      Author: zulehner
 */

#include <simulator.h>

Simulator::Simulator() {
	// TODO Auto-generated constructor stub
	epsilon = mpreal(0.01);
	for(int i = 0; i < MAXN; i++) {
		line[i] = -1;
	}
	circ.e = QMDDone;
	QMDDincref(circ.e);
}

Simulator::~Simulator() {
	// TODO Auto-generated destructor stub
}

void Simulator::Reset() {
	QMDDdecref(circ.e);
	QMDDgarbageCollect();
	cleanCtable(std::vector<QMDDedge>());
	nqubits = 0;
	circ.e = QMDDone;
	QMDDincref(circ.e);
	circ.n = 0;
}

void Simulator::AddVariables(int add, std::string name) {
	QMDDedge f = QMDDone;
	QMDDedge edges[4];
	edges[1]=edges[2]=edges[3]=QMDDzero;

	for(int p=0;p<add;p++) {
		edges[0] = f;
		f = QMDDmakeNonterminal(p, edges);
	}
	if(circ.e.p != QMDDzero.p) {
		f = AddVariablesRec(circ.e, f, add);
		dag_edges.clear();
	}
	QMDDincref(f);
	QMDDdecref(circ.e);
	circ.e = f;

	for(int i = nqubits-1; i >= 0; i--) {
		strncpy(circ.line[i+add].variable, circ.line[i].variable, MAXSTRLEN);
	}
	for(int i = 0; i < add; i++) {
		snprintf(circ.line[nqubits + add - 1 - i].variable, MAXSTRLEN , "%s[%d]",name.c_str(), i);
	}

	nqubits += add;
	circ.n = nqubits;
}

QMDDedge Simulator::AddVariablesRec(QMDDedge e, QMDDedge t, int add) {

	if(e.p == QMDDtnode) {
		if(e.w == 0) {
			return QMDDzero;
		}
		t.w = Cmul(e.w, t.w);
		return t;
	}

	std::map<QMDDnodeptr, QMDDedge>::iterator it = dag_edges.find(e.p);
	if(it != dag_edges.end()) {
		QMDDedge e2 = it->second;
		e2.w = Cmul(e.w, e2.w);
		return e2;
	}

	QMDDedge edges[MAXRADIX*MAXRADIX];

	for(int i=0; i<MAXRADIX*MAXRADIX; i++) {
		edges[i] = AddVariablesRec(e.p->e[i], t, add);
	}

	QMDDedge e2 = QMDDmakeNonterminal(e.p->v+add, edges);
	dag_edges[e.p] = e2;
	e2.w = Cmul(e.w, e2.w);
	return e2;
}

mpreal Simulator::AssignProbs(QMDDedge& e) {
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
		sum = AssignProbs(e.p->e[0]) + AssignProbs(e.p->e[1]) + AssignProbs(e.p->e[2]) + AssignProbs(e.p->e[3]);
	}

	probs.insert(std::pair<QMDDnodeptr, mpreal>(e.p, sum));

	it2 = Cmag.find(e.w & 0x7FFFFFFF7FFFFFFFull);

	return it2->second * it2->second * sum;
}

void Simulator::MeasureAll(bool reset_state) {
	QMDDedge e;

	std::unordered_map<QMDDnodeptr, mpreal>::iterator it;
	std::unordered_map<uint64_t, mpreal>::iterator it2;

	probs.clear();
	e = circ.e;

	mpreal p,p0,p1,tmp,w;
	p = AssignProbs(e);

	if(abs(p -1) > epsilon) {
		std::cerr << "Numerical error occurred during simulation: |alpha0|^2 + |alpha1|^2 = " << p<< ", but should be 1!"<<std::endl;
		exit(1);
	}
	QMDDedge cur = e;
	QMDDedge edges[4];
	QMDDedge prev = QMDDone;
	int prev_index = 0;
	for(int i = QMDDinvorder[e.p->v]; i >= 0;--i) {

		//std::cout << "  -- measure qubit " << circ.line[cur.p->v].variable << ": " << std::flush;
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
			std::cerr << "Numerical error occurred during simulation: |alpha0|^2 + |alpha1|^2 = " << p0+p1 << ", but should be 1!"<< std::endl;
			exit(1);
		}

		//std::cout << "p0 = " << p0 << ", p1 = " << p1 << std::flush;
		mpreal n = mpreal(rand()) / RAND_MAX;

		if(n < p0) {
			//std::cout << " -> measure 0" << std::endl;
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
			//std::cout << " -> measure 1" << std::endl;
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

	if(reset_state) {
		QMDDdecref(circ.e);

		QMDDedge e = QMDDone;
		QMDDedge edges[4];
		edges[1]=edges[3]=QMDDzero;

		for(int p=0;p<circ.n;p++) {
			if(measurements[p] == 0) {
				edges[0] = e;
				edges[2] = QMDDzero;
			} else {
				edges[0] = QMDDzero;
				edges[2] = e;
			}
			e = QMDDmakeNonterminal(p, edges);
		}
		QMDDincref(e);
		circ.e = e;
		QMDDgarbageCollect();
		cleanCtable(std::vector<QMDDedge>());
	}

	//std::cout << std::endl;
}

int Simulator::MeasureOne(int index) {
	std::pair<mpreal, mpreal> probs = AssignProbsOne(circ.e, index);

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

std::pair<mpreal, mpreal> Simulator::AssignProbsOne(QMDDedge e, int index) {
	probs.clear();
	AssignProbs(e);
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

uint64_t Simulator::GetElementOfVector(QMDDedge e, unsigned long element) {
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

void Simulator::ApplyGate(QMDDedge gate) {
	gatecount++;

	QMDDedge tmp;

	tmp = QMDDmultiply(gate, circ.e);
	QMDDincref(tmp);
	QMDDdecref(circ.e);
	circ.e = tmp;

	QMDDgarbageCollect();

	if(ActiveNodeCount > max_active) {
		max_active = ActiveNodeCount;
	}

	if(Ctable.size() > complex_limit) {
		std::vector<QMDDedge> v;
		v.push_back(circ.e);

		cleanCtable(v);
		v.clear();
		if(complex_limit < 2*Ctable.size()) {
			complex_limit *= 2;
		}
	}

}

void Simulator::ApplyGate(QMDD_matrix& m) {
	QMDDedge f = QMDDmvlgate(m, circ.n, line);
	ApplyGate(f);
}

