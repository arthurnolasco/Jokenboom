#pragma once

#define MSG_SIZE 256

typedef enum {
    MSG_REQUEST,             
    MSG_RESPONSE,            
    MSG_RESULT,              
    MSG_PLAY_AGAIN_REQUEST,  
    MSG_PLAY_AGAIN_RESPONSE, 
    MSG_ERROR,               
    MSG_END                  
} MessageType;

typedef struct {
    MessageType type;
    int client_action;      
    int server_action;      
    int result;             
    int client_wins;        
    int server_wins;        
    char message[MSG_SIZE]; 
} GameMessage;


const char* obter_nome_acao(int acao) {
    switch (acao) {
        case 0: return "Nuclear Attack";
        case 1: return "Intercept Attack";
        case 2: return "Cyber Attack";
        case 3: return "Drone Strike";
        case 4: return "Bio Attack";
        default: return "Desconhecida";
    }
}