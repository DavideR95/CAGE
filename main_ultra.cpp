#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <signal.h>
#include <set>
#include <unordered_set>

#include "util/graph.hpp"
#include "permute/permute.hpp"

#include "/opt/intel/oneapi/vtune/latest/sdk/include/ittnotify.h"


//#ifndef TIMEOUT
#define TIMEOUT (60 * 15) /* Timer 30 minutes */
//#endif

#define START_REC (baseline_pp(S, graph, k, N_of_S, 0))

#define IS_DELETED(neighbor, node) (neighbor < node)

#define IN_ARRAY(elem, arr) (std::find(arr.begin(), arr.end(), elem) != arr.end()) 

#define IS_IN_N_OR_S(elem) (inverted_N.count(elem))

template <typename node_t, typename label_t>
std::unique_ptr<fast_graph_t<node_t, label_t>> ReadFastGraph(
    const std::string& input_file, bool directed = false) {
    FILE* in = fopen(input_file.c_str(), "re");
    if (!in) throw std::runtime_error("Could not open " + input_file);
    return ReadNde<node_t, fast_graph_t>(in, directed);
}

uint64_t solutions = 0;
// std::vector<bool> excluded;
// std::vector<bool> in_S;
// std::vector<bool> in_C;
//std::vector<bool> tmp;
// std::vector<size_t> current_degree;

bool interrupted = false; // Timer

using node_t = uint32_t;

uint64_t recursion_nodes = 0;
uint64_t leaves = 0;
uint64_t fruitful_leaves = 0;
uint64_t min_sol_per_leaf = 10000000L;
uint64_t max_sol_per_leaf = 0L;
uint64_t count_min_leaf = 0L;
uint64_t count_max_leaf = 0L;

cuckoo_hash_set<node_t> inverted_N;

bool baseline_pp(std::vector<node_t>& S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& N_of_S, int start) {
    recursion_nodes++;

    // INV: S.size() <= k-2

    if(S.size() == k) {
        solutions++;
        leaves++;
        fruitful_leaves++;
        return true;
    }

    if(interrupted) return false; // Timer

    auto end = N_of_S.size();
    if(start == end) { leaves++; return false; } // No nuovi nodi
    bool found = false;

    auto first_node = S.front();

    bool im_a_parent = false;

    // cuckoo_hash_set<node_t> inverted_N;
    // inverted_N.reserve(N_of_S.size()*2);
    /*inverted_N.clear();
    for(int i=0;i<end;i++) inverted_N.insert(N_of_S[i]);
    for(auto& v : S) inverted_N.insert(v);*/

    for(;start < end; start++) {
        if(interrupted) return false;

        auto old_size = N_of_S.size();
        auto v = N_of_S[start];
        
        if(IS_DELETED(v, S.front()) || IN_ARRAY(v, S)) continue;
        size_t tmp = 0;
        for(auto& neigh : graph->neighs(v)) {
            if(!IS_DELETED(neigh, S.front()) && !IS_IN_N_OR_S(neigh)) {
                N_of_S.push_back(neigh);
                // in_C[neigh] = true;
                // graph->put_in_N(neigh);
                //in_N.insert(neigh);
                inverted_N.insert(neigh);
                tmp++;
            }
        }

        // if(tmp == 0 && ) { std::cout << "\t\t\tAAAA\n"; ; }

        bool left;
        // std::cout << "Ora dovrei prendere " << v << std::endl;
        if(start+1 < end+tmp) {
            // Chiamata sx
            S.push_back(v); 
            inverted_N.insert(v);
            // in_S[v] = true;
            // graph->put_in_S(v);
            
            left = baseline_pp(S, graph, k, N_of_S, start+1);
            
            im_a_parent = true;
            // std::cout << "Finita la rec call di " << S->back() << std::endl;
            S.pop_back();
            inverted_N.erase(v);
            // in_S[v] = false;
            // graph->remove_from_S(v);
            // excluded[v] = true;
            // std::cout << "Ora tolgo " << v << ", e left = " << left << std::endl;
        }
        else {
            // std::cout << "(Non ho fatto nulla) " << std::endl;
            left = false;
            // std::cout << "E quindi found = " << found << std::endl;
        }

        

        while(N_of_S.size() > old_size) {
            // in_C[N_of_S.back()] = false;
            // graph->remove_from_N(N_of_S.back());
            inverted_N.erase(N_of_S.back());
            N_of_S.pop_back();
        }
        if(interrupted) return false;
        
        //if(left) found = true;
        //else break;

        if(left && !interrupted) {
            /*excluded[v] = true;
            enhanced(S, graph, k, C_of_S, start+1);
            excluded[v] = false;*/
            found = true;
        }
        else {
            break;
        }
    }

    if(!found && !im_a_parent) { leaves++; }

    return found;
}




void main_enum(std::vector<node_t>& S, fast_graph_t<node_t, void>* graph, int k) {
    // std::set<node_t, vertexCmp> C_of_S;
    std::vector<node_t> N_of_S;
    N_of_S.reserve(graph->size()/10);
    for(auto v=0;v<graph->size()-k+1;v++) {
    
    /*for(auto v=103;v<104;v++) {
        for(auto i=0;i<v;i++) excluded[i] = true;*/
        if(interrupted) return; // Timer 
        S.push_back(v);
        inverted_N.insert(v);
        // in_S[v] = true;
        // graph->put_in_S(v);
        // Prima lista C(S)
        // Popolo C(S) con i vicini non esclusi di v

        for(auto& neigh : graph->fwd_neighs(v)) {
            // if(!excluded[neigh]) {
            // if(!graph->is_deleted(neigh)) {
            if(!IS_DELETED(neigh, v)) { 
                N_of_S.push_back(neigh);
                // C_of_S.insert(neigh);
                // in_C[neigh] = true;
                // graph->put_in_N(neigh);
                inverted_N.insert(neigh);
            }
        }


        START_REC;
        
        S.pop_back();
        inverted_N.erase(v);

        // graph->remove_from_S(v);

        // no need to explicitly delete v
        
        // for(auto& v : N_of_S) graph->remove_from_N(v); 
        N_of_S.clear();
        inverted_N.clear();

    }
}

int main(int argc, char* argv[]) {
    __itt_pause();
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
            std::cerr << "Usage: " << argv[0] << " <graph_file.nde> <k> [Skip Printing Graph Info]" << std::endl;
            return 1;
    }
    

    auto graph = ReadFastGraph<node_t, void>(argv[1]);

    std::vector<node_t> S;

    if(!skip) std::cout << "Nodes: " << graph->size() << std::endl;
    size_t edges = 0;
    size_t max_degree = 0;
    size_t deg = 0;
    for(size_t i = 0;i < graph->size(); i++) {
        edges += graph->degree(i);
        if(graph->degree(i) > max_degree) max_degree = graph->degree(i);
        if(graph->fwd_degree(i) > deg) deg = graph->fwd_degree(i);
        // current_degree[i] = graph->degree(i);
    }

    edges /= 2; // Undirected graph
    inverted_N.reserve(2*max_degree);

    if(!skip) std::cout << "Edges: " << edges << std::endl;

    graph->Permute(DegeneracyOrder(*graph));

    signal(SIGALRM, [](int) { interrupted = true; });
    signal(SIGINT, [](int) { interrupted = true; });
    alarm(TIMEOUT); // Set timer 

    std::cerr << "Graph read, max degree: " << max_degree << " degeneracy: " << deg << std::endl;

    __itt_resume();
    auto start = std::chrono::high_resolution_clock::now();

    main_enum(S, graph.get(), k);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    __itt_pause();

    std::cout << "Found " << solutions << " graphlets of size " << k;
    std::cout << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()/1000. << " s" << std::endl;
    std::cout << "Recursion nodes - rec. leaves: " << recursion_nodes << " - " << leaves << " = " << recursion_nodes-leaves << std::endl;
    //std::cerr << "Solutions per sec: " << counter / std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000 << std::endl;
    std::cout << "Leaves: " << leaves << " fruitful leaves: " << fruitful_leaves << " dead leaves: " << (leaves-fruitful_leaves);
    std::cout << " ( " << (double) (leaves-fruitful_leaves)/leaves * 100. << " % )" << std::endl;
    std::cout << "Graphlets/leaves ratio: " << (double) solutions / leaves << std::endl;
    std::cout << "Average sol. per leaf: " << (double) solutions / fruitful_leaves << std::endl;
    std::cout << "Min sol. per leaf: " << min_sol_per_leaf << " ( " << count_min_leaf << " times )" << std::endl;
    std::cout << "Max sol. per leaf: " << max_sol_per_leaf << " ( " << count_max_leaf << " times )"<< std::endl;

    return (interrupted ? 14 : 0);
}
