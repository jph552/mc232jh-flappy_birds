/*   Point_Control_Via_Joystick
*    ==========================
*
*    Function:               Creates the Flappy Bird game on an LCD, controlled by an external joystick
*    Circuit Schematic No.:  9        : https://github.com/ELECXJEL2645/Circuit_Schematics
*    Required Libraries:     Joystick : https://github.com/ELECXJEL2645/Joystick
*                            N5110    : https://github.com/ELECXJEL2645/N5110
*
*    Authored by:            Joseph Horlock
*    Date:                   2024
*    Version:                1.0
*    MBED Studio Version:    1.4.1
*    MBED OS Version:        6.12.0
*    Board:	                 NUCLEO L476RG  */

#include "mbed.h"
#include <ctime> // For time-based seed
#include "Joystick.h"
#include "N5110.h"
#include <list>
#include <vector>
#include <set>
#include <cstdlib> // for rand()

// Initialization of random seed
void init_random_seed() {
    srand(time(NULL)); // Seed the random generator with the current time
}

using namespace std;

// LCD and Joystick initialization
N5110 lcd(PC_7, PA_9, PB_10, PB_5, PB_3, PA_10);
Joystick joystick(PC_2, PC_3); // Joystick (Y-axis, X-axis)

DigitalIn buttonJoystick(PC_1, PullUp); // Joystick button
DigitalIn buttonFree(PC_0, PullUp); // Free button

// Bird initial position and score variable
float xBirdPosition = 42;
float yBirdPosition = 24;
int score = 0; // Score variable

// Boundary limits for the screen
const int LCD_WIDTH = 84;
const int LCD_HEIGHT = 48;

// Bird sprites for different motions
const int BirdUp[11][10] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
};

const int BirdDown[11][10] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 0, 1, 0, 0},
    {1, 0, 1, 0, 1, 0, 0, 1, 0, 0},
    {1, 0, 1, 1, 1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
};

// Tube list and tube heights
list<float> tubeSections; // Holds tube x-positions
vector<int> tubeHeights = {20, 25, 15, 30}; // Tube heights
std::set<float> passedTubes; // Track which tubes have been passed
float tubeInterval = 20.0f; // Distance between new tubes
float tubeSpeed = 1.5f; // Speed of tube movement

// Initialize LEDs
void init_leds() {
    DigitalOut led1(LED1); // Example LED
    led1.write(0); // Turn off by default
}

// Boundary check function to keep the bird within the screen
void boundary() {
    if (xBirdPosition < 1) xBirdPosition = 1;
    if (xBirdPosition > LCD_WIDTH - 3) xBirdPosition = LCD_WIDTH - 3;
    if (yBirdPosition < 1) yBirdPosition = 1;
    if (yBirdPosition > LCD_HEIGHT - 3) yBirdPosition = LCD_HEIGHT - 3;
}

// Function to update tube positions and manage new tubes
void updateTubes(float speed) {
    list<float> updatedSections;

    // Move existing tubes
    for (auto section : tubeSections) {
        float newPos = section - speed;
        if (newPos > 0) {
            updatedSections.push_back(newPos); // Keep tubes on-screen
        }
    }

    // Add new tubes if needed
    if (updatedSections.empty() || updatedSections.back() < (LCD_WIDTH - tubeInterval)) {
        updatedSections.push_back(LCD_WIDTH + 10); // Start new tube at the far right

        // Generate a new random height for each new tube
        int minTubeHeight = 10; // Minimum tube height to avoid too narrow passages
        int maxTubeHeight = LCD_HEIGHT - 16; // Maximum height considering screen constraints
        int newHeight = minTubeHeight + rand() % (maxTubeHeight - minTubeHeight);

        tubeHeights.push_back(newHeight);
    }

    tubeSections = updatedSections;
}



// Draw the tubes on the LCD
void drawTubes() {
    int tubeWidth = 5;
    int sectionIndex = 0;

    for (auto section : tubeSections) {
        if (section > 0) {
             auto heightIndex = static_cast<std::vector<int>::size_type>(sectionIndex % tubeHeights.size());

            // Draw lower tube
            lcd.drawRect(
                section,
                LCD_HEIGHT - tubeHeights[heightIndex] - 1,
                tubeWidth,
                tubeHeights[heightIndex],
                FILL_BLACK
            );

            // Draw upper tube
            lcd.drawRect(
                section,
                0,
                tubeWidth,
                LCD_HEIGHT - tubeHeights[heightIndex] - 16,
                FILL_BLACK
            );
        }
    }
}

// Collision detection function
bool checkCollision() {
    int tubeWidth = 5;
    int sectionIndex = 0;

    for (auto section : tubeSections) {
        if (section > 0) {
            auto heightIndex = static_cast<std::vector<int>::size_type>(sectionIndex % tubeHeights.size());

            // Check if the bird collides with lower tube
            if (xBirdPosition >= section and xBirdPosition <= section + tubeWidth and
                yBirdPosition >= LCD_HEIGHT - tubeHeights[heightIndex] - 1) {
                return true;
            }

            // Check if the bird collides with upper tube
            if (xBirdPosition >= section and xBirdPosition <= section + tubeWidth and
                yBirdPosition <= LCD_HEIGHT - tubeHeights[heightIndex] - 16) {
                return true;
            }
        }
    }

    return false; // No collision detected
}

// Function to check if the bird passes through a tube and update the score
bool checkScoreUpdate(const list<float>& tubeSections, std::set<float>& passedTubes, float birdXPosition) {
    int tubeWidth = 5; // Width of the tube
    bool scoreUpdated = false;

    // Debug output for tube sections and bird position
    printf("Bird X Position: %f\n", birdXPosition);
    printf("Tube Sections:\n");
    for (auto section : tubeSections) {
        printf("  %f\n", section);
    }

    for (auto section : tubeSections) {
        if (section <= 0) continue; // Skip invalid sections

        // If the tube has already been passed, skip it
        if (passedTubes.find(section) != passedTubes.end()) {
            continue;
        }

        // If the bird's x-position has passed the tube's trailing edge
        if (birdXPosition > section + tubeWidth) {
            passedTubes.insert(section); // Mark this tube as passed
            scoreUpdated = true; // Signal that the score should be updated
        }
    }

    return scoreUpdated;
}


// Main game loop
void gameLoop() {
    // Initial tube positions and bird setup
    tubeSections = {LCD_WIDTH + 10, LCD_WIDTH + 30, LCD_WIDTH + 50}; 
    xBirdPosition = 42;
    yBirdPosition = 24;
    score = 0;

    std::set<float> passedTubes; 
    bool isMovingUp = false;

    while (true) {
        // Check if the joystick button is pressed to exit the game
        if (!buttonJoystick.read()) {
            break; // Exit the game loop if the joystick button is pressed
        }

        lcd.clear(); 
        lcd.drawRect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, FILL_TRANSPARENT);

        updateTubes(tubeSpeed);
        drawTubes(); 


        // Move the bird based on joystick direction
        Direction direction = static_cast<Direction>(joystick.get_direction());
        switch (direction) {
            case N: 
                yBirdPosition--; 
                isMovingUp = true;
                break;
            case S: 
                yBirdPosition++; 
                isMovingUp = false;
                break;
            case E: 
                xBirdPosition++;
                break;
            case W: 
                xBirdPosition--;
                break;
            case NE: 
                xBirdPosition++; 
                yBirdPosition--; 
                isMovingUp = true;
                break;
            case NW: 
                xBirdPosition--; 
                yBirdPosition--; 
                isMovingUp = true;
                break;
            case SE: 
                xBirdPosition++; 
                yBirdPosition++; 
                isMovingUp = false;
                break;
            case SW: 
                xBirdPosition--; 
                yBirdPosition++; 
                isMovingUp = false;
                break;
            default: 
                break;
        }

        // Keep the bird within the screen boundaries
        boundary();

        // Draw the bird sprite based on its movement direction
        if (isMovingUp) {
            lcd.drawSprite(xBirdPosition, yBirdPosition, 11, 10, (int*)BirdUp);
        } else {
            lcd.drawSprite(xBirdPosition, yBirdPosition, 11, 10, (int*)BirdDown);
        }

        // Check for collisions with tubes
        if (checkCollision()) {
            return; // Exit the game loop if there's a collision
        }

        // Update the score if the bird has passed a tube
        if (checkScoreUpdate(tubeSections, passedTubes, xBirdPosition)) {
            score++; // Increment score
        }

        // Display the score
        char scoreBuffer[16]; 
        sprintf(scoreBuffer, "%d", score);
        lcd.drawRect(0, 0, 48, 9, FILL_TRANSPARENT); 
        lcd.printString("Score: ", 0, 0); 
        lcd.printString(scoreBuffer, 36, 0); 

        lcd.refresh(); 
        ThisThread::sleep_for(50ms); 
    }
}

int main() {
    joystick.init();
    init_leds(); 
    lcd.init(LPH7366_1); 
    lcd.setContrast(0.35); 
    lcd.setBrightness(0.75); 
    
    init_random_seed(); // Initialize the random seed

    while (true) {
        lcd.clear(); 
        lcd.printString("Press the free button to Start", 42, 24); 
        lcd.refresh(); 
        
        // Wait for the free button to be pressed to start the game
        while (buttonFree.read()) {
            ThisThread::sleep_for(100ms); // Poll the free button
        }

        gameLoop(); // Start the game loop

        lcd.clear(); 
        lcd.printString("Game Over", 15, 20); 
        lcd.refresh(); 
        ThisThread::sleep_for(2000ms); // Show "Game Over" message for a while
    }
}

