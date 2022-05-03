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

//#define START_REC (ciproviamo(S, graph, k, C_of_S, 0, 0))
#define START_REC (enhanced(S, graph, k, N_of_S, 0))

template <typename node_t, typename label_t>
std::unique_ptr<fast_graph_t<node_t, label_t>> ReadFastGraph(
    const std::string& input_file, bool directed = false) {
    FILE* in = fopen(input_file.c_str(), "re");
    if (!in) throw std::runtime_error("Could not open " + input_file);
    return ReadNde<node_t, fast_graph_t>(in, directed);
}

uint64_t solutions = 0;
std::vector<bool> excluded;
std::vector<bool> in_S;
std::vector<bool> in_C;
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

/* struct {
    bool operator()(const node_t& lhs, const node_t& rhs) const {
        if(current_degree[lhs] < current_degree[rhs])
            return true;
        else if(lhs < rhs)
            return true;
        
        return false;
    }
} vertexCmp;*/

bool enhanced(std::vector<node_t>& S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& N_of_S, int start) {
    recursion_nodes++;

    // INV: S.size() <= k-2

    if(interrupted) return false; // Timer

    if(start == N_of_S.size()) { leaves++; return false; } // No nuovi nodi
    auto end = N_of_S.size();
    bool found = false;

    if(S.size() == k-1) {
        auto diff = end - start;
        solutions += diff; 
        
        if(diff < min_sol_per_leaf) { min_sol_per_leaf = diff; count_min_leaf = 1; }
        else if(diff == min_sol_per_leaf) count_min_leaf++;
        if(diff > max_sol_per_leaf) { max_sol_per_leaf = diff; count_max_leaf = 1; }
        else if(diff == max_sol_per_leaf) count_max_leaf++;

        // std::cout << "Trovati graphlet: "; 
        // for(int i=start;i<C_of_S.size();i++) {
        //     auto neigh = C_of_S[i];
        //     for(auto& v : *S) std::cout << v << " ";
        //     std::cout << neigh << std::endl;
        // }
        
        leaves++;
        if(diff > 0) fruitful_leaves++;
        return diff > 0;
    } 

    bool im_a_parent = false;

    for(;start < end; start++) {
        if(interrupted) return false;

        auto old_size = N_of_S.size();
        auto v = N_of_S[start];
        if(excluded[v] || /*graph->is_in_S(v)*/in_S[v]) continue;
        size_t tmp = 0;
        for(auto& neigh : graph->neighs(v)) {
            if(!in_C[neigh] && !excluded[neigh] && /*!graph->is_in_S(neigh)*/!in_S[neigh]) {
                N_of_S.push_back(neigh);
                in_C[neigh] = true;
                tmp++;
            }
        }

        // if(tmp == 0 && ) { std::cout << "\t\t\tAAAA\n"; ; }

        bool left;
        // std::cout << "Ora dovrei prendere " << v << std::endl;
        if(start+1 < end+tmp) {
            // Chiamata sx
            S.push_back(v); 
            in_S[v] = true;
            // graph->put_in_S(v);
            
            left = enhanced(S, graph, k, N_of_S, start+1);
            
            im_a_parent = true;
            // std::cout << "Finita la rec call di " << S->back() << std::endl;
            S.pop_back();
            in_S[v] = false;
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
            in_C[N_of_S.back()] = false;
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
    N_of_S.reserve(graph->size()/2);
    for(auto v=0;v<graph->size()-k+1;v++) {
    
    /*for(auto v=103;v<104;v++) {
        for(auto i=0;i<v;i++) excluded[i] = true;*/
        if(interrupted) return; // Timer 
        S.push_back(v);
        in_S[v] = true;
        // graph->put_in_S(v);
        // Prima lista C(S)
        // Popolo C(S) con i vicini non esclusi di v

        for(auto& neigh : graph->fwd_neighs(v)) {
            if(!excluded[neigh]) {
                N_of_S.push_back(neigh);
                // C_of_S.insert(neigh);
                in_C[neigh] = true;
            }
        }
        // std::cout << "V è " << v << " e quindi i vicini sono: ";
        // for(auto& n : C_of_S) std::cout << n << " ";
        // std::cout << std::endl;

        //enhanced(S, graph, k, C_of_S, 0);

        // std::sort(C_of_S.begin(), C_of_S.end(), vertexCmp);

        START_REC;
        
        S.pop_back();
        in_S[v] = false;
        // graph->remove_from_S(v);

        excluded[v] = true;

        // for(auto& neigh : graph->fwd_neighs(v)) {
        //     if(!excluded[neigh]) {
        //         current_degree[neigh]--;
        //     }
        // }
        
        for(auto& v : N_of_S) in_C[v] = false;
        N_of_S.clear();
        //solutions.clear();
        //return;
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

    excluded.resize(graph->size());
    in_S.resize(graph->size());
    in_C.resize(graph->size());
    // current_degree.resize(graph->size());
    // tmp.resize(graph->size(), false);

    if(!skip) std::cout << "Nodes: " << graph->size() << std::endl;
    size_t edges = 0;
    for(size_t i = 0;i < graph->size(); i++) {
        edges += graph->degree(i);
        // current_degree[i] = graph->degree(i);
    }

    if(!skip) std::cout << "Edges: " << edges << std::endl;

    graph->Permute(DegeneracyOrder(*graph));

    signal(SIGALRM, [](int) { interrupted = true; });
    signal(SIGINT, [](int) { interrupted = true; });
    alarm(TIMEOUT); // Set timer 

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
