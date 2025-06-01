#include "common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

void * client_thread(void *data)
{
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    ssize_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);

    if (count < 0) {
        perror("recv");
    } else if (count == 0) {
        printf("[log] client %s disconnected (recv returned 0)\n", caddrstr);
    } else {
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        snprintf(buf, BUFSZ, "remote endpoint: %.60s\n", caddrstr);
        size_t len_to_send = strlen(buf);
        if (send(cdata->csock, buf, len_to_send + 1, 0) != (ssize_t)(len_to_send + 1)) {
            logexit("send");
        }
    }

    close(cdata->csock);
    free(cdata);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }
    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            perror("accept"); 
            continue;
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(struct sockaddr_storage));

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, cdata) != 0) {
            perror("pthread_create");
            free(cdata); 
        } else {
            pthread_detach(tid);
        }
    }

    return EXIT_SUCCESS;
}