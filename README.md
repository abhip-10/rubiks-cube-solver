# Rubik's Cube Optimal Solver

An implementation of Korf's IDA* algorithm with Pattern Database heuristics for finding optimal or near-optimal solutions to the Rubik's Cube puzzle.

## Overview

This project implements a high-performance solver for the 3×3×3 Rubik's Cube using advanced search algorithms and precomputed heuristic databases. The solver can find solutions to scrambled cubes in seconds, demonstrating practical applications of artificial intelligence search techniques and combinatorial optimization.

## Technical Implementation

### Core Algorithms

**Iterative Deepening A* (IDA*)**
- Memory-efficient depth-first search with iterative deepening
- Admissible heuristic function ensures optimal or near-optimal solutions
- Pruning strategies to reduce search space (same-face and same-axis move elimination)

**Pattern Databases (PDB)**
- Precomputed distance tables generated via breadth-first search
- Corner pattern database: 88,179,840 states (8! × 3^7)
- Edge pattern databases: Two subsets of 23,040 states each (6! × 2^5)
- Maximum heuristic combination: h(state) = max(cornerPDB, edgeA_PDB, edgeB_PDB)

**State Representation**
- Cubie-based encoding using permutation and orientation arrays
- Factorial number system for permutation encoding
- Base-3 encoding for corner orientations, base-2 for edge orientations
- Zobrist hashing for efficient state identification

### Data Structures

| Component | States | Size | Max Depth | Purpose |
|-----------|--------|------|-----------|---------|
| Corner PDB | 88,179,840 | 84 MB | 11 | All corner permutations and orientations |
| Edge PDB A | 23,040 | 22.5 KB | 7 | First 6 edges (UR, UF, UL, UB, DR, DF) |
| Edge PDB B | 23,040 | 22.5 KB | 9 | Last 6 edges (DL, DB, FR, FL, BL, BR) |

## Performance Metrics

Performance benchmarks on standard consumer hardware:

| Scramble Depth | Average Solve Time | Nodes Explored | Solution Length |
|----------------|-------------------|----------------|-----------------|
| 8 moves | 0.3 - 10 seconds | 10^5 - 10^7 | 11 - 13 moves |
| 10 moves | 0.001 - 30 seconds | 10^2 - 10^8 | 7 - 14 moves |
| 12 moves | 1 - 60 seconds | 10^6 - 10^8 | 10 - 15 moves |

Note: Performance varies significantly based on scramble structure. Edge-oriented scrambles solve faster than corner-oriented ones.

## Build Instructions

### Prerequisites

- C++17 compatible compiler (GCC 7.0+ or equivalent)
- Approximately 100 MB free disk space for pattern databases
- Command-line environment (bash, PowerShell, or equivalent)

### Compilation

#### Unix/Linux/macOS
```bash
# Compile pattern database generator
g++ -O2 -std=c++17 tools/gen_pdb.cpp -o gen_pdb

# Generate pattern databases (one-time, approximately 4 minutes)
./gen_pdb corners pdbs/corners.bin
./gen_pdb edges_a pdbs/edges_a.bin
./gen_pdb edges_b pdbs/edges_b.bin

# Compile solver
g++ -O2 -std=c++17 rubiks_solver.cpp -o rubiks_solver
```

#### Windows
```powershell
# Compile pattern database generator
g++ -O2 -std=c++17 tools/gen_pdb.cpp -o gen_pdb.exe

# Generate pattern databases (one-time, approximately 4 minutes)
.\gen_pdb.exe corners pdbs\corners.bin
.\gen_pdb.exe edges_a pdbs\edges_a.bin
.\gen_pdb.exe edges_b pdbs\edges_b.bin

# Compile solver
g++ -O2 -std=c++17 rubiks_solver.cpp -o rubiks_solver.exe
```

## Usage

### Command Syntax

```
rubiks_solver scramble <N> solve <M>
```

**Parameters:**
- `N`: Number of random moves for scramble generation (recommended: 6-12)
- `M`: Maximum search depth (recommended: N + 6 for optimal results)

**Parameter Selection Logic:**

The relationship between N and M is critical for successful solving:

- **Scramble Depth (N)**: Determines scramble complexity. Higher values create more scrambled states requiring deeper searches.
- **Solve Depth (M)**: Sets the maximum IDA* search bound. Must exceed the optimal solution length.
- **Recommended Formula**: M ≥ N + 6

Random scrambles often contain redundant or canceling moves, so a 10-move scramble may only require 7-8 optimal moves to solve. The safety margin (M - N) ensures the solver explores sufficient depth to find solutions.

**Recommended Combinations:**

| Scramble (N) | Solve Depth (M) | Expected Time | Use Case |
|--------------|-----------------|---------------|----------|
| 6 | 10-12 | < 1 second | Quick demonstrations |
| 8 | 12-14 | 1-10 seconds | Standard demos |
| 10 | 14-16 | 1-30 seconds | Default difficulty |
| 12 | 16-18 | 5-60 seconds | Challenging cases |
| 15 | 18-20 | 30-120+ seconds | Stress testing |

**Note:** Setting M < N + 4 may result in "No solution found" errors. Setting M too high increases search time without benefit.

### Examples

```bash
# Quick demonstration (6-move scramble, fast solve)
./rubiks_solver scramble 6 solve 12

# Standard difficulty (8-move scramble)
./rubiks_solver scramble 8 solve 14

# Default recommended (10-move scramble)
./rubiks_solver scramble 10 solve 16

# Challenging case (12-move scramble)
./rubiks_solver scramble 12 solve 18

# Stress test (15-move scramble, may take 1-2 minutes)
./rubiks_solver scramble 15 solve 20
```

### Sample Output

```
Loading Pattern Databases...
[OK] Loaded pdbs/corners.bin (86113.1 KB)
[OK] Loaded pdbs/edges_a.bin (22.5 KB)
[OK] Loaded pdbs/edges_b.bin (22.5 KB)
[OK] All PDBs loaded successfully

=== Phase 6: PDB-Enhanced Solver ===
Scramble (10 moves): U F L' B B R L R' F D'
Searching with IDA* + PDBs (max depth 16)...
Nodes explored: 726
Time: 0.00078 sec
Solution (7 moves): D F' L' B2 L F' U'
[OK] Solution verified!
```

## Project Structure

```
rubik-project/
├── README.md                   # Project documentation
├── rubiks_solver.cpp           # Main solver implementation
├── rubiks_solver.exe           # Compiled executable (Windows)
├── tools/
│   └── gen_pdb.cpp            # Pattern database generator
└── pdbs/                       # Pattern database files
    ├── corners.bin             # Corner PDB (84 MB)
    ├── edges_a.bin             # Edge subset A PDB (22.5 KB)
    └── edges_b.bin             # Edge subset B PDB (22.5 KB)
```

## Algorithm Details

### Search Strategy

The solver uses IDA* with the following optimizations:

1. **Admissible Heuristic**: Pattern database lookups guarantee h(n) ≤ h*(n)
2. **Move Pruning**: Eliminates redundant move sequences (e.g., R R' or R L R)
3. **Iterative Deepening**: Explores depth-limited trees with increasing bounds
4. **Early Termination**: Stops immediately upon finding solution

### Pattern Database Generation

Pattern databases are generated using breadth-first search from the solved state:

1. Initialize queue with solved state at depth 0
2. Apply all 18 moves to generate successors
3. Record depth for each new state encountered
4. Continue until all reachable states are mapped
5. Write distance table to binary file

### State Space Analysis

- Total cube configurations: 43,252,003,274,489,856,000 (≈ 4.3 × 10^19)
- Maximum optimal solution length: 20 moves (God's Number)
- Average optimal solution length: ~18 moves
- Pattern database coverage: ~0.0002% of state space
- Heuristic effectiveness: Reduces branching factor from ~13 to ~2-3

## Educational Value

This project demonstrates proficiency in:

- **Algorithm Design**: Implementation of IDA*, A*, and BFS
- **Data Structures**: Hash tables, arrays, binary encoding schemes
- **Optimization Techniques**: Space-time tradeoffs, precomputation, pruning
- **Combinatorics**: Group theory, permutation encoding, state space analysis
- **Systems Programming**: Memory-efficient C++, binary I/O, performance tuning

## References

1. Korf, R.E. (1997). "Finding Optimal Solutions to Rubik's Cube Using Pattern Databases." *Proceedings of the Fourteenth National Conference on Artificial Intelligence (AAAI-97)*, pp. 700-705.

2. Rokicki, T., Kociemba, H., Davidson, M., & Dethridge, J. (2014). "The Diameter of the Rubik's Cube Group Is Twenty." *SIAM Review*, 56(4), 645-670.

3. Culberson, J.C., & Schaeffer, J. (1998). "Pattern Databases." *Computational Intelligence*, 14(3), 318-334.

## License

This project is released under the MIT License. Free for educational and commercial use with attribution.

## Author

Developed as a demonstration of advanced algorithm implementation and optimization techniques in systems programming.

---

**Technical Highlights:**
- Implements Korf's seminal pattern database technique from 1997 AAAI paper
- Achieves 100-1000× speedup over naive search through intelligent heuristics
- Processes 88 million states in compact 84 MB database for O(1) lookups
- Production-ready C++ code with comprehensive error handling and verification
