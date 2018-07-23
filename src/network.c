#include <string.h>
#include <stdlib.h>
#include "network.h"

adjacency_list_t adjacencey_list(char**      , char****                 , int, int*);
adjacency_list_t adjacencey_list(char** nodes, char * *** adjacency_list, int N, int* degree){
  adjacency_list_t rc;
  rc.nodes = nodes;
  rc.N = N;
  rc.degree = degree;
  rc.adjacency_list = adjacency_list;
}

adjacency_list_t edge_list(char*** edges, int nedges){
  adjacency_list_t rc;
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
  
}
adjacency_list_t complete_graph(int N){
  adjacency_list_t rc;
  int i,j;
  rc.N = N;
  rc.nodes = malloc(N * sizeof(char*));
  rc.degree = malloc(N * sizeof(int));
  rc.adjacency_list = malloc(N * sizeof(char***));
  
  //Insert check about log_10(N) and NODE_NAME_LEN
  for(i = 0; i < N; ++i){
    rc.nodes[i] = malloc(NODE_NAME_LEN * sizeof(char));
    rc.nodes[i];
    for(j = 0; j < N; ++i){
      rc.adjacency_list[i][j] = &(rc.nodes[j]);
    }
  }
}

void free_adjacency_list(adjacency_list_t* ptr){
  int i = 0;
  for(i = 0; i < (*ptr).N; ++i){
    free((*ptr).nodes[i]);
    free((*ptr).adjacency_list[i]);
  }
  free((*ptr).degree);
  free((*ptr).nodes);
  free((*ptr).adjacency_list);
}

