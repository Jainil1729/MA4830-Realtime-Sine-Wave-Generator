#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 20
#define INTEGER 0
#define FLOAT 1
#define FREQUENCY_MIN 0.0
#define FREQUENCY_MAX 100.0
#define MEAN_MIN 1.0
#define MEAN_MAX 10.0
#define AMPLITUDE_MIN 0
#define AMPLITUDE_MAX 4095

float frequency;
float mean;
unsigned int amplitude;
char* waveform_options[] = {"sine", "square", "triangular", "sawtooth"};
int current_waveform = -1;
const unsigned int len_waveform = sizeof(waveform_options)/sizeof(waveform_options[0]);

// Function Prototypes
bool input(char* var_pointer);
bool convertNum(char* input, void* var_pointer, int format, float min, float max);
int promptInt(char* msg, int min, int max);
float promptFloat(char* msg, float min, float max);

void usage(char* progname) {
    printf("Usage: %s [-f frequency] [-m mean] [-a amplitude] [-w waveform] [-s setting_file]\n", progname);
	printf("[-s setting_file]: (file) Default setting for frequency, mean and amplitude. Overwritten when corresponding option is used during program call.\n");
	printf("[-f frequency]: (float) Frequency of wave (Hz). Range: %f - %f\n", FREQUENCY_MIN, FREQUENCY_MAX);
	printf("[-m mean]: (float) Prescaled mean of wave. Range: %f - %f\n", MEAN_MIN, MEAN_MAX);
	printf("[-a amplitude]: (unsigned int) Amplitude of wave. Range: %d - %d\n", AMPLITUDE_MIN, AMPLITUDE_MAX);
    printf("[-w waveform]: (string) Type of waveform. Options: sine, square, triangular, sawtooth\n");
    exit(EXIT_FAILURE);
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
    if (!f_opt) frequency = promptFloat("Input Freqeuncy: ", FREQUENCY_MIN, FREQUENCY_MAX);
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
    printf("Frequency: %f\n",frequency);
    printf("Mean: %f\n",mean);
    printf("Amplitude: %d\n",amplitude);
    printf("Waveform: %s\n",waveform_options[current_waveform]);
    return(EXIT_SUCCESS);
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