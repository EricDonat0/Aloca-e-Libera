#include <stdio.h>
#include <stddef.h>

#define TAMANHO_MEMORIA (16 * 1024)

/*
 * ============================================================
 * LISTA DUPLAMENTE ENCADEADA
 * ============================================================
 */

typedef struct No {
    int valor;
    struct No *anterior;
    struct No *proximo;
} No;

typedef struct {
    No *inicio;
    No *fim;
    size_t tamanho;
} Lista;


/*
 * ============================================================
 * GERENCIADOR DE MEMORIA
 * ============================================================
 */

/*
 * Cada bloco da nossa memoria possui um cabecalho.
 *
 * O cabecalho informa:
 * - tamanho: quantidade de bytes disponiveis para o usuario
 * - livre: 1 se o bloco estiver livre, 0 se estiver ocupado
 * - proximo: proximo bloco da memoria
 */
typedef struct Bloco {
    size_t tamanho;
    int livre;
    struct Bloco *proximo;
} Bloco;


/*
 * Os 16 KiB pedidos pelo professor.
 */
_Alignas(max_align_t)
static unsigned char memoria[TAMANHO_MEMORIA];


/*
 * Primeiro bloco da nossa memoria.
 */
static Bloco *primeiro_bloco = NULL;


/*
 * Inicializa o primeiro bloco.
 */
static void inicializa_memoria(void)
{
    primeiro_bloco = (Bloco *)memoria;

    primeiro_bloco->tamanho =
        TAMANHO_MEMORIA - sizeof(Bloco);

    primeiro_bloco->livre = 1;
    primeiro_bloco->proximo = NULL;
}


/*
 * Funcao equivalente ao malloc.
 *
 * Recebe a quantidade de bytes desejada
 * e retorna um ponteiro para a memoria.
 */
void *aloca(size_t tamanho)
{
    Bloco *atual;

    if (tamanho == 0)
        return NULL;

    /*
     * Na primeira chamada, inicializamos
     * toda a memoria como um unico bloco livre.
     */
    if (primeiro_bloco == NULL)
        inicializa_memoria();

    atual = primeiro_bloco;

    /*
     * Procura um bloco livre que tenha
     * espaco suficiente.
     */
    while (atual != NULL) {

        if (atual->livre && atual->tamanho >= tamanho) {

            /*
             * Se houver espaco suficiente,
             * dividimos o bloco em dois:
             *
             * [bloco ocupado][bloco livre]
             */
            if (atual->tamanho >=
                tamanho + sizeof(Bloco) + 1) {

                Bloco *novo;

                novo = (Bloco *)(
                    (unsigned char *)atual
                    + sizeof(Bloco)
                    + tamanho
                );

                novo->tamanho =
                    atual->tamanho
                    - tamanho
                    - sizeof(Bloco);

                novo->livre = 1;
                novo->proximo = atual->proximo;

                atual->tamanho = tamanho;
                atual->proximo = novo;
            }

            /*
             * Marca o bloco como ocupado.
             */
            atual->livre = 0;

            /*
             * Retorna somente a parte destinada
             * ao usuario, e nao o cabecalho.
             */
            return (unsigned char *)atual
                   + sizeof(Bloco);
        }

        atual = atual->proximo;
    }

    /*
     * Nao encontramos espaco suficiente.
     */
    return NULL;
}


/*
 * Junta blocos livres consecutivos.
 *
 * Isso evita deixar varios pequenos espacos
 * livres separados.
 */
static void junta_blocos_livres(void)
{
    Bloco *atual;

    if (primeiro_bloco == NULL)
        return;

    atual = primeiro_bloco;

    while (atual != NULL && atual->proximo != NULL) {

        if (atual->livre &&
            atual->proximo->livre) {

            atual->tamanho +=
                sizeof(Bloco)
                + atual->proximo->tamanho;

            atual->proximo =
                atual->proximo->proximo;
        } else {
            atual = atual->proximo;
        }
    }
}


/*
 * Funcao equivalente ao free.
 *
 * Recebe um ponteiro anteriormente retornado
 * por aloca() e devolve o bloco para nossa memoria.
 */
void libera(void *ponteiro)
{
    Bloco *bloco;

    if (ponteiro == NULL)
        return;

    /*
     * O ponteiro aponta para depois do cabecalho.
     *
     * Voltamos sizeof(Bloco) bytes para encontrar
     * o cabecalho correspondente.
     */
    bloco = (Bloco *)(
        (unsigned char *)ponteiro
        - sizeof(Bloco)
    );

    /*
     * Marca o bloco como livre.
     */
    bloco->livre = 1;

    /*
     * Tenta juntar blocos livres consecutivos.
     */
    junta_blocos_livres();
}


/*
 * ============================================================
 * FUNCOES DA LISTA
 * ============================================================
 */


/*
 * Inicializa uma lista vazia.
 */
void lista_inicializa(Lista *lista)
{
    lista->inicio = NULL;
    lista->fim = NULL;
    lista->tamanho = 0;
}


/*
 * Insere um elemento no final da lista.
 *
 * IMPORTANTE:
 * O no e criado usando nossa funcao aloca(),
 * e nao malloc().
 */
int lista_insere_fim(Lista *lista, int valor)
{
    No *novo;

    novo = (No *)aloca(sizeof(No));

    if (novo == NULL)
        return 0;

    novo->valor = valor;
    novo->anterior = lista->fim;
    novo->proximo = NULL;

    if (lista->fim != NULL) {
        lista->fim->proximo = novo;
    } else {
        lista->inicio = novo;
    }

    lista->fim = novo;
    lista->tamanho++;

    return 1;
}


/*
 * Insere um elemento no inicio da lista.
 */
int lista_insere_inicio(Lista *lista, int valor)
{
    No *novo;

    novo = (No *)aloca(sizeof(No));

    if (novo == NULL)
        return 0;

    novo->valor = valor;
    novo->anterior = NULL;
    novo->proximo = lista->inicio;

    if (lista->inicio != NULL) {
        lista->inicio->anterior = novo;
    } else {
        lista->fim = novo;
    }

    lista->inicio = novo;
    lista->tamanho++;

    return 1;
}


/*
 * Remove o primeiro elemento que possuir
 * o valor informado.
 */
int lista_remove(Lista *lista, int valor)
{
    No *atual = lista->inicio;

    while (atual != NULL) {

        if (atual->valor == valor) {

            if (atual->anterior != NULL) {
                atual->anterior->proximo =
                    atual->proximo;
            } else {
                lista->inicio =
                    atual->proximo;
            }

            if (atual->proximo != NULL) {
                atual->proximo->anterior =
                    atual->anterior;
            } else {
                lista->fim =
                    atual->anterior;
            }

            /*
             * Devolve o no para nosso gerenciador
             * de memoria.
             */
            libera(atual);

            lista->tamanho--;

            return 1;
        }

        atual = atual->proximo;
    }

    return 0;
}


/*
 * Imprime a lista do inicio para o fim.
 */
void lista_imprime(const Lista *lista)
{
    const No *atual = lista->inicio;

    printf("Lista: ");

    while (atual != NULL) {
        printf("%d", atual->valor);

        if (atual->proximo != NULL)
            printf(" <-> ");

        atual = atual->proximo;
    }

    printf("\n");
}


/*
 * Imprime a lista do fim para o inicio.
 *
 * Isso demonstra que ela realmente e
 * duplamente encadeada.
 */
void lista_imprime_reversa(const Lista *lista)
{
    const No *atual = lista->fim;

    printf("Reversa: ");

    while (atual != NULL) {
        printf("%d", atual->valor);

        if (atual->anterior != NULL)
            printf(" <-> ");

        atual = atual->anterior;
    }

    printf("\n");
}


/*
 * Libera todos os nos da lista.
 */
void lista_destroi(Lista *lista)
{
    No *atual = lista->inicio;

    while (atual != NULL) {
        No *proximo = atual->proximo;

        libera(atual);

        atual = proximo;
    }

    lista->inicio = NULL;
    lista->fim = NULL;
    lista->tamanho = 0;
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(void)
{
    Lista lista;

    lista_inicializa(&lista);

    /*
     * Criamos elementos usando nossa lista.
     *
     * Internamente, lista_insere_* usa ALoca(),
     * e nao malloc().
     */
    lista_insere_fim(&lista, 10);
    lista_insere_fim(&lista, 20);
    lista_insere_fim(&lista, 30);
    lista_insere_inicio(&lista, 5);

    lista_imprime(&lista);
    lista_imprime_reversa(&lista);

    printf("Quantidade de elementos: %zu\n",
           lista.tamanho);

    /*
     * Remove o elemento 20.
     *
     * Internamente, lista_remove() usa libera().
     */
    lista_remove(&lista, 20);

    printf("\nDepois de remover 20:\n");

    lista_imprime(&lista);
    lista_imprime_reversa(&lista);

    printf("Quantidade de elementos: %zu\n",
           lista.tamanho);

    /*
     * Libera todos os elementos restantes.
     */
    lista_destroi(&lista);

    printf("\nLista destruida.\n");
    printf("Quantidade de elementos: %zu\n",
           lista.tamanho);

    return 0;
}
