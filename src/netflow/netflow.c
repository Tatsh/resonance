#include "netflow/netflow.h"

#include <stdio.h>
#include <stdlib.h>

// NTSC-U/C: 0x007b88d8, PAL: 0x007fc5d8
// Edge examinations counted across the search, statistics only.
static int eq = 0;

// NTSC-U/C: 0x007b88dc, PAL: 0x007fc5dc
// Augmentations applied, statistics only.
static int eex = 0;

// NTSC-U/C: 0x007b88e0, PAL: 0x007fc5e0
// Head of the free U queue, 2500 when the queue is empty.
static int iUnmatched = 0;

// One slot of the augmenting path search queue.
struct netflow_queue_entry {
    int u_vertex; // U vertex stored in this slot.
    int parent;   // Parent queue index, -1 for a root.
    int v_vertex; // V vertex used to arrive at this slot.
};

// NTSC-U/C: 0x007b88e8, PAL: 0x007fc5e8
// The search queue, 2500 slots of twelve bytes.
static struct netflow_queue_entry queue[NETFLOW_MAX_VERTICES];

// NTSC-U/C: 0x007bfe18, PAL: 0x00803b18
// The U vertices on the current search path.
static unsigned char visited[NETFLOW_MAX_VERTICES];

// NTSC-U/C: 0x00610880, PAL: 0x006514f0
// Reports an empty edge pool and returns.
static void Barf(const char *message) {
    fprintf(stderr, "%s\n", message);
}

// NTSC-U/C: 0x00610840, PAL: 0x006514b0
// Allocates an edge record, reporting an empty pool. The image routes the request through the
// shared reentrancy allocator; the toolchain allocator serves the same pool here.
static void *Alloc(unsigned size) {
    void *memory = malloc(size);
    if (memory == NULL) {
        Barf("Out of space");
    }
    return memory;
}

// NTSC-U/C: 0x00569c38, PAL: 0x005aa100
// Matches every U vertex with its first free neighbour and reports the count.
static void GreedyMatch(struct netflow_graph *graph, struct netflow_v_side *side) {
    int matched = 0;
    for (int u = 1; u <= graph->u_count; ++u) {
        struct netflow_edge *edge = graph->u[u].edges;
        while (edge != NULL && side->mate[edge->v] != 0) {
            edge = edge->next;
        }
        if (edge != NULL) {
            side->mate[edge->v] = u;
            graph->u[u].mate = edge->v;
            ++matched;
        }
    }
    const double percent =
        (double)((float)matched * 100.0f / (float)graph->u_count);
    printf("%d vertices in U were initially matched (%.1f%%).\n", matched, percent);
}

// NTSC-U/C: 0x0062be30, PAL: 0x0066c9c0
// Clears the search statistics and queues every unmatched U vertex.
static void Inits(struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    eq = 0;
    eex = 0;
    int slot = NETFLOW_MAX_VERTICES;
    for (int u = 1; u <= graph->u_count; ++u) {
        graph->u[u].cursor = graph->u[u].edges;
        visited[u] = 0;
        if (graph->u[u].mate == 0) {
            --slot;
            queue[slot].u_vertex = u;
        }
    }
    iUnmatched = slot;
}

// NTSC-U/C: 0x0062bb38, PAL: 0x0066c6c8
// Grows the matching along augmenting paths from every queued U vertex.
static void FindPaths(struct netflow_graph *graph, struct netflow_v_side *side) {
    while (iUnmatched < NETFLOW_MAX_VERTICES) {
        const int root = queue[iUnmatched].u_vertex;
        ++iUnmatched;
        queue[0].u_vertex = root;
        queue[0].parent = -1;
        int write = 0;
        int read = 0;
        int parent = -1;
        struct netflow_queue_entry *entry = queue;
        for (;;) {
            const int node = entry->u_vertex;
            ++parent;
            ++read;
            ++entry;
            ++eq;
            struct netflow_edge *edge = graph->u[node].edges;
            if (edge != NULL) {
                do {
                    ++eq;
                    const int vertex = edge->v;
                    const int mate = side->mate[vertex];
                    if (visited[mate] == 0) {
                        ++eq;
                        ++write;
                        queue[write].u_vertex = mate;
                        queue[write].parent = parent;
                        queue[write].v_vertex = vertex;
                        visited[mate] = 1;
                        struct netflow_edge *cursor = graph->u[mate].cursor;
                        while (cursor != NULL && side->mate[cursor->v] != 0) {
                            ++eq;
                            cursor = cursor->next;
                        }
                        graph->u[mate].cursor = cursor;
                        if (cursor != NULL) {
                            const int free_vertex = cursor->v;
                            side->mate[free_vertex] = mate;
                            graph->u[mate].mate = free_vertex;
                            int child = write;
                            int up = queue[child].parent;
                            while (up >= 0) {
                                const int up_vertex = queue[child].v_vertex;
                                const int up_mate = queue[up].u_vertex;
                                side->mate[up_vertex] = up_mate;
                                graph->u[up_mate].mate = up_vertex;
                                child = up;
                                up = queue[child].parent;
                                // Yes, the binary counts every step of the path, not each path.
                                ++eex;
                            }
                            for (int i = 0; i <= write; ++i) {
                                visited[queue[i].u_vertex] = 0;
                            }
                            // The binary abandons this root after one augmentation.
                            write = parent;
                            break;
                        }
                    }
                    edge = edge->next;
                } while (edge != NULL);
            }
            if (write >= read) {
                continue;
            }
            break;
        }
    }
}

void Init_U(struct netflow_graph *graph) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        graph->u[i].edges = NULL;
        graph->u[i].mate = 0;
    }
    graph->u_count = 0;
}

void Init_V(struct netflow_v_side *side) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        side->mate[i] = 0;
    }
    side->v_count = 0;
}

void AddEdge(int u, int v, struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    struct netflow_edge *forward = Alloc(sizeof(struct netflow_edge));
    struct netflow_edge *reverse = Alloc(sizeof(struct netflow_edge));
    forward->next = graph->u[u].edges;
    graph->u[u].edges = forward;
    forward->v = v;
    reverse->v = u;
}

void Match(struct netflow_graph *graph, struct netflow_v_side *side) {
    GreedyMatch(graph, side);
    Inits(graph, side);
    FindPaths(graph, side);
}
