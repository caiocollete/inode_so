#include "sistema_arquivos.h"

int main() {
    printf("=== SIMULADOR DE SISTEMA DE ARQUIVOS ===\n");
    
    // Inicializar o disco
    inicializar_disco();
    
    // Configurar o diretório raiz
    configurar_diretorio_raiz();
    
    // Inicializar o diretório atual do shell
    inicializar_diretorio_atual();
    
    // Mostrar informações do sistema após inicialização
    printf("\n=== INFORMAÇÕES DO SISTEMA ===\n");
    printf("Total de blocos no disco: %d\n", NUMERO_TOTAL_BLOCOS);
    printf("Bloco inode do diretório raiz: encontrado\n");
    printf("Bloco de dados do diretório raiz: encontrado\n");
    printf("Diretório atual inicializado\n\n");
    
    // Executar o shell
    executar_shell();
    
    // Limpar memória
    liberar_pilha();
    free(disco);
    
    printf("\nSimulador finalizado com sucesso!\n");
    return 0;
}
