#pragma once
#include "Game.h"
#include "imgui/imgui.h"
#include <array>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <cmath>

// Connect 4 board: 7 columns x 6 rows (columns [0..6], rows [0..5])
// We draw with ImGui primitives; no Grid/Bits are used for gameplay.

class Connect4 : public Game {
public:
    Connect4();
    ~Connect4();

    // ---- Application integration ----
    void setUpBoard() override;   // required by Game (pure virtual)
    void drawFrame() override;    // we do our own left/right layout

    // Per-frame update (animations + AI turn handling)
    void update(float dt);

    // Helpers used by Application.cpp status panel
    const char* getName() const { return "Connect 4"; }
    int  getCurrentPlayerNumber() const { return currentPlayer; }
    int  getWinnerNumber()       const { return winner; }
    bool isDrawn()               const { return isDraw(); }
    bool isRunning()             const { return running; }

    // Lifecycle
    void startGame(bool vsAI, int aiPlaysAs);
    void stopGame() override;

    // ---- Required overrides from Game.h (make class non-abstract) ----
    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    Player* checkForWinner() override;
    bool checkForDraw() override;
    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;
    Grid* getGrid() override;

private:
    // Board state: 0 = empty, 1 = red, 2 = yellow
    static constexpr int COLS = 7;
    static constexpr int ROWS = 6;
    using Board = std::array<std::array<int, ROWS>, COLS>;

    Board board{};
    bool  running = false;
    bool  vsAI = false;
    int   aiSide = 2;          // 1 = AI is Red, 2 = AI is Yellow
    int   currentPlayer = 1;   // 1 = Red, 2 = Yellow
    bool  gameOver = false;
    int   winner = 0;          // 0=none/draw, 1=red, 2=yellow
    int   movesMade = 0;

    // UI layout / input
    float cell = 64.0f;
    ImVec2 boardTopLeft{0,0};
    int   hoverColumn = -1;
    bool  animateDrops = true;

    // Drop animation (one at a time)
    struct DropAnim {
        bool  active = false;
        int   col = -1;
        int   targetRow = -1;
        float y = 0.0f;
        float vy = 0.0f;
        int   color = 0; // 1 or 2
    } anim;

    // --- Helpers ---
    void resetBoard();
    bool applyMove(int col, int player, int& outRow);
    bool canPlay(int col) const;
    std::vector<int> legalMoves() const;

    bool checkWinAt(int col, int row, int player) const;
    bool checkAnyWin(int& outWinner) const;
    bool isDraw() const { return movesMade >= COLS * ROWS && !winner; }

    // 🔧 add back the declaration to match Connect4.cpp
    int countDir(int c, int r, int dc, int dr, int player) const;

    // drawing
    void drawLeftPanel();
    void drawRightPanel();
    void drawBoard();
    void drawGridBackground(ImDrawList* drawList, const ImVec2& p, float size);
    void drawDisk(ImDrawList* drawList, const ImVec2& center, float radius, int color);
    void drawHoverIndicator(ImDrawList* drawList);
    void launchDropAnim(int col, int row, int color);
    void stepDropAnim(float dt);

    // turn / flow
    void nextTurn();
    void concludeIfTerminal();

    // --- AI (minimax with alpha-beta, depth-limited) ---
    int aiChooseMove();
    int evaluateBoard(const Board& b, int me) const;
    int scoreWindow(const std::array<int,4>& w, int me) const;
    int minimax(Board b, int depth, int alpha, int beta, bool maximizing, int me);

    // board sims for AI
    static bool simApply(Board& b, int col, int player);
    static bool simCanPlay(const Board& b, int col);
    static std::vector<int> simMoves(const Board& b);
    static bool simWinAt(const Board& b, int col, int row, int player);
    static int  simCountDir(const Board& b, int c, int r, int dc, int dr, int player);
};
