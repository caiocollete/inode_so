#include "sistema_arquivos.h"
#include <time.h>

// Implementação das funções push e pop para pilha de blocos livres
void push(int numero_bloco) {
    struct NoPilha* novo_no = (struct NoPilha*)malloc(sizeof(struct NoPilha));
    if (novo_no == NULL) {
        printf("Erro: Falha ao alocar memória para novo nó da pilha\n");
        return;
    }
    
    novo_no->numero_bloco = numero_bloco;
    novo_no->proximo = pilha_blocos_livres;
    pilha_blocos_livres = novo_no;
}

int pop() {
    if (pilha_blocos_livres == NULL) {
        return -1;  // Pilha vazia
    }
    
    struct NoPilha* no_remover = pilha_blocos_livres;
    int numero_bloco = no_remover->numero_bloco;
    pilha_blocos_livres = no_remover->proximo;
    free(no_remover);
    
    return numero_bloco;
}

// Função para inicializar o disco
void inicializar_disco() {
    printf("=== Inicialização do Disco ===\n");
    printf("Digite a quantidade de blocos do disco (padrão: 1000): ");
    
    int quantidade_blocos;
    if (scanf("%d", &quantidade_blocos) != 1 || quantidade_blocos <= 0) {
        printf("Entrada inválida. Usando valor padrão de 1000 blocos.\n");
        quantidade_blocos = 1000;
    }
    
    NUMERO_TOTAL_BLOCOS = quantidade_blocos;
    
    // Alocar dinamicamente a memória para o disco
    disco = (struct Bloco*)malloc(NUMERO_TOTAL_BLOCOS * sizeof(struct Bloco));
    if (disco == NULL) {
        printf("Erro: Falha ao alocar memória para o disco\n");
        exit(1);
    }
    
    // Inicializar pilha de blocos livres como vazia
    pilha_blocos_livres = NULL;
    
    // Percorrer todo o array disco e inicializar como livres
    printf("Inicializando %d blocos como livres...\n", NUMERO_TOTAL_BLOCOS);
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        disco[i].tipo = 'F';
        push(i);
    }
    
    printf("Disco inicializado com sucesso!\n");
    printf("Total de blocos: %d\n", NUMERO_TOTAL_BLOCOS);
}

// Função para obter um bloco livre
int obter_bloco_livre() {
    int numero_bloco = pop();
    if (numero_bloco == -1) {
        printf("Erro: Nenhum bloco livre disponível\n");
        return -1;
    }
    return numero_bloco;
}

// Função para liberar um bloco
void liberar_bloco(int numero_bloco) {
    if (numero_bloco < 0 || numero_bloco >= NUMERO_TOTAL_BLOCOS) {
        printf("Erro: Número de bloco inválido (%d)\n", numero_bloco);
        return;
    }
    
    disco[numero_bloco].tipo = 'F';
    push(numero_bloco);
    printf("Bloco %d liberado com sucesso\n", numero_bloco);
}

// Função para configurar o diretório raiz
void configurar_diretorio_raiz() {
    printf("\n=== Configuração do Diretório Raiz ===\n");
    
    // Alocar bloco 0 para o inode do diretório raiz
    int bloco_inode = obter_bloco_livre();
    if (bloco_inode == -1) {
        printf("Erro: Não foi possível alocar bloco para inode do diretório raiz\n");
        return;
    }
    disco[bloco_inode].tipo = 'I';
    printf("Bloco %d alocado para inode do diretório raiz\n", bloco_inode);
    
    // Alocar bloco 1 para os dados do diretório raiz
    int bloco_dados = obter_bloco_livre();
    if (bloco_dados == -1) {
        printf("Erro: Não foi possível alocar bloco para dados do diretório raiz\n");
        return;
    }
    disco[bloco_dados].tipo = 'A';
    printf("Bloco %d alocado para dados do diretório raiz\n", bloco_dados);
    
    // Inicializar o inode do diretório raiz
    struct Inode inode_raiz;
    strcpy(inode_raiz.permissoes, "drwxr-xr-x");
    strcpy(inode_raiz.data, "01/01/2024");
    strcpy(inode_raiz.hora, "00:00:00");
    inode_raiz.tamanho_bytes = 0;
    strcpy(inode_raiz.nome_usuario, "root");
    strcpy(inode_raiz.nome_grupo, "root");
    inode_raiz.contador_links = 2;  // Para "." e ".."
    
    // Inicializar ponteiros (apenas o primeiro aponta para dados)
    inode_raiz.ponteiros[0] = bloco_dados;
    for (int i = 1; i < 8; i++) {
        inode_raiz.ponteiros[i] = -1;  // -1 indica ponteiro não usado
    }
    
    // Escrever o inode no bloco 0
    memcpy(disco[bloco_inode].dados, &inode_raiz, sizeof(struct Inode));
    printf("Inode do diretório raiz configurado\n");
    
    // Criar as entradas do diretório raiz
    struct EntradaDiretorio entrada_ponto;
    strcpy(entrada_ponto.nome_arquivo, ".");
    entrada_ponto.numero_inode = bloco_inode;
    
    struct EntradaDiretorio entrada_ponto_ponto;
    strcpy(entrada_ponto_ponto.nome_arquivo, "..");
    entrada_ponto_ponto.numero_inode = bloco_inode;
    
    // Escrever as entradas no bloco de dados
    // Primeira entrada (.)
    memcpy(disco[bloco_dados].dados, &entrada_ponto, sizeof(struct EntradaDiretorio));
    
    // Segunda entrada (..)
    memcpy(disco[bloco_dados].dados + sizeof(struct EntradaDiretorio), 
           &entrada_ponto_ponto, sizeof(struct EntradaDiretorio));
    
    printf("Entradas do diretório raiz criadas:\n");
    printf("  - \".\" -> inode %d\n", bloco_inode);
    printf("  - \"..\" -> inode %d\n", bloco_inode);
    
    printf("Diretório raiz configurado com sucesso!\n");
}
