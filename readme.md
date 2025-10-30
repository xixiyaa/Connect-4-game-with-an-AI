# Connect 4 — README
## 1 What it is
A new game (Connect 4) added to the existing ImGui board-game project that already had TicTacToe, Checkers, and Othello.
### Files
classes/Connect4.h
classes/Connect4.cpp
(Application.cpp updated)
(CMakeLists.txt updated)
### How it integrates
- Application.cpp adds a button: "Start Connect 4"
- On click:
    ::Connect4* c4 = new ::Connect4();
    game = c4;
    c4->setUpBoard();
- Each frame:
    c4->update(dt);
    c4->drawFrame();
###Board + turns
- 7 columns × 6 rows
- board[col][row] = 0/1/2
- 1 = Red, 2 = Yellow
- Click column → drop to lowest empty → check win/draw → switch player
### Modes
- 2 Players
- vs AI
- AI can be Player 1 or Player 2
### Win / draw
- Check 4 in a row (horizontal / vertical / 2 diagonals)
- If full (42 moves) and no winner → draw
### AI
- Minimax (depth-limited) with alpha-beta
- Prefers center columns
- Called automatically on AI turn in update()
### Animation
- Optional drop animation for the piece
- While animating → ignore input and AI

### 🧱 Step-by-Step Build Commands

In **PowerShell or CMD**, run the following from your project root folder:

```bash
# Step 1: Generate Visual Studio project files
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Step 2: Build in Release mode
cmake --build build --config Release

✅ After building, your executable will be located at:

build/Release/demo.exe