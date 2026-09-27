#include "netflow/netflow.h"

#include <cstddef>

extern "C" {

// The edge builder requests eight byte records from this allocator.
void *NetflowAllocChecked(unsigned nSize);

// The matcher runs this greedy pass first and reports the result.
void NetflowReportInitialMatching(struct netflow_graph *graph, struct netflow_v_side *side);

// The matcher runs this pool pass between the greedy pass and the search.
void NetflowInitMatchingPool(struct netflow_graph *graph, struct netflow_v_side *side);

// The matcher runs this augmenting path search last.
void NetflowFindAugmentingPaths(struct netflow_graph *graph, struct netflow_v_side *side);

} // extern "C"

// 0x005e6508
// Clear every adjacency list and mate, and clear the U count.
void netflow_graph_init(struct netflow_graph *graph) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        graph->u[i].edges = nullptr;
        graph->u[i].mate = 0;
    }
    graph->u_count = 0;
}

// 0x005e64e8
// Clear every mate, and clear the V count.
void netflow_v_side_init(struct netflow_v_side *side) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        side->mate[i] = 0;
    }
    side->v_count = 0;
}

// 0x005e6538
// Prepend a record for the edge, and allocate a reverse record recording u.
void netflow_add_edge(int u, int v, struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    struct netflow_edge *pForward = static_cast<struct netflow_edge *>(NetflowAllocChecked(8));
    struct netflow_edge *pReverse = static_cast<struct netflow_edge *>(NetflowAllocChecked(8));
    pForward->next = graph->u[u].edges;
    graph->u[u].edges = pForward;
    pForward->v = v;
    pReverse->v = u;
}

// 0x00569bf0
// Seed a greedy matching, initialise the pool, and grow along augmenting paths.
void netflow_build_matching(struct netflow_graph *graph, struct netflow_v_side *side) {
    NetflowReportInitialMatching(graph, side);
    NetflowInitMatchingPool(graph, side);
    NetflowFindAugmentingPaths(graph, side);
}
