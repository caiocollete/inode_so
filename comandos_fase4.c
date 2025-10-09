#include "sistema_arquivos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// Variável global para o diretório atual (declarada em shell_comandos.c)
extern int diretorio_atual;

// Função auxiliar para obter data e hora atual
void obter_data_hora_atual(char* data, char* hora) {
    time_t agora = time(NULL);
    struct tm* tm_info = localtime(&agora);
    
    // Formato: DD/MM/AAAA
    strftime(data, 11, "%d/%m/%Y", tm_info);
    
    // Formato: HH:MM:SS
    strftime(hora, 9, "%H:%M:%S", tm_info);
}

// Função auxiliar para verificar se arquivo existe no diretório atual
int verificar_arquivo_existe(const char* nome_arquivo) {
    if (diretorio_atual == -1) {
        return -1;
    }
    
    struct Inode inode_atual;
    converter_bloco_para_inode(&disco[diretorio_atual], &inode_atual);
    
    // Procurar arquivo nos blocos de dados do diretório atual
    for (int i = 0; i < 8; i++) {
        if (inode_atual.ponteiros[i] != -1) {
            int bloco_dados = inode_atual.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    // Verificar se é o arquivo procurado (e se a entrada é válida)
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1 && 
                        strcmp(entrada.nome_arquivo, nome_arquivo) == 0) {
                        return entrada.numero_inode;
                    }
                }
            }
        }
    }
    
    return -1; // Arquivo não encontrado
}

// Implementação do comando touch
void executar_touch_real(char* nome_arquivo, int tamanho_bytes) {
    if (nome_arquivo == NULL || strlen(nome_arquivo) == 0) {
        printf("Erro: Nome do arquivo não pode ser vazio\n");
        return;
    }
    
    if (tamanho_bytes < 0) {
        printf("Erro: Tamanho deve ser não negativo\n");
        return;
    }
    
    // Verificar se arquivo já existe
    int inode_existente = verificar_arquivo_existe(nome_arquivo);
    if (inode_existente != -1) {
        printf("Arquivo '%s' já existe\n", nome_arquivo);
        return;
    }
    
    // Obter bloco livre para novo inode
    int novo_inode = obter_bloco_livre();
    if (novo_inode == -1) {
        printf("Erro: Não há blocos livres para criar inode\n");
        return;
    }
    
    disco[novo_inode].tipo = 'I';
    
    // Inicializar o inode
    struct Inode inode_arquivo;
    strcpy(inode_arquivo.permissoes, "-rw-r--r--");
    obter_data_hora_atual(inode_arquivo.data, inode_arquivo.hora);
    inode_arquivo.tamanho_bytes = tamanho_bytes;
    strcpy(inode_arquivo.nome_usuario, "user");
    strcpy(inode_arquivo.nome_grupo, "users");
    inode_arquivo.contador_links = 1;
    
    // Inicializar ponteiros como não usados
    for (int i = 0; i < 8; i++) {
        inode_arquivo.ponteiros[i] = -1;
    }
    
    // Escrever inode no disco diretamente
    memcpy(disco[novo_inode].dados, &inode_arquivo, sizeof(struct Inode));
    
    // Alocar blocos para o arquivo se necessário
    if (tamanho_bytes > 0) {
        alocar_blocos_para_inode(novo_inode, tamanho_bytes);
    }
    
    // Adicionar entrada no diretório atual
    int resultado = adicionar_entrada_diretorio(diretorio_atual, nome_arquivo, novo_inode);
    if (resultado == 0) {
        printf("Arquivo '%s' criado com sucesso (tamanho: %d bytes)\n", nome_arquivo, tamanho_bytes);
    } else {
        printf("Erro: Falha ao adicionar arquivo ao diretório\n");
        liberar_bloco(novo_inode);
    }
}

// Implementação do comando mkdir
void executar_mkdir_real(char* nome_dir) {
    if (nome_dir == NULL || strlen(nome_dir) == 0) {
        printf("Erro: Nome do diretório não pode ser vazio\n");
        return;
    }
    
    // Verificar se diretório já existe
    int inode_existente = verificar_arquivo_existe(nome_dir);
    if (inode_existente != -1) {
        printf("Diretório '%s' já existe\n", nome_dir);
        return;
    }
    
    // Obter bloco livre para novo inode
    int novo_inode = obter_bloco_livre();
    if (novo_inode == -1) {
        printf("Erro: Não há blocos livres para criar inode\n");
        return;
    }
    
    disco[novo_inode].tipo = 'I';
    
    // Obter bloco livre para dados do diretório
    int bloco_dados = obter_bloco_livre();
    if (bloco_dados == -1) {
        printf("Erro: Não há blocos livres para dados do diretório\n");
        liberar_bloco(novo_inode);
        return;
    }
    
    disco[bloco_dados].tipo = 'A';
    
    // Inicializar o inode do diretório
    struct Inode inode_dir;
    strcpy(inode_dir.permissoes, "drwxr-xr-x");
    obter_data_hora_atual(inode_dir.data, inode_dir.hora);
    inode_dir.tamanho_bytes = 2 * sizeof(struct EntradaDiretorio); // Para "." e ".."
    strcpy(inode_dir.nome_usuario, "user");
    strcpy(inode_dir.nome_grupo, "users");
    inode_dir.contador_links = 2; // Para "." e ".."
    
    // Configurar ponteiros
    inode_dir.ponteiros[0] = bloco_dados;
    for (int i = 1; i < 8; i++) {
        inode_dir.ponteiros[i] = -1;
    }
    
    // Escrever inode no disco
    converter_inode_para_bloco(&inode_dir, &disco[novo_inode]);
    
    // Criar entradas "." e ".." no bloco de dados
    struct EntradaDiretorio entrada_ponto;
    strcpy(entrada_ponto.nome_arquivo, ".");
    entrada_ponto.numero_inode = novo_inode;
    
    struct EntradaDiretorio entrada_ponto_ponto;
    strcpy(entrada_ponto_ponto.nome_arquivo, "..");
    entrada_ponto_ponto.numero_inode = diretorio_atual;
    
    // Escrever entradas no bloco de dados
    memcpy(disco[bloco_dados].dados, &entrada_ponto, sizeof(struct EntradaDiretorio));
    memcpy(disco[bloco_dados].dados + sizeof(struct EntradaDiretorio), &entrada_ponto_ponto, sizeof(struct EntradaDiretorio));
    
    // Adicionar entrada no diretório atual
    int resultado = adicionar_entrada_diretorio(diretorio_atual, nome_dir, novo_inode);
    if (resultado == 0) {
        printf("Diretório '%s' criado com sucesso\n", nome_dir);
    } else {
        printf("Erro: Falha ao adicionar diretório ao diretório pai\n");
        liberar_bloco(novo_inode);
        liberar_bloco(bloco_dados);
    }
}

// Implementação do comando ls (versão simplificada)
void executar_ls_real(bool longa) {
    if (diretorio_atual == -1) {
        printf("Erro: Diretório atual não inicializado\n");
        return;
    }
    
    struct Inode inode_atual;
    memcpy(&inode_atual, disco[diretorio_atual].dados, sizeof(struct Inode));
    
    if (longa) {
        printf("total %d\n", inode_atual.tamanho_bytes);
    }
    
    // Percorrer todos os blocos de dados do diretório
    for (int i = 0; i < 8; i++) {
        if (inode_atual.ponteiros[i] != -1) {
            int bloco_dados = inode_atual.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    // Verificar se entrada é válida (incluindo "." e "..")
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                        if (longa) {
                            // Carregar inode do arquivo/diretório
                            struct Inode inode_arquivo;
                            memcpy(&inode_arquivo, disco[entrada.numero_inode].dados, sizeof(struct Inode));
                            
                            printf("%s %2d %-8s %-8s %8d %s %s %s\n",
                                   inode_arquivo.permissoes,
                                   inode_arquivo.contador_links,
                                   inode_arquivo.nome_usuario,
                                   inode_arquivo.nome_grupo,
                                   inode_arquivo.tamanho_bytes,
                                   inode_arquivo.data,
                                   inode_arquivo.hora,
                                   entrada.nome_arquivo);
                        } else {
                            printf("%s\n", entrada.nome_arquivo);
                        }
                    }
                }
            }
        }
    }
}

// Implementação do comando rm
void executar_rm_real(char* nome_arquivo) {
    if (nome_arquivo == NULL || strlen(nome_arquivo) == 0) {
        printf("Erro: Nome do arquivo não pode ser vazio\n");
        return;
    }
    
    // Encontrar inode do arquivo
    int inode_arquivo = verificar_arquivo_existe(nome_arquivo);
    if (inode_arquivo == -1) {
        printf("Arquivo '%s' não encontrado\n", nome_arquivo);
        return;
    }
    
    // Verificar se é um arquivo (não diretório)
    struct Inode inode;
    memcpy(&inode, disco[inode_arquivo].dados, sizeof(struct Inode));
    if (inode.permissoes[0] == 'd') {
        printf("Erro: '%s' é um diretório. Use 'rmdir' para remover diretórios\n", nome_arquivo);
        return;
    }
    
    // Liberar blocos do arquivo
    liberar_blocos_de_inode(inode_arquivo);
    
    // Liberar o inode
    liberar_bloco(inode_arquivo);
    
    // Remover entrada do diretório
    int resultado = remover_entrada_diretorio(diretorio_atual, nome_arquivo);
    if (resultado == 0) {
        printf("Arquivo '%s' removido com sucesso\n", nome_arquivo);
    } else {
        printf("Erro: Falha ao remover entrada do diretório\n");
    }
}

// Implementação do comando rmdir
void executar_rmdir_real(char* nome_dir) {
    if (nome_dir == NULL || strlen(nome_dir) == 0) {
        printf("Erro: Nome do diretório não pode ser vazio\n");
        return;
    }
    
    // Encontrar inode do diretório
    int inode_dir = verificar_arquivo_existe(nome_dir);
    if (inode_dir == -1) {
        printf("Diretório '%s' não encontrado\n", nome_dir);
        return;
    }
    if(inode_dir == NUMERO_TOTAL_BLOCOS-1){
        printf("Erro: Diretório raiz não pode ser removido\n");
        return;
    }
    
    // Verificar se é um diretório
    struct Inode inode;
    memcpy(&inode, disco[inode_dir].dados, sizeof(struct Inode));
    if (inode.permissoes[0] != 'd') {
        printf("Erro: '%s' não é um diretório\n", nome_dir);
        return;
    }
    
    // Verificar se diretório está vazio (apenas "." e "..")
    int entradas_validas = 0;
    for (int i = 0; i < 8; i++) {
        if (inode.ponteiros[i] != -1) {
            int bloco_dados = inode.ponteiros[i];
            if (disco[bloco_dados].tipo == 'A') {
                int entradas_por_bloco = 500 / sizeof(struct EntradaDiretorio);
                
                for (int j = 0; j < entradas_por_bloco; j++) {
                    struct EntradaDiretorio entrada;
                    memcpy(&entrada, disco[bloco_dados].dados + j * sizeof(struct EntradaDiretorio), sizeof(struct EntradaDiretorio));
                    
                    if (entrada.nome_arquivo[0] != '\0' && entrada.numero_inode != -1) {
                        // Contar apenas entradas que não são "." e ".."
                        if (strcmp(entrada.nome_arquivo, ".") != 0 && strcmp(entrada.nome_arquivo, "..") != 0) {
                            entradas_validas++;
                        }
                    }
                }
            }
        }
    }
    
    if (entradas_validas > 0) {
        printf("Erro: Diretório '%s' não está vazio (%d entradas)\n", nome_dir, entradas_validas);
        return;
    }
    
    // Liberar blocos do diretório
    liberar_blocos_de_inode(inode_dir);
    
    // Liberar o inode
    liberar_bloco(inode_dir);
    
    // Remover entrada do diretório pai
    int resultado = remover_entrada_diretorio(diretorio_atual, nome_dir);
    if (resultado == 0) {
        printf("Diretório '%s' removido com sucesso\n", nome_dir);
    } else {
        printf("Erro: Falha ao remover entrada do diretório pai\n");
    }
}

// Implementação do comando cd
void executar_cd_real(char* caminho) {
    if (caminho == NULL || strlen(caminho) == 0) {
        printf("Erro: Caminho não pode ser vazio\n");
        return;
    }
    
    // Encontrar inode do destino
    int inode_destino = encontrar_inode_por_caminho(caminho);
    if (inode_destino == -1) {
        printf("Erro: Caminho '%s' não encontrado\n", caminho);
        return;
    }
    
    // Verificar se é um diretório
    struct Inode inode;
    memcpy(&inode, disco[inode_destino].dados, sizeof(struct Inode));
    if (inode.permissoes[0] != 'd') {
        printf("Erro: '%s' não é um diretório\n", caminho);
        return;
    }
    
    // Atualizar diretório atual
    diretorio_atual = inode_destino;
    
    // Atualizar o caminho atual baseado na navegação
    if (strcmp(caminho, "..") == 0) {
        navegar_diretorio_pai();
    } else if (strcmp(caminho, "/") == 0) {
        atualizar_caminho_atual("/");
    } else if (caminho[0] == '/') {
        // Caminho absoluto
        atualizar_caminho_atual(caminho);
    } else {
        // Caminho relativo - adicionar ao caminho atual
        atualizar_caminho_atual(caminho);
    }
    
    // Mostrar novo diretório atual
    char novo_caminho[256];
    obter_caminho_atual(novo_caminho, sizeof(novo_caminho));
    printf("Diretório atual: %s\n", novo_caminho);
}

// Implementação do comando chmod
void executar_chmod_real(char* permissoes, char* nome_arquivo) {
    if (permissoes == NULL || nome_arquivo == NULL) {
        printf("Erro: Permissões e nome do arquivo são obrigatórios\n");
        return;
    }
    
    if (strlen(permissoes) != 10) {
        printf("Erro: Formato de permissões inválido. Use formato: drwxr-xr-x\n");
        return;
    }
    
    // Encontrar inode do arquivo
    int inode_arquivo = verificar_arquivo_existe(nome_arquivo);
    if (inode_arquivo == -1) {
        printf("Arquivo '%s' não encontrado\n", nome_arquivo);
        return;
    }
    
    // Carregar inode
    struct Inode inode;
    memcpy(&inode, disco[inode_arquivo].dados, sizeof(struct Inode));
    
    // Atualizar permissões
    strcpy(inode.permissoes, permissoes);
    
    // Salvar inode atualizado
    converter_inode_para_bloco(&inode, &disco[inode_arquivo]);
    
    printf("Permissões de '%s' alteradas para: %s\n", nome_arquivo, permissoes);
}

// Implementação do comando link
void executar_link_real(char* origem, char* destino, bool simbolico) {
    if (origem == NULL || destino == NULL) {
        printf("Erro: Origem e destino são obrigatórios\n");
        return;
    }
    
    // Verificar se destino já existe
    int destino_existente = verificar_arquivo_existe(destino);
    if (destino_existente != -1) {
        printf("Erro: Destino '%s' já existe\n", destino);
        return;
    }
    
    if (simbolico) {
        // Link simbólico - criar novo arquivo com caminho como conteúdo
        int novo_inode = obter_bloco_livre();
        if (novo_inode == -1) {
            printf("Erro: Não há blocos livres para criar inode\n");
            return;
        }
        
        disco[novo_inode].tipo = 'I';
        
        // Inicializar inode do link simbólico
        struct Inode inode_link;
        strcpy(inode_link.permissoes, "lrwxrwxrwx");
        obter_data_hora_atual(inode_link.data, inode_link.hora);
        inode_link.tamanho_bytes = strlen(origem);
        strcpy(inode_link.nome_usuario, "user");
        strcpy(inode_link.nome_grupo, "users");
        inode_link.contador_links = 1;
        
        // Inicializar ponteiros
        for (int i = 0; i < 8; i++) {
            inode_link.ponteiros[i] = -1;
        }
        
        // Alocar bloco para conteúdo (caminho)
        if (inode_link.tamanho_bytes > 0) {
            int bloco_conteudo = obter_bloco_livre();
            if (bloco_conteudo == -1) {
                printf("Erro: Não há blocos livres para conteúdo\n");
                liberar_bloco(novo_inode);
                return;
            }
            
            disco[bloco_conteudo].tipo = 'A';
            inode_link.ponteiros[0] = bloco_conteudo;
            
            // Escrever caminho no bloco
            strncpy(disco[bloco_conteudo].dados, origem, 500);
            disco[bloco_conteudo].dados[499] = '\0';
        }
        
        // Escrever inode no disco
        converter_inode_para_bloco(&inode_link, &disco[novo_inode]);
        
        // Adicionar entrada no diretório atual
        int resultado = adicionar_entrada_diretorio(diretorio_atual, destino, novo_inode);
        if (resultado == 0) {
            printf("Link simbólico '%s' -> '%s' criado com sucesso\n", destino, origem);
        } else {
            printf("Erro: Falha ao criar link simbólico\n");
            liberar_bloco(novo_inode);
        }
    } else {
        // Link físico - encontrar inode da origem
        int inode_origem = encontrar_inode_por_caminho(origem);
        if (inode_origem == -1) {
            printf("Erro: Arquivo de origem '%s' não encontrado\n", origem);
            return;
        }
        
        // Verificar se origem não é diretório
        struct Inode inode_origem_struct;
        memcpy(&inode_origem_struct, disco[inode_origem].dados, sizeof(struct Inode));
        if (inode_origem_struct.permissoes[0] == 'd') {
            printf("Erro: Não é possível criar link físico para diretório\n");
            return;
        }
        
        // Criar nova entrada apontando para o mesmo inode
        int resultado = adicionar_entrada_diretorio(diretorio_atual, destino, inode_origem);
        if (resultado == 0) {
            // Incrementar contador de links
            inode_origem_struct.contador_links++;
            converter_inode_para_bloco(&inode_origem_struct, &disco[inode_origem]);
            
            printf("Link físico '%s' -> '%s' criado com sucesso\n", destino, origem);
        } else {
            printf("Erro: Falha ao criar link físico\n");
        }
    }
}

// Implementação do comando bad
void executar_bad_real(int numero_bloco) {
    if (numero_bloco < 0 || numero_bloco >= NUMERO_TOTAL_BLOCOS) {
        printf("Erro: Número de bloco inválido (%d)\n", numero_bloco);
        return;
    }
    
    if (disco[numero_bloco].tipo == 'B') {
        printf("Bloco %d já está marcado como defeituoso\n", numero_bloco);
        return;
    }
    
    // Marcar bloco como defeituoso
    disco[numero_bloco].tipo = 'B';
    
    printf("Bloco %d marcado como defeituoso\n", numero_bloco);
}

// Implementação do comando df
void executar_df_real() {
    int blocos_livres = 0;
    int blocos_ocupados = 0;
    int blocos_defeituosos = 0;
    
    // Contar blocos por tipo
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        switch (disco[i].tipo) {
            case 'F':
                blocos_livres++;
                break;
            case 'B':
                blocos_defeituosos++;
                break;
            case 'I':
            case 'A':
                blocos_ocupados++;
                break;
        }
    }
    
    int total_blocos = NUMERO_TOTAL_BLOCOS;
    int bytes_por_bloco = 500; // Tamanho do bloco em bytes
    
    printf("Sistema de Arquivos - Uso do Disco:\n");
    printf("=====================================\n");
    printf("Total de blocos:     %d (%.2f MB)\n", total_blocos, (total_blocos * bytes_por_bloco) / (1024.0 * 1024.0));
    printf("Blocos ocupados:     %d (%.2f MB)\n", blocos_ocupados, (blocos_ocupados * bytes_por_bloco) / (1024.0 * 1024.0));
    printf("Blocos livres:       %d (%.2f MB)\n", blocos_livres, (blocos_livres * bytes_por_bloco) / (1024.0 * 1024.0));
    printf("Blocos defeituosos:  %d (%.2f MB)\n", blocos_defeituosos, (blocos_defeituosos * bytes_por_bloco) / (1024.0 * 1024.0));
    printf("Uso:                 %.1f%%\n", (blocos_ocupados * 100.0) / total_blocos);
}

// Implementação do comando vi
void executar_vi_real(char* nome_arquivo) {
    if (nome_arquivo == NULL || strlen(nome_arquivo) == 0) {
        printf("Erro: Nome do arquivo não pode ser vazio\n");
        return;
    }
    
    // Verificar se arquivo existe
    int numero_inode = verificar_arquivo_existe(nome_arquivo);
    if (numero_inode == -1) {
        printf("vi: '%s': Arquivo não encontrado\n", nome_arquivo);
        return;
    }
    
    // Exibir mensagem de visualização com número do inode
    printf("Arquivo '%s' foi visualizado (inode: %d)\n", nome_arquivo, numero_inode);
}
