/*
 * simulator.h
 *
 *  Created on: Jul 3, 2018
 *      Author: zulehner
 */

#ifndef SRC_SIMULATOR_H_
#define SRC_SIMULATOR_H_

#include <QMDDcore.h>
#include <QMDDpackage.h>
#include <QMDDcomplex.h>
#include <map>
#include <set>
#include <unordered_map>
#include <queue>

#include <gmp.h>
#include <mpreal.h>

#define VERBOSE 0


class Simulator {
public:
	Simulator();
	virtual void Simulate() = 0;
	virtual void Simulate(int shots) = 0;
	virtual void Reset();
	int GetGatecount() {
		return gatecount;
	}
	int GetQubits() {
		return nqubits;
	}
	int GetMaxActive() {
		return max_active;
	}
	virtual ~Simulator();

protected:
	int MeasureOne(int index);
	void MeasureAll(bool reset_state=true);
	void ApplyGate(QMDD_matrix& m);
	void ApplyGate(QMDDedge gate);
	void AddVariables(int add, std::string name);

	int line[MAXN];
	int measurements[MAXN];
	int nqubits = 0;
	QMDDrevlibDescription circ;

	bool intermediate_measurement = false;

	uint64_t GetElementOfVector(QMDDedge e, unsigned long element);
private:
	QMDDedge AddVariablesRec(QMDDedge e, QMDDedge t, int add);
	mpreal AssignProbs(QMDDedge& e);
	std::pair<mpreal, mpreal> AssignProbsOne(QMDDedge e, int index);

	std::unordered_map<QMDDnodeptr, mpreal> probs;
	std::map<QMDDnodeptr,mpreal> probsMone;
	std::set<QMDDnodeptr> visited_nodes2;
	std::map<QMDDnodeptr, QMDDedge> dag_edges;

	int max_active = 0;
	unsigned int complex_limit = 10000;
	int gatecount = 0;
	int max_gates = 0x7FFFFFFF;

	mpreal epsilon;
};

#endif /* SRC_SIMULATOR_H_ */
