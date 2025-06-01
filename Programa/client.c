#include "common.h"
#include "protocolo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("uso: %s <IP servidor> <porta servidor>\n", argv[0]);
    printf("exemplo: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void limpar_buffer_entrada() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    int s;
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }
    
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    printf("Conectado ao servidor.\n"); 
    GameMessage msg_recebida;
    GameMessage msg_para_enviar;

    while (1) {
        memset(&msg_recebida, 0, sizeof(GameMessage));
        ssize_t count = recv(s, &msg_recebida, sizeof(GameMessage), 0);

        if (count == 0) {
            printf("Servidor encerrou a conexao.\n");
            break;
        }
        if (count < 0) {
            logexit("recv");
        }

        switch (msg_recebida.type) {
            case MSG_REQUEST:
                printf("\nEscolha sua jogada:\n");
                printf("0 - %s\n", obter_nome_acao(0));    
                printf("1 - %s\n", obter_nome_acao(1));
                printf("2 - %s\n", obter_nome_acao(2));
                printf("3 - %s\n", obter_nome_acao(3));
                printf("4 - %s\n", obter_nome_acao(4));

                int acao_jogador;
                if (scanf("%d", &acao_jogador) != 1) {
                    limpar_buffer_entrada();
                    acao_jogador = -1;
                }
                limpar_buffer_entrada();

                msg_para_enviar.type = MSG_RESPONSE;
                msg_para_enviar.client_action = acao_jogador;
                if (send(s, &msg_para_enviar, sizeof(GameMessage), 0) <= 0) {
                    logexit("send response jogada");
                }
                break;

            case MSG_RESULT:
                printf("\nVocê escolheu: %s\n", obter_nome_acao(msg_recebida.client_action));
                printf("Servidor escolheu: %s\n", obter_nome_acao(msg_recebida.server_action));
                printf("Resultado: %s\n", msg_recebida.message);
                break;

            case MSG_PLAY_AGAIN_REQUEST:
                printf("\nDeseja jogar novamente?\n");
                printf("1 - Sim\n");
                printf("0 - Não\n");

                int jogar_novamente;
                if (scanf("%d", &jogar_novamente) != 1) {
                    limpar_buffer_entrada();
                    jogar_novamente = -1;
                }
                limpar_buffer_entrada();

                msg_para_enviar.type = MSG_PLAY_AGAIN_RESPONSE;
                msg_para_enviar.client_action = jogar_novamente;
                if (send(s, &msg_para_enviar, sizeof(GameMessage), 0) <= 0) {
                    logexit("send play again response");
                }
                break;

            case MSG_ERROR:
                printf("\n%s\n", msg_recebida.message);
                break;
            
            case MSG_END:
                printf("\n%s\n", msg_recebida.message);
                printf("Obrigado por jogar!\n");
                close(s);
                return EXIT_SUCCESS;

            default:
                printf("Mensagem desconhecida ou inesperada do servidor: tipo %d\n", msg_recebida.type);
                break;
        }
    }

    close(s);
    return EXIT_SUCCESS;
}