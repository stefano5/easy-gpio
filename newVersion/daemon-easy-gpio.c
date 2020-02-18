#include <sys/msg.h>
#include <sys/stat.h>

#include "lib/function-gpio.h"
#include "lib/daemon-easy-gpio.h"
#include "lib/management_arg.c"

#define PRIMARY_DIRECTORY "/home/%s/.easygpio"

typedef struct {
	float dutyCycle;	//it's a duty cycle, value: [0:0.001:1]  (read range like matlab)
	int pin;
} _arg_pwm;

struct {
    int state;
    float oldF;
    pthread_t pinTid;
} gpio_pwm[28];

void init_gpio_pwm() {
    for (int i=0; i < 28; i++) {
        gpio_pwm[i].state = 0;
        gpio_pwm[i].oldF = 0.0;
    }
}

void turn_off_all_gpio_pwm() {
    for (int i=0; i < 28; i++) {
        if (gpio_pwm[i].state != 0) {
            pthread_cancel(gpio_pwm[i].pinTid);
            gpio_pwm[i].state = 5;
        }
    }

    sleep(1);

    for (int i=0; i < 28; i++) {
        if (gpio_pwm[i].state == 5) {
            write_pin(i + 2, 0);
            break;
        }
    } 
}

void turn_off_gpio_pwm(int pin_pwm) {
    PRINT_IF_DEBUG_ON("Turn off pwm pin: %d\n", pin_pwm + 2);
    if (gpio_pwm[pin_pwm].state == 1) {
        PRINT_IF_DEBUG_ON("    ^= that thread was in execution, now it has been killed\n");
        pthread_cancel(gpio_pwm[pin_pwm].pinTid);
    }

}

void *routine_new_thread(void* param) {
    _arg_pwm *arg_pwm = (_arg_pwm*)param;
    float dutyCycle = arg_pwm->dutyCycle;
    int pin = arg_pwm->pin;
    free(arg_pwm);
    PRINT_IF_DEBUG_ON("Routine thread, it received: f=%.2f, pin=%d\n", dutyCycle, pin);
    
    f_pwm_gpio(dutyCycle, pin); //this function blocks the execution
    PRINT_IF_DEBUG_ON("f_pwm_gpio finished\n");
    pthread_exit(NULL);
}

int isNull(int pin) {
    return gpio_pwm[pin-2].state == 0;
}

void create_pwm(float f, int pin) {
    _arg_pwm *arg_pwm=(_arg_pwm*)malloc(sizeof(_arg_pwm));		//_arg_pwm it's a type

    arg_pwm->dutyCycle = f;
    arg_pwm->pin = pin;
    
    turn_off_gpio_pwm(pin - 2);

    if (isNull(pin)) {
        PRINT_IF_DEBUG_ON("\t[create_pwm]Pin %d was not used. Now it's used\n", pin);
        if (f == 0.0) {
            PRINT_IF_DEBUG_ON("     write 0 on pin %d\n\n", pin);
            gpio_pwm[pin - 2].state = 0;
            write_pin(pin, 0);
        } else if (f == 1.0) {
            PRINT_IF_DEBUG_ON("     write 1 on pin %d\n\n", pin);
            gpio_pwm[pin - 2].state = 1;
            write_pin(pin, 1);
        } else {
            PRINT_IF_DEBUG_ON("generete pwm\n");
            gpio_pwm[pin - 2].state = 1;
            pthread_create(&gpio_pwm[pin - 2].pinTid, NULL, routine_new_thread, (void*)arg_pwm);
            gpio_pwm[pin - 2].oldF = f;
        }
    } else {
        PRINT_IF_DEBUG_ON("\t[create_pwm]Pin %d was used, will be stopped then will be generated new pwm (or simple assignment)\n", pin);
        if (f == 0.0) {
            PRINT_IF_DEBUG_ON("     write 0 on pin %d\n\n", pin);
            PRINT_IF_DEBUG_ON("\t\t[create_pwm]You give me a zero frequency then the older thread had been killed and the pin %d was turn off. (duty cycle 0%%)\n", pin);
            gpio_pwm[pin - 2].state = 0;
            gpio_pwm[pin - 2].oldF = f;
            write_pin(pin, 0);
        } else if (f == 1.0) {
            PRINT_IF_DEBUG_ON("     write 1 on pin %d\n\n", pin);
            PRINT_IF_DEBUG_ON("\t\t[create_pwm]You give me a unitary frequency then the older thread had been killed and the pin %d was turn on (duty cycle 100%%)\n", pin);
            gpio_pwm[pin - 2].state = 1;
            gpio_pwm[pin - 2].oldF = f;
            write_pin(pin, 1);
        } else {
            if (gpio_pwm[pin - 2].oldF != f) {
                PRINT_IF_DEBUG_ON("\t\t[create_pwm]you give me a certaint frequency != 0 and != 'old duty cycle' then the older thread was killed and was launched new thread (duty cycle %f)\n", f);
                pthread_create(&gpio_pwm[pin - 2].pinTid, NULL, routine_new_thread, (void*)arg_pwm);
                gpio_pwm[pin-2].state = 1;
                gpio_pwm[pin-2].oldF = f;
            } //else nothing to do
        }
    }
}

void try_and_solve_directory() {
    struct stat st = {0};
    char directory[64];
    getUserId(user, TRUE);
    initArray_str(directory, 64);
    sprintf(directory,  PRIMARY_DIRECTORY, user);

    if (stat(directory, &st) == -1) {
        int res=mkdir(directory, 0700);
        if (res == -1) {
            printf("The directory [%s] was lost. Please run the \"install.sh\" to use this software\n", directory);
            exit(EXIT_FAILURE);
        }
        printf("[Warning] The directory [%s] was lost, now it has been successfully recreated\n", directory);
    }
}

int msgid;
void stopExe(int sig) {
    printf("Signal recived\n");
    turn_off_all_gpio_pwm();
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

    while(TRUE){
        try_and_solve_directory();

        msgrcv(msgid, (void*)&messageFromQueue, sizeof(struct MessageData) - sizeof(long int), 0, 0);
        PRINT_IF_DEBUG_ON("%s I received pin=%d, value=%.2f\n", argv[0], (int)messageFromQueue.pin, (float)messageFromQueue.value);

        create_pwm((float)messageFromQueue.value, (int)messageFromQueue.pin);
    }
}
