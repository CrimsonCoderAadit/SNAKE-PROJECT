#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CELL_SIZE 20
#define GRID_WIDTH (WINDOW_WIDTH / CELL_SIZE)
#define GRID_HEIGHT (WINDOW_HEIGHT / CELL_SIZE)

// Score display constants
#define SCORE_DIGIT_WIDTH 10
#define SCORE_DIGIT_HEIGHT 20
#define SCORE_PADDING 5
#define SCORE_SEGMENT_THICKNESS 3

// Button dimensions
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_PADDING 20

// Game states
typedef enum {
    MENU,
    PLAYING,
    GAME_OVER
} GameState;

typedef struct {
    int x, y;
} Segment;

typedef struct {
    Segment body[100]; 
    int length;
    int dx, dy;
    bool alive;
} Snake;

typedef struct {
    int x, y;
} Food;

typedef struct {
    SDL_Rect rect;
    char text[20];
    bool hover;
} Button;

// Function prototypes
void draw_grid(SDL_Renderer *renderer);
void draw_snake(SDL_Renderer *renderer, Snake *snake);
void draw_food(SDL_Renderer *renderer, Food *food);
void draw_segment(SDL_Renderer *renderer, int x, int y, char segment, int width, int height, int thickness);
void draw_digit(SDL_Renderer *renderer, int x, int y, int digit, int width, int height, int thickness);
void draw_score(SDL_Renderer *renderer, int score, TTF_Font *font);
void move_snake(Snake *snake);
bool check_food_collision(Snake *snake, Food *food);
void place_food(Food *food, Snake *snake);
void grow_snake(Snake *snake);
void init_button(Button *button, int x, int y, const char *text);
void draw_button(SDL_Renderer *renderer, Button *button, TTF_Font *font);
bool is_point_in_rect(int x, int y, SDL_Rect *rect);
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void draw_text_centered(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void draw_welcome_screen(SDL_Renderer *renderer, Button *playButton, TTF_Font *font);
void draw_game_over_screen(SDL_Renderer *renderer, int score, Button *playAgainButton, Button *exitButton, TTF_Font *font);
void reset_game(Snake *snake, Food *food, int *score);

// Main function remains at the bottom

void draw_grid(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

    for (int x = 0; x <= WINDOW_WIDTH; x += CELL_SIZE) {
        SDL_RenderDrawLine(renderer, x, 0, x, WINDOW_HEIGHT);
    }

    for (int y = 0; y <= WINDOW_HEIGHT; y += CELL_SIZE) {
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
}

void draw_snake(SDL_Renderer *renderer, Snake *snake) {
    // Draw body segments in green
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    for (int i = 1; i < snake->length; i++) {
        SDL_Rect rect = {snake->body[i].x * CELL_SIZE, snake->body[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        SDL_RenderFillRect(renderer, &rect);
    }
    
    // Draw head in brighter green
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect head_rect = {snake->body[0].x * CELL_SIZE, snake->body[0].y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(renderer, &head_rect);
}

void draw_food(SDL_Renderer *renderer, Food *food) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect rect = {food->x * CELL_SIZE, food->y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(renderer, &rect);
}

// Function to draw a digit segment for the score display
void draw_segment(SDL_Renderer *renderer, int x, int y, char segment, int width, int height, int thickness) {
    SDL_Rect rect;
    
    switch(segment) {
        case 'a': // Top horizontal
            rect = (SDL_Rect){x, y, width, thickness};
            break;
        case 'b': // Top-right vertical
            rect = (SDL_Rect){x + width - thickness, y, thickness, height / 2};
            break;
        case 'c': // Bottom-right vertical
            rect = (SDL_Rect){x + width - thickness, y + height / 2, thickness, height / 2};
            break;
        case 'd': // Bottom horizontal
            rect = (SDL_Rect){x, y + height - thickness, width, thickness};
            break;
        case 'e': // Bottom-left vertical
            rect = (SDL_Rect){x, y + height / 2, thickness, height / 2};
            break;
        case 'f': // Top-left vertical
            rect = (SDL_Rect){x, y, thickness, height / 2};
            break;
        case 'g': // Middle horizontal
            rect = (SDL_Rect){x, y + height / 2 - thickness / 2, width, thickness};
            break;
        default:
            return;
    }
    
    SDL_RenderFillRect(renderer, &rect);
}

// Function to draw a digit (0-9) for the score display
void draw_digit(SDL_Renderer *renderer, int x, int y, int digit, int width, int height, int thickness) {
    // Define which segments to light up for each digit (a-g)
    const char* segments[] = {
        "abcdef",  // 0
        "bc",      // 1
        "abged",   // 2
        "abgcd",   // 3
        "fbgc",    // 4
        "afgcd",   // 5
        "afgcde",  // 6
        "abc",     // 7
        "abcdefg", // 8
        "abfgcd"   // 9
    };
    
    if (digit < 0 || digit > 9) return;
    
    const char* active_segments = segments[digit];
    size_t len = strlen(active_segments);
    
    for (size_t i = 0; i < len; i++) {
        draw_segment(renderer, x, y, active_segments[i], width, height, thickness);
    }
}

// Function to draw the score in the top-left corner using SDL_ttf
void draw_score(SDL_Renderer *renderer, int score, TTF_Font *font) {
    // Background for score display
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect score_bg = {SCORE_PADDING, SCORE_PADDING, 150, 40};
    SDL_RenderFillRect(renderer, &score_bg);
    
    // Draw score text with SDL_ttf
    char score_text[32];
    sprintf(score_text, "SCORE: %d", score);
    
    SDL_Color white = {255, 255, 255, 255};
    draw_text(renderer, font, score_text, SCORE_PADDING * 2, SCORE_PADDING * 2, white);
}

void move_snake(Snake *snake) {
    // Move body segments
    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }
    
    // Move head
    snake->body[0].x += snake->dx;
    snake->body[0].y += snake->dy;

    // Check wall collision
    if (snake->body[0].x < 0 || snake->body[0].x >= GRID_WIDTH ||
        snake->body[0].y < 0 || snake->body[0].y >= GRID_HEIGHT) {
        snake->alive = false;
    }
    
    // Check self collision
    for (int i = 1; i < snake->length; i++) {
        if (snake->body[0].x == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
            snake->alive = false;
            break;
        }
    }
}

bool check_food_collision(Snake *snake, Food *food) {
    return (snake->body[0].x == food->x && snake->body[0].y == food->y);
}

void place_food(Food *food, Snake *snake) {
    bool valid_position;
    
    do {
        valid_position = true;
        food->x = rand() % GRID_WIDTH;
        food->y = rand() % GRID_HEIGHT;
        
        // Check if food is not on the snake
        for (int i = 0; i < snake->length; i++) {
            if (food->x == snake->body[i].x && food->y == snake->body[i].y) {
                valid_position = false;
                break;
            }
        }
    } while (!valid_position);
}

void grow_snake(Snake *snake) {
    if (snake->length < 100) {
        // The new segment is initially placed at the same position as the last segment
        // It will move correctly in the next frame
        snake->body[snake->length] = snake->body[snake->length - 1];
        snake->length++;
    }
}

// Initialize a button
void init_button(Button *button, int x, int y, const char *text) {
    button->rect.x = x;
    button->rect.y = y;
    button->rect.w = BUTTON_WIDTH;
    button->rect.h = BUTTON_HEIGHT;
    button->hover = false;
    strncpy(button->text, text, sizeof(button->text) - 1);
    button->text[sizeof(button->text) - 1] = '\0'; // Ensure null termination
}

// Draw a button with SDL_ttf
void draw_button(SDL_Renderer *renderer, Button *button, TTF_Font *font) {
    // Button background
    if (button->hover) {
        SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255); // Highlight color when hovering
    } else {
        SDL_SetRenderDrawColor(renderer, 70, 120, 170, 255); // Normal button color
    }
    SDL_RenderFillRect(renderer, &button->rect);
    
    // Button border
    SDL_SetRenderDrawColor(renderer, 40, 80, 120, 255);
    SDL_RenderDrawRect(renderer, &button->rect);
    
    // Button text with SDL_ttf
    SDL_Color white = {255, 255, 255, 255};
    draw_text_centered(renderer, font, button->text, 
                     button->rect.x + button->rect.w / 2, 
                     button->rect.y + button->rect.h / 2, 
                     white);
}

// Check if a point is inside a rectangle
bool is_point_in_rect(int x, int y, SDL_Rect *rect) {
    return (x >= rect->x && x < rect->x + rect->w &&
            y >= rect->y && y < rect->y + rect->h);
}

// Draw text with SDL_ttf
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Draw centered text with SDL_ttf
void draw_text_centered(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect rect = {x - surface->w / 2, y - surface->h / 2, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Draw welcome screen with SDL_ttf
void draw_welcome_screen(SDL_Renderer *renderer, Button *playButton, TTF_Font *font) {
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
    SDL_RenderClear(renderer);
    
    // Title
    SDL_Color green = {0, 200, 0, 255};
    draw_text_centered(renderer, font, "WELCOME TO SNAKE GAME", 
                     WINDOW_WIDTH / 2, 
                     WINDOW_HEIGHT / 3, 
                     green);
    
    // Draw play button with SDL_ttf
    draw_button(renderer, playButton, font);
}

// Draw game over screen with SDL_ttf
void draw_game_over_screen(SDL_Renderer *renderer, int score, Button *playAgainButton, Button *exitButton, TTF_Font *font) {
    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Game over text
    SDL_Color red = {255, 0, 0, 255};
    draw_text_centered(renderer, font, "GAME OVER", 
                     WINDOW_WIDTH / 2, 
                     WINDOW_HEIGHT / 4, 
                     red);
    
    // Score text
    char score_text[32];
    sprintf(score_text, "YOUR SCORE: %d", score);
    SDL_Color white = {255, 255, 255, 255};
    draw_text_centered(renderer, font, score_text, 
                     WINDOW_WIDTH / 2, 
                     WINDOW_HEIGHT / 3, 
                     white);
    
    // Draw buttons with SDL_ttf
    draw_button(renderer, playAgainButton, font);
    draw_button(renderer, exitButton, font);
}

// Reset game state
void reset_game(Snake *snake, Food *food, int *score) {
    snake->length = 5;
    snake->dx = 1;
    snake->dy = 0;
    snake->alive = true;
    
    for (int i = 0; i < snake->length; i++) {
        snake->body[i].x = 5 - i;
        snake->body[i].y = 5;
    }
    
    place_food(food, snake);
    *score = 0;
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Load font - using DejaVuSans.ttf from the correct path
    TTF_Font *font = TTF_OpenFont("dejavu-fonts-ttf-2.37/ttf/DejaVuSans.ttf", 24);
    if (!font) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Load a smaller font for the score using the same DejaVuSans.ttf
    TTF_Font *small_font = TTF_OpenFont("dejavu-fonts-ttf-2.37/ttf/DejaVuSans.ttf", 18);
    if (!small_font) {
        printf("Failed to load small font! SDL_ttf Error: %s\n", TTF_GetError());
        small_font = font; // Use main font if small font fails to load
    }

    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize game state
    GameState gameState = MENU;
    
    // Initialize snake
    Snake snake;
    snake.length = 5;
    snake.dx = 1;  
    snake.dy = 0;  
    snake.alive = true;

    for (int i = 0; i < snake.length; i++) {
        snake.body[i].x = 5 - i;
        snake.body[i].y = 5;
    }
    
    // Initialize food
    Food food;
    place_food(&food, &snake);

    // Initialize buttons
    Button playButton, playAgainButton, exitButton;
    init_button(&playButton, WINDOW_WIDTH / 2 - BUTTON_WIDTH / 2, 
               WINDOW_HEIGHT / 2, "PLAY");
    init_button(&playAgainButton, WINDOW_WIDTH / 2 - BUTTON_WIDTH / 2, 
               WINDOW_HEIGHT / 2, "PLAY AGAIN");
    init_button(&exitButton, WINDOW_WIDTH / 2 - BUTTON_WIDTH / 2, 
               WINDOW_HEIGHT / 2 + BUTTON_HEIGHT + BUTTON_PADDING, "EXIT");

    int running = 1;
    SDL_Event event;
    int score = 0;
    int mouseX, mouseY;
    
    // Game speed control
    Uint32 lastUpdateTime = 0;
    const int UPDATE_INTERVAL = 150; // milliseconds between updates

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
                mouseX = event.motion.x;
                mouseY = event.motion.y;
                
                // Update button hover states based on mouse position
                if (gameState == MENU) {
                    playButton.hover = is_point_in_rect(mouseX, mouseY, &playButton.rect);
                } else if (gameState == GAME_OVER) {
                    playAgainButton.hover = is_point_in_rect(mouseX, mouseY, &playAgainButton.rect);
                    exitButton.hover = is_point_in_rect(mouseX, mouseY, &exitButton.rect);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseX = event.button.x;
                    mouseY = event.button.y;
                    
                    // Handle button clicks
                    if (gameState == MENU && is_point_in_rect(mouseX, mouseY, &playButton.rect)) {
                        gameState = PLAYING;
                        reset_game(&snake, &food, &score);
                    } else if (gameState == GAME_OVER) {
                        if (is_point_in_rect(mouseX, mouseY, &playAgainButton.rect)) {
                            gameState = PLAYING;
                            reset_game(&snake, &food, &score);
                        } else if (is_point_in_rect(mouseX, mouseY, &exitButton.rect)) {
                            running = 0;
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN && gameState == PLAYING) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (snake.dy == 0) { // Prevent 180-degree turns
                            snake.dx = 0;
                            snake.dy = -1;
                        }
                        break;
                    case SDLK_DOWN:
                        if (snake.dy == 0) {  
                            snake.dx = 0;
                            snake.dy = 1;
                        }
                        break;
                    case SDLK_LEFT:
                        if (snake.dx == 0) {  
                            snake.dx = -1;
                            snake.dy = 0;
                        }
                        break;
                    case SDLK_RIGHT:
                        if (snake.dx == 0) {  
                            snake.dx = 1;
                            snake.dy = 0;
                        }
                        break;
                    case SDLK_ESCAPE:
                        gameState = MENU;
                        break;
                }
            }
        }

        // Current time for game update
        Uint32 currentTime = SDL_GetTicks();
        
        // Update game state at fixed intervals
        if (gameState == PLAYING && currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
            lastUpdateTime = currentTime;
            if (snake.alive) {
                move_snake(&snake);
                
                // Check food collision
                if (check_food_collision(&snake, &food)) {
                    grow_snake(&snake);
                    place_food(&food, &snake);
                    score += 10;
                }
            } else {
                gameState = GAME_OVER;
            }
        }

        // Render based on game state
        switch (gameState) {
            case MENU:
                draw_welcome_screen(renderer, &playButton, font);
                break;
                
            case PLAYING:
                // Clear the screen
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                
                // Draw game elements
                draw_grid(renderer);
                draw_snake(renderer, &snake);
                draw_food(renderer, &food);
                draw_score(renderer, score, small_font);
                break;
                
            case GAME_OVER:
                // Keep the game screen visible in the background
                draw_game_over_screen(renderer, score, &playAgainButton, &exitButton, font);
                break;
        }

        SDL_RenderPresent(renderer);
        
        // Cap the frame rate
        SDL_Delay(16); // ~60 FPS
    }

    // Clean up resources
    TTF_CloseFont(font);
    TTF_CloseFont(small_font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
