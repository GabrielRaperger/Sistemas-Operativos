#include "header.h"

int sem_pedido, sem_resposta;
long pid;
long pid_f;
struct shmid_ds info_shm ;
int id_shm;

void tratar_sigtstp()
{
  if(pid==getpid()){
  char cmd[20];
  printf("\nEscreva o comamdo a executar: ");
  fgets(cmd, sizeof(cmd), stdin);
  system(cmd);
  printf("\nOK!\n");}
}

void tratar_usr()
{
  printf("\nO servidor encerrou\n");
  printf("\nVou encerrar tambem\n");
  sem_remover(sem_pedido);
  sem_remover(sem_resposta);
  kill(pid_f, SIGKILL);
  kill(pid, SIGKILL);
}

void encerrar()
{
  int r;
  if(pid == getpid())
  {
  r=shmctl(id_shm, IPC_STAT, &info_shm);
  // printf("[%d]", info_shm.shm_cpid);
  kill(info_shm.shm_cpid, SIGUSR1);
  printf("\nO Cliente %ld vai encerrar e informou o servidor\n", pid);
  //printf("Semaforos removidos\n");
  sem_remover(sem_pedido);
  sem_remover(sem_resposta);
  }
  kill(pid_f, SIGKILL);
  kill(pid, SIGKILL);
}

void main()
{
  long para;
  int r, inf;
  pid = getpid();
  int id_msg;
  struct mensagem msg;
  struct clientes *ptr;
  id_shm = shmget(chave_shm, 0, 0); // obter o id da memoria partilhada
  id_msg = msgget(chave_msg, 0); // obter o id da fila
  exit_on_error(id_shm, "O servidor que tentou contactar nao esta disponivel\n");
  exit_on_error(id_msg, "Erro ao tentar abrir a fila de mensagens\n");
  sem_pedido = sem_criar(IPC_PRIVATE, 0600, 1);
  sem_resposta = sem_criar(IPC_PRIVATE, 0600, 0);
  // fazer a associaçao do sinal
  signal(SIGTSTP, tratar_sigtstp);
  signal(SIGUSR1, tratar_usr);
  struct sigaction sa;
  sa.sa_handler=encerrar;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags=0;
  sigaction(SIGINT, &sa, NULL); /* trata SIGINT */
  // acoplamento com a memoria partilhada
  int id_sem;
  id_sem = sem_id(chave_sem);
  sem_operacao(id_sem, -1); // esperar
  ptr = (struct clientes *) shmat(id_shm, NULL, 0);
  for(int p=0;p<MAX_CLIENTES;p++)
  {
    if(!ptr->pid[p])
    {
	ptr->pid[p]=pid;
	break;
    }
  }
  shmdt(ptr);
  sem_operacao(id_sem, 1); // assinalar
  inf = shmctl(id_shm, IPC_STAT, &info_shm);
  kill(info_shm.shm_cpid, SIGUSR2);
  // sinal
  if(fork()) // pai
  {
    // escrever pedido
    // erro: o Ctrl+Z é o sinal SIGTSTP
    printf("Como exemplo, se quiser executar o comando ps -afH prima as teclas Ctrl+Z\n");
    printf("Insira os codigos dos produtos e prima ENTER:\n");
    // ler pedido: string ou inteiro? é necessario fazer a conversão
    // servidor
    // cliente
    while(1)
    {
//      wait();
      msg.para = 1;
      msg.de = pid;
      sem_operacao(sem_pedido, -1); // esperar
      fgets(msg.texto,sizeof(msg.texto),stdin);
      int valido=0;
      for(int l=0;l<strlen(msg.texto);l++)
      {
	if(msg.texto[l] >= '0' || msg.texto[l] <= '9')
	  valido=1;
        else
	{
	  valido=0;
	  break;
  	}
      }
      if(valido==1 && msg.de!=1)
      {
        r = msgsnd(id_msg, (struct msgbuf *) &msg, sizeof(msg)-sizeof(long), 0); // envia a mensagem
        exit_on_error(r, "Erro ao enviar o pedido\n");
        sem_operacao(sem_resposta, 1); // assinalar
      }
      else
	printf("Codigo invalido\n");
    }
  }
  else // filho
  {
    pid_f = getpid();
    // ler resposta do servidor
    para = pid; // vai receber as mensagens do tipo para que corresponde ao pid
    while(1)
    {
      sem_operacao(sem_resposta, -1); // esperar filho
      r = msgrcv(id_msg, (struct msgbuf *) &msg, sizeof(msg)-sizeof(long), para, 0); // recebe as msg
      //exit_on_error(r, "Erro ao obter a resposta do servidor\n");
      if(strlen(msg.texto)>0 && pid!=1)
      {
        printf("Cliente %ld. Resposta: %s\n", pid, msg.texto);
      }
      sem_operacao(sem_pedido, 1); // assinalar pai
    }
  }
}
