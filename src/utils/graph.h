// Copyright 2014 asarcar Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// 
// Class Graph:
// DESCRIPTION:
//   (i) ADT represents a graph.
//   (ii) The data structure choices were optimized assuming the following:
//   (ii.a) The graph is neither very sparse nor very dense 
//         (i.e. 20% < edge_density < 60%).
//   (ii.b) The scale of the graph (# of vertices & edges) are not in the 
//          "big data" range (ie low enough for entire graph to fit in the 
//          memory footprint of few megabytes.
//   (ii.c) The vertices of the graph are never addded/removed. Thus:
//          The vertices are stored in a simple vertex. 
//          The adjacencies are conceptually realized by a N^2 bitmaps.
// 
// EXCEPTION HANDLING: For now we are NOT going to try, catch, throw, 
//                     any exceptions.
// TEMPLATES/GENERICS:
// 
// TEMPLATES
//   Original intent was any template class can be stored in the 
//   vertex of the graph. However, templates force all code to get 
//   sucked into a header file or a cxx file that is include. I have
//   abandoned that idea until I have a better sense of how to enforce
//   modularity and code separation.
//   The edge cost is double. This may be changed to a genreric type later.
// 
// ITERATORS
//   No STL type iterators were used to traverse the vertices or edges of 
//   a graph. This is TBD based on my familiarity with that style.
//   
// EXAMPLE USAGE:
//   hexgame::utils::Graph g(hexgame::utils::Graph::DIRECTED)
//           creates a directed graph with other default parameter:
//           50 vertices, edges created with probability 0.4, 
//           edge costs are chosen randomly ranging from 1.0 to 10.0.  
//   hexgame::utils::Graph g(hexgame::utils::Graph::DIRECTED, 10)
//           creates an directed graph with 10 vertices and rest default
//           parameters: 0.4 edge density & distance range 1.0 to 10.0
//   hexgame::utils::Graph 
//           g(hexgame::utils::Graph::UNDIRECTED, 20, 0.5, 2.0, 4.0)
//           creates an undirected graph with 20 vertices, 0.5
//           edge density & distance range 2.0 to 4.0
//   hexgame::utils::Graph g("file_name")
//           creates an undirected graph with all parameters as provided
//           in the file
//   
// REPRESENTATION: 
// 1. Vertex Representation:
// 1.a. Namespace: non-negative integers 
//      (i.e. Vertex a, Vertex b, ... are given a unique non -ve 
//      integer ID (0, 1, ...)
// 1.b. Data Structure: vector<int>. Assuming the graph had fixed number of 
//      vertices, we could have chosen array class (C++11). However, we 
//      chose vector to allow dynamic insertion/removal of vertices.
// 1.c. Meta Data: 
//      (i) TBD Name: Each vertex stores internally a name of the vertex.
//              This is redundant information. But it allows one 
//              to easily provide a human readable name of a vector.  
//      (ii) # edges: density of edges coming out from the vertex.
// 1.d. TBD: Validity of Vertex: bitmap represents validity of a vertex i. 
//      bitmap sized: # of vertices (N)
//      (TBD: round N to nearest power of 2 if vertex can be deleted from graph)
//      The validity representation of bitmaps for vertices allows one to 
//      delete vertex i from a graph.
//
// 2. Edge Representation:
// 2.a. Namespace: Edges are ordered tuple of non-negative 
//      integers, where the first integer in the tuple represents the 
//      originating node of the edge and the second integer in the tuple
//      represents the terminating node of the edge. 
//      An UNDIRECTED edge between Vx and Vy: {Vx, Vy} and {Vy, Vx}.
//      As edges are undirected, conceptually edge {Vx, Vy} == edge {Vy, Vx}.
//      To save space we would just store one of the tuples.
// 2.b. Data Structure: unordered_map
// 2.c. Meta Data:
//      REM (i) Name: Each edge stores internally a name. 
//              Redundant information. Provides readable name for an edge.
//      (ii) Cost: A +ve integer designating the cost of traversing the edge.
// 2.d. Adjacency Representation:
// 2.d.a. Bitmap: vector<unsigned int> v(N*N) where N is # of vertices 
//        (TBD: round N to nearest power of 2 if vertex can be deleted from graph)
//        If Edge (Vi, Vj) exists then bit N*i+j == TRUE. 
//        For undirected graphs bit N*j + i is also TRUE.
//        Note that the bitmap representation is space efficient but is not 
//        dynamic i.e. cost of inserting or deleting vertices from the graph 
//        is high. This needs to be addressed later and the data structure
//        representation would be hidden via accessor methods.
// 

#ifndef _GRAPH_H_
#define _GRAPH_H_

// Standard C++ Headers
#include <fstream>          // std::ifstream & std::ofstream
#include <iostream>         // std::cout
#include <limits>           // std::numeric_limits
#include <string>           // std::string
#include <unordered_map>    // std::map hash data store of edges
#include <utility>          // std::pair
#include <vector>           // std::vector
// Standard Headers
#include <cassert>          // assert
// Google Headers
// Local Headers
#include "utils/basictypes.h"
#include "utils/bit_set.h"

namespace hexgame { namespace utils {
//-----------------------------------------------------------------------------
// Forward Declarations
template <typename GCost> 
class Graph;

template <typename GCost> 
std::ostream& operator << (std::ostream& os, const Graph<GCost> &g);
// End of Forward Declarations

using GVertexId = uint32_t;
using GEdgeId   = std::pair<GVertexId, GVertexId>; // edge id
using GFileName = std::string;

// Type of Graph: Directed or Undirected
enum class GEdgeType {UNDIRECTED = 0, DIRECTED};

// Iterator Options: BFS ORDER, DFS ORDER
enum class GVertexIterType {DFS_ORDER=0, BFS_ORDER};

template <typename GCost>
class GVertexCIter;

// template <typename GCost>
// typedef class GVertexCIter<GCost> vconst_iterator; //using construct not working

using GVertexIterSeed     = std::vector<GVertexId>;
using GVertexIterValue    = GVertexId;
using GVertexIterConstReference= const GVertexId&;

template <typename GCost>
class GEdgeCIter;

// Used for the hashing function to store edges in unordered map
struct PairKeyHash {
  inline std::size_t operator() (const GEdgeId &key) const {
    std::hash<GVertexId> hasher;
    return (hasher(hasher(key.first) ^ hasher(key.second)));
  }
};

// Graph container allows iteration of edges of a given vertex
// Default Vertex is assumed to be vertex_id 0 (ie first vertex)
template <typename GCost = uint32_t>
using GEdgeContainer = typename std::unordered_map<GEdgeId, 
                                                   GCost, 
                                                   PairKeyHash>;

template <typename GCost = uint32_t>
using GEdgeContainerIter = typename GEdgeContainer<GCost>::const_iterator;

template <typename GCost = uint32_t>
using GEdgeIterValue    = typename GEdgeContainer<GCost>::value_type;

template <typename GCost = uint32_t>
using GEdgeIterConstReference= typename GEdgeContainer<GCost>::const_reference;

// Constant Limit Values
template <typename GCost = uint32_t>
constexpr inline GVertexId kGMaxVertexId(void) {
  return GVertexId{1U << 10};
} 
template <typename GCost = uint32_t>
constexpr inline GCost     kGInfinityCost(void) {
  return GCost{std::numeric_limits<GCost>::max()};
}
template <typename GCost = uint32_t>
constexpr inline GCost     kGMinCost(void) {
  return GCost{1};
}


template <typename GCost = uint32_t>
class Graph {
 public:
  
  // Constructors
  // Init an undirected or directed graph (based on type) with num_vertices.
  // The edges created with probability of edge_density.
  // The edge cost is chosen with equal prob in range mix to max range
  explicit Graph(const GEdgeType type = GEdgeType::UNDIRECTED,
                 const uint32_t num_vertices=50, 
                 const double   edge_density=0.5, 
                 const GCost    min_distance_range=1, 
                 const GCost    max_distance_range=10,
                 // true when called from automated test scripts
                 const bool     auto_test = false); 
  
  // Init an undirected graph (note that sample file data provided in HW does not 
  // specify directed or undirected: hence we will always assume undirected graph)
  // based on content of the file
  explicit Graph(std::string file_name);
  
  // Destructor
  virtual ~Graph() {}

  // Prevent unintended bad usage: 
  // Disallow: copy ctor/assignable or move ctor/assignable (C++11)
  Graph(const Graph &) = delete;
  Graph(Graph &&) = delete; // C++11 only
  void operator=(const Graph &) = delete;
  void operator=(Graph &&) = delete; // C++11 only

  // METHODS:
  // == operator used when comparing iterators
  // Two graphs are "equal" when they are pointing to the same location 
  bool operator ==(const Graph& o) const { return (this == &o); }
  // V (G): returns the number of vertices in the graph
  inline uint32_t get_num_vertices() const { 
    return (_num_vertices); 
  }

  // E (G): returns the number of "unique edges" in the graph (unique: we count
  // the two ends of an undirected graph <v1, v2> and <v2, v1> as one egde)
  // adjacent (G, x, y): tests whether there is an edge from node x to node y.
  // neighbors (G, x): lists all nodes y such that there is an edge from x to y.
  inline uint32_t get_num_edges() const { 
    return (this->_edges.size()); 
  }

  // add (g, x, y): adds to G the edge from x to y, if it is not there.
  // Creates an edge (adjacency) in the graph with edge and (optional) value
  void add_edge(GVertexId v1, GVertexId v2, GCost value =kGMinCost<GCost>());

  // delete (G, x, y): removes the edge from x to y, if it is there.
  // Removes an edge (adjacency) in the graph with edge and (optional) value
  void del_edge(GVertexId v1, GVertexId v2);

  // get_edge_value( G, x, y): returns the value associated to the edge (x,y).
  // non existent edge: return infinity cost
  // bad arg check: non existent edge automatically checked by container
  GCost get_edge_value(GVertexId v1, GVertexId v2) const;
  
  // set_edge_value (G, x, y, v): sets the value to the edge (x,y) to v.
  // bad arg check: non existent edge automatically checked by container
  void set_edge_value(GVertexId v1, GVertexId v2, 
                      GCost value=kGMinCost<GCost>);

  // Dumps the graph to the file "file_name"
  void output_to_file(std::string file_name);

  // helper function to allow chained cout cmds: example
  // cout << "The graph: " << endl << g << endl << "---------" << endl;
  friend std::ostream& operator << <>(std::ostream& os, const Graph<GCost> &g);

 public:
  // ITERATORS: 
  // Vertex Iterator: 
  friend class GVertexCIter<GCost>; 
  GVertexCIter<GCost> vcbegin(GVertexIterType itype, 
                              const GVertexId &seed_vid) const;
  GVertexCIter<GCost> vcbegin(GVertexIterType itype, 
                              const GVertexIterSeed& seed_vid) const;
  GVertexCIter<GCost> vcend(GVertexIterType) const;
  
  static const uint32_t kNumVertexIterTypes{2};
  // Display VertexIterType string
  // Display String: State Enumberator
  static inline const std::string&
  str_vertex_iter_type(const GVertexIterType& iType) {
    static std::array<std::string, kNumVertexIterTypes>
        VertexIterTypeStr = {{"\"DFS_ORDER\"", "\"BFS_ORDER\""}};
    return VertexIterTypeStr[static_cast<std::size_t>(iType)];
  }

  // Edge Iterator Given a source vertex
  friend class GEdgeCIter<GCost>;

  GEdgeCIter<GCost> ecbegin(GVertexId vid) const;
  GEdgeCIter<GCost> ecend(GVertexId vid) const;

 protected:
  // get_next_nbr: provide the first nbr vertex that is available
  // immediately from or after the passed nbr_vid 
  virtual GVertexId get_next_nbr(GVertexId vid, GVertexId nbr_vid) const {
    GVertexId vid_end = get_num_vertices();
    assert(vid < vid_end);
    for (GVertexId vid2 = nbr_vid; vid2 < vid_end; ++vid2) {
      if (isset_adjmap(std::make_pair(vid, vid2)) == true)
        return vid2;
    }
    return kGMaxVertexId<GCost>();
  }
  // Tests whether edge eid is present in the adjacency map
  inline bool isset_adjmap(const GEdgeId &eid) const {
    // For Undirected graph both edge {v1, v2} and {v2, v1} would be present
    // So absence of any one of the two signifies the edge is not present
    return _adjmap.is_bit_set(pos(eid.first, eid.second));
  }


 private:
  //! Fixed seed generates predictable MC runs when running test SW or debugging
  const static uint32_t kFixedCostSeedForRandomEngine = 13607; 
  const static uint32_t kFixedEdgePresenceSeedForRandomEngine = 24718;

  // Graph type
  GEdgeType _type;

  // Number of Vertices in graph
  uint32_t _num_vertices;

  // Vector of Vertices:
  // Vertex Vi Presence is realized via Bit_Set: 
  // one bit for every Vertex possible in the graph
  // bit position for Vi is bit position i: where 
  // N = # of vertices 
  // (TBD: round N to nearest power of 2 if vertex can be deleted from graph)
  // BitMap is unused for now as we do not allow vertex deletions from graph
  // typedef std::vector<gword_t> gvector_valid_map_t; 
  // gvector_valid_map_t _vectorvalidmap;

  // Unordered Map (Hash Map) of Edges
  // Key: Pair i.e. GEdgeId is a tuple {vertex1, vertex2}
  // Value: GCost
  // unordered_map of complex keys require definition of 
  // 1) hash i.e. operator() - we need to define this operator
  // 2) operator== Pair already has operator== defined. 
  GEdgeContainer<GCost>  _edges;

  // Adjacency Map: 
  // Edge Presence is realized via BitSet: one bit for every edge 
  // possible in the graph: bit position for (Vi,Vj) is bit position 
  // N*i + j where:
  BitSet _adjmap;

  // Given an edge: find the adjacency word position
  inline uint32_t pos(uint32_t svid, uint32_t dvid) const {
    return (this->get_num_vertices()*svid + dvid);
  }

  // private utilities on bitmap
  // Set the presence of the edge eid in the adjacency map
  inline void set_adjmap(const GEdgeId &eid) {
    _adjmap.set_bit(pos(eid.first, eid.second));
    // For Undirected graph edge {v1, v2} is equivalent to {v2, v1}
    if (this->_type == GEdgeType::UNDIRECTED) {
      _adjmap.set_bit(pos(eid.second, eid.first));
    }
    return;
  }
  // Clear the presence of the edge eid in the adjacency map
  inline void clr_adjmap(const GEdgeId &eid) {
    _adjmap.clr_bit(pos(eid.first, eid.second));
    // For Undirected graph edge {v1, v2} is equivalent to {v2, v1}
    if (this->_type == GEdgeType::UNDIRECTED) {
      _adjmap.clr_bit(pos(eid.second, eid.first));
    }
    return;
  }
};

// Suppress implicit instantiation
extern template std::ostream& 
operator << <uint32_t>(std::ostream& os, const Graph<uint32_t> &g);

extern template class Graph<uint32_t>;

// Create few aliases 
using GraphI = Graph<uint32_t>;

//-----------------------------------------------------------------------------
} } // namespace hexgame { namespace utils {

#endif // _GRAPH_H_
