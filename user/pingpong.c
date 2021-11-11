#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int fd1[2];
    int fd2[2];
    pipe(fd1);
    pipe(fd2);
    int pid=fork();
    if(pid==0){
        char buffer[2];
        close(fd2[1]);
        close(fd1[0]);
        read(fd2[0],buffer,1);
        write(fd1[1],"a",1);
        close(fd1[1]);
        printf("%d: received ping\n", getpid());
    }else{
        char buffer[20];
        close(fd2[0]);
        close(fd1[1]);
        write(fd2[1],"b",1);
        close(fd2[1]);
        read(fd1[0],buffer,2);
        sleep(20);
        printf("%d: received pong\n", getpid());
    }

    exit(0);
}
