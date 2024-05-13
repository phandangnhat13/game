#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int BIRD_SIZE = 40;
const int PIPE_WIDTH = 80;
const int PIPE_GAP = 200;
const int PIPE_SPEED = 2;
const int MAX_PIPES = 3;
const int PIPE_Y_SPEED = 1; // Tốc độ di chuyển lên xuống của ống
const int PIPE_Y_RANGE = 100; // Phạm vi di chuyển lên xuống của ống

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* birdTexture = NULL;
SDL_Texture* pipeTopTexture = NULL;
SDL_Texture* pipeBottomTexture = NULL;
SDL_Texture* backgroundTexture = NULL;
SDL_Texture* menuBackgroundTexture = NULL;
SDL_Texture* highScoreBackgroundTexture = NULL;
TTF_Font* font = NULL;

enum GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    HIGH_SCORE
};

GameState gameState = MENU;
int score = 0;
int highScore = 0;

struct Bird {
    int x, y;
    float velocity;
};

struct Pipe {
    int x, y;
    int originalY; // Vị trí y ban đầu của ống
    bool passed;
    int yDirection; // Hướng di chuyển của ống
};

// Hàm tải hình ảnh thành texture
SDL_Texture* loadTexture(const string& filePath) {
    SDL_Surface* surface = IMG_Load(filePath.c_str());
    if (!surface) {
        cerr << "Error loading surface: " << IMG_GetError() << endl;
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Hàm tạo văn bản kết cấu
SDL_Texture* createTextTexture(const string& text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        cerr << "Error creating text surface: " << TTF_GetError() << endl;
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Khởi tạo trò chơi
bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL init error: " << SDL_GetError() << endl;
        return false;
    }
    if (TTF_Init() < 0) {
        cerr << "TTF init error: " << TTF_GetError() << endl;
        return false;
    }
    window = SDL_CreateWindow("Flappy Bird", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Window creation error: " << SDL_GetError() << endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Renderer creation error: " << SDL_GetError() << endl;
        return false;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        cerr << "IMG_Init error: " << IMG_GetError() << endl;
        return false;
    }
    // Tải font chữ
    font = TTF_OpenFont("font.ttf", 24);
    if (!font) {
        cerr << "Font load error: " << TTF_GetError() << endl;
        return false;
    }
    return true;
}

// Tải tài nguyên trò chơi
bool loadResources() {
    birdTexture = loadTexture("bird.png");
    pipeTopTexture = loadTexture("top_pipe.png");
    pipeBottomTexture = loadTexture("bottom_pipe.png");
    backgroundTexture = loadTexture("background.png");
    menuBackgroundTexture = loadTexture("menu_background.png");
    highScoreBackgroundTexture = loadTexture("high_score_background.png");

    if (!birdTexture || !pipeTopTexture || !pipeBottomTexture ||
        !backgroundTexture || !menuBackgroundTexture || !highScoreBackgroundTexture) {
        cerr << "Failed to load one or more textures." << endl;
        return false;
    }
    return true;
}

// Dọn dẹp tài nguyên
void cleanup() {
    if (birdTexture) SDL_DestroyTexture(birdTexture);
    if (pipeTopTexture) SDL_DestroyTexture(pipeTopTexture);
    if (pipeBottomTexture) SDL_DestroyTexture(pipeBottomTexture);
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (menuBackgroundTexture) SDL_DestroyTexture(menuBackgroundTexture);
    if (highScoreBackgroundTexture) SDL_DestroyTexture(highScoreBackgroundTexture);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// Hàm kiểm tra va chạm giữa chim và ống
bool checkCollision(const Bird& bird, const Pipe& pipe) {
    if ((bird.x + BIRD_SIZE > pipe.x && bird.x < pipe.x + PIPE_WIDTH) &&
        ((bird.y < pipe.y) || (bird.y + BIRD_SIZE > pipe.y + PIPE_GAP))) {
        return true;
    }
    return false;
}

// Vẽ trò chơi
void renderGame(const Bird& bird, const Pipe pipes[], int numPipes) {
    SDL_RenderClear(renderer);

    // Vẽ background
    SDL_Rect backgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, backgroundTexture, NULL, &backgroundRect);

    // Vẽ chim
    SDL_Rect birdRect = {bird.x, bird.y, BIRD_SIZE, BIRD_SIZE};
    SDL_RenderCopy(renderer, birdTexture, NULL, &birdRect);

    // Vẽ các ống
    for (int i = 0; i < numPipes; i++) {
        const Pipe& pipe = pipes[i];
        // Vẽ ống trên
        SDL_Rect topPipeRect = {pipe.x, 0, PIPE_WIDTH, pipe.y};
        SDL_RenderCopy(renderer, pipeTopTexture, NULL, &topPipeRect);
        // Vẽ ống dưới
        SDL_Rect bottomPipeRect = {pipe.x, pipe.y + PIPE_GAP, PIPE_WIDTH, SCREEN_HEIGHT - (pipe.y + PIPE_GAP)};
        SDL_RenderCopy(renderer, pipeBottomTexture, NULL, &bottomPipeRect);
    }

    // Hiển thị điểm
    SDL_Color textColor = {255, 255, 255, 255};
    ostringstream oss;
    oss << "Score: " << score;
    SDL_Texture* scoreTexture = createTextTexture(oss.str(), textColor);
    int textWidth, textHeight;
    SDL_QueryTexture(scoreTexture, NULL, NULL, &textWidth, &textHeight);

    SDL_Rect textRect = {10, 10, textWidth, textHeight};
    SDL_RenderCopy(renderer, scoreTexture, NULL, &textRect);
    SDL_DestroyTexture(scoreTexture);

    SDL_RenderPresent(renderer);
}

// Hiển thị màn hình trò chơi kết thúc
void renderGameOver() {
    SDL_RenderClear(renderer);

    // Vẽ background cho màn hình trò chơi kết thúc
    SDL_Rect gameOverBackgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, &gameOverBackgroundRect);

    SDL_Color textColor = {255, 255, 255, 255};

    // Hiển thị thông báo trò chơi kết thúc
    SDL_Texture* gameOverTexture = createTextTexture("Game Over! Press P to Play Again or Q to Quit", textColor);
    int textWidth, textHeight;
    SDL_QueryTexture(gameOverTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect gameOverRect = {SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
    SDL_DestroyTexture(gameOverTexture);

    SDL_RenderPresent(renderer);
}

// Hiển thị menu
void renderMenu() {
    SDL_RenderClear(renderer);

    // Vẽ background cho menu
    SDL_Rect menuBackgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, &menuBackgroundRect);

    SDL_Color textColor = {255, 255, 255, 255};

    // Hiển thị tiêu đề trò chơi
    SDL_Texture* titleTexture = createTextTexture("Flappy Bird", textColor);
    int textWidth, textHeight;
    SDL_QueryTexture(titleTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 4, textWidth, textHeight};
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_DestroyTexture(titleTexture);

    // Hiển thị hướng dẫn để bắt đầu chơi
    SDL_Texture* playTexture = createTextTexture("Press SPACE to Play", textColor);
    SDL_QueryTexture(playTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect playRect = {SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, playTexture, NULL, &playRect);
    SDL_DestroyTexture(playTexture);

    // Hiển thị hướng dẫn để xem high score
    SDL_Texture* highScoreTexture = createTextTexture("Press H to View High Scores", textColor);
    SDL_QueryTexture(highScoreTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect highScoreRect = {SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 + textHeight, textWidth, textHeight};
    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreRect);
    SDL_DestroyTexture(highScoreTexture);

    SDL_RenderPresent(renderer);
}



// Hiển thị màn hình high score
void renderHighScore() {
    SDL_RenderClear(renderer);

    // Vẽ background cho màn hình high score
    SDL_Rect highScoreBackgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, highScoreBackgroundTexture, NULL, &highScoreBackgroundRect);

    SDL_Color textColor = {255, 255, 255, 255};

    // Hiển thị high score
    ostringstream oss;
    oss << "High Score: " << highScore;
    SDL_Texture* highScoreTexture = createTextTexture(oss.str(), textColor);
    int textWidth, textHeight;
    SDL_QueryTexture(highScoreTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect highScoreRect = {SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreRect);
    SDL_DestroyTexture(highScoreTexture);

    // Hiển thị hướng dẫn để quay lại menu
    SDL_Texture* backToMenuTexture = createTextTexture("Press M to go back to Menu", textColor);
    SDL_QueryTexture(backToMenuTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect backToMenuRect = {SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 + textHeight / 2 + 50, textWidth, textHeight};
    SDL_RenderCopy(renderer, backToMenuTexture, NULL, &backToMenuRect);
    SDL_DestroyTexture(backToMenuTexture);
    SDL_RenderPresent(renderer);
}

// Xử lý đầu vào của người chơi
void handleInput(bool& running, Bird& bird) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            running = false;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_SPACE:
                    if (gameState == MENU) {
                        gameState = PLAYING;
                    } else if (gameState == PLAYING) {
                        bird.velocity = -5; // Chim bay lên
                    }
                    break;
                case SDLK_p:
                    if (gameState == GAME_OVER) {
                        gameState = MENU;
                    }
                    break;
                case SDLK_h:
                    if (gameState == MENU) {
                        gameState = HIGH_SCORE;
                    }
                    break;
                case SDLK_m:
                    if (gameState == HIGH_SCORE) {
                        gameState = MENU;
                    }
                    break;
                case SDLK_q:
                    running = false; // Thoát khỏi trò chơi
                    break;
            }
        }
    }
}

// Cập nhật trò chơi
void updateGame(Bird& bird, Pipe pipes[], int& numPipes, bool& gameOver) {
    bird.velocity += 0.5; // Trọng lực ảnh hưởng đến chim
    bird.y += bird.velocity;

    if (bird.y < 0) {
        bird.y = 0; // Chim không được bay quá cao
    }

    // Kiểm tra va chạm giữa chim và ống
    for (int i = 0; i < numPipes; i++) {
        if (checkCollision(bird, pipes[i])) {
            gameOver = true;
            break;
        }

        // Cập nhật ống di chuyển sang trái
        pipes[i].x -= PIPE_SPEED;

        // Cập nhật di chuyển lên xuống của ống
        pipes[i].y += pipes[i].yDirection * PIPE_Y_SPEED;

        // Kiểm tra nếu ống đã chạm vào giới hạn di chuyển lên xuống
        if (pipes[i].y < pipes[i].originalY - PIPE_Y_RANGE || pipes[i].y > pipes[i].originalY + PIPE_Y_RANGE) {
            pipes[i].yDirection *= -1; // Đảo ngược hướng di chuyển của ống
        }

        // Nếu ống đi ra khỏi màn hình, đặt lại vị trí ống và tăng điểm
        if (pipes[i].x < -PIPE_WIDTH) {
            pipes[i].x = SCREEN_WIDTH;
            pipes[i].y = rand() % (SCREEN_HEIGHT - PIPE_GAP - 100) + 50;
            pipes[i].originalY = pipes[i].y;
            pipes[i].yDirection = (rand() % 2 == 0) ? 1 : -1;
            pipes[i].passed = false;
        }

        // Kiểm tra nếu chim đã vượt qua ống
        if (!pipes[i].passed && pipes[i].x < bird.x) {
            pipes[i].passed = true;
            score++;
            if (score > highScore) {
                highScore = score; // Cập nhật high score
            }
        }
    }

    // Kiểm tra va chạm với đáy màn hình
    if (bird.y + BIRD_SIZE > SCREEN_HEIGHT) {
        gameOver = true;
    }
}

// Hàm chính của chương trình
int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!init()) {
        cerr << "Initialization failed." << endl;
        return 1;
    }

    if (!loadResources()) {
        cerr << "Failed to load resources." << endl;
        cleanup();
        return 1;
    }

    bool running = true;
    Bird bird = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2, 0};
    Pipe pipes[MAX_PIPES];
    int numPipes = MAX_PIPES;
    for (int i = 0; i < numPipes; i++) {
        pipes[i].x = SCREEN_WIDTH + i * (SCREEN_WIDTH / numPipes);
        pipes[i].y = rand() % (SCREEN_HEIGHT - PIPE_GAP - 100) + 50;
        pipes[i].originalY = pipes[i].y; // Lưu vị trí y ban đầu
        pipes[i].yDirection = (rand() % 2 == 0) ? 1 : -1; // Chọn ngẫu nhiên hướng di chuyển
        pipes[i].passed = false;
    }

    while (running) {
        handleInput(running, bird);

        if (gameState == PLAYING) {
            bool gameOver = false;
            updateGame(bird, pipes, numPipes, gameOver);
            renderGame(bird, pipes, numPipes);

            if (gameOver) {
                gameState = GAME_OVER;
            }
        } else if (gameState == MENU) {
            renderMenu();
        } else if (gameState == GAME_OVER) {
            renderGameOver();
        } else if (gameState == HIGH_SCORE) {
            renderHighScore();
        }

        SDL_Delay(16); // Đảm bảo tốc độ khung hình ổn định (khoảng 60fps)
    }

    cleanup();
    return 0;
}
