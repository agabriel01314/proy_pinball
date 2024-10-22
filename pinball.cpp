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
    state.ballSpeed = 1;  // Velocidad inicial de la bola
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


void updateBall(GameState& state) {
    if (state.ballLaunched) {
        // Mover la bola
        state.ballX += state.ballDirX * state.ballSpeed;
        state.ballY += state.ballDirY * state.ballSpeed;

        // Rebotes en los bordes superior e inferior
        if (state.ballY == 0 || state.ballY == height - 1) {
            state.ballDirY *= -1;

            // Cambio aleatorio en dirección X tras un rebote en el borde
            if (rand() % 2 == 0) {
                state.ballDirX += (rand() % 3 - 1);  // -1, 0 o 1
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
            }
            if (state.ballX >= 3 * width / 4 - 4 && state.ballX <= 3 * width / 4 + 4 && state.flipperRightActive) {
                state.ballDirY = -1;
                int deltaX = state.ballX - (3 * width / 4);
                state.ballDirX = deltaX / 2;
                state.score += 10;
            }
        }

        // Colisiones con las plataformas de rebote
        if (state.ballY == 5 && (state.ballX >= 10 && state.ballX <= 15)) {
            state.ballDirY *= -1;
            state.score += 50;

            // Cambio aleatorio de dirección tras el rebote
            state.ballDirX += (rand() % 3 - 1);
        }
        if (state.ballY == 8 && (state.ballX >= 20 && state.ballX <= 25)) {
            state.ballDirY *= -1;
            state.score += 50;

            // Cambio aleatorio de dirección tras el rebote
            state.ballDirX += (rand() % 3 - 1);
        }

        // Colisiones con plataformas de aceleración
        if (state.ballY == 12 && (state.ballX >= 10 && state.ballX <= 15)) {
            state.ballSpeed = 2;  // Aumentar la velocidad
            state.score += 100;
        } else {
            state.ballSpeed = 1;  // Restaurar la velocidad
        }

        // Si la bola llega al fondo (última fila del tablero)
        if (state.ballY == height - 1) {
            // Verificar si la bola está en el centro vacío entre los flippers
            if (state.ballX > width / 4 - 2 && state.ballX < 3 * width / 4 + 2) {
                // La bola cae en el centro vacío
                state.lives--;  // Perder una vida

                if (state.lives > 0) {
                    // Reiniciar la bola si aún quedan vidas
                    state.ballLaunched = false;
                    state.ballX = width - 2;
                    state.ballY = height / 2;
                    state.ballDirX = 0;
                    state.ballDirY = 0;
                    state.launchPower = 0;
                    
                    // Pequeña pausa para reiniciar después de perder una vida
                    this_thread::sleep_for(chrono::seconds(2));
                } else {
                    // Si ya no hay más vidas, terminar el juego
                    state.gameOver = true;
                    cout << "¡Juego terminado! No te quedan más vidas." << endl;
                }
            } else {
                // Si la bola cae en los bordes, rebotar
                state.ballDirY *= -1;
            }
        }
    }
}


// Función para manejar las entradas del jugador
void handleInput(GameState& state) {
    while (!state.gameOver) {
        if (_kbhit()) {
            char key = _getch();
            {
                lock_guard<mutex> lock(mtx);
                if (key == 'a') {
                    state.flipperLeftActive = true;  // Activa el flipper izquierdo
                }
                if (key == 'd') {
                    state.flipperRightActive = true; // Activa el flipper derecho
                }
                if (key == 'p') {
                    state.launchPower += (state.launchPower < 10) ? 1 : 0; // Aumentar potencia de lanzamiento
                }
                if (key == ' ' && !state.ballLaunched) {
                    state.ballLaunched = true;
                    state.ballDirX = (rand() % 2 == 0) ? 1 : -1; // Direcciones aleatorias
                    state.ballDirY = -1; // Lanzar hacia arriba
                }
                if (key == 'q') state.gameOver = true; // Salir del juego
            }
        }

        // Desactiva las palancas si no se presionan las teclas correspondientes
        {
            lock_guard<mutex> lock(mtx);
            state.flipperLeftActive = GetAsyncKeyState('A') & 0x8000;  // Activar flipper izquierdo si 'A' está presionada
            state.flipperRightActive = GetAsyncKeyState('D') & 0x8000; // Activar flipper derecho si 'D' está presionada
        }

        // Pequeña pausa para no sobrecargar el CPU
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}
// Hilo para la lógica del juego
void gameLogic(GameState& state) {
    while (!state.gameOver) {
        {
            lock_guard<mutex> lock(mtx);
            updateBall(state);
        }
        this_thread::sleep_for(chrono::milliseconds(50));  // Velocidad de la lógica
    }
}

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
    cout << "Vidas restantes: " << state.lives << endl;

    return 0;
}
