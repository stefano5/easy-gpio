#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<time.h>
#include<pwd.h>
#include<unistd.h>
#include<signal.h>


#define TRUE 1
#define FALSE 0

#define FFLUSH while(getchar()!='\n')

int __debug=0;
void printf_d(char *s){
	if (__debug)
		printf("%s\n", s);
}

void debug_on(){
	printf("[MODALITA' DEBUG: ON]\n");
	__debug=1;
}

void debug_off(){
	printf("[MODALITA' DEBUG: OFF]\n");
	__debug=0;
}

int debug(){return __debug;}

char *ltrim(char *s){
	while(isspace(*s)) s++;
	return s;
}

char *rtrim(char *s){
	char* back = s + strlen(s);
	while(isspace(*--back));
	*(back+1) = '\0';
	return s;
}

char *trim(char *s){
	return rtrim(ltrim(s)); 
}

char* getDate(){
	time_t mytime = time(NULL);
	return trim(ctime(&mytime));
}

void readString(char s[], int dim){
        int i = 0;
        for (i = 0; i<dim - 1; i++)
                if ((s[i] = getchar()) == '\n') break;
        if (i == dim - 1) FFLUSH;
        s[i] = '\0';
}

void getUserId(char *user, int dim_str){
        int uid;
        if (geteuid()==0)	//We don't want ROOT!!
                uid=1000;
        else
                uid=getuid();
     
     if (getpwuid(uid)->pw_name == NULL){
                printf("Function getpwuid failed, what is your user id? [userID@namePC:~$]\n>");
                readString(user, dim_str);
        } else {
		strcpy(user, getpwuid(uid)->pw_name);
	}
}

int subString(char *str, int start, int end){
	int len = strlen(str);
	if (end<0) end=len;
	if (start<0) start=0;
	if (end>len || start > len || start > end) {printf("\n\nError substring\n\n"); return -1; }
	char *str_temp = (char *)malloc(sizeof(char) * end-start); //of course, sizeof(char) = 1, but to leggibility...
	int new_index=0;
	for (int i=0;i<len; i++){
		if(i>=start && i<end){
			str_temp[new_index++]=str[i];
		}
	}
	strcpy(str, str_temp);
	return new_index;
}


/*
 * @return: 	1 = success
 * 		-1= fail
 * @param example: "w" to write, "a" to append ecc
 * NOT use "r" in mode!!!
*/
int writeFile(char *path, char *text, char *mode){	
	if (strcmp(mode, "r")) 
		return -1;
	FILE *f=fopen(path, mode);
	if (f==NULL) 
		return -1;
	int success=fprintf(f, "%s", text);
	fclose(f);
	if (success==-1) return 0;
	return 1;
}



