#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
int current_waveform = 0;
const unsigned int len_waveform = sizeof(waveform_options)/sizeof(waveform_options[0]);

int main() {
    char str1[100] = "Select Waveform: \n";
    for (int i=0; i<len_waveform; i++) {
        char str2[20] = "";
        sprintf(str2, "%d) %s\n", i, waveform_options[i]);
        strcat(str1, str2);
    }
    printf("%s", str1);
    printf("%ld\n", sizeof(waveform_options)/sizeof(waveform_options[0]));
}