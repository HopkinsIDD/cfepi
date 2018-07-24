#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "network.h"

#include <R.h>
//#include <Rmath.h>
//#include <Rinternals.h>
#include <R_ext/Print.h>
//#include <Rdefines.h>

void adjacencey_list(adjacency_list_t *output, char** nodes, person_t** adjacency_list, person_t N, person_t* degree){
  (*output).nodes = nodes;
  (*output).N = N;
  (*output).degree = degree;
  (*output).adjacency_list = adjacency_list;
}

void edge_list(adjacency_list_t *output, char*** edges, person_t nedges){
  printf("Start edge_list\n");
  int edge_id = 0;
  int vertex_id = 0;
  int v0_unique,v1_unique,nvertex;
  int* degree;
  char** vertices;
  nvertex = 0;
  vertices = malloc(2*nedges * sizeof(char*));
  for(edge_id = 0; edge_id < nedges; ++edge_id){
    // edges[edge_id][0]
    v0_unique = 1;
    v1_unique = 1;
    for(vertex_id = 0; vertex_id < nvertex; ++vertex_id){
      if(edges[edge_id][0] == vertices[vertex_id]){
        v0_unique = 0;
      }
      if(edges[edge_id][1] == vertices[vertex_id]){
        v1_unique = 0;
      }
    }
    
    if(v0_unique > 0){
      vertices[vertex_id] = malloc(NODE_NAME_LEN * sizeof(char));
      strcpy(edges[edge_id][0],vertices[vertex_id]);
      ++nvertex;
    }
    if(v1_unique > 0){
      vertices[vertex_id] = malloc(NODE_NAME_LEN * sizeof(char));
      strcpy(edges[edge_id][1],vertices[vertex_id]);
      ++nvertex;
    }
  }
  free(vertices);
  Rf_error("This function not yet written.");
  printf("End edge_list\n");
}
void complete_graph(adjacency_list_t *output,person_t N){
  printf("Start complete_graph\n");
  person_t i,j;
  (*output).N = N;
  (*output).nodes = malloc(N * sizeof(char*));
  if((*output).nodes == NULL){
    Rf_error("Failed to allocate nodes\n");
  }
  (*output).degree = malloc(N * sizeof(person_t));
  if((*output).degree == NULL){
    Rf_error("Failed to allocate degrees\n");
  }
  (*output).adjacency_list = malloc(N * sizeof(person_t*));
  if((*output).adjacency_list == NULL){
    Rf_error("Failed to allocate adjacency_list\n");
  }
  
  //Insert check about log_10(N) and NODE_NAME_LEN
  for(i = 0; i < N; ++i){
    (*output).nodes[i] = malloc(NODE_NAME_LEN * sizeof(char));
    (*output).nodes[i];
    (*output).degree[i] = N;
    if((*output).nodes[i] == NULL){
      Rf_error("Failed to allocate node element %d\n",i);
    }
    (*output).adjacency_list[i] = malloc(N * sizeof(person_t));
    if((*output).adjacency_list[i] == NULL){
      Rf_error("Failed to allocate adjacency_list element %d\n",i);
    }
    for(j = 0; j < N; ++j){
      (*output).adjacency_list[i][j] = j;
    }
  }
  printf("End complete_graph\n");
}

void free_adjacency_list(adjacency_list_t* ptr){
  int i = 0;
  if((*ptr).N <= 0){
    Rf_error("Cannot free a graph with no nodes.\n");
  }
  Rf_warning("Freeing %d nodes.\n",(*ptr).N);
  for(i = 0; i < (*ptr).N; ++i){
    printf("%d",i);
    fflush(stdout);
    free((*ptr).nodes[i]);
    free((*ptr).adjacency_list[i]);
  }
  free((*ptr).degree);
  free((*ptr).nodes);
  free((*ptr).adjacency_list);
}

