#include "sistema_arquivos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// Variável global para o diretório atual
int diretorio_atual = -1;

// Variável global para o caminho atual
char caminho_atual[512] = "/";

// Função para inicializar o diretório atual
void inicializar_diretorio_atual() {
    diretorio_atual = encontrar_inode_raiz();
    if (diretorio_atual == -1) {
        printf("Erro: Não foi possível encontrar o diretório raiz\n");
        exit(1);
    }
}

// Função para obter o caminho do diretório atual
void obter_caminho_atual(char* caminho, int tamanho) {
    strncpy(caminho, caminho_atual, tamanho - 1);
    caminho[tamanho - 1] = '\0';
}

// Função para atualizar o caminho atual após cd
void atualizar_caminho_atual(const char* novo_caminho) {
    if (novo_caminho == NULL) {
        strcpy(caminho_atual, "/");
        return;
    }
    
    // Se o caminho começa com /, é um caminho absoluto
    if (novo_caminho[0] == '/') {
        strncpy(caminho_atual, novo_caminho, sizeof(caminho_atual) - 1);
        caminho_atual[sizeof(caminho_atual) - 1] = '\0';
        return;
    }
    
    // Processar caminho relativo com resolução de ".." e "."
    char caminho_temp[512];
    strncpy(caminho_temp, caminho_atual, sizeof(caminho_temp) - 1);
    caminho_temp[sizeof(caminho_temp) - 1] = '\0';
    
    // Dividir o caminho em componentes
    char componentes[64][64];
    int num_componentes = 0;
    
    // Adicionar componentes do caminho atual
    char* token = strtok(caminho_temp, "/");
    while (token != NULL && num_componentes < 64) {
        strncpy(componentes[num_componentes], token, 63);
        componentes[num_componentes][63] = '\0';
        num_componentes++;
        token = strtok(NULL, "/");
    }
    
    // Adicionar componentes do novo caminho
    char novo_caminho_copy[256];
    strncpy(novo_caminho_copy, novo_caminho, sizeof(novo_caminho_copy) - 1);
    novo_caminho_copy[sizeof(novo_caminho_copy) - 1] = '\0';
    
    token = strtok(novo_caminho_copy, "/");
    while (token != NULL && num_componentes < 64) {
        if (strcmp(token, "..") == 0) {
            // Voltar um nível
            if (num_componentes > 0) {
                num_componentes--;
            }
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            // Adicionar componente (ignorar ".")
            strncpy(componentes[num_componentes], token, 63);
            componentes[num_componentes][63] = '\0';
            num_componentes++;
        }
        token = strtok(NULL, "/");
    }
    
    // Reconstruir o caminho
    strcpy(caminho_atual, "/");
    for (int i = 0; i < num_componentes; i++) {
        if (strlen(caminho_atual) > 1) {
            strcat(caminho_atual, "/");
        }
        strcat(caminho_atual, componentes[i]);
    }
}

// Função para navegar para o diretório pai
void navegar_diretorio_pai() {
    if (strcmp(caminho_atual, "/") == 0) {
        return; // Já estamos na raiz
    }
    
    // Encontrar a última barra
    char* ultima_barra = strrchr(caminho_atual, '/');
    if (ultima_barra != NULL && ultima_barra != caminho_atual) {
        *ultima_barra = '\0';
    } else {
        strcpy(caminho_atual, "/");
    }
}

// Função para exibir o prompt
void exibir_prompt() {
    char caminho[256];
    obter_caminho_atual(caminho, sizeof(caminho));
    printf("sistema_arquivos:%s$ ", caminho);
    fflush(stdout);
}

// Função para ler entrada do usuário
int ler_entrada(char* buffer, int tamanho) {
    if (fgets(buffer, tamanho, stdin) == NULL) {
        return 0; // EOF ou erro
    }
    
    // Remover quebra de linha
    buffer[strcspn(buffer, "\n")] = '\0';
    return 1;
}

// Função para dividir comando em tokens
int parse_comando(const char* entrada, char** tokens, int max_tokens) {
    if (entrada == NULL || tokens == NULL) {
        return 0;
    }
    
    // Fazer uma cópia da entrada para não modificar a original
    char entrada_copia[strlen(entrada) + 1];
    strcpy(entrada_copia, entrada);
    
    int token_count = 0;
    char* token = strtok(entrada_copia, " \t");
    
    while (token != NULL && token_count < max_tokens) {
        tokens[token_count] = malloc(strlen(token) + 1);
        if (tokens[token_count] == NULL) {
            printf("Erro: Falha na alocação de memória\n");
            // Liberar tokens já alocados
            for (int i = 0; i < token_count; i++) {
                free(tokens[i]);
            }
            return 0;
        }
        strcpy(tokens[token_count], token);
        token_count++;
        token = strtok(NULL, " \t");
    }
    
    tokens[token_count] = NULL; // Terminar array com NULL
    return token_count;
}

// Função para liberar tokens
void liberar_tokens(char** tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
}

// Funções que conectam o shell com as implementações reais da Fase 4

void executar_ls(char** argumentos) {
    bool longa = false;
    
    // Verificar se foi passado -l
    for (int i = 1; argumentos[i] != NULL; i++) {
        if (strcmp(argumentos[i], "-l") == 0) {
            longa = true;
            break;
        }
    }
    
    executar_ls_real(longa);
}

void executar_mkdir(char** argumentos) {
    if (argumentos[1] == NULL) {
        printf("Erro: Nome do diretório é obrigatório\n");
        return;
    }
    
    executar_mkdir_real(argumentos[1]);
}

void executar_cd(char** argumentos) {
    char* destino = argumentos[1];
    if (destino == NULL) {
        destino = "/"; // Padrão: diretório raiz
    }
    
    executar_cd_real(destino);
}

void executar_touch(char** argumentos) {
    if (argumentos[1] == NULL) {
        printf("Erro: Nome do arquivo é obrigatório\n");
        return;
    }
    
    // Verificar se foi especificado tamanho
    int tamanho = 0;
    if (argumentos[2] != NULL) {
        tamanho = atoi(argumentos[2]);
        if (tamanho < 0) {
            printf("Erro: Tamanho deve ser não negativo\n");
            return;
        }
    }
    
    executar_touch_real(argumentos[1], tamanho);
}

void executar_rm(char** argumentos) {
    if (argumentos[1] == NULL) {
        printf("Erro: Nome do arquivo é obrigatório\n");
        return;
    }
    
    executar_rm_real(argumentos[1]);
}

void executar_cat(char** argumentos) {
    printf("Comando 'cat' executado (placeholder)\n");
    printf("Argumentos recebidos: ");
    for (int i = 1; argumentos[i] != NULL; i++) {
        printf("'%s' ", argumentos[i]);
    }
    printf("\n");
}

void executar_echo(char** argumentos) {
    printf("Comando 'echo' executado (placeholder)\n");
    printf("Argumentos recebidos: ");
    for (int i = 1; argumentos[i] != NULL; i++) {
        printf("'%s' ", argumentos[i]);
    }
    printf("\n");
}

void executar_pwd(char** argumentos) {
    (void)argumentos; // Evitar warning de parâmetro não utilizado
    char caminho[256];
    obter_caminho_atual(caminho, sizeof(caminho));
    printf("%s\n", caminho);
}

void executar_rmdir(char** argumentos) {
    if (argumentos[1] == NULL) {
        printf("Erro: Nome do diretório é obrigatório\n");
        return;
    }
    
    executar_rmdir_real(argumentos[1]);
}

void executar_chmod(char** argumentos) {
    if (argumentos[1] == NULL || argumentos[2] == NULL) {
        printf("Erro: Uso: chmod <permissoes> <arquivo>\n");
        printf("Exemplo: chmod -rwxr-xr-x arquivo.txt\n");
        return;
    }
    
    executar_chmod_real(argumentos[1], argumentos[2]);
}

void executar_link(char** argumentos) {
    if (argumentos[1] == NULL || argumentos[2] == NULL) {
        printf("Erro: Uso: link [-s] <origem> <destino>\n");
        printf("  -s: criar link simbólico\n");
        return;
    }
    
    bool simbolico = false;
    char* origem;
    char* destino;
    
    // Verificar se foi passado -s
    if (strcmp(argumentos[1], "-s") == 0) {
        simbolico = true;
        origem = argumentos[2];
        destino = argumentos[3];
        
        if (destino == NULL) {
            printf("Erro: Destino é obrigatório\n");
            return;
        }
    } else {
        origem = argumentos[1];
        destino = argumentos[2];
    }
    
    executar_link_real(origem, destino, simbolico);
}

void executar_bad(char** argumentos) {
    if (argumentos[1] == NULL) {
        printf("Erro: Número do bloco é obrigatório\n");
        return;
    }
    
    int numero_bloco = atoi(argumentos[1]);
    executar_bad_real(numero_bloco);
}

void executar_df(char** argumentos) {
    (void)argumentos; // Evitar warning de parâmetro não utilizado
    executar_df_real();
}

void executar_report(char** argumentos) {
    if (argumentos[1] == NULL) {
        printf("Uso: report <numero> [argumentos]\n");
        printf("Relatórios disponíveis:\n");
        printf("  1 <arquivo>     - Blocos ocupados por arquivo\n");
        printf("  2               - Tamanho máximo de arquivo\n");
        printf("  3               - Integridade dos arquivos\n");
        printf("  4               - Blocos perdidos\n");
        printf("  5               - Estado atual do disco\n");
        printf("  6               - Visualização Explorer\n");
        printf("  7               - Árvore detalhada\n");
        printf("  8               - Visualização de links\n");
        return;
    }
    
    int numero_relatorio = atoi(argumentos[1]);
    
    switch (numero_relatorio) {
        case 1:
            if (argumentos[2] == NULL) {
                printf("Erro: Nome do arquivo é obrigatório para o relatório 1\n");
            } else {
                report_blocos_ocupados(argumentos[2]);
            }
            break;
        case 2:
            report_tamanho_maximo();
            break;
        case 3:
            report_integridade();
            break;
        case 4:
            report_blocos_perdidos();
            break;
        case 5:
            report_estado_disco();
            break;
        case 6:
            report_explorer_view();
            break;
        case 7:
            report_arvore_detalhada();
            break;
        case 8:
            report_links();
            break;
        default:
            printf("Erro: Número de relatório inválido. Use 1-8.\n");
            break;
    }
}

void executar_help(char** argumentos) {
    (void)argumentos; // Evitar warning de parâmetro não utilizado
    printf("Comandos disponíveis:\n");
    printf("  ls [-l]    - Listar conteúdo do diretório\n");
    printf("  mkdir      - Criar diretório\n");
    printf("  cd         - Mudar diretório\n");
    printf("  touch      - Criar arquivo\n");
    printf("  rm         - Remover arquivo\n");
    printf("  rmdir      - Remover diretório vazio\n");
    printf("  chmod      - Alterar permissões\n");
    printf("  link [-s]  - Criar link físico ou simbólico\n");
    printf("  bad        - Marcar bloco como defeituoso\n");
    printf("  df         - Mostrar uso do disco\n");
    printf("  pwd        - Mostrar diretório atual\n");
    printf("  report     - Gerar relatórios do sistema\n");
    printf("  help       - Mostrar esta ajuda\n");
    printf("  exit       - Sair do programa\n");
}

// Função para executar comando no processo filho
void executar_comando(char** tokens, int token_count) {
    if (token_count == 0) {
        return; // Comando vazio
    }
    
    char* comando = tokens[0];
    
    // Verificar comandos válidos
    if (strcmp(comando, "ls") == 0) {
        executar_ls(tokens);
    } else if (strcmp(comando, "mkdir") == 0) {
        executar_mkdir(tokens);
    } else if (strcmp(comando, "cd") == 0) {
        executar_cd(tokens);
    } else if (strcmp(comando, "touch") == 0) {
        executar_touch(tokens);
    } else if (strcmp(comando, "rm") == 0) {
        executar_rm(tokens);
    } else if (strcmp(comando, "rmdir") == 0) {
        executar_rmdir(tokens);
    } else if (strcmp(comando, "chmod") == 0) {
        executar_chmod(tokens);
    } else if (strcmp(comando, "link") == 0) {
        executar_link(tokens);
    } else if (strcmp(comando, "bad") == 0) {
        executar_bad(tokens);
    } else if (strcmp(comando, "df") == 0) {
        executar_df(tokens);
    } else if (strcmp(comando, "cat") == 0) {
        executar_cat(tokens);
    } else if (strcmp(comando, "echo") == 0) {
        executar_echo(tokens);
    } else if (strcmp(comando, "pwd") == 0) {
        executar_pwd(tokens);
    } else if (strcmp(comando, "report") == 0) {
        executar_report(tokens);
    } else if (strcmp(comando, "help") == 0) {
        executar_help(tokens);
    } else {
        printf("Comando não encontrado: '%s'\n", comando);
        printf("Digite 'help' para ver os comandos disponíveis.\n");
    }
}

// Função principal do shell
void executar_shell() {
    char entrada[1024];
    char* tokens[64]; // Máximo de 64 tokens por comando
    int token_count;
    
    printf("=== SHELL DO SIMULADOR DE SISTEMA DE ARQUIVOS ===\n");
    printf("Digite 'help' para ver os comandos disponíveis.\n");
    printf("Digite 'exit' para sair.\n\n");
    
    while (1) {
        // Exibir prompt
        exibir_prompt();
        
        // Ler entrada do usuário
        if (!ler_entrada(entrada, sizeof(entrada))) {
            printf("\nEOF detectado. Saindo...\n");
            break;
        }
        
        // Verificar se é comando de saída
        if (strcmp(entrada, "exit") == 0) {
            printf("Saindo do simulador...\n");
            break;
        }
        
        // Ignorar linhas vazias
        if (strlen(entrada) == 0) {
            continue;
        }
        
        // Parse do comando
        token_count = parse_comando(entrada, tokens, 64);
        if (token_count == 0) {
            continue;
        }
        
        // Executar comando diretamente (sem fork para manter estado do sistema de arquivos)
        executar_comando(tokens, token_count);
        liberar_tokens(tokens, token_count);
    }
}
