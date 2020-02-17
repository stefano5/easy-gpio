#include "lib/function-gpio.h"
#include "lib/daemon-easy-gpio.h"

#define KEY_MESSAGE_QUEUE 1234

void printHelp(char **argv){
    printf("Help:\n\tname\t\t\t\t\t\tVersion");
    printf("\n\t%s\t\t\t\t\t"VERSION"\n\n", argv[0]);
    printf("Command:\n"
            "\t-i pin in/out		initialized pin\n"
            "\t-r | -ra		read value and direction for each gpio [-r(ead)a(ll)]\n "
            "\tpwm pin f    	create a pwm in a pin 'pin' at the frequency 'f' [pin=integer, f=float]\n"
            "\t-t pin       switch value of the pin 'pin'\n"
            "\t-h 			show this message\n\n");
    printf("Syntax:\n\n");
    printf("%s pin -t (or --togle)\tto switch the value of the pin\n", argv[0]);
    printf("%s pin value		to write the value 'value' on the pin 'pin'\n", argv[0]);
    printf("%s pin			to read the value and the direction of the pin 'pin'\n", argv[0]);
}

int read_gpio(char *file){
    char path[32];
    initArray_str(path, 32);
    sprintf(path, "/sys/class/gpio/%s/value", file);
    FILE *f=fopen(path, "r");
    if (f==NULL) return -1;
    int value;
    fscanf(f, "%d", &value);
    fclose(f);
    return value;
}

struct {
    char path[28][64];      //path, eg: gpio25
    char direction[28][4];  //in or out
    int gpio[28];           //gpio number, eg: 25
    int n_gpio;             //number of gpio, eg: 5 (that is, there are 5 gpio initialized)
} Gpio;
int read_directory_ad(char *directory){
    DIR *d;
    struct dirent *dir;
    d = opendir(directory);  
    if (d!= NULL) {
        int i=0;
        while ((dir = readdir(d)) != NULL) {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") || !strcmp(dir->d_name, "export") || strlen(dir->d_name) > 6) 
                continue;
            strcpy(Gpio.path[i], dir->d_name);
            subString(dir->d_name, 4, strlen(dir->d_name));
            Gpio.gpio[i]=atoi(dir->d_name);
            read_direction(Gpio.gpio[i]) == 1 ? strcpy(Gpio.direction[i], "out") : strcpy(Gpio.direction[i], "in");
            i++;
        }
        closedir(d);
        Gpio.n_gpio=i;
        return 1;
    } else {
        printf("Directory [%s] not found\n", directory);
        exit(EXIT_FAILURE);
    }
}

void read_all_gpio(){
    read_directory_ad("/sys/class/gpio/");
    printf( "\t+----- GPIO ----+----- VALUE ----+---- DIRECTION -------+\n");
    for (int i=0; i<Gpio.n_gpio; i++){
        int value = read_gpio(Gpio.path[i]);
        if (value!=-1){
            printf("\t|\t%d\t|\t %d\t |\t  %s\t\t|\n", Gpio.gpio[i], value, Gpio.direction[i]);
        } else {
            printf("Error 147. file: %s at line %d. Contact developer please\n", __FILE__, __LINE__);
        }
    }
    printf("\t+---------------+----------------+----------------------+\n");
}

void analyze_command_argc1(char **argv) {
    if (!strcmp(argv[1], "-h")) {
        printHelp(argv);
        exit(EXIT_SUCCESS);
    }

    if (!strcmp(argv[1], "-i")) {
        printf("Miss the number of pin to initialized\n");
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[1], "pwm")) {
        printf("Bad argument. %s pwm pin f\n", argv[0]); 
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "-ra")) {
        read_all_gpio();
        exit(EXIT_SUCCESS);
    }

    int pin = atoi(argv[1]);

    if (pin == 0) {
        printf("Error input. This (%s) should be a number without space or other char. Or, if you want, a known command (try -h param)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (pinIsInitialized(pin)) {
        printf("Value of pin is: %d\n" ,read_pin(pin));
        print_direction(pin);
        exit(EXIT_SUCCESS);
    } else {
        printf("This pin (%d) is not initialized\n", pin);
        exit(EXIT_FAILURE);
    }
}

void analyze_command_argc2(int argc, char **argv){
    if (!strcmp(argv[1], "-i")) {
        if (argc > 3){  //argv[0] -i 25 in/out
            int par = f_init_gpio(atoi(argv[2]), argv[3]);
            if (par == 1 || par == 0){
                exit(EXIT_SUCCESS);
            } else { 
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Syntax error: %s -i pin direction\twhere 'pin' is a number and 'direction' is only 'in'/'out'\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    //argv[0] pwm 12 0.5 
    if (!strcasecmp(argv[1], "pwm")){
        int pin = atoi(argv[2]);
        if (pin == 0){
            printf("Syntax error. Should be\t%s pwm pin f   where"
                    " 'pwm' is a command, 'pin' is a number of the pin, 'f' is the normalized frequency (or duty cycle) whit value from 0 to 1\n", argv[0]); 
            exit(EXIT_FAILURE); 
        }

        
        float frequency = (float)atof(trim(argv[3])); //cosi non fa differenza tra passare '0' e passare 'ciao', si ottiene la stessa cosa
        
        if (frequency == 0) {
                send_message(pin, 0.0);
                exit(EXIT_SUCCESS);
        } else if (frequency == 1) {
                send_message(pin, 1.0);
                exit(EXIT_SUCCESS);
        } else {
                send_message(pin, frequency);        
                exit(EXIT_SUCCESS);
        }
    }

    int pin = atoi(argv[1]);
    if (!strcmp(argv[2], "-t") || !strcmp(argv[2], "--toggles")){
        toggles_pin_msg(pin);           //all'interno l'accesso alla coda e la cancellazione non è gia' fatto? verificalo
        exit(EXIT_SUCCESS);
    } else if (!strcmp(argv[2], "1") || !strcmp(argv[2], "0")){
        send_message(pin, atof(argv[2]));
        //printf("%s ho mandato: pin= %d value=%.2f\n", argv[0], pin, atof(argv[0]));
        exit(EXIT_SUCCESS);
    } else {
        float may_frequency = atof(argv[2]);
        if (may_frequency > 0 && may_frequency < 1) {
            send_message(pin, may_frequency);
            //printf("%s ho mandato: pin= %d value=%.2f\n", argv[0], pin, atof(argv[0]));
        } else {
            printf("Sintax error. Should be\t%s pin value   Where 'pin' is a number of the pin, 'value' is a number [0, 1].\n\tNB: if you pass a intermediete value pwm will be generated\n", argv[0]); 
            printf("\tFor example: %s 12 0\tturn off gpio 12\n", argv[0]);
            printf("\tFor example: %s 12 -t\ttooggles pin\n", argv[0]);
            printf("\tFor example: %s 12 0.5\tcreate new soft pwm on pin 12\n", argv[0]);
            printf("\tFor example: %s 12 0.01\tchange frequency pwm\n", argv[0]);
        }
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv){
    if (argc==1){
        printHelp(argv);
        exit(EXIT_SUCCESS);
    }
    
    if (argc==2){
        analyze_command_argc1(argv);
    } else  { 
        analyze_command_argc2(argc, argv);
    }
}
