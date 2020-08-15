/*
  tracie: Tracing I/O on Executables
  v1.0, released 2020-08-15
  Developed by CsokiCraft
  tracie is free software; see LICENSE
  
  Tracie works by intercepting `read()` and `write()`
  calls executed on a certain file (known as the
  'target file', given as the $TRACIE_TARGET
  environmental variable). Results of `read()`
  are written to TRACIE_PIPE_R, and bytes written
  by `write()` are duplicated to TRACIE_PIPE_W.
  
  To use tracie, you need to:
  1. Set $TRACIE_TARGET
  2. LD_PRELOAD this `.so` and execute the program-under-test
  3. Start reading on TRACIE_PIPE_R and TRACIE_PIPE_W.
     tracie will wait for the pipes to be read before
     the main program code will be executed.
  That's it! The pipes will auto-close as the program quits.
 */

#include <sys/types.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define TRACIE_PIPE_R "/tmp/tracie-pipe-r"
#define TRACIE_PIPE_W "/tmp/tracie-pipe-w"

ssize_t (*_orig_read)(int fd, void *buf, size_t count);
ssize_t (*_orig_write)(int fd, const void *buf, size_t count);
FILE *_tracie_pipe_r, *_tracie_pipe_w;
char* _tracie_target;
size_t _tracie_target_len=0;

// Set up _orig_*() fnptrs using dlsym()
void _init_tracie() __attribute__((constructor));
//void _exit_tracie() __attribute__(( destructor));

void _init_tracie(){
    printf("***tracie v1.0\n");
    _orig_read =dlsym((void*)-1, "read");
    _orig_write=dlsym((void*)-1, "write");
    _tracie_target=getenv("TRACIE_TARGET");
    if(!_tracie_target)
         printf("***TRACIE_TARGET not set!\n");
    else
        _tracie_target_len=strlen(_tracie_target);
    //printf("***Waiting for connection on `%s'\n", TRACIE_PIPE);
    printf("***Waiting for connection...\n");
    if(!(_tracie_pipe_r=fopen(TRACIE_PIPE_R, "w")))
        perror("Unable to open read pipe");
    if(!(_tracie_pipe_w=fopen(TRACIE_PIPE_W, "w")))
        perror("Unable to open write pipe");
    printf("***Connected\n");
}

int _is_tracie_target(int fd){
    char fdlink[]="/proc/self/fd/XXX", fname[_tracie_target_len+1];
    snprintf(fdlink, strlen(fdlink), "/proc/self/fd/%d", fd);
    readlink(fdlink, fname, _tracie_target_len);
    
    return !strncmp(fname, _tracie_target, _tracie_target_len);
}

ssize_t read(int fd, void *buf, size_t count){
    ssize_t ret=_orig_read(fd, buf, count);
    
    if(_is_tracie_target(fd)){
        //fprintf(_tracie_pipe, "r[%d] ", fd);
        for(size_t i=0;i<ret;i++)
            fputc(((char*)buf)[i], _tracie_pipe_r);
        //fputc('\n', _tracie_pipe);
        fflush(_tracie_pipe_r);
    }
    
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count){
    ssize_t ret=_orig_write(fd, buf, count);
    
    if(_is_tracie_target(fd)){
        //fprintf(_tracie_pipe, "w[%d] ", fd);
        for(size_t i=0;i<ret;i++)
            fputc(((char*)buf)[i], _tracie_pipe_w);
        //fputc('\n', _tracie_pipe);
        fflush(_tracie_pipe_w);
    }
    
    return ret;
}
