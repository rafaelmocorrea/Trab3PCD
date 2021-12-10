#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define VIVO 1
#define MORTO 0
#define HIGH 1
#define NORMAL 0

void imprime_matriz(int** matriz, int n){
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            printf("%d ", matriz[i][j]);
        }
        printf("\n");
    }
}

int getNeighbors(int** grid, int i, int j, int n){
    int count = 0;
    int i_local, j_local;
    for (int k = -1; k < 2; k++){
        for (int l = -1; l < 2; l++){
            i_local = i + k;
            j_local = j + l;
            if (i_local < 0){ // infinito pela esquerda
                i_local = n - 1;
            } else if (i_local == n){
                i_local = 0; // infinito pela direita
            }
            if (j_local < 0){
                j_local = n - 1; // infinito por cima
            } else if (j_local == n){
                j_local = 0; // infinito por baixo
            }
            if (grid[i_local][j_local] > 0 && (i_local != i || j_local != j)){
                count++;
            }
        }
    }

    return count;
}

int somaMatriz(int **grid, int n){
    int count = 0;

    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            count += grid[i][j];
        }
    }

    return count;
}

void glider(int **grid){
    int lin = 1, col = 1;
    grid[lin+1][col+2] = 1;
    grid[lin+2][col  ] = 1;
    grid[lin  ][col+1] = 1;
    grid[lin+2][col+1] = 1;
    grid[lin+2][col+2] = 1;

}

void pentomino(int **grid){
    int lin = 10, col = 30;
    grid[lin  ][col+1] = 1;
    grid[lin  ][col+2] = 1;
    grid[lin+1][col  ] = 1;
    grid[lin+1][col+1] = 1;
    grid[lin+2][col+1] = 1;

}

void copia_matriz(int **destino, int **origem, int n){
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            destino[i][j] = origem[i][j];
        }
    }
}

int main(int argc, char **argv){

    int **grid;             //grid atual
    int **newgrid;          //proximo grid
    int N = 2048;           //NxN
    int iteracoes = 2000;    //Numero de iteracoes
    int cont_aux;           //Contador auxiliar
    int tipo = HIGH;        //Tipo do jogo
    int i, j, k, l;         //Auxiliares dos lacos
    int n_proc = 1;
    struct timespec inicio_prog, fim_prog;
    long total_diff;

    if (argc != 2) {
      printf("Uso: %s <num_processadores>\n", argv[0]);
      return 0;
    }

    n_proc = atoi(argv[1]);

    clock_gettime(CLOCK_MONOTONIC_RAW, &inicio_prog);

    printf("Numero de processadores escolhidos: %d\n",n_proc);

    grid = calloc(N, sizeof(int *));
    newgrid = calloc(N, sizeof(int *));

    for (j = 0; j < N; j++){
        grid[j] = calloc(N,sizeof(int));
        newgrid[j] = calloc(N,sizeof(int));
    }

    glider(grid);
    pentomino(grid);

    printf("Condicao inicial: %d",somaMatriz(grid,N));

    for (i = 0; i < iteracoes; i++){
        #pragma omp parallel num_threads(n_proc)
            {
            #pragma omp for private(k,l,cont_aux) collapse(2)
            for (k = 0; k < N; k++){
                for (l = 0; l < N; l++){
                    cont_aux = getNeighbors(grid,k,l,N);
                    if (grid[k][l] == VIVO){
                        if (cont_aux < 2 || cont_aux >= 4){
                            newgrid[k][l] = MORTO;
                        } else {
                            newgrid[k][l] = VIVO;
                        }
                    } else {
                        if (tipo != HIGH){
                            if (cont_aux == 3){
                                newgrid[k][l] = VIVO;
                            } else {
                                newgrid[k][l] = MORTO;
                            }
                        } else {
                            if (cont_aux == 3 || cont_aux == 6){
                                newgrid[k][l] = VIVO;
                            } else {
                                newgrid[k][l] = MORTO;
                            }
                        }
                    }
                }
            }
        }
        copia_matriz(grid,newgrid,N);
        if (i+1 == iteracoes)
            printf("\nGeracao %d: %d",i+1,somaMatriz(grid,N));
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &fim_prog);

    total_diff = ((fim_prog.tv_sec - inicio_prog.tv_sec)*(1000*1000*1000) + (fim_prog.tv_nsec - inicio_prog.tv_nsec)) / 1000000;

    printf("\nTempo total: %lums\n", total_diff);


    free(grid);
    free(newgrid);

    return 0;
}
