#ifndef QMDDreorder_H
#define QMDDreorder_H

//#include <stdio.h>

#include "QMDDpackage.h"
//#include "QMDDcomplex.h"
#include "timing.h"
//#include "textFileUtilities.h"
//#include <string.h>
//#include <iostream>

/*****************************************************************

    Routines            
*****************************************************************/

int siftingCostFunction(QMDDedge a);

int checkandsetBlockProperty(QMDDedge a);
int checkBlockMatrices(QMDDedge a, int triggerValue);
void QMDDrestoreSpecialMatrices(QMDDedge a);
void QMDDmarkupSpecialMatrices(QMDDedge a);
void QMDDresetVertexWeights(QMDDedge a, uint64_t standardValue);

QMDDedge QMDDbuildIntermediate(QMDDedge a);
QMDDedge QMDDrenormalize(QMDDedge a);
void QMDDchangeNonterminal(short v,QMDDedge edge[],QMDDnodeptr p);
int QMDDcheckDontCare(QMDDnodeptr p, int v2);
void QMDDswapNode(QMDDnodeptr p,int v1,int v2, int swap);
void QMDDswap(int i);
int QMDDsift(int n, QMDDedge *root, QMDDrevlibDescription *circ, std::ostream &os);
int myQMDDsift(int n, QMDDedge *root, QMDDrevlibDescription *circ, std::ostream &os, int lowerbound, int upperbound);
int QMDDsift(int n, QMDDedge *root, QMDDrevlibDescription *circ);
int lookupLabel(char buffer[], char moveLabel[], QMDDrevlibDescription *circ);
void QMDDreorder(int order[],int n, QMDDedge *root);
void myQMDDreorder(int order[],int n, QMDDedge *root);
int QMDDmoveVariable(QMDDedge *basic, char buffer[], QMDDrevlibDescription *circ);
void SJTalgorithm(QMDDedge a, int n);
/*******************************************************************************/
#endif
