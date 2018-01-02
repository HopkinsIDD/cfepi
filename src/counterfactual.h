#ifndef COUNTERFACTUAL_H_
#define COUNTERFACTUAL_H_

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))
#define MAX(x,y) (x) > (y) ? (x) : (y);
#define MIN(x,y) (x) < (y) ? (x) : (y);

void runCounterfactualAnalysis(char*,int*, int, int, double*, double*,char*,char*);
void runFullCounterfactualAnalysis(int*, int, int, double*, double*,char*,char*);
void runFastCounterfactualAnalysis(int*, int, int, double*, double*,char*,char*);
void constructTimeSeries(int* ,int, int,int (*reduceBeta)(int,int,int,int,int),void (*eliminateSusceptible)(int**, int,int,int),char*,char*,char*);
//Fake R functions
void sample(int**, int,int);
void swap(int*,int*);
void no_interventionSusceptible(int**,int,int,int);
int no_interventionBeta(int,int,int,int,int);
void interventionSusceptible(int**,int,int,int);
int interventionBeta(int,int,int,int,int);

#endif //COUNTERFACTUAL_H_
