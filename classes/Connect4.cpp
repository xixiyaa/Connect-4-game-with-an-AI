#include "Connect4.h"
#include "imgui/imgui.h"
#include <cstring>
#include <sstream>

Connect4::Connect4() { resetBoard(); }
Connect4::~Connect4() { stopGame(); }

// ---- Game (pure virtual) ----
void Connect4::setUpBoard() {
    // Ensure two players exist in the Game base so checkForWinner() can return them.
    setNumberOfPlayers(2);
    // Start a fresh Connect 4 game in 2P mode; AI can be toggled in the right panel.
    startGame(false, 2);
}

void Connect4::drawFrame() {
    // Two columns: left = board, right = status (matches the app's UI pattern)
    ImGui::Columns(2, nullptr, true);
    drawLeftPanel();
    ImGui::NextColumn();
    drawRightPanel();
    ImGui::Columns(1);
}

// ---- Lifecycle ----
void Connect4::startGame(bool vAI, int aiPlays)
{
    vsAI      = vAI;
    aiSide    = (aiPlays == 1) ? 1 : 2;
    running   = true;
    gameOver  = false;
    winner    = 0;
    currentPlayer = 1;
    movesMade = 0;
    hoverColumn = -1;
    anim = {};
    resetBoard();

    // Keep Game's turn history coherent (it won’t be used by our UI, but it’s safe).
    Game::startGame();
}

void Connect4::stopGame()
{
    running = false;
    gameOver = false;
    winner = 0;
    anim = {};
}

// ---- Per-frame update ----
void Connect4::update(float dt)
{
    if (!running) return;

    // Step piece drop animation if active
    if (anim.active) {
        stepDropAnim(dt);
        return; // while animating, we don't accept new input / AI
    }

    if (gameOver) return;

    // AI turn?
    if (vsAI && currentPlayer == aiSide) {
        int col = aiChooseMove();
        int placedRow = -1;
        if (col >= 0 && applyMove(col, currentPlayer, placedRow)) {
            if (animateDrops) launchDropAnim(col, placedRow, currentPlayer);
            concludeIfTerminal();
            if (!gameOver && !animateDrops) nextTurn();
        } else {
            // fallback: pick first legal move
            for (int c : legalMoves()) {
                int r = -1;
                if (applyMove(c, currentPlayer, r)) {
                    if (animateDrops) launchDropAnim(c, r, currentPlayer);
                    concludeIfTerminal();
                    if (!gameOver && !animateDrops) nextTurn();
                    break;
                }
            }
        }
        return;
    }
    // Human input handled in drawLeftPanel via clickable columns.
}

// ---- Drawing (left/right panes) ----
void Connect4::drawLeftPanel()
{
    ImGui::TextUnformatted("Connect 4");
    ImGui::Separator();

    ImGui::SliderFloat("Cell size", &cell, 40.f, 90.f, "%.0f px");
    ImGui::Checkbox("Animate drop", &animateDrops);

    // Board canvas
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGui::Dummy(ImVec2(cell*COLS + 16, cell*ROWS + 16)); // reserve space
    ImVec2 p = ImGui::GetItemRectMin();
    boardTopLeft = ImVec2(p.x + 8, p.y + 8);

    drawGridBackground(drawList, boardTopLeft, cell);
    drawBoard();
    drawHoverIndicator(drawList);

    // Column click handling
    if (!gameOver && (!vsAI || currentPlayer != aiSide) && !anim.active) {
        ImVec2 mouse = ImGui::GetMousePos();
        hoverColumn = -1;
        // detect hover
        for (int c = 0; c < COLS; ++c) {
            ImVec2 min(boardTopLeft.x + c*cell, boardTopLeft.y);
            ImVec2 max(min.x + cell, min.y + cell*ROWS);
            if (mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y) {
                hoverColumn = c;
                break;
            }
        }
        if (hoverColumn >= 0) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                int placedRow = -1;
                if (applyMove(hoverColumn, currentPlayer, placedRow)) {
                    if (animateDrops) launchDropAnim(hoverColumn, placedRow, currentPlayer);
                    concludeIfTerminal();
                    if (!gameOver && !animateDrops) nextTurn();
                }
            }
        }
    }
}

void Connect4::drawRightPanel()
{
    ImGui::TextUnformatted("Status");
    ImGui::Separator();

    if (!running) {
        ImGui::TextUnformatted("Game not started.");
        return;
    }

    if (gameOver) {
        if (winner == 1) { ImGui::TextColored(ImVec4(0.9f,0.2f,0.2f,1), "Red wins!"); }
        else if (winner == 2) { ImGui::TextColored(ImVec4(1.0f,0.85f,0.1f,1), "Yellow wins!"); }
        else { ImGui::TextUnformatted("Draw!"); }
    } else {
        if (currentPlayer == 1) ImGui::TextColored(ImVec4(0.9f,0.2f,0.2f,1), "Turn: Red");
        else                     ImGui::TextColored(ImVec4(1.0f,0.85f,0.1f,1), "Turn: Yellow");
        if (vsAI) {
            ImGui::Text("AI plays as: %s", aiSide==1? "Red":"Yellow");
        } else {
            ImGui::TextUnformatted("Mode: 2 Players");
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Reset")) {
        startGame(vsAI, aiSide);
    }

    if (ImGui::BeginCombo("Mode", vsAI ? "vs AI" : "2 Players")) {
        bool selected2 = !vsAI;
        bool selectedA = vsAI;
        if (ImGui::Selectable("2 Players", selected2)) {
            vsAI = false; startGame(vsAI, aiSide);
        }
        if (ImGui::Selectable("vs AI", selectedA)) {
            vsAI = true; startGame(vsAI, aiSide);
        }
        ImGui::EndCombo();
    }
    if (vsAI) {
        int tmp = aiSide;
        ImGui::RadioButton("AI is Red (Player 1)", &tmp, 1); ImGui::SameLine();
        ImGui::RadioButton("AI is Yellow (Player 2)", &tmp, 2);
        if (tmp != aiSide) { aiSide = tmp; startGame(vsAI, aiSide); }
    }
}

// ===== helpers =====
void Connect4::resetBoard()
{
    for (auto& col : board) col.fill(0);
}

bool Connect4::canPlay(int col) const
{
    if (col < 0 || col >= COLS) return false;
    return board[col][ROWS-1] == 0; // top cell empty
}

std::vector<int> Connect4::legalMoves() const
{
    std::vector<int> m;
    m.reserve(COLS);
    for (int c = 0; c < COLS; ++c) if (canPlay(c)) m.push_back(c);
    return m;
}

bool Connect4::applyMove(int col, int player, int& outRow)
{
    if (!canPlay(col)) return false;
    for (int r = 0; r < ROWS; ++r) {
        if (board[col][r] == 0) {
            board[col][r] = player;
            outRow = r;
            movesMade++;
            return true;
        }
    }
    return false;
}

int Connect4::countDir(int c, int r, int dc, int dr, int player) const
{
    int cnt = 0; int cc = c; int rr = r;
    while (cc >= 0 && cc < COLS && rr >= 0 && rr < ROWS && board[cc][rr] == player) {
        cnt++; cc += dc; rr += dr;
    }
    return cnt;
}

bool Connect4::checkWinAt(int col, int row, int player) const
{
    auto line = [&](int dc, int dr) {
        int total = 1; // placed disk
        int cc = col - dc, rr = row - dr;
        while (cc>=0 && cc<COLS && rr>=0 && rr<ROWS && board[cc][rr]==player) { total++; cc-=dc; rr-=dr; }
        cc = col + dc; rr = row + dr;
        while (cc>=0 && cc<COLS && rr>=0 && rr<ROWS && board[cc][rr]==player) { total++; cc+=dc; rr+=dr; }
        return total >= 4;
    };
    return line(1,0) || line(0,1) || line(1,1) || line(1,-1);
}

bool Connect4::checkAnyWin(int& outWinner) const
{
    for (int c=0;c<COLS;++c) for (int r=0;r<ROWS;++r) {
        int p = board[c][r];
        if (!p) continue;
        static const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
        for (auto& d : dirs) {
            int need = 3;
            int cc = c + d[0], rr = r + d[1];
            while (need && cc>=0 && cc<COLS && rr>=0 && rr<ROWS && board[cc][rr]==p) { need--; cc+=d[0]; rr+=d[1]; }
            if (need==0) { outWinner = p; return true; }
        }
    }
    return false;
}

void Connect4::drawGridBackground(ImDrawList* dl, const ImVec2& p, float size)
{
    const ImU32 bg = IM_COL32(25, 71, 140, 255); // board blue
    dl->AddRectFilled(p, ImVec2(p.x + size*COLS, p.y + size*ROWS), bg, 12.0f);

    // holes: draw white circles then overlay later with disks
    for (int c=0;c<COLS;++c) for (int r=0;r<ROWS;++r) {
        ImVec2 center(p.x + (c+0.5f)*size, p.y + (ROWS-1-r + 0.5f)*size);
        dl->AddCircleFilled(center, size*0.38f, IM_COL32(240,240,240,255), 32);
    }
}

void Connect4::drawDisk(ImDrawList* dl, const ImVec2& center, float radius, int color)
{
    ImU32 fill = (color==1) ? IM_COL32(220,60,60,255) : IM_COL32(240,220,60,255);
    ImU32 border = IM_COL32(0,0,0,80);
    dl->AddCircleFilled(center, radius, fill, 48);
    dl->AddCircle(center, radius, border, 48, 2.0f);
}

void Connect4::drawHoverIndicator(ImDrawList* dl)
{
    if (hoverColumn < 0 || gameOver || (vsAI && currentPlayer==aiSide) || anim.active) return;
    if (!canPlay(hoverColumn)) return;
    ImVec2 center(boardTopLeft.x + (hoverColumn+0.5f)*cell, boardTopLeft.y - cell*0.5f);
    drawDisk(dl, center, cell*0.38f, currentPlayer);
}

void Connect4::launchDropAnim(int col, int row, int color)
{
    anim.active = true;
    anim.col = col;
    anim.targetRow = row;
    anim.y = -cell*0.5f; // start slightly above
    anim.vy = 0.0f;
    anim.color = color;
}

void Connect4::stepDropAnim(float dt)
{
    float g = 2200.0f; // px/s^2
    anim.vy += g * dt;
    anim.y  += anim.vy * dt;

    float targetY = (ROWS-1-anim.targetRow + 0.5f) * cell;
    if (anim.y >= targetY) {
        anim.active = false;
        anim.y = targetY;

        concludeIfTerminal();
        if (!gameOver) nextTurn();
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 center(boardTopLeft.x + (anim.col+0.5f)*cell, boardTopLeft.y + anim.y);
    drawDisk(dl, center, cell*0.38f, anim.color);
}

void Connect4::drawBoard()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int c=0;c<COLS;++c) for (int r=0;r<ROWS;++r) {
        int p = board[c][r];
        if (!p) continue;
        ImVec2 center(boardTopLeft.x + (c+0.5f)*cell, boardTopLeft.y + (ROWS-1-r + 0.5f)*cell);
        if (anim.active && anim.col==c && anim.targetRow==r) continue; // don't double-draw animating piece
        drawDisk(dl, center, cell*0.38f, p);
    }
}

void Connect4::nextTurn()
{
    currentPlayer = (currentPlayer == 1) ? 2 : 1;
}

void Connect4::concludeIfTerminal()
{
    int w=0;
    if (checkAnyWin(w)) {
        winner = w;
        gameOver = true;
        running = true;
    } else if (isDraw()) {
        winner = 0;
        gameOver = true;
        running = true;
    }
}

// ===== AI =====

static inline int randFallback() { return 3; } // deterministic fallback center

int Connect4::aiChooseMove()
{
    const int maxDepth = 6;

    Board b = board;
    int me = aiSide;
    int bestCol = -1;
    int bestScore = std::numeric_limits<int>::min();

    auto moves = legalMoves();
    if (moves.empty()) return -1;

    // Prefer center in ties
    std::sort(moves.begin(), moves.end(), [](int a, int b){
        int ca = std::abs(3 - a), cb = std::abs(3 - b);
        return ca < cb;
    });

    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();
    for (int col : moves) {
        Board nb = b;
        if (!simApply(nb, col, me)) continue;
        int score = minimax(nb, maxDepth-1, alpha, beta, false, me);
        if (score > bestScore) { bestScore = score; bestCol = col; }
        alpha = std::max(alpha, score);
        if (beta <= alpha) break;
    }

    if (bestCol < 0) {
        for (int c : {3,2,4,1,5,0,6}) if (canPlay(c)) return c;
        return randFallback();
    }
    return bestCol;
}

int Connect4::minimax(Board b, int depth, int alpha, int beta, bool maximizing, int me)
{
    int w=0;
    for (int c=0;c<COLS;++c) for (int r=0;r<ROWS;++r) {
        int p = b[c][r]; if (!p) continue;
        if (simWinAt(b,c,r,p)) { w = p; goto doneScan; }
    }
doneScan:
    if (w)   return (w == me) ? 100000 : -100000;

    bool hasMove = false;
    for (int c=0;c<COLS;++c) if (simCanPlay(b,c)) { hasMove = true; break; }
    if (!hasMove) return 0;
    if (depth == 0) return evaluateBoard(b, me);

    if (maximizing) {
        int best = std::numeric_limits<int>::min();
        auto moves = simMoves(b);
        std::sort(moves.begin(), moves.end(), [](int a, int b){
            int ca = std::abs(3-a), cb = std::abs(3-b);
            return ca < cb;
        });
        for (int col : moves) {
            Board nb = b;
            simApply(nb, col, me);
            int val = minimax(nb, depth-1, alpha, beta, false, me);
            best = std::max(best, val);
            alpha = std::max(alpha, val);
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int opp = (me==1)?2:1;
        int best = std::numeric_limits<int>::max();
        auto moves = simMoves(b);
        std::sort(moves.begin(), moves.end(), [](int a, int b){
            int ca = std::abs(3-a), cb = std::abs(3-b);
            return ca < cb;
        });
        for (int col : moves) {
            Board nb = b;
            simApply(nb, col, opp);
            int val = minimax(nb, depth-1, alpha, beta, true, me);
            best = std::min(best, val);
            beta = std::min(beta, val);
            if (beta <= alpha) break;
        }
        return best;
    }
}

int Connect4::evaluateBoard(const Board& b, int me) const
{
    int opp = (me==1)?2:1;
    auto scoreFor = [&](int who)->int{
        int s = 0;

        // center preference
        int centerCount = 0;
        for (int r=0;r<ROWS;++r) if (b[3][r]==who) centerCount++;
        s += centerCount * 6;

        auto scoreLine = [&](int c0,int r0,int dc,int dr){
            std::array<int,4> w{};
            int sLocal=0;
            for (;;) {
                int c=c0, r=r0;
                for (int i=0;i<4;++i) {
                    if (c<0||c>=COLS||r<0||r>=ROWS) return sLocal;
                    w[i] = b[c][r]; c+=dc; r+=dr;
                }
                sLocal += scoreWindow(w, who);
                c0+=dc; r0+=dr;
            }
        };

        // Horizontal rows
        for (int r=0;r<ROWS;++r) s += scoreLine(0,r,1,0);
        // Vertical cols
        for (int c=0;c<COLS;++c) s += scoreLine(c,0,0,1);
        // Diagonals /
        for (int c=0;c<=COLS-4;++c) s += scoreLine(c,ROWS-4,1,1);
        for (int r=ROWS-4;r>=0;--r) s += scoreLine(0,r,1,1);
        // Diagonals \
        for (int c=0;c<=COLS-4;++c) s += scoreLine(c,3,1,-1);
        for (int r=3;r<ROWS;++r)     s += scoreLine(0,r,1,-1);

        return s;
    };

    int myS  = scoreFor(me);
    int opS  = scoreFor(opp);
    return myS - opS;
}

int Connect4::scoreWindow(const std::array<int,4>& w, int me) const
{
    int opp = (me==1)?2:1;
    int meCnt=0, oppCnt=0, empty=0;
    for (int v : w) { if (v==me) meCnt++; else if (v==opp) oppCnt++; else empty++; }

    if (meCnt==4) return 10000;
    if (meCnt==3 && empty==1) return 100;
    if (meCnt==2 && empty==2) return 12;

    if (oppCnt==3 && empty==1) return -120; // block threats more urgently
    if (oppCnt==4) return -10000;

    return 0;
}

// === static board helpers for AI ===
bool Connect4::simCanPlay(const Board& b, int col)
{
    if (col<0 || col>=COLS) return false;
    return b[col][ROWS-1]==0;
}

bool Connect4::simApply(Board& b, int col, int player)
{
    if (!simCanPlay(b,col)) return false;
    for (int r=0;r<ROWS;++r) if (b[col][r]==0) { b[col][r]=player; return true; }
    return false;
}

std::vector<int> Connect4::simMoves(const Board& b)
{
    std::vector<int> m;
    for (int c=0;c<COLS;++c) if (simCanPlay(b,c)) m.push_back(c);
    return m;
}

int Connect4::simCountDir(const Board& b, int c, int r, int dc, int dr, int player)
{
    int cnt=0;
    while (c>=0&&c<COLS&&r>=0&&r<ROWS&&b[c][r]==player){cnt++; c+=dc; r+=dr;}
    return cnt;
}

bool Connect4::simWinAt(const Board& b, int col, int row, int player)
{
    auto line = [&](int dc,int dr){
        int total=1;
        int cc=col-dc, rr=row-dr; while (cc>=0&&cc<COLS&&rr>=0&&rr<ROWS&&b[cc][rr]==player){total++; cc-=dc; rr-=dr;}
        cc=col+dc; rr=row+dr;     while (cc>=0&&cc<COLS&&rr>=0&&rr<ROWS&&b[cc][rr]==player){total++; cc+=dc; rr+=dr;}
        return total>=4;
    };
    return line(1,0)||line(0,1)||line(1,1)||line(1,-1);
}

// ---- Game-required overrides (non-UI) ----
bool Connect4::canBitMoveFrom(Bit &, BitHolder &) { return false; }
bool Connect4::canBitMoveFromTo(Bit &, BitHolder &, BitHolder &) { return false; }

Player* Connect4::checkForWinner()
{
    if (winner == 1) return getPlayerAt(0);
    if (winner == 2) return getPlayerAt(1);
    return nullptr;
}

bool Connect4::checkForDraw()
{
    return (gameOver && winner == 0) || isDraw();
}

// Serialize as: C4;<curP>;<42 cells left-to-right, bottom-to-top>
// e.g. C4;1;0000000... (42 digits)
std::string Connect4::initialStateString() { return "C4;1;000000000000000000000000000000000000000000"; }

std::string Connect4::stateString()
{
    std::string s = "C4;";
    s += (currentPlayer==1?'1':'2');
    s += ';';
    s.reserve(3 + 42);
    for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) s.push_back(char('0'+board[c][r]));
    return s;
}

void Connect4::setStateString(const std::string &s)
{
    // very forgiving parse; on failure just reset to initial state
    int cp = 1;
    Board b{};
    bool ok = false;
    do {
        if (s.size() < 3) break;
        if (!(s[0]=='C' && s[1]=='4' && s[2]==';')) break;
        size_t p1 = s.find(';', 3);
        if (p1 == std::string::npos) break;
        if (p1+1 >= s.size()) break;
        char ccp = s[3];
        cp = (ccp=='2') ? 2 : 1;
        size_t p2 = s.find(';', 2); // not used
        std::string cells = s.substr(p1+1);
        if (cells.size() < COLS*ROWS) break;
        // fill b from string (same order as stateString)
        for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) {
            char ch = cells[r*COLS + c];
            int v = (ch>='0' && ch<='2') ? (ch-'0') : 0;
            b[c][r] = v;
        }
        ok = true;
    } while(false);

    if (!ok) { resetBoard(); currentPlayer = 1; return; }

    board = b;
    currentPlayer = cp;
    // recompute movesMade and winner
    movesMade = 0;
    for (int c=0;c<COLS;++c) for (int r=0;r<ROWS;++r) if (board[c][r]) movesMade++;
    int w=0; winner=0; gameOver=false;
    if (checkAnyWin(w)) { winner=w; gameOver=true; }
}

Grid* Connect4::getGrid()
{
    // Not used for Connect 4 (we draw with ImGui directly).
    // Returning nullptr is safe because Application never calls Game::drawFrame() for Connect4.
    return nullptr;
}
