#include "sistema_arquivos.h"

// Função para demonstrar a resolução de caminhos
void demonstrar_resolucao_caminhos() {
    printf("\n=== DEMONSTRAÇÃO DE RESOLUÇÃO DE CAMINHOS ===\n");
    
    // Testar caminho raiz
    int inode_raiz = encontrar_inode_por_caminho("/");
    if (inode_raiz != -1) {
        printf("✓ Caminho '/' resolve para inode %d\n", inode_raiz);
    } else {
        printf("✗ Falha ao resolver caminho '/'\n");
    }
    
    // Testar caminho relativo
    int inode_atual = encontrar_inode_por_caminho(".");
    if (inode_atual != -1) {
        printf("✓ Caminho '.' resolve para inode %d\n", inode_atual);
    } else {
        printf("✗ Falha ao resolver caminho '.'\n");
    }
    
    // Testar caminho pai
    int inode_pai = encontrar_inode_por_caminho("..");
    if (inode_pai != -1) {
        printf("✓ Caminho '..' resolve para inode %d\n", inode_pai);
    } else {
        printf("✗ Falha ao resolver caminho '..'\n");
    }
}

// Função para demonstrar alocação de blocos
void demonstrar_alocacao_blocos() {
    printf("\n=== DEMONSTRAÇÃO DE ALOCAÇÃO DE BLOCOS ===\n");
    
    // Criar um novo inode para teste
    int novo_inode = obter_bloco_livre();
    if (novo_inode == -1) {
        printf("Erro: Não há blocos livres para criar inode de teste\n");
        return;
    }
    
    disco[novo_inode].tipo = 'I';
    
    // Configurar o inode
    struct Inode inode_teste;
    strcpy(inode_teste.permissoes, "-rw-r--r--");
    strcpy(inode_teste.data, "15/01/2024");
    strcpy(inode_teste.hora, "14:30:00");
    inode_teste.tamanho_bytes = 0;
    strcpy(inode_teste.nome_usuario, "user");
    strcpy(inode_teste.nome_grupo, "users");
    inode_teste.contador_links = 1;
    
    // Inicializar ponteiros como não usados
    for (int i = 0; i < 8; i++) {
        inode_teste.ponteiros[i] = -1;
    }
    
    converter_inode_para_bloco(&inode_teste, &disco[novo_inode]);
    printf("Inode de teste criado no bloco %d\n", novo_inode);
    
    // Testar alocação de diferentes tamanhos
    printf("\nTestando alocação de 25 bytes (3 blocos):\n");
    alocar_blocos_para_inode(novo_inode, 25);
    
    printf("\nTestando alocação de 100 bytes (10 blocos):\n");
    alocar_blocos_para_inode(novo_inode, 100);
    
    // Mostrar informações do inode após alocação
    struct Inode inode_atualizado;
    converter_bloco_para_inode(&disco[novo_inode], &inode_atualizado);
    printf("\nInode após alocação:\n");
    printf("  Tamanho: %d bytes\n", inode_atualizado.tamanho_bytes);
    printf("  Ponteiros diretos: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", inode_atualizado.ponteiros[i]);
    }
    printf("\n  Ponteiros indiretos: ");
    for (int i = 5; i < 8; i++) {
        printf("%d ", inode_atualizado.ponteiros[i]);
    }
    printf("\n");
    
    // Liberar os blocos
    printf("\nLiberando blocos do inode de teste:\n");
    liberar_blocos_de_inode(novo_inode);
    
    // Liberar o próprio inode
    liberar_bloco(novo_inode);
}

// Função para demonstrar manipulação de diretórios
void demonstrar_manipulacao_diretorios() {
    printf("\n=== DEMONSTRAÇÃO DE MANIPULAÇÃO DE DIRETÓRIOS ===\n");
    
    // Encontrar o inode raiz
    int inode_raiz = encontrar_inode_por_caminho("/");
    if (inode_raiz == -1) {
        printf("Erro: Inode raiz não encontrado\n");
        return;
    }
    
    // Criar um novo inode para arquivo
    int novo_inode = obter_bloco_livre();
    if (novo_inode == -1) {
        printf("Erro: Não há blocos livres para criar inode\n");
        return;
    }
    
    disco[novo_inode].tipo = 'I';
    
    // Configurar o inode do arquivo
    struct Inode inode_arquivo;
    strcpy(inode_arquivo.permissoes, "-rw-r--r--");
    strcpy(inode_arquivo.data, "15/01/2024");
    strcpy(inode_arquivo.hora, "15:00:00");
    inode_arquivo.tamanho_bytes = 50;
    strcpy(inode_arquivo.nome_usuario, "user");
    strcpy(inode_arquivo.nome_grupo, "users");
    inode_arquivo.contador_links = 1;
    
    // Inicializar ponteiros como não usados
    for (int i = 0; i < 8; i++) {
        inode_arquivo.ponteiros[i] = -1;
    }
    
    converter_inode_para_bloco(&inode_arquivo, &disco[novo_inode]);
    printf("Inode de arquivo criado no bloco %d\n", novo_inode);
    
    // Adicionar entrada no diretório raiz
    printf("\nAdicionando arquivo 'teste.txt' no diretório raiz:\n");
    int resultado = adicionar_entrada_diretorio(inode_raiz, "teste.txt", novo_inode);
    if (resultado == 0) {
        printf("✓ Arquivo adicionado com sucesso\n");
    } else {
        printf("✗ Falha ao adicionar arquivo\n");
    }
    
    // Testar resolução do novo arquivo
    printf("\nTestando resolução do caminho '/teste.txt':\n");
    int inode_resolvido = encontrar_inode_por_caminho("/teste.txt");
    if (inode_resolvido == novo_inode) {
        printf("✓ Caminho '/teste.txt' resolve corretamente para inode %d\n", inode_resolvido);
    } else {
        printf("✗ Falha ao resolver caminho '/teste.txt'\n");
    }
    
    // Adicionar mais arquivos para testar múltiplas entradas
    for (int i = 1; i <= 3; i++) {
        char nome_arquivo[50];
        sprintf(nome_arquivo, "arquivo%d.txt", i);
        
        // Criar novo inode
        int inode_temp = obter_bloco_livre();
        if (inode_temp == -1) {
            printf("Erro: Não há blocos livres para criar inode %d\n", i);
            break;
        }
        
        disco[inode_temp].tipo = 'I';
        
        // Configurar inode
        struct Inode inode_temp_struct;
        strcpy(inode_temp_struct.permissoes, "-rw-r--r--");
        strcpy(inode_temp_struct.data, "15/01/2024");
        strcpy(inode_temp_struct.hora, "15:00:00");
        inode_temp_struct.tamanho_bytes = 30;
        strcpy(inode_temp_struct.nome_usuario, "user");
        strcpy(inode_temp_struct.nome_grupo, "users");
        inode_temp_struct.contador_links = 1;
        
        for (int j = 0; j < 8; j++) {
            inode_temp_struct.ponteiros[j] = -1;
        }
        
        converter_inode_para_bloco(&inode_temp_struct, &disco[inode_temp]);
        
        // Adicionar entrada
        printf("Adicionando %s:\n", nome_arquivo);
        resultado = adicionar_entrada_diretorio(inode_raiz, nome_arquivo, inode_temp);
        if (resultado == 0) {
            printf("  ✓ %s adicionado com sucesso\n", nome_arquivo);
        } else {
            printf("  ✗ Falha ao adicionar %s\n", nome_arquivo);
            liberar_bloco(inode_temp);
        }
    }
    
    // Testar remoção de arquivo
    printf("\nRemovendo arquivo 'teste.txt':\n");
    resultado = remover_entrada_diretorio(inode_raiz, "teste.txt");
    if (resultado == 0) {
        printf("✓ Arquivo removido com sucesso\n");
    } else {
        printf("✗ Falha ao remover arquivo\n");
    }
    
    // Verificar se o caminho não resolve mais
    printf("\nVerificando se '/teste.txt' ainda resolve:\n");
    inode_resolvido = encontrar_inode_por_caminho("/teste.txt");
    if (inode_resolvido == -1) {
        printf("✓ Caminho '/teste.txt' não resolve mais (arquivo removido)\n");
    } else {
        printf("✗ Caminho '/teste.txt' ainda resolve (erro na remoção)\n");
    }
    
    // Liberar o inode do arquivo
    liberar_bloco(novo_inode);
}

// Função para mostrar informações do sistema
void mostrar_informacoes_sistema_fase2() {
    printf("\n=== INFORMAÇÕES DO SISTEMA ===\n");
    
    // Contar blocos por tipo
    int blocos_livres = 0, blocos_inode = 0, blocos_arquivo = 0, blocos_defeituosos = 0;
    
    for (int i = 0; i < NUMERO_TOTAL_BLOCOS; i++) {
        switch (disco[i].tipo) {
            case 'F': blocos_livres++; break;
            case 'I': blocos_inode++; break;
            case 'A': blocos_arquivo++; break;
            case 'B': blocos_defeituosos++; break;
        }
    }
    
    printf("Distribuição de blocos:\n");
    printf("  - Livres (F): %d\n", blocos_livres);
    printf("  - Inode (I): %d\n", blocos_inode);
    printf("  - Arquivo/Diretório (A): %d\n", blocos_arquivo);
    printf("  - Defeituosos (B): %d\n", blocos_defeituosos);
    printf("  - Total: %d\n", NUMERO_TOTAL_BLOCOS);
    
    // Mostrar inode raiz
    int inode_raiz = encontrar_inode_raiz();
    if (inode_raiz != -1) {
        struct Inode inode_raiz_struct;
        converter_bloco_para_inode(&disco[inode_raiz], &inode_raiz_struct);
        
        printf("\nDiretório Raiz (Bloco %d):\n", inode_raiz);
        printf("  - Permissões: %s\n", inode_raiz_struct.permissoes);
        printf("  - Usuário: %s\n", inode_raiz_struct.nome_usuario);
        printf("  - Grupo: %s\n", inode_raiz_struct.nome_grupo);
        printf("  - Links: %d\n", inode_raiz_struct.contador_links);
        printf("  - Ponteiros: ");
        for (int i = 0; i < 8; i++) {
            printf("%d ", inode_raiz_struct.ponteiros[i]);
        }
        printf("\n");
    }
}
