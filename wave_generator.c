# include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>

# define USAGE_FMT  "usage: %s default_setting_file [-f frequency] [-m mean] [-a amplitude] [-h]"

# ifndef M_PI
#	 define M_PI           3.14159265358979323846
# endif

# define STEPS 100
# define STEP_SIZE 2 * M_PI / STEPS
# define AMPLITUDE 0x0fff
# define MEAN 1
# define FREQUENCY 1

void usage(char* progname) {
	fprintf(stderr, USAGE_FMT, progname);
	/*
	printf("default_setting_file: Default setting for frequency, mean and amplitude. Overwritten when corresponding option is used during program call.");
	printf("[-f frequency]: (float) Frequency of sine wave generator (Hz). Range: 1.0 - 10.0");
	printf("[-m mean]: (float) Prescaled mean of sine wave. Range: 1.0 - 10.0");
	printf("[-a amplitude]: (unsigned int) Amplitude of sine wave. Range: 0 - 4095");
	printf("[-h]: help");
	*/
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	int frequency, mean, amplitude;
	for (int optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
        case 'f':
			if (argv[++optind][0] == '-') usage(argv[0]);
			sscanf(argv[optind], "%d", &frequency);
			printf("Frequency set to %d Hz\n", frequency);
			break;
        case 'm':
			if (argv[++optind][0] == '-') usage(argv[0]);
			sscanf(argv[optind], "%d", &mean);
			printf("Mean set to %d\n", mean);
			break;
        case 'a': 
			if (argv[++optind][0] == '-') usage(argv[0]);
			sscanf(argv[optind], "%d", &amplitude);
			printf("Amplitude set to %d\n", amplitude);
			break;
		case 'h':
		default:
			//usage(argv[0]);
			return(EXIT_FAILURE);
        }   
    }
	
	/*
	unsigned int sine_wave[STEPS];

	// Precalculate sine wave
	for (int i = 0; i < STEPS; i++) {
		double result = sin(i * STEP_SIZE);
		unsigned int scaled_result = (unsigned int)((result + 1.0) * AMPLITUDE / 2);
		//printf("The sine of step %i is 0x%03X\n", i, scaled_result);
		sine_wave[i] = scaled_result;
	}
	*/

	return EXIT_SUCCESS;
}