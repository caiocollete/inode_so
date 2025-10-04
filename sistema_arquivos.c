#include "sistema_arquivos.h"

// Função para converter dados de um bloco para a estrutura Inode
void converter_bloco_para_inode(struct Bloco* bloco, struct Inode* inode) {
    if (bloco == NULL || inode == NULL) return;
    
    memcpy(inode, bloco->dados, sizeof(struct Inode));
}

// Função para converter dados de uma estrutura Inode para um bloco
void converter_inode_para_bloco(struct Inode* inode, struct Bloco* bloco) {
    if (inode == NULL || bloco == NULL) return;
    
    // Não alterar o tipo do bloco aqui - isso deve ser feito externamente
    memcpy(bloco->dados, inode, sizeof(struct Inode));
}

// Função para converter dados de um bloco para a estrutura InodeExtensao
void converter_bloco_para_inode_extensao(struct Bloco* bloco, struct InodeExtensao* extensao) {
    if (bloco == NULL || extensao == NULL) return;
    
    memcpy(extensao, bloco->dados, sizeof(struct InodeExtensao));
}

// Função para converter dados de uma estrutura InodeExtensao para um bloco
void converter_inode_extensao_para_bloco(struct InodeExtensao* extensao, struct Bloco* bloco) {
    if (extensao == NULL || bloco == NULL) return;
    
    // Não alterar o tipo do bloco aqui - isso deve ser feito externamente
    memcpy(bloco->dados, extensao, sizeof(struct InodeExtensao));
}

// Função para converter dados de um bloco para a estrutura EntradaDiretorio
void converter_bloco_para_entrada_diretorio(struct Bloco* bloco, struct EntradaDiretorio* entrada) {
    if (bloco == NULL || entrada == NULL) return;
    
    memcpy(entrada, bloco->dados, sizeof(struct EntradaDiretorio));
}

// Função para converter dados de uma estrutura EntradaDiretorio para um bloco
void converter_entrada_diretorio_para_bloco(struct EntradaDiretorio* entrada, struct Bloco* bloco) {
    if (entrada == NULL || bloco == NULL) return;
    
    // Não alterar o tipo do bloco aqui - isso deve ser feito externamente
    memcpy(bloco->dados, entrada, sizeof(struct EntradaDiretorio));
}

// Função para inicializar a pilha de blocos livres
void inicializar_pilha() {
    pilha_blocos_livres = NULL;
}

// Função para empilhar um bloco livre
void empilhar_bloco_livre(int numero_bloco) {
    struct NoPilha* novo_no = (struct NoPilha*)malloc(sizeof(struct NoPilha));
    if (novo_no == NULL) {
        printf("Erro: Falha ao alocar memória para novo nó da pilha\n");
        return;
    }
    
    novo_no->numero_bloco = numero_bloco;
    novo_no->proximo = pilha_blocos_livres;
    pilha_blocos_livres = novo_no;
}

// Função para desempilhar um bloco livre
int desempilhar_bloco_livre() {
    if (pilha_vazia()) {
        printf("Erro: Tentativa de desempilhar de uma pilha vazia\n");
        return -1;
    }
    
    struct NoPilha* no_remover = pilha_blocos_livres;
    int numero_bloco = no_remover->numero_bloco;
    pilha_blocos_livres = no_remover->proximo;
    free(no_remover);
    
    return numero_bloco;
}

// Função para verificar se a pilha está vazia
int pilha_vazia() {
    return pilha_blocos_livres == NULL;
}

// Função para liberar toda a pilha
void liberar_pilha() {
    while (!pilha_vazia()) {
        desempilhar_bloco_livre();
    }
}
