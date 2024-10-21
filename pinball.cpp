#include <iostream>
#include <thread>
#include <mutex>
#include <conio.h>
#include <windows.h>

using namespace std;

const int width = 40; // Ancho del tablero
const int height = 20; // Alto del tablero
const int refreshRate = 50; // Tasa de refresco de la pantalla

mutex mtx;


struct GameState {
    int ballX, ballY;          
    int ballDirX, ballDirY;    
    bool ballLaunched;         
    bool flipperLeftActive, flipperRightActive; 
    int score;                 
    int launchPower;           
    bool gameOver;             
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

// Configuración inicial del juego
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
    hideCursor();
}

// Función para dibujar el tablero
void draw(const GameState& state) {
    mtx.lock();
    gotoXY(0, 0);

    // Dibuja la parte superior del tablero
    for (int i = 0; i < width + 2; i++) cout << "#";
    cout << endl;

    // Dibuja el área de juego
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x == 0 || x == width - 1) cout << "#"; // Bordes
            else if (x == state.ballX && y == state.ballY) cout << "O"; // Bola
            else if (y == height - 1 && (x >= width / 4 - 2 && x <= width / 4 + 2)) cout << "|"; // Flippers no activados
            else if (y == height - 1 && (x >= 3 * width / 4 - 2 && x <= 3 * width / 4 + 2)) cout << "|"; // Flippers no activados
            else if (y == height - 2 && state.flipperLeftActive && (x >= width / 4 - 2 && x <= width / 4 + 2)) cout << "/"; // Pala izquierda activada
            else if (y == height - 2 && state.flipperRightActive && (x >= 3 * width / 4 - 2 && x <= 3 * width / 4 + 2)) cout << "\\"; // Pala derecha activada
            else if (x == width - 2 && y == height / 2) cout << "L";  // Lanzador
            else if (x == width - 2 && y == state.ballY && !state.ballLaunched) cout << "O";  // Bola en lanzador
            else cout << " ";
        }
        cout << endl;
    }

    // Dibuja la parte inferior del tablero
    for (int i = 0; i < width + 2; i++) cout << "#";
    cout << endl;

    cout << "Puntuacion: " << state.score << endl;
    cout << "Potencia de lanzamiento: " << state.launchPower << endl;

    mtx.unlock();
}

// Función para actualizar el movimiento de la bola y las colisiones
void updateBall(GameState& state) {
    if (state.ballLaunched) {
        state.ballX += state.ballDirX;
        state.ballY += state.ballDirY;

        // Rebotes en bordes
        if (state.ballY == 0 || state.ballY == height - 1) state.ballDirY *= -1;
        if (state.ballX == 0 || state.ballX == width - 1) state.ballDirX *= -1;

        // Colisiones con las paletas
        if (state.ballY == height - 2) {
            if (state.ballX >= width / 4 - 2 && state.ballX <= width / 4 + 2 && state.flipperLeftActive) {
                state.ballDirY = -1;
                state.ballDirX = -1;
                state.score += 10;
            }
            if (state.ballX >= 3 * width / 4 - 2 && state.ballX <= 3 * width / 4 + 2 && state.flipperRightActive) {
                state.ballDirY = -1;
                state.ballDirX = 1;
                state.score += 10;
            }
        }

        if (state.ballY == height - 1) state.gameOver = true;
    }
}

// Función para manejar las entradas del jugador
void handleInput(GameState& state) {
    while (!state.gameOver) {
        if (_kbhit()) {
            char key = _getch();
            mtx.lock();
            if (key == 'a') state.flipperLeftActive = true;
            if (key == 'd') state.flipperRightActive = true;
            if (key == 'p' && !state.ballLaunched) {
                if (state.launchPower < 10) state.launchPower++;
            }
            if (key == 32 && !state.ballLaunched) {
                state.ballLaunched = true;
                state.ballDirX = -1;
                state.ballDirY = state.launchPower / 3;
            }
            mtx.unlock();
        }

        // Verificar si las teclas siguen presionadas para bajar las paletas
        mtx.lock();
        if (!GetAsyncKeyState('A')) state.flipperLeftActive = false;
        if (!GetAsyncKeyState('L')) state.flipperRightActive = false;
        mtx.unlock();

        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

// Hilo para la lógica del juego
void gameLogic(GameState& state) {
    while (!state.gameOver) {
        mtx.lock();
        updateBall(state);
        mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(50));  //Velocidad de la lógica
    }
}

// Hilo para el renderizado
void render(GameState& state) {
    while (!state.gameOver) {
        draw(state);
        this_thread::sleep_for(chrono::milliseconds(refreshRate));
    }
}

int main() {
    GameState state;
    setup(state);

    thread gameThread(gameLogic, ref(state));
    thread inputThread(handleInput, ref(state));
    thread renderThread(render, ref(state));

    gameThread.join();
    inputThread.join();
    renderThread.join();

    gotoXY(0, height + 2);
    cout << "GAME OVER" << endl;
    cout << "Puntuacion final: " << state.score << endl;

    return 0;
}
