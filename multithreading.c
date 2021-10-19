// gcc -o mt multithreading.c -lncurses -lpthread

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <ncurses.h>

#define MAX_INPUT 20
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

float frequency;
float mean;
unsigned int amplitude;
char* waveform_options[] = {"sine", "square", "triangular", "sawtooth"};
int current_waveform = -1;
const unsigned int len_waveform = sizeof(waveform_options)/sizeof(waveform_options[0]);

// Multithreading variables
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shutdown_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  shutdown_cond  = PTHREAD_COND_INITIALIZER;

// Function Prototypes
void usage(char* progname);
bool input(char* var_pointer);
bool convertNum(char* input, void* var_pointer, int format, float min, float max);
int promptInt(char* msg, int min, int max);
float promptFloat(char* msg, float min, float max);
void* get_keyboard_input();
void constrain(void* var_pointer, float min, float max, int format);
void print_keyboard_usage();

void* shutdown() {
    // For demonstration of shutdown of all threads using conditional variable
    sleep(10);
    
    // Signal main thread to shutdown all threads and exit program
    pthread_mutex_lock(&shutdown_mutex);
    pthread_cond_signal(&shutdown_cond);
    pthread_mutex_unlock(&shutdown_mutex);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int opt;
    bool f_opt = false, m_opt = false, a_opt = false, s_opt = false, w_opt = false;
    while ((opt = getopt(argc, argv, ":f:m:a:w:s")) != -1) {
        char arg[MAX_INPUT] = "";
        switch (opt) {
        case 'f':
            f_opt = true;
            // printf("Argument to f is %s\n", optarg);
            if (!convertNum(optarg, &frequency, FLOAT, FREQUENCY_MIN, FREQUENCY_MAX)) {
                usage(argv[0]);
            }
            break;
        case 'm':
            m_opt = true;
            // printf("Argument to m is %s\n", optarg);
            if (!convertNum(optarg, &mean, FLOAT, MEAN_MIN, MEAN_MAX)) {
                usage(argv[0]);
            }
            break;
        case 'a':
            a_opt = true;
            // printf("Argument to a is %s\n", optarg);
            if (!convertNum(optarg, &amplitude, INTEGER, AMPLITUDE_MIN, AMPLITUDE_MAX)) {
                usage(argv[0]);
            }
            break;
        case 'w':
            w_opt = true;
            
            // Convert string to lowercase
            for(int i = 0; optarg[i]; i++) {
                optarg[i] = tolower(optarg[i]);
            }
            
            // Match option argument to waveform options
            for (int i; i<len_waveform; i++) {
                if (strcmp(optarg, waveform_options[i]) == 0) {
                    current_waveform = i;
                    break;
                }
            }
            
            if (current_waveform == -1) {
                printf("Undefined Waveform\n");
                usage(argv[0]);
            }
            break;
        case 's':
            // Not Implemented Yet
            s_opt = true;
            printf("Argument to s is %s\n", optarg);
            break;
        case '?':
            printf("Unknown option: %c\n", optopt);
            usage(argv[0]);
            break;
        case ':':
            printf("Missing arg for %c\n", optopt);
            usage(argv[0]);
            break;
        default:
            usage(argv[0]);
        }
    }
    
    // Prompt user for input
    if (!f_opt) frequency = promptFloat("Input Frequency: ", FREQUENCY_MIN, FREQUENCY_MAX);
    if (!m_opt) mean = promptFloat("Input Mean: ", MEAN_MIN, MEAN_MAX);
    if (!a_opt) amplitude = promptInt("Input Amplitude: ", AMPLITUDE_MIN, AMPLITUDE_MAX);
    if (!w_opt) {   
        
        // TODO: Use malloc
        char str1[100] = "Select Waveform: \n";
        for (int i=0; i<len_waveform; i++) {
            char str2[20] = "";
            sprintf(str2, "%d) %s\n", i, waveform_options[i]);
            strcat(str1, str2);
        }
        current_waveform = promptInt(str1, 0, len_waveform-1);
    }
    
    /* Curses Initialisations */
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    print_keyboard_usage();
    pthread_t kb_thread, shutdown_thread;
    pthread_create( &kb_thread, NULL, get_keyboard_input, NULL);
    pthread_create( &shutdown_thread, NULL, shutdown, NULL);

    // Wait for shutdown condition
    pthread_mutex_lock(&shutdown_mutex);
    pthread_cond_wait(&shutdown_cond, &shutdown_mutex);
    pthread_mutex_unlock(&shutdown_mutex);

    // Shutdown all threads
    pthread_cancel(kb_thread); 
    pthread_cancel(shutdown_thread);

    endwin();
    printf("Ending Program.\n");
    return EXIT_SUCCESS;
}

void usage(char* progname) {
    printf("Usage: %s [-f frequency] [-m mean] [-a amplitude] [-w waveform] [-s setting_file] [-h]\n", progname);
	printf("[-s setting_file]: (file) Default setting for frequency, mean and amplitude. Overwritten when corresponding option is used during program call.\n");
	printf("[-f frequency]: (float) Frequency of wave (Hz). Range: %f - %f\n", FREQUENCY_MIN, FREQUENCY_MAX);
	printf("[-m mean]: (float) Prescaled mean of wave. Range: %f - %f\n", MEAN_MIN, MEAN_MAX);
	printf("[-a amplitude]: (unsigned int) Amplitude of wave. Range: %d - %d\n", AMPLITUDE_MIN, AMPLITUDE_MAX);
    printf("[-w waveform]: (string) Type of waveform. Options: sine, square, triangular, sawtooth.\n");
    printf("[-h]: Display this information..\n");
    exit(EXIT_FAILURE);
}

bool input(char* var_pointer) {
    /*
    Prompts for keyboard inputs from user.
    Stores input within a given address in string format.
    
    Parameters:
        var_pointer: address to store user input in

    Returns:
        true when input is sucessful, else false
    */
    char value[MAX_INPUT] = "";
    char c;

    // Read input from stdin as string
    fgets(value, MAX_INPUT, stdin);

    // If last character in value is not \n, input buffer is not cleared.
    if (!strchr(value, '\n')) {
        printf("Exceed maximum input size\n\n");
        
        // Clear Input buffer
        while ((c = getchar()) != '\n' && c != EOF);
        return false;
    }

    sscanf(value, "%s", var_pointer);
    return true;
}

bool convertNum(char* input, void* var_pointer, int format, float min, float max) {
    /*
    Converts input into desired format and stores value within a given address.
    Accepts INTEGER or FLOAT format.

    Parameters:
        input: string to convert to number format
        var_pointer: address to store converted input in
        format: format for conversion of input
        min: minimum value to accept
        max: maximum value to accept

    Returns:
        true when input is in correct format, else false
    */
    char format_type[20] = "";
    char c;
    
    if (format == INTEGER) {
        int temp;
        int* var_pointer_2 = (int*)var_pointer;
        sprintf(format_type, "integer");
        if (sscanf(input, "%d %c", &temp, &c) == 1) {
            if (temp < (int) min) {
                printf("%s is less than minimum number %d\n\n", input, (int) min);
                return false;
            }
            else if (temp > (int) max) {
                printf("%s is greater than maximum number %d\n\n", input, (int) max);
                return false;
            }
            *var_pointer_2 = temp;
            return true;
        }
    }
    else if (format == FLOAT) {
        float temp;
        float* var_pointer_2 = (float*)var_pointer;
        sprintf(format_type, "float");
        if (sscanf(input, "%f %c", &temp, &c) == 1) {
            if (temp < min) {
                printf("%s is less than minimum number %f\n\n", input, min);
                return false;
            }
            else if (temp > max) {
                printf("%s is greater than maximum number %f\n\n", input, max);
                return false;
            }
            *var_pointer_2 = temp;
            return true;
        }
    }
    else {
        printf("Unsupported format\n\n");
        return false;
    }

    printf("Invalid format. %s is not %s\n\n", input, format_type);
    return false;
}

int promptInt(char* msg, int min, int max) {
    /*
    Prompts for int input from user.
    Parameters:
        msg: message to display to user during the prompt
        min: minimum value to accept
        max: maximum value to accept

    Returns:
        int input by user
    */
    int result;
    char input_str[MAX_INPUT] = "";
    while (true) {
        printf("%s", msg);
        if (input(input_str)) {
            // printf("%s\n", input_str);
            if (convertNum(input_str, &result, INTEGER, min, max)) {
                break;
            }
        }
    }
    return result;
}

float promptFloat(char* msg, float min, float max) {
    /*
    Prompts for float input from user.
    Parameters:
        msg: message to display to user during the prompt
        min: minimum value to accept
        max: maximum value to accept

    Returns:
        float input by user
    */
    float result;
    char input_str[MAX_INPUT] = "";
    while (true) {
        printf("%s", msg);
        if (input(input_str)) {
            if (convertNum(input_str, &result, FLOAT, min, max)) {
                break;
            }
        }
    }
    return result;
}

void* get_keyboard_input() {
    int ch;
    while((ch = getch()) != 'E') {
        clear();
        print_keyboard_usage();
        pthread_mutex_lock(&global_mutex);
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
        constrain(&current_waveform, 0, len_waveform-1, INTEGER);
        pthread_mutex_unlock(&global_mutex);
    }

    // Signal main thread to shutdown all threads and exit program
    pthread_mutex_lock(&shutdown_mutex);
    pthread_cond_signal(&shutdown_cond);
    pthread_mutex_unlock(&shutdown_mutex);
    pthread_exit(NULL);
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
    pthread_mutex_lock(&global_mutex);
    printw("Press E to Exit\n\n");
    printw("Up/Down: Change Amplitude\n");
    printw("Left/Right: Change Frequency\n");
    printw("W/S: Change Mean\n");
    printw("A/D: Change Waveform\n");
    printw("\nFrequency: %f",frequency);
    printw("\nMean: %f",mean);
    printw("\nAmplitude: %d",amplitude);
    printw("\nWaveform: %s",waveform_options[current_waveform]);
    pthread_mutex_unlock(&global_mutex);
}