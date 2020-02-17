#include"../inc/function-gpio.h"
#include<dirent.h>	//read_directory_ad


int read_gpio(char *file){                                                                                                                                                                      
        char path[32]="";                                                                                                                                                                       
        sprintf(path, "/sys/class/gpio/%s/value", file);                                                                                                                                        
        FILE *f=fopen(path, "r");                                                                                                                                                               
        if (f==NULL) return -1;                                                                                                                                                                 
        int value;                                                                                                                                                                              
        fscanf(f, "%d", &value);                                                                                                                                                                
        fclose(f);                                                                                                                                                                              
        return value;                                                                                                                                                                           
}                                                                                                                                                                                               

struct {
        char path[26][64];      //path, eg: gpio25
        char direction[26][4];  //in or out
        int gpio[26];           //gpio number, eg: 25
        int n_gpio;             //number of gpio, eg: 5 (that is, there are 5 gpio initialized)
} Gpio;
int read_directory_ad(char *directory){  //particolare implementazione della read_directory
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
                        printf("Error. file: %s at line %d \n", __FILE__, __LINE__);
                }
        }
        printf("\t+---------------+----------------+----------------------+\n");
}


//passo comando, pin, stato
int main(int argc, char **argv){
	if (argc==1){
help:
		printf("This is free software with ABSOLUTELY NO WARRANTY.\n\n");
		printf("Help:\n\tname\t\t\t\t\t\tVersion");
		printf("\n\t%s\t\t\t\t\tV3.0\n\n", argv[0]);
		printf("Command:\n"
			"\t-i pin in/out		initialized pin\n"
			"\t-r | -ra		read value and direction for each gpio\n "
			"\tpwm f pin		create a pwm\n"
			"\t-h 			show this message\n\n");
		printf("Syntax:\n\n");
		printf("%s pin -t (or --toogle)\tto switch the pin's value\n", argv[0]);
		printf("%s pin value		to write the value \"value\" on the pin \"pin\"\n", argv[0]);
		printf("%s pin			to read the pin value and the direction of the \"pin\"\n", argv[0]);
		exit(EXIT_SUCCESS);
	}
	int pin = atoi(argv[1]);
	if (argc==2){	
		if (!strcmp(argv[1], "-h")){
			goto help; //srr :(
		}
		
		if (!strcmp(argv[1], "-i")){
			printf("Miss the number of pin to initialized. %s -i 25 in/out\n", argv[1]);
			exit(EXIT_FAILURE);
		}
		
		if (!strcmp(argv[1], "pwm")){
			printf("Bad argument. %s pwm f pin\n", argv[0]); 
			exit(EXIT_FAILURE);
		}
		
		if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "-ra")){
			read_all_gpio();
			exit(EXIT_SUCCESS);
		}
		
		if (pin==0){ printf("Why the pin value is zero or is not a number?\n"); exit(EXIT_FAILURE);}
		
		if (pinIsInitialized(pin)){
			printf("Value of pin is: %d\n" ,read_pin(pin));
			print_direction(pin);
		}
	} else  {	//write the value
		if (!strcmp(argv[1], "-i")){
			if (argc==4){ //argv[0] -i 25 in/out
				int par = f_init_gpio(atoi(argv[2]), argv[3]);
				if (par == 1 || par == 0)
					exit(EXIT_SUCCESS);
				else 
					exit(EXIT_FAILURE);
			} else {
				printf("Error. Syntax: %s -i pin direction\twhere pin is a number and direction is only in/out\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		
		//argv[0] pwm 0.5 13
		if (!strcmp(argv[1], "pwm")){
			if (argc<=3){
_bad_argument_pwm:
				printf("Bad argument. Syntax is:\t%s pwm f pin\n", argv[0]); 
				exit(EXIT_FAILURE);
			}
			float frequency = (float)atof(argv[2]);
			pin = atoi(argv[3]);
			if (pin == 0) 
				goto _bad_argument_pwm;
			f_pwm_gpio(frequency, pin);   //never return
		}

		if (pin==0){
			printf("Why the pin value is zero or is not a number?\n"); 
			exit(EXIT_FAILURE);
		} else 	if (!strcmp(argv[2], "-t") || !strcmp(argv[2], "--toggles")){
			toggles_pin(pin);
		} else if (!strcmp(argv[2], "1") || !strcmp(argv[2], "0")){
			write_pin(pin, atoi(argv[2]));
		} else {
			printf("Bad argument [%s]\n", argv[2]);
		}
	}
	exit(EXIT_SUCCESS);
}
