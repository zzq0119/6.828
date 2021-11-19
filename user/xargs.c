#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

char args[MAXARG][MAXPATH];
char* p[MAXARG];
int main(int argc, char *argv[])
{
    if(argc < 3){
        fprintf(2, "Syntax not supported\n");
        exit(1);
    }
    argc--;
    argv++;
    for(int i=0;i<argc;++i){
        strcpy(args[i], argv[i]);
    }
    int cursor=0;
    char buffer=0;
    while(read(0, &buffer, 1)){
        if(buffer=='\n' || buffer==' '){
            args[argc++][cursor]=0;
            cursor=0;
        }else{
            args[argc][cursor++]=buffer;
        }
    }
    for (int i = 0; i < argc; i++){
        p[i]=&args[i][0];
    }
    int pid=fork();
    if(pid==0){
        exec(p[0], p);
    }else{
        int status;
        wait(&status);
    }

    
    exit(0);
}
