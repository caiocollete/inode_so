#include "sistema_arquivos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Função auxiliar para contar blocos de um arquivo recursivamente
int contar_blocos_arquivo_recursivo(int inode_num, bool* blocos_visitados) {
    if (inode_num < 0 || inode_num >= NUMERO_TOTAL_BLOCOS || disco[inode_num].tipo != 'I') {
        return 0;
    }
    
    if (blocos_visitados[inode_num]) {
        return 0; // Evitar loops infinitos
    }
    blocos_visitados[inode_num] = true;
    
    struct Inode inode;
    memcpy(&inode, disco[inode_num].dados, sizeof(struct Inode));
    
    int total_blocos = 1; // Contar o próprio bloco do inode
    
    // Percorrer ponteiros diretos
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco = inode.ponteiros[i];
            if (bloco >= 0 && bloco < NUMERO_TOTAL_BLOCOS) {
                if (disco[bloco].tipo == 'A' || disco[bloco].tipo == 'I') {
                    total_blocos++;
                    if (disco[bloco].tipo == 'I') {
                        // Bloco de ponteiros indiretos - contar recursivamente
                        total_blocos += contar_blocos_indiretos(bloco, blocos_visitados);
                    }
                }
            }
        }
    }
    
    return total_blocos;
}

// Função auxiliar para contar blocos indiretos
int contar_blocos_indiretos(int bloco_ponteiros, bool* blocos_visitados) {
    if (bloco_ponteiros < 0 || bloco_ponteiros >= NUMERO_TOTAL_BLOCOS || disco[bloco_ponteiros].tipo != 'I') {
        return 0;
    }
    
    if (blocos_visitados[bloco_ponteiros]) {
        return 0;
    }
    blocos_visitados[bloco_ponteiros] = true;
    
    int total = 0;
    int ponteiros_por_bloco = 500 / sizeof(int);
    int* ponteiros = (int*)(disco[bloco_ponteiros].dados);
    
    for (int i = 0; i < ponteiros_por_bloco; i++) {
        if (ponteiros[i] != -1) {
            int bloco = ponteiros[i];
            if (bloco >= 0 && bloco < NUMERO_TOTAL_BLOCOS) {
                total++;
                if (disco[bloco].tipo == 'I') {
                    // Mais um nível de indireção
                    total += contar_blocos_indiretos(bloco, blocos_visitados);
                }
            }
        }
    }
    
    return total;
}

// Relatório 1: Blocos Ocupados por Arquivo
void report_blocos_ocupados(char* nome_arquivo) {
    if (nome_arquivo == NULL || strlen(nome_arquivo) == 0) {
        printf("Erro: Nome do arquivo é obrigatório\n");
        return;
    }
    
    // Encontrar o inode do arquivo
    int inode_arquivo = verificar_arquivo_existe(nome_arquivo);
    if (inode_arquivo == -1) {
        printf("Erro: Arquivo '%s' não encontrado\n", nome_arquivo);
        return;
    }
    
    // Criar array para evitar loops infinitos
    bool* blocos_visitados = calloc(NUMERO_TOTAL_BLOCOS, sizeof(bool));
    if (blocos_visitados == NULL) {
        printf("Erro: Falha na alocação de memória\n");
        return;
    }
    
    int total_blocos = contar_blocos_arquivo_recursivo(inode_arquivo, blocos_visitados);
    
    printf("=== RELATÓRIO 1: BLOCOS OCUPADOS POR ARQUIVO ===\n");
    printf("Arquivo: %s\n", nome_arquivo);
    printf("Número total de blocos ocupados: %d\n", total_blocos);
    printf("Tamanho em bytes: %d\n", total_blocos * 10);
    printf("================================================\n");
    
    free(blocos_visitados);
}

// Relatório 2: Tamanho Máximo de Arquivo
void report_tamanho_maximo() {
    // Contar blocos livres na pilha
    int blocos_livres = 0;
    struct NoPilha* atual = pilha_blocos_livres;
    while (atual != NULL) {
        blocos_livres++;
        atual = atual->proximo;
    }
    
    // Calcular tamanho máximo em bytes
    int tamanho_max_bytes = blocos_livres * 10;
    
    printf("=== RELATÓRIO 2: TAMANHO MÁXIMO DE ARQUIVO ===\n");
    printf("Blocos livres disponíveis: %d\n", blocos_livres);
    printf("Tamanho máximo em bytes: %d\n", tamanho_max_bytes);
    printf("==============================================\n");
}

// Função auxiliar para verificar integridade de um arquivo
bool verificar_integridade_arquivo(int inode_num, bool* blocos_visitados) {
    if (inode_num < 0 || inode_num >= NUMERO_TOTAL_BLOCOS || disco[inode_num].tipo != 'I') {
        return false;
    }
    
    if (blocos_visitados[inode_num]) {
        return true; // Já verificado
    }
    blocos_visitados[inode_num] = true;
    
    struct Inode inode;
    memcpy(&inode, disco[inode_num].dados, sizeof(struct Inode));
    
    // Verificar se o próprio inode está corrompido
    if (disco[inode_num].tipo == 'B') {
        return false;
    }
    
    // Verificar ponteiros diretos
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco = inode.ponteiros[i];
            if (bloco >= 0 && bloco < NUMERO_TOTAL_BLOCOS) {
                if (disco[bloco].tipo == 'B') {
                    return false; // Bloco corrompido encontrado
                }
                if (disco[bloco].tipo == 'I') {
                    // Verificar blocos indiretos
                    if (!verificar_integridade_indiretos(bloco, blocos_visitados)) {
                        return false;
                    }
                }
            }
        }
    }
    
    return true;
}

// Função auxiliar para verificar integridade de blocos indiretos
bool verificar_integridade_indiretos(int bloco_ponteiros, bool* blocos_visitados) {
    if (bloco_ponteiros < 0 || bloco_ponteiros >= NUMERO_TOTAL_BLOCOS || disco[bloco_ponteiros].tipo != 'I') {
        return false;
    }
    
    if (disco[bloco_ponteiros].tipo == 'B') {
        return false;
    }
    
    if (blocos_visitados[bloco_ponteiros]) {
        return true;
    }
    blocos_visitados[bloco_ponteiros] = true;
    
    int ponteiros_por_bloco = 500 / sizeof(int);
    int* ponteiros = (int*)(disco[bloco_ponteiros].dados);
    
    for (int i = 0; i < ponteiros_por_bloco; i++) {
        if (ponteiros[i] != -1) {
            int bloco = ponteiros[i];
            if (bloco >= 0 && bloco < NUMERO_TOTAL_BLOCOS) {
                if (disco[bloco].tipo == 'B') {
                    return false;
                }
                if (disco[bloco].tipo == 'I') {
                    if (!verificar_integridade_indiretos(bloco, blocos_visitados)) {
                        return false;
                    }
                }
            }
        }
    }
    
    return true;
}

// Função auxiliar para listar arquivos recursivamente
void listar_arquivos_recursivo(int inode_dir, char* caminho_atual, bool* blocos_visitados, bool* integridade_ok) {
    if (inode_dir < 0 || inode_dir >= NUMERO_TOTAL_BLOCOS || disco[inode_dir].tipo != 'I') {
        return;
    }
    
    if (blocos_visitados[inode_dir]) {
        return;
    }
    blocos_visitados[inode_dir] = true;
    
    struct Inode inode;
    memcpy(&inode, disco[inode_dir].dados, sizeof(struct Inode));
    
    // Percorrer todos os blocos de dados do diretório
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco_dados = inode.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                        // Pular "." e ".."
                        if (strcmp(entrada.nome_arquivo, ".") != 0 && strcmp(entrada.nome_arquivo, "..") != 0) {
                            char caminho_completo[512];
                            if (strcmp(caminho_atual, "/") == 0) {
                                snprintf(caminho_completo, sizeof(caminho_completo), "/%s", entrada.nome_arquivo);
                            } else {
                                snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", caminho_atual, entrada.nome_arquivo);
                            }
                            
                            // Verificar integridade do arquivo/diretório
                            bool* blocos_arquivo = calloc(NUMERO_TOTAL_BLOCOS, sizeof(bool));
                            bool integro = verificar_integridade_arquivo(entrada.numero_inode, blocos_arquivo);
                            *integridade_ok = integro;
                            
                            printf("  %s: %s\n", caminho_completo, integro ? "íntegro" : "corrompido");
                            
                            free(blocos_arquivo);
                            
                            // Se for diretório, listar recursivamente
                            struct Inode inode_entrada;
                            memcpy(&inode_entrada, disco[entrada.numero_inode].dados, sizeof(struct Inode));
                            if (inode_entrada.permissoes[0] == 'd') {
                                listar_arquivos_recursivo(entrada.numero_inode, caminho_completo, blocos_visitados, integridade_ok);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Relatório 3: Integridade dos Arquivos
void report_integridade() {
    printf("=== RELATÓRIO 3: INTEGRIDADE DOS ARQUIVOS ===\n");
    
    bool* blocos_visitados = calloc(NUMERO_TOTAL_BLOCOS, sizeof(bool));
    if (blocos_visitados == NULL) {
        printf("Erro: Falha na alocação de memória\n");
        return;
    }
    
    int inode_raiz = encontrar_inode_raiz();
    if (inode_raiz == -1) {
        printf("Erro: Não foi possível encontrar o diretório raiz\n");
        free(blocos_visitados);
        return;
    }
    
    printf("Verificando integridade de todos os arquivos...\n");
    bool integridade_ok = true;
    listar_arquivos_recursivo(inode_raiz, "/", blocos_visitados, &integridade_ok);
    
    printf("==============================================\n");
    free(blocos_visitados);
}

// Relatório 4: Blocos Perdidos
void report_blocos_perdidos() {
    printf("=== RELATÓRIO 4: BLOCOS PERDIDOS ===\n");
    
    // Criar array para marcar blocos referenciados
    bool* blocos_referenciados = calloc(NUMERO_TOTAL_BLOCOS, sizeof(bool));
    if (blocos_referenciados == NULL) {
        printf("Erro: Falha na alocação de memória\n");
        return;
    }
    
    // Marcar todos os blocos como não referenciados inicialmente
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        blocos_referenciados[i] = false;
    }
    
    // Marcar blocos referenciados a partir da raiz
    marcar_blocos_referenciados_recursivo(encontrar_inode_raiz(), blocos_referenciados);
    
    // Encontrar blocos perdidos
    int blocos_perdidos = 0;
    int bytes_perdidos = 0;
    
    printf("Blocos perdidos encontrados:\n");
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        if (disco[i].tipo != 'F' && disco[i].tipo != 'B' && !blocos_referenciados[i]) {
            printf("  Bloco %d (tipo: %c)\n", i, disco[i].tipo);
            blocos_perdidos++;
            bytes_perdidos += 10;
        }
    }
    
    printf("\nResumo:\n");
    printf("Total de blocos perdidos: %d\n", blocos_perdidos);
    printf("Espaço perdido em bytes: %d\n", bytes_perdidos);
    printf("=====================================\n");
    
    free(blocos_referenciados);
}

// Função auxiliar para marcar blocos referenciados recursivamente
void marcar_blocos_referenciados_recursivo(int inode_num, bool* blocos_referenciados) {
    if (inode_num < 0 || inode_num >= NUMERO_TOTAL_BLOCOS || disco[inode_num].tipo != 'I') {
        return;
    }
    
    if (blocos_referenciados[inode_num]) {
        return; // Evitar loops infinitos
    }
    blocos_referenciados[inode_num] = true;
    
    struct Inode inode;
    memcpy(&inode, disco[inode_num].dados, sizeof(struct Inode));
    
    // Marcar blocos de dados do inode
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco = inode.ponteiros[i];
            if (bloco >= 0 && bloco < NUMERO_TOTAL_BLOCOS) {
                blocos_referenciados[bloco] = true;
                
                if (disco[bloco].tipo == 'I') {
                    // Marcar blocos indiretos
                    marcar_blocos_indiretos(bloco, blocos_referenciados);
                } else if (disco[bloco].tipo == 'A' && inode.permissoes[0] == 'd') {
                    // Se for diretório, marcar entradas recursivamente
                    int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                    for (int j = 0; j < entradas_por_bloco; j++) {
                        struct EntradaDiretorio entrada;
                        memcpy(&entrada, disco[bloco].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                        
                        if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                            if (strcmp(entrada.nome_arquivo, ".") != 0 && strcmp(entrada.nome_arquivo, "..") != 0) {
                                marcar_blocos_referenciados_recursivo(entrada.numero_inode, blocos_referenciados);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Função auxiliar para marcar blocos indiretos
void marcar_blocos_indiretos(int bloco_ponteiros, bool* blocos_referenciados) {
    if (bloco_ponteiros < 0 || bloco_ponteiros >= NUMERO_TOTAL_BLOCOS || disco[bloco_ponteiros].tipo != 'I') {
        return;
    }
    
    if (blocos_referenciados[bloco_ponteiros]) {
        return;
    }
    blocos_referenciados[bloco_ponteiros] = true;
    
    int ponteiros_por_bloco = 500 / sizeof(int);
    int* ponteiros = (int*)(disco[bloco_ponteiros].dados);
    
    for (int i = 0; i < ponteiros_por_bloco; i++) {
        if (ponteiros[i] != -1) {
            int bloco = ponteiros[i];
            if (bloco >= 0 && bloco < NUMERO_TOTAL_BLOCOS) {
                blocos_referenciados[bloco] = true;
                if (disco[bloco].tipo == 'I') {
                    marcar_blocos_indiretos(bloco, blocos_referenciados);
                }
            }
        }
    }
}

// Relatório 5: Estado Atual do Disco
void report_estado_disco() {
    printf("=== RELATÓRIO 5: ESTADO ATUAL DO DISCO ===\n");
    printf("Legenda: F=Free, B=Bad, I=Inode, A=Allocated\n");
    printf("Total de blocos: %d\n\n", NUMERO_TOTAL_BLOCOS);
    
    int blocos_por_linha = 50;
    
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i += blocos_por_linha) {
        printf("Blocos %4d-%4d: ", i, i + blocos_por_linha - 1);
        for (int j = 0; j < blocos_por_linha && i + j < NUMERO_TOTAL_BLOCOS; j++) {
            printf("%c", disco[i + j].tipo);
        }
        printf("\n");
    }
    
    // Estatísticas
    int count_f = 0, count_b = 0, count_i = 0, count_a = 0;
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        switch (disco[i].tipo) {
            case 'F': count_f++; break;
            case 'B': count_b++; break;
            case 'I': count_i++; break;
            case 'A': count_a++; break;
        }
    }
    
    printf("\nEstatísticas:\n");
    printf("  Blocos livres (F): %d\n", count_f);
    printf("  Blocos ruins (B): %d\n", count_b);
    printf("  Blocos inode (I): %d\n", count_i);
    printf("  Blocos dados (A): %d\n", count_a);
    printf("=====================================\n");
}

// Função auxiliar para explorer view recursivo
void explorer_view_recursivo(int inode_dir, char* caminho_atual, int nivel_indentacao) {
    if (inode_dir < 0 || inode_dir >= NUMERO_TOTAL_BLOCOS || disco[inode_dir].tipo != 'I') {
        return;
    }
    
    struct Inode inode;
    memcpy(&inode, disco[inode_dir].dados, sizeof(struct Inode));
    
    // Percorrer todos os blocos de dados do diretório
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco_dados = inode.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                        // Pular "." e ".."
                        if (strcmp(entrada.nome_arquivo, ".") != 0 && strcmp(entrada.nome_arquivo, "..") != 0) {
                            // Imprimir indentação
                            for (int k = 0; k < nivel_indentacao; k++) {
                                printf("  ");
                            }
                            
                            // Obter informações do inode
                            struct Inode inode_entrada;
                            memcpy(&inode_entrada, disco[entrada.numero_inode].dados, sizeof(struct Inode));
                            
                            // Imprimir nome e tipo
                            if (inode_entrada.permissoes[0] == 'd') {
                                printf("📁 %s/\n", entrada.nome_arquivo);
                                // Recursão para subdiretórios
                                char novo_caminho[512];
                                if (strcmp(caminho_atual, "/") == 0) {
                                    snprintf(novo_caminho, sizeof(novo_caminho), "/%s", entrada.nome_arquivo);
                                } else {
                                    snprintf(novo_caminho, sizeof(novo_caminho), "%s/%s", caminho_atual, entrada.nome_arquivo);
                                }
                                explorer_view_recursivo(entrada.numero_inode, novo_caminho, nivel_indentacao + 1);
                            } else {
                                printf("📄 %s\n", entrada.nome_arquivo);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Relatório 6: Visualização "Explorer"
void report_explorer_view() {
    printf("=== RELATÓRIO 6: VISUALIZAÇÃO EXPLORER ===\n");
    
    int inode_raiz = encontrar_inode_raiz();
    if (inode_raiz == -1) {
        printf("Erro: Não foi possível encontrar o diretório raiz\n");
        return;
    }
    
    printf("📁 /\n");
    explorer_view_recursivo(inode_raiz, "/", 1);
    printf("===========================================\n");
}

// Relatório 7: Visualização da Árvore Detalhada
void report_arvore_detalhada() {
    printf("=== RELATÓRIO 7: ÁRVORE DETALHADA ===\n");
    
    int inode_raiz = encontrar_inode_raiz();
    if (inode_raiz == -1) {
        printf("Erro: Não foi possível encontrar o diretório raiz\n");
        return;
    }
    
    arvore_detalhada_recursivo(inode_raiz, "/", 0);
    printf("====================================\n");
}

// Função auxiliar para árvore detalhada recursiva
void arvore_detalhada_recursivo(int inode_dir, char* caminho_atual, int nivel) {
    if (inode_dir < 0 || inode_dir >= NUMERO_TOTAL_BLOCOS || disco[inode_dir].tipo != 'I') {
        return;
    }
    
    struct Inode inode;
    memcpy(&inode, disco[inode_dir].dados, sizeof(struct Inode));
    
    // Imprimir informações do diretório atual
    for (int i = 0; i < nivel; i++) printf("  ");
    printf("📁 %s (inode: %d, blocos: %d, permissões: %s)\n", 
           caminho_atual, inode_dir, (int)(inode.tamanho_bytes / 10), inode.permissoes);
    
    // Percorrer blocos de dados
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco_dados = inode.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                        if (strcmp(entrada.nome_arquivo, ".") != 0 && strcmp(entrada.nome_arquivo, "..") != 0) {
                            // Obter informações do inode
                            struct Inode inode_entrada;
                            memcpy(&inode_entrada, disco[entrada.numero_inode].dados, sizeof(struct Inode));
                            
                            char novo_caminho[512];
                            if (strcmp(caminho_atual, "/") == 0) {
                                snprintf(novo_caminho, sizeof(novo_caminho), "/%s", entrada.nome_arquivo);
                            } else {
                                snprintf(novo_caminho, sizeof(novo_caminho), "%s/%s", caminho_atual, entrada.nome_arquivo);
                            }
                            
                            if (inode_entrada.permissoes[0] == 'd') {
                                arvore_detalhada_recursivo(entrada.numero_inode, novo_caminho, nivel + 1);
                            } else {
                                for (int k = 0; k <= nivel; k++) printf("  ");
                                printf("📄 %s (inode: %d, tamanho: %d bytes, permissões: %s)\n",
                                       entrada.nome_arquivo, entrada.numero_inode, inode_entrada.tamanho_bytes, inode_entrada.permissoes);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Função auxiliar para listar links recursivamente
void listar_links_recursivo(int inode_dir, char* caminho_atual, bool* blocos_visitados) {
    if (inode_dir < 0 || inode_dir >= NUMERO_TOTAL_BLOCOS || disco[inode_dir].tipo != 'I') {
        return;
    }
    
    if (blocos_visitados[inode_dir]) {
        return;
    }
    blocos_visitados[inode_dir] = true;
    
    struct Inode inode;
    memcpy(&inode, disco[inode_dir].dados, sizeof(struct Inode));
    
    // Percorrer todos os blocos de dados do diretório
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco_dados = inode.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                        if (strcmp(entrada.nome_arquivo, ".") != 0 && strcmp(entrada.nome_arquivo, "..") != 0) {
                            char caminho_completo[512];
                            if (strcmp(caminho_atual, "/") == 0) {
                                snprintf(caminho_completo, sizeof(caminho_completo), "/%s", entrada.nome_arquivo);
                            } else {
                                snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", caminho_atual, entrada.nome_arquivo);
                            }
                            
                            // Verificar se é link simbólico
                            struct Inode inode_entrada;
                            memcpy(&inode_entrada, disco[entrada.numero_inode].dados, sizeof(struct Inode));
                            
                            if (inode_entrada.permissoes[0] == 'l') {
                                printf("  🔗 Link simbólico: %s -> (conteúdo do arquivo)\n", caminho_completo);
                            } else if (inode_entrada.contador_links > 1) {
                                printf("  🔗 Link físico: %s (contador: %d)\n", caminho_completo, inode_entrada.contador_links);
                            }
                            
                            // Se for diretório, continuar recursivamente
                            if (inode_entrada.permissoes[0] == 'd') {
                                listar_links_recursivo(entrada.numero_inode, caminho_completo, blocos_visitados);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Relatório 8: Visualização de Links
void report_links() {
    printf("=== RELATÓRIO 8: VISUALIZAÇÃO DE LINKS ===\n");
    
    bool* blocos_visitados = calloc(NUMERO_TOTAL_BLOCOS, sizeof(bool));
    if (blocos_visitados == NULL) {
        printf("Erro: Falha na alocação de memória\n");
        return;
    }
    
    int inode_raiz = encontrar_inode_raiz();
    if (inode_raiz == -1) {
        printf("Erro: Não foi possível encontrar o diretório raiz\n");
        free(blocos_visitados);
        return;
    }
    
    printf("Links encontrados no sistema:\n");
    listar_links_recursivo(inode_raiz, "/", blocos_visitados);
    printf("=====================================\n");
    
    free(blocos_visitados);
}
