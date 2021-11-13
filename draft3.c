// cc -o draft draft.c -lncurses -lpthread -lm

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <ncurses.h>
#include <hw/pci.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <math.h>

// PCI Registers
#define	INTERRUPT		iobase[1] + 0				// Badr1 + 0 : also ADC register
#define	MUXCHAN			iobase[1] + 2				// Badr1 + 2
#define	TRIGGER			iobase[1] + 4				// Badr1 + 4
#define	AUTOCAL			iobase[1] + 6				// Badr1 + 6
#define DA_CTLREG		iobase[1] + 8				// Badr1 + 8

#define AD_DATA		    iobase[2] + 0				// Badr2 + 0
#define	AD_FIFOCLR		iobase[2] + 2				// Badr2 + 2

#define	TIMER0			iobase[3] + 0				// Badr3 + 0
#define	TIMER1			iobase[3] + 1				// Badr3 + 1
#define	TIMER2			iobase[3] + 2				// Badr3 + 2
#define	COUNTCTL		iobase[3] + 3				// Badr3 + 3
#define	DIO_PORTA		iobase[3] + 4				// Badr3 + 4
#define	DIO_PORTB		iobase[3] + 5				// Badr3 + 5
#define	DIO_PORTC		iobase[3] + 6				// Badr3 + 6
#define	DIO_CTLREG		iobase[3] + 7				// Badr3 + 7
#define	PACER1			iobase[3] + 8				// Badr3 + 8
#define	PACER2			iobase[3] + 9				// Badr3 + 9
#define	PACER3			iobase[3] + a				// Badr3 + a
#define	PACERCTL		iobase[3] + b				// Badr3 + b

#define DA_Data			iobase[4] + 0				// Badr4 + 0
#define	DA_FIFOCLR		iobase[4] + 2				// Badr4 + 2


#define MAX_INPUT 20
#define INTEGER 0
#define FLOAT 1

#define FREQUENCY_MIN 0.0
#define FREQUENCY_MAX 10.0
#define FREQUENCY_STEPS 100
#define FREQUENCY_STEP_SIZE (FREQUENCY_MAX-FREQUENCY_MIN)/FREQUENCY_STEPS 
#define MEAN_MIN 1.0
#define MEAN_MAX 10.0
#define MEAN_STEPS 100
#define MEAN_STEP_SIZE (MEAN_MAX-MEAN_MIN)/MEAN_STEPS 
#define AMPLITUDE_MIN 0
#define AMPLITUDE_MAX 65535
#define AMPLITUDE_STEPS 100
#define AMPLITUDE_STEP_SIZE (AMPLITUDE_MAX-AMPLITUDE_MIN)/AMPLITUDE_STEPS 

# define STEPS 100
# define STEP_SIZE 2 * M_PI / STEPS
uintptr_t iobase[6];
unsigned int i;
float frequency;
float mean;
unsigned int amplitude;
unsigned int output;
char* waveform_options[] = {"sine", "square", "sawtooth", "triangular"};
int current_waveform = -1;
const unsigned int len_waveform = sizeof(waveform_options)/sizeof(waveform_options[0]);
useconds_t freq_delay;

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
void* output_result();

//vars
float current_freq;
float current_mean;
unsigned int current_amp;
int current_wf;

// Waveform Functions
unsigned int sine(int step);
unsigned int square(int step);
unsigned int sawtooth(int step);
unsigned int triangular(int step);
unsigned int (*waveformArray[]) (int) = {sine, square, sawtooth, triangular};
void* waveform_generator() {
    // Thread for generating waveform
    int step = 0;
    while (TRUE) {
        pthread_mutex_lock(&global_mutex);
        output = waveformArray[current_waveform](step);
        pthread_mutex_unlock(&global_mutex);
	
        // Output Data to DAC
        out16(DA_CTLREG, 0x0a23);
        out16(DA_FIFOCLR, 0);
        out16(DA_Data, (short)output);																																		
        out16(DA_CTLREG, 0x0a43);
        out16(DA_FIFOCLR, 0);
        out16(DA_Data, (short)output);	
        
         delay((10/frequency)-2);				//testing
              
        step += 1;
        if (step == STEPS) step = 0;
		
        // Sleep in microseconds
        //freq_delay = (useconds_t) 1000000.0/frequency/100.0;
        //usleep(freq_delay);
        //sleep(1.0/frequency/200.0);
    }
    
}

void* shutdown() {
    // For demonstration of shutdown of all threads using conditional variable
    //sleep(10000);
    
    // Signal main thread to shutdown all threads and exit program
    pthread_mutex_lock(&shutdown_mutex);
    pthread_cond_signal(&shutdown_cond);
    pthread_mutex_unlock(&shutdown_mutex);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    // PCI Variables Declaration
	struct pci_dev_info info;
	void *hdl;
	
	uintptr_t dio_in;
	uint16_t adc_in;
    int badr[5];
	
    // Thread Variables Declaration
    pthread_t kb_thread, waveform_thread, output_thread, shutdown_thread;
    
    // Command Line Argument Variables Declaration
    int opt;
    bool f_opt = FALSE, m_opt = FALSE, a_opt = FALSE, s_opt = FALSE, w_opt = FALSE;

    // Parse Command Line Arguments
    while ((opt = getopt(argc, argv, ":f:m:a:w:s")) != -1) {
        char arg[MAX_INPUT] = "";
        switch (opt) {
        case 'f':
            f_opt = TRUE;
            // printf("Argument to f is %s\n", optarg);
            if (!convertNum(optarg, &frequency, FLOAT, FREQUENCY_MIN, FREQUENCY_MAX)) {
                usage(argv[0]);
            }
            break;
        case 'm':
            m_opt = TRUE;
            // printf("Argument to m is %s\n", optarg);
            if (!convertNum(optarg, &mean, FLOAT, MEAN_MIN, MEAN_MAX)) {
                usage(argv[0]);
            }
            break;
        case 'a':
            a_opt = TRUE;
            // printf("Argument to a is %s\n", optarg);
            if (!convertNum(optarg, &amplitude, INTEGER, AMPLITUDE_MIN, AMPLITUDE_MAX)) {
                usage(argv[0]);
            }
            break;
        case 'w':
            w_opt = TRUE;
            
            // Convert string to lowercase
            for(i = 0; optarg[i]; i++) {
                optarg[i] = tolower(optarg[i]);
            }
            
            // Match option argument to waveform options
            for (i=0; i<len_waveform; i++) {
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
            s_opt = TRUE;
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
        printf("Select Waveform: \n");
        for (i=0; i<len_waveform; i++) {
            printf("%d) %s\n", i, waveform_options[i]);
        }
       current_waveform = promptInt("Input: ", 0, len_waveform-1);
    }
    
    // PCI Setup
    memset(&info,0,sizeof(info));
	if(pci_attach(0)<0) {
 		 perror("pci_attach");
 		 exit(EXIT_FAILURE);
 	 }

	info.VendorId=0x1307;
	info.DeviceId=0x01;

	if ((hdl=pci_attach_device(0, PCI_SHARE|PCI_INIT_ALL, 0, &info))==0) {
		 perror("pci_attach_device");
 		 exit(EXIT_FAILURE);
  	}	

	for(i=0;i<5;i++) {
 		badr[i]=PCI_IO_ADDR(info.CpuBaseAddress[i]);
 	 }
 
	// Map I/O base address to user space						
	for(i=0;i<5;i++) {
 		 iobase[i]=mmap_device_io(0x0f,badr[i]);	
  	}				
  	
  	// Modify thread control privity
	if(ThreadCtl(_NTO_TCTL_IO,0)==-1) {
  		perror("Thread Control");
 		exit(EXIT_FAILURE);
  	}

    /* Curses Initialisations */
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    pthread_create(&kb_thread, NULL, get_keyboard_input, NULL);
    pthread_create(&waveform_thread, NULL, waveform_generator, NULL);
    //pthread_create(&output_thread, NULL, output_result, NULL);
    pthread_create(&shutdown_thread, NULL, shutdown, NULL);

    // Wait for shutdown condition
    pthread_mutex_lock(&shutdown_mutex);
    pthread_cond_wait(&shutdown_cond, &shutdown_mutex);
    pthread_mutex_unlock(&shutdown_mutex);

    // Shutdown all threads
    pthread_cancel(kb_thread); 
    pthread_cancel(waveform_thread);
    //pthread_cancel(output_thread);
    pthread_cancel(shutdown_thread);

    endwin();
	
	/*
    out16(DA_CTLREG,(short)0x0a23);
	out16(DA_FIFOCLR,(short) 0);
	out16(DA_Data, 0x8fff);															
  
	out16(DA_CTLREG,(short)0x0a43);
	out16(DA_FIFOCLR,(short) 0);
	out16(DA_Data, 0x8fff);
	*/
																																						
	pci_detach_device(hdl);
    printf("Ending Program.\n");
    return EXIT_SUCCESS;
}

void usage(char* progname) {
    printf("Usage: %s [-f frequency] [-m mean] [-a amplitude] [-w waveform] [-s setting_file] [-h]\n", progname);
	printf("[-s setting_file]: (file) Default setting for frequency, mean and amplitude. Overwritten when corresponding option is used during program call.\n");
	printf("[-f frequency]: (float) Frequency of wave (Hz). Range: %f - %f\n", FREQUENCY_MIN, FREQUENCY_MAX);
	printf("[-m mean]: (float) Prescaled mean of wave. Range: %f - %f\n", MEAN_MIN, MEAN_MAX);
	printf("[-a amplitude]: (unsigned int) Amplitude of wave. Range: %d - %d\n", AMPLITUDE_MIN, AMPLITUDE_MAX);
    printf("[-w waveform]: (string) Type of waveform. Options: sine, square, sawtooth, triangular.\n");
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
        TRUE when input is sucessful, else FALSE
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
        return FALSE;
    }

    sscanf(value, "%s", var_pointer);
    return TRUE;
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
        TRUE when input is in correct format, else FALSE
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
                return FALSE;
            }
            else if (temp > (int) max) {
                printf("%s is greater than maximum number %d\n\n", input, (int) max);
                return FALSE;
            }
            *var_pointer_2 = temp;
            return TRUE;
        }
    }
    else if (format == FLOAT) {
        float temp;
        float* var_pointer_2 = (float*)var_pointer;
        sprintf(format_type, "float");
        if (sscanf(input, "%f %c", &temp, &c) == 1) {
            if (temp < min) {
                printf("%s is less than minimum number %f\n\n", input, min);
                return FALSE;
            }
            else if (temp > max) {
                printf("%s is greater than maximum number %f\n\n", input, max);
                return FALSE;
            }
            *var_pointer_2 = temp;
            return TRUE;
        }
    }
    else {
        printf("Unsupported format\n\n");
        return FALSE;
    }

    printf("Invalid format. %s is not %s\n\n", input, format_type);
    return FALSE;
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
    while (TRUE) {
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
    while (TRUE) {
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

unsigned int sine(int step) {
    double result = sin(step * STEP_SIZE);
    unsigned int scaled_result = (unsigned int)((result + mean) * amplitude);//2
    delay((1000/frequency));
    return scaled_result;
}

unsigned int square(int step) {
    double result;
    unsigned int scaled_result;
    static const int half_step_size = STEPS/2;
    if (step < half_step_size) {
        result = - 1.0;
    }
    else {
        result = 1.0;
    }
    scaled_result = (unsigned int)((result + mean) * amplitude); //2.0
    // delay((10/frequency)-2);
    return scaled_result;
}

unsigned int sawtooth(int step) {
    double result = -1.0 + ((double)step)/((double)(STEPS-1)) * 2.0;
    unsigned int scaled_result = (unsigned int)((result + mean) * amplitude); //2.0
    delay(1000/frequency);
    return scaled_result;
}

unsigned int triangular(int step) {
    double result = asin(sin(step * STEP_SIZE));
    unsigned int scaled_result = (unsigned int)((result + mean) * 2 * amplitude/ M_PI); //2.0
    delay(1000/frequency);
    return scaled_result;
}

void* output_result() {
    while (TRUE) {
       clear();
        printw("Press E to Exit\n\n");
        printw("Up/Down: Change Amplitude\n");
        printw("Left/Right: Change Frequency\n");
        printw("W/S: Change Mean\n");
        printw("A/D: Change Waveform\n");
        pthread_mutex_lock(&global_mutex);
        printw("\nFrequency: %f",frequency);
        printw("\nMean: %f",mean);
        printw("\nAmplitude: %d",amplitude);
        printw("\nWaveform: %s",waveform_options[current_waveform]);
        //printw("\n%f",1000000.0/frequency/STEPS);
        pthread_mutex_unlock(&global_mutex);
      
        // Update rate at 100 Hz
        //usleep(10000);
       sleep(0.5);
       refresh();
    }
}