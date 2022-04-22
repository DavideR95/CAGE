#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <signal.h>

#include "util/graph.hpp"

#ifndef TIMEOUT
#define TIMEOUT (60 * 60 * 3) /* Timer 3 hours */
#endif

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
//std::vector<bool> X;

bool interrupted = false;

/*std::random_device rd;
auto seed = rd(); // Quello incriminato è 3882478540 clique.nde 3
// Per prova.nde 3 -> 3154536903

std::mt19937 mt(seed); */

//std::vector<size_t> indexes;


void on_alarm(int) {
    interrupted = true;
}

using node_t = uint32_t;


bool graphlet_enum(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k) {

    if(S->size() == k) {
        counter++;
        /*std::cout << "Trovato Graphlet: ";
        for(auto& v : *S)
            std::cout << v << " ";
        std::cout << std::endl;*/
        return true;
    }

    if(interrupted) return false; // Timer

    node_t v;

    size_t count_neighbors = 0;

    /* if(S->empty()) {
        assert(false);
        size_t i;

        for(i=0;i<indexes.size();i++) {
            if(!excluded[indexes[i]]) {
                v = indexes[i];
                break;
            }
        }

        if(i == indexes.size()) {
            return false; // Tutti esclusi
        }
    }
    else { */
        // Any node connected to any S
        // Qui S non è vuoto

        bool works = false;
        for(node_t node : *S) {
            for(node_t neigh : graph->neighs(node)) {
                // Prima cerco un candidato che non sia già in S e non sia escluso
                if(!excluded[neigh] && !in_S[neigh] /*&& !X[neigh]*/) {
                    v = neigh;
                    works = true;
                    break;
                }
            }
            if(works)
                break;
        }
        if(!works)
            return false; // Non esiste un v adatto

    //}

    S->push_back(v);
    in_S[v] = true;

    bool left = graphlet_enum(S, graph, k);

    S->pop_back();
    in_S[v] = false;
   
    if(left && !interrupted /*|| S->empty()*/) { // Timer
        excluded[v] = true;
        graphlet_enum(S, graph, k); // graph senza v 
        excluded[v] = false;
        
        return true;
    }
    
    return false;
}

void main_enum(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k) {
    for(auto v=0;v<graph->size();v++) {
        if(interrupted) return; // Timer 
        
        S->push_back(v);
        in_S[v] = true;
        // Prima lista C(S)
        graphlet_enum(S, graph, k);
        
        S->pop_back();
        in_S[v] = false;
        excluded[v] = true;
        
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

    std::vector<node_t> S;

    excluded.resize(graph->size());
    in_S.resize(graph->size());
    //X.resize(graph->size());

    if(!skip) {
        std::cout << "Nodes: " << graph->size() << std::endl;
        size_t edges = 0;
        for(size_t i = 0;i < graph->size(); i++)
            edges += graph->degree(i);

        std::cout << "Edges: " << edges << std::endl;
    }

    signal(SIGALRM, on_alarm);
    alarm(TIMEOUT); // Set timer 

    auto start = std::chrono::high_resolution_clock::now();

    main_enum(&S, graph.get(), k);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    std::cerr << "Found " << counter << " graphlets of size " << k;
    std::cerr << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()/1000. << " s" << std::endl;
    std::cerr << "Solutions per sec: " << counter / std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000 << std::endl;

    return (interrupted ? 14 : 0);
}