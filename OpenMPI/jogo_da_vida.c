#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"
#define HIGH 1
#define NORMAL 0
#define VIVO 1
#define MORTO 0
#define SEL_E_L 0
#define SEL_BUFF_INF 1
#define SEL_BUFF_SUP 2

int getNeighbors(int *grid, int i, int j, int N, int tamLocal, int *buf_sup, int *buf_inf) {
  int count = 0;
  int i_local, j_local;
  int vizinho;
  int flag = SEL_E_L;
  for (int k = -1; k < 2; k++) {
    for (int l = -1; l < 2; l++) {
      flag = SEL_E_L;
      i_local = i + k;
      j_local = j + l;

      if (i_local < 0) { // infinito por cima
        flag = SEL_BUFF_SUP;
      } else if (i_local == tamLocal) { // infinito por baixo
        flag = SEL_BUFF_INF;
      }

      if (j_local < 0) { // infinito pela esquerda
        j_local = N - 1;
      } else if (j_local == N) { // infinito por direita
        j_local = 0;
      }

      if (flag == SEL_BUFF_SUP) {

        vizinho = buf_sup[j_local];

      } else if (flag == SEL_BUFF_INF) {

        vizinho = buf_inf[j_local];

      } else {

        vizinho = *(grid + i_local*N + j_local);

      }

      if (vizinho > 0 && (i_local != i || j_local != j)) {
        count++;
      }

    }
  }

  return count;
}

int main(int argc, char **argv) {

  int processId;
	int noProcesses;
  int init, fim, tamLocal;
  int vizinho_inferior, vizinho_superior;
  int *grid, *newgrid, *buff_superior, *buff_inferior;
  int *msg_inferior, *msg_superior;
  int lin, col;
  int soma_local, soma_aux, soma_total;
  int N = 2048;
  int n_iteracoes = 10;
  int tipo = HIGH;
  int count_aux;
  MPI_Status status;

  MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &noProcesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &processId);

  tamLocal = N/noProcesses;
  init = processId*tamLocal;

  if (processId + 1 > noProcesses - 1) {
    vizinho_inferior = 0;
  } else {
    vizinho_inferior = processId + 1;
  }

  if (processId - 1 < 0) {
    vizinho_superior = noProcesses - 1;
  } else {
    vizinho_superior = processId - 1;
  }

  grid = (int *) malloc (tamLocal * N * sizeof(int));
  newgrid = (int *) malloc (tamLocal * N * sizeof(int));

  buff_inferior = (int *) malloc (N * sizeof(int));
  buff_superior = (int *) malloc (N * sizeof(int));

  msg_inferior = (int *) malloc (N * sizeof(int));
  msg_superior = (int *) malloc (N * sizeof(int));

  // Inicializacao do vetor local

  for (int i = 0; i < tamLocal; i++) {
    for (int j = 0; j < N; j++) {
      *(grid + i*N + j) = 0;
      *(newgrid + i*N + j) = 0;
    }
  }

  // Glider e Pentomimo

  if (processId == 0) {
    //Glider
    lin = 1;
    col = 1;
    *(grid + (lin+1)*N + (col+2)) = 1;
    *(grid + (lin+2)*N + col) = 1;
    *(grid + lin*N + (col+1)) = 1;
    *(grid + (lin+2)*N + (col+1)) = 1;
    *(grid + (lin+2)*N + (col+2)) = 1;
    //Pentomimo
    lin = 10;
    col = 30;
    *(grid + lin*N + (col+1)) = 1;
    *(grid + lin*N + (col+2)) = 1;
    *(grid + (lin + 1)*N + col) = 1;
    *(grid + (lin+1)*N + (col+1)) = 1;
    *(grid + (lin+2)*N + (col+1)) = 1;
  }

  // condicao inicial

  soma_local = 0;
  soma_aux = 0;
  soma_total = 0;

  for (int k = 0; k < tamLocal; k++) {
    for (int l = 0; l < tamLocal; l++) {
      soma_local += *(grid + k*N + l);
    }
  }

  if (processId == 0) {
    soma_total += soma_local;
    for (int i = 1; i < noProcesses; i++) {
      MPI_Recv(&soma_aux, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
      soma_total += soma_aux;
    }
    printf("Condicao Inicial: %d\n", soma_total);
  } else {
    MPI_Send(&soma_local, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  }

  // Jogo
  MPI_Barrier(MPI_COMM_WORLD);
  for (int n = 0; n < n_iteracoes; n++) {

    soma_local = 0;
    soma_aux = 0;
    soma_total = 0;

    // prepara o array das linhas de borda para enviar aos vizinhos

    for (int i = 0; i < N; i++) {
      msg_superior[i] = *(grid + i);
      msg_inferior[i] = *(grid + (tamLocal-1)*N + i);
    }

    // troca de mensagens das linhas de borda

    if (processId % 2 == 0) {
      MPI_Send(msg_inferior, N, MPI_INT, vizinho_inferior, 1, MPI_COMM_WORLD);
      MPI_Send(msg_superior, N, MPI_INT, vizinho_superior, 1, MPI_COMM_WORLD);
      MPI_Recv(buff_superior, N, MPI_INT, vizinho_superior, 1, MPI_COMM_WORLD, &status);
      MPI_Recv(buff_inferior, N, MPI_INT, vizinho_inferior, 1, MPI_COMM_WORLD, &status);
    } else {
      MPI_Recv(buff_superior, N, MPI_INT, vizinho_superior, 1, MPI_COMM_WORLD, &status);
      MPI_Recv(buff_inferior, N, MPI_INT, vizinho_inferior, 1, MPI_COMM_WORLD, &status);
      MPI_Send(msg_inferior, N, MPI_INT, vizinho_inferior, 1, MPI_COMM_WORLD);
      MPI_Send(msg_superior, N, MPI_INT, vizinho_superior, 1, MPI_COMM_WORLD);
    }

    // executa o jogo
    for (int k = 0; k < tamLocal; k++) {
      for (int l = 0; l < N; l ++) {
        count_aux = getNeighbors(grid, k, l, N, tamLocal, buff_superior, buff_inferior);
        if (*(grid + k*N + l) == VIVO) {
          if (count_aux < 2 || count_aux >= 4) {
            *(newgrid + k*N + l) = MORTO;
          } else {
            *(newgrid + k*N + l) = VIVO;
          }
        } else {
          if (tipo != HIGH) {
            if (count_aux == 3){
              *(newgrid + k*N + l) = VIVO;
            } else {
              *(newgrid + k*N + l) = MORTO;
            }
          } else {
            if (count_aux == 3 || count_aux == 6) {
              *(newgrid + k*N + l) = VIVO;
            } else {
              *(newgrid + k*N + l) = MORTO;
            }
          }
        }
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // copia o newgrid para grid e faz a soma

    for (int k = 0; k < tamLocal; k++) {
      for (int l = 0; l < tamLocal; l++) {
        *(grid + k*N + l) = *(newgrid + k*N + l);
        soma_local += *(newgrid + k*N + l);
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (n < 5 || n == n_iteracoes - 1) {
      if (processId == 0) {
        soma_total += soma_local;
        for (int i = 1; i < noProcesses; i++) {
          MPI_Recv(&soma_aux, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
          soma_total += soma_aux;
        }
        printf("Geracao %d: %d\n", n + 1, soma_total);
      } else {
        MPI_Send(&soma_local, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
      }
    }
  }

  //printf("Processo %d: v_inferior = %d v_superior = %d\n", processId, vizinho_inferior, vizinho_superior);
  free(grid);
  free(newgrid);
  free(buff_inferior);
  free(buff_superior);
  MPI_Finalize();

  return 0;
}
