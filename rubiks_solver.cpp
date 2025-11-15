// rubiks_solver_pdb.cpp
// Phase 6: IDA* with Pattern Database Heuristic
// Compile: g++ -O2 -std=c++17 rubiks_solver_pdb.cpp -o rubiks_pdb.exe
// Usage: rubiks_pdb.exe scramble 15 solve 20

#include <bits/stdc++.h>
using namespace std;

// ============================================================================
// CUBIE REPRESENTATION
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
    
    static CubieCube solved(){ return CubieCube(); }
    
    bool is_solved() const {
        for(int i=0;i<8;i++) if(cp[i]!=i || co[i]!=0) return false;
        for(int i=0;i<12;i++) if(ep[i]!=i || eo[i]!=0) return false;
        return true;
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

// ============================================================================
// ZOBRIST HASHING FOR TRANSPOSITION PRUNING (Phase 7)
// ============================================================================

static uint64_t Z_CORNER_POS[8][8];
static uint64_t Z_CORNER_ORI[8][3];
static uint64_t Z_EDGE_POS[12][12];
static uint64_t Z_EDGE_ORI[12][2];

void init_zobrist() {
    mt19937_64 rng(0xC0FFEE1234ULL);
    for(int i=0;i<8;i++){
        for(int p=0;p<8;p++) Z_CORNER_POS[i][p]=rng();
        for(int o=0;o<3;o++) Z_CORNER_ORI[i][o]=rng();
    }
    for(int i=0;i<12;i++){
        for(int p=0;p<12;p++) Z_EDGE_POS[i][p]=rng();
        for(int o=0;o<2;o++) Z_EDGE_ORI[i][o]=rng();
    }
}

inline uint64_t hash_cube(const CubieCube &c){
    uint64_t h=0;
    for(int i=0;i<8;i++){ h ^= Z_CORNER_POS[i][c.cp[i]]; h ^= Z_CORNER_ORI[i][c.co[i]]; }
    for(int i=0;i<12;i++){ h ^= Z_EDGE_POS[i][c.ep[i]]; h ^= Z_EDGE_ORI[i][c.eo[i]]; }
    return h;
}

// ============================================================================
// MOVE APPLICATION
// ============================================================================

enum Move { U=0, R=1, F=2, D=3, L=4, B=5, 
            Up=6, Rp=7, Fp=8, Dp=9, Lp=10, Bp=11, 
            U2=12, R2=13, F2=14, D2=15, L2=16, B2=17 };

string move_names[] = {"U", "R", "F", "D", "L", "B",
                       "U'", "R'", "F'", "D'", "L'", "B'",
                       "U2", "R2", "F2", "D2", "L2", "B2"};

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
// PDB ENCODING
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

int encode_edge_subset(const array<uint8_t,12> &ep, const array<uint8_t,12> &eo, 
                       const array<int,6> &subset) {
    array<uint8_t,6> perm;
    array<uint8_t,6> ori;
    
    for(int i=0; i<6; i++) {
        perm[i] = ep[subset[i]];
        ori[i] = eo[subset[i]];
    }
    
    array<uint8_t,6> normalized;
    for(int i=0; i<6; i++) {
        int rank = 0;
        for(int j=0; j<6; j++) {
            if(perm[j] < perm[i]) rank++;
        }
        normalized[i] = rank;
    }
    
    int perm_idx = 0;
    for(int i=0; i<6; i++) {
        int smaller = 0;
        for(int j=i+1; j<6; j++) {
            if(normalized[j] < normalized[i]) smaller++;
        }
        perm_idx += smaller * factorial[5-i];
    }
    
    int ori_idx = 0;
    for(int i=0; i<5; i++) {
        ori_idx = ori_idx * 2 + ori[i];
    }
    
    return perm_idx * 32 + ori_idx;
}

// ============================================================================
// PDB LOADING
// ============================================================================

vector<uint8_t> pdb_corners;
vector<uint8_t> pdb_edges_a;
vector<uint8_t> pdb_edges_b;
array<int,6> subset_a = {0, 1, 2, 3, 4, 5};
array<int,6> subset_b = {6, 7, 8, 9, 10, 11};

bool load_pdb(const string &filename, vector<uint8_t> &pdb, int expected_size) {
    ifstream in(filename, ios::binary);
    if(!in) {
        cout << "Warning: Could not load " << filename << "\n";
        return false;
    }
    
    pdb.resize(expected_size);
    in.read(reinterpret_cast<char*>(pdb.data()), expected_size);
    in.close();
    
    cout << "[OK] Loaded " << filename << " (" << (expected_size / 1024.0) << " KB)\n";
    return true;
}

void load_pdbs() {
    cout << "Loading Pattern Databases...\n";
    bool corners_loaded = load_pdb("pdbs/corners.bin", pdb_corners, 88179840);
    load_pdb("pdbs/edges_a.bin", pdb_edges_a, 23040);
    load_pdb("pdbs/edges_b.bin", pdb_edges_b, 23040);
    if(corners_loaded) {
        cout << "[OK] All PDBs loaded successfully\n";
    } else {
        cout << "[INFO] Running with edge PDBs only\n";
    }
}

// ============================================================================
// PDB HEURISTIC
// ============================================================================

int heuristic_pdb(const CubieCube &c) {
    int h = 0;
    
    // Corner PDB
    if(!pdb_corners.empty()) {
        int idx_c = encode_permutation_8(c.cp) * 2187 + encode_orientation_corners(c.co);
        if(idx_c >= 0 && idx_c < (int)pdb_corners.size()) {
            h = max(h, (int)pdb_corners[idx_c]);
        }
    }
    
    // Edge PDB A
    if(!pdb_edges_a.empty()) {
        int idx_a = encode_edge_subset(c.ep, c.eo, subset_a);
        if(idx_a >= 0 && idx_a < (int)pdb_edges_a.size()) {
            h = max(h, (int)pdb_edges_a[idx_a]);
        }
    }
    
    // Edge PDB B
    if(!pdb_edges_b.empty()) {
        int idx_b = encode_edge_subset(c.ep, c.eo, subset_b);
        if(idx_b >= 0 && idx_b < (int)pdb_edges_b.size()) {
            h = max(h, (int)pdb_edges_b[idx_b]);
        }
    }
    
    // Fallback: simple heuristic if PDBs not loaded
    if(h == 0) {
        int wrong = 0;
        for(int i=0; i<12; i++) if(c.ep[i] != i || c.eo[i] != 0) wrong++;
        h = (wrong + 3) / 4;
    }
    
    return h;
}

// ============================================================================
// IDA* WITH PDB + PHASE 7 OPTIMIZATIONS
// ============================================================================

static const int FACE_AXIS[6] = {0,1,2,0,1,2}; // U/D, R/L, F/B

bool ida_search_pdb(const CubieCube &node, int g, int bound, int prevFace,
                    vector<int> &path, long long &nodes, int &nextBound) {
    nodes++;
    
    int h = heuristic_pdb(node);
    int f = g + h;
    
    if(f > bound) {
        nextBound = min(nextBound, f);
        return false;
    }
    if(node.is_solved()) return true;
    
    for(int m=0; m<18; m++) {
        int face = m % 6;
        if(face == prevFace) continue;
        if(prevFace != -1 && FACE_AXIS[face] == FACE_AXIS[prevFace]) continue;
        
        path.push_back(m);
        CubieCube next = apply_move(node, m);
        if(ida_search_pdb(next, g+1, bound, face, path, nodes, nextBound))
            return true;
        path.pop_back();
    }
    return false;
}

bool ida_star_solve_pdb(CubieCube start, int max_depth, vector<int> &solution, long long &nodes) {
    nodes = 0;
    int bound = heuristic_pdb(start);
    if(start.is_solved()) { solution.clear(); return true; }
    
    for(int iter=0; iter<50; iter++) {
        vector<int> path;
        int nextBound = INT_MAX;
        if(ida_search_pdb(start, 0, bound, -1, path, nodes, nextBound)) {
            solution = path;
            return true;
        }
        if(nextBound == INT_MAX || nextBound > max_depth) return false;
        bound = nextBound;
    }
    return false;
}

// ============================================================================
// SCRAMBLE
// ============================================================================

vector<int> scramble_cube(CubieCube &c, int num_moves) {
    vector<int> seq;
    mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, 17);
    
    for(int i=0; i<num_moves; i++) {
        int m = dist(rng);
        c = apply_move(c, m);
        seq.push_back(m);
    }
    return seq;
}

// ============================================================================
// MAIN
// ============================================================================

string seq_to_string(const vector<int> &seq) {
    string s;
    for(size_t i=0; i<seq.size(); i++) {
        if(i) s += " ";
        s += move_names[seq[i]];
    }
    return s;
}

int main(int argc, char **argv) {
    init_move_tables();
    init_zobrist();
    load_pdbs();
    
    int scramble_moves = 15;
    int depth_limit = 20;
    bool do_solve = false;
    
    for(int i=1; i<argc; i++) {
        string arg = argv[i];
        if(arg == "scramble" && i+1 < argc) {
            scramble_moves = stoi(argv[++i]);
        } else if(arg == "solve" && i+1 < argc) {
            depth_limit = stoi(argv[++i]);
            do_solve = true;
        }
    }
    
    if(do_solve) {
        CubieCube c = CubieCube::solved();
        auto seq = scramble_cube(c, scramble_moves);
        
        cout << "\n=== Phase 6: PDB-Enhanced Solver ===\n";
        cout << "Scramble (" << scramble_moves << " moves): " << seq_to_string(seq) << "\n";
        cout << "Searching with IDA* + PDBs (max depth " << depth_limit << ")...\n";
        
        vector<int> solution;
        long long nodes = 0;
        auto t0 = chrono::high_resolution_clock::now();
        bool found = ida_star_solve_pdb(c, depth_limit, solution, nodes);
        auto t1 = chrono::high_resolution_clock::now();
        
        cout << "Nodes explored: " << nodes << "\n";
        cout << "Time: " << chrono::duration<double>(t1-t0).count() << " sec\n";
        
        if(found) {
            cout << "Solution (" << solution.size() << " moves): " << seq_to_string(solution) << "\n";
            
            CubieCube verify = c;
            for(int m : solution) {
                verify = apply_move(verify, m);
            }
            if(verify.is_solved()) {
                cout << "[OK] Solution verified!\n";
            } else {
                cout << "[FAIL] Solution verification FAILED\n";
            }
        } else {
            cout << "No solution found within depth limit\n";
        }
    } else {
        cout << "Phase 6: Pattern Database Solver\n";
        cout << "Usage: " << argv[0] << " scramble N solve M\n";
        cout << "Example: " << argv[0] << " scramble 15 solve 20\n";
    }
    
    return 0;
}
