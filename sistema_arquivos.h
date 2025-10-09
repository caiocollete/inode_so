#ifndef SISTEMA_ARQUIVOS_H
#define SISTEMA_ARQUIVOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Estrutura que representa o inode principal de um arquivo ou diretório
struct Inode {
    char permissoes[11];        // Formato: "-rwxr-xr--" ou "drwxr-xr--" ou "lrwxr-xr--"
    char data[11];              // Data da última modificação
    char hora[9];               // Hora da última modificação
    int tamanho_bytes;          // Tamanho do arquivo em bytes
    char nome_usuario[50];      // Nome do usuário dono
    char nome_grupo[50];        // Nome do grupo
    int contador_links;         // Contador para hard links
    int ponteiros[8];           // Ponteiros de blocos: 5 diretos + 3 indiretos
};

// Estrutura para blocos de inode de extensão
struct InodeExtensao {
    int ponteiros[5];           // 5 ponteiros para blocos
};

// Estrutura que representa uma entrada em um diretório
struct EntradaDiretorio {
    char nome_arquivo[100];     // Nome do arquivo ou subdiretório
    int numero_inode;           // Número do bloco que contém o inode
};

// Estrutura que representa um bloco do disco simulado
struct Bloco {
    char tipo;                  // 'F' (Livre), 'B' (Defeituoso), 'I' (Inode), 'A' (Arquivo/Diretório)
    char dados[500];            // Array de 500 bytes para simular conteúdo do bloco (acomodar múltiplas estruturas)
};

// Estrutura para nó da pilha de blocos livres
struct NoPilha {
    int numero_bloco;           // Número do bloco
    struct NoPilha* proximo;    // Ponteiro para o próximo nó
};

// Variáveis globais
struct Bloco* disco;            // Ponteiro para o array de blocos do disco
int NUMERO_TOTAL_BLOCOS;        // Tamanho total do disco em blocos
struct NoPilha* pilha_blocos_livres;  // Ponteiro para o topo da pilha de blocos livres

// Protótipos de funções auxiliares para conversão de dados
void converter_bloco_para_inode(struct Bloco* bloco, struct Inode* inode);
void converter_inode_para_bloco(struct Inode* inode, struct Bloco* bloco);
void converter_bloco_para_inode_extensao(struct Bloco* bloco, struct InodeExtensao* extensao);
void converter_inode_extensao_para_bloco(struct InodeExtensao* extensao, struct Bloco* bloco);
void converter_bloco_para_entrada_diretorio(struct Bloco* bloco, struct EntradaDiretorio* entrada);
void converter_entrada_diretorio_para_bloco(struct EntradaDiretorio* entrada, struct Bloco* bloco);

// Protótipos de funções para gerenciamento da pilha
void inicializar_pilha();
void empilhar_bloco_livre(int numero_bloco);
int desempilhar_bloco_livre();
int pilha_vazia();
void liberar_pilha();

// Protótipos das funções da Fase 1 - Gerenciamento do Disco
void push(int numero_bloco);
int pop();
void inicializar_disco();
int obter_bloco_livre();
void liberar_bloco(int numero_bloco);
void configurar_diretorio_raiz();

// Protótipos das funções da Fase 2 - Lógica Central do Sistema de Arquivos
int encontrar_inode_por_caminho(const char* caminho);
int encontrar_inode_por_caminho_com_inicial(const char* caminho, int inode_inicial);
void alocar_blocos_para_inode(int numero_inode, int tamanho_bytes);
void liberar_blocos_de_inode(int numero_inode);
int adicionar_entrada_diretorio(int inode_dir_pai, const char* nome, int inode_filho);
int remover_entrada_diretorio(int inode_dir_pai, const char* nome);

// Protótipos das funções auxiliares da Fase 2
int calcular_blocos_necessarios(int tamanho_bytes);
int encontrar_inode_raiz();
int encontrar_slot_vazio(int bloco_dados, struct EntradaDiretorio* entrada_vazia);

// Protótipos das funções de demonstração da Fase 2
void demonstrar_resolucao_caminhos();
void demonstrar_alocacao_blocos();
void demonstrar_manipulacao_diretorios();
void mostrar_informacoes_sistema_fase2();

// Protótipos das funções da Fase 3 - Shell/Interpretador de Comandos
void inicializar_diretorio_atual();
void obter_caminho_atual(char* caminho, int tamanho);
void exibir_prompt();
int ler_entrada(char* buffer, int tamanho);
int parse_comando(const char* entrada, char** tokens, int max_tokens);
void liberar_tokens(char** tokens, int count);
void executar_comando(char** tokens, int token_count);
void executar_shell();

// Protótipos das funções de comandos do shell
void executar_ls(char** argumentos);
void executar_mkdir(char** argumentos);
void executar_cd(char** argumentos);
void executar_touch(char** argumentos);
void executar_rm(char** argumentos);
void executar_rmdir(char** argumentos);
void executar_chmod(char** argumentos);
void executar_link(char** argumentos);
void executar_bad(char** argumentos);
void executar_df(char** argumentos);
void executar_cat(char** argumentos);
void executar_echo(char** argumentos);
void executar_pwd(char** argumentos);
void executar_vi(char** argumentos);
void executar_report(char** argumentos);
void executar_help(char** argumentos);

// Protótipos das funções reais da Fase 4 - Implementação dos Comandos
void executar_touch_real(char* nome_arquivo, int tamanho_bytes);
void executar_mkdir_real(char* nome_dir);
void executar_ls_real(bool longa);
void executar_rm_real(char* nome_arquivo);
void executar_rmdir_real(char* nome_dir);
void executar_cd_real(char* caminho);
void executar_chmod_real(char* permissoes, char* nome_arquivo);
void executar_link_real(char* origem, char* destino, bool simbolico);
void executar_bad_real(int numero_bloco);
void executar_df_real();
void executar_vi_real(char* nome_arquivo);

// Protótipos das funções auxiliares da Fase 4
void obter_data_hora_atual(char* data, char* hora);
int verificar_arquivo_existe(const char* nome_arquivo);

// Protótipos das funções de navegação de caminho
void atualizar_caminho_atual(const char* novo_caminho);
void navegar_diretorio_pai();

// Protótipos das funções de relatórios da Fase 5
void report_blocos_ocupados(char* nome_arquivo);
void report_tamanho_maximo();
void report_integridade();
void report_blocos_perdidos();
void report_estado_disco();
void report_explorer_view();
void report_arvore_detalhada();
void report_links();

// Protótipos das funções auxiliares dos relatórios
int contar_blocos_arquivo_recursivo(int inode_num, bool* blocos_visitados);
int contar_blocos_indiretos(int bloco_ponteiros, bool* blocos_visitados);
bool verificar_integridade_arquivo(int inode_num, bool* blocos_visitados);
bool verificar_integridade_indiretos(int bloco_ponteiros, bool* blocos_visitados);
void listar_arquivos_recursivo(int inode_dir, char* caminho_atual, bool* blocos_visitados, bool* integridade_ok);
void marcar_blocos_referenciados_recursivo(int inode_num, bool* blocos_referenciados);
void marcar_blocos_indiretos(int bloco_ponteiros, bool* blocos_referenciados);
void explorer_view_recursivo(int inode_dir, char* caminho_atual, int nivel_indentacao);
void arvore_detalhada_recursivo(int inode_dir, char* caminho_atual, int nivel);
void listar_links_recursivo(int inode_dir, char* caminho_atual, bool* blocos_visitados);


#endif // SISTEMA_ARQUIVOS_H
