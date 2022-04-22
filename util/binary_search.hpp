#ifndef UTIL_BINARY_SEARCH_H
#define UTIL_BINARY_SEARCH_H
#include <cstdint>
#include <vector>

#include <iostream>

#include "util/dynarray.hpp"
#include "util/serialize.hpp"

template <typename T = uint32_t>
class binary_search_t {
 private:
  dynarray<T> support;

 public:
  using data_type = dynarray<T>&;
  using iterator = const T*;

  void Serialize(std::vector<size_t>* out) const { ::Serialize(support, out); }

  void Deserialize(const size_t** in) { ::Deserialize(in, &support); }

  void init(const std::vector<T>& v) {
    support.resize(v.size());
    unsigned cnt = 0;
    while (cnt != v.size()) {
      support[cnt] = v[cnt];
      cnt++;
    }
  }

  iterator begin() const { return support.begin(); }

  iterator it_at(size_t p) const { return begin() + p; }

  iterator end() const { return support.end(); }

  size_t size() const { return support.size(); }

  T get_at(size_t idx) const { return support[idx]; }

  T operator[](size_t idx) const { return get_at(idx); }

  bool count(T v) const {
    size_t n = support.size();
    size_t cur = 0;
    while (n > 1) {
      const int64_t half = n / 2;
      cur = support[cur + half] < v ? cur + half : half;
      n -= half;
    }
    cur += support[cur] < v;
    return cur < support.size() && support[cur] == v;
  }

  iterator lower_bound(T v) const {
    return std::lower_bound(support.begin(), support.end(), v);
  }

  iterator upper_bound(T v) const {
    return std::upper_bound(support.begin(), support.end(), v);
  }

  data_type data() { return support; }
};

template <typename T = uint32_t>
class linear_search_t {
 private:
  dynarray<T> support;
  size_t max_index; // Max index è l'ultimo elemento valido nell'array

 public:
  using data_type = dynarray<T>&;
  using iterator = const T*;

  void Serialize(std::vector<size_t>* out) const { ::Serialize(support, out); }

  void Deserialize(const size_t** in) { ::Deserialize(in, &support); }

  void init(const std::vector<T>& v) {
    support.resize(v.size());
    max_index = support.size() - 1;
    unsigned cnt = 0;
    while (cnt != v.size()) {
      support[cnt] = v[cnt];
      cnt++;
    }
  }

  iterator begin() const { return support.begin(); }

  iterator it_at(size_t p) const { return begin() + p; }

  iterator end() const { return support.end() /*-  begin() + (max_index + 1)*/; }

  size_t size() const { return max_index + 1; /*support.size();*/ }

  T get_at(size_t idx) const { return support[idx]; }

  T operator[](size_t idx) const { return get_at(idx); }

  size_t get_max_index() { return max_index; }

  void revert() {
    max_index = support.size() - 1;
  }

  void revert_last_item() {
    if(max_index + 1 < support.size()) {
      max_index++;
    }
  }

  void revert_at_index(size_t i) {
    if(max_index+1 == support.size()-1) {
      max_index++;
      return;
    }
    if(max_index < i && i < support.size()) {
      auto aux = support[i];
      max_index++;
      support[i] = support[max_index];
      support[max_index] = aux;
    }
  }

  void revert_item(T v) {
    //std::cerr << "Sto revertando il vertice " << v << std::endl;
    //std::cerr << "E la lista di adiacenza è ";
    //for(auto& u : support) { std::cerr << u << " "; }
    //std::cerr << std::endl << " e max_index è " << max_index << std::endl;
    int i = max_index+1;
    if(support[i] == v) {
      max_index++;
      return;
    }
    i++;
    while(i < support.size()) {
      if(support[i] == v) {
        max_index++;
        auto aux = support[i];
        support[i] = support[max_index];
        support[max_index] = aux;
        return;
      }
      i++;
    }
    
    return;




    //int i = max_index + 1;
    if(support[i++] == v) {
      max_index++;
      return;
    }
    while(i < support.size()) {
      if(support[i] == v) {
        auto aux = support[i];
        support[i] = support[max_index];
        support[max_index] = support[i];
        max_index++;
        break;
      }
      i++;
    }
  }

  void delete_at_index(size_t i) {
    auto aux = support[i];
    support[i] = support[max_index];
    support[max_index] = aux;

    max_index--;
  }

  size_t delete_item(T v) {
    size_t n = max_index + 1;
    int i = 0;
    while(i < n) {
      if(support[i] == v) break;
      i++;
    }

    if(i >= n) return max_index+1;
    
    auto aux = support[i];
    support[i] = support[max_index];
    support[max_index] = aux;

    max_index--;

    return max_index+1;
  }

  bool count(T v) const {
    size_t n = max_index + 1;
    int i = 0;
    while(i < n) {
      if(support[i] == v) return true;
      i++;
    }
    return false;
    // Vecchia binary search
    /*size_t cur = 0;
    while (n > 1) {
      const int64_t half = n / 2;
      cur = support[cur + half] < v ? cur + half : half;
      n -= half;
    }
    cur += support[cur] < v;
    return cur < support.size() && support[cur] == v;*/
  }

  iterator lower_bound(T v) const {
    return std::lower_bound(support.begin(), support.end() - max_index, v);
  }

  iterator upper_bound(T v) const {
    return std::upper_bound(support.begin(), support.end() - max_index, v);
  }

  data_type data() { return support; }
};

extern template class linear_search_t<int32_t>;
extern template class linear_search_t<int64_t>;
extern template class linear_search_t<uint32_t>;
extern template class linear_search_t<uint64_t>;

extern template class binary_search_t<int32_t>;
extern template class binary_search_t<int64_t>;
extern template class binary_search_t<uint32_t>;
extern template class binary_search_t<uint64_t>;

#endif  // UTIL_BINARY_SEARCH_H
