#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#define exit_on_error(s,m) if (s < 0) { perror(m); exit(1); }
#define chave_shm 1000
#define tam_shm 100
#define chave_msg 1100
#define tam_msg 255
#define MAX_PRODUTOS 20
#define BD "database"
#define chave_sem 1110
#define MAX_CLIENTES 100

struct Produto {
 int codigo;
 char nome[100];
 float preco;
}PRODUTO;

struct mensagem {
 long para; // pid destino
 long de; // pid remetente
 char texto[tam_msg];
};

struct clientes {
 long pid[MAX_CLIENTES];
};

/* semaforos  */

union semun { /* codigo copiado da manpage da semctl: */
 int val; /* value for SETVAL */
 struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
 unsigned short *array; /* array for GETALL, SETALL */
 struct seminfo *__buf; /* buffer for IPC_INFO */
};

int sem_criar_n(int chave, int numero_de_semaforos, int permissoes)
{
  int id;
  id=semget(chave, numero_de_semaforos, IPC_CREAT|IPC_EXCL|permissoes);
  exit_on_error(id,"Erro ao criar semaforo");
  return id;
}

void sem_ini_var_n(int id, int indice_do_semaforo, int valor)
{
  int r;
  union semun su;
  su.val=valor;
  r=semctl(id, indice_do_semaforo, SETVAL, su);
  exit_on_error(r,"Erro ao atribuir valor ao semaforo");
}

int sem_criar(int chave, int permissoes, int valor)
{
  int id = sem_criar_n(chave, 1, permissoes);
  sem_ini_var_n(id, 0, valor);
  return id;
}

int sem_id(int chave)
{
  int id;
  id=semget(chave, 0, 0);
  exit_on_error(id,"Erro ao obter id do semaforo");
  return id;
}

void sem_remover(int id)
{
  semctl(id, 1, IPC_RMID, NULL);
}

void sem_operacao_n(int id, int indice_do_semaforo, int operacao)
{
  int r;
  struct sembuf sb;
  sb.sem_num = indice_do_semaforo; /* Indice do semaforo no conjunto */
  sb.sem_op = operacao;
  sb.sem_flg = 0; /* flags */
  r=semop(id, &sb, 1 );
}

void sem_operacao(int id, int operacao)
{
  sem_operacao_n(id, 0, operacao); /*Indice 0 assumido*/
}
