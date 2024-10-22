#include <iostream>
#include <thread>
#include <mutex>
#include <conio.h>
#include <windows.h>
#include <cstdlib>  // Para usar rand() y generar números aleatorios

using namespace std;

const int width = 40;   ///< Ancho del tablero de juego
const int height = 20;  ///< Alto del tablero de juego
const int refreshRate = 50;  ///< Tasa de refresco de la pantalla en milisegundos

mutex mtx;

/**
 * @struct GameState
 * @brief Estructura que contiene el estado actual del juego.
 */
struct GameState {
    int ballX, ballY;          ///< Posición actual de la bola
    int ballDirX, ballDirY;    ///< Dirección de movimiento de la bola
    bool ballLaunched;         ///< Indica si la bola ha sido lanzada
    bool flipperLeftActive, flipperRightActive; ///< Estado de los flippers
    int score;                 ///< Puntuación del jugador
    int launchPower;           ///< Potencia del lanzamiento
    bool gameOver;             ///< Estado del juego (terminado o no)
    int ballSpeed;             ///< Velocidad de la bola
    int lives;                 ///< Número de vidas restantes
    int winningScore;          ///< Puntuación para ganar el juego
};

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void gotoXY(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setup(GameState& state) {
    state.gameOver = false;
    state.ballLaunched = false;
    state.ballX = width - 2;
    state.ballY = height / 2;
    state.ballDirX = 0;
    state.ballDirY = 0;
    state.flipperLeftActive = false;
    state.flipperRightActive = false;
    state.score = 0;
    state.launchPower = 0;
    state.lives = 3;
    state.winningScore = 50;
    hideCursor();
}

void draw(const GameState& state) {
    lock_guard<mutex> lock(mtx);
    gotoXY(0, 0);

    for (int i = 0; i < width + 2; i++) cout << "#";
    cout << endl;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x == 0 || x == width - 1) cout << "#";
            else if (x == state.ballX && y == state.ballY) cout << "O";
            else if (y == height - 1 && (x >= width / 4 - 2 && x <= width / 4 + 2)) cout << "|";
            else if (y == height - 1 && (x >= 3 * width / 4 - 2 && x <= 3 * width / 4 + 2)) cout << "|";
            else if (y == height - 1 && (x < width / 4 - 2 || x > 3 * width / 4 + 2)) cout << "#";
            else if (y == height - 2 && state.flipperLeftActive && (x >= width / 4 - 2 && x <= width / 4 + 2)) cout << "/";
            else if (y == height - 2 && state.flipperRightActive && (x >= 3 * width / 4 - 2 && x <= 3 * width / 4 + 2)) cout << "\\";
            else if (x == width - 2 && y == height / 2) cout << "L";
            else if (x == width - 2 && y == state.ballY && !state.ballLaunched) cout << "O";

            else if (y == 5 && (x >= 10 && x <= 15)) cout << "^";
            else if (y == 8 && (x >= 20 && x <= 25)) cout << "^";
            else if (y == 12 && (x >= 10 && x <= 15)) cout << ">";
            else if (y == 10 && (x >= 5 && x <= 7)) cout << "/";
            else if (y == 14 && (x >= 30 && x <= 32)) cout << "/";
            else cout << " ";
        }
        cout << endl;
    }

    for (int i = 0; i < width + 2; i++) {
        if (i < width / 4 - 2 || i > 3 * width / 4 + 2) cout << "#";
        else cout << " ";
    }
    cout << endl;

    cout << "Puntuacion: " << state.score << endl;
    cout << "Vidas restantes: " << state.lives << endl;
}

void resetBall(GameState& state) {
    state.ballLaunched = false;
    state.ballX = width - 2;
    state.ballY = height / 2;
    state.ballDirX = 0;
    state.ballDirY = 0;
}

void updateBall(GameState& state) {
    while (!state.gameOver) {
        if (state.ballLaunched) {
            state.ballX += state.ballDirX * state.ballSpeed;
            state.ballY += state.ballDirY * state.ballSpeed;

            if (state.ballY == 0) state.ballDirY *= -1;
            else if (state.ballY == height - 1) {
                if (state.ballX > width / 4 + 2 && state.ballX < 3 * width / 4 - 2) {
                    state.lives--;
                    if (state.lives <= 0) state.gameOver = true;
                    else resetBall(state);
                } else state.ballDirY *= -1;
            }

            if (state.ballX <= 0) {
                state.ballDirX *= -1;
                state.ballX = 0;
            } else if (state.ballX >= width - 1) {
                state.ballDirX *= -1;
                state.ballX = width - 1;
            }

            if (state.ballY == height - 2) {
                if (state.ballX >= width / 4 - 4 && state.ballX <= width / 4 + 4 && state.flipperLeftActive) {
                    state.ballDirY = -1;
                    state.ballDirX = (state.ballX - (width / 4)) / 2;
                    state.score += 10;
                } else if (state.ballX >= 3 * width / 4 - 4 && state.ballX <= 3 * width / 4 + 4 && state.flipperRightActive) {
                    state.ballDirY = -1;
                    state.ballDirX = (state.ballX - (3 * width / 4)) / 2;
                    state.score += 10;
                }
            }

            if ((state.ballY == 5 && state.ballX >= 10 && state.ballX <= 15) || (state.ballY == 8 && state.ballX >= 20 && state.ballX <= 25)) {
                state.ballDirY *= -1;
            }

            if (state.ballY == 12 && state.ballX >= 10 && state.ballX <= 15) {
                state.ballDirX = 2;
            }

            if (state.ballY == 10 && state.ballX >= 5 && state.ballX <= 7) {
                state.ballDirX = 1;
                state.ballDirY = -1;
            }
            if (state.ballY == 14 && state.ballX >= 30 && state.ballX <= 32) {
                state.ballDirX = -1;
                state.ballDirY = -1;
            }

            if (state.score >= state.winningScore) {
                state.gameOver = true;
                cout << "¡Felicidades, has ganado!" << endl;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(30));
    }
}

void handleInput(GameState& state) {
    while (!state.gameOver) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 'a') state.flipperLeftActive = true;
            else if (key == 'd') state.flipperRightActive = true;
            else if (key == ' ') {
                if (!state.ballLaunched) {
                    state.ballDirX = -1;
                    state.ballDirY = 1;
                    state.ballLaunched = true;
                }
            }
        } else {
            state.flipperLeftActive = false;
            state.flipperRightActive = false;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

int main() {
    GameState gameState;
    setup(gameState);

    thread inputThread(handleInput, ref(gameState));
    thread updateBallThread(updateBall, ref(gameState));

    while (!gameState.gameOver) {
        draw(gameState);
        this_thread::sleep_for(chrono::milliseconds(refreshRate));
    }

    inputThread.join();
    updateBallThread.join();

    return 0;
}
