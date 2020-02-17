#define _GNU_SOURCE
#include"function.h"

char __user[32];

/*
 * to debug
 * */
void write_generic_log(char *text){
	FILE *f=fopen("/home/pi/Desktop/log_generic.txt", "a+");
	char * toWrite = (char*)malloc(strlen(text) + strlen(getDate()) + 3);	//sizeof(char) = 1
	sprintf(toWrite, "[%s] %s", getDate(), text);
	fprintf(f, "%s\n\n", toWrite);
	fclose(f);
	free(toWrite);
}

/*	control path: /sys/class/gpio/gpioN/value. if not exist return 0, else 1
 * */
int pinIsInitialized(int pin){
	char path[48]="";
	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	FILE *f=fopen(path, "r");
	if(f == NULL) return 0;
	fclose(f);
	return 1;
}

/*	write: 	pin into /sys/class/gpio/export 
 *		direction into /sys/class/gpio/gpiopin/direction
 *	
 *	At the end the pin will be initialized and it will be possible to use it correctly
 *
 *	Eg: initializePin(4, "out");
 * */
int initializePin(int pin, int direction){	//direction = 1 ==> "out"; direction = 0 ==> "in"
	char path[48]="";
	if (!pinIsInitialized(pin)){
		FILE *f=fopen("/sys/class/gpio/export", "w");
		fprintf(f, "%d", pin);
		fclose(f);
		sleep(1);
		sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
		f=fopen(path, "w");
		direction == 0 ? fprintf(f, "in") : fprintf(f, "out");
		fclose(f);
		return 0;
	}
	return 1;
}

/*	turn on/off pin		and, if is necessary, initialize automatically the pin
 *
 * Eg: 	write_pin(12, 1);	turn on gpio 12
 * 	write_pin(12,0);	turn off gpio 12
 * */
void write_pin(int pin, int value){
	initializePin(pin, 1);
	char path[32]="";
	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	FILE *f=fopen(path, "w");
	fprintf(f, "%d", value);
	fclose(f);
}

/* 
 * @return	0 if gpio is "in"
 * 		1 if gpio is "out"
 * 		-1 if gpio is not initialize
 *
 * 	Eg:	if (read_direction(12) == 0)
 * 			printf("Pin 12 is set like INPUT, the value is %d\n", read_pin(12));
 *
 * 	OUTPUT:	Pin 12 is set like INPUT, the value is 0
 * */
int read_direction(int pin){	
	if (pinIsInitialized(pin)){
		char path[48]="";
		char direction[8];
		sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
		FILE *f=fopen(path, "r"); //se e' inizializzato questo file deve esistere
		fscanf(f, "%s", direction);
		fclose(f);
		if (!strcmp(direction, "in")) return 0;
		if (!strcmp(direction, "out")) return 1;
	}
	//read direction without initilization is impossible
	printf("[%s->read_direction] Pin (%d) Error 498\n", __FILE__, pin);
	printf_d("Read direction without initilization is not possible\n");
	return -1;
}

/**
 * The direction of pin is "in"?
 * */
int is_input(int pin){
	char path_gpio[64]="";
	char state[3]="";
	sprintf(path_gpio, "/sys/class/gpio/gpio%d/direction", pin);
	FILE *f =fopen(path_gpio, "r");
	if (f == NULL) {printf("Error, pin [%d] not found\n", pin); return -1; }
	fscanf(f, "%s", state);
	fclose(f);
	if (!strcmp(state, "in")) return TRUE;
	return FALSE;
}

/*	read_pin(pin)
 *	read the value of the gpio (0/1)
 *
 * @return value of the gpio
 * */
int read_pin(int pin){
	int value;
	char path[32]="";
	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
/*	//if you want print here the direction:
	if (read_direction(pin) == 1)
		printf("This pin (%d) is set like out\n", pin);
	else
		printf("This pin (%d) is set like in\n", pin);
*/	//Or, with different output you can use 
//	print_direction(pin);

	FILE *f=fopen(path, "r");
	if (f==NULL) return -1;
	fscanf(f, "%d", &value);
	fclose(f);
	return value;
}

void print_direction(int pin){
	if (pinIsInitialized(pin)){
		printf("The direction is: "); is_input(pin)==1 ? printf("IN\n") : printf("OUT\n");
	} else {
		printf("Pin is not inizilized\n");
	}
}

/*
 *	to example see down: f_pwm_gpio()
 * */
void create_pwm(float f, int pinPwm){
	for (unsigned int i=0; i<100; i++){
		if (i<f){
			write_pin(pinPwm, 1);
		} else {
			write_pin(pinPwm, 0);
		}
		usleep(5);
	}
}

void change_priority(){
	nice(-15);  //-20 MAX priority, 19 MIN, 0 DEFAULT	
}

void set_priority(int priority){
	if (priority>19 || priority<-20) {
		write_generic_log("[function-gpio.h->set_priority] Value of priority is only: [-20, +19]");
	} else {
		nice(priority);  //-20 MAX, 19 MIN, 0 DEFAULT	
	} 
}

void remove_file_pid(int pin){
	if (__user==NULL)
		getUserId(__user, 32);
	char cmd[32]="";
	sprintf(cmd, "/home/%s/pid_pwm%d.txt", __user, pin);
	remove(cmd);
}

int _pinPwm, _flag=0;
void sig_handler(int sig){
	printf("lose\n");
	fcloseall();
	write_pin(_pinPwm, 0);	//turn off gpio
	if (_flag == 1)
		remove_file_pid(_pinPwm);
	exit(EXIT_SUCCESS);
}

void write_pid(int pid, int pin, float frequency){
	if (__user==NULL)
		getUserId(__user, 32);
	char path[48]="";
	sprintf(path, "/home/%s/pid_pwm%d.txt", __user, pin);	
	FILE *f=fopen(path, "w");
	fprintf(f, "%d\n", pid);
	fprintf(f, "%.4f\n", frequency);
	fclose(f);
}


//Only for function_pwm
void clear_other_istance(int pin){
	if (__user==NULL)
		getUserId(__user, 32);
	char path[48]="";
	sprintf(path, "/home/%s/pid_pwm%d.txt", __user, pin);
	FILE *f=fopen(path, "r");
	if (f!=NULL){
		int pid;
		fscanf(f, "%d", &pid);
		fclose(f);
		if (pid==0){
			printf("[%s->clear_other_istance] ERROR: file [%s] have a bad value\n", __FILE__, path);
			return;
		}
		kill(pid, 9);
	}
}

//Only for function_pwm
void verify_input(char **argv){
	float f=(float)atof(argv[1]);
	if (f<0 || f>1) {
		printf("Frequency error [%.2f]. Value: 0<f<1\n", f); 
		exit(EXIT_FAILURE);
	}

	if (atoi(argv[2])==0){
		printf("Pin value is a number >0. [%s] in not a number >0\n", argv[2]);
		exit(EXIT_FAILURE);
	}
}


//The follow function are usable to control, thoughtless the gpio



/*	f(unction)_print_value_gpio(pin)
 *	print value and direction
 * */
void f_print_value_gpio(int pin){
	if (pin<=0 || pin >= 27){
		printf("Bad argument. [%s->f_print_value_gpio] read/-r pin 	where pin is a number [1,26]\n", __FILE__);
		exit(EXIT_FAILURE);
	}
	printf("Value: %d\n", read_pin(pin));
	printf("Direction: "); read_direction(pin) == 0 ? printf("in") : printf("out");
}

/*	f(unction)_write_gpio(pin)
 * */
void f_write_gpio(int pin, int value){
	if (pin<=0 || pin >= 27){
		printf("Bad argument. [NOME PROGRAMMA] write/-w pin 	where pin is a number [1,26]\n");
		exit(EXIT_FAILURE);
	}
	write_pin(pin, value);
}

/**
 * f(unction)_init(ilized)_gpio(pin, dir(ection))
 * @return 1 if the direction is out, 0 if the direction is in, -1 if an error is detected
 * eg: f_init_gpio(12, "out")		to initialize gpio 12 like output
 * */
int f_init_gpio(int pin, char *dir){
	if (pin<=0 || pin >= 27){
		printf("Bad argument. [%s] init/-i pin direction	where pin is a number [1,26] and direction is 0 (in) or 1 (out)\n", __FILE__);
		return -1;
		//exit(EXIT_FAILURE);
	}
	int direction;
	if (!strcmp(dir, "in")){ 
		direction=0;
	} else if (!strcmp(dir, "out")){
	       	direction=1;
	} else {
		printf("Bad argument. [%s] init/-i pin direction	where pin is a number [1,26] and direction is 0 (in) or 1 (out)\n", __FILE__);
		return -1;
	}
	initializePin(pin, direction);
	return direction;
}

/*	f(unction)_pwm_gpio(frequency, pin)
 * @param f	is a float (kHz)    value: [0:0.001:1]
 * @param pin	is a gpio
 *
 * Eg: 	f_pwm_gpio(0.001, 12);	//ok
 * 	f_pwm_gpio(0.84, 12);	//ok
 *	f_pwm_gpio(2, 12);	//NO! It's a error
 * */
int f_pwm_gpio(float f, int pin){
	if (pin<=0 || pin >= 27 || f>1 || f<0){
		printf("Bad argument. [%s] f_pwm_gpio(f, pin)		where f is a float [0,1] and pin is a int [1,26]\n", __FILE__);
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, sig_handler);
	getUserId(__user, 32);
	if (f==0.0){ clear_other_istance(pin); remove_file_pid(pin); write_pin(pin, 0); exit(EXIT_SUCCESS); }
	if (f==1.0){ clear_other_istance(pin); remove_file_pid(pin); write_pin(pin, 1); exit(EXIT_SUCCESS); }
	initializePin(pin, 1);
	int pid = getpid();
	clear_other_istance(pin);
	change_priority();
	write_pid(pid, pin, f); //and frequency
	_pinPwm=pin;	//per routine chiusura
	_flag=1;
	printf("Frequency: %.2f pin: %d pid: %d\n", f, pin, pid); //or %.4f
	f*=100;	//to normalize
	while(1){
		create_pwm(f, _pinPwm);
	}
}


/**
 * switch the value.
 * Eg:	tooggles_pin(6);
 * 	if pin 6 is on, it will be turned off
 *
 * @return the new value
 * */
int toggles_pin(int pin){
	if (pinIsInitialized(pin)){
		if (read_pin(pin)==0){
			write_pin(pin, 1);
			printf("Pin %d now is HIGH\n", pin);
			return 1;
		} else {
			write_pin(pin, 0);
			printf("Pin %d now is LOW\n", pin);
			return 0;
		}
	} else {
		write_pin(pin, 1);
		printf("Pin %d has been initialized like output and now is HIGH\n", pin);
		return 1;
	}
}
