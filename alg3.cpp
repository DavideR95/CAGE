#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <stack>
//#include <unordered_set>
#include <set>
#include <unistd.h>
#include <signal.h>

#include "util/graph.hpp"

#ifndef TIMEOUT
#define TIMEOUT (60 * 60 * 3) /* Timer 3 hours */
#endif

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

template <typename node_t, typename label_t>
std::unique_ptr<fast_graph_t<node_t, label_t>> ReadFastGraph(
    const std::string& input_file, bool directed = false) {
    FILE* in = fopen(input_file.c_str(), "re");
    if (!in) throw std::runtime_error("Could not open " + input_file);
    return ReadNde<node_t, fast_graph_t>(in, directed);
}

using node_t = uint32_t;

uint64_t counter = 0;
std::vector<bool> excluded;
std::vector<bool> in_S;
std::vector<bool> visited;
std::stack<node_t> visited_list;

bool interrupted = false;

void on_alarm(int) {
    interrupted = true;
}


bool fruitful(fast_graph_t<node_t, void>* G,
              std::vector<node_t>* S,
              std::vector<bool>* X,
              int k) {

    assert(S->size() < k);

    if(interrupted) return false;

    //std::cout << "Fruit start" << std::endl;


    std::stack<node_t> frontier; // La pila della DFS
    //std::set<node_t> visited;

    bool success = false;


    for(auto& v : *S) { // sono le source della DFS
        frontier.push(v);
        //std::cout << "Parto con la DFS da " << v << std::endl;
        while(!frontier.empty()) {
            auto u = frontier.top(); // Estraggo in coda (LIFO)
            frontier.pop();
            visited[u] = true;
            visited_list.push(u);
            if(visited.size() /*+ S->size() */>= k) {
                success = true;
                break;
            }

            size_t count = 0;
            //std::cout << "Estraggo " << u << " dalla coda e lo visito" << std::endl;
            for(auto& neigh : G->neighs(u)) {
                //std::cout << "Vicino " << neigh << " di " << u << std::endl;
                if(!((*X)[neigh]) && !excluded[neigh] && !in_S[neigh] && !visited[neigh]) {
                    frontier.push(neigh);
                    count++;
                }
                //std::cout << " (visited.size = " << visited.size() << std::endl;
            }
            if(count + S->size() >= k) {
                success = true;
                break;
            }
        }
        //visited.clear();
    }
    //std::cout << "Fruit end" << std::endl; 

    while(!visited_list.empty()) {
        visited[visited_list.top()] = false;
        visited_list.pop();
    }

    return success;
}


void graphlet_enum(fast_graph_t<node_t, void>* G,
                   std::vector<node_t>* S,
                   std::vector<bool>* X,
                   int k) {

    if(S->size() == k) {
        counter++;
        /*std::cout << "\t\tTrovato: ";
        for(auto& v : *S)
            std::cout << v << " ";
        std::cout << std::endl;*/
        return;
    }

    if(interrupted) return; // Timer check
  
    std::set<node_t> vicinato;
    //vicinato.reserve(2*k);

    size_t old_S_size = S->size();

    node_t x, y; 
    bool pick_y;
    size_t start = 0;
    // Gli excluded sono anche quelli in X
    /*std::cout << "Benvenuti nella chiamata con S = ";
    for(auto& v : *S) std::cout << v << " ";
    std::cout << std::endl;*/
    do {
        vicinato.clear();
        pick_y = false;
        node_t last_neigh;
        /* Ottimizzare (ok) */
        for(size_t i=start;i<S->size();i++) {
            auto v = (*S)[i];
            for(auto neigh : G->neighs(v)) {
                //std::cout << "Ecco il vicino " << neigh << " di " << v << "... ";
                if(!excluded[neigh] && !((*X)[neigh]) && !in_S[neigh] && !vicinato.count(neigh)) {
                    vicinato.insert(neigh);
                    last_neigh = neigh;
                    if(!pick_y) {
                        x = neigh;
                        pick_y = true;
                    }
                    else {
                        y = neigh;
                        //pick_y = false;
                    }
                    //std::cout << "L'ho aggiunto";
                }
                //std::cout << std::endl;
            }
        }
        if(vicinato.size() == 1) {
            S->push_back(last_neigh);
            in_S[last_neigh] = true;
            start = S->size() - 1;
        }
        //std::cout << "???" << std::endl;
    }
    while(vicinato.size() == 1 && S->size() < k);


    if(S->size() == k) {
        counter++;
        /*std::cout << "\t\tTrovato dal path: ";
        for(auto& v : *S)
            std::cout << v << " ";
        std::cout << std::endl;*/
        while(S->size() != old_S_size) {
            in_S[S->back()] = false;
            S->pop_back();
        }
        /*std::cout << "Alla fine dentro S ci ho lasciato ";
        for(auto& v : *S)
            std::cout << v << " ";*/
        //std::cout << std::endl;

        return;
    }

    if(vicinato.empty()) { // Devo togliere quelli che ho messo quando vicinato.size() == 1  
        //std::cout << "Zan zan" << std::endl;
        while(S->size() != old_S_size) {
            in_S[S->back()] = false;
            S->pop_back();
        }
        return;
    }

    // Se arrivo qui vuol dire che |C(S)| > 1

    node_t z;
    excluded[x] = true;
    if(S->size() + vicinato.size() - 1 >= k || fruitful(G, S, X, k)) // -1 perché tolgo x
        z = x;
    else
        z = y;
    excluded[x] = false;


    // X = vuoto
    if(!fruitful(G, S, X, k) || interrupted) { // Includes timer check
        while(S->size() != old_S_size) {
            in_S[S->back()] = false;
            S->pop_back();
        }
        return; // Non devo ripristinare X perché X era vuoto
    }
    S->push_back(z);
    in_S[z] = true;
    //std::cout << "Ho aggiunto z che è " << z << std::endl;
    graphlet_enum(G, S, X, k); // Enum(S \cup {z}, G \ X)
    //std::cout << "Fine della chiamata con z = " << z << std::endl;
    //std::cout << "Quindi il back che poppo è: " << S->back() << std::endl;
    S->pop_back();
    in_S[z] = false;
    excluded[z] = true;
    /*std::cout << "Prima del foreach il vicinato è: ";
    for(auto& v : vicinato) std::cout << v << " ";
    std::cout << " e S è ";
    for(auto& v : *S) std::cout << v << " ";
    std::cout << std::endl;*/
    // Ora completare il for sui restanti vertici del vicinato
    for(auto& v : vicinato) { // vicinato = C(S)
        if(interrupted) return; // Timer check
        if(v != z) {
            if(!fruitful(G, S, X, k)) {
                //std::cout << "Ma non è fruitful" << std::endl;
                break; // E ripristina X
            }
            S->push_back(v);
            in_S[v] = true;
            //std::cout << "Dentro il foreach v è " << v << std::endl;
            graphlet_enum(G, S, X, k);
            //std::cout << "Fine della chiamata di " << v << " del foreach" << std::endl;
            S->pop_back(); 
            in_S[v] = false;
            excluded[v] = true;
        }
    }

    while(S->size() != old_S_size) {
        in_S[S->back()] = false;
        S->pop_back();
    }

    // Ripristino "X" (cioè excluded)
    for(auto& v : vicinato) {
        excluded[v] = false;
        //std::cout << "Ora " << v << " non è più escluso\n";
    }

    excluded[z] = false;
   
}

void enum_main(fast_graph_t<node_t, void>* G, int k) {
    std::unique_ptr<std::vector<bool>> X = make_unique<std::vector<bool>>();
    std::unique_ptr<std::vector<node_t>> S = make_unique<std::vector<node_t>>();
    
    X->resize(G->size());
    S->reserve(k);

    for(auto v=0;v<G->size();v++) {
        if(interrupted) return; // Timer check
        //std::cout << "\tBig chiamata con v = " << v << std::endl;
        S->push_back(v);
        in_S[v] = true;
        graphlet_enum(G, S.get(), X.get(), k);
        //std::cout << "Chi è il back di S? " << S->back() << std::endl;
        S->pop_back();
        in_S[v] = false;
        (*X)[v] = true;
        //std::cout << "\tFine big chiamata con v = " << v << std::endl;
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

    auto graph = ReadFastGraph<node_t, void>(argv[1]);

    if(!skip) {
        std::cout << "Nodes: " << graph->size() << std::endl;
        size_t edges = 0;
        for(size_t i = 0;i < graph->size(); i++)
            edges += graph->degree(i);

        std::cout << "Edges: " << edges << std::endl;
    }

    excluded.resize(graph->size());
    in_S.resize(graph->size());
    visited.resize(graph->size(), false);

    signal(SIGALRM, on_alarm);
    alarm(TIMEOUT); // Set timer

    auto start = std::chrono::high_resolution_clock::now();

    enum_main(graph.get(), k);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()/1000.;

    std::cerr << "Found " << counter << " graphlets of size " << k;
    std::cerr << " in " << seconds << " s" << std::endl;
    std::cerr << "Solutions per sec: " << counter / std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000 << std::endl;


    return (interrupted ? 14 : 0);
}