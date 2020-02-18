#ifndef MANAGEMENT_ARG_C
#define MANAGEMENT_ARG_C

bool enable_print;

void print_syntax() {
    printf("name sw: %s\n", name_exe);
    printf("This software receve command from message queue from easy-gpio software\n");
    printf("Don't use you this software, must be systemd to use that\n");
    printf("Please follow the guide\n");
    printf("Else, if you would improve this code you can use '-p' parameter in order to enable all debug print\n");
    exit(EXIT_SUCCESS);
}

//unused
void managementCommand(int i) {
	if (!strcasecmp(command[i], "-h") || !strcasecmp(command[i], "--help")) {
        print_syntax();
	} else if (!strcasecmp(command[i], "-p") || !strcasecmp(command[i], "--enable-print")) {
        debug_on();
	}
}

//unused
void managementParam(int i) {
	if (!strcasecmp(param[i], "param") || !strcasecmp(param[i], "parameter")) {
        
	} else if (!strcasecmp(param[i], "") || !strcmp(param[i], "")) {

	}
}

void managementArg() {
	if (countParam != 0) {
		for (PRECONDITION countParam POSTCONDITION) {
			managementParam(i);
		}
	}

	if (countCommand != 0) {
		for (PRECONDITION countCommand POSTCONDITION) {
			managementCommand(i);
		}
	}
}
#endif
