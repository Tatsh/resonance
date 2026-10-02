#ifndef NETFLOW_NETFLOW_H
#define NETFLOW_NETFLOW_H

/**
 * Bipartite matching from the netflow package.
 *
 * The package is vendored into the image, which its diagnostics establish
 * ("Inconsistent matching between %d(U) and %d(V)" and "matching NOT maximum; augm. path:"). It is
 * upstream code and its bodies are not reconstructed. This header declares only the part that
 * Rnd::Mesh::AssignFlatVerts() uses, with the layouts that routine and the four entry points below
 * read and write. The upstream identifiers are not recovered, so every name here is inferred.
 *
 * Both vertex sets are numbered from 1, and a mate of 0 means unmatched.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Capacity of each vertex set, including the unused entry 0. */
enum { NETFLOW_MAX_VERTICES = 2500 };

/** One edge in the adjacency list of a U vertex. */
struct netflow_edge {
    struct netflow_edge *next; /*!< The next edge of the same U vertex, or null. */
    int v;                     /*!< The V vertex at the other end. */
};

/** One vertex of the U set. */
struct netflow_u_vertex {
    struct netflow_edge *edges; /*!< Adjacency list, most recently added edge first. */
    int reserved;               // +0x04 Not written by the entry points below.
    int mate;                   /*!< The matched V vertex, or 0. */
};

/** The U set and its adjacency lists. */
struct netflow_graph {
    struct netflow_u_vertex u[NETFLOW_MAX_VERTICES]; /*!< The U vertices, from index 1. */
    int u_count;                                     /*!< Number of U vertices in use. */
    int edge_count; /*!< Upper bound on the edge count, set by the caller. */
};

/** The V set. */
struct netflow_v_side {
    int mate[NETFLOW_MAX_VERTICES]; /*!< The matched U vertex of each V vertex, or 0. */
    int v_count;                    /*!< Number of V vertices in use. */
};

/**
 * Clear every adjacency list and mate of a graph, and its U count.
 *
 * @param graph The graph to clear.
 * @ghidraAddress 0x005e6508
 */
void netflow_graph_init(struct netflow_graph *graph);

/**
 * Clear every mate of a V set, and its V count.
 *
 * @param side The V set to clear.
 * @ghidraAddress 0x005e64e8
 */
void netflow_v_side_init(struct netflow_v_side *side);

/**
 * Add an edge from a U vertex to a V vertex.
 *
 * Prepends the edge to the adjacency list of u. The routine also allocates a second record for
 * the reverse direction, stores u in it, and links it nowhere; side is not read.
 *
 * @param u The U vertex, from 1.
 * @param v The V vertex, from 1.
 * @param graph The graph that owns u.
 * @param side The V set.
 * @ghidraAddress 0x005e6538
 */
void netflow_add_edge(int u, int v, struct netflow_graph *graph, struct netflow_v_side *side);

/**
 * Find a maximum matching of graph against side.
 *
 * Seeds a greedy matching, reports its size, and then grows it along augmenting paths. The result
 * is left in the mate members of both sets.
 *
 * @param graph The U set and its edges.
 * @param side The V set.
 * @ghidraAddress 0x00569bf0
 */
void netflow_build_matching(struct netflow_graph *graph, struct netflow_v_side *side);

#ifdef __cplusplus
}
#endif

#endif
