#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char buffer[512];
void find(char* filename, int prev, int end){
    int fd;
    if((fd = open(buffer, 0)) < 0){
        fprintf(2, "cannot open path.\n");
        return ;
    }
    struct stat st;
    if(fstat(fd, &st) < 0){
        fprintf(2, "cannot stat path.\n");
        close(fd);
        return;
    }
    struct dirent de;
    switch(st.type){
        case T_FILE:
            if(strcmp(filename, buffer + prev)==0){
                printf("%s\n", buffer);
            }
            break;
        case T_DIR:
            buffer[end++]='/';
            buffer[end]='\0';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
                    continue;
                }
                int len=strlen(de.name);
                strcpy(buffer+end, de.name);
                if(stat(buffer, &st) < 0){
                    printf("cannot stat %s\n", buffer);
                    continue;
                }
                find(filename, end, end+len);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]){
    if(argc < 3){
        fprintf(2, "Usage: find <dir> <filename>\n");
        exit(1);
    }
    int len = strlen(argv[1]);
    if(argv[1][len-1]=='/'){
        --len;
    }
    
    strncpy(buffer, argv[1], len);
    find(argv[2], 0, len);

    exit(0);
}
