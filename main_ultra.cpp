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
#define TIMEOUT (60 * 30) /* Timer 30 minutes */
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

bool enhanced(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& N_of_S, int start) {
    recursion_nodes++;

    // INV: S.size() <= k-2

    if(interrupted) return false; // Timer

    if(start == N_of_S.size()) { leaves++; return false; } // No nuovi nodi
    auto end = N_of_S.size();
    bool found = false;

    if(S->size() == k-3) {
        // std::ofstream f("../../out_ultra.txt", std::ios::out);
        uint64_t diff = 0;
        auto neighbors = N_of_S.size() - start;

        // Caso 1: 3 nodi a distanza 1
        diff += (neighbors * (neighbors - 1) * (neighbors - 2)) / 6; // Se ci sono 1 o 2 neighbors fa zero e va bene
        // std::cout << "I neighbor sono " << neighbors << " per cui diff = " << diff << std::endl;
        
        auto diff1 = diff;
        // Caso 2: un nodo a distanza 1 e due a distanza 2:
        // std::unordered_set<node_t> n_quadro_S;
        // n_quadro_S.reserve(150);

        for(int i=start;i<end;i++) {
            auto u = N_of_S[i];
            //std::cout << "neigh " << u << std::endl;
            uint64_t deg_u = 0;
            // std::cout << "u = " << u << std::endl;
            for(auto& neigh : graph->neighs(u)) {
                if(!in_C[neigh] && !excluded[neigh] &&/*!graph->is_in_S(neigh)*/!in_S[neigh]) {
                    deg_u++;
                    // std::cout << neigh << " è vicino di u = " << u << std::endl;
                    // n_quadro_S.insert(neigh);
                }
            }
            diff += (deg_u * (deg_u-1)) / 2;
        }
        auto diff2 = diff - diff1;
        //std::cout << "Col metodo 2 ne ho trovati altri " << diff2 << std::endl;



        // Caso 3: due nodi a distanza 1 e uno a distanza 2:
        if(neighbors > 1) {
            uint64_t contatore = 0;
            for(int i=start;i<end;i++) {
                auto u = N_of_S[i];
                for(int j=start;j<end;j++) {
                    auto v = N_of_S[j];
                    // assert(!n_quadro_S.count(v) && !n_quadro_S.count(u));
                    if(u != v) {
                        for(auto& neigh : graph->neighs(v)) {
                            if(neigh != u && !in_C[neigh] && !excluded[neigh] && /*!graph->is_in_S(neigh)*/!in_S[neigh]) {
                                contatore++;
                            }
                        }
                        for(auto& neigh : graph->neighs(u)) {
                            if(neigh != v && !in_C[neigh] && !excluded[neigh] && /*!graph->is_in_S(neigh)*/!in_S[neigh] && !graph->are_neighs(v, neigh)) {
                                contatore++;
                            }
                        }
                    }
                }
            }
            diff += contatore / 2;
        }
        auto diff3 = diff - diff2 - diff1;
        //std::cout << "Col metodo 3 ne ho trovati altri " << diff3 << std::endl;

        // Caso 4: uno + uno + uno
        for(int i=start;i<end;i++) {
            auto u = N_of_S[i]; // Scelgo u
            //std::cout << "Vedo u che è: " << u << std::endl;
            for(auto& v : graph->neighs(u)) {
                if(!in_C[v] && !excluded[v] && /*!graph->is_in_S(v)*/!in_S[v] /*&& n_quadro_S.count(v)*/) { // Scelgo v
                    //std::cout << "Poi vedo v che è " << v << std::endl;
                    // assert(n_quadro_S.count(v));
                    for(auto& neigh : graph->neighs(v)) {
                        if(!in_C[neigh] && !excluded[neigh] && /*!graph->is_in_S(neigh)*/!in_S[neigh] && !graph->are_neighs(u, neigh) /*&& !n_quadro_S.count(neigh)*/) {
                            diff++;
                        }
                    }
                }
            }
        }

        auto diff4 = diff - diff3 - diff2 - diff1;
        //std::cout << "Col metodo 4 ne ho trovati altri " << diff4 << std::endl;

        solutions += diff;
        if(diff < min_sol_per_leaf) { min_sol_per_leaf = diff; count_min_leaf = 1; }
        else if(diff == min_sol_per_leaf) count_min_leaf++;
        if(diff > max_sol_per_leaf) { max_sol_per_leaf = diff; count_max_leaf = 1; }
        else if(diff == max_sol_per_leaf) count_max_leaf++;

        leaves++;
        if(diff > 0) fruitful_leaves++;
        // f.close();
        return (diff > 0);
    }

    if(S->size() == k-2) {
        auto neighbors = N_of_S.size() - start;
        uint64_t diff = 0;

        if(neighbors == 1) {
            for(auto& neigh : graph->neighs(N_of_S[start])) {
                if(!in_C[neigh] && !excluded[neigh] && /*!graph->is_in_S(neigh)*/!in_S[neigh]) {
                    diff++;
                }
            }
        }
        else {
            diff = (neighbors) * (neighbors - 1) / 2; // Questi sono quelli che posso fare a distanza 1
            
            assert(neighbors > 1);

            // qui devo ciclare tra i k-1 

            for(int i=start;i<end;i++) {
                auto u = N_of_S[i];
                for(auto& neigh : graph->neighs(u)) {
                    if(!in_C[neigh] && !excluded[neigh] && /*!graph->is_in_S(neigh)*/!in_S[neigh]) {
                        diff++;
                    }
                }
            }
        } 

        solutions += diff; 
        //assert(diff != 0);
        if(diff < min_sol_per_leaf) { min_sol_per_leaf = diff; count_min_leaf = 1; }
        else if(diff == min_sol_per_leaf) count_min_leaf++;
        if(diff > max_sol_per_leaf) { max_sol_per_leaf = diff; count_max_leaf = 1; }
        else if(diff == max_sol_per_leaf) count_max_leaf++;

        leaves++;
        if(diff > 0) fruitful_leaves++;
        return (diff > 0);
    } 

    bool im_a_parent = false;

    for(;start < end; start++) {
        if(interrupted) return false;

        auto old_size = N_of_S.size();
        auto v = N_of_S[start];
        if(excluded[v] || graph->is_in_S(v)/*in_S[v]*/) continue;
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
            S->push_back(v); 
            in_S[v] = true;
            // graph->put_in_S(v);
            
            left = enhanced(S, graph, k, N_of_S, start+1);
            
            im_a_parent = true;
            // std::cout << "Finita la rec call di " << S->back() << std::endl;
            S->pop_back();
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




void main_enum(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k) {
    // std::set<node_t, vertexCmp> C_of_S;
    std::vector<node_t> N_of_S;
    N_of_S.reserve(graph->size()/2);
    for(auto v=0;v<graph->size()-k+1;v++) {
    
    /*for(auto v=103;v<104;v++) {
        for(auto i=0;i<v;i++) excluded[i] = true;*/
        if(interrupted) return; // Timer 
        S->push_back(v);
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
        
        S->pop_back();
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

    main_enum(&S, graph.get(), k);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    __itt_pause();

    std::cerr << "Found " << solutions << " graphlets of size " << k;
    std::cerr << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()/1000. << " s" << std::endl;
    std::cerr << "Recursion nodes - rec. leaves: " << recursion_nodes << " - " << leaves << " = " << recursion_nodes-leaves << std::endl;
    //std::cerr << "Solutions per sec: " << counter / std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000 << std::endl;
    std::cerr << "Leaves: " << leaves << " fruitful leaves: " << fruitful_leaves << " dead leaves: " << (leaves-fruitful_leaves);
    std::cerr << " ( " << (double) (leaves-fruitful_leaves)/leaves * 100. << " % )" << std::endl;
    std::cerr << "Graphlets/leaves ratio: " << (double) solutions / leaves << std::endl;
    std::cerr << "Average sol. per leaf: " << (double) solutions / fruitful_leaves << std::endl;
    std::cerr << "Min sol. per leaf: " << min_sol_per_leaf << " ( " << count_min_leaf << " times )" << std::endl;
    std::cerr << "Max sol. per leaf: " << max_sol_per_leaf << " ( " << count_max_leaf << " times )"<< std::endl;

    return (interrupted ? 14 : 0);
}
