#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <signal.h>
#include <cassert>
#include <atomic>

#include <omp.h>

#include "util/cuckoo.hpp"
#include "util/graph.hpp"

#ifndef TIMEOUT
#define TIMEOUT (60 * 30) /* Timer 30 minutes */
#endif

//#define START_REC (ciproviamo(S, graph, k, C_of_S, 0, 0))
#define START_REC (enhanced(S, graph, k, C_of_S, 0))

template <typename node_t, typename label_t>
std::unique_ptr</*unsorted*/fast_graph_t<node_t, label_t>> ReadFastGraph(
    const std::string& input_file, bool directed = false) {
    FILE* in = fopen(input_file.c_str(), "re");
    if (!in) throw std::runtime_error("Could not open " + input_file);
    return ReadNde<node_t, /*unsorted*/fast_graph_t>(in, directed);
}

std::atomic<std::uint_fast64_t> counter(0);
//std::vector<bool> excluded;
//std::vector<bool> in_S;
//std::vector<bool> in_C;
//std::vector<bool> tmp;

bool interrupted = false; // Timer

using node_t = uint32_t;

std::atomic<std::uint_fast64_t> recursion_nodes(0);
std::atomic<std::uint_fast64_t> leaves(0);


/* Questa versione va più che bene per O(|E|) delay
 * La versione "ci proviamo" era per provare ad arrivare a O(k) delay
 */
bool enhanced(std::vector<node_t>& S,
  /*unsorted*/fast_graph_t<node_t, void>* graph,
              int k,
              std::vector<node_t>& C_of_S, 
              int start,
              cuckoo_hash_set<node_t>& in_S,
              cuckoo_hash_set<node_t>& in_C,
              cuckoo_hash_set<node_t>& excluded,
              node_t first_node
              ) {


    recursion_nodes++;


    if(S.size() == k) {
        counter++;
        leaves++;
        /*std::cout << "Trovato graphlet: "; 
        for(auto& v : *S) std::cout << v << " ";
        std::cout << std::endl;*/
        return true;
    }

    if(interrupted) return false; // Timer


    if(start == C_of_S.size()) { leaves++; return false; } // No nuovi nodi
    auto end = C_of_S.size();
    bool found = false;

    auto old_start = start;
    //std::vector<node_t> deleted;
    //deleted.reserve(end - start + 1);
    for(;start < end; start++) {
        if(interrupted) return false;

        auto old_size = C_of_S.size();
        auto v = C_of_S[start];
        assert(!excluded.count(v));
        if(v < first_node || excluded.count(v) || in_S.count(v)) continue;
        //auto neighs = graph->neighs(v);
        //for(auto i=0;i<=(long long)neighs.get_max_index();i++) {
        for(auto& neigh : graph->neighs(v)) {
            //auto neigh = neighs[i];
            if(v > first_node && !in_C.count(neigh) && !excluded.count(neigh) && !in_S.count(neigh)) {
                C_of_S.push_back(neigh);
                in_C.insert(neigh);
            }
        }
        // Chiamata sx
        S.push_back(v);
        in_S.insert(v);
        bool left = enhanced(S, graph, k, C_of_S, start+1, in_S, in_C, excluded, first_node);

        S.pop_back();
        in_S.erase(v);

        while(C_of_S.size() > old_size) {
            in_C.erase(C_of_S.back());
            C_of_S.pop_back();
        }
        if(interrupted) return false;
        
        if(left) {
            found = true;

            excluded.insert(v);
                //std::cout << "Ho cancellato " << v << " ";
            //graph->delete_node(v);
            //}
        }
        else break;

        //if(left && !interrupted) {
        //    excluded[v] = true;
        //    enhanced(S, graph, k, C_of_S, old_size);
        //    excluded[v] = false;
        //    found = true;
        //}
        //else {
        //    break;
        //}
    }


    for(int i = ((start < end) ? start : end -1);i>=old_start;i--) {
        auto v = C_of_S[i];
        //std::cerr << "Ripristino " << v << std::endl;
        //graph->restore_node(v);
        excluded.erase(v);
    }

    return found;
}




void main_enum(/*std::vector<node_t>* S, unsorted*/fast_graph_t<node_t, void>* graph, int k) {

    std::vector<cuckoo_hash_set<node_t>> all_in_S(17);
    std::vector<cuckoo_hash_set<node_t>> all_in_C(17);
    std::vector<cuckoo_hash_set<node_t>> all_excluded(17);
    std::vector<std::vector<node_t>> all_S(17);
    std::vector<std::vector<node_t>> all_C_of_S(17);

    for(auto i=0;i<17;i++) {
        all_S[i].reserve(k);
        all_C_of_S[i].reserve(1500);
        all_in_S[i].reserve(2*k);
        all_in_C[i].reserve(1500);
        all_excluded[i].reserve(1500);
    }

#pragma omp parallel for num_threads(16)
    for(node_t v=0;v<graph->size()-k+1;v++) {
        if(interrupted) continue; // Timer 
        // Prima lista C(S)
        // Popolo C(S) con i vicini non esclusi di v
        int th_id = omp_get_thread_num();
        std::vector<node_t>& S = all_S[th_id % 17];
        std::vector<node_t>& C_of_S = all_C_of_S[th_id % 17];
        cuckoo_hash_set<node_t>& in_S = all_in_S[th_id % 17];
        cuckoo_hash_set<node_t>& in_C = all_in_C[th_id % 17];
        cuckoo_hash_set<node_t>& excluded = all_excluded[th_id % 17];

        
        //for(node_t u=0;u<v;u++) excluded.insert(u);

        S.push_back(v);
        in_S.insert(v);

        for(auto& neigh : graph->neighs(v)) {
            //auto neigh = neighs[i];
            // Qui con la cancellazione non devo fare nient'altro
            if(neigh > v/*!excluded.count(neigh)*/) {
                C_of_S.push_back(neigh);
                in_C.insert(neigh);
            }
        }
        
        //for(auto& n : C_of_S) std::cout << n << " ";
        //std::cout << std::endl;

        //enhanced(S, graph, k, C_of_S, 0);

        enhanced(S, graph, k, C_of_S, 0, in_S, in_C, excluded, v);
        //START_REC;
        
        //S->pop_back();
        //in_S.[v] = false;
        // Cancellare v dal grafo
        //excluded[v] = true;

        //graph->delete_node(v);
        
        //for(auto& v : C_of_S) in_C[v] = false;
        //C_of_S.clear();

        S.clear();
        C_of_S.clear();
        in_C.clear();
        in_S.clear();
        excluded.clear();

        //return;
    }
}

int main(int argc, char* argv[]) {

    bool skip = false;
    int k;

    switch(argc) {
        case 3:
            k = std::stoi(argv[2]);
            break;
        case 4:
            k = std::stoi(argv[2]);
            skip = true;
            break;
        default: 
            std::cerr << "Usage: " << argv[0] << " <graph_file.nde> <k> [Skip Printing Info]" << std::endl;
            return 1;
    }
    

    //std::cout << "Seme: " << seed << std::endl;

    auto graph = ReadFastGraph<node_t, void>(argv[1]);

    /*indexes.resize(graph->size());

    std::iota(indexes.begin(), indexes.end(), 0);

    std::shuffle(indexes.begin(), indexes.end(), mt);*/

    /*std::vector<node_t> S;

    excluded.resize(graph->size());
    in_S.resize(graph->size());
    in_C.resize(graph->size());*/
    //tmp.resize(graph->size(), false);

    size_t edges = 0;
    if(!skip) {
        std::cout << "Nodes: " << graph->size() << std::endl;
        for(size_t i = 0;i < graph->size(); i++)
            edges += graph->degree(i);

        std::cout << "Edges: " << edges << std::endl;
    }

    signal(SIGALRM, [](int) { interrupted = true; });
    signal(SIGINT, [](int) { interrupted = true; });
    alarm(TIMEOUT); // Set timer 

    auto start = std::chrono::high_resolution_clock::now();

    main_enum(graph.get(), k);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    std::cerr << "Found " << counter << " graphlets of size " << k;
    std::cerr << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()/1000. << " s" << std::endl;
    std::cerr << "Recursion nodes - rec. roots: " << recursion_nodes << " - " << graph->size() << " = " << recursion_nodes-graph->size() << std::endl;
    std::cerr << "Solutions per sec: " << counter / std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000 << std::endl;
    std::cerr << "Leaves: " << leaves << " fruitful leaves: " << counter << " dead leaves: " << leaves-counter << std::endl;
    std::cerr << "Graphlets/leaves ratio: " << (double) counter / leaves << std::endl;

    std::cerr << "Rehash totali: " << graph->get_rehashes() << std::endl;
    std::cerr << "Density: " << (double) edges / (graph->size() * (graph->size()-1)) << std::endl;

    return (interrupted ? 14 : 0);
}
