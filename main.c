#include "mongoose.h"
#include <stdio.h>
#include <stdlib.h>

static const char *s_http_port = "http://localhost:3000";

void processar_triagem_avancada(struct mg_connection *c, struct mg_http_message *hm) {
    char familia_buf[10] = {0}, material_buf[10] = {0}, motivo_buf[10] = {0};
    char cont_buf[10] = {0}, corte_buf[10] = {0}, desc_buf[10] = {0};
    
    // O C extrai os novos parâmetros enviados pelo site
    mg_http_get_var(&hm->body, "familia", familia_buf, sizeof(familia_buf));
    mg_http_get_var(&hm->body, "material", material_buf, sizeof(material_buf));
    mg_http_get_var(&hm->body, "motivo", motivo_buf, sizeof(motivo_buf));
    mg_http_get_var(&hm->body, "contaminado", cont_buf, sizeof(cont_buf));
    mg_http_get_var(&hm->body, "riscoCorte", corte_buf, sizeof(corte_buf));
    mg_http_get_var(&hm->body, "descaracterizado", desc_buf, sizeof(desc_buf));

    int id_familia = atoi(familia_buf); // 1=Respiratoria, 2=Maos, 3=Corpo, 4=Calcados, 5=Ocular
    int id_material = atoi(material_buf); // 1=Borracha, 2=Couro, 3=Plastico, 4=Tecido, 5=Metal
    int id_motivo = atoi(motivo_buf); // 1=Saturacao, 2=Dano, 3=Validade/CA Vencido
    int id_contaminante = atoi(cont_buf); // 0=Nenhum, 1=Quimico Org, 2=Quimico Corrosivo, 3=Metais, 4=Biologico, 5=Oleo, 6=Radioativo
    int risco_corte = atoi(corte_buf);
    int descaracterizado = atoi(desc_buf);

    char classe[100], lixeira[100], destino[200], acondicionamento[300], alerta[300] = "";

    // Lembrete ético de descaracterização (Diretriz de Segurança)
    if (descaracterizado == 0) {
        snprintf(alerta, sizeof(alerta), "⚠️ REQUISITO OPERACIONAL: Recomenda-se fragmentar ou cortar o EPI antes de enviá-lo ao descarte para mitigar riscos de mercado clandestino e reuso ilegal.");
    }

    // ÁRVORE DE DECISÃO NORMATIVA (NBR 10004 / NR-06)
    if (id_contaminante == 6) { // Radioativo
        snprintf(classe, sizeof(classe), "Classe I - Perigoso (Radioativo)");
        snprintf(lixeira, sizeof(lixeira), "Depósito Blindado de Rejeitos Radioativos (Área Controlada)");
        snprintf(destino, sizeof(destino), "Isolamento Decadencial e Gerenciamento Estrito sob Diretrizes da CNEN.");
        snprintf(acondicionamento, sizeof(acondicionamento), "Acondicionar em recipientes de chumbo ou caixas blindadas apropriadas com o símbolo de radiação ionizante.");

    } else if (id_contaminante == 4) { // Biológico
        snprintf(classe, sizeof(classe), "Classe I - Perigoso (Grupo A - Infectante)");
        snprintf(lixeira, sizeof(lixeira), "Lixeira Branca Industrial (Resíduos Infectocontagiosos)");
        snprintf(destino, sizeof(destino), "Tratamento prévio por Autoclavagem ou Incineração Industrial Certificada.");
        snprintf(acondicionamento, sizeof(acondicionamento), "Acondicionar obrigatoriamente em saco plástico branco leitoso, com o símbolo de risco biológico bem visível.");

    } else if (id_contaminante == 1 || id_contaminante == 2 || id_contaminante == 3 || id_contaminante == 5 || id_familia == 1) {
        // Químicos Inflamáveis, Corrosivos, Metais Pesados, Óleos ou Proteção Respiratória (Filtros herdam perigo)
        snprintf(classe, sizeof(classe), "Classe I - Perigoso (Químico / Industrial)");
        snprintf(lixeira, sizeof(lixeira), "Lixeira Laranja (Setores Industriais / Resíduos Perigosos)");
        snprintf(destino, sizeof(destino), "Destinação para Coprocessamento em fornos de cimento ou Incineração Térmica Dedicada.");
        snprintf(acondicionamento, sizeof(acondicionamento), "Acondicionar em sacos plásticos de alta resistência química ou tambores herméticos, rotulados com identificação clara da substância.");

    } else { // Nenhuma contaminação / Uso comum
        
        // Se houver risco físico de perfuração
        if (risco_corte == 1) {
            snprintf(acondicionamento, sizeof(acondicionamento), "⚠️ ALERTA DE SEGURANÇA: Acondicionar obrigatoriamente em RECIPIENTES RÍGIDOS E ESTANQUES (como caixas amarelas) para evitar acidentes com os operadores de coleta.");
        } else {
            snprintf(acondicionamento, sizeof(acondicionamento), "Acondicionar em sacos de alta densidade ou caixas padronizadas de coleta seletiva interna.");
        }

        // Triagem baseada na circularidade do material predominante
        if (id_material == 3) { // Plástico Técnico Limpo (Capacete, óculos intactos)
            snprintf(classe, sizeof(classe), "Classe II-B - Inerte (Reciclável)");
            snprintf(lixeira, sizeof(lixeira), "Lixeira Vermelha (Plásticos Técnicos Limpos)");
            snprintf(destino, sizeof(destino), "Encaminhar para moagem, descaracterização física e reciclagem polimérica.");
        } else if (id_material == 2 || id_material == 1) { // Couro ou Borracha (Botinas, luvas comuns de raspa)
            snprintf(classe, sizeof(classe), "Classe II-A - Não Inerte (Rejeito Comum)");
            snprintf(lixeira, sizeof(lixeira), "Lixeira Cinza (Resíduo Geral Não Reciclável Manualmente)");
            snprintf(destino, sizeof(destino), "Disposição final em Aterro Industrial Licenciado ou Logística Reversa se previsto pelo fabricante.");
        } else {
            snprintf(classe, sizeof(classe), "Classe II-B - Inerte");
            snprintf(lixeira, sizeof(lixeira), "Coleta Comum / Tambor Cinza");
            snprintf(destino, sizeof(destino), "Aterro Sanitário Industrial Licenciado.");
        }
    }

    mg_http_reply(c, 200, "Content-Type: application/json; charset=utf-8\r\n",
        "{\"classe\":\"%s\",\"lixeira\":\"%s\",\"destino\":\"%s\",\"acondicionamento\":\"%s\",\"alerta\":\"%s\"}",
        classe, lixeira, destino, acondicionamento, alerta);
}

static void evento_http_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_strcmp(hm->uri, mg_str("/analisar")) == 0) {
            processar_triagem_avancada(c, hm);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

int main(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    printf("====================================================\n");
    printf("  EPI-CYCLE ADVANCED ENGINE v3.0 ONLINE             \n");
    printf("  Acesse o software em: %s             \n", s_http_port);
    printf("====================================================\n");
    
    mg_http_listen(&mgr, s_http_port, evento_http_handler, NULL);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    return 0;
}