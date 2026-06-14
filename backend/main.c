#include "mongoose.h"
#include <stdio.h>
#include <stdlib.h>

// Para funcionar com threads (multitarefa)
#ifdef _WIN32
#include <windows.h>
#define THREAD_RETURN DWORD WINAPI
#else
#include <pthread.h>
#define THREAD_RETURN void*
#endif

// Função para processar os dados da árvore de decisão de EPIs
void calcular_triagem(int id_familia, int id_material, int id_motivo, int id_contaminante, int risco_corte, int descaracterizado,
                      char *classe, char *lixeira, char *destino, char *acondicionamento, char *alerta, size_t max_len) {
    
    alerta[0] = '\0';

    if (descaracterizado == 0) {
        snprintf(alerta, max_len, "⚠️ REQUISITO OPERACIONAL: Recomenda-se fragmentar ou cortar o EPI antes de enviá-lo ao descarte.");
    }

    if (id_contaminante == 6) { 
        snprintf(classe, max_len, "Classe I - Perigoso (Radioativo)");
        snprintf(lixeira, max_len, "Depósito Blindado de Rejeitos Radioativos (Área Controlada)");
        snprintf(destino, max_len, "Isolamento Decadencial e Gerenciamento Estrito sob Diretrizes da CNEN.");
        snprintf(acondicionamento, max_len, "Acondicionar em recipientes de chumbo ou caixas blindadas apropriadas.");
    } else if (id_contaminante == 4) { 
        snprintf(classe, max_len, "Classe I - Perigoso (Grupo A - Infectante)");
        snprintf(lixeira, max_len, "Lixeira Branca Industrial (Resíduos Infectocontagiosos)");
        snprintf(destino, max_len, "Tratamento previo por Autoclavagem ou Incineração Industrial Certificada.");
        snprintf(acondicionamento, max_len, "Acondicionar obrigatoriamente em saco plástico branco leitoso.");
    } else if (id_contaminante == 1 || id_contaminante == 2 || id_contaminante == 3 || id_contaminante == 5 || id_familia == 1) {
        snprintf(classe, max_len, "Classe I - Perigoso (Químico / Industrial)");
        snprintf(lixeira, max_len, "Lixeira Laranja (Setores Industriais / Resíduos Perigosos)");
        snprintf(destino, max_len, "Destinação para Coprocessamento em fornos ou Incineração Térmica.");
        snprintf(acondicionamento, max_len, "Acondicionar em sacos plásticos de alta resistência química.");
    } else { 
        if (risco_corte == 1) {
            snprintf(acondicionamento, max_len, "⚠️ ALERTA: Acondicionar obrigatoriamente em RECIPIENTES RÍGIDOS E ESTANQUES.");
        } else {
            snprintf(acondicionamento, max_len, "Acondicionar em sacos de alta densidade ou caixas padronizadas.");
        }

        if (id_material == 3) { 
            snprintf(classe, max_len, "Classe II-B - Inerte (Reciclável)");
            snprintf(lixeira, max_len, "Lixeira Vermelha (Plásticos Técnicos Limpos)");
            snprintf(destino, max_len, "Encaminhar para moagem e reciclagem polimérica.");
        } else if (id_material == 2 || id_material == 1) { 
            snprintf(classe, max_len, "Classe II-A - Não Inerte (Rejeito Comum)");
            snprintf(lixeira, max_len, "Lixeira Cinza (Resíduo Geral Não Reciclável)");
            snprintf(destino, max_len, "Disposição final em Aterro Industrial Licenciado.");
        } else {
            snprintf(classe, max_len, "Classe II-B - Inerte");
            snprintf(lixeira, max_len, "Coleta Comum / Tambor Cinza");
            snprintf(destino, max_len, "Aterro Sanitário Industrial Licenciado.");
        }
    }
}

// Rota web (Netlify -> Mongoose)
void processar_triagem_avancada(struct mg_connection *c, struct mg_http_message *hm) {
    char familia_buf[10] = {0}, material_buf[10] = {0}, motivo_buf[10] = {0};
    char cont_buf[10] = {0}, corte_buf[10] = {0}, desc_buf[10] = {0};
    
    mg_http_get_var(&hm->body, "familia", familia_buf, sizeof(familia_buf));
    mg_http_get_var(&hm->body, "material", material_buf, sizeof(material_buf));
    mg_http_get_var(&hm->body, "motivo", motivo_buf, sizeof(motivo_buf));
    mg_http_get_var(&hm->body, "contaminado", cont_buf, sizeof(cont_buf));
    mg_http_get_var(&hm->body, "riscoCorte", corte_buf, sizeof(corte_buf));
    mg_http_get_var(&hm->body, "descaracterizado", desc_buf, sizeof(desc_buf));

    char classe[100], lixeira[100], destino[200], acondicionamento[300], alerta[300];
    
    calcular_triagem(atoi(familia_buf), atoi(material_buf), atoi(motivo_buf), atoi(cont_buf), 
                     atoi(corte_buf), atoi(desc_buf), classe, lixeira, destino, acondicionamento, alerta, 300);

    // Suporte a CORS para o Netlify não travar
    mg_http_reply(c, 200, 
        "Content-Type: application/json; charset=utf-8\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n",
        "{\"classe\":\"%s\",\"lixeira\":\"%s\",\"destino\":\"%s\",\"acondicionamento\":\"%s\",\"alerta\":\"%s\"}",
        classe, lixeira, destino, acondicionamento, alerta);
}

static void evento_http_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
            mg_http_reply(c, 204, 
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n", "");
            return;
        }

        if (mg_strcmp(hm->uri, mg_str("/analisar")) == 0) {
            processar_triagem_avancada(c, hm);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

// Thread paralela que mantém o servidor web escutando de fundo
THREAD_RETURN rodar_servidor_web(void *arg) {
    struct mg_mgr *mgr = (struct mg_mgr *)arg;
    for (;;) {
        mg_mgr_poll(mgr, 50);
    }
    return 0;
}

// Menu interativo via terminal (questionário numérico com limpeza de buffer)
void rodar_questionario_terminal(void) {
    int fam, mat, mot, cont, corte, desc;
    char classe[100], lixeira[100], destino[200], acondicionamento[300], alerta[300];

    // Pequena pausa para garantir que os prints do main apareçam primeiro
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif

    while(1) {
        printf("\n--- QUESTIONARIO DE TRIAGEM EPI-CYCLE ---\n");
        printf("Familia (1=Respiratoria, 2=Maos, 3=Corpo, 4=Calcados, 5=Ocular): ");
        fflush(stdout); // Força o texto a aparecer na tela IMEDIATAMENTE
        
        if (scanf("%d", &fam) <= 0) {
            // Limpa o caractere inválido se o usuário digitar algo errado
            while (getchar() != '\n'); 
            continue;
        }
        
        printf("Material (1=Borracha, 2=Couro, 3=Plastico, 4=Tecido, 5=Metal): ");
        fflush(stdout);
        scanf("%d", &mat);
        
        printf("Motivo (1=Saturacao, 2=Dano, 3=Validade): ");
        fflush(stdout);
        scanf("%d", &mot);
        
        printf("Contaminante (0=Nenhum, 4=Biologico, 6=Radioativo, 1=Quimico): ");
        fflush(stdout);
        scanf("%d", &cont);
        
        printf("Risco de Corte? (1=Sim, 0=Nao): ");
        fflush(stdout);
        scanf("%d", &corte);
        
        printf("Descaracterizado? (1=Sim, 0=Nao): ");
        fflush(stdout);
        scanf("%d", &desc);

        calcular_triagem(fam, mat, mot, cont, corte, desc, classe, lixeira, destino, acondicionamento, alerta, 300);

        printf("\n================ RESULTADO ================\n");
        printf("Classe: %s\n", classe);
        printf("Lixeira: %s\n", lixeira);
        printf("Destino: %s\n", destino);
        printf("Acondicionamento: %s\n", acondicionamento);
        if (alerta[0] != '\0') printf("Alerta: %s\n", alerta);
        printf("===========================================\n\n");
        fflush(stdout);
    }
}

int main(void) {
  
    // Força o terminal do Windows a aceitar UTF-8 e renderizar os emojis certinho
    #ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    #endif

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    const char *port = getenv("PORT");
    char listen_addr[50];
    if (port == NULL) {
        port = "3000";
        // Se estiver no computador local, força o Mongoose a ligar no IP correto
        snprintf(listen_addr, sizeof(listen_addr), "http://127.0.0.1:%s", port);
    } else {
        // Se estiver na nuvem usa o padrão para aceitar a rede externa
        snprintf(listen_addr, sizeof(listen_addr), "http://0.0.0.0:%s", port);
    }
    
    mg_http_listen(&mgr, listen_addr, evento_http_handler, NULL);

    printf("====================================================\n");
    printf("  EPI-CYCLE ENGINE v3.0 MULTI-THREAD ONLINE         \n");
    printf("  Servidor Web escutando em: %s                     \n", listen_addr);
    printf("====================================================\n");

    // Inicia a tarefa de fundo da web de acordo com o sistema operacional
#ifdef _WIN32
    CreateThread(NULL, 0, rodar_servidor_web, &mgr, 0, NULL);
#else
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, rodar_servidor_web, &mgr);
#endif

    // Roda o menu interativo por números no terminal principal
    rodar_questionario_terminal();

    mg_mgr_free(&mgr);
    return 0;
}
