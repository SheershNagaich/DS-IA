/ MemoryGame.cpp
// MEMOY!! - Neon Terminal Edition (Arcade Intro vFinal)
// Left-aligned classic header box, blinking "Press ENTER to start..." (infinite),
// larger gameplay, 2 difficulties (2x2, 4x4), boot animation.
// Compile: clang++ -std=c++17 MemoryGame.cpp -o MemoryGame
// Run: ./MemoryGame

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>

using namespace std;

// ---------------- Colors & styling ----------------
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string CYAN = "\033[1;36m";
const string MAGENTA = "\033[1;35m";
const string WHITE = "\033[1;37m";
const string GREEN = "\033[1;32m";
const string YELLOW = "\033[1;33m";
const string RED = "\033[1;31m";
const string BLUE = "\033[1;34m";
const string DIMGRAY = "\033[90m";

inline void msleep(int ms){ this_thread::sleep_for(chrono::milliseconds(ms)); }
inline void clearScreen(){ system("clear"); }

// ---------------- MEMOY header (left-aligned, classic) ----------------
// block letters (keeps same look as earlier left-aligned version)
const vector<string> MEMOY_BLOCK = {
 " ███╗ ███╗███████╗███╗ ███╗ ██████╗ ██╗ ██╗██╗██╗ ",
 " ████╗ ████║██╔════╝████╗ ████║██╔═══██╗╚██╗ ██╔╝██║██║ ",
 " ██╔████╔██║█████╗ ██╔████╔██║██║ ██║ ╚████╔╝ ██║██║ ",
 " ██║╚██╔╝██║██╔══╝ ██║╚██╔╝██║██║ ██║ ╚██╔╝ ╚═╝╚═╝ ",
 " ██║ ╚═╝ ██║███████╗██║ ╚═╝ ██║╚██████╔╝ ██║ ██╗██╗ ",
 " ╚═╝ ╚═╝╚══════╝╚═╝ ╚═╝ ╚═════╝ ╚═╝ ╚═╝╚═╝ "
};

// fixed header width (keeps same box width as you liked)
const int HEADER_WIDTH = 74;

void printHeaderBox() {
 string top = "+" + string(HEADER_WIDTH - 2, '=') + "+";
 string empty = "|" + string(HEADER_WIDTH - 2, ' ') + "|";
 string bottom = "+" + string(HEADER_WIDTH - 2, '=') + "+";

 cout << CYAN << top << RESET << "\n";
 cout << CYAN << empty << RESET << "\n";

 for (const auto &line : MEMOY_BLOCK) {
 int pad = max(0, (HEADER_WIDTH - 2 - (int)line.size()) / 2);
 cout << "|" << string(pad, ' ') << MAGENTA << line << RESET;
 int right = HEADER_WIDTH - 2 - pad - (int)line.size();
 if (right > 0) cout << string(right, ' ');
 cout << "|\n";
 }

 cout << CYAN << empty << RESET << "\n";
 string subtitle = "✨ A Memory Match Game by Sheersh & Radhesh ✨";
 int pad = max(0, (HEADER_WIDTH - 2 - (int)subtitle.size()) / 2);
 cout << "|" << string(pad, ' ') << CYAN << subtitle << RESET;
 int right = HEADER_WIDTH - 2 - pad - (int)subtitle.size();
 if (right > 0) cout << string(right, ' ');
 cout << "|\n";
 cout << CYAN << empty << RESET << "\n";
 cout << CYAN << bottom << RESET << "\n\n";
}

// ---------------- Game classes & helpers ----------------
struct HighScore { string name; int moves; int timeSec; int score; };

class Card {
public:
 string sym;
 bool flipped;
 bool matched;
 int color; // ANSI color number
 Card(): sym("#"), flipped(false), matched(false), color(37) {}
 Card(const string &s, int c): sym(s), flipped(false), matched(false), color(c) {}
};

class MemoryGame {
private:
 vector<Card> board;
 int gridSize = 4;
 int totalPairs = 8;
 int moves = 0;
 int matches = 0;
 int score = 0;
 bool timedMode = false;
 int maxTimeSec = 120;
 chrono::steady_clock::time_point startTime;
 string playerName = "Player";
 vector<HighScore> highs;
 vector<string> symbolPool = {
 "S","C","H","D","P","T","R","Q",
 "*","&","@","#","$","%","!","?","1","2","3","4",
 "A","B","E","F","G","J","K","L"
 };
 vector<int> colorPool = {36, 35, 33, 32, 34, 31}; // neon-ish order
 string hsFile = "memoy_highscores.txt";

 void loadHighs() {
 highs.clear();
 ifstream f(hsFile);
 if (!f.good()) return;
 HighScore h;
 while (f >> h.name >> h.moves >> h.timeSec >> h.score) highs.push_back(h);
 }
 void saveHighs() {
 ofstream f(hsFile);
 if (!f.good()) return;
 for (auto &h : highs) f << h.name << " " << h.moves << " " << h.timeSec << " " << h.score << "\n";
 }
 void pushHigh() {
 int t = (int)chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - startTime).count();
 highs.push_back({playerName, moves, t, score});
 sort(highs.begin(), highs.end(), [](const HighScore &a, const HighScore &b){ return a.score > b.score; });
 if (highs.size() > 10) highs.resize(10);
 saveHighs();
 }

 void prepareBoard() {
 board.clear();
 totalPairs = (gridSize * gridSize) / 2;
 vector<string> pick(symbolPool.begin(), symbolPool.begin() + totalPairs);
 vector<string> cards = pick;
 cards.insert(cards.end(), pick.begin(), pick.end());
 unsigned seed = (unsigned)chrono::system_clock::now().time_since_epoch().count();
 shuffle(cards.begin(), cards.end(), default_random_engine(seed));
 for (size_t i = 0; i < cards.size(); ++i) {
 board.emplace_back(cards[i], colorPool[i % colorPool.size()]);
 }
 }

 void calculateScore() {
 score = max(0, 1000 - moves * 10 + matches * 150);
 }

 // Render header + playable area. Show revealA/revealB during a move.
 // <-- THIS is the fixed render that aligns column numbers with tiles perfectly
 void render(int revealA = -1, int revealB = -1, const string &msg = "") {
 clearScreen();
 printHeaderBox(); // left-aligned classic header

 // larger, bolder stats for readability
 cout << BOLD << WHITE << "Player: " << CYAN << playerName << RESET;
 cout << " " << BOLD << WHITE << "Moves: " << YELLOW << moves << RESET;
 cout << " " << BOLD << WHITE << "Matches: " << GREEN << matches << "/" << totalPairs << RESET;
 cout << " " << BOLD << WHITE << "Score: " << MAGENTA << score << RESET << "\n\n";

 // FIXED COLUMN ALIGNMENT: use consistent cell width and header spacing
 const int cellWidth = 5; // width per cell (matches tile display)
 cout << BOLD << WHITE << " ";
 for (int c = 0; c < gridSize; ++c) cout << setw(cellWidth) << c;
 cout << RESET << "\n";

 cout << " " << string(gridSize * cellWidth + 1, '-') << "\n";

 for (int r = 0; r < gridSize; ++r) {
 cout << BOLD << WHITE << setw(2) << r << " |" << RESET;
 for (int c = 0; c < gridSize; ++c) {
 int idx = r * gridSize + c;
 cout << " ";
 if (board[idx].matched) {
 // matched: [X] fits into cellWidth
 cout << "\033[1;" << board[idx].color << "m[" << board[idx].sym << "]" << RESET;
 int extra = cellWidth - 3; if (extra > 0) cout << string(extra, ' ');
 } else if (board[idx].flipped || idx == revealA || idx == revealB) {
 // revealed: " X " fits into cellWidth
 cout << "\033[1;" << board[idx].color << "m " << board[idx].sym << " " << RESET;
 int extra = cellWidth - 3; if (extra > 0) cout << string(extra, ' ');
 } else {
 // hidden: "###" centered in the cell
 int leftPad = (cellWidth - 3) / 2;
 int rightPad = cellWidth - 3 - leftPad;
 cout << DIMGRAY << string(leftPad, ' ') << "###" << string(rightPad, ' ') << RESET;
 }
 }
 cout << "\n";
 }

 if (!msg.empty()) cout << "\n" << msg << RESET << "\n";
 cout << "\n";
 }

 bool validPick(int r, int c) {
 if (r < 0 || c < 0 || r >= gridSize || c >= gridSize) return false;
 int idx = r * gridSize + c;
 if (board[idx].matched) return false;
 return true;
 }

public:
 MemoryGame() { loadHighs(); }

 void setDifficulty(int opt) {
 if (opt == 1) gridSize = 2;
 else gridSize = 4;
 }

 void showHighs() {
 clearScreen();
 printHeaderBox();
 cout << BOLD << WHITE << "High Scores\n\n" << RESET;
 if (highs.empty()) {
 cout << YELLOW << "No high scores yet. Play to add your name!\n\n" << RESET;
 } else {
 cout << BOLD << setw(14) << "Player" << setw(8) << "Moves" << setw(8) << "Time" << setw(10) << "Score" << RESET << "\n";
 cout << string(45, '-') << "\n";
 for (auto &h : highs) {
 cout << CYAN << setw(14) << h.name << RESET
 << YELLOW << setw(8) << h.moves << RESET
 << GREEN << setw(8) << h.timeSec << RESET
 << MAGENTA << setw(10) << h.score << RESET << "\n";
 }
 cout << "\n";
 }
 cout << "Press Enter to return..."; cin.ignore(numeric_limits<streamsize>::max(), '\n'); cin.get();
 }

 // Play a game session (timedMode optional)
 void play(bool timed = false) {
 timedMode = timed;
 clearScreen();
 printHeaderBox();
 cout << BOLD << WHITE << "Enter your name: " << RESET;
 getline(cin, playerName);
 if (playerName.empty()) playerName = "Player";

 prepareBoard();
 moves = 0; matches = 0; score = 0;
 startTime = chrono::steady_clock::now();

 while (matches < totalPairs) {
 render();

 if (timedMode) {
 int elapsed = (int)chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - startTime).count();
 if (elapsed >= maxTimeSec) {
 render(-1,-1, RED + "⏰ Time's up! Game over." + RESET);
 msleep(900);
 break;
 }
 }

 // First pick
 int r1, c1;
 cout << BOLD << CYAN << "Enter first card (Row and Column): " << RESET;
 if (!(cin >> r1 >> c1)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); continue; }
 if (!validPick(r1,c1)) { cout << RED << "Invalid pick. Try again." << RESET << "\n"; msleep(700); continue; }
 int idx1 = r1 * gridSize + c1;
 if (board[idx1].flipped || board[idx1].matched) { cout << RED << "Already revealed. Choose another." << RESET << "\n"; msleep(600); continue; }
 board[idx1].flipped = true;

 render(idx1, -1);

 // Second pick
 int r2, c2;
 cout << BOLD << CYAN << "Enter second card (Row and Column): " << RESET;
 if (!(cin >> r2 >> c2)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); board[idx1].flipped = false; continue; }
 if (!validPick(r2,c2) || (r1 == r2 && c1 == c2)) { board[idx1].flipped = false; cout << RED << "Invalid second pick." << RESET << "\n"; msleep(700); continue; }
 int idx2 = r2 * gridSize + c2;
 board[idx2].flipped = true;

 moves++;
 render(idx1, idx2);

 // Wait so user sees whole move
 msleep(1000);

 if (board[idx1].sym == board[idx2].sym) {
 board[idx1].matched = board[idx2].matched = true;
 matches++;
 score += 100;
 calculateScore();
 render(-1,-1, GREEN + BOLD + "✨ MATCH FOUND! +100 ✨" + RESET);
 msleep(700);
 } else {
 render(-1,-1, RED + BOLD + "❌ Not a match." + RESET);
 msleep(700);
 board[idx1].flipped = board[idx2].flipped = false;
 }
 // loop continues
 }

 // End of game
 pushHigh();
 cin.ignore(numeric_limits<streamsize>::max(), '\n');
 render(-1,-1, GREEN + BOLD + "🎉 Finished! Press Enter to continue." + RESET);
 cin.get();
 }

 void menu() {
 while (true) {
 clearScreen();
 printHeaderBox();
 cout << BOLD << WHITE << "1) " << RESET << "New Game (choose difficulty)\n";
 cout << BOLD << WHITE << "2) " << RESET << "Timed Game (4x4, 2 minutes)\n";
 cout << BOLD << WHITE << "3) " << RESET << "High Scores\n";
 cout << BOLD << WHITE << "4) " << RESET << "Exit\n\n";
 cout << CYAN << "Enter choice: " << RESET;

 int ch;
 if (!(cin >> ch)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); continue; }
 cin.ignore(numeric_limits<streamsize>::max(), '\n');

 if (ch == 1) {
 cout << BOLD << WHITE << "\nChoose Difficulty:\n" << RESET;
 cout << " 1) Easy (2 x 2)\n";
 cout << " 2) Hard (4 x 4)\n";
 cout << CYAN << "Enter choice: " << RESET;
 int d;
 if (!(cin >> d)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); continue; }
 cin.ignore(numeric_limits<streamsize>::max(), '\n');
 setDifficulty(d == 1 ? 1 : 2);
 play(false);
 } else if (ch == 2) {
 setDifficulty(2); // timed uses 4x4
 play(true);
 } else if (ch == 3) {
 showHighs();
 } else if (ch == 4) {
 break;
 }
 }
 }
};

// ---------------- Boot animation (brief) ----------------
void bootAnimation() {
 clearScreen();
 string msg = "Launching MEMOY";
 cout << MAGENTA << BOLD;
 for (char ch : msg) { cout << ch << flush; msleep(35); }
 cout << RESET;
 for (int i = 0; i < 3; ++i) { cout << "." << flush; msleep(250); }
 cout << "\n";
 msleep(250);
}

// ---------------- Infinite blinking title wait (press ENTER to start) ----------------
// Uses select() to wait with timeout so the text can blink until Enter is pressed.
void waitForEnterBlink() {
 while (true) {
 // Show header + prompt visible
 clearScreen();
 printHeaderBox();
 cout << CYAN << BOLD << " Press ENTER to start..." << RESET << "\n\n";
 // wait with timeout for input (500ms)
 fd_set rfds;
 FD_ZERO(&rfds);
 FD_SET(STDIN_FILENO, &rfds);
 timeval tv; tv.tv_sec = 0; tv.tv_usec = 500000; // 500ms
 int ready = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
 if (ready > 0) {
 // input available: read a line (Enter) and continue
 string tmp;
 getline(cin, tmp);
 break;
 }

 // Show header + prompt hidden (blink off)
 clearScreen();
 printHeaderBox();
 // print nothing here (empty line to keep spacing)
 cout << "\n\n";
 // wait again
 FD_ZERO(&rfds);
 FD_SET(STDIN_FILENO, &rfds);
 tv.tv_sec = 0; tv.tv_usec = 500000;
 ready = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
 if (ready > 0) {
 string tmp;
 getline(cin, tmp);
 break;
 }
 // loop repeats: visible -> hidden -> visible...
 }
}

// ---------------- main ----------------
int main() {
 ios::sync_with_stdio(false);
 cin.tie(nullptr);

 bootAnimation();
 waitForEnterBlink();

 MemoryGame game;
 game.menu();

 clearScreen();
 printHeaderBox();
 cout << GREEN << BOLD << "Thanks for playing MEMOY!! See you next time!" << RESET << "\n\n";
 return 0;
}