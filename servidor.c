#include "header.h"
int id_sem;

void criar_bd()
{
  char ans, name[100];
  FILE* file;
  int valido=1;
  float valor;
  struct Produto P[MAX_PRODUTOS];
  printf("Database nao existe. Deseja cria-la? ");
  ans = getchar();
  if(ans == 'S' || ans == 's')
  {
    file=fopen(BD, "wb");
    for(int i=0;i<MAX_PRODUTOS;i++)
    {
      printf("\nCodigo do produto: %d", i);
      printf("\nNome do produto: ");
      getchar();
      fgets(name, sizeof(name), stdin);
      if((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= '0' && name[0] <= '9')){
        valido=1;
	}
      else{
	valido=0; break;}
      printf("Preco do produto: ");
      scanf("%f", &valor);
      if(valor < 0){
	valido=0;
	break;}
      if(valido==1)
      {
	int n=strlen(name);
	if(name[n-1] == '\n')
	  name[n-1] = '\0';
	P[i].codigo=i;
	strcpy(P[i].nome, name);
	P[i].preco=valor;
	fprintf(file, "%s Preço: %.2f\n", P[i].nome, P[i].preco);
      }
    }
    fclose(file);
  }
  else
    kill(getpid(), SIGINT);
}

void cliente_iniciou(int sig, siginfo_t *siginfo, void *context)
{
  printf("\nCliente %ld iniciou\n", (long)siginfo->si_pid);
}

void cliente_encerrou(int sig, siginfo_t *siginfo, void *context)
{
  printf("\nCliente %ld terminou\n", (long)siginfo->si_pid);
}

void encerrar(int sinal)
{
  struct clientes *ptr;
  int id_msg, id_shm;
  id_msg = msgget(chave_msg, 0);
  id_shm = shmget(chave_shm, 0, 0);
  id_msg = msgctl(id_msg, IPC_RMID, NULL);
  exit_on_error(id_msg, "Erro ao remover a fila de mensagens\n");
  sem_operacao(id_sem, -1); // esperar
  //enviar sinais para os clientes
  //acoplamento
  ptr = (struct clientes*) shmat(id_shm, NULL, 0);
  for(int i=0;i<MAX_CLIENTES;i++)
  {
    if(ptr->pid[i]!=0)
      kill(ptr->pid[i], SIGUSR1);
  }
  //desacoplamento
  shmdt(ptr);
  sem_operacao(id_sem, 1); // assinalar
  //semctl(id_sem, 1, IPC_RMID, NULL);
  char cmd[20];
  strcpy(cmd, "ipcrm -S 1110");
  system(cmd);
  id_shm = shmctl(id_shm, IPC_RMID, NULL);
  exit_on_error(id_shm, "Erro ao remover a memoria partilhada\n");
  printf("Vou encerrar\n");
  kill(getpid(), SIGKILL);
}

void main()
{
  int id_shm;
  id_shm = shmget(chave_shm, tam_shm, IPC_CREAT | 0600);
  exit_on_error(id_shm, "Erro ao criar memoria partilhada\n");
  int id_sem;
  id_sem=sem_criar(chave_sem, 0600, 1);
  struct sigaction sa, out, in;
  sa.sa_handler=encerrar;
  out.sa_sigaction=cliente_encerrou;
  in.sa_sigaction=cliente_iniciou;
  sigemptyset (&sa.sa_mask);
  out.sa_flags=SA_SIGINFO;
  in.sa_flags=SA_SIGINFO;
  sa.sa_flags=0;
  sigaction(SIGUSR2, &in, NULL); /* trata SIGUSR2  */
  sigaction(SIGUSR1, &out, NULL); /* trata SIGUSR1  */
  sigaction(SIGINT, &sa, NULL); /* trata SIGINT */
  int r, pesquisa, valido=0;
  int para;
  FILE* file;
  char line[100];
  file = fopen(BD, "rb");
  if(file == NULL)
  {
    // criar bd
    criar_bd();
  }
  int id_msg; // filas de mensagens
  struct mensagem msg;
  struct clientes *ptr; // analisar
  id_msg = msgget(chave_msg, IPC_CREAT | 0600);
  exit_on_error(id_msg, "Erro ao criar a fila de mensagens\n");
  printf("Servidor Inciou...\n");
  while(1)
  {
    int linha=0;
    pesquisa =0;
    para = 1;
    r = msgrcv(id_msg, (struct msgbuf *) &msg, sizeof(msg)-sizeof(long), para, 0);
    //exit_on_error(r, "Erro ao receber o pedido\n");
    if(strlen(msg.texto)>0 && (msg.de != 1))
    printf("Cliente %ld solicitou o produto %s\n", msg.de, msg.texto);
    msg.para = msg.de;
    msg.de = 1;
    file = fopen(BD, "rb");
    pesquisa = atoi(msg.texto);
    if(pesquisa >= 0)
    {
      pesquisa++;
      while((!feof(file)) && (linha != pesquisa))
      {
        fgets(line, 100, file);
        linha++;
      }
      if(line)
      {
        strcpy(msg.texto, line);
      }
      if(pesquisa > linha)
      {
        strcpy(msg.texto, "Codigo inexistente\n");
      }
    }
    else
    {
      strcpy(msg.texto, "Aconteceu algum problema\n");
    }
    if(msg.para!=1)
      r = msgsnd(id_msg, (struct msgbuf *) &msg, sizeof(msg)-sizeof(long), 0);
    //exit_on_error(r, "Erro ao enviar resposta\n");
  }
}
