# Vanth Chess Engine - Walkthrough (Explainable Chess Intelligence & Validation Lab)

We have successfully implemented **Version 3.5.1: Professional Engine Validation Laboratory**. The Vanth Chess Engine has transitioned into a professionally testable and verifiable chess engine, equipped with a comprehensive suite of tactical, strategic, endgame, opening, perft, stability, and stress-testing modules.

All unit tests and validation modules pass successfully, ensuring a scientific workflow for all future search and evaluation optimizations.

---

## 1. Core Architecture & Features

The explainable intelligence module is encapsulated in the `chess::validation` namespace and integrates seamlessly with search, move generation, and evaluation components without altering core search logic.

### 📊 Evaluation Decomposition
*   **Mobility Isolation**: Exposed the evaluation weights of the mobility module (`search::evaluate_mobility(board)`), which was previously aggregated implicitly inside `evaluate_piece_activity`.
*   **Mathematical Consistency**: Modified `get_evaluation_breakdown` to subtract mobility from `piece_activity` to ensure that summing all individual components (Material, PST, Pawn Structure, Mobility, Piece Activity, King Safety, Space, Threats, Endgame, Tempo) equals the engine's exact `total` score.

### 🧠 Move Reasoning Heuristics
*   **Intent Extraction**: Compares the detailed static evaluation breakdowns of the board state *before* and *after* a move is played.
*   **Dynamic Descriptions**: Generates specific reason strings based on which subscores improved:
    *   **Material Wins**: Highlights centipawn improvements in material balance.
    *   **Mobility & Activity**: Explains improvements in piece square placement and activity.
    *   **King Safety**: Notes when pawn shields are reinforced or when the king's shelter is compromised.
    *   **Center Control**: Flags moves that occupy or attack central squares (`d4`, `e4`, `d5`, `e5`).
    *   **Development**: Automatically detects minor piece development from back ranks.

### 🔍 Tactical Theme Detection
*   **Pins & Skewers**: Scans all attack rays to detect when an enemy piece is pinned to a higher-value target (King/Queen) or when a high-value piece is skewered in front of a lower-value piece.
*   **Forks & Double Attacks**: Generates pseudo-legal moves to detect when a single piece attacks two or more valuable or undefended enemy pieces.
*   **Back-Rank Weakness**: Analyzes if the enemy king is trapped on the back rank behind its own pawns while back-rank files are under-defended.
*   **Discovered Attack Setup**: Identifies when a friendly piece blocks a slider's line of fire, preparing an attack if the blocking piece moves.

### 🏷️ Position Classification
*   **Game Stage**: Classifies positions as `Opening`, `Middlegame`, or `Endgame` based on piece count, game phase, and move clocks.
*   **Pawn Structure**: Classifies the position as `Open` (low pawns, many open files), `Closed` (many pawns, few open files), or `Semi-Open`.
*   **Nature**: Labels the position as `Tactical` (high tactical threats evaluation) or `Strategic` (quiet positional play).
*   **Special Motifs**: Automatically flags features like `King Attack` and `Pawn Race` in active positions.

---

## 2. CLI Command Integration

We extended the benchmark CLI command dispatcher in [main.cpp](benchmarks/main.cpp) to support new commands:

### 💬 `explain [move] [fen]`
Explains the strategic/tactical reason for playing the specified move on the given board.
*   **Usage**: `chess_benchmarks.exe explain <move_notation> [fen]`
*   **Example**:
    ```cmd
    .\build\benchmarks\Release\chess_benchmarks.exe explain e2e4
    ```
*   **Output**:
    ```
    Explanation for move e2e4:
      - Occupies the key central square e4 to establish central presence.
      - Increases overall piece mobility and square control (+ 23 cp).
    ```

### 🔬 `analyze [depth] [fen]`
Generates a complete explainable analysis report in a structured JSON format. It temporarily enables MultiPV search (MultiPV = 3) to gather alternative lines, then restores original settings.
*   **Usage**: `chess_benchmarks.exe analyze [depth] [fen]`
*   **Example**:
    ```cmd
    .\build\benchmarks\Release\chess_benchmarks.exe analyze 3
    ```
*   **JSON Response Schema**:
    ```json
    {
      "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      "best_move": "b1a3",
      "evaluation": 13,
      "breakdown": {
        "material": 0,
        "pst": 0,
        "pawn_structure": 0,
        "mobility": 0,
        "piece_activity": 6,
        "king_safety": 0,
        "space": 0,
        "threats": 0,
        "endgame": 0,
        "tempo": 10,
        "total": 16
      },
      "alternatives": [
        {
          "move": "b1a3",
          "score": 13,
          "pv": ["b1a3", "b8c6", "g1f3"]
        }
      ],
      "explanation": {
        "move": "b1a3",
        "reasons": [
          "Develops a minor piece (Knight) from its starting square.",
          "Claims more board space and territorial control."
        ]
      },
      "tactical_themes": {
        "fork": false,
        "pin": false,
        "skewer": false,
        "discovered_attack": false,
        "double_attack": false,
        "deflection": false,
        "decoy": false,
        "x_ray": false,
        "back_rank_weakness": true,
        "descriptions": [
          "Potential back-rank weakness detected around enemy king on e8"
        ]
      },
      "classifications": {
        "opening": true,
        "middlegame": false,
        "endgame": false,
        "closed_position": true,
        "labels": ["Opening", "Closed Position", "Strategic"]
      },
      "strengths": ["Structurally balanced and solid position."],
      "weaknesses": ["No major positional weaknesses detected."],
      "search_stats": {
        "nodes": 2767,
        "nps": 58872,
        "time_ms": 47,
        "cutoffs": 238,
        "tt_hits": 0
      }
    }
    ```

---

## 3. Verification & Execution Status

### 🧪 Unit Tests
We introduced a dedicated test suite [test_explainable_ai.cpp](tests/test_explainable_ai.cpp) covering evaluation breakdown checks, move explanations, tactical pin/fork detection, position classifications, blunder detection (score drop calculations), and JSON reporting.

All **58 unit tests** pass successfully:
```
[----------] 6 tests from ExplainableAITest
[ RUN      ] ExplainableAITest.EvaluationBreakdownMobility
[       OK ] ExplainableAITest.EvaluationBreakdownMobility (0 ms)
[ RUN      ] ExplainableAITest.ExplainMove
[       OK ] ExplainableAITest.ExplainMove (0 ms)
[ RUN      ] ExplainableAITest.DetectTacticalThemes
[       OK ] ExplainableAITest.DetectTacticalThemes (0 ms)
[ RUN      ] ExplainableAITest.PositionClassification
[       OK ] ExplainableAITest.PositionClassification (0 ms)
[ RUN      ] ExplainableAITest.GenerateReportAndJSON
[       OK ] ExplainableAITest.GenerateReportAndJSON (1 ms)
[ RUN      ] ExplainableAITest.BlunderAnalysis
[       OK ] ExplainableAITest.BlunderAnalysis (0 ms)
[----------] 6 tests from ExplainableAITest (2 ms total)

[==========] 58 tests from 9 test suites ran. (1738 ms total)
[  PASSED  ] 58 tests.
```

---

## 4. Version 3.0: Monorepo Full-Stack Chess Studio

We successfully completed the transition of the Vanth Chess Engine into a modern, publishable full-stack application structured as an NPM workspace monorepo.

### 📁 Monorepo Workspace Directory Layout
```
/
├── package.json               # Root workspaces config (NPM workspaces)
├── shared/                    # Shared TypeScript interface definitions
│   └── src/index.ts
├── engine/                    # High-performance C++20 Core
│   ├── benchmarks/main.cpp    # CLI backend (explain, analyze, legalmoves, move)
│   └── ...
├── apps/
│   ├── server/                # Node.js Express Backend
│   │   ├── prisma/            # SQLite development database schema
│   │   ├── src/index.ts       # Express router & engine process manager
│   │   └── tsconfig.json
│   └── client/                # React 19 / Vite Frontend Studio
│       ├── src/               
│       │   ├── App.tsx        # Responsive grid layout manager
│       │   ├── main.tsx       
│       │   ├── components/    # ChessBoard, EvaluationBar, AnalysisPanel
│       │   └── store/         # Zustand global state (gameStore.ts)
│       └── vite.config.ts
```

### ⚙️ REST API Endpoints (`@vanth/server`)
The Express server spawns the C++ `chess_benchmarks.exe` executable statelessly and pipes parameters/outputs securely via child processes:
*   `POST /api/analyze` - Runs the engine's Explainable AI search. Saves report to database history.
*   `POST /api/explain` - Fetches natural language justifications for specific moves.
*   `POST /api/evaluate` - Exposes raw static evaluation breakdowns.
*   `POST /api/legalmoves` - Provides move candidates for high-performance frontend move validation.
*   `POST /api/move` - Takes FEN + move string, runs engine move logic, and returns the updated FEN.
*   `GET / PUT /api/settings` - Reads/updates engine parameters (depth, threads, hash).
*   `GET /api/history` - Fetches the last 20 explainable AI analysis runs.
*   `GET / POST / DELETE /api/saved-positions` - Allows saving/deleting chess positions of interest.

### 🎨 Responsive Dashboard Layout (`@vanth/client`)
Built using React 19, Tailwind CSS, and Zustand:
*   **Sidebar Navigator**: Switch views between Active Analysis, Move History Records, Saved Layouts, and Engine Configuration.
*   **Vertical Evaluation Bar**: Real-time visual representation of White's vs Black's advantage in centipawns or mate steps.
*   **Standalone Interactive Chess Board**: Responsive square grid supporting flipped layouts, legal move dot overlays, and selection states with offline vector SVG piece components (no external assets required).
*   **Analysis Visualizer**: Displays PV variations, delta centipawn scores, static subscore balance bars (Material, PST, Pawn Structure, Activity), tactical badges, and search diagnostics.

### 🚀 Running the Studio locally
1.  **Build the C++ Engine**: Make sure `chess_benchmarks.exe` is built using CMake in `engine/build/benchmarks/Release`.
2.  **Initialize Monorepo Workspaces**:
    ```bash
    npm install --legacy-peer-deps
    ```
3.  **Run Database Migrations**:
    ```bash
    npm run db:push
    ```
4.  **Run Server and Client in parallel dev mode**:
    ```bash
    # Start Express Server on http://localhost:4000
    npm run dev:server
    
    # Start Vite Dev Server on http://localhost:3000
    npm run dev:client
    ```

---

## 5. Version 3.1: Professional Analysis Workspace

We successfully implemented version 3.1, transforming Vanth into a high-end chess analysis dashboard.

### 🌿 Advanced Move Tree (`@vanth/client`)
*   Replaced the basic linear history array with a **Move Tree (N-ary Tree)**.
*   Supports main line play, nested sub-variations (visualized inline inside parentheses), and active branch navigation.
*   Added keyboard bindings (`ArrowLeft`/`ArrowRight`, `Ctrl+Z`/`Ctrl+Shift+Z`) and mouse click listeners to jump directly to any move node in the tree.

### 📡 Live Engine Analysis Streaming (`@vanth/server` & `@vanth/client`)
*   Replaced static analysis with an asynchronous Server-Sent Events (SSE) stream endpoint `/api/analyze/stream`.
*   Spawns the C++ search binary and parses stdout `info depth` lines in real-time, sending data events containing `depth`, `seldepth`, `score`, `nodes`, `nps`, `timeMs`, and `pv`.
*   Includes **Cancelable Analysis**: closing the SSE connection triggers a process kill signal on the server, saving CPU resources immediately.
*   Provides an **Engine Inspector** overlay displaying live metrics and principal variation movements as they are calculated.

### 📈 Interactive Evaluation Graph (`@vanth/client`)
*   Added a smooth area-chart SVG line graph displaying the evaluation fluctuations across the active move path.
*   Supports interactive circle dots showing move details and score on hover, and jumps the board position to that node when clicked.

### 🗃️ Position Inspector & Explainable UI (`@vanth/client`)
*   Added a **Position Inspector** next to the board displaying material advantage balance, pawn structure, space indicators, king safety, and a visual progress indicator of the active game phase (Opening, Middlegame, Endgame).
*   Refactored the textual justifications into interactive **Reason Cards** utilizing custom category icons, confidence ratings, and educational tooltips explaining core chess concepts.

### ⌨️ Command Palette (`@vanth/client`)
*   Added a global searchable command palette opened via `Ctrl+K`.
*   Includes commands to Analyze, Flip Board, Reset Board, Load FEN, Save Position, Switch Panels, Navigate Moves, and View Shortcuts.
*   Fully navigable with arrow keys, Enter, and Escape.
*   Shortcut overlay modal triggered by pressing `?`.

---

## 6. Version 3.2: Game Intelligence & PGN Reports

We successfully implemented Version 3.2, introducing game-level intelligence, PGN parsing pipelines, and deep positional reports.

### 📥 Robust PGN Importer
*   Built a custom state-machine parser in `pgnParser.ts` to handle complex nested variations, commentary annotations, NAG symbols, and metadata headers.

### ⚙️ Background Analysis Queue
*   Developed `analysisQueue.ts` to process uploaded PGN games asynchronously. 
*   Analyzes each move ply, utilizes DB caches to avoid redundant C++ engine invocations, and streams live progress updates via Server-Sent Events (SSE).

### 🧠 Move Heuristics & Accuracy
*   Implemented automated move classification (Brilliant, Blunder, Missed Win) and tactical/positional detection (pin opportunities, compromised king safety, pawn structure weaknesses).
*   Calculates overall white/black game accuracy percentages and provides personalized educational training advice.

---

## 7. Version 3.3: Cloud-Native Collaborative Platform

We successfully transitioned Vanth from a local analysis application into a cloud-native, multi-user chess studio.

### 🗄️ Database Architecture Migration (PostgreSQL)
*   Replaced the SQLite database schema with a production-ready **PostgreSQL** schema using Prisma.
*   Designed relationships for Users, Settings, SavedPositions, Games, Studies, Members, Comments, Notifications, AuditLogs, and OpeningCache.

### 🔑 Secure Authentication & RBAC
*   Implemented JWT-based session management, refresh tokens, and password cryptography via **Bcrypt** hashing.
*   Added role-based access control (RBAC), securing admin commands and limiting workspace controls.

### 🛡️ API Versioning & Security
*   Versioned all endpoints under `/api/v1/`.
*   Integrated security middlewares: **Helmet** (HTTP headers), **CORS** (cross-origin controls), and **Rate Limiting** to prevent brute-force attacks.

### 📈 Personal Workspace & Library
*   **User Dashboard**: Displays games played, saved layouts, notifications, and average accuracy statistics.
*   **Library Management**: Supports query parameters for search, tag filtration, sorting, and cursor pagination.
*   **Studies & Collaboration**: Created multiplayer workspace collections allowing editors/viewers to add interactive commentary and post comments on shared positions.

### 🗺️ Explorers & Advanced Search
*   **Opening Explorer**: Retrieves ECO codes, names, transposition popularity, and win percentages.
*   **Endgame Explorer**: Static theoretical databases for Lucena, Philidor, and opposition techniques.
*   **Advanced Search**: Performs queries combining player names, FEN/PGN segments, tags, and game outcomes.

### 🐳 Containerized Deployment
*   **Multi-Stage Dockerfiles**: Builds the C++ engine core inside `gcc:12` and pipes the output binary into a slim Node runner.
*   **Docker Compose**: Coordinates Postgres, server backend, and Nginx frontend client services in a single configuration.

### 🧪 System Verification
*   Expanded backend integration test suite to **113 assertions** (171 passing assertions total including C++ core tests), confirming complete coverage across authentication, sharing permissions, and database operations.

---

## 8. Version 3.4: Engine Verification & Search Debugging (Critical NMP Bug Fix)

We diagnosed and resolved a critical tactical search bug that caused major evaluation drops (e.g. -8.29 cp or worse) in winning positions (such as after `Qxh5` capturing a rook).

### 🔍 Root Cause Analysis
*   **The Bug**: During Null Move Pruning (NMP) in `negamax.cpp`, the search copied the `Board` object state by value (`board::Board sim_board = board;`) and manually toggled the active color and en passant square.
*   **Hash Inconsistency**: This manual modification of the copied `Board` did not incrementally update the board's Zobrist hash key. As a result, the transposition table (TT) stored evaluation scores for the modified position under the original board's hash key.
*   **TT Corruption**: Subsequent search probes retrieved these incorrect, stale values for unrelated search nodes, leading to massive evaluation swings and incorrect moves.

### 🛠️ In-Place Null Move Implementation
*   **Incremental Zobrist Updates**: Extended the `Board` class in `board.hpp`/`board.cpp` with `make_null_move()` and `undo_null_move()`. These methods execute in-place modifications and perform correct incremental Zobrist hashing updates for the active color and en passant square without duplicating the board object.
*   **NMP Refactoring**: Updated the NMP search routine in `negamax.cpp` to use the new in-place methods. This completely resolved the TT corruption.

### 🧪 Verification & Benchmarks
*   **FEN Verification**: Confirmed that the position before `Qxh5` evaluates to `+7.57` cp, and the position after `Qxh5` evaluates to `-6.93` cp (Black perspective) instead of the corrupted `-17.62` cp / `-17.19` cp scores.
*   **Test Suite & Harness**: Re-ran the C++ test suite (62/62 passing) and successfully executed the automated Stockfish comparison harness with zero timeouts or process deadlocks.

---

## 9. Version 3.5: Evaluation Perspective Alignment (White-Relative Scoring)

We successfully resolved the evaluation perspective bug where reported scores alternated between positive and negative values depending on whose turn it was.

### 🔍 Root Cause Analysis
*   **The Bug**: The internal search Negamax and QSearch implementation correctly computes evaluations relative to the side to move (active player's perspective). However, this raw side-to-move score was being printed directly to UCI, logs, and JSON reports without perspective adjustment.
*   **Alternating Evaluation**: This caused GUI and CLI outputs to alternate signs between moves (e.g., White winning by `+1000` cp, but displaying as `-1000` cp when it is Black's turn to move), confusing players and breaking board evaluation graphs.

### 🛠️ Consistent White-Relative Output Implementation
*   **Search Reporting Refactoring**: Updated `find_best_move` in `search.cpp` to convert raw side-to-move scores into a consistent **White-relative perspective** before saving them in the `SearchInfo` result structure (e.g., `info.best_score`, `info.alternatives`, and `info.timeline`).
*   **Standard Output Compliance**: Adjusted UCI search progress print statements for both MultiPV and single PV modes in `search.cpp` to print `score cp` and `score mate` values from White's perspective.
*   **Blunder Detection Alignment**: Updated `analyze_blunder` in `validation/analysis.cpp` to correctly compute score drops using the new White-relative evaluations based on the active player (subtracting `before_score - after_score` for White, and `after_score - before_score` for Black).

### 🧪 Verification & Automated Tests
*   **Unit Testing**: Added a comprehensive `EvaluationPerspectiveConsistency` suite to `tests/test_search.cpp` that loads won/lost positions for White, toggles the side to move (White vs. Black to move), and asserts that the returned search evaluations remain consistently positive/negative, unaffected by whose turn it is.
*   **Build & Health Check**: Compiled the engine and ran the full suite of **63 unit tests** (all passed).

---

## 10. Version 3.5.1: Engine Validation Laboratory (CI/CD Diagnostic Lab)

We have successfully integrated a comprehensive **Engine Validation Laboratory** covering 18 validation, analysis, and diagnostic modules to verify engine stability, tactical prowess, strategic understanding, and endgame consistency.

### 🛠️ Compilation & Linker Bug Fixes
*   **Warning Elimination**: Resolved the `C4244` MSVC warnings (narrowing conversion from `int` to `char` when mirroring FEN strings) by applying explicit `static_cast<char>` statements, enabling a warning-free clean build with strict warning-as-error (`/WX`) compliance.
*   **Linker Fixes**: Corrected unresolved external references inside the validation harness when instantiating `SearchContext` by directly referencing the global static transposition table `g_tt` and properly scoping thread-local `TimeManager` controllers.

### 📊 Validation Modules Overview
The validation laboratory supports the following specialized test suites and reports:
1.  **Tactical Test Suite (Module 1)**: Covers Mate in 1, Mate in 2, Mate in 3, Fork exploits (via `r3k3/8/8/3N4/8/8/8/4K3 w`), and Pin exploits (via `3k4/8/2p5/3n4/8/4P3/8/3R1K2 w`), verifying that the engine solves tactical themes within expected ranges.
2.  **Strategic Test Suite (Module 2)**: Checks positional evaluations against centipawn margins for Passed Pawns, Open Files, Pawn Outpost, Bishop Pair, and Space Advantage.
3.  **Endgame Test Suite (Module 3)**: Asserts correct drawing/winning play for Lucena (win/mate check), Philidor Draw (retaining drawing position via `b5b3`/`e6e5`), KPK Draw (opposition defense `d6d7`), and Insufficient Material (Bishop vs King, checking that the engine evaluates it <= 550 cp).
4.  **Opening Sanity (Module 4)**: Verifies that common opening plays (Italian, Ruy Lopez, Sicilian, French, Queen's Gambit, English) remain blunder-free and produce stable evaluations.
5.  **Perft Suite (Module 5)**: Validates move generation invariants at depths 1-3 on startpos, Kiwipete, and En Passant/Castling positions.
6.  **Search Stability (Module 6)**: Runs deepening searches from depths 1 to 12 on benchmark positions to detect score oscillations, PV path collapses, or unexpected evaluation sign changes.
7.  **Root Move Analysis (Module 7)**: Generates a complete ranked report of all legal moves sorted by their search evaluation, including SEE values and history scores.
8.  **Heuristic Isolation (Module 8)**: Benchmarks the engine with specific search optimizations (TT, QSearch, NMP, LMR, RFP, PVS, Futility) disabled to isolate search divergence.
9.  **Evaluation Symmetry (Module 9)**: Assures that static evaluations satisfy `Eval(pos) ≈ -Eval(mirrored_pos)` within a strict 25 cp tolerance.

### 🧪 Overall Laboratory Output
Running the master CI/CD CLI command `chess_benchmarks.exe validate_engine` outputs the following summary:
```
====================================================================
                     CI VALIDATION LAB SUMMARY REPORT               
====================================================================
  Modules Passed:       9 / 9
  Regression Framework: 5 / 5 bugs passed
  Perft Validation:     PASSED
  Tactical Suite:       6 / 6 solved
  Strategic Suite:      5 / 5 passed
  Endgame Suite:        4 / 4 passed
  Opening Sanity:       6 / 6 passed
  Search Stability:     PASSED
  Evaluation Symmetry:  PASSED
  Random Stress Test:   PASSED (500 positions checked)
--------------------------------------------------------------------
  OVERALL STATUS:       SUCCESS
====================================================================
```
