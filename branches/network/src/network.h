#ifndef COUNTERFACTUAL_NETWORK_H_
#define COUNTERFACTUAL_NETWORK_H_

#include "types.h"

#endif //COUNTERFACTUAL_NETWORK_H_

typedef struct {
  char** nodes;
  // char* *** adjacency_list; //list of lists of char* pointers
  person_t** adjacency_list; //list of list of indices to nodes;
  person_t N; // number of nodes
  person_t* degree; // number of nodes
} adjacency_list_t;

void adjacencey_list(adjacency_list_t*, char**, person_t**, person_t, person_t*);
void edge_list(adjacency_list_t*, char***, person_t);
void complete_graph(adjacency_list_t*, person_t);
void free_adjacency_list(adjacency_list_t*);
