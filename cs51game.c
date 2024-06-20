#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for usleep
#include <stdbool.h>

// RGB565 color definitions
#define RED     0xF800
#define YELLOW  0xFFE0
#define GREEN   0x07E0
#define BLUE    0x001F
#define WHITE   0xFFFF	
#define SHOOTER_SPEED 100
#define SW_BASE  0xFF200040
#define FPGA_CHAR_BASE 0x09000000
#define FPGA_CHAR_END  0x09001FFF

volatile int * KEY_ptr = (int *) 0xFF200050;
volatile int * SW_ptr = (int *) 0xFF200040;

typedef struct {
    int x;
    bool moving_left;
    bool moving_right;
} SHOOTER;

SHOOTER shooter;

int shooter_width = 65;
int shooter_height = 10;
int shooter_y = 230; // Y-coordinate of the shooter

// Function to set a single pixel on the screen at x, y with the specified color
void write_pixel(int x, int y, int colour)
{
    volatile short *_vga_addr = (volatile short *)(0x08000000 + (y << 10) + (x << 1));
    *_vga_addr = colour;
}

void clear_screen()
{
    int x, y;
    for (x = 0; x < 320; x++)
    {
        for (y = 0; y < 240; y++)
        {
            write_pixel(x, y, 0);
        }
    }
}

void clear_screen2(){
    // Clear pixel data
    for(int x = 0; x < 320 ; x++) {
        for(int y = 0; y < 240 ; y++) {
            write_pixel(x, y, 0);
        }
    }
    // Clear text area
    for(int x = 0; x < 80 ; x++) {
        for(int y = 0; y < 60 ; y++) {    
            volatile char * char_buffer = (char*)(0x09000000 + (y << 7) + x);
            *char_buffer = 0; // Clear character
        }
    }
}

// Function to clear the area where the square was
void clear_square(int square_x, int square_y, int square_size)
{
    for (int i = 0; i < square_size; i++)
    {
        for (int j = 0; j < square_size; j++)
        {
            write_pixel(square_x + i, square_y + j, 0);
        }
    }
}

// Function to draw the shooter at its current position
void draw_shooter(int shooter_x, int shooter_y, int shooter_width)
{
    for (int i = 0; i < shooter_width; i++)
    {
		for(int j = 0; j < shooter_height; j++){
            write_pixel(shooter_x + i, shooter_y + j, WHITE); // Draw the shooter as a horizontal line
        }
    }
}

void update_shooter_position(SHOOTER *shooter) {
    if (shooter->moving_left && shooter->x > 0) {
        shooter->x -= SHOOTER_SPEED;
        if (shooter->x < 0) {
            shooter->x = 0; // Ensure shooter doesn't go beyond left edge
            shooter->moving_left = false; // Reverse direction
            shooter->moving_right = true;
        }
    }
    if (shooter->moving_right && shooter->x + shooter_width < 320) {
        shooter->x += SHOOTER_SPEED;
        if (shooter->x + shooter_width >= 320) {
            shooter->x = 320 - shooter_width; // Ensure shooter doesn't go beyond right edge
            shooter->moving_right = false; // Reverse direction
            shooter->moving_left = true;
        }
    }
}

void delay(int iterations) {
    for (int i = 0; i < iterations; i++) {
        // Introduce delay by consuming CPU cycles
        for (int j = 0; j < 10000; j++) {
            asm(""); // This line ensures that the loop isn't optimized away by the compiler
        }
    }
}

void write_char(int x, int y, char c) {
    volatile char *character_buffer = (volatile char *)(0x09000000 + (y << 7) + x);
    *character_buffer = c;
}

bool check_condition(int x1, int y1, int width1, int x2, int y2, int width2) {
    return (x1 < x2 + width2) && (x1 + width1 > x2) && (y1 == y2);
}

void game(){
    srand(time(NULL)); // Seed random number generator

    // Array of possible colors
    int colors[] = {RED, YELLOW, GREEN, BLUE};
    int miss_count = 0;
    // Loop for creating squares falling from random positions at random timings
    while (1)
    {
        clear_screen(); // Clear the screen before drawing each square
        int chosen_color_index = rand() % 4;
        int chosen_color = colors[chosen_color_index];

        int square_size = 30; // Random size of the square between 20 and 29 pixels
        int square_x = rand() % (320 - square_size); // Random x-coordinate of the square
        int square_y = 0; // Start from the top of the screen
        int dy = 1; // Default speed

        while(square_y < 230 - square_size){
            clear_square(square_x, square_y, square_size); // Clear the previous square
            square_y += dy; // Move the square
            for (int i = 0; i < square_size; i++) // Draw the new square
            {
                for (int j = 0; j < square_size; j++)
                {
                    write_pixel(square_x + i, square_y + j, chosen_color);
                }
            }
        }

        // Read the switch values
        int switch_value = *SW_ptr;

        // Update shooter movements based on switch values
        shooter.moving_left = (switch_value & 0x1) ? true : false;
        shooter.moving_right = (switch_value & 0x2) ? true : false;

        update_shooter_position(&shooter);

        draw_shooter(shooter.x, shooter_y, shooter_width); // Draw the shooter
        delay(500);

        if ((chosen_color == RED && check_condition(square_x, square_y, square_size, shooter.x, shooter_y, shooter_width)) || (chosen_color != RED && square_y >= 230 - square_size)) {
            miss_count++;
        }

        if (miss_count == 3) {
            break; // Exit the game loop
        }
    }
    end(miss_count); // Display game over message and wait for restart
}

void end(int p){
    clear_screen();
    char* game_over_msg = "GAME OVER";
    int  x = 30; // Center the message horizontally
    while (*game_over_msg) {
        write_char(x, 20, *game_over_msg);
        x++; // Move to the next character position
        game_over_msg++;
    }

    char* restart_message = "PRESS PUSH BUTTON 0 TO RESTART";
    int  a = 30; // Center the message horizontally
    while (*restart_message) {
        write_char(a, 30, *restart_message);
        a++; // Move to the next character position
        restart_message++;
    }

    while(1){
        if (*KEY_ptr != 0){
            while(*KEY_ptr != 0);
			clear_screen2();
			game();
        }
    }
}

int main()	
{
    while (1) {
        clear_screen2();
        game();
    }
    return 0;
}
