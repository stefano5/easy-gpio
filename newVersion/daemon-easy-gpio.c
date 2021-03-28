#include <sys/msg.h>
#include <sys/stat.h>

#include "lib/function-gpio.h"
#include "lib/daemon-easy-gpio.h"
#include "lib/management_arg.c"

#define PRIMARY_DIRECTORY "/home/%s/.easygpio"
#define PIN_OUT 1

typedef struct {
	float dutyCycle;	//it's a duty cycle, value: [0:0.001:1]  (read range like matlab)
	int pin;
} _arg_pwm;

struct {                            //22/3
    int pin;
    int state;
    float oldF;
    float newF;
    pthread_t pinTid;
} gpio_pwm[27];

void init_gpio_pwm() {              //22/3
    for (int i=0; i < 27; i++) {
        gpio_pwm[i].state = 0;
        gpio_pwm[i].pin = i+1;
        gpio_pwm[i].oldF = gpio_pwm[i].newF = 0.0;
    }
}

void stop_pwm(int pin) {
    for (int i=1; i < 28; i++) {
        if (i == pin)
            gpio_pwm[i].newF = 0.0;
    }
}

void stop_all_pwm() {
    for (int i=1; i < 28; i++) {
        gpio_pwm[i].newF = 0.0;
    }
}

void update_pwm(float f, int pin) {
    gpio_pwm[pin - 1].newF = f;
}

void *task_gpio_control(void *param) {
    int pin = *(int*)param;
    float dutyCycle = 0.0;
    initializePin(pin, PIN_OUT);


    while (TRUE) {
        if (gpio_pwm[pin-1].oldF != gpio_pwm[pin-1].newF) {
            dutyCycle = gpio_pwm[pin-1].newF;
            gpio_pwm[pin-1].oldF = dutyCycle;

            if (dutyCycle == 0.0 || dutyCycle == 1.0) {
                write_pin(pin, (int)dutyCycle);
                printf("write_pin. pin: [%d], value: [%d]\n", pin, (int)dutyCycle);
            } else {
                gpio_pwm[pin-1].oldF = gpio_pwm[pin-1].newF;
                printf("START PWM. pin: [%d], value: [%.2f]\n", pin ,dutyCycle);
                while(gpio_pwm[pin-1].oldF == gpio_pwm[pin-1].newF) {
                    _create_pwm(dutyCycle * 100, pin);
                }
            }
        }
        usleep(1000);
    }
    pthread_exit(NULL);
}



void init_thread() { 
    for (int i=0; i < 27; i++) {
        int                 res;
        int *pin = (int*)malloc(sizeof(int));
        *pin = i + 1;
        printf("init thread %d\n", *pin);

        res = pthread_create(&gpio_pwm[i].pinTid, NULL, task_gpio_control, (void*)pin);
        if (res != 0) {
            printf("[FATAL ERROR] Thread %d not running. Do you have permissions?\n", i);
        }
        gpio_pwm[i].pin = *pin;
    }
}



void try_and_solve_directory() {
    struct stat st = {0};
    char directory[64];
    getUserId(user, TRUE);
    initArray_str(directory, 64);
    sprintf(directory,  PRIMARY_DIRECTORY, user);

    if (stat(directory, &st) == -1) {   //check existance dir
        if (mkdir(directory, 0700) == -1) {
            printf("Directory [%s] missed. Please run the \"install.sh\" to use this software\n", directory);
            exit(EXIT_FAILURE);
        }
        printf("[Warning] The directory [%s] was lost, now it has been successfully recreated\n", directory);
    }
}

int msgid;
void stopExe(int sig) {
    printf("Signal recived\n");
    for (int i=1; i<28; i++)
        stop_pwm(i);
    //turn_off_all_gpio_pwm();
    msgctl(msgid, IPC_RMID, 0);
    exit(EXIT_SUCCESS);
}

int main(int argc, char**argv) {
    analyzeArg(argc, argv);
    managementArg();
    if (action == CLOSE) 
        exit(EXIT_SUCCESS); 


    signal(SIGINT, stopExe);
    signal(2, stopExe);
    struct MessageData messageFromQueue;
    key_t key = (key_t)KEY_MESSAGE_QUEUE;
    msgid = msgget(key, 0666 | IPC_CREAT);

    printf("\n\t\t\t\t how to kill me:	kill -2 %d\n", getpid());
    init_gpio_pwm();
    change_priority();
    init_thread();

    while(TRUE) {
        try_and_solve_directory();

        msgrcv(msgid, (void*)&messageFromQueue, sizeof(struct MessageData) - sizeof(long int), 0, 0);
        PRINT_IF_DEBUG_ON("%s I received pin=%d, value=%.2f\n", argv[0], (int)messageFromQueue.pin, (float)messageFromQueue.value);

        update_pwm((float)messageFromQueue.value, (int)messageFromQueue.pin);
    }
}
