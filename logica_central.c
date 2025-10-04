#include "sistema_arquivos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Declaração da variável global
extern int diretorio_atual;

// Função auxiliar para calcular o número de blocos necessários
int calcular_blocos_necessarios(int tamanho_bytes) {
    return (int)ceil(tamanho_bytes / 10.0);
}

// Função auxiliar para encontrar o inode raiz
int encontrar_inode_raiz() {
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        if (disco[i].tipo == 'I') {
            struct Inode inode;
            memcpy(&inode, disco[i].dados, sizeof(struct Inode));
            // Verificar se é o inode raiz (tem permissões de diretório)
            if (inode.permissoes[0] == 'd' && strcmp(inode.nome_usuario, "root") == 0) {
                return i;
            }
        }
    }
    return -1; // Inode raiz não encontrado
}

// Função de resolução de caminhos
int encontrar_inode_por_caminho_com_inicial(const char* caminho, int inode_inicial) {
    if (caminho == NULL || strlen(caminho) == 0) {
        return -1;
    }
    
    int inode_atual = inode_inicial;
    if (inode_atual == -1) {
        printf("Erro: Inode inicial inválido\n");
        return -1;
    }
    
    // Se o caminho é apenas "/", retornar o inode raiz
    if (strcmp(caminho, "/") == 0) {
        return inode_atual;
    }
    
    // Copiar o caminho para manipulação
    char caminho_copia[strlen(caminho) + 1];
    strcpy(caminho_copia, caminho);
    
    // Remover barra inicial se presente
    if (caminho_copia[0] == '/') {
        memmove(caminho_copia, caminho_copia + 1, strlen(caminho_copia));
    }
    
    // Dividir o caminho em componentes
    char* token = strtok(caminho_copia, "/");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            // Permanecer no diretório atual
            token = strtok(NULL, "/");
            continue;
        } else if (strcmp(token, "..") == 0) {
            // Subir para o diretório pai
            struct Inode inode_atual_struct;
            memcpy(&inode_atual_struct, disco[inode_atual].dados, sizeof(struct Inode));
            
            // Procurar entrada ".." no diretório atual
            int encontrado = 0;
            for (int i = 0; i < 8; i++) {
                if (inode_atual_struct.ponteiros[i] != -1) {
                    int bloco_dados = inode_atual_struct.ponteiros[i];
                    if (disco[bloco_dados].tipo == 'A') {
                        // Ler todas as entradas do bloco de diretório
                        int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                        
                        for (int j = 0; j < entradas_por_bloco; j++) {
                            struct EntradaDiretorio entrada;
                            memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                            
                            // Verificar se é entrada ".."
                            if (strcmp(entrada.nome_arquivo, "..") == 0) {
                                inode_atual = entrada.numero_inode;
                                encontrado = 1;
                                break;
                            }
                        }
                        
                        if (encontrado) break;
                    }
                }
            }
            
            if (!encontrado) {
                printf("Erro: Diretório pai não encontrado\n");
                return -1;
            }
        } else {
            // Procurar entrada com o nome especificado
            struct Inode inode_atual_struct;
            memcpy(&inode_atual_struct, disco[inode_atual].dados, sizeof(struct Inode));
            
            int encontrado = 0;
            for (int i = 0; i < 8; i++) {
                if (inode_atual_struct.ponteiros[i] != -1) {
                    int bloco_dados = inode_atual_struct.ponteiros[i];
                    if (disco[bloco_dados].tipo == 'A') {
                        // Ler todas as entradas do bloco de diretório
                        int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                        
                        for (int j = 0; j < entradas_por_bloco; j++) {
                            struct EntradaDiretorio entrada;
                            memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                            
                            // Verificar se é a entrada procurada
                            if (strcmp(entrada.nome_arquivo, token) == 0) {
                                inode_atual = entrada.numero_inode;
                                encontrado = 1;
                                break;
                            }
                        }
                        
                        if (encontrado) break;
                    }
                }
            }
            
            if (!encontrado) {
                printf("Erro: '%s' não encontrado\n", token);
                return -1;
            }
        }
        
        token = strtok(NULL, "/");
    }
    
    return inode_atual;
}

// Função wrapper para manter compatibilidade
int encontrar_inode_por_caminho(const char* caminho) {
    // Para caminhos absolutos, sempre começar do raiz
    if (caminho != NULL && caminho[0] == '/') {
        int inode_raiz = encontrar_inode_raiz();
        return encontrar_inode_por_caminho_com_inicial(caminho, inode_raiz);
    }
    
    // Para caminhos relativos, usar o diretório atual
    return encontrar_inode_por_caminho_com_inicial(caminho, diretorio_atual);
}

// Função de alocação de blocos para arquivo
void alocar_blocos_para_inode(int numero_inode, int tamanho_bytes) {
    if (numero_inode < 0 || numero_inode >= NUMERO_TOTAL_BLOCOS || disco[numero_inode].tipo != 'I') {
        printf("Erro: Inode inválido\n");
        return;
    }
    
    struct Inode inode;
    memcpy(&inode, disco[numero_inode].dados, sizeof(struct Inode));
    
    int blocos_necessarios = calcular_blocos_necessarios(tamanho_bytes);
    printf("Alocando %d blocos para inode %d (tamanho: %d bytes)\n", blocos_necessarios, numero_inode, tamanho_bytes);
    
    int blocos_alocados = 0;
    
    // 1. Alocar ponteiros diretos (primeiros 5 blocos)
    for (int i = 0; i < 5 && blocos_alocados < blocos_necessarios; i++) {
        int bloco_dados = obter_bloco_livre();
        if (bloco_dados == -1) {
            printf("Erro: Não há blocos livres suficientes\n");
            return;
        }
        
        disco[bloco_dados].tipo = 'A';
        inode.ponteiros[i] = bloco_dados;
        blocos_alocados++;
        printf("  Bloco direto %d: %d\n", i, bloco_dados);
    }
    
    // 2. Alocar ponteiro indireto simples (6º ponteiro)
    if (blocos_alocados < blocos_necessarios) {
        int bloco_ponteiros = obter_bloco_livre();
        if (bloco_ponteiros == -1) {
            printf("Erro: Não há blocos livres suficientes\n");
            return;
        }
        
        disco[bloco_ponteiros].tipo = 'I';
        inode.ponteiros[5] = bloco_ponteiros;
        
        // Alocar blocos de dados e armazenar seus IDs no bloco de ponteiros
        int ponteiros_por_bloco = 500 / sizeof(int); // Quantos ponteiros cabem em um bloco
        int ponteiros_usados = 0;
        
        for (int i = 0; i < ponteiros_por_bloco && blocos_alocados < blocos_necessarios; i++) {
            int bloco_dados = obter_bloco_livre();
            if (bloco_dados == -1) {
                printf("Erro: Não há blocos livres suficientes\n");
                return;
            }
            
            disco[bloco_dados].tipo = 'A';
            
            // Armazenar o ponteiro no bloco de ponteiros
            int* ponteiros = (int*)(disco[bloco_ponteiros].dados);
            ponteiros[i] = bloco_dados;
            ponteiros_usados++;
            blocos_alocados++;
            printf("  Bloco indireto simples %d: %d\n", i, bloco_dados);
        }
        
        printf("  Bloco de ponteiros indiretos simples: %d (%d ponteiros)\n", bloco_ponteiros, ponteiros_usados);
    }
    
    // 3. Alocar ponteiro indireto duplo (7º ponteiro)
    if (blocos_alocados < blocos_necessarios) {
        int bloco_ponteiros_duplo = obter_bloco_livre();
        if (bloco_ponteiros_duplo == -1) {
            printf("Erro: Não há blocos livres suficientes\n");
            return;
        }
        
        disco[bloco_ponteiros_duplo].tipo = 'I';
        inode.ponteiros[6] = bloco_ponteiros_duplo;
        
        int ponteiros_por_bloco = 500 / sizeof(int);
        int ponteiros_usados = 0;
        
        for (int i = 0; i < ponteiros_por_bloco && blocos_alocados < blocos_necessarios; i++) {
            // Alocar bloco de ponteiros simples
            int bloco_ponteiros_simples = obter_bloco_livre();
            if (bloco_ponteiros_simples == -1) {
                printf("Erro: Não há blocos livres suficientes\n");
                return;
            }
            
            disco[bloco_ponteiros_simples].tipo = 'I';
            
            // Armazenar ponteiro para o bloco de ponteiros simples
            int* ponteiros_duplo = (int*)(disco[bloco_ponteiros_duplo].dados);
            ponteiros_duplo[i] = bloco_ponteiros_simples;
            ponteiros_usados++;
            
            // Alocar blocos de dados para este bloco de ponteiros simples
            int* ponteiros_simples = (int*)(disco[bloco_ponteiros_simples].dados);
            for (int j = 0; j < ponteiros_por_bloco && blocos_alocados < blocos_necessarios; j++) {
                int bloco_dados = obter_bloco_livre();
                if (bloco_dados == -1) {
                    printf("Erro: Não há blocos livres suficientes\n");
                    return;
                }
                
                disco[bloco_dados].tipo = 'A';
                ponteiros_simples[j] = bloco_dados;
                blocos_alocados++;
                printf("  Bloco indireto duplo %d.%d: %d\n", i, j, bloco_dados);
            }
        }
        
        printf("  Bloco de ponteiros indiretos duplos: %d (%d ponteiros)\n", bloco_ponteiros_duplo, ponteiros_usados);
    }
    
    // 4. Alocar ponteiro indireto triplo (8º ponteiro)
    if (blocos_alocados < blocos_necessarios) {
        int bloco_ponteiros_triplo = obter_bloco_livre();
        if (bloco_ponteiros_triplo == -1) {
            printf("Erro: Não há blocos livres suficientes\n");
            return;
        }
        
        disco[bloco_ponteiros_triplo].tipo = 'I';
        inode.ponteiros[7] = bloco_ponteiros_triplo;
        
        int ponteiros_por_bloco = 500 / sizeof(int);
        int ponteiros_usados = 0;
        
        for (int i = 0; i < ponteiros_por_bloco && blocos_alocados < blocos_necessarios; i++) {
            // Alocar bloco de ponteiros duplos
            int bloco_ponteiros_duplo = obter_bloco_livre();
            if (bloco_ponteiros_duplo == -1) {
                printf("Erro: Não há blocos livres suficientes\n");
                return;
            }
            
            disco[bloco_ponteiros_duplo].tipo = 'I';
            
            // Armazenar ponteiro para o bloco de ponteiros duplos
            int* ponteiros_triplo = (int*)(disco[bloco_ponteiros_triplo].dados);
            ponteiros_triplo[i] = bloco_ponteiros_duplo;
            ponteiros_usados++;
            
            // Alocar blocos de ponteiros simples para este bloco de ponteiros duplos
            int* ponteiros_duplo = (int*)(disco[bloco_ponteiros_duplo].dados);
            for (int j = 0; j < ponteiros_por_bloco && blocos_alocados < blocos_necessarios; j++) {
                int bloco_ponteiros_simples = obter_bloco_livre();
                if (bloco_ponteiros_simples == -1) {
                    printf("Erro: Não há blocos livres suficientes\n");
                    return;
                }
                
                disco[bloco_ponteiros_simples].tipo = 'I';
                ponteiros_duplo[j] = bloco_ponteiros_simples;
                
                // Alocar blocos de dados para este bloco de ponteiros simples
                int* ponteiros_simples = (int*)(disco[bloco_ponteiros_simples].dados);
                for (int k = 0; k < ponteiros_por_bloco && blocos_alocados < blocos_necessarios; k++) {
                    int bloco_dados = obter_bloco_livre();
                    if (bloco_dados == -1) {
                        printf("Erro: Não há blocos livres suficientes\n");
                        return;
                    }
                    
                    disco[bloco_dados].tipo = 'A';
                    ponteiros_simples[k] = bloco_dados;
                    blocos_alocados++;
                    printf("  Bloco indireto triplo %d.%d.%d: %d\n", i, j, k, bloco_dados);
                }
            }
        }
        
        printf("  Bloco de ponteiros indiretos triplos: %d (%d ponteiros)\n", bloco_ponteiros_triplo, ponteiros_usados);
    }
    
    // Atualizar o inode com o novo tamanho
    inode.tamanho_bytes = tamanho_bytes;
    
    // Salvar o inode atualizado no disco
    memcpy(disco[numero_inode].dados, &inode, sizeof(struct Inode));
    
    printf("Alocação concluída: %d blocos alocados para inode %d\n", blocos_alocados, numero_inode);
}

// Função de liberação de blocos de arquivo
void liberar_blocos_de_inode(int numero_inode) {
    if (numero_inode < 0 || numero_inode >= NUMERO_TOTAL_BLOCOS || disco[numero_inode].tipo != 'I') {
        printf("Erro: Inode inválido\n");
        return;
    }
    
    struct Inode inode;
    memcpy(&inode, disco[numero_inode].dados, sizeof(struct Inode));
    
    printf("Liberando blocos do inode %d\n", numero_inode);
    
    // Liberar ponteiros diretos (primeiros 5)
    for (int i = 0; i < 5; i++) {
        if (inode.ponteiros[i] != -1) {
            printf("  Liberando bloco direto %d: %d\n", i, inode.ponteiros[i]);
            liberar_bloco(inode.ponteiros[i]);
            inode.ponteiros[i] = -1;
        }
    }
    
    // Liberar ponteiro indireto simples (6º ponteiro)
    if (inode.ponteiros[5] != -1) {
        int bloco_ponteiros = inode.ponteiros[5];
        printf("  Liberando ponteiros indiretos simples: %d\n", bloco_ponteiros);
        
        // Liberar todos os blocos apontados pelos ponteiros
        int* ponteiros = (int*)(disco[bloco_ponteiros].dados);
        int ponteiros_por_bloco = 500 / sizeof(int);
        
        for (int i = 0; i < ponteiros_por_bloco; i++) {
            if (ponteiros[i] != -1 && ponteiros[i] != 0) {
                printf("    Liberando bloco indireto simples %d: %d\n", i, ponteiros[i]);
                liberar_bloco(ponteiros[i]);
            }
        }
        
        // Liberar o bloco de ponteiros
        liberar_bloco(bloco_ponteiros);
        inode.ponteiros[5] = -1;
    }
    
    // Liberar ponteiro indireto duplo (7º ponteiro)
    if (inode.ponteiros[6] != -1) {
        int bloco_ponteiros_duplo = inode.ponteiros[6];
        printf("  Liberando ponteiros indiretos duplos: %d\n", bloco_ponteiros_duplo);
        
        // Liberar todos os blocos de ponteiros simples
        int* ponteiros_duplo = (int*)(disco[bloco_ponteiros_duplo].dados);
        int ponteiros_por_bloco = 500 / sizeof(int);
        
        for (int i = 0; i < ponteiros_por_bloco; i++) {
            if (ponteiros_duplo[i] != -1) {
                int bloco_ponteiros_simples = ponteiros_duplo[i];
                printf("    Liberando ponteiros simples %d: %d\n", i, bloco_ponteiros_simples);
                
                // Liberar blocos de dados apontados pelos ponteiros simples
                int* ponteiros_simples = (int*)(disco[bloco_ponteiros_simples].dados);
                for (int j = 0; j < ponteiros_por_bloco; j++) {
                    if (ponteiros_simples[j] != -1 && ponteiros_simples[j] != 0) {
                        printf("      Liberando bloco indireto duplo %d.%d: %d\n", i, j, ponteiros_simples[j]);
                        liberar_bloco(ponteiros_simples[j]);
                    }
                }
                
                // Liberar o bloco de ponteiros simples
                liberar_bloco(bloco_ponteiros_simples);
            }
        }
        
        // Liberar o bloco de ponteiros duplos
        liberar_bloco(bloco_ponteiros_duplo);
        inode.ponteiros[6] = -1;
    }
    
    // Liberar ponteiro indireto triplo (8º ponteiro)
    if (inode.ponteiros[7] != -1) {
        int bloco_ponteiros_triplo = inode.ponteiros[7];
        printf("  Liberando ponteiros indiretos triplos: %d\n", bloco_ponteiros_triplo);
        
        // Liberar todos os blocos de ponteiros duplos
        int* ponteiros_triplo = (int*)(disco[bloco_ponteiros_triplo].dados);
        int ponteiros_por_bloco = 500 / sizeof(int);
        
        for (int i = 0; i < ponteiros_por_bloco; i++) {
            if (ponteiros_triplo[i] != -1) {
                int bloco_ponteiros_duplo = ponteiros_triplo[i];
                printf("    Liberando ponteiros duplos %d: %d\n", i, bloco_ponteiros_duplo);
                
                // Liberar blocos de ponteiros simples
                int* ponteiros_duplo = (int*)(disco[bloco_ponteiros_duplo].dados);
                for (int j = 0; j < ponteiros_por_bloco; j++) {
                    if (ponteiros_duplo[j] != -1) {
                        int bloco_ponteiros_simples = ponteiros_duplo[j];
                        printf("      Liberando ponteiros simples %d.%d: %d\n", i, j, bloco_ponteiros_simples);
                        
                        // Liberar blocos de dados apontados pelos ponteiros simples
                        int* ponteiros_simples = (int*)(disco[bloco_ponteiros_simples].dados);
                        for (int k = 0; k < ponteiros_por_bloco; k++) {
                            if (ponteiros_simples[k] != -1 && ponteiros_simples[k] != 0) {
                                printf("        Liberando bloco indireto triplo %d.%d.%d: %d\n", i, j, k, ponteiros_simples[k]);
                                liberar_bloco(ponteiros_simples[k]);
                            }
                        }
                        
                        // Liberar o bloco de ponteiros simples
                        liberar_bloco(bloco_ponteiros_simples);
                    }
                }
                
                // Liberar o bloco de ponteiros duplos
                liberar_bloco(bloco_ponteiros_duplo);
            }
        }
        
        // Liberar o bloco de ponteiros triplos
        liberar_bloco(bloco_ponteiros_triplo);
        inode.ponteiros[7] = -1;
    }
    
    // Atualizar o inode
    inode.tamanho_bytes = 0;
    memcpy(disco[numero_inode].dados, &inode, sizeof(struct Inode));
    
    printf("Liberação concluída para inode %d\n", numero_inode);
}

// Função para encontrar um slot vazio em um bloco de diretório
int encontrar_slot_vazio(int bloco_dados, struct EntradaDiretorio* entrada_vazia) {
    if (disco[bloco_dados].tipo != 'A') {
        return 0; // Não é um bloco de dados
    }
    
    int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
    
    for (int i = 0; i < entradas_por_bloco; i++) {
        struct EntradaDiretorio entrada;
        memcpy(&entrada, disco[bloco_dados].dados + i * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
        
        // Verificar se a entrada está vazia (nome vazio ou inode inválido)
        if (entrada.nome_arquivo[0] == '\0' || entrada.numero_inode == -1) {
            if (entrada_vazia != NULL) {
                *entrada_vazia = entrada;
            }
            return i; // Retorna o índice do slot vazio
        }
    }
    
    return -1; // Nenhum slot vazio encontrado
}

// Função para adicionar entrada de diretório
int adicionar_entrada_diretorio(int inode_dir_pai, const char* nome, int inode_filho) {
    if (inode_dir_pai < 0 || inode_dir_pai >= NUMERO_TOTAL_BLOCOS || disco[inode_dir_pai].tipo != 'I') {
        printf("Erro: Inode do diretório pai inválido\n");
        return -1;
    }
    
    if (inode_filho < 0 || inode_filho >= NUMERO_TOTAL_BLOCOS || disco[inode_filho].tipo != 'I') {
        printf("Erro: Inode do filho inválido\n");
        return -1;
    }
    
    if (nome == NULL || strlen(nome) == 0) {
        printf("Erro: Nome inválido\n");
        return -1;
    }
    
    struct Inode inode_pai;
    memcpy(&inode_pai, disco[inode_dir_pai].dados, sizeof(struct Inode));
    
    printf("Adicionando entrada '%s' -> inode %d no diretório %d\n", nome, inode_filho, inode_dir_pai);
    
    // Procurar um slot vazio nos blocos de dados existentes
    for (int i = 0; i < 8; i++) {
        if (inode_pai.ponteiros[i] != -1) {
            int bloco_dados = inode_pai.ponteiros[i];
            int slot_vazio = encontrar_slot_vazio(bloco_dados, NULL);
            
            if (slot_vazio >= 0) {
                // Encontrar slot vazio, adicionar entrada
                struct EntradaDiretorio nova_entrada;
                strncpy(nova_entrada.nome_arquivo, nome, sizeof(nova_entrada.nome_arquivo) - 1);
                nova_entrada.nome_arquivo[sizeof(nova_entrada.nome_arquivo) - 1] = '\0';
                nova_entrada.numero_inode = inode_filho;
                
                // Escrever a entrada no slot vazio
                memcpy(disco[bloco_dados].dados + slot_vazio * sizeof(struct EntradaDiretorio), 
                       &nova_entrada, sizeof(struct EntradaDiretorio));
                
                printf("  Entrada adicionada no bloco %d, slot %d\n", bloco_dados, slot_vazio);
                
                return 0; // Sucesso
            }
        }
    }
    
    // Se chegou aqui, todos os blocos existentes estão cheios
    // Alocar um novo bloco de dados para o diretório
    int novo_bloco = obter_bloco_livre();
    if (novo_bloco == -1) {
        printf("Erro: Não há blocos livres para alocar novo bloco de diretório\n");
        return -1;
    }
    
    disco[novo_bloco].tipo = 'A';
    
    // Encontrar um ponteiro livre no inode pai
    int ponteiro_livre = -1;
    for (int i = 0; i < 8; i++) {
        if (inode_pai.ponteiros[i] == -1) {
            ponteiro_livre = i;
            break;
        }
    }
    
    if (ponteiro_livre == -1) {
        printf("Erro: Diretório pai já tem 8 blocos de dados (limite atingido)\n");
        liberar_bloco(novo_bloco);
        return -1;
    }
    
    // Atualizar o inode pai
    inode_pai.ponteiros[ponteiro_livre] = novo_bloco;
    memcpy(disco[inode_dir_pai].dados, &inode_pai, sizeof(struct Inode));
    
    // Adicionar a entrada no novo bloco
    struct EntradaDiretorio nova_entrada;
    strncpy(nova_entrada.nome_arquivo, nome, sizeof(nova_entrada.nome_arquivo) - 1);
    nova_entrada.nome_arquivo[sizeof(nova_entrada.nome_arquivo) - 1] = '\0';
    nova_entrada.numero_inode = inode_filho;
    
    memcpy(disco[novo_bloco].dados, &nova_entrada, sizeof(struct EntradaDiretorio));
    
    printf("  Novo bloco %d alocado e entrada adicionada no slot 0\n", novo_bloco);
    return 0; // Sucesso
}

// Função para remover entrada de diretório
int remover_entrada_diretorio(int inode_dir_pai, const char* nome) {
    if (inode_dir_pai < 0 || inode_dir_pai >= NUMERO_TOTAL_BLOCOS || disco[inode_dir_pai].tipo != 'I') {
        printf("Erro: Inode do diretório pai inválido\n");
        return -1;
    }
    
    if (nome == NULL || strlen(nome) == 0) {
        printf("Erro: Nome inválido\n");
        return -1;
    }
    
    struct Inode inode_pai;
    memcpy(&inode_pai, disco[inode_dir_pai].dados, sizeof(struct Inode));
    
    printf("Removendo entrada '%s' do diretório %d\n", nome, inode_dir_pai);
    
    // Procurar a entrada nos blocos de dados
    for (int i = 0; i < 8; i++) {
        if (inode_pai.ponteiros[i] != -1) {
            int bloco_dados = inode_pai.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    // Verificar se é a entrada procurada
                    if (strcmp(entrada.nome_arquivo, nome) == 0) {
                        // Marcar a entrada como removida
                        entrada.nome_arquivo[0] = '\0';
                        entrada.numero_inode = -1;
                        
                        // Escrever a entrada modificada de volta
                        memcpy(disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), 
                               &entrada, sizeof(struct EntradaDiretorio));
                        
                        printf("  Entrada removida do bloco %d, slot %d\n", bloco_dados, j);
                        return 0; // Sucesso
                    }
                }
            }
        }
    }
    
    printf("Erro: Entrada '%s' não encontrada no diretório\n", nome);
    return -1; // Não encontrado
}
