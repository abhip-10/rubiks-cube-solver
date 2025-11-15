// gen_pdb.cpp
// Phase 6: Pattern Database Generator using BFS
// Compile: g++ -O2 -std=c++17 gen_pdb.cpp -o gen_pdb.exe
// Usage: gen_pdb.exe corners pdbs/corners.bin
//        gen_pdb.exe edges_a pdbs/edges_a.bin

#include <bits/stdc++.h>
using namespace std;

// ============================================================================
// CUBIE REPRESENTATION (copied from main solver)
// ============================================================================

struct CubieCube {
    array<uint8_t,8> cp;
    array<uint8_t,8> co;
    array<uint8_t,12> ep;
    array<uint8_t,12> eo;
    
    CubieCube(){ 
        for(int i=0;i<8;i++){cp[i]=i; co[i]=0;} 
        for(int i=0;i<12;i++){ep[i]=i; eo[i]=0;} 
    }
    
    CubieCube operator*(const CubieCube &b) const {
        CubieCube result;
        for(int i=0; i<8; i++) {
            result.cp[i] = cp[b.cp[i]];
            result.co[i] = (co[b.cp[i]] + b.co[i]) % 3;
        }
        for(int i=0; i<12; i++) {
            result.ep[i] = ep[b.ep[i]];
            result.eo[i] = (eo[b.ep[i]] + b.eo[i]) % 2;
        }
        return result;
    }
};

// ============================================================================
// MOVE TABLES
// ============================================================================

CubieCube moveU() {
    CubieCube c;
    c.cp = {1, 2, 3, 0, 4, 5, 6, 7};
    c.co = {0, 0, 0, 0, 0, 0, 0, 0};
    c.ep = {1, 2, 3, 0, 4, 5, 6, 7, 8, 9, 10, 11};
    c.eo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return c;
}

CubieCube moveR() {
    CubieCube c;
    c.cp = {3, 1, 2, 7, 0, 5, 6, 4};
    c.co = {2, 0, 0, 1, 1, 0, 0, 2};
    c.ep = {11, 1, 2, 3, 8, 5, 6, 7, 0, 9, 10, 4};
    c.eo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return c;
}

CubieCube moveF() {
    CubieCube c;
    c.cp = {4, 0, 2, 3, 5, 1, 6, 7};
    c.co = {1, 2, 0, 0, 2, 1, 0, 0};
    c.ep = {0, 8, 2, 3, 4, 9, 6, 7, 5, 1, 10, 11};
    c.eo = {0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0};
    return c;
}

CubieCube moveD() {
    CubieCube c;
    c.cp = {0, 1, 2, 3, 7, 4, 5, 6};
    c.co = {0, 0, 0, 0, 0, 0, 0, 0};
    c.ep = {0, 1, 2, 3, 7, 4, 5, 6, 8, 9, 10, 11};
    c.eo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return c;
}

CubieCube moveL() {
    CubieCube c;
    c.cp = {0, 5, 1, 3, 4, 6, 2, 7};
    c.co = {0, 1, 2, 0, 0, 2, 1, 0};
    c.ep = {0, 1, 9, 3, 4, 5, 10, 7, 8, 6, 2, 11};
    c.eo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return c;
}

CubieCube moveB() {
    CubieCube c;
    c.cp = {0, 1, 6, 2, 4, 5, 7, 3};
    c.co = {0, 0, 1, 2, 0, 0, 2, 1};
    c.ep = {0, 1, 2, 10, 4, 5, 6, 11, 8, 9, 7, 3};
    c.eo = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1};
    return c;
}

CubieCube moveTables[6];
void init_move_tables() {
    moveTables[0] = moveU();
    moveTables[1] = moveR();
    moveTables[2] = moveF();
    moveTables[3] = moveD();
    moveTables[4] = moveL();
    moveTables[5] = moveB();
}

CubieCube apply_move(const CubieCube &cube, int move) {
    int base = move % 6;
    int count = 1;
    if(move >= 6 && move < 12) count = 3;
    else if(move >= 12) count = 2;
    
    CubieCube result = cube;
    for(int i=0; i<count; i++) {
        result = result * moveTables[base];
    }
    return result;
}

// ============================================================================
// ENCODING FUNCTIONS
// ============================================================================

const int factorial[13] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800, 479001600};

int encode_permutation_8(const array<uint8_t,8> &perm) {
    int index = 0;
    for(int i=0; i<8; i++) {
        int smaller = 0;
        for(int j=i+1; j<8; j++) {
            if(perm[j] < perm[i]) smaller++;
        }
        index += smaller * factorial[7-i];
    }
    return index;
}

int encode_orientation_corners(const array<uint8_t,8> &ori) {
    int index = 0;
    for(int i=0; i<7; i++) {
        index = index * 3 + ori[i];
    }
    return index;
}

// Edge subset encoding: choose 6 edges from 12, track their permutation and orientation
// For edges_a: positions {0,1,2,3,4,5} (UR,UF,UL,UB,DR,DF)
// For edges_b: positions {6,7,8,9,10,11} (DL,DB,FR,FL,BL,BR)

int encode_edge_subset(const array<uint8_t,12> &ep, const array<uint8_t,12> &eo, 
                       const array<int,6> &subset) {
    // Extract permutation and orientation for subset
    array<uint8_t,6> perm;
    array<uint8_t,6> ori;
    
    for(int i=0; i<6; i++) {
        perm[i] = ep[subset[i]];
        ori[i] = eo[subset[i]];
    }
    
    // Normalize permutation (map to 0..5)
    array<uint8_t,6> normalized;
    for(int i=0; i<6; i++) {
        int rank = 0;
        for(int j=0; j<6; j++) {
            if(perm[j] < perm[i]) rank++;
        }
        normalized[i] = rank;
    }
    
    // Encode permutation
    int perm_idx = 0;
    for(int i=0; i<6; i++) {
        int smaller = 0;
        for(int j=i+1; j<6; j++) {
            if(normalized[j] < normalized[i]) smaller++;
        }
        perm_idx += smaller * factorial[5-i];
    }
    
    // Encode orientation (first 5 bits)
    int ori_idx = 0;
    for(int i=0; i<5; i++) {
        ori_idx = ori_idx * 2 + ori[i];
    }
    
    return perm_idx * 32 + ori_idx;
}

// ============================================================================
// PDB GENERATION
// ============================================================================

void generate_corner_pdb(const string &filename) {
    cout << "Generating Corner PDB...\n";
    
    const int CORNER_STATES = 88179840;  // 8! * 3^7
    vector<uint8_t> pdb(CORNER_STATES, 255);
    
    CubieCube solved;
    int solved_idx = encode_permutation_8(solved.cp) * 2187 + encode_orientation_corners(solved.co);
    pdb[solved_idx] = 0;
    
    queue<pair<CubieCube, int>> q;
    q.push({solved, 0});
    
    long long processed = 0;
    int max_depth = 0;
    
    while(!q.empty()) {
        auto [state, depth] = q.front();
        q.pop();
        
        processed++;
        if(processed % 1000000 == 0) {
            cout << "Processed: " << processed << " / " << CORNER_STATES 
                 << " (" << (100.0 * processed / CORNER_STATES) << "%)" 
                 << " Depth: " << max_depth << "\n";
        }
        
        for(int m=0; m<18; m++) {
            CubieCube next = apply_move(state, m);
            int idx = encode_permutation_8(next.cp) * 2187 + encode_orientation_corners(next.co);
            
            if(pdb[idx] == 255) {
                pdb[idx] = depth + 1;
                max_depth = max(max_depth, depth + 1);
                q.push({next, depth + 1});
            }
        }
    }
    
    cout << "Corner PDB complete! Max depth: " << max_depth << "\n";
    cout << "Saving to " << filename << "...\n";
    
    ofstream out(filename, ios::binary);
    out.write(reinterpret_cast<const char*>(pdb.data()), pdb.size());
    out.close();
    
    cout << "[OK] Corner PDB saved (" << (pdb.size() / 1024.0 / 1024.0) << " MB)\n";
}

void generate_edge_pdb(const string &filename, const string &subset_name) {
    cout << "Generating Edge PDB (" << subset_name << ")...\n";
    
    array<int,6> subset;
    if(subset_name == "edges_a") {
        subset = {0, 1, 2, 3, 4, 5};  // UR,UF,UL,UB,DR,DF
    } else {
        subset = {6, 7, 8, 9, 10, 11}; // DL,DB,FR,FL,BL,BR
    }
    
    const int EDGE_STATES = 23040;  // 6! * 2^5
    vector<uint8_t> pdb(EDGE_STATES, 255);
    
    CubieCube solved;
    int solved_idx = encode_edge_subset(solved.ep, solved.eo, subset);
    pdb[solved_idx] = 0;
    
    queue<pair<CubieCube, int>> q;
    q.push({solved, 0});
    
    long long processed = 0;
    int max_depth = 0;
    
    while(!q.empty()) {
        auto [state, depth] = q.front();
        q.pop();
        
        processed++;
        
        for(int m=0; m<18; m++) {
            CubieCube next = apply_move(state, m);
            int idx = encode_edge_subset(next.ep, next.eo, subset);
            
            if(pdb[idx] == 255) {
                pdb[idx] = depth + 1;
                max_depth = max(max_depth, depth + 1);
                q.push({next, depth + 1});
            }
        }
    }
    
    cout << "Edge PDB complete! Max depth: " << max_depth << "\n";
    cout << "Saving to " << filename << "...\n";
    
    ofstream out(filename, ios::binary);
    out.write(reinterpret_cast<const char*>(pdb.data()), pdb.size());
    out.close();
    
    cout << "[OK] Edge PDB saved (" << (pdb.size() / 1024.0) << " KB)\n";
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    if(argc < 3) {
        cout << "Pattern Database Generator\n";
        cout << "Usage:\n";
        cout << "  " << argv[0] << " corners <output_file>\n";
        cout << "  " << argv[0] << " edges_a <output_file>\n";
        cout << "  " << argv[0] << " edges_b <output_file>\n";
        cout << "\nExample:\n";
        cout << "  " << argv[0] << " corners pdbs/corners.bin\n";
        cout << "  " << argv[0] << " edges_a pdbs/edges_a.bin\n";
        cout << "  " << argv[0] << " edges_b pdbs/edges_b.bin\n";
        return 1;
    }
    
    init_move_tables();
    
    string type = argv[1];
    string filename = argv[2];
    
    auto start = chrono::high_resolution_clock::now();
    
    if(type == "corners") {
        generate_corner_pdb(filename);
    } else if(type == "edges_a") {
        generate_edge_pdb(filename, "edges_a");
    } else if(type == "edges_b") {
        generate_edge_pdb(filename, "edges_b");
    } else {
        cout << "Unknown PDB type: " << type << "\n";
        cout << "Use: corners, edges_a, or edges_b\n";
        return 1;
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration<double>(end - start).count();
    
    cout << "\n[OK] Generation complete in " << duration << " seconds\n";
    
    return 0;
}
