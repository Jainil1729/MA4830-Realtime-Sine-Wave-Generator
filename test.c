#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_INPUT 20
#define INTEGER 0
#define FLOAT 1

// Function Prototypes
bool input(char* var_pointer);
bool convertNum(char* input, void* var_pointer, int format, float min, float max);
int promptInt(char* msg, int min, int max);
float promptFloat(char* msg, float min, float max);

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
            if (temp < min) {
                printf("%s is less than minimum number %d\n\n", input, (int) min);
                return false;
            }
            if (temp > max) {
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
            if (temp > max) {
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

    printf("Invalid input! Required format: %s\n\n", format_type);
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

int main() {
    // char* int_input = "-32532532";
    // char* float_input = "-32532532";
    // int i;
    // float f;
    // if (convertNum(int_input, &i, INTEGER, true)) {
    //     printf("%i\n", i);
    // }
    // if (convertNum(float_input, &f, FLOAT, true)) {
    //     printf("%f\n", f);
    // }
    int i = promptInt("Please input an integer: ", 0, 1023);
    printf("%i\n", i);
    return 0;
}