#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <signal.h>

#include "util/graph.hpp"
#include "permute/permute.hpp"

#ifdef __INTEL_COMPILER
#include "/opt/intel/oneapi/vtune/latest/sdk/include/ittnotify.h"
#endif

#ifndef TIMEOUT
#define TIMEOUT (60 * 15) /* Timer 30 minutes */
#endif

//#define START_REC (ciproviamo(S, graph, k, C_of_S, 0, 0))
#define START_REC (enhanced(S, graph, k, C_of_S, 0))

template <typename node_t, typename label_t>
std::unique_ptr<fast_graph_t<node_t, label_t>> ReadFastGraph(
    const std::string& input_file, bool directed = false) {
    FILE* in = fopen(input_file.c_str(), "re");
    if (!in) throw std::runtime_error("Could not open " + input_file);
    return ReadNde<node_t, fast_graph_t>(in, directed);
}

uint64_t counter = 0;
std::vector<bool> excluded;
std::vector<bool> in_S;
std::vector<bool> in_C;
//std::vector<bool> tmp;

bool interrupted = false; // Timer

using node_t = uint32_t;

uint64_t ricorsioni = 0;
uint64_t leaves = 0;


/* Questa versione va più che bene per O(|E|) delay
 * La versione "ci proviamo" era per provare ad arrivare a O(k) delay
 */
bool enhanced(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& C_of_S, int start) {
    ricorsioni++;
    if(S->size() == k) {
        counter++;
        leaves++;
        //std::cout << "Trovato graphlet: "; 
        //for(auto& v : *S) std::cout << v << " ";
        //std::cout << std::endl;
        return true;
    }

    if(interrupted) return false; // Timer

    if(start == C_of_S.size()) { leaves++; return false; } // No nuovi nodi
    auto end = C_of_S.size();
    bool found = false;

    // PROVARE A:
    // Generare k-i figli dove i è la profondità della ricorsione.
    // Come evitare che vengano generati i duplicati del graphlet fatto da k-i?


    for(;start < end; start++) {
        if(interrupted) return false;

        auto old_size = C_of_S.size();
        auto v = C_of_S[start];
        if(excluded[v] || in_S[v]) continue;
        for(auto& neigh : graph->neighs(v)) {
            if(!in_C[neigh] && !excluded[neigh] && !in_S[neigh]) {
                C_of_S.push_back(neigh);
                in_C[neigh] = true;
            }
        }
        // Chiamata sx
        S->push_back(v);
        in_S[v] = true;
        bool left = enhanced(S, graph, k, C_of_S, start+1);

        S->pop_back();
        in_S[v] = false;

        while(C_of_S.size() > old_size) {
            in_C[C_of_S.back()] = false;
            C_of_S.pop_back();
        }
        if(interrupted) return false;
        
        if(left) found = true;
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

    if(!found && start >= end) leaves++;

    return found;
}




void main_enum(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k) {
    std::vector<node_t> C_of_S;
    C_of_S.reserve(graph->size()/2);
    for(auto v=0;v<graph->size()-k+1;v++) {
    /*for(auto v=103;v<104;v++) {
        for(auto i=0;i<v;i++) excluded[i] = true;*/
        if(interrupted) return; // Timer 
        S->push_back(v);
        in_S[v] = true;
        // Prima lista C(S)
        // Popolo C(S) con i vicini non esclusi di v

        for(auto& neigh : graph->fwd_neighs(v)) {
            if(!excluded[neigh]) {
                C_of_S.push_back(neigh);
                in_C[neigh] = true;
            }
        }
        //std::cout << "V è " << v << " e quindi i vicini sono: ";
        //for(auto& n : C_of_S) std::cout << n << " ";
        //std::cout << std::endl;

        //enhanced(S, graph, k, C_of_S, 0);

        START_REC;
        
        S->pop_back();
        in_S[v] = false;
        excluded[v] = true;
        
        for(auto& v : C_of_S) in_C[v] = false;
        C_of_S.clear();

        //return;
    }
}

int main(int argc, char* argv[]) {
#ifdef __INTEL_COMPILER
    __itt_pause();
#endif
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

    std::vector<node_t> S;

    excluded.resize(graph->size());
    in_S.resize(graph->size());
    in_C.resize(graph->size());
    //tmp.resize(graph->size(), false);

    
    if(!skip) std::cout << "Nodes: " << graph->size() << std::endl;
    
    size_t edges = 0;
    for(size_t i = 0;i < graph->size(); i++)
        edges += graph->degree(i);

    if(!skip) std::cout << "Edges: " << edges << std::endl;
    

    graph->Permute(DegeneracyOrder(*graph));

    signal(SIGALRM, [](int) { interrupted = true; });
    signal(SIGINT, [](int) { interrupted = true; });
    alarm(TIMEOUT); // Set timer 

    std::cerr << "Graph read." << std::endl;
#ifdef __INTEL_COMPILER
    __itt_resume();
#endif
    auto start = std::chrono::high_resolution_clock::now();

    main_enum(&S, graph.get(), k);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
#ifdef __INTEL_COMPILER
    __itt_pause();
#endif

    std::cout << "Found " << counter << " graphlets of size " << k;
    std::cout << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()/1000. << " s" << std::endl;
    std::cout << "Recursion nodes - rec. roots: " << ricorsioni << " - " << graph->size() << " = " << ricorsioni-graph->size() << std::endl;
    //std::cerr << "Solutions per sec: " << counter / std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000 << std::endl;
    std::cout << "Leaves: " << leaves << " fruitful leaves: " << counter << " dead leaves: " << leaves-counter << std::endl;
    std::cout << "Graphlets/leaves ratio: " << (double) counter / leaves << std::endl;

    return (interrupted ? 14 : 0);
}