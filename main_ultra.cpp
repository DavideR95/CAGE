#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <signal.h>

#include "util/graph.hpp"
#include "permute/permute.hpp"

// Used for vtune code profiling
#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
#include "/opt/intel/oneapi/vtune/latest/sdk/include/ittnotify.h"
#endif

/* Likely / unlikely macros for branch prediction */
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

/* Timer 30 minutes */
#define TIMEOUT (60 * 15) 

/* Check if neighbor is currently deleted from the graph */
#define IS_DELETED(neighbor, node) (neighbor < node) 

/* (Linear) Search for elem in std::vector arr */
#define IN_ARRAY(elem, arr) (std::find(arr.begin(), arr.end(), elem) != arr.end()) 

/* Look for elem into N via inverted_N hash table or into S with linear search */
#define IS_IN_N_OR_S(elem) (inverted_N.count(elem) || (std::find(S.begin(), S.end(), elem) != S.end()) )

/* Auxiliary function used to read input graphs */
template <typename node_t, typename label_t>
std::unique_ptr<fast_graph_t<node_t, label_t>> ReadFastGraph(
    const std::string& input_file, bool nde = true, bool directed = false) {
    FILE* in = fopen(input_file.c_str(), "re");
    if (!in) throw std::runtime_error("Could not open " + input_file);
    return (nde) ? ReadNde<node_t, fast_graph_t>(in, directed) : ReadOlympiadsFormat<node_t, fast_graph_t>(in, directed);
}

// Solutions counter
uint64_t solutions = 0;


// Timeout handler
bool interrupted = false; 

// We assume to work with less than 2^32-1 vertices
using node_t = uint32_t; // This can be changed to uint64_t if needed

/* Statistical counters */
uint64_t recursion_nodes = 0; // Total number of nodes in the recursion tree generated
uint64_t leaves = 0; // Total number of leaves in the recursion tree
uint64_t fruitful_leaves = 0; // Total number of leaves that produced at least one solution
uint64_t min_sol_per_leaf(-1); // Minimum number of solutions found in one fruitful leaf
uint64_t max_sol_per_leaf = 0L; // Maximum number of solutions found in one fruitful leaf
uint64_t count_min_leaf = 0L; // How many leaves achieved the minimum
uint64_t count_max_leaf = 0L; // How many leaves achieved the maximum
size_t avg_n_of_s_size = 0; // Average size of N(S) throughout the recursion

// Inverted index (hash table) for N(S): contains all and only nodes currently in N(S)
cuckoo_hash_set<node_t> inverted_N;


/* Main recursive function
 * assumes k > 2 and S.size() <= k-2
 * (k-2 case is present in order to handle k=3) */
bool enumeration_ultra(std::vector<node_t>& S, fast_graph_t<node_t, void>* graph, unsigned short k, std::vector<node_t>& N_of_S, int start) {
    recursion_nodes++; // Every call counts as a recursion node

    // INV: S.size() <= k-2

    if(unlikely(interrupted)) return false; // Timer expired, stop working

    auto end = N_of_S.size(); 
    avg_n_of_s_size += end; // Used for statistical purposes 
    if(unlikely(start == end)) { leaves++; return false; } // There are no new nodes to process, return
    bool found = false; // Has this call found any solution?

    if(S.size() == k-3) {
        uint64_t diff = 0;
        auto neighbors = end - start; // |N(S)|

        // First case: pick any combination of 3 vertices from N(S)
        // This is the binomial coefficient (|N(S)| over 3)
        diff += (neighbors * (neighbors - 1) * (neighbors - 2)) / 6; 
        
        // Caso 2: un nodo a distanza 1 e due a distanza 2:

        // Remaning three cases to pick three nodes connected to S:
        // Case 2 (denoted with 1 + 2 + 0): pick one node in N(S) and then two of its neighbors in N^2(S)
        // Case 3 (denoted with 2 + 1 + 0): pick two nodes in N(S) and then one of their neighbors in N^2(S)
        // Case 4: (denoted with 1 + 1 + 1): pick one node in N(S), a neighbor in N^2(S) and a neighbor in N^3(S)

        if(likely(neighbors > 0)) { 
            uint64_t counter_dist_1 = 0; // Used for case 3
            for(int i=start;i<end;i++) { 
                if(unlikely(interrupted)) break;
                auto u = N_of_S[i]; // First, pick a node from N(S), this is needed for all the three cases
                uint64_t deg_u = 0; // Used for 1 + 2 + 0

                for(auto& neigh : graph->neighs(u)) { // Scan the neighbors of u
                    if(!IS_DELETED(neigh, S.front()) && !IS_IN_N_OR_S(neigh)) {
                        // Here we are in N^2(S) from the viewpoint of u
                        deg_u++; // Count neighbors of u in N^2(S) [1+2+0]

                        // For case 3, we need to pick another v in N(S)
                        for(int j=start;j<end;j++) {
                            auto v = N_of_S[j]; // Scan N(S) again
                            if(likely(v != u && v != neigh)) { // Avoid u from N(S) and the current neighbor of u
                                // [2+1+0]
                                if(likely(!graph->are_neighs(neigh, v)))
                                    diff++; 
                                else 
                                    counter_dist_1++; // These nodes will be counted two times, one from v and one from u
                            }
                        }

                        for(auto& v : graph->neighs(neigh)) {
                            if(!IS_DELETED(v, S.front()) && !IS_IN_N_OR_S(v) && !graph->are_neighs(u, v)) {
                                // Here we are in N^3(S) from the viewpoint of v, neighbor of u
                                diff++; // [1+1+1]
                            }
                        }
                        
                    }
                    if(unlikely(interrupted)) break;
                }
                // Add the possible combinations of the neighbors of u in N^2(S)
                diff += (deg_u * (deg_u - 1)) / 2;
            }
            // Avoid the duplicates generated from line 113
            diff += counter_dist_1 / 2;
        }

        // Add all the solutions found
        solutions += diff;
        // Update statistics
        if(diff > 0 && diff < min_sol_per_leaf) { min_sol_per_leaf = diff; count_min_leaf = 1; }
        else if(diff == min_sol_per_leaf) count_min_leaf++;
        if(diff > max_sol_per_leaf) { max_sol_per_leaf = diff; count_max_leaf = 1; }
        else if(diff == max_sol_per_leaf) count_max_leaf++;



        leaves++;
        if(diff > 0) fruitful_leaves++;
        return (diff > 0);
    }
    else if(unlikely(S.size() == k-2)) { // This is just a fallback for k = 3
        auto neighbors = N_of_S.size() - start; // |N(S)|
        uint64_t diff = 0;

        if(neighbors == 1) {
            // If there's just one neighbor we need to always use it and take all its neighbors.
            for(auto& neigh : graph->neighs(N_of_S[start])) {
                // Using S.front() helps keeping S (very small) in cache
                if(!IS_DELETED(neigh, S.front()) && !IS_IN_N_OR_S(neigh)) {
                    diff++; // Every neighbor contributes to a solution (kind: 1+1)
                }
            }
        }
        else { // There is more than one vertex in N(S)
            // First, compute the number of solutions using pairs of nodes from N(S)
            diff = (neighbors) * (neighbors - 1) / 2; // (kind: 2+0)
            
            // Then, pick one vertex from N(S) and one vertex from N^2(S)
            for(int i=start;i<end;i++) {
                auto u = N_of_S[i]; 
                for(auto& neigh : graph->neighs(u)) {
                    if(!IS_DELETED(neigh, S.front()) && !IS_IN_N_OR_S(neigh)) {
                        diff++; // Every neighbor of u corresponds to a solution (kind: 1+1)
                    }
                }
                if(interrupted) break;
            }
        } 

        solutions += diff; // Update the number of solutions found
        // Update statistics
        if(diff > 0 && diff < min_sol_per_leaf) { min_sol_per_leaf = diff; count_min_leaf = 1; }
        else if(diff == min_sol_per_leaf) count_min_leaf++;
        if(diff > max_sol_per_leaf) { max_sol_per_leaf = diff; count_max_leaf = 1; }
        else if(diff == max_sol_per_leaf) count_max_leaf++;

        leaves++; // This is a leaf node in the recursion tree
        if(diff > 0) fruitful_leaves++; // If this node found any solution, then it is a fruitful leaf
        return (diff > 0);
    } 

    bool im_a_parent = false; // Used to check if this is a dead leaf

    // Recursive part: take each node from N(S), one at a time and add it to S
    for(;start < end; start++) { 
        if(unlikely(interrupted)) return false;

        auto old_size = N_of_S.size(); // Used to
        auto v = N_of_S[start];
        
        // If this neighbor is deleted or it's already part of the solution, skip it
        if(unlikely(IS_DELETED(v, S.front()) || IN_ARRAY(v, S))) continue;
        size_t increase = 0; // How many new vertices will v bring to N(S)?
        for(auto& neigh : graph->neighs(v)) {
            if(!IS_DELETED(neigh, S.front()) && !IS_IN_N_OR_S(neigh)) {
                // Pick only vertices not already deleted, not already in N(S) and not already in S
                N_of_S.push_back(neigh);
                inverted_N.insert(neigh);
                increase++;
            }
        }

        bool left; 
        // If there are still nodes to process in the next recursive calls
        if(start+1 < end+increase) {
            // Add v to S
            S.push_back(v); 
            // Recur with the new S (this is called the "left" call)
            left = enumeration_ultra(S, graph, k, N_of_S, start+1); // Advance with the start pointer (process N(S) in order)
            
            im_a_parent = true; // We have generated a child node in the recursion tree, so we are not a leaf
            
            S.pop_back(); // Remove v from S
        }
        else {
            // No room to make new calls, the (fake) "left" call failed
            left = false;
        }

        // Remove the nodes previously added to N(S) by using v
        while(N_of_S.size() > old_size) {
            inverted_N.erase(N_of_S.back());
            N_of_S.pop_back();
        }
        if(unlikely(interrupted)) return false; // Check the timer

        if(left)
            found = true; // This call found at least one solution
        else
            break; // If this call did not find solutions, no subsequent calls with this S can
    }

    if(!found && !im_a_parent) { leaves++; } 

    return found;
}

/* Main loop used to start the recursive algorithm.
 * Processes each node one at a time */
void main_loop(fast_graph_t<node_t, void>* graph, unsigned short k, size_t max_degree) {
    std::vector<node_t> N_of_S; // N(S) is a vector of nodes
    N_of_S.reserve(1.5*max_degree); // Make room for enough neighbors

    std::vector<node_t> S;
    S.reserve(k);
    auto size = graph->size();
    std::cout << "---------------------------------" << std::endl;

    // Traverse all vertices in index order
    for(auto v=0;v<size-k+1;v++) {
        if(unlikely(interrupted)) return; // Check the timer 

        std::cerr << "\rProcessing node " << v << "/" << size << " (degree = " << graph->degree(v) << ")..."; 

        S.push_back(v); // S = {v}

        // Add only the forward neighbors of v to N(S)
        // This way we will find all the graphlets that contain v as the "smallest" vertex
        for(auto& neigh : graph->fwd_neighs(v)) { 
            // If neigh is deleted, don't add it to N(S)
            if(!IS_DELETED(neigh, v)) { 
                N_of_S.push_back(neigh);
                inverted_N.insert(neigh);
            }
        }

        // Start the recursion from v
        enumeration_ultra(S, graph, k, N_of_S, 0);
        
        // Remove v from S
        S.pop_back();

        // There is no need to explicitly delete v because the nodes are processed in order
        // Thus only vertices < v will be considered deleted

        // Clear the additional data structures for the next vertex
        N_of_S.clear();
        inverted_N.clear();

    }

    std::cerr << "Done. " << std::endl << "---------------------------------" << std::endl;
}

int main(int argc, char* argv[]) {
    // Used for vtune code profiling
#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
    __itt_pause();
#endif
    bool is_nde = true; // Input format
    unsigned short k;

    switch(argc) {
        case 3:
            k = static_cast<unsigned short>(std::stoi(argv[2]));
            break;
        case 4:
            k = static_cast<unsigned short>(std::stoi(argv[2]));
            is_nde = false;
            break;
        default: 
            std::cerr << "Usage: " << argv[0] << " <graph_file.nde> <k> [Skip Printing Graph Info]" << std::endl;
            return 1;
    }
    
    // Read the input graph from file
    auto graph = ReadFastGraph<node_t, void>(argv[1], is_nde);

    std::cout << "Nodes: " << graph->size() << std::endl;
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

    std::cout << "Edges: " << edges << std::endl;

    if(unlikely(k <= 2)) {
        std::cout << "Found " << ((k <= 1) ? graph->size() : edges) << " graphlets of size " << k << std::endl;
        std::cout << "No computations done, exiting." << std::endl;
        return 0;
    }

    // Find a degeneracy ordering of the graph
    graph->Permute(DegeneracyOrder(*graph));

    // Set the timeout
    signal(SIGALRM, [](int) { interrupted = true; });
    signal(SIGINT, [](int) { interrupted = true; });
    alarm(TIMEOUT);
    // Output stats on the graph (used to understand when the graph is ready)
    std::cerr << "Graph read. ";
    std::cout << "Max degree: " << max_degree << " avg. degree: " << edges/graph->size() << " degeneracy: " << deg << std::endl;
    std::cerr << "Starting the computation... use CTRL-C to manually stop or wait for the timeout (";
    std::cerr << TIMEOUT << "s)." << std::endl;
#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
    __itt_resume(); // For vtune profiling
#endif

    // Start working
    auto start = std::chrono::high_resolution_clock::now();

    main_loop(graph.get(), k, max_degree); // Start the enumeration

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)   
    __itt_pause(); // For vtune profiling
#endif

    // Output results
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
    std::cout << "Avg |N(S)|: " << avg_n_of_s_size / recursion_nodes << std::endl;

    return (interrupted ? 14 : 0);
}
