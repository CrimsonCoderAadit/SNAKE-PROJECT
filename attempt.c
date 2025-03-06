#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

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

// Function to draw the score in the top-left corner
void draw_score(SDL_Renderer *renderer, int score) {
    // Background for score display
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect score_bg = {SCORE_PADDING, SCORE_PADDING, 
                         (SCORE_DIGIT_WIDTH + SCORE_PADDING) * 8, 
                         SCORE_DIGIT_HEIGHT + SCORE_PADDING * 2};
    SDL_RenderFillRect(renderer, &score_bg);
    
    // Set color for score text
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    // Draw "SCORE: " text (using a fixed-width approach)
    int x_pos = SCORE_PADDING * 2;
    
    // Convert score number to digits
    int temp_score = score;
    int digits[5] = {0}; // Maximum 5 digits (up to 99999)
    int digit_count = 0;
    
    // Handle score of 0 specially
    if (score == 0) {
        draw_digit(renderer, x_pos, SCORE_PADDING * 2, 0, 
                  SCORE_DIGIT_WIDTH, SCORE_DIGIT_HEIGHT, SCORE_SEGMENT_THICKNESS);
        digit_count = 1;
    } else {
        // Extract digits from score
        while (temp_score > 0 && digit_count < 5) {
            digits[digit_count++] = temp_score % 10;
            temp_score /= 10;
        }
    }
    
    // Draw digits from right to left
    for (int i = digit_count - 1; i >= 0; i--) {
        draw_digit(renderer, x_pos, SCORE_PADDING * 2, digits[i], 
                  SCORE_DIGIT_WIDTH, SCORE_DIGIT_HEIGHT, SCORE_SEGMENT_THICKNESS);
        x_pos += SCORE_DIGIT_WIDTH + SCORE_PADDING;
    }
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

void draw_game_over(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Game over message could be improved with text rendering
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    
    // Draw "GAME OVER" text (simplified representation)
    // For a proper text, SDL_ttf should be used
    int centerX = WINDOW_WIDTH / 2;
    int centerY = WINDOW_HEIGHT / 2;
    
    // Draw a red rectangle to indicate game over
    SDL_Rect gameOverRect = {centerX - 100, centerY - 30, 200, 60};
    SDL_RenderFillRect(renderer, &gameOverRect);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize random number generator
    srand(time(NULL));
    
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

    int running = 1;
    SDL_Event event;
    int score = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
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
                    case SDLK_r:
                        if (!snake.alive) {
                            // Reset game
                            snake.length = 5;
                            snake.dx = 1;
                            snake.dy = 0;
                            snake.alive = true;
                            for (int i = 0; i < snake.length; i++) {
                                snake.body[i].x = 5 - i;
                                snake.body[i].y = 5;
                            }
                            place_food(&food, &snake);
                            score = 0;
                        }
                        break;
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                }
            }
        }

        if (snake.alive) {
            move_snake(&snake);
            
            // Check food collision
            if (check_food_collision(&snake, &food)) {
                grow_snake(&snake);
                place_food(&food, &snake);
                score += 10;
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw game elements
        draw_grid(renderer);
        draw_snake(renderer, &snake);
        draw_food(renderer, &food);
        
        // Draw score
        draw_score(renderer, score);
        
        // Draw game over screen if snake is dead
        if (!snake.alive) {
            draw_game_over(renderer);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(150);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}