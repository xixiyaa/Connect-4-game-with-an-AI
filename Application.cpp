#include "Application.h"
#include "imgui/imgui.h"
#include "classes/TicTacToe.h"
#include "classes/Checkers.h"
#include "classes/Othello.h"
// NEW:
#include "classes/Connect4.h"

namespace ClassGame {
        //
        // our global variables
        //
        Game *game = nullptr;
        bool gameOver = false;
        int gameWinner = -1;

        //
        // game starting point
        // this is called by the main render loop in main.cpp
        //
        void GameStartUp() 
        {
            game = nullptr;
        }

        //
        // game render loop
        // this is called by the main render loop in main.cpp
        //
        void RenderGame() 
        {
                ImGui::DockSpaceOverViewport();

                //ImGui::ShowDemoWindow();

                ImGui::Begin("Settings");

                if (gameOver) {
                    ImGui::Text("Game Over!");
                    if (gameWinner > 0) {
                        ImGui::Text("Winner: %d", gameWinner);
                    } else {
                        ImGui::TextUnformatted("Result: Draw");
                    }

                    if (ImGui::Button("Reset Game")) {
                        // Special-case Connect 4 so we don't depend on base AI plumbing
                        if (auto c4 = dynamic_cast<::Connect4*>(game)) {
                            c4->stopGame();
                            c4->setUpBoard();
                        } else if (game) {
                            game->stopGame();
                            game->setUpBoard();
                        }
                        gameOver = false;
                        gameWinner = -1;
                    }
                }

                if (!game) {
                    if (ImGui::Button("Start Tic-Tac-Toe")) {
                        game = new TicTacToe();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Checkers")) {
                        game = new Checkers();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Othello")) {
                        game = new Othello();
                        game->setUpBoard();
                    }
                    // NEW: Connect 4
                    if (ImGui::Button("Start Connect 4")) {
                        ::Connect4* c4 = new ::Connect4();
                        game = c4;
                        c4->setUpBoard();
                    }
                } else {
                    // Status line (works for legacy games; Connect4 has its own getter)
                    if (auto c4 = dynamic_cast<::Connect4*>(game)) {
                        ImGui::Text("Current Player Number: %d", c4->getCurrentPlayerNumber());
                        // (Board state string is game-specific; Connect4 shows rich status on the right panel)
                    } else {
                        ImGui::Text("Current Player Number: %d", game->getCurrentPlayer()->playerNumber());
                        ImGui::Text("Current Board State: %s", game->stateString().c_str());
                    }
                }
                ImGui::End();

                ImGui::Begin("GameWindow");
                if (game) {
                    if (auto c4 = dynamic_cast<::Connect4*>(game)) {
                        // Step animations/AI internally for Connect 4
                        c4->update(ImGui::GetIO().DeltaTime);

                        // Mirror Connect4 terminal state to the existing Settings panel
                        if (!gameOver) {
                            int w = c4->getWinnerNumber();
                            if (w > 0) { gameOver = true; gameWinner = w; }
                            else if (c4->isDrawn()) { gameOver = true; gameWinner = -1; }
                        }

                        // Draw board (left) + status (right) in one frame
                        c4->drawFrame();
                    } else {
                        // Original behavior for TicTacToe / Checkers / Othello
                        if (game->gameHasAI() && (game->getCurrentPlayer()->isAIPlayer() || game->_gameOptions.AIvsAI))
                        {
                            game->updateAI();
                        }
                        game->drawFrame();
                    }
                }
                ImGui::End();
        }

        //
        // end turn is called by the game code at the end of each turn
        // this is where we check for a winner
        //
        void EndOfTurn() 
        {
            // Legacy games call this hook; Connect4 handles terminal state internally.
            Player *winner = game->checkForWinner();
            if (winner)
            {
                gameOver = true;
                gameWinner = winner->playerNumber();
            }
            if (game->checkForDraw()) {
                gameOver = true;
                gameWinner = -1;
            }
        }
}
