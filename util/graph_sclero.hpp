#ifndef UTIL_GRAPH_H
#define UTIL_GRAPH_H

#include <functional>
#include <memory>
#include <numeric>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/types/span.h"
#include "util/binary_search.hpp"
#include "util/cuckoo.hpp"
#include "util/dynarray.hpp"
#include "util/fastio.hpp"
#include "util/serialize.hpp"


template <typename index_t>
struct complex_graph_node {
  index_t index;
  bool in_S;
  bool in_N;
  bool deleted;

  cuckoo_hash_set<index_t> neighbors;
};


template <typename index_t = uint32_t>
class graph_t {
 public:
  // TODO(veluca): switch to CSR format?
  using edges_t = std::vector<std::vector<node_t>>;
  using Builder = std::function<std::unique_ptr<graph_t>(node_t, const edges_t&,
                                                         const labels_t&)>;
  graph_t() {}  // For deserialization

  graph_t(size_t N, const edges_t& edg)
      : N_(N), edges_(N) {
    for (size_t i = 0; i < N; i++) {
      edges_[i].init(edg[i]);
    }
  }

  size_t size() const { return N_; }
  size_t degree(node_t i) const { return edges_[i].size(); }
  node_t fwd_degree(node_t n) const { return fwd_neighs(n).size(); }
  const binary_search_t<node_t>& neighs(node_t i) const { return edges_[i]; }

  const absl::Span<const node_t> fwd_neighs(node_t n) const {
    auto beg = edges_[n].upper_bound(n);
    auto end = edges_[n].end();
    return absl::Span<const node_t>(beg, end - beg);
  }

  bool are_neighs(node_t a, node_t b) const { return edges_[a].count(b); }

  /**
   *  Node new_order[i] will go in position i.
   */
  std::unique_ptr<graph_t> Permute(const std::vector<node_t>& new_order) const {
    std::vector<node_t> new_pos(size(), -1);
    for (node_t i = 0; i < size(); i++) new_pos[new_order[i]] = i;
    edges_t new_edges(size());
    for (node_t i = 0; i < size(); i++) {
      for (node_t x : neighs(i)) {
        new_edges[new_pos[i]].push_back(new_pos[x]);
      }
      std::sort(new_edges[new_pos[i]].begin(), new_edges[new_pos[i]].end());
    }
    return absl::make_unique<graph_t>(size(), new_edges,
                                      labels_.Permute(new_order));
  }

  std::unique_ptr<graph_t> Clone() const {
    std::vector<node_t> identity(size());
    std::iota(identity.begin(), identity.end(), 0);
    return Permute(identity);
  }

  graph_t(const graph_t&) = delete;
  graph_t(graph_t&&) noexcept = default;
  graph_t& operator=(const graph_t&) = delete;
  graph_t& operator=(graph_t&&) = delete;
  virtual ~graph_t() = default;

  void Serialize(std::vector<size_t>* out) const {
    ::Serialize(N_, out);
    ::Serialize(edges_, out);
    ::Serialize(labels_, out);
  }

  void Deserialize(const size_t** in) {
    ::Deserialize(in, &N_);
    ::Deserialize(in, &edges_);
    ::Deserialize(in, &labels_);
  }

 protected:
  node_t N_;
  std::vector<binary_search_t<node_t>> edges_;
  labels_t labels_;
};




template <typename index_t>
class graph_t {
  public: 
    // edges_t è un vettore di puntatori a complex_graph_nodes
    using edges_t = std::vector<complex_graph_node<index_t>*>;

    graph_t() {}

    graph_t(size_t N, const std::vector<std::vector<index_t>>& edg) : size_(N), nodes(N), fwd_iter_(N) {
      for (size_t i = 0; i < N; i++) {
        nodes[i] = new complex_graph_node<index_t>;
        nodes[i]->index = i;
        nodes[i]->in_S = false;
        nodes[i]->in_N = false;
        nodes[i]->deleted = false;

        edges_standard[i].init(edg[i]);

        for(auto& neigh : edg[i]) { 
          nodes[i]->neighbors.insert(neigh);
        }
        // for (node_t x : edg[i]) edges_[i].insert(x);
        // For ogni nodo devo copiarmi gli archi, quindi è meglio tenersi dei puntatori?
        fwd_iter_[i] = neighs(i).upper_bound(i) - neighs(i).begin();
        // deleted_[i].reserve(edges_[i].size());
      }
    }

    const bool is_in_S(index_t node) {
      return nodes[node]->in_S;
    }

    const bool is_in_N(index_t node) {
      return nodes[node]->in_N;
    }

    const bool is_deleted(index_t node) {
      return nodes[node]->deleted;
    }

    void delete_node(index_t node) {
      nodes[node]->deleted = true;
    }

    void restore_node(index_t node) {
      nodes[node]->deleted = false;
    }

    void put_in_S(index_t node) {
      nodes[node]->in_S = true;
    }

    void remove_from_S(index_t node) {
      nodes[node]->in_S = false;
    }

    void put_in_N(index_t node) {
      nodes[node]->in_N = true;
    }

    void remove_from_N(index_t node) {
      nodes[node]->in_N = false;
    }

    const absl::Span<const index_t> fwd_neighs(index_t n) const {
      auto beg = neighs(n).begin() + fwd_iter_[n];
      auto end = neighs(n).end();
      return absl::Span<const index_t>(beg, end - beg);
    }

    bool are_neighs(index_t a, index_t b) const {
      return nodes[a]->neighbors.count(b);
    }

    const binary_search_t<index_t>& neighs(index_t i) const { return edges_standard[i]; }
    size_t degree(index_t i) const { return nodes[i]->neighbors.size(); }
    size_t fwd_degree(index_t n) const { return fwd_neighs(n).size(); }
    size_t size() const { return size_; }

    std::unique_ptr<graph_t> Permute(
      const std::vector<index_t>& new_order) const {
      std::vector<index_t> new_pos(size(), -1);
      for (index_t i = 0; i < size(); i++) new_pos[new_order[i]] = i;
      edges_t new_edges(size());
      for (index_t i = 0; i < size(); i++) {
        for (index_t x : neighs(i)) {
          new_edges[new_pos[i]].push_back(new_pos[x]);
        }
        std::sort(new_edges[new_pos[i]].begin(), new_edges[new_pos[i]].end());
      }
      return absl::make_unique<graph_t>(size(), new_edges);
    }

  std::unique_ptr<graph_t> Clone() const {
    std::vector<index_t> identity(size());
    std::iota(identity.begin(), identity.end(), 0);
    return Permute(identity);
  }

    graph_t(const graph_t&) = delete;
    graph_t(graph_t&&) noexcept = default;
    graph_t& operator=(const graph_t&) = delete;
    graph_t& operator=(graph_t&&) = delete;
    virtual ~graph_t() {
      for(auto n : nodes) delete n;
    }

    void Serialize(std::vector<size_t>* out) const {
      ::Serialize(size, out);
      ::Serialize(nodes, out);
    }

    void Deserialize(const size_t** in) {
      ::Deserialize(in, &size);
      ::Deserialize(in, &nodes);
    }

  private:
    size_t size_; // number of nodes
    std::vector<complex_graph_node<index_t>*> nodes;
    std::vector<binary_search_t<index_t>> edges_standard;
    dynarray<size_t> fwd_iter_;
};







/*

template <typename T>
struct complex_graph_node {
  T index;
  bool in_S;
  bool in_N;
  bool deleted;

  bool operator<(const complex_graph_node& other) const {
    return (index < other.index);
  }

  bool operator==(const complex_graph_node& other) const {
    return index == other.index;
  }

  bool operator>=(const complex_graph_node& other) const {
    return index >= other.index;
  }

  bool operator!=(const complex_graph_node& other) const {
    return index != other.index;
  }
}; */

template <typename index_t>
std::vector<std::vector<index_t>> ReadEdgeList(FILE* in, bool directed,
                                              bool one_based, index_t nodes) {
  std::vector<std::vector<index_t>> edges(nodes);
  while (true) {
    index_t a = fastio::FastRead<index_t>(in);
    index_t b = fastio::FastRead<index_t>(in);
    if (a == index_t(EOF) ||b == index_t(EOF)) break;
    if (a == b) continue;
    if (one_based) { 
      a--;
      b--;
    }
    edges[a].push_back(b);
    if (!directed) edges[b].push_back(a);
  }
  for (std::vector<index_t>& adj : edges) {
    std::sort(adj.begin(), adj.end());
    adj.erase(std::unique(adj.begin(), adj.end()), adj.end());
  }
  return edges;
}

extern template std::vector<std::vector<uint32_t>> ReadEdgeList<uint32_t>(
    FILE* in, bool directed, bool one_based, uint32_t nodes);

extern template std::vector<std::vector<uint64_t>> ReadEdgeList<uint64_t>(
    FILE* in, bool directed, bool one_based, uint64_t nodes);


/* template <typename index_t = uint32_t>
class graph_t {
  public:
    using node_t = complex_graph_node<index_t>;
    using edges_t = std::vector<std::vector<node_t>>;

    graph_t() {}

    graph_t(size_t N, const edges_t& edg) : N_(N), edges_(N) {
      for(size_t i=0;i<N;i++) {
        edges_[i].init(edg[i]);
      }
    }

    size_t size() const { return N_; }
    size_t degree(node_t& i) const { return edges_[i.index].size(); }
    size_t fwd_degree(node_t& n) const { return fwd_neighs(n).size(); }
    const binary_search_t<node_t>& neighs(node_t& i) const { return edges_[i.index]; }

    const absl::Span<const node_t> fwd_neighs(node_t& n) const {
      auto beg = edges_[n.index].upper_bound(n.index);
      auto end = edges_[n.index].end();
      return absl::Span<const node_t>(beg, end - beg);
    }

    bool are_neighs(node_t& a, node_t& b) const { return edges_[a.index].count(b); }

    std::unique_ptr<graph_t> Permute(const std::vector<node_t>& new_order) const {
      std::vector<node_t> new_pos(size(), -1);
      for (size_t i = 0; i < size(); i++) new_pos[new_order[i]] = edges_[i];
      edges_t new_edges(size());
      for (size_t i = 0; i < size(); i++) {
        for (node_t& x : neighs(edges_[i])) {
          new_edges[new_pos[i]].push_back(new_pos[x.index]);
        }
        std::sort(new_edges[new_pos[i]].begin(), new_edges[new_pos[i]].end());
      }
      return absl::make_unique<graph_t>(size(), new_edges);
    }

    std::unique_ptr<graph_t> Clone() const {
      std::vector<node_t> identity(size());
      std::iota(identity.begin(), identity.end(), 0);
      return Permute(identity);
    }

    graph_t(const graph_t&) = delete;
    graph_t(graph_t&&) noexcept = default;
    graph_t& operator=(const graph_t&) = delete;
    graph_t& operator=(graph_t&&) = delete;
    virtual ~graph_t() = default;

    void Serialize(std::vector<size_t>* out) const {
      ::Serialize(N_, out);
      ::Serialize(edges_, out);
      // ::Serialize(labels_, out);
    }

    void Deserialize(const size_t** in) {
      ::Deserialize(in, &N_);
      ::Deserialize(in, &edges_);
      // ::Deserialize(in, &labels_);
    }

  protected: 
    size_t N_;
    std::vector<binary_search_t<node_t>> edges_;
}


template <typename index_t = uint32_t>
class fast_graph_t : public graph_t<index_t> {
  using base_ = graph_t<index_t>;

  public: 
    using node_t = complex_graph_node<index_t>;
    using edges_t = typename base_::edges_t;

    fast_graph_t() {}

    fast_graph_t(size_t N, const edges_t& edg) : base_(N, edg), fwd_iter_(N) {
      for (size_t i = 0; i < N; i++) {
        for (node_t x : edg[i]) edges_[i].insert(x);
        fwd_iter_[i] = base_::neighs(edges_[i]).upper_bound(i) - base_::neighs(edges_[i]).begin();
      }
    }

    ~fast_graph_t() = default;

    const cuckoo_hash_set<node_t>& neighs(node_t& i) const { return edges_[i.index]; }
    bool are_neighs(node_t& a, node_t& b) const { return edges_[a.index].count(b); }

    std::unique_ptr<fast_graph_t> Permute(
      const std::vector<node_t>& new_order) const {
      std::vector<node_t> new_pos(base_::size(), -1);
      for (size_t i = 0; i < base_::size(); i++) new_pos[new_order[i]] = edges_[i];
      edges_t new_edges(base_::size());
      for (size_t i = 0; i < base_::size(); i++) {
        for (node_t x : base_::neighs(i)) {
          new_edges[new_pos[i]].push_back(new_pos[x.index]);
        }
        std::sort(new_edges[new_pos[i]].begin(), new_edges[new_pos[i]].end());
      }
      return absl::make_unique<fast_graph_t>(base_::size(), new_edges,
                                            base_::labels_.Permute(new_order));
    }

    std::unique_ptr<fast_graph_t> Clone() const {
      std::vector<node_t> identity(base_::size());
      std::iota(identity.begin(), identity.end(), 0);
      return Permute(identity);
    }
    void Serialize(std::vector<size_t>* out) const {
      base_::Serialize(out);
      ::Serialize(edges_, out);
      ::Serialize(fwd_iter_, out);
    }

    void Deserialize(const size_t** in) {
      base_::Deserialize(in);
      ::Deserialize(in, &edges_);
      ::Deserialize(in, &fwd_iter_);
    }


  private: 
    dynarray<cuckoo_hash_set<node_t>> edges_;
    dynarray<size_t> fwd_iter_;
}

template <typename node_t = uint32_t,
          template <typename> class Graph = fast_graph_t>
std::unique_ptr<Graph<node_t>> ReadNde(FILE* in = stdin,
                                             bool directed = false) {
  node_t N = fastio::FastRead<node_t>(in);
  for (node_t i = 0; i < N; i++) {
    // Discard degree information
    fastio::FastRead<node_t>(in);
    fastio::FastRead<node_t>(in);
  }
  //auto labels = graph_internal::ReadLabels<node_t, void>(in, N);
  auto edges = ReadEdgeList<node_t>(in, directed,
                                                     one_based = false, N);
  return absl::make_unique<Graph<node_t, void>>(N, edges);
} */

/* extern template class graph_t<uint32_t, void>;
extern template class graph_t<uint32_t, int32_t>;
extern template class graph_t<uint32_t, int64_t>;
extern template class graph_t<uint32_t, uint32_t>;
extern template class graph_t<uint32_t, uint64_t>;
extern template class graph_t<uint64_t, void>;
extern template class graph_t<uint64_t, int32_t>;
extern template class graph_t<uint64_t, int64_t>;
extern template class graph_t<uint64_t, uint32_t>;
extern template class graph_t<uint64_t, uint64_t>;
extern template class fast_graph_t<uint32_t, void>;
extern template class fast_graph_t<uint32_t, int32_t>;
extern template class fast_graph_t<uint32_t, int64_t>;
extern template class fast_graph_t<uint32_t, uint32_t>;
extern template class fast_graph_t<uint32_t, uint64_t>;
extern template class fast_graph_t<uint64_t, void>;
extern template class fast_graph_t<uint64_t, int32_t>;
extern template class fast_graph_t<uint64_t, int64_t>;
extern template class fast_graph_t<uint64_t, uint32_t>;
extern template class fast_graph_t<uint64_t, uint64_t>; 
extern template class graph_t<uint32_t>;
extern template class graph_t<uint64_t>; */
extern template class graph_t<uint32_t>;
extern template class graph_t<uint64_t>;

#endif  // UTIL_GRAPH_H
