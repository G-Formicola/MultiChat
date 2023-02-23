#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>
#include<signal.h>
#include<errno.h>

#define SERVERPORT 23456
#define SERVERADDRESS "127.0.0.1"
#define MAXSLEEP 8 // Used by connect_retry
#define BUF_SIZE 1024

int server_socket_descriptor;

// Returns -1 if the client decides not to connect, a positive socket descriptor otherwise
int open_communication();
// Used by open_communication(), Returns the Socket Descriptor if connection gets approved, -1 if connection fails
int connect_retry(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen);
// Handler of signals
void signalHandler (int numSignal);
// Entrypoint of the thread which will take care of receiveing responses
void * receiver(void * args);


int main(int argc, char* argv[]){

  // Ignoring the SIGPIPE generated when writing on a socket which connection has crashed
  if(signal(SIGPIPE,signalHandler) == SIG_ERR ){
      perror("Signal error ");
      return (-2) ;
  }

  // Exiting on ctrl+c
  if(signal(SIGINT,signalHandler) == SIG_ERR ){
      perror("Signal error ");
      return (-3) ;
  }

  char send_buff[BUF_SIZE];
  pthread_t receiver_thread = 0;


  printf("\n---------- PROGETTO LABORATORIO DI SISTEMI OPERATIVI A.A. 22/23 ----------\n");
  printf("-                                                                          -\n");
  printf("- Sviluppato da :                                                          -\n");
  printf("- ~ Formicola Giorgio           N86/2220                                   -\n");
  printf("- ~ Daniele Caiazzo             N86/2780                                   -\n");
  printf("- ~ Francesco Dell'Oglio        N86/2666                                   -\n");
  printf("-                                                                          -\n");




  // If opening goes well
  if((server_socket_descriptor = open_communication())>0){ // CONNESSIONE AVVENUTA.

    // Launching the thread responsable of receiving messages
    pthread_create(&receiver_thread,NULL,&receiver,NULL);

    printf("\n***** BENVENUTI IN MULTICHAT ! *****\n\n");
    printf("--- Digitare //command:<HELP> per conoscere i comandi disponibili ---\n\n");
    printf("\\/\n");
    // If user prompts Ctrl-D (EOF) or the server stops to respond the client will exit;
    //fgets(send_buff, BUF_SIZE-1, stdin)!=NULL
    int message_lenght;
    while ((message_lenght = read(0, send_buff, BUF_SIZE))){
         send_buff[message_lenght]='\0';
         int n_written_char;
         if ( (n_written_char = write(server_socket_descriptor,send_buff,message_lenght )) == message_lenght )
           printf("-sent\n\n");
         else
           printf("-error sending the message : \n-only %d characters sent, try again. \n\n",n_written_char);
    }

  }

  //exithandler:
  printf("Closing the main thread of the client ...\n\n");
  close(server_socket_descriptor);
  if(receiver_thread!=0){
    if ((pthread_join(receiver_thread,NULL))!=0)
      printf("\nError pthread_join : %s\n",strerror(errno));
  }
  printf("*** GOODBYE ! ***\n" );
  return 0;

}

// Returns -1 if the client decides not to connect, a positive socket descriptor otherwise
int open_communication(){

  int socket_descriptor ;

  struct sockaddr_in server_address ;

  memset(&server_address, '\0', sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVERPORT);
  inet_aton(SERVERADDRESS, &server_address.sin_addr);

  socklen_t server_addr_len = sizeof(server_address);

  int user_choice;

  char inputString[128];

  do {

    printf("\nConnessione al server in corso ... attendere prego ... \n");
    socket_descriptor = connect_retry(PF_INET, SOCK_STREAM, 0, (struct sockaddr*)&server_address, server_addr_len);

    if (socket_descriptor < 0){

      printf("\nConnessione al server non riuscita, riprovare ? \n");
      printf("1 = Riprova a connetterti \n");
      printf("0 = Esci \n");
      printf("----- \n");

      // Asks for a choice until the client doesn't insert an available one
      while(1) {
        printf("Scelta : ");

        fgets(inputString, 128, stdin);
        inputString[strlen(inputString)-1]='\0'; //remove '\n'

        char* c = NULL;

        user_choice = (int) strtol(inputString, &c, 10);

        if ( c == NULL || *c != '\0' || user_choice<0 || user_choice>1)
        {
          printf("Attenzione : scelta non consentita, riprovare : %s\n",inputString);
        }
        else
        {
          printf("\n");
          break;
        }
      }

    }else{
      printf("Connessione avvenuta con successo !\n\n");
    }

  } while( socket_descriptor < 0 && user_choice!=0 );

  return socket_descriptor;
}
// Returns the Socket Descriptor if connection gets approved, -1 if connection fails. Used by open_communication()
int connect_retry(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen){
    int numsec, fd;
    /*
     * Try to connect with exponential backoff.
     */
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        printf("... \n");
        if ((fd = socket(domain, type, protocol)) < 0)
            return(-1);
        if (connect(fd, addr, alen) == 0) {
            /*
             * Connection accepted.
             */
            return(fd);
        }
        close(fd);
        /*
         * Delay before trying again.
         */
        if (numsec <= MAXSLEEP/2)
            sleep(numsec);
    }

    return(-1);

}
// Handler of signals
void signalHandler (int numSignal){
  if (numSignal == SIGINT){
    printf("\nClosing the client ...\n\n");
    close(server_socket_descriptor);
    printf("*** GOODBYE ! ***\n\n" );
    exit(0);
  }
  if (numSignal == SIGPIPE){
    printf("Error trying to send data...\nPlease try again and if error persists, restart the client ...\n\n");
  }
}
// Entrypoint of the thread which will take care of receiveing responses, and printing them on the standard output
void * receiver(void * args){

	char recv_buff[BUF_SIZE];
	int n_read_char, dim_recv_messagge = 0;

  while ( (n_read_char = read(server_socket_descriptor, recv_buff+dim_recv_messagge, BUF_SIZE-dim_recv_messagge-2)) != 0){
     if(n_read_char < 0){ // When we close the socket from the main thread, the read will return -1 and we go here
       goto exit_thread;
   	 }else{
       dim_recv_messagge += n_read_char;
       // If the space in the buffer has finished, or newline appears in the string then we can print the response from the server
       if(dim_recv_messagge >= BUF_SIZE-2 || recv_buff[dim_recv_messagge-1] == '\n' ){

         if(dim_recv_messagge < BUF_SIZE-2)
           recv_buff[dim_recv_messagge]='\0';
         else // buffer is full
           recv_buff[BUF_SIZE-1]='\0';

         if ( strcmp(recv_buff,"\nLOGIN\nUsername :\n") == 0 || strcmp(recv_buff,"\nPassword :\n") == 0 || strcmp(recv_buff,"\nSIGNIN - REGISTRAZIONE\n*Inserire i dati utente*\nUsername :\n") == 0 || strcmp(recv_buff,"\n*** INSERIRE IL NOME DELLA STANZA (max 32 caratteri ***)\nRoom name :\n") == 0 ){
            printf("%s",recv_buff);
         }else{
            printf("\n-received :\n\\/ %s \n\n\\/\n ",recv_buff);
         }

       }
     }
      dim_recv_messagge = 0;

  }

   if (n_read_char==0){
      printf("\nConnection closed by the Server\n\n");
   }

  exit_thread:
  printf("Closing the client thread for receiving ...\n\n");
  close(server_socket_descriptor);
  // If the connection has been closed by the server
  if (n_read_char==0){
    printf("*** GOODBYE ! ***\n\n" );
    _exit(0);
  }else{ // If the connection has been closed by the client
    pthread_exit(0);
  }
}
