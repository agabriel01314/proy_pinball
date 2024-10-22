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
};

/**
 * @brief Muestra el menú de selección de modo de juego.
 * @param state Referencia al estado del juego.
 */
void showMenu(GameState& state) {
    int option;
    cout << "Bienvenido al Pinball!\n";
    cout << "Selecciona un modo de juego:\n";
    cout << "1. Modo Lento\n";
    cout << "2. Modo Rapido\n";
    cout << "Opcion: ";
    cin >> option;

    if (option == 1) {
        state.ballSpeed = 1;  ///< Modo lento
        cout << "Has seleccionado el Modo Lento. ¡Buena suerte!\n";
    } else {
        state.ballSpeed = 2;  ///< Modo rápido
        cout << "Has seleccionado el Modo Rápido. ¡Buena suerte!\n";
    }
}

/**
 * @brief Oculta el cursor de la consola.
 */
void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}
/**
 * @brief Posiciona el cursor en las coordenadas especificadas de la consola.
 * @param x Coordenada x (horizontal).
 * @param y Coordenada y (vertical).
 */
void gotoXY(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

/**
 * @brief Configura el estado inicial del juego.
 * @param state Referencia al estado del juego.
 */
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

/**
 * @brief Dibuja el tablero de juego en función del estado actual.
 * @param state Estado del juego.
 */
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
    cout << "Vidas restantes: " << state.lives << endl;
}

/**
 * @brief Resetea la posición de la bola tras perder una vida.
 * @param state Estado del juego.
 */
void resetBall(GameState& state) {
    state.ballLaunched = false;
    state.ballX = width - 2;
    state.ballY = height / 2;
    state.ballDirX = 0;
    state.ballDirY = 0;
}

/**
 * @brief Actualiza la posición de la bola y maneja las colisiones.
 * @param state Estado del juego.
 */
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

/**
 * @brief Controla el lanzamiento de la bola.
 * @param state Estado del juego.
 */
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
/**
 * @brief Controla la lógica principal del juego y el refresco de la pantalla.
 * @param state Estado del juego.
 */
void gameLoop(GameState& state) {
    while (!state.gameOver) {
        draw(state);
        input(state);
        updateBall(state);
        this_thread::sleep_for(chrono::milliseconds(refreshRate));
    }
}
/**
 * @brief Función principal que ejecuta el juego.
 * @return Código de salida del programa.
 */
int main() {
    GameState state;
    setup(state);
    showMenu(state);
    gameLoop(state);
    cout << "Juego terminado! Puntuacion final: " << state.score << endl;
    return 0;
}
