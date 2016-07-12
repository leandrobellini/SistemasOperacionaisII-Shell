#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_SIZE 1024
#define MAX_JOB_SHELL 10


/* Variáveis globais */
int cont;
int numPipe;
pid_t pid;
char caminho[100];

typedef struct{
    pid_t pid;
    char *entrada;
    char *saida;
    char *arquivoEntrada;
    char *arquivoSaida;
    char **args;
    int qtdeArgs;
    int estado;
    int bg;
} job;

job *jobs[MAX_JOB_SHELL];
job **jobsPipe;


/* Função para adicionar um Job a uma posição vazia do vetor de jobs */
int adicionaJob(pid_t pid, job *job){
    int i=0;
    job->pid = pid;
    job->estado = 1;
    for(i=0; jobs[i] != NULL && i<MAX_JOB_SHELL; i++);
    if(i<MAX_JOB_SHELL){
        jobs[i] = job;
        return 1;
    }
    return 0;
}


/* Função para remover os Jobs ja concluidos do vetor de jobs */
void removeJob(){
    int i;
    for(i=0; i < MAX_JOB_SHELL; i++){
        if(jobs[i] != NULL){
            if(waitpid(jobs[i]->pid, NULL,WNOHANG)){
                jobs[i] = NULL;
            }
        }
    }
}

/* Função para executar o cd */
int cd (char *comando) {
    int ret;
    ret = chdir(comando) + 1;                    //chdir retorna 0 se sucesso e -1 se falha
    return ret;
}

/* Função para listar os jobs */
void listaJob(){
    int i;
    removeJob();
    for(i=0; i<MAX_JOB_SHELL; i++){
        if(jobs[i] != NULL){
            printf("[%d]   ", i);
            if(jobs[i]->estado == 0){
                if(jobs[i]->bg == 0){
                    printf("Parado    Forground      %s\n",jobs[i]->args[0]);
                }
                else{
                    printf("Parado    Background      %s\n",jobs[i]->args[0]);
                }
            }
            else{
                if(jobs[i]->bg == 0){
                    printf("Rodando    Forground      %s\n",jobs[i]->args[0]);
                }
                else{
                    printf("Rodando    Background      %s\n",jobs[i]->args[0]);
                }
            }
        }
    }
}


/* Função para executar o comando KILL em que ele finaliza um processo com tal id */
int mataJob(char *id){
    int pid;
    pid = atoi(id);
    int i;
    for(i=0; i < MAX_JOB_SHELL; i++){
        if(jobs[i] != NULL){
            if(jobs[i]->pid == pid){
                printf("Processo:[%d]  id:%d  Killed\n", i, jobs[i]->pid);
                kill (jobs[i]->pid,SIGKILL);
                jobs[i] = NULL;
                return 1;
            }
        }
    }
    return 0;
}


/* Função para trazer um dado processo da lista de jobs para Forground */
int forground(char *num){
    int auxNum;
    auxNum = atoi(num);
    // Verifica se o processo está realmente na lista de jobs
    if(jobs[auxNum] != NULL){
        printf("%s\n",jobs[auxNum]->args[0]);
        pid = jobs[auxNum]->pid;
        jobs[auxNum]->estado = 1;
        jobs[auxNum]->bg = 0;
        kill (jobs[auxNum]->pid,SIGCONT);
        waitpid(jobs[auxNum]->pid,NULL,WUNTRACED);
        return 1;
    }
    return 0;
}


/* Função para levar um dado processo da lista de jobs para Background */
int backgound(char *num){
    int auxNum;
    auxNum = atoi(num);
    if(jobs[auxNum] != NULL){
        jobs[auxNum]->estado = 1;
        jobs[auxNum]->bg = 1;
        kill (jobs[auxNum]->pid,SIGCONT);
        printf("[%d]  %s&\n",auxNum, jobs[auxNum]->args[0]);
        pid = 0;
        return 1;
    }
    return 0;
}


/* Funcao para implementacao do comando HELP. */
void help (void){
    char * args[3];
    int pid = fork ();
    if (pid <0){
        printf("Erro\n");
    }
    if ( pid > 0 ){
        wait(NULL);
    }
    else{
        args[0]="cat";
        args[1]="help";
        args[2]=NULL;
        execvp(args[0], args);
    }
}


/* Funcao para interpretar o comando HELP, EXIT, GB, FG, KILL, JOBS. */
int exec_builtin (char *command){
    if (!strcmp (command, "help")){
        help();
        return 1;
    }
    if (!strcmp (command, "exit")){
        cont = 0;
        return 1;
    }
    if (!strcmp (command, "cd")){
        if(jobsPipe[0]->args[1] == NULL)
        {
            cd("~");
        }
        else 
        {
            cd(jobsPipe[0]->args[1] );
        }
        return 1;
    }
    if (!strcmp (command, "jobs")){
        listaJob();
        return 1;
    }
    if (!strcmp (command, "kill")){
        if(jobsPipe[0]->args[1] != NULL){
            if(!mataJob(jobsPipe[0]->args[1])){
                printf("Processo não existe ou não é permitido essa ação para esse processo\n");
            }
            return 1;
        }
    }
    if (!strcmp (command, "fg")){
        if(jobsPipe[0]->args[1] != NULL){
            if(!forground(jobsPipe[0]->args[1])){
                printf("Processo não encontrado\n");
            }
        }
        else{
            printf("Insira um processo\n");
        }
        return 1;
    }
    if (!strcmp (command, "bg")){
        if(jobsPipe[0]->args[1] != NULL){
            if(!backgound(jobsPipe[0]->args[1])){
                printf("Processo não encontrado\n");
            }
        }
        else{
            printf("Insira um processo\n");
        }
        return 1;
    }
    return 0;
}


/* Funcao para mudar a saída e entrada padrao do programa a ser chamado no shell. */
void mudaEntradaSaida(char *entrada, char *arquivoEntrada, char *saida, char *arquivoSaida){
    // Muda a entrada padrao para um arquivo especificado
    if(entrada != NULL && (strcmp(entrada, "<") == 0)){
        int fd;
        fflush(stdin);
        close (0);
        fd = open (arquivoEntrada, O_EXCL | O_RDONLY, S_IRUSR | S_IWUSR);
        if (fd<0){
            perror("Erro.\n");
            exit(1);
        }
    }
    // Muda a saida padrao para um arquivo especificado e dependendo do tipo de saida o arquivo tem um parametro diferente.
    if(saida != NULL){
        int fd;
        fflush(stdout);
        close (1);
        if(strcmp(saida, ">") == 0){
            // Parametro para reescrever.
            fd = open (arquivoSaida, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
        }
        else{
            // Parametro para adicionar ao final do arquivo.
            fd = open (arquivoSaida, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR);
        }
        if (fd<0){
            perror("Erro.\n");
            exit(1);
        }
    }
}


/* Função para alocar em memória um job e inicializa-lo com seus devidos valores */
job* alocaJob(){
    job *pJob = (job*) malloc(sizeof(job));
    if(pJob != NULL){
        pJob->pid = 0;
        pJob->entrada = NULL;
        pJob->saida = NULL;
        pJob->arquivoEntrada = NULL;
        pJob->arquivoSaida = NULL;
        pJob->args = NULL;
        pJob->estado = -1;
        pJob->qtdeArgs = 0;
        pJob->bg = 0;
        return pJob;
    }
    return NULL;
}


/* Função para desalocar o vetor de jobs auxiliar vindo do pipe */
void desalocaJob(){
    free(jobsPipe);
}


/* Função para separar uma string do job em um vetor de strings onde na posição 0 é o comando e as demais são os parametros.
Atribui também as flags para controle de redirecinamento de entrada e saida */
void separaArg(char *comando, job *job){
    int i, j, count = 0;
    char *arg;
    for(i=0; i<strlen(comando); i++){
        if(comando[i] == ' ' && i!=(strlen(comando)-1) && comando[i-1] != ' ' && comando[i+1] != ' '){
            count++;
        }
        if(comando[i] == '&'){
            job->bg = 1;
            comando[i] = '\0';
        }
    }

    job->qtdeArgs = count +1;
    job->args = (char **)malloc(sizeof(char *) * (count + 2));
    j = 0;
    arg = strtok(comando, " ");
    while(arg != NULL){
        job->args[j] = (char *)malloc(sizeof(char) * strlen(arg));
        strcpy(job->args[j++], arg);
        arg  = strtok(NULL, " ");
    }
    job->args[j] = NULL;
    for(i=0; i<j; i++){
        if(strcmp(job->args[i], "<") == 0){
            job->entrada = (char *) malloc(sizeof(char) * strlen(job->args[i]));
            strcpy(job->entrada, job->args[i++]);
            job->arquivoEntrada = (char *) malloc(sizeof(char) * strlen(job->args[i]));
            strcpy(job->arquivoEntrada, job->args[i]);
            job->args[i-1] = NULL;
        }
        else if((strcmp(job->args[i], ">") == 0) || (strcmp(job->args[i], ">>") == 0)){
            job->saida = (char *) malloc(sizeof(char) * strlen(job->args[i]));
            strcpy(job->saida, job->args[i++]);
            job->arquivoSaida = (char *) malloc(sizeof(char) * strlen(job->args[i]));
            strcpy(job->arquivoSaida, job->args[i]);
            job->args[i-1] = NULL;
        }
    }
}


/* Função para verificar se a linha de comando vinda do G2 possui pipe e se sim, quebra os comandos, e transforma em jobs e armazena
em um vetor auxiliar chamado jobsPipe */
int dividePipe(char *comando){
    char *aux;
    char *auxCmd;
    char **auxPipe;
    int i=0;
    int j;
    numPipe = 0;
    aux = strchr(comando, '|');
    while(aux != NULL){
        numPipe++;
        aux = strchr(aux+1, '|');
    }
    jobsPipe = (job **) malloc(sizeof(job *) * (numPipe+1));
    auxPipe = (char **) malloc(sizeof(char *) * (numPipe+1));
    if(numPipe > 0){
        auxCmd = strtok(comando, "|");
        while(auxCmd != NULL){
            auxPipe[i]= (char *)malloc(sizeof(char) * strlen(auxCmd));
            strcpy(auxPipe[i++], auxCmd);
            auxCmd  = strtok(NULL, "|");
        }
        for(j=0; j<i; j++){
            jobsPipe[j] = alocaJob();
            separaArg(auxPipe[j], jobsPipe[j]);
        }
    }
    else{
        jobsPipe[0] = alocaJob();
        separaArg(comando, jobsPipe[0]);
    }
    return 1;
}


/* Função para executar os comandos vindo do pipe */
void pipee(){
        int status2;
        int pid2;
        int ret;
        int j;
        int i;
        int pipes = 0;

        //criação tunelamento para comunicação dos processos
        int pipefd[2*numPipe+1];
        for(j=0;j<numPipe+1;j++){
            ret = pipe(pipefd + j*2);
            if(ret<0) printf("erro\n");
        }

        // Cria um processo dentro do outro e fecha as devidas entradas e saidas padrões e liga com o tunelamento
        for(i=0;i<numPipe+1;i++){
            pipes++;
            pid2 = fork();
            if(pid2==0){
                // Redireciona a entrada se não for o primeiro processo do pipe duplicando o tunel
                if(pipes>1){
                    dup2(pipefd[(pipes*2)-4], 0);
                }
                // Redireciona a saida se não for o ultimo processo do pipe duplicando o tunel
                if(numPipe+1!=pipes){
                    dup2(pipefd[((pipes*2)-1)],1);
                }
                // Fecha os túneis que já foram duplicados acima
                for(j=0;j<numPipe+1;j++){
                    close(pipefd[2*j]);
                    close(pipefd[(2*j)+1]);
                }
                setpgid(0,0);
                if(execvp(jobsPipe[i]->args[0], jobsPipe[i]->args) == -1){
                    printf ("%s: Comando não encontrado.\n", jobsPipe[i]->args[0]);
                    exit(EXIT_FAILURE);
                }

            }
        }

        // Fecha os túneis que já foram duplicados acima
        for(j=0;j<numPipe+1;j++){
                close(pipefd[2*j]);
                close(pipefd[(2*j)+1]);
        }
        // Aguarda a execução de cada processo
        for(j=0;j<numPipe+1;j++){
                wait(&status2);
        }
        // Finaliza os processos executados
        for(j=0;j<numPipe+1;j++){
                exit(1);
        }
}


/* Função HANDLER para interpretar o sinal de background CTRL+Z */
void sinalParada (int signal) {
	int i;
	// Procura um lugar vazio na lista de jobs e adiciona o novo processo e em seguida envia o sinal de parada
    for(i=0; i < MAX_JOB_SHELL; i++){
        if(jobs[i] != NULL){
            if(jobs[i]->pid == pid){
                jobs[i]->estado = 0;
                jobs[i]->bg = 1;
                printf("\n[%d]+     Parado       %s\n", i, jobs[i]->args[0]);
                break;
            }
        }
    }
    kill(jobs[i]->pid,SIGTSTP);
	pid = 0;
	printf("\n");
}


/* Função HANDLER para interpretar o sinal de finalizar CTRL+C */
void sinalFinaliza (int signal) {
    if(pid != 0){
        kill(pid,SIGINT);
    }
	pid = 0;
	printf("\n");
}




int main(int argc, char **argv){
    char command[MAX_SIZE];
    int i;
    cont = 1;

    // Inicializa o vetor de jobs do G2
    for(i=0; i<MAX_JOB_SHELL; i++){
        jobs[i] = NULL;
    }

    // Sinal CTRL+Z
    signal(SIGTSTP, sinalParada);
    // Sinal CTRL+C
    signal(SIGINT, sinalFinaliza);

    // G2 roda infintamente, ate ser interpretado o comando EXIT.
    while (cont){
        printf ("%s G2: ", getcwd(caminho,100));
        // Le o comando passado pelo usuario.
        fgets(command,1024,stdin);
        if(command[0] == '\n'){
            continue;
        }
        command[strlen(command)-1] = '\0';
        dividePipe(command);
        if (!exec_builtin(jobsPipe[0]->args[0])){
            pid = fork();
            if(pid < 0){
                fatal();
            }
            /* O pai executa essa parte, adicionando o processo na lista de jobs e verificando se o processo deve ser executando em background
            ou forground */
            if(pid > 0){
                // Background
                if(jobsPipe[0]->bg == 1){
                    adicionaJob(pid, jobsPipe[0]);
                    pid=0;
                }
                // Forground
                else{
                    adicionaJob(pid, jobsPipe[0]);
                    waitpid(pid,NULL,WUNTRACED | WUNTRACED);
                }
            }
            /* O filho executa essa parte */
            else{
                setpgid(0,0);
                // Executa os comandos do pipe se tiver o pipe
                if(numPipe > 0){
                    pipee();
                }
                // Executa um comando simples com seus parametros e se houver redirecionamento de entrada e saida
                else{
                    mudaEntradaSaida(jobsPipe[0]->entrada, jobsPipe[0]->arquivoEntrada, jobsPipe[0]->saida, jobsPipe[0]->arquivoSaida);
                    if(execvp(jobsPipe[0]->args[0], jobsPipe[0]->args) == -1){
                        printf("%s: Comando nao encontrado\n", jobsPipe[0]->args[0]);
                    }
                    exit(0);
                }
            }
            // Desaloca o vetor auxiliar de jobs
            desalocaJob();
        }
    }
    return EXIT_SUCCESS;
}

