#include "common.h"
#include "protocolo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("uso: %s <v4|v6> <porta servidor>\n", argv[0]);
    printf("exemplo: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int determinar_vencedor(int acao_cliente, int acao_servidor) {
    if (acao_cliente == acao_servidor) {
        return -1;
    }
    if (acao_cliente == 0 && (acao_servidor == 2 || acao_servidor == 3)) return 1;
    if (acao_cliente == 1 && (acao_servidor == 0 || acao_servidor == 4)) return 1;
    if (acao_cliente == 2 && (acao_servidor == 1 || acao_servidor == 3)) return 1;
    if (acao_cliente == 3 && (acao_servidor == 1 || acao_servidor == 4)) return 1;
    if (acao_cliente == 4 && (acao_servidor == 0 || acao_servidor == 2)) return 1;

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    srand(time(NULL));

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
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char protocol_mode_str[20];
    if (strcmp(argv[1], "v4") == 0) {
        strcpy(protocol_mode_str, "IPv4");
    } else if (strcmp(argv[1], "v6") == 0) {
        strcpy(protocol_mode_str, "IPv6");
    } else {
        strcpy(protocol_mode_str, "modo desconhecido");
    }
    printf("Servidor iniciado em modo %s na porta %s. Aguardando conexão...\n", protocol_mode_str, argv[2]);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }
        printf("Cliente conectado.\n");

        int vitorias_cliente = 0;
        int vitorias_servidor = 0;
        int jogar_novamente = 1;

        while (jogar_novamente) {
            GameMessage msg_rodada_atual;
            int resultado_rodada = -1;

            while (resultado_rodada == -1) {
                printf("Apresentando as opções para o cliente.\n");
                GameMessage msg_req_jogada;
                msg_req_jogada.type = MSG_REQUEST;
                strcpy(msg_req_jogada.message, "Escolha sua acao (0-4):");
                if (send(csock, &msg_req_jogada, sizeof(GameMessage), 0) <= 0) {
                    perror("send request jogada");
                    close(csock);
                    goto proximo_cliente;
                }

                GameMessage msg_resp_cliente;
                memset(&msg_resp_cliente, 0, sizeof(GameMessage));
                ssize_t bytes_recebidos_resp = recv(csock, &msg_resp_cliente, sizeof(GameMessage), 0);
                if (bytes_recebidos_resp <= 0) {
                    printf("Cliente desconectou ou erro no recv (resp jogada).\n");
                    close(csock);
                    goto proximo_cliente;
                }
                printf("Cliente escolheu %d.\n", msg_resp_cliente.client_action);

                if (msg_resp_cliente.client_action < 0 || msg_resp_cliente.client_action > 4) {
                    printf("Erro: opção inválida de jogada.\n");
                    GameMessage msg_erro;
                    msg_erro.type = MSG_ERROR;
                    strcpy(msg_erro.message, "Por favor, selecione um valor de 0 a 4.");
                    if (send(csock, &msg_erro, sizeof(GameMessage), 0) <= 0) {
                        perror("send error jogada invalida");
                        close(csock);
                        goto proximo_cliente;
                    }
                    continue;
                }

                int acao_servidor = rand() % 5;
                printf("Servidor escolheu aleatoriamente %d.\n", acao_servidor);

                resultado_rodada = determinar_vencedor(msg_resp_cliente.client_action, acao_servidor);

                msg_rodada_atual.type = MSG_RESULT;
                msg_rodada_atual.client_action = msg_resp_cliente.client_action;
                msg_rodada_atual.server_action = acao_servidor;
                msg_rodada_atual.result = resultado_rodada;

                if (resultado_rodada == 1) {
                    strcpy(msg_rodada_atual.message, "Vitória!");
                } else if (resultado_rodada == 0) {
                    strcpy(msg_rodada_atual.message, "Derrota!");
                } else {
                    strcpy(msg_rodada_atual.message, "Empate!");
                }

                if (send(csock, &msg_rodada_atual, sizeof(GameMessage), 0) <= 0) {
                    perror("send result");
                    close(csock);
                    goto proximo_cliente;
                }

                if (resultado_rodada == -1) {
                    printf("Jogo empatado.\n");
                    printf("Solicitando ao cliente mais uma escolha.\n");
                }
            }

            if (resultado_rodada == 1) {
                vitorias_cliente++;
            } else if (resultado_rodada == 0) {
                vitorias_servidor++;
            }
            printf("Placar atualizado: Cliente %d x %d Servidor\n", vitorias_cliente, vitorias_servidor);

            while (1) {
                printf("Perguntando novamente se o cliente deseja jogar novamente.\n");
                GameMessage msg_pa_req;
                msg_pa_req.type = MSG_PLAY_AGAIN_REQUEST;
                strcpy(msg_pa_req.message, "Deseja jogar novamente? (1-Sim, 0-Nao)");
                if (send(csock, &msg_pa_req, sizeof(GameMessage), 0) <= 0) {
                    perror("send play again req");
                    close(csock);
                    goto proximo_cliente;
                }

                GameMessage msg_pa_resp;
                memset(&msg_pa_resp, 0, sizeof(GameMessage));
                ssize_t bytes_recebidos_pa = recv(csock, &msg_pa_resp, sizeof(GameMessage), 0);
                if (bytes_recebidos_pa <= 0) {
                    printf("Cliente desconectou ou erro no recv (play again).\n");
                    close(csock);
                    goto proximo_cliente;
                }

                if (msg_pa_resp.client_action == 0 || msg_pa_resp.client_action == 1) {
                    jogar_novamente = msg_pa_resp.client_action;
                    if (!jogar_novamente) {
                        printf("Cliente não deseja jogar novamente.\n");
                        printf("Enviando placar final.\n");
                    } else {
                        printf("Cliente deseja jogar novamente.\n");
                    }
                    break;
                } else {
                    printf("Erro: resposta inválida para jogar novamente.\n");
                    GameMessage msg_erro_pa;
                    msg_erro_pa.type = MSG_ERROR;
                    strcpy(msg_erro_pa.message, "Por favor, digite 1 para jogar novamente ou 0 para encerrar.");
                    if (send(csock, &msg_erro_pa, sizeof(GameMessage), 0) <= 0) {
                        perror("send error play again invalido");
                        close(csock);
                        goto proximo_cliente;
                    }
                }
            }

            if (!jogar_novamente) {
                GameMessage msg_fim;
                msg_fim.type = MSG_END;
                msg_fim.client_wins = vitorias_cliente;
                msg_fim.server_wins = vitorias_servidor;
                sprintf(msg_fim.message, "Fim de jogo!\nPlacar final: Você %d x %d Servidor", vitorias_cliente, vitorias_servidor);
                if (send(csock, &msg_fim, sizeof(GameMessage), 0) <= 0) {
                    perror("send end game");
                    close(csock);
                    goto proximo_cliente;
                }
            }
        }

        printf("Encerrando conexão.\n");
        close(csock);
        printf("Cliente desconectado.\n");

        proximo_cliente:;
    }

    return EXIT_SUCCESS;
}