#ifndef COUNTERFACTUAL_NETWORK_H_
#define COUNTERFACTUAL_NETWORK_H_

#include "types.h"

#endif //COUNTERFACTUAL_NETWORK_H_

typedef struct {
  char** nodes;
  char* *** adjacency_list; //list of lists of char* pointers
  int N; // number of nodes
  int* degree; // number of nodes
} adjacency_list_t;

adjacency_list_t adjacencey_list(char**, char****, int, int*);
adjacency_list_t edge_list(char***, int);
adjacency_list_t complete_graph(int);
void free_adjacency_list(adjacency_list_t*);
