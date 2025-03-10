// Fixed issues with chaos mode and multifruit mode

// The main issue with multi-fruit mode was that the initialize_multi_fruits function
// wasn't being properly called during mode initialization and food wasn't properly 
// tracked across different modes

// In chaos mode, there were several initialization issues and conflicts with 
// how it handles multiple fruits and their movement

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

// Original grid dimensions
#define CELL_SIZE 20
#define GRID_WIDTH 32  // 640 / 20
#define GRID_HEIGHT 24 // 480 / 20

// UI dimensions
#define UI_HEIGHT 60  // Height of the UI area above the grid
#define UI_PADDING 10 // Padding inside UI area

// New window dimensions
#define WINDOW_WIDTH (GRID_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (GRID_HEIGHT * CELL_SIZE + UI_HEIGHT)

// Score display constants
#define SCORE_DIGIT_WIDTH 10
#define SCORE_DIGIT_HEIGHT 20
#define SCORE_PADDING 5
#define SCORE_SEGMENT_THICKNESS 3

// Button dimensions
#define BUTTON_WIDTH 300
#define BUTTON_HEIGHT 40
#define BUTTON_PADDING 10

// Max number of obstacles and foods
#define MAX_OBSTACLES 30
#define MAX_FOODS 5

// Game states
typedef enum {
    MENU,
    PLAYING,
    GAME_OVER
} GameState;

// Game modes
typedef enum {
    CLASSIC,
    TIMED,
    OBSTACLES,
    MOVING_FRUIT,
    MULTI_FRUIT,
    CHAOS
} GameMode;

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
    int value;   // Point value
    int type;    // Visual type
    bool moving; // Whether it moves
    int dx, dy;  // Direction for moving fruits
} Food;

typedef struct {
    int x, y;
} Obstacle;

typedef struct {
    SDL_Rect rect;
    char text[30];
    bool hover;
} Button;

typedef struct {
    bool timed;
    int timeRemaining; // In seconds
    int maxTime;       // Starting time
    
    bool hasObstacles;
    Obstacle obstacles[MAX_OBSTACLES];
    int obstacleCount;
    
    bool movingFruit;
    int fruitMoveInterval; // How often the fruit moves (in milliseconds)
    Uint32 lastFruitMove;  // Time of last fruit movement
    
    bool multiFruit;
    Food foods[MAX_FOODS];
    int foodCount;
    
    GameMode mode;
} GameConfig;

// Function prototypes
void draw_grid(SDL_Renderer *renderer);
void draw_snake(SDL_Renderer *renderer, Snake *snake);
void draw_food(SDL_Renderer *renderer, Food *food);
void draw_obstacles(SDL_Renderer *renderer, GameConfig *config);
void draw_segment(SDL_Renderer *renderer, int x, int y, char segment, int width, int height, int thickness);
void draw_digit(SDL_Renderer *renderer, int x, int y, int digit, int width, int height, int thickness);
void draw_score(SDL_Renderer *renderer, int score, TTF_Font *font);
void move_snake(Snake *snake);
bool check_food_collision(Snake *snake, Food *food);
void place_food(Food *food, Snake *snake, GameConfig *config);
void place_obstacles(GameConfig *config, Snake *snake);
void grow_snake(Snake *snake);
void init_button(Button *button, int x, int y, const char *text);
void draw_button(SDL_Renderer *renderer, Button *button, TTF_Font *font);
bool is_point_in_rect(int x, int y, SDL_Rect *rect);
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void draw_text_centered(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color);
void draw_menu_screen(SDL_Renderer *renderer, Button buttons[], int buttonCount, TTF_Font *font);
void draw_game_over_screen(SDL_Renderer *renderer, int score, Button *playAgainButton, Button *exitButton, TTF_Font *font);
void reset_game(Snake *snake, GameConfig *config, int *score);
void draw_ui_area(SDL_Renderer *renderer, int score, GameConfig *config, TTF_Font *font);
void move_foods(GameConfig *config);
void initialize_game_mode(GameConfig *config, GameMode mode, Snake *snake);
void update_game(Snake *snake, GameConfig *config, int *score, Uint32 currentTime);
void initialize_multi_fruits(GameConfig *config, Snake *snake);

// Main function remains at the bottom

void draw_grid(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

    // Draw grid inside the game area only
    for (int x = 0; x <= WINDOW_WIDTH; x += CELL_SIZE) {
        SDL_RenderDrawLine(renderer, x, UI_HEIGHT, x, WINDOW_HEIGHT);
    }

    for (int y = UI_HEIGHT; y <= WINDOW_HEIGHT; y += CELL_SIZE) {
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
    
    // Draw a more prominent border around the grid
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect border = {0, UI_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT - UI_HEIGHT};
    SDL_RenderDrawRect(renderer, &border);
}

void draw_snake(SDL_Renderer *renderer, Snake *snake) {
    // Draw body segments in green
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    for (int i = 1; i < snake->length; i++) {
        SDL_Rect rect = {
            snake->body[i].x * CELL_SIZE, 
            snake->body[i].y * CELL_SIZE + UI_HEIGHT, // Adjust for UI area
            CELL_SIZE, 
            CELL_SIZE
        };
        SDL_RenderFillRect(renderer, &rect);
    }
    
    // Draw head in brighter green
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect head_rect = {
        snake->body[0].x * CELL_SIZE, 
        snake->body[0].y * CELL_SIZE + UI_HEIGHT, // Adjust for UI area
        CELL_SIZE, 
        CELL_SIZE
    };
    SDL_RenderFillRect(renderer, &head_rect);
}

void draw_food(SDL_Renderer *renderer, Food *food) {
    // Different colors for different food types
    switch (food->type) {
        case 0: // Regular food (red)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case 1: // Bonus food (gold)
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
            break;
        case 2: // Special food (purple)
            SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
            break;
        case 3: // Rare food (blue)
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            break;
        default:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }
    
    SDL_Rect rect = {
        food->x * CELL_SIZE, 
        food->y * CELL_SIZE + UI_HEIGHT, // Adjust for UI area
        CELL_SIZE, 
        CELL_SIZE
    };
    SDL_RenderFillRect(renderer, &rect);
}

void draw_obstacles(SDL_Renderer *renderer, GameConfig *config) {
    if (!config->hasObstacles) return;
    
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    
    for (int i = 0; i < config->obstacleCount; i++) {
        SDL_Rect rect = {
            config->obstacles[i].x * CELL_SIZE,
            config->obstacles[i].y * CELL_SIZE + UI_HEIGHT,
            CELL_SIZE,
            CELL_SIZE
        };
        SDL_RenderFillRect(renderer, &rect);
    }
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

// Function to draw the UI area with score and game mode specific info
void draw_ui_area(SDL_Renderer *renderer, int score, GameConfig *config, TTF_Font *font) {
    // Background for UI area
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_Rect ui_rect = {0, 0, WINDOW_WIDTH, UI_HEIGHT};
    SDL_RenderFillRect(renderer, &ui_rect);
    
    // Draw a border between UI area and game grid
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawLine(renderer, 0, UI_HEIGHT, WINDOW_WIDTH, UI_HEIGHT);
    
    // Draw score text with SDL_ttf
    char score_text[32];
    sprintf(score_text, "SCORE: %d", score);
    
    SDL_Color white = {255, 255, 255, 255};
    draw_text(renderer, font, score_text, UI_PADDING, UI_HEIGHT / 2 - 10, white);
    
    // Draw game mode name
    const char *mode_names[] = {
        "CLASSIC MODE", "TIMED MODE", "OBSTACLES MODE", 
        "MOVING FRUIT MODE", "MULTI FRUIT MODE", "CHAOS MODE"
    };
    
    draw_text(renderer, font, mode_names[config->mode], 
              WINDOW_WIDTH / 2 - 80, UI_HEIGHT / 2 - 10, white);
    
    // Draw time remaining for timed mode
    if (config->timed) {
        char time_text[20];
        sprintf(time_text, "TIME: %ds", config->timeRemaining);
        draw_text(renderer, font, time_text, WINDOW_WIDTH - 150, UI_HEIGHT / 2 - 10, white);
    }
}

// Modified score function now calls the UI area function
void draw_score(SDL_Renderer *renderer, int score, TTF_Font *font) {
    // This is a legacy function, kept for compatibility
    GameConfig config = {0};
    draw_ui_area(renderer, score, &config, font);
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

bool check_obstacle_collision(Snake *snake, GameConfig *config) {
    if (!config->hasObstacles) return false;
    
    for (int i = 0; i < config->obstacleCount; i++) {
        if (snake->body[0].x == config->obstacles[i].x && 
            snake->body[0].y == config->obstacles[i].y) {
            return true;
        }
    }
    
    return false;
}

bool is_position_valid(int x, int y, Snake *snake, GameConfig *config) {
    // Check if position is on the snake
    for (int i = 0; i < snake->length; i++) {
        if (x == snake->body[i].x && y == snake->body[i].y) {
            return false;
        }
    }
    
    // Check if position is on an obstacle
    if (config->hasObstacles) {
        for (int i = 0; i < config->obstacleCount; i++) {
            if (x == config->obstacles[i].x && y == config->obstacles[i].y) {
                return false;
            }
        }
    }
    
    // Check if position is on another food (for multi-fruit mode)
    if (config->multiFruit) {
        for (int i = 0; i < config->foodCount; i++) {
            // Skip the food we're checking for
            if (x == config->foods[i].x && y == config->foods[i].y) {
                return false;
            }
        }
    }
    
    return true;
}

void place_food(Food *food, Snake *snake, GameConfig *config) {
    bool valid_position;
    
    do {
        valid_position = true;
        food->x = rand() % GRID_WIDTH;
        food->y = rand() % GRID_HEIGHT;
        
        // Check if position is on the snake
        for (int i = 0; i < snake->length; i++) {
            if (food->x == snake->body[i].x && food->y == snake->body[i].y) {
                valid_position = false;
                break;
            }
        }
        
        // Check if position is on an obstacle
        if (valid_position && config->hasObstacles) {
            for (int i = 0; i < config->obstacleCount; i++) {
                if (food->x == config->obstacles[i].x && food->y == config->obstacles[i].y) {
                    valid_position = false;
                    break;
                }
            }
        }
        
        // Check if position is on another food
        if (valid_position && config->multiFruit) {
            for (int i = 0; i < config->foodCount; i++) {
                // Skip the current food item being placed
                if (&config->foods[i] == food) continue;
                
                if (food->x == config->foods[i].x && food->y == config->foods[i].y) {
                    valid_position = false;
                    break;
                }
            }
        }
        
    } while (!valid_position);
    
    // Set random direction for moving fruits
    if (food->moving) {
        int dirs[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; // Up, Right, Down, Left
        int dir = rand() % 4;
        food->dx = dirs[dir][0];
        food->dy = dirs[dir][1];
    }
}

void place_obstacles(GameConfig *config, Snake *snake) {
    if (!config->hasObstacles) return;
    
    // Clear existing obstacles
    config->obstacleCount = 0;
    
    // Create appropriate number of obstacles based on game mode
    int numObstacles = (config->mode == CHAOS) ? MAX_OBSTACLES/2 : MAX_OBSTACLES/3;
    
    for (int i = 0; i < numObstacles; i++) {
        bool valid_position;
        int x, y;
        
        do {
            valid_position = true;
            x = rand() % GRID_WIDTH;
            y = rand() % GRID_HEIGHT;
            
            // Check if position is on the snake or too close to snake head
            for (int j = 0; j < snake->length; j++) {
                if ((x == snake->body[j].x && y == snake->body[j].y) ||
                    (abs(x - snake->body[0].x) + abs(y - snake->body[0].y) < 3)) {
                    valid_position = false;
                    break;
                }
            }
            
            // Check if position is on another obstacle
            for (int j = 0; j < config->obstacleCount; j++) {
                if (x == config->obstacles[j].x && y == config->obstacles[j].y) {
                    valid_position = false;
                    break;
                }
            }
            
            // Check if position is on any food
            if (config->multiFruit) {
                for (int j = 0; j < config->foodCount; j++) {
                    if (x == config->foods[j].x && y == config->foods[j].y) {
                        valid_position = false;
                        break;
                    }
                }
            } else {
                if (x == config->foods[0].x && y == config->foods[0].y) {
                    valid_position = false;
                }
            }
            
        } while (!valid_position);
        
        config->obstacles[config->obstacleCount].x = x;
        config->obstacles[config->obstacleCount].y = y;
        config->obstacleCount++;
    }
}

void initialize_multi_fruits(GameConfig *config, Snake *snake) {
    if (!config->multiFruit) return;
    if (snake == NULL) return; // Add safety check
    
    // Set the appropriate number of foods based on game mode
    config->foodCount = (config->mode == CHAOS) ? MAX_FOODS : 3;
    
    // Initialize each food item
    for (int i = 0; i < config->foodCount; i++) {
        // Set different values and types
        switch (i) {
            case 0:
                config->foods[i].value = 10;  // Regular food
                config->foods[i].type = 0;
                break;
            case 1:
                config->foods[i].value = 20;  // Bonus food
                config->foods[i].type = 1;
                break;
            case 2:
                config->foods[i].value = 30;  // Special food
                config->foods[i].type = 2;
                break;
            case 3:
                config->foods[i].value = 50;  // Rare food
                config->foods[i].type = 3;
                break;
            default:
                config->foods[i].value = 10 * (i + 1);
                config->foods[i].type = i % 4;
        }
        
        // Set moving property based on game mode
        config->foods[i].moving = config->movingFruit;
        
        // Place food at valid position
        place_food(&config->foods[i], snake, config);
    }
}

void move_foods(GameConfig *config) {
    if (!config->movingFruit) return;
    
    int foodCount = config->multiFruit ? config->foodCount : 1;
    
    for (int i = 0; i < foodCount; i++) {
        Food *food = &config->foods[i];
        
        if (!food->moving) continue;
        
        // Try to move in current direction
        int newX = food->x + food->dx;
        int newY = food->y + food->dy;
        
        // If it would go off-grid, change direction
        if (newX < 0 || newX >= GRID_WIDTH || newY < 0 || newY >= GRID_HEIGHT) {
            // Choose a new random direction
            int dirs[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; // Up, Right, Down, Left
            int dir;
            
            // Find a valid direction that doesn't lead off-grid
            do {
                dir = rand() % 4;
                food->dx = dirs[dir][0];
                food->dy = dirs[dir][1];
                newX = food->x + food->dx;
                newY = food->y + food->dy;
            } while (newX < 0 || newX >= GRID_WIDTH || newY < 0 || newY >= GRID_HEIGHT);
        }
        
        // Check if new position would overlap with obstacles
        bool collides = false;
        if (config->hasObstacles) {
            for (int j = 0; j < config->obstacleCount; j++) {
                if (newX == config->obstacles[j].x && newY == config->obstacles[j].y) {
                    collides = true;
                    break;
                }
            }
        }
        
        // Check if new position would overlap with other foods
        if (!collides && config->multiFruit) {
            for (int j = 0; j < config->foodCount; j++) {
                if (i == j) continue; // Skip self
                
                if (newX == config->foods[j].x && newY == config->foods[j].y) {
                    collides = true;
                    break;
                }
            }
        }
        
        // Only update position if no collision
        if (!collides) {
            food->x = newX;
            food->y = newY;
        } else {
            // If collision, change direction
            int dirs[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
            int dir = rand() % 4;
            food->dx = dirs[dir][0];
            food->dy = dirs[dir][1];
        }
    }
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

// Draw menu screen with game mode buttons
void draw_menu_screen(SDL_Renderer *renderer, Button buttons[], int buttonCount, TTF_Font *font) {
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
    SDL_RenderClear(renderer);
    
    // Title
    SDL_Color green = {0, 200, 0, 255};
    draw_text_centered(renderer, font, "SNAKE GAME CHALLENGE", 
                     WINDOW_WIDTH / 2, 
                     50, 
                     green);
    
    // Subtitle
    SDL_Color white = {255, 255, 255, 255};
    draw_text_centered(renderer, font, "SELECT GAME MODE:", 
                     WINDOW_WIDTH / 2, 
                     100, 
                     white);
    
    // Draw all buttons
    for (int i = 0; i < buttonCount; i++) {
        draw_button(renderer, &buttons[i], font);
    }
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

// Set up game configuration based on selected mode
void initialize_game_mode(GameConfig *config, GameMode mode, Snake *snake) {
    // Reset all settings
    memset(config, 0, sizeof(GameConfig));
    
    config->mode = mode;
    
    switch (mode) {
        case CLASSIC:
            // Classic mode: No special features
            config->timed = false;
            config->hasObstacles = false;
            config->movingFruit = false;
            config->multiFruit = false;
            
            // Initialize single food
            config->foodCount = 1;
            config->foods[0].value = 10;
            config->foods[0].type = 0;
            config->foods[0].moving = false;
            place_food(&config->foods[0], snake, config);
            break;
            
        case TIMED:
            // Timed mode: Limited time to get as many points as possible
            config->timed = true;
            config->timeRemaining = 60; // 60 seconds
            config->maxTime = 60;
            config->hasObstacles = false;
            config->movingFruit = false;
            config->multiFruit = false;
            
            // Initialize single food
            config->foodCount = 1;
            config->foods[0].value = 10;
            config->foods[0].type = 0;
            config->foods[0].moving = false;
            place_food(&config->foods[0], snake, config);
            break;
            
        case OBSTACLES:
            // Obstacles mode: Static obstacles on the grid
            config->timed = false;
            config->hasObstacles = true;
            config->movingFruit = false;
            config->multiFruit = false;
            
            // Place obstacles
            place_obstacles(config, snake);
            
            // Initialize single food
            config->foodCount = 1;
            config->foods[0].value = 10;
            config->foods[0].type = 0;
            config->foods[0].moving = false;
            place_food(&config->foods[0], snake, config);
            break;
            
        case MOVING_FRUIT:
            // Moving fruit mode: Fruit moves around the grid
            config->timed = false;
            config->hasObstacles = false;
            config->movingFruit = true;
            config->multiFruit = false;
            config->fruitMoveInterval = 500; // Move every 500ms
            
            // Initialize single moving food
            config->foodCount = 1;
            config->foods[0].value = 10;
            config->foods[0].type = 0;
            config->foods[0].moving = true;
            place_food(&config->foods[0], snake, config);
            break;
            
        case MULTI_FRUIT:
            // Multi-fruit mode: Multiple fruits with different values
            config->timed = false;
            config->hasObstacles = false;
            config->movingFruit = false;
            config->multiFruit = true;
            
            // Initialize multiple fruits
            initialize_multi_fruits(config, snake);
            break;
            
        case CHAOS:
            // Chaos mode: Combines all challenges
            config->timed = true;
            config->timeRemaining = 90; // 90 seconds
            config->maxTime = 90;
            config->hasObstacles = true;
            config->movingFruit = true;
            config->multiFruit = true;
            config->fruitMoveInterval = 300; // Move faster than normal
            
            // Place obstacles
            place_obstacles(config, snake);
            
            // Initialize multiple moving fruits
            initialize_multi_fruits(config, snake);
            break;
    }
}

// Update game state
void update_game(Snake *snake, GameConfig *config, int *score, Uint32 currentTime) {
    // Check for obstacle collision
    if (check_obstacle_collision(snake, config)) {
        snake->alive = false;
        return;
    }
    
    // Check for food collision
    if (config->multiFruit) {
        for (int i = 0; i < config->foodCount; i++) {
            if (check_food_collision(snake, &config->foods[i])) {
                // Add score based on food value
                *score += config->foods[i].value;
                
                // Grow the snake
                grow_snake(snake);
                
                // Place new food
                place_food(&config->foods[i], snake, config);
                
                // In timed mode, eating food gives more time
                if (config->timed) {
                    config->timeRemaining += 5; // Add 5 seconds
                    if (config->timeRemaining > config->maxTime) {
                        config->timeRemaining = config->maxTime;
                    }
                }
                
                break;
            }
        }
    } else {
        // Simple single-food logic
        if (check_food_collision(snake, &config->foods[0])) {
            *score += config->foods[0].value;
            grow_snake(snake);
            place_food(&config->foods[0], snake, config);
            
            // In timed mode, eating food gives more time
            if (config->timed) {
                config->timeRemaining += 5; // Add 5 seconds
                if (config->timeRemaining > config->maxTime) {
                    config->timeRemaining = config->maxTime;
                }
            }
        }
    }
    
    // Move food if it's time
    if (config->movingFruit && currentTime - config->lastFruitMove >= config->fruitMoveInterval) {
        move_foods(config);
        config->lastFruitMove = currentTime;
    }
}

// Reset all game variables to start a new game
void reset_game(Snake *snake, GameConfig *config, int *score) {
    // Reset score
    *score = 0;
    
    // Reset snake
    snake->length = 3;
    snake->dx = 1;
    snake->dy = 0;
    snake->alive = true;
    
    // Place snake in the middle of the grid
    snake->body[0].x = GRID_WIDTH / 4;
    snake->body[0].y = GRID_HEIGHT / 2;
    
    for (int i = 1; i < snake->length; i++) {
        snake->body[i].x = snake->body[0].x - i;
        snake->body[i].y = snake->body[0].y;
    }
    
    // Reset current mode
    initialize_game_mode(config, config->mode, snake);
}

int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("Snake Game Challenge", 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         WINDOW_WIDTH, 
                                         WINDOW_HEIGHT, 
                                         SDL_WINDOW_SHOWN);
    
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create font
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if (font == NULL) {
        printf("Font could not be loaded! TTF_Error: %s\n", TTF_GetError());
        // Fall back to default system font if possible
        font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", 24);
        if (font == NULL) {
            // Try another fallback
            font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 24);
            if (font == NULL) {
                printf("Fallback fonts also failed! TTF_Error: %s\n", TTF_GetError());
                return 1;
            }
        }
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    // Initialize game objects
    Snake snake = {0};
    snake.length = 3;
    snake.dx = 1;
    snake.dy = 0;
    snake.alive = true;
    
    // Place snake in the middle of the grid
    snake.body[0].x = GRID_WIDTH / 4;
    snake.body[0].y = GRID_HEIGHT / 2;
    
    for (int i = 1; i < snake.length; i++) {
        snake.body[i].x = snake.body[0].x - i;
        snake.body[i].y = snake.body[0].y;
    }
    
    // Game configuration
    GameConfig config = {0};
    config.mode = CLASSIC; // Default mode
    
    // Generate food
    place_food(&config.foods[0], &snake, &config);
    
    // Create menu buttons
    Button modeButtons[6];
    const char* modeNames[] = {
        "CLASSIC MODE",
        "TIMED MODE",
        "OBSTACLES MODE",
        "MOVING FRUIT MODE",
        "MULTI-FRUIT MODE",
        "CHAOS MODE"
    };
    
    for (int i = 0; i < 6; i++) {
        init_button(&modeButtons[i], 
                  (WINDOW_WIDTH - BUTTON_WIDTH) / 2, 
                  150 + i * (BUTTON_HEIGHT + BUTTON_PADDING), 
                  modeNames[i]);
    }
    
    // Create game over buttons
    Button playAgainButton, exitButton;
    init_button(&playAgainButton, 
              (WINDOW_WIDTH - BUTTON_WIDTH) / 2, 
              WINDOW_HEIGHT / 2, 
              "PLAY AGAIN");
              
    init_button(&exitButton, 
              (WINDOW_WIDTH - BUTTON_WIDTH) / 2, 
              WINDOW_HEIGHT / 2 + BUTTON_HEIGHT + BUTTON_PADDING, 
              "EXIT TO MENU");
    
    // Game variables
    GameState gameState = MENU;
    int score = 0;
    bool quit = false;
    Uint32 lastUpdateTime = 0;
    int updateDelay = 150; // snake speed (lower = faster)
    Uint32 lastSecondTick = 0;
    
    // Event loop
    SDL_Event e;
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        if (snake.dy != 1 && gameState == PLAYING) { // Prevent 180-degree turns
                            snake.dx = 0;
                            snake.dy = -1;
                        }
                        break;
                    case SDLK_DOWN:
                        if (snake.dy != -1 && gameState == PLAYING) {
                            snake.dx = 0;
                            snake.dy = 1;
                        }
                        break;
                    case SDLK_LEFT:
                        if (snake.dx != 1 && gameState == PLAYING) {
                            snake.dx = -1;
                            snake.dy = 0;
                        }
                        break;
                    case SDLK_RIGHT:
                        if (snake.dx != -1 && gameState == PLAYING) {
                            snake.dx = 1;
                            snake.dy = 0;
                        }
                        break;
                    case SDLK_ESCAPE:
                        if (gameState == PLAYING) {
                            gameState = MENU;
                        } else if (gameState == MENU) {
                            quit = true;
                        }
                        break;
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                // Check button hover states
                int mouseX = e.motion.x;
                int mouseY = e.motion.y;
                
                if (gameState == MENU) {
                    for (int i = 0; i < 6; i++) {
                        modeButtons[i].hover = is_point_in_rect(mouseX, mouseY, &modeButtons[i].rect);
                    }
                } else if (gameState == GAME_OVER) {
                    playAgainButton.hover = is_point_in_rect(mouseX, mouseY, &playAgainButton.rect);
                    exitButton.hover = is_point_in_rect(mouseX, mouseY, &exitButton.rect);
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    
                    if (gameState == MENU) {
                        for (int i = 0; i < 6; i++) {
                            if (is_point_in_rect(mouseX, mouseY, &modeButtons[i].rect)) {
                                // Start game with selected mode
                                config.mode = (GameMode)i;
                                initialize_game_mode(&config, config.mode, &snake);
                                reset_game(&snake, &config, &score);
                                gameState = PLAYING;
                                lastUpdateTime = SDL_GetTicks();
                                lastSecondTick = lastUpdateTime;
                                break;
                            }
                        }
                    } else if (gameState == GAME_OVER) {
                        if (is_point_in_rect(mouseX, mouseY, &playAgainButton.rect)) {
                            // Restart game with same mode
                            reset_game(&snake, &config, &score);
                            gameState = PLAYING;
                            lastUpdateTime = SDL_GetTicks();
                            lastSecondTick = lastUpdateTime;
                        } else if (is_point_in_rect(mouseX, mouseY, &exitButton.rect)) {
                            gameState = MENU;
                        }
                    }
                }
            }
        }
        
        // Get current time
        Uint32 currentTime = SDL_GetTicks();
        
        // Update timer for timed mode
        if (gameState == PLAYING && config.timed) {
            if (currentTime - lastSecondTick >= 1000) { // 1 second has passed
                config.timeRemaining--;
                lastSecondTick = currentTime;
                
                // Check if time is up
                if (config.timeRemaining <= 0) {
                    snake.alive = false;
                }
            }
        }
        
        // Game update logic
        if (gameState == PLAYING && currentTime - lastUpdateTime >= updateDelay) {
            move_snake(&snake);
            
            if (snake.alive) {
                update_game(&snake, &config, &score, currentTime);
            } else {
                gameState = GAME_OVER;
            }
            
            lastUpdateTime = currentTime;
        }
        
        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        switch (gameState) {
            case MENU:
                draw_menu_screen(renderer, modeButtons, 6, font);
                break;
                
            case PLAYING:
                // Draw UI (includes score)
                draw_ui_area(renderer, score, &config, font);
                
                // Draw grid
                draw_grid(renderer);
                
                // Draw obstacles
                draw_obstacles(renderer, &config);
                
                // Draw food(s)
                if (config.multiFruit) {
                    for (int i = 0; i < config.foodCount; i++) {
                        draw_food(renderer, &config.foods[i]);
                    }
                } else {
                    draw_food(renderer, &config.foods[0]);
                }
                
                // Draw snake
                draw_snake(renderer, &snake);
                break;
                
            case GAME_OVER:
                // First draw the game state behind the overlay
                draw_ui_area(renderer, score, &config, font);
                draw_grid(renderer);
                draw_obstacles(renderer, &config);
                
                if (config.multiFruit) {
                    for (int i = 0; i < config.foodCount; i++) {
                        draw_food(renderer, &config.foods[i]);
                    }
                } else {
                    draw_food(renderer, &config.foods[0]);
                }
                
                draw_snake(renderer, &snake);
                
                // Then draw the game over screen
                draw_game_over_screen(renderer, score, &playAgainButton, &exitButton, font);
                break;
        }
        
        // Update screen
        SDL_RenderPresent(renderer);
    }
    
    // Clean up
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}