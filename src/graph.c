/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#include "bstree.h"
#include "linked_list.h"
#include "graph.h"
#include <stdbool.h>
#include <string.h>

#define TEST_EDGE_ADDITIONS

Graph_t *Graph_Init(Data_Initializer_cb initializer, Data_Uninitializer_cb uninitializer, Comparison_cb vertex_data_cmp_cb, size_t vertex_data_size) {
  Graph_t *ret;
  if (NULL==initializer) {
    if (NULL!=uninitializer) {
      return NULL;
    }
  }
  if (0UL == vertex_data_size)
    return NULL;
  if (NULL == vertex_data_cmp_cb)
    return NULL;
  ret = malloc(sizeof(Graph_t));
  if (ret == NULL)
    return NULL;

  ret->vertex_ct = 0UL;
  ret->vertices = NULL;
  ret->data_initer = initializer;
  ret->data_uniniter = uninitializer;
  ret->vertex_data_map = BinaryTree_Create(malloc, free, initializer, uninitializer, vertex_data_cmp_cb, vertex_data_size+sizeof(void*), NULL);
  ret->vertex_data_size = vertex_data_size;
  return ret;
}

bool Graph_Has_Edge(const Graph_t *graph, int src_vertex, int dst_vertex) {
  if (!graph || 0 > src_vertex || 0 > dst_vertex)
    return false;
  GraphEdge_LL_t *ll = &graph->vertices[src_vertex].adj_list;
  if (0UL==ll->nmemb)
    return false;
  LL_FOREACH(GraphEdge_LL_Node_t *cur, cur, ll) {
    if (cur->data.dst_idx < dst_vertex)
      continue;
    else if (cur->data.dst_idx==dst_vertex)
      return true;
    else
      break;
  }
  return false;
}

int Graph_Add_Vertex(Graph_t *graph, const void *vertex_data) {
  if (!graph || !vertex_data)
    return -1;
  uint8_t *vmap_entry = malloc(sizeof(uint8_t)*(graph->vertex_data_size + sizeof(void*)));
  memcpy(vmap_entry, vertex_data, graph->vertex_data_size);
  memset(&vmap_entry[graph->vertex_data_size], 0, sizeof(void*));
  if (BinaryTree_Contains(graph->vertex_data_map, vmap_entry)) {
    free(vmap_entry);
    return -1;
  }
  int newnode_idx = graph->vertex_ct;
  GraphNode_t *graphnodes, *new;
  assert(NULL != (graphnodes = realloc(graph->vertices, sizeof(GraphNode_t)*(newnode_idx+1))));
  graph->vertices = graphnodes;

  
  new = &graphnodes[newnode_idx];
  *(int*)(&vmap_entry[graph->vertex_data_size]) = newnode_idx;
  new->idx = newnode_idx;
  new->adj_list = LL_INIT(GraphEdge);
  new->data = malloc(graph->vertex_data_size);
  if (graph->data_initer)
    graph->data_initer(new->data, vertex_data);
  else
    memcpy(new->data, vertex_data, graph->vertex_data_size);
  assert(BinaryTree_Insert(graph->vertex_data_map, vmap_entry));
  ++(graph->vertex_ct);
  free(vmap_entry);
  return newnode_idx;
}

bool Graph_Update_Vertex_Data(Graph_t *graph, int vertex, const void *new_data) {
  if (!graph || !new_data)
    return false;
  if ((const size_t)vertex >= graph->vertex_ct)
    return false;
  if (BinaryTree_Contains(graph->vertex_data_map, new_data))
    return false;

  void *vert_data = graph->vertices[vertex].data;

  assert(BinaryTree_Remove(graph->vertex_data_map, vert_data));
  if (graph->data_uniniter) {
    graph->data_uniniter(vert_data);
  }
  free(vert_data);
  vert_data = NULL;

  uint8_t *new_data_vmap_entry = malloc(sizeof(uint8_t)*(graph->vertex_data_size+sizeof(void*)));
  memcpy(new_data_vmap_entry, new_data, graph->vertex_data_size);
  *(int*)(&new_data_vmap_entry[graph->vertex_data_size]) = vertex;

  BinaryTree_Insert(graph->vertex_data_map, new_data_vmap_entry);
  free(new_data_vmap_entry);
  new_data_vmap_entry = NULL;
  assert(BinaryTree_Contains(graph->vertex_data_map, new_data));
  vert_data = graph->vertices[vertex].data = malloc(graph->vertex_data_size);
  
  if (graph->data_initer) {
    graph->data_initer(vert_data, new_data);
  } else {
    memcpy(vert_data, new_data, graph->vertex_data_size);
  }
  {
    void *vertex_data = graph->vertices[vertex].data,
          *map_data = BinaryTree_Retrieve(graph->vertex_data_map, vertex_data);
    int i = *(int*)(&((uint8_t*)map_data)[graph->vertex_data_size]);
    assert(NULL!=vertex_data && NULL!=map_data);
    assert(i==vertex);
  }
  
  return true;
  
   
}

bool Graph_LazyDelete_Vertex(Graph_t *graph, int vert_idx) {
  if (!graph || 0 > vert_idx)
    return false;
  if ((const size_t)vert_idx >= graph->vertex_ct)
    return false;
  const size_t VERT_IDX = vert_idx, VERT_COUNT = graph->vertex_ct;
  GraphNode_t *delnode = &graph->vertices[vert_idx];
  GraphEdge_LL_t *edges;
  GraphEdge_LL_Node_t *cur, *prev;
  void *vdata = delnode->data;
  assert(BinaryTree_Contains(graph->vertex_data_map, vdata));
  assert(BinaryTree_Remove(graph->vertex_data_map, vdata));
  if (graph->data_uniniter) {
    graph->data_uniniter(vdata);
  }
  free(vdata);
  vdata = NULL;
  edges = &delnode->adj_list;
  LL_CLOSE(GraphEdge, edges);
  delnode->data = NULL;
  for (size_t i = 0; VERT_COUNT > i; ++i) {
    if (VERT_IDX==i)
      continue;
    edges = &graph->vertices[i].adj_list;
    if (0UL==edges->nmemb) {
      assert(NULL == edges->head && NULL == edges->tail);
      continue;
    }
    prev = edges->head;
    if ((const int)prev->data.dst_idx==vert_idx) {
      edges->head = prev->next;
      --(edges->nmemb);
      free(prev);
      prev = NULL;
      if (0UL==edges->nmemb) {
        edges->head = edges->tail = NULL;
      }
      continue;
    }
    if (1UL==edges->nmemb ||
        (const int)prev->data.dst_idx > vert_idx)
      continue;
    for (cur = prev->next; NULL!=cur; prev = cur, cur = cur->next) {
      if ((const int)cur->data.dst_idx < vert_idx) {
        continue;
      } else if ((const int)cur->data.dst_idx == vert_idx) {
        prev->next = cur->next;
        --(edges->nmemb);
        if ((const GraphEdge_LL_Node_t*)edges->tail==cur) {
          edges->tail = prev;
#ifdef _DEBUG_BUILD_
          assert(NULL==cur->next && NULL==prev->next && NULL!=prev);
#else
          assert(NULL==cur->next);
#endif
        }
        free(cur);
        break;
      } else {
        break;
      }
    } 
  }
  return true;

}

bool Graph_Add_Edge(Graph_t *graph, int src_vertex, int dst_vertex, int weight) {
  if (!graph)
    return false;
  if (src_vertex < 0 || (unsigned)src_vertex >= graph->vertex_ct)
    return false;
  if (dst_vertex < 0 || (unsigned)dst_vertex >= graph->vertex_ct)
    return false;
  GraphNode_t *src = graph->vertices + src_vertex;
  GraphEdge_LL_t *adjs = &(src->adj_list);
  GraphEdge_t edge = (GraphEdge_t){.weight = weight, .dst_idx = dst_vertex};
  if (adjs->nmemb==0UL) {
    LL_NODE_PREPEND(GraphEdge, adjs, edge);
    return true;
  }
  if (dst_vertex < adjs->head->data.dst_idx) {
    LL_NODE_PREPEND(GraphEdge, adjs, edge);
    return true;
  }
  LL_NODE_VAR_INITIALIZER(GraphEdge, node);

  LL_NODE_VAR_INITIALIZER(GraphEdge, prev) = NULL;
  LL_FOREACH(LL_NO_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
    if (dst_vertex > node->data.dst_idx)  {
      prev = node;
      continue;
    }
    if (dst_vertex == node->data.dst_idx)
      return false;  // return false. Make caller use update weight to update existing edge's weight
    break;
  }
  // if node is NULL, then foreach never preemptively broke, so idx of dst_vertex 
  // is largest of the bunch. Append it as new tail using LL_APPEND
  if (!node) {
    assert(prev == adjs->tail); 
    LL_NODE_APPEND(GraphEdge, adjs, edge);
    return true;
  }

  GraphEdge_LL_Node_t *insert = malloc(sizeof(*insert));
  insert->next = node;
  prev->next = insert;
  insert->data = edge;
  ++(adjs->nmemb);
  return true;
}




bool Graph_Update_Edge_Weight(Graph_t *graph, int src_vertex, int dst_vertex, int new_weight) {
  if (!graph)
    return false;
  if (src_vertex < 0 || (unsigned)src_vertex >= graph->vertex_ct)
    return false;
  if (dst_vertex < 0 || (unsigned)dst_vertex >= graph->vertex_ct)
    return false;
  GraphEdge_LL_t *adjs = &(graph->vertices[src_vertex].adj_list);
  if (adjs->nmemb==0UL)
    return false;
  int diff;
  LL_FOREACH(LL_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
    diff = dst_vertex - node->data.dst_idx;
    if (0 < diff) {
      continue;
    } else if (0 > diff) {
      return false;
    }
    // ELSE: diff==0 aka: target edge found
    node->data.weight = new_weight;
    return true;
  }
  return false;
}

void Graph_Close(Graph_t *graph) {
  size_t vct = graph->vertex_ct;
  GraphNode_t *curvert;
  GraphEdge_LL_t *adjs;
  for (unsigned i = 0; i < vct; ++i) {
    curvert = &(graph->vertices[i]);
    adjs = &(curvert->adj_list);
    LL_CLOSE(GraphEdge, adjs);
    assert(adjs->nmemb == 0UL && adjs->tail == adjs->head && adjs->head == NULL);
    if (NULL==curvert->data)
      continue;
    if (graph->data_uniniter) {
      graph->data_uniniter(curvert->data);
    }
    free(curvert->data);
  }
  free(graph->vertices);
  BinaryTree_Destroy(graph->vertex_data_map);
  free(graph);
}




GraphEdge_LL_t *Graph_Get_Vertex_Adjacents(Graph_t *graph, int vertex_id) {
  if (!graph)
    return NULL;
  if ((unsigned)vertex_id >= graph->vertex_ct)
    return NULL;
  return &(graph->vertices[vertex_id].adj_list);
}

GraphNode_t *Graph_Get_Vertex(const Graph_t *graph, const void *vertdata) {
  if (!graph || !vertdata)
    return NULL;
  
  uint8_t *vmap_entry = BinaryTree_Retrieve(graph->vertex_data_map, (void*)vertdata);
  if (!vmap_entry)
    return NULL;
  vmap_entry += graph->vertex_data_size;
  return &(graph->vertices[*(int*)vmap_entry]);
}

bool Graph_Remove_Edge(Graph_t *graph, int src_vertex, int dst_vertex) {
  if (NULL==graph)
    return false;
  if ((const size_t)src_vertex >= graph->vertex_ct)
    return false;
  if ((const size_t)dst_vertex >= graph->vertex_ct)
    return false;

  GraphEdge_LL_t *ll = &(graph->vertices[src_vertex].adj_list);
  const GraphEdge_LL_Node_t *TAIL = ll->tail;
  GraphEdge_LL_Node_t *cur, *prev;
  if ((size_t)0 == ll->nmemb)
    return false;

  if ((const int)dst_vertex == ll->head->data.dst_idx) {
    cur = ll->head;
    ll->head = cur->next;
    free((void*)cur);
    --ll->nmemb;
    return true;
  }
  prev = ll->head;
  for (cur = prev->next; NULL!=cur; prev = cur, cur = cur->next) {
    if ((const int)dst_vertex != cur->data.dst_idx)
      continue;
    if (TAIL == cur) {
      ll->tail = prev;
      prev->next = NULL;
    } else {
      prev->next = cur->next;
    }
    free((void*)cur);
    --ll->nmemb;
    return true;
  }
  return false;
}
