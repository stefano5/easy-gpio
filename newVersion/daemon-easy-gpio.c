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
    PRINT_IF_DEBUG_ON("spengo pwm pin: %d\n", pin_pwm + 2);
    if (gpio_pwm[pin_pwm].state == 1) {
        PRINT_IF_DEBUG_ON("    ^= era in esecuzione un thread che adesso e' stato arrestato\n");
        pthread_cancel(gpio_pwm[pin_pwm].pinTid);
        //write_pin(pin_pwm + 2, 0); may be is done ?
    }

}

void *routine_new_thread(void* param) {
    _arg_pwm *arg_pwm = (_arg_pwm*)param;
    float dutyCycle = arg_pwm->dutyCycle;
    int pin = arg_pwm->pin;
    free(arg_pwm);
    PRINT_IF_DEBUG_ON("Routine thread: f=%.2f, pin=%d\n", dutyCycle, pin);
    
    f_pwm_gpio(dutyCycle, pin); //this function blocks the execution
    PRINT_IF_DEBUG_ON("f_pwm_gpio conclusa\n");
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
        PRINT_IF_DEBUG_ON("\t[create_pwm]Il pin  risultava inutilizzato. Ora e' partito\n");
        if (f == 0.0) {
            PRINT_IF_DEBUG_ON("scrivo 0 al pin %d\n\n", pin);
            gpio_pwm[pin - 2].state = 0;
            write_pin(pin, 0);
        } else if (f == 1.0) {
            PRINT_IF_DEBUG_ON("scrivo 1 al pin %d\n\n", pin);
            gpio_pwm[pin - 2].state = 1;
            write_pin(pin, 1);
        } else {
            PRINT_IF_DEBUG_ON("generazione pwm\n");
            gpio_pwm[pin - 2].state = 1;
            pthread_create(&gpio_pwm[pin - 2].pinTid, NULL, routine_new_thread, (void*)arg_pwm);
            gpio_pwm[pin - 2].oldF = f;
        }
    } else {
        PRINT_IF_DEBUG_ON("\t[create_pwm]Il pin era utilizzato, verra' fermato e verra' generata una nuova pwm\n");
        if (f == 0.0) {
            PRINT_IF_DEBUG_ON("scrivo 0 al pin %d\n\n", pin);
            printf_d("\t\t[create_pwm]Hai passato una frequenza nulla, il thread precedente e' stato bloccato e il pin spento\n");
            gpio_pwm[pin - 2].state = 0;
            gpio_pwm[pin - 2].oldF = f;
            write_pin(pin, 0);
        } else if (f == 1.0) {
            PRINT_IF_DEBUG_ON("scrivo 1 al pin %d\n\n", pin);
            printf_d("\t\t[create_pwm]Hai passato una frequenza unitaria, il thread precedente e' stato bloccato e il pin acceso\n");
            gpio_pwm[pin - 2].state = 1;
            gpio_pwm[pin - 2].oldF = f;
            write_pin(pin, 1);
        } else {
            if (gpio_pwm[pin - 2].oldF != f) {
                printf_d("\t\t[create_pwm]hai passato una frequenza != 0, il thread precendete e' stato bloccato ed e' stato creato un nuovo thread\n");
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
            printf("The directory [%s] is lost. Please run the \"install.sh\" to use this software\n", directory);
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
        if (enable_print)
            printf("%s Ho ricevuto il pin=%d, valore=%.2f\n", argv[0], (int)messageFromQueue.pin, (float)messageFromQueue.value);   //translate me  //TODO

        create_pwm((float)messageFromQueue.value, (int)messageFromQueue.pin);
    }
}
