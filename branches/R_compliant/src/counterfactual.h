#ifndef COUNTERFACTUAL_H_
#define COUNTERFACTUAL_H_

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))

void runCounterfactualAnalysis(char*,int*, int, int, float*, float*,char*,char*);
void runFullCounterfactualAnalysis(int*, int, int, float*, float*,char*,char*);
void runFastCounterfactualAnalysis(int*, int, int, float*, float*,char*,char*);
void constructTimeSeries(int* ,int, int,int (*reduceBeta)(int,int,int,int,int),void (*eliminateSusceptible)(int**, int,int,int),char*,char*,char*);
//Fake R functions
void sample(int**, int,int);
void swap(int*,int*);

#endif //COUNTERFACTUAL_H_
