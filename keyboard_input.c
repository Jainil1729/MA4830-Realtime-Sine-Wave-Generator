// gcc -o kb keyboard_input.c -l ncurses

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

#define INTEGER 0
#define FLOAT 1

#define FREQUENCY_MIN 0.0
#define FREQUENCY_MAX 100.0
#define FREQUENCY_STEPS 100
#define FREQUENCY_STEP_SIZE (FREQUENCY_MAX-FREQUENCY_MIN)/FREQUENCY_STEPS 
#define MEAN_MIN 1.0
#define MEAN_MAX 10.0
#define MEAN_STEPS 100
#define MEAN_STEP_SIZE (MEAN_MAX-MEAN_MIN)/MEAN_STEPS 
#define AMPLITUDE_MIN 0
#define AMPLITUDE_MAX 4095
#define AMPLITUDE_STEPS 100
#define AMPLITUDE_STEP_SIZE (AMPLITUDE_MAX-AMPLITUDE_MIN)/AMPLITUDE_STEPS 

float mean = 1.0;
float frequency = 0.0;
unsigned int amplitude = 0;
char* waveform_options[] = {"sine", "square", "triangular", "sawtooth"};
int current_waveform = 0;
const unsigned int len_waveform = sizeof(waveform_options)/sizeof(waveform_options[0]);

// Function Prototypes
void constrain(void* var_pointer, float min, float max, int format);
void print_keyboard_usage();

int main() {
    int ch;

    /* Curses Initialisations */
    initscr();
    // raw();
    keypad(stdscr, TRUE);
    noecho();

    print_keyboard_usage();
    while((ch = getch()) != 'E') {
        clear();
        print_keyboard_usage();
        switch(ch) {
            case KEY_UP:
                amplitude += AMPLITUDE_STEP_SIZE;
                break;
            case KEY_DOWN:
                amplitude -= AMPLITUDE_STEP_SIZE;
                break;
            case KEY_RIGHT:
                frequency += FREQUENCY_STEP_SIZE;
                break;
            case KEY_LEFT:
                frequency -= FREQUENCY_STEP_SIZE;
                break;
            case 'w':
            case 'W':
                mean += MEAN_STEP_SIZE;
                break;
            case 's':
            case 'S':
                mean -= MEAN_STEP_SIZE;
                break;
            case 'a':
            case 'A':
                current_waveform -= 1;
                break;
            case 'd':
            case 'D':
                current_waveform += 1;
                break;
            // default:    
            //     printw("\nThe pressed key is %c",ch);
        }
        constrain(&frequency, FREQUENCY_MIN, FREQUENCY_MAX, FLOAT);
        constrain(&mean, MEAN_MIN, MEAN_MAX, FLOAT);
        constrain(&amplitude, AMPLITUDE_MIN, AMPLITUDE_MAX, INTEGER);
        constrain(&current_waveform, 0, len_waveform - 1, INTEGER);
    }
    endwin();

    return EXIT_SUCCESS;
}

void constrain(void* var_pointer, float min, float max, int format) {
    if (format == INTEGER) {
        int* var_pointer_2 = (int*)var_pointer;
        if (*var_pointer_2 < (int) min) {
            *var_pointer_2 = (int) min;
        }
        else if (*var_pointer_2 > (int) max) {
            *var_pointer_2 = (int) max;
        }
    }
    else if (format == FLOAT) {
        float* var_pointer_2 = (float*)var_pointer;
        if (*var_pointer_2 < min) {
            *var_pointer_2 = min;
        }
        else if (*var_pointer_2 > max) {
            *var_pointer_2 = max;
        }
    }
    else {
        printf("Unsupported format\n\n");
    }
}

void print_keyboard_usage() {
    printw("Press E to Exit\n\n");
    printw("Up/Down: Change Amplitude\n");
    printw("Left/Right: Change Frequency\n");
    printw("W/S: Change Mean\n");
    printw("A/D: Change Waveform\n");
    printw("\nFrequency: %f",frequency);
    printw("\nMean: %f",mean);
    printw("\nAmplitude: %d",amplitude);
    printw("\nWaveform: %s",waveform_options[current_waveform]);
}