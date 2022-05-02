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
#define START_REC (enumeration_ultra(S, graph, k, N_of_S, 0))

#define IS_DELETED(neighbor, node) (neighbor < node)

#define IN_ARRAY(elem, arr) (std::find(arr.begin(), arr.end(), elem) != arr.end())

#define CHECK_N(elem) (inverted_N[elem%next_prime])

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

std::unordered_set<node_t> in_N;

bool isPrime(node_t n) {
    node_t i,root;

    if (n%2 == 0 || n%3 == 0)
        return false;

    root = (node_t)sqrt(n);

    for (i=5; i<=root; i+=6)
    {
        if (n%i == 0)
           return false;
    }

    for (i=7; i<=root; i+=6)
    {
        if (n%i == 0)
           return false;
    }

    return true;
}

node_t find_prime_after(node_t n) {
    int i=n+1;
    while(1)
    {
        if(isPrime(i))
            return i;
        i++;
    }
    return 1;
}

bool enumeration_ultra(std::vector<node_t>& S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& N_of_S, int start) {
    recursion_nodes++;

    // INV: S.size() <= k-2

    if(interrupted) return false; // Timer

    auto end = N_of_S.size();
    if(start == end) { leaves++; return false; } // No nuovi nodi
    bool found = false;

    auto first_node = S.front();

    if(S.size() == k-3) {
        uint64_t diff = 0;
        auto neighbors = end - start;

        // Caso 1: 3 nodi a distanza 1
        diff += (neighbors * (neighbors - 1) * (neighbors - 2)) / 6; // Se ci sono 1 o 2 neighbors fa zero e va bene
        
        // Caso 2: un nodo a distanza 1 e due a distanza 2:

        // Caso 3: due nodi a distanza 1 e uno a distanza 2:
        if(neighbors > 0) {

            node_t next_prime = find_prime_after(N_of_S.size() * 2.5);
            std::vector<bool> inverted_N(next_prime+1, false);
            for(int i=start;i<end;i++) inverted_N[N_of_S[i] % next_prime] = true;

            uint64_t contatore = 0;
            for(int i=start;i<end;i++) {
                if(interrupted) break;
                auto u = N_of_S[i];
                uint64_t deg_u = 0;

                auto neighbors_of_u = graph->neighs(u);

                for(auto& neigh : neighbors_of_u) {
                    if(!CHECK_N(neigh) && !IS_DELETED(neigh, first_node) && !IN_ARRAY(neigh, S)) {
                        deg_u++; // Step 2

                        for(auto& v : graph->neighs(neigh)) {
                            if(!CHECK_N(v) && !IS_DELETED(v, first_node) && !IN_ARRAY(v, S) && !graph->are_neighs(u, v)) {
                                diff++; // Step 4
                            }
                        }
                        // Qui v è il neigh di u, devo controllare se è vicino di qualcuno in N(S)
                        for(int j=start;j<end;j++) {
                            auto v = N_of_S[j];
                            if(v != neigh && !graph->are_neighs(neigh, v) && v != u) {
                                contatore++;
                            }
                        }
                    }
                    if(interrupted) break;
                }
                diff += (deg_u * (deg_u - 1)) / 2;

                for(int j=start;j<end;j++) { // Step 3
                    auto v = N_of_S[j];
                    
                    if(u != v) {
                        for(auto& neigh : graph->neighs(v)) {
                            if(neigh != u && !CHECK_N(neigh) && !IS_DELETED(neigh, first_node) && !IN_ARRAY(neigh, S)) {
                                contatore++;
                            }
                        }
                    }

                    if(interrupted) break;
                }
            }
            diff += contatore / 2;
        }

        // Caso 4: uno + uno + uno

        solutions += diff;
        if(diff < min_sol_per_leaf) { min_sol_per_leaf = diff; count_min_leaf = 1; }
        else if(diff == min_sol_per_leaf) count_min_leaf++;
        if(diff > max_sol_per_leaf) { max_sol_per_leaf = diff; count_max_leaf = 1; }
        else if(diff == max_sol_per_leaf) count_max_leaf++;

        leaves++;
        if(diff > 0) fruitful_leaves++;
        return (diff > 0);
    }

    if(S.size() == k-2) {
        auto neighbors = N_of_S.size() - start;
        uint64_t diff = 0;

        node_t next_prime = find_prime_after(N_of_S.size() * 2.5);
        std::vector<bool> inverted_N(next_prime+1, false);
        for(int i=start;i<end;i++) inverted_N[N_of_S[i] % next_prime] = true;

        if(neighbors == 1) {
            for(auto& neigh : graph->neighs(N_of_S[start])) {
                // if(!in_C[neigh] && !excluded[neigh] && !graph->is_in_S(neigh)/*!in_S[neigh]*/) {
                if(!CHECK_N(neigh) && !IS_DELETED(neigh, first_node) && !IN_ARRAY(neigh, S)) {
                    diff++;
                }
            }
        }
        else {
            diff = (neighbors) * (neighbors - 1) / 2; // Questi sono quelli che posso fare a distanza 1
            
            for(int i=start;i<end;i++) {
                auto u = N_of_S[i];
                for(auto& neigh : graph->neighs(u)) {
                    if(!CHECK_N(neigh) && !IS_DELETED(neigh, first_node) && !IN_ARRAY(neigh, S)) {
                        diff++;
                    }
                }
                if(interrupted) break;
            }
        } 

        solutions += diff; 
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
        
        if(IS_DELETED(v, first_node) || IN_ARRAY(v, S)) continue;
        size_t tmp = 0;
        for(auto& neigh : graph->neighs(v)) {
            if(!in_N.count(neigh) && !IS_DELETED(neigh, first_node) && !IN_ARRAY(neigh, S)) {
                N_of_S.push_back(neigh);
                // in_C[neigh] = true;
                // graph->put_in_N(neigh);
                in_N.insert(neigh);
                tmp++;
            }
        }

        // if(tmp == 0 && ) { std::cout << "\t\t\tAAAA\n"; ; }

        bool left;
        // std::cout << "Ora dovrei prendere " << v << std::endl;
        if(start+1 < end+tmp) {
            // Chiamata sx
            S.push_back(v); 
            // in_S[v] = true;
            // graph->put_in_S(v);
            
            left = enumeration_ultra(S, graph, k, N_of_S, start+1);
            
            im_a_parent = true;
            // std::cout << "Finita la rec call di " << S->back() << std::endl;
            S.pop_back();
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
            in_N.erase(N_of_S.back());
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
                in_N.insert(neigh);
            }
        }


        START_REC;
        
        S.pop_back();

        // graph->remove_from_S(v);

        // no need to explicitly delete v
        
        // for(auto& v : N_of_S) graph->remove_from_N(v); 
        N_of_S.clear();
        in_N.clear();

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
    for(size_t i = 0;i < graph->size(); i++) {
        edges += graph->degree(i);
        if(graph->degree(i) > max_degree) max_degree = graph->degree(i);
        // current_degree[i] = graph->degree(i);
    }

    edges /= 2; // Undirected graph
    in_N.reserve(1.5*max_degree);

    if(!skip) std::cout << "Edges: " << edges << std::endl;

    graph->Permute(DegeneracyOrder(*graph));

    signal(SIGALRM, [](int) { interrupted = true; });
    signal(SIGINT, [](int) { interrupted = true; });
    alarm(TIMEOUT); // Set timer 

    std::cerr << "Graph read." << std::endl;

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
