#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int mian(int argc, char *argv[]) {
  int child_p[2];
  int parent_p[2];
  pipe(child_p);
  pipe(parent_p);

  int pid = fork();

  if(pid == 0) {
    // this is the child
    
    close(child_p[1]);
    close(parent_p[0]);

    // read from parent
    int _pid;
    read(child_p[0], &_pid, sizeof _pid);
    // we read from child_p because the parent sends information
    // to the child process via this pipe
    close(child_p[0]);

    printf("%d: received ping from pid %d\n", getpid(), _pid);
    
    // reuse _pid and set the pid of the child process
    _pid = getpid();
    // send pong (response) to praent
    write(parent_p[1], &_pid, sizeof _pid);
    close(parent_p[1]);
  } else if(pid > 0) {
    // this is the parent
    close(child_p[0]);
    close(parent_p[1]);
    
    int _pid = getpid();
    write(child_p[1], &_pid, sizeof _pid);
    // send ping to child process with the current process's _pid
    close(child_p[1]);

    // wait to receive pong from child and read its _pid
    read(parent_p[0], &_pid, sizeof pid);
    printf("%d: received pong from pid %d\n", getpid(), _pid);
    close(parent_p[0]);
  }

  exit(0);
}

