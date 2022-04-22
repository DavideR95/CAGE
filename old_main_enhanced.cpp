/*
bool graphlet_enum(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& C_of_S) {

    if(S->size() == k) {
        counter++
        return true;
    }

    if(interrupted || C_of_S.empty()) return false; // Timer

    node_t v;

    // Any node connected to any S
    // Qui S non è vuoto

    // Selezione di un nodo da C(S) 
    size_t i = 1;
    do {
        v = C_of_S[C_of_S.size()-i]; // Questo è facile
        i++;
    }
    while(i <= C_of_S.size() && (in_S[v] || excluded[v]));

    if(in_S[v] || excluded[v]) {  return false; }

    //in_C[v] = false;
    //C_of_S.pop_back();

    // Fine selezione del nodo da C(S) 

    S->push_back(v);
    in_S[v] = true;

    // Per la chiamata di sx, C(S) è cambiato, quindi lo aggiorniamo con i vicini di v

    size_t old_C_size = C_of_S.size();
    bool works = true;
    for(auto& neigh : graph->neighs(v))
        if(!in_C[neigh] && !excluded[neigh] && !in_S[neigh]) {
            C_of_S.push_back(neigh);
            in_C[neigh] = true;
            works = true;
        }

    //if(!works) { assert(C_of_S.size() == old_C_size); return false; }
    //std::cout << "Chiamata sx con v = " << v << std::endl;
    bool left = graphlet_enum(S, graph, k, C_of_S);

    // Ripristino il vecchio C(S) 
    while(C_of_S.size() > old_C_size) {
        //std::cout << "E empty? " << C_of_S.size() << std::endl;
        //assert(false);
        in_C[C_of_S.back()] = false;
        C_of_S.pop_back();
    }

    S->pop_back();
    in_S[v] = false;      

    if(left && !interrupted) {
        excluded[v] = true;
        //std::cout << "\t Vado a destra escludendo il " << v << " ";
        graphlet_enum(S, graph, k, C_of_S); // graph senza v 
        //std::cout << "\tFine dx" << std::endl;
        excluded[v] = false;
        
        return true;
    }
    
    return false;
}
*/


bool ciproviamo(std::vector<node_t>* S, fast_graph_t<node_t, void>* graph, int k, std::vector<node_t>& C_of_S, int start, int cnt) {
    ricorsioni++;
    if(S->size() == k) {
        counter++;
        return true;
    }

    if(interrupted) return false;

    if(start == C_of_S.size()) return false;

    auto end = C_of_S.size();
    bool kmenol = false;
    auto old_start = start;

    bool found = false;
    if(S->size() == k-1 && end - start >= 1) { // Anche i padri delle foglie ora fanno output
        counter++;
        start++;
        old_start++;
        found = true;
    }
    /*else if(S->size() < k-1 && end - start >= k - S->size()) {
        counter++;
        // Lui ripassa da qui, è questo che non va bene
        std::cout << "Sono passato da qui quando la S.size era " << S->size() << std::endl;
        for(auto i=0;i<k-S->size();i++) {
            tmp[C_of_S[start+i]] = true;
            std::cout << "Ho messo a true il nodo " << C_of_S[start+i] << std::endl;
        }
        start += k-S->size(); // Non va bene
    }
    else {
        std::cout << "La s size è " << S->size() << " e end-start = " << end << "-" << start << " = " << end-start << std::endl;
    }*/


    for(;start < end; start++) {
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
        bool left = ciproviamo(S, graph, k, C_of_S, start+1, cnt);

        S->pop_back();
        in_S[v] = false;
        if(tmp[v]) cnt--;

        while(C_of_S.size() > old_size) {
            in_C[C_of_S.back()] = false;
            C_of_S.pop_back();
        }
        //if(interrupted) return false;

        if(left && !interrupted)
            found = true;
        else
            break;

    }

    return found;

    if(S->size() < k-1 && end - old_start >= k - S->size()) {
        for(auto i=0;i<k-S->size();i++) {
            tmp[C_of_S[old_start+i]] = false;
            //std::cout << "Metto a false il nodo che: " << C_of_S[old_start+i] << " (cnt = " << cnt << std::endl;
        }
    }
}