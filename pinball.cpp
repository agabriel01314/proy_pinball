#include <iostream>
#include <thread>
#include <mutex>
#include <conio.h>
#include <windows.h>
#include <cstdlib>  // Para usar rand() y generar números aleatorios

using namespace std;

const int width = 40;  // Ancho del tablero
const int height = 20;  // Alto del tablero
const int refreshRate = 50;  // Tasa de refresco de la pantalla

mutex mtx;

struct GameState {
    int ballX, ballY;        
    int ballDirX, ballDirY;  
    bool ballLaunched;       
    bool flipperLeftActive, flipperRightActive; 
    int score;               
    int launchPower;         
    bool gameOver;           
    int ballSpeed;       
    int lives;               
};

// Función para mostrar el menú y permitir al jugador seleccionar el modo de juego
void showMenu(GameState& state) {
    int option;
    cout << "Bienvenido al Pinball!\n";
    cout << "Selecciona un modo de juego:\n";
    cout << "1. Modo Lento\n";
    cout << "2. Modo Rápido\n";
    cout << "Opción: ";
    cin >> option;

    if (option == 1) {
        state.ballSpeed = 1;  // Modo lento (más lento)
        cout << "Has seleccionado el Modo Lento. ¡Buena suerte!\n";
    } else {
        state.ballSpeed = 2;  // Modo rápido (velocidad normal)
        cout << "Has seleccionado el Modo Rápido. ¡Buena suerte!\n";
    }
}

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
    state.lives = 3;      // Inicializamos con 3 vidas
    hideCursor();
}

// Función para dibujar el tablero
void draw(const GameState& state) {
    lock_guard<mutex> lock(mtx);
    gotoXY(0, 0);

    // Dibuja la parte superior del tablero
    for (int i = 0; i < width + 2; i++) cout << "#";
    cout << endl;

    // Dibuja el área de juego
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x == 0 || x == width - 1) cout << "#"; // Bordes
            else if (x == state.ballX && y == state.ballY) cout << "O"; // Bola
            else if (y == height - 1 && (x >= width / 4 - 2 && x <= width / 4 + 2)) cout << "|"; // Flipper izquierdo
            else if (y == height - 1 && (x >= 3 * width / 4 - 2 && x <= 3 * width / 4 + 2)) cout << "|"; // Flipper derecho
            else if (y == height - 1 && (x < width / 4 - 2 || x > 3 * width / 4 + 2)) cout << "#"; // Bordes laterales, centro vacío
            else if (y == height - 2 && state.flipperLeftActive && (x >= width / 4 - 2 && x <= width / 4 + 2)) cout << "/"; // Flipper izquierdo activado
            else if (y == height - 2 && state.flipperRightActive && (x >= 3 * width / 4 - 2 && x <= 3 * width / 4 + 2)) cout << "\\"; // Flipper derecho activado
            else if (x == width - 2 && y == height / 2) cout << "L"; // Lanzador
            else if (x == width - 2 && y == state.ballY && !state.ballLaunched) cout << "O"; // Bola en lanzador

            // Zonas de plataformas de rebote
            else if (y == 5 && (x >= 10 && x <= 15)) cout << "^";  // Plataforma de rebote
            else if (y == 8 && (x >= 20 && x <= 25)) cout << "^";  // Otra plataforma de rebote

            // Zonas de plataformas de aceleración
            else if (y == 12 && (x >= 10 && x <= 15)) cout << ">";  // Plataforma de aceleración

            // Plataformas diagonales
            else if (y == 10 && (x >= 5 && x <= 7)) cout << "/";  // Plataforma diagonal en la posición (10, 5-7)
            else if (y == 14 && (x >= 30 && x <= 32)) cout << "/";  // Plataforma diagonal en la posición (14, 30-32)

            else cout << " ";
        }
        cout << endl;
    }

    // Dibuja la parte inferior del tablero (con centro vacío)
    for (int i = 0; i < width + 2; i++) {
        if (i < width / 4 - 2 || i > 3 * width / 4 + 2) cout << "#"; // Bordes en los extremos
        else cout << " "; // Centro vacío
    }
    cout << endl;

    // Mostrar puntuación, potencia de lanzamiento y vidas restantes
    cout << "Puntuacion: " << state.score << endl;
    cout << "Potencia de lanzamiento: " << state.launchPower << endl;
    cout << "Vidas restantes: " << state.lives << endl;
}

// Resetea la posición de la bola para un nuevo lanzamiento
void resetBall(GameState& state) {
    state.ballLaunched = false;
    state.ballX = width - 2;
    state.ballY = height / 2;
    state.ballDirX = 0;
    state.ballDirY = 0;
}

// Función para actualizar la posición de la pelota y detectar colisiones
void updateBall(GameState& state) {
    if (state.ballLaunched) {
        // Mover la bola
        state.ballX += state.ballDirX * state.ballSpeed;
        state.ballY += state.ballDirY * state.ballSpeed;

        // Rebotes en los bordes superior e inferior
        if (state.ballY == 0) {
            state.ballDirY *= -1;  // Rebote en el borde superior
        } else if (state.ballY == height - 1) {
            if (state.ballX > width / 4 + 2 && state.ballX < 3 * width / 4 - 2) {
                state.lives--;  // Perder una vida
                if (state.lives <= 0) {
                    state.gameOver = true;
                } else {
                    resetBall(state);  // Respawn de la bola
                }
            } else {
                state.ballDirY *= -1;  // Rebote en las paredes laterales
            }
        }

        // Rebotes en los lados
        if (state.ballX <= 0) {
            state.ballDirX *= -1; // Rebote en el borde izquierdo
            state.ballX = 0;
        } else if (state.ballX >= width - 1) {
            state.ballDirX *= -1; // Rebote en el borde derecho
            state.ballX = width - 1;
        }

        // Colisiones con las paletas
        if (state.ballY == height - 2) {
            if (state.ballX >= width / 4 - 4 && state.ballX <= width / 4 + 4 && state.flipperLeftActive) {
                state.ballDirY = -1;
                int deltaX = state.ballX - (width / 4);
                state.ballDirX = deltaX / 2;
                state.score += 10;
            } else if (state.ballX >= 3 * width / 4 - 4 && state.ballX <= 3 * width / 4 + 4 && state.flipperRightActive) {
                state.ballDirY = -1;
                int deltaX = state.ballX - (3 * width / 4);
                state.ballDirX = deltaX / 2;
                state.score += 10;
            }
        }

        // Colisiones con plataformas '^' (rebote vertical)
        if ((state.ballY == 5 && state.ballX >= 10 && state.ballX <= 15) || (state.ballY == 8 && state.ballX >= 20 && state.ballX <= 25)) {
            state.ballDirY *= -1;  // Rebote en la dirección opuesta
        }

        // Colisiones con plataformas '>' (aceleración horizontal)
        if (state.ballY == 12 && state.ballX >= 10 && state.ballX <= 15) {
            state.ballDirX = 2;  // Aceleración hacia la derecha
        }

        // Colisiones con plataformas '/' (rebote diagonal)
        if (state.ballY == 10 && state.ballX >= 5 && state.ballX <= 7) {
            state.ballDirX = 1;  // Rebote diagonal hacia la derecha
            state.ballDirY = -1; // Rebote hacia arriba
        }
        if (state.ballY == 14 && state.ballX >= 30 && state.ballX <= 32) {
            state.ballDirX = -1;  // Rebote diagonal hacia la izquierda
            state.ballDirY = -1;  // Rebote hacia arriba
        }
    }
}

// Captura la entrada del jugador
void input(GameState& state) {
    if (_kbhit()) {
        char key = _getch();

        if (!state.ballLaunched) {
            if (key == ' ') {
                state.ballLaunched = true;
                state.ballDirX = -1 + rand() % 3;  // Dirección aleatoria
                state.ballDirY = -1;  // Hacia arriba
            } else if (key == 'w') {
                state.launchPower++;  // Aumentar la potencia de lanzamiento
            } else if (key == 's') {
                state.launchPower--;  // Disminuir la potencia de lanzamiento
            }
        }

        if (key == 'a') {
            state.flipperLeftActive = true;
        } else if (key == 'd') {
            state.flipperRightActive = true;
        }

        // Función de la tecla 'u' para perder una vida y reiniciar la bola
        if (key == 'u') {
            state.lives--;
            if (state.lives <= 0) {
                state.gameOver = true;
            } else {
                resetBall(state);  // Respawn de la bola
            }
        }
    } else {
        state.flipperLeftActive = false;
        state.flipperRightActive = false;
    }
}

void gameLoop(GameState& state) {
    while (!state.gameOver) {
        draw(state);
        input(state);
        updateBall(state);
        this_thread::sleep_for(chrono::milliseconds(refreshRate));
    }
}

int main() {
    GameState state;
    setup(state);
    showMenu(state);
    gameLoop(state);
    cout << "Juego terminado! Puntuacion final: " << state.score << endl;
    return 0;
}
