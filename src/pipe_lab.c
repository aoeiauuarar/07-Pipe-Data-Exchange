#include "pipe_lab.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
  Task A – one-way pipe
 */
int task_one_way(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    fflush(stdout); 
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(fd[1]);
        char buf[64];
        ssize_t n = read(fd[0], buf, sizeof(buf) - 1);
        if (n >= 0) {
            buf[n] = '\0';
        } else {
            buf[0] = '\0';
        }
        printf("child received: %s\n", buf);
        close(fd[0]);
        exit(0);
    }

    close(fd[0]);
    if (write(fd[1], "ping", 4) == -1) { perror("write"); return 1; }
    close(fd[1]);
    waitpid(pid, NULL, 0);
    printf("parent: sent ping\n");
    return 0;
}

/*
  Task B – two-way pipe
 */
int task_two_way(void) {
    int p2c[2], c2p[2];
    if (pipe(p2c) == -1 || pipe(c2p) == -1) { perror("pipe"); return 1; }

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(p2c[1]);
        close(c2p[0]);

        int val;
        if (read(p2c[0], &val, sizeof(val)) > 0) {
            val *= 2;
            printf("child computed: %d\n", val);
            write(c2p[1], &val, sizeof(val));
        }

        close(p2c[0]);
        close(c2p[1]);
        exit(0);
    }

    close(p2c[0]);
    close(c2p[1]);

    int val = 21;
    write(p2c[1], &val, sizeof(val));
    close(p2c[1]);

    int res;
    if (read(c2p[0], &res, sizeof(res)) > 0) {
        printf("parent received: %d\n", res);
    }

    close(c2p[0]);
    waitpid(pid, NULL, 0);
    return 0;
}

/*
  Task C – EOF limitation
 */
int task_eof(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    fflush(stdout); 
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(fd[1]);
        char buf[64];
        ssize_t n;
        size_t total_bytes = 0;
        while ((n = read(fd[0], buf, sizeof(buf))) > 0) {
            total_bytes += (size_t)n;
        }
        printf("child: got EOF after %zu bytes\n", total_bytes);
        close(fd[0]);
        exit(0);
    }

    close(fd[0]);
    write(fd[1], "hello", 5);
    close(fd[1]);
    waitpid(pid, NULL, 0);
    return 0;
}

/*
  Task D – SIGPIPE / EPIPE limitation
 */
int task_broken_pipe(void) {
    int data_fd[2];
    int sync_fd[2];
    if (pipe(data_fd) == -1 || pipe(sync_fd) == -1) { perror("pipe"); return 1; }

    fflush(stdout); 
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(sync_fd[0]);
        close(data_fd[0]);
        close(data_fd[1]);

        char ack = '1';
        write(sync_fd[1], &ack, 1);
        close(sync_fd[1]);
        exit(0);
    }

    close(sync_fd[1]);

    char dummy;
    read(sync_fd[0], &dummy, 1);
    close(sync_fd[0]);
    close(data_fd[0]);

    signal(SIGPIPE, SIG_IGN);

    char byte = 'x';
    ssize_t ret = write(data_fd[1], &byte, 1);
    if (ret == -1 && errno == EPIPE) {
        printf("parent: broken pipe detected\n");
    }

    close(data_fd[1]);
    waitpid(pid, NULL, 0);
    return 0;
}

int main(void) {
    if (task_one_way() != 0)     { fprintf(stderr, "task_one_way failed\n");     return 1; }
    if (task_two_way() != 0)     { fprintf(stderr, "task_two_way failed\n");     return 1; }
    if (task_eof() != 0)         { fprintf(stderr, "task_eof failed\n");         return 1; }
    if (task_broken_pipe() != 0) { fprintf(stderr, "task_broken_pipe failed\n"); return 1; }
    printf("all tasks done\n");
    return 0;
}