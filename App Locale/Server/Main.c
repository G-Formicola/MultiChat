#include"Libs/Structures/Data_Structures.h"
#include"Libs/DB/Database_Utils.h"

#define MYPORT 23456

// SERVER FUNCTIONS - A series of functions which use system-calls like "socket" "setsockopt" "bind" & "listen" for network programming

/*
 * Initializes socket, binds the special address INADDR_ANY to the socket, and put the socket in a listen state with
 * backlog = qlen. Returns a socket descriptor or -1 in case there will be any error.
*/
int initServerSocket(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
// Handler of signals
void signalHandler (int numSignal);

//  THREAD FUNCTIONS - A series of functions which use system-calls of the pthread family for multithreading programming

/*
 * Returns -1 or 0 for unknown or extraneous requests, a positive number which will indicates the type of request otherwise.
 * Function used do decode which request the user is asking for.
*/
int parse_client_request(const char* request_buffer);
// Takes info of one client as input and starts a new detached thread using "manage_a_single_client" as entrypoint
void launch_manage_a_single_client_thread(client_info* client);
// Entrypoint of the thread that will manage one client at time
void *manage_a_single_client(void *arg);
// Entrypoint of the thread that will manage one chatroom at time
void *manage_a_single_room(void *arg);
// Entrypoint of the thread that will manage an access request to a room
void *manage_access_to_a_room(void *arg);

//Global list of available rooms
linkedListRooms* rooms_list ;

// Main Entrypoint
int main(int argc, char* argv[]){

  // Impostiamo maschere segnali
  // Ignoring the SIGPIPE generated when writing on a socket which connection has crashed
  if(signal(SIGPIPE,signalHandler) == SIG_ERR ){
      perror("Signal error, restart the server.");
      return (-2) ;
  }

  // Exiting on ctrl-c
  if(signal(SIGINT,signalHandler) == SIG_ERR ){
      perror("Signal error, restart the server.");
      return (-3) ;
  }

  // Allocating the global list of available rooms checking that the allocation was fine
  if ( ( rooms_list = createANewLinkedListRooms() ) == NULL ){
    printf("Error allocating rooms list ! Restart the server ...\n");
    return (-4) ;
  }

  // Preparing the server address
  int server_socket ;
  struct sockaddr_in server_address ;
  memset(&server_address, '0', sizeof(server_address));
  server_address.sin_family = AF_INET ;
  server_address.sin_port = htons(MYPORT);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  // Initializes the socket with system calls socket, bind, listen
  while( (server_socket = initServerSocket(SOCK_STREAM,(struct sockaddr*)&server_address,sizeof(server_address),10)) < 0 ){
    printf("Error during init. of the server socket ... \n");
    printf("Wait for another try or press Ctrl-C to terminate ... \n");
    sleep(3);
  }

  // Needed for printing the IP address of a client in a human readable format
  const char* address_dot_format ;
  char buffer_address_dot_format[INET_ADDRSTRLEN];

  // Parameters for the accept
  int client_socket;
  struct sockaddr_in client_address ;
  socklen_t client_addr_size = sizeof(client_address);

  // Parameters for launching threads
  client_info* client;

  // Server main cycle
  while(1){

    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_addr_size);

    if(client_socket>0){

      // LOGGING NEW CONNECTIONS
      address_dot_format = inet_ntop(AF_INET, &client_address.sin_addr, buffer_address_dot_format, INET_ADDRSTRLEN);
      printf("\n-NEW CLIENT CONNECTED :\nSocket Descriptor : %d\nIP ADDRESS : %s\n",client_socket,address_dot_format);

      // Preparing the struct to pass for every new "manage_a_single_client" thread
      client = (client_info *)malloc(sizeof(client_info));
      strcpy(client->IP_address, address_dot_format) ;
      client->client_sd = client_socket ;
      client->authenticated = false ;
      memset(client->username, '\0', sizeof(client->username));



      launch_manage_a_single_client_thread(client);
    }

  }

  return 0;
}

// Initializes socket, binds the special address INADDR_ANY to the socket, and put the socket in a listen state with backlog = qlen
// Returns a socket descriptor or -1 in the case there will be any error
int initServerSocket(int type, const struct sockaddr *addr, socklen_t alen, int qlen) {
           int fd ;
           int reuse = 1;
           if ((fd = socket(addr->sa_family, type, 0)) < 0){
               printf("Error calling socket() \n");
               return(-1);
           }
           // It allows to reuse the same ip_address in case we restart the Server after some seconds of shutting down
           if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int)) < 0){
               printf("Error calling setsockopt() \n");
               goto errout;
           }

           /* Commentate perchè su MAC non compila
           int flags = 10;
           if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void *)&flags, sizeof(flags))){
               printf("ERROR: setsocketopt(), SO_KEEPIDLE");
               goto errout;

           }

           flags = 5;
           if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&flags, sizeof(flags))){
               printf("ERROR: setsocketopt(), SO_KEEPCNT");
               goto errout;

           }

           flags = 5;
           if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&flags, sizeof(flags))){
               printf("ERROR: setsocketopt(), SO_KEEPINTVL");
               goto errout;
           }*/


           if (bind(fd, addr, alen) < 0){
               printf("Error calling bind() \n");
               goto errout;
           }

           if (type == SOCK_STREAM || type == SOCK_SEQPACKET){
              if (listen(fd, qlen) < 0){
                   printf("Error calling listen() \n");
                   goto errout;
              }
           }
           return(fd);
    errout:
    close(fd);
    return(-1);
}

// Handler of signals SIGPIPE when writing on a closed socket, and SIGINT for closing the server
void signalHandler (int numSignal){
  if (numSignal == SIGINT){
    printf("\nClosing the server ...\n\n");
    destroy_list_rooms(rooms_list);
    exit(0);
  }
  if (numSignal == SIGPIPE){
    printf("Error trying to send response to the client...\n\n");
  }
}

// Returns -1 for unknown or extraneous requests, a positive number which will indicates the type of request otherwise
int parse_client_request(const char* request_buffer){

  size_t less_index, major_index ;
  char *find_less; // Less is intended to be the char '<'
  char *find_major; // Major instead '>'

  find_less = strchr(request_buffer,'<');
  find_major = strchr(request_buffer,'>');
  if ( find_less != NULL && find_major != NULL ){

    less_index = (size_t)(find_less - request_buffer);
    major_index = (size_t)(find_major - request_buffer);

    // If after the '>' there is somethig else, then wrong syntax
    if (request_buffer[major_index+1]!='\n' && request_buffer[major_index+1]!='\0')
      goto invalidSyntax ;

    // If the request doesn't start with //command: or //command:JOIN , then wrong syntax
     if (strncmp(request_buffer,"//command:",less_index)!=0){
              goto invalidSyntax;
     }else{

      if (strncmp( find_less,"<HELP>", major_index-less_index+1 )==0 ){
        return 1;
      }else if( strncmp(find_less,"<LOGIN>", major_index-less_index+1 )==0 ){
        return 2;
      }else if (strncmp( find_less,"<SIGNIN>", major_index-less_index+1 )==0 ){
        return 3;
      }else if (strncmp( find_less,"<LOGOUT>", major_index-less_index+1 )==0 ){
        return 4;
      }else if (strncmp( find_less,"<CREATE ROOM>", major_index-less_index+1 )==0 ){
        return 5;
      }else if (strncmp( find_less,"<ROOMS>", major_index-less_index+1 )==0 ){
        return 6;
      }else if (strncmp( find_less,"<JOIN ROOM>", major_index-less_index+1 )==0 ){
        return 7;
      }else if (strncmp( find_less,"<EXIT ROOM>", major_index-less_index+1 )==0 ){
        return 8;
      } else{
        return 0;
      }

    }
  }else{
    goto invalidSyntax;
  }

  invalidSyntax:
  return -1 ;
}

// Takes info of one client as input and starts a new detached thread using "manage_a_single_client" as entrypoint
void launch_manage_a_single_client_thread(client_info* client){
  // Parameters for launching threads
  pthread_t tinfo;
  int err = 0;
  pthread_attr_t  t_attr;
  char threadErrorMessage[128] = "Error from the server accepting connection, please restart the client !\n\0";

  // Thread attribute creation
  err = pthread_attr_init(&t_attr);
  if (err != 0){ //error
        printf("Error pthread_attr_init : %s\n", strerror(err));
        write(client->client_sd,threadErrorMessage,128);
        free(client);
  }else{
    // Thread attribute setting to "detached"
    if ( (err=pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED))!=0 ){ // error
          printf("Error pthread_attr_setdetachstate : %s\n", strerror(err));
          write(client->client_sd,threadErrorMessage,128);
          free(client);
          pthread_attr_destroy(&t_attr);
    }else{
      // Threads creation. They need to be detached so performance won't deteriorate during time cause of zombies threads after users disconnections
      if ( (err=pthread_create(&tinfo, &t_attr, manage_a_single_client, (void*)client) )!=0 ) { // error
          printf("Error calling pthread_create : %s\n", strerror(err));
          write(client->client_sd,threadErrorMessage,128);
          free(client);
      }
      pthread_attr_destroy(&t_attr);
    }
  }
}

// Entrypoint of the thread that will manage one client at time
void *manage_a_single_client(void *arg) {


  client_info* client = (client_info*)arg;
	char recv_buff[BUF_SIZE];
  char send_buff[BUF_SIZE];
	int n_read_char, dim_recv_messagge = 0;

  /*
  int flags = 1;
  if (setsockopt(client->client_sd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags))){
    goto gone_client;
  }*/

  // Main cicle serving the client
	while (1){

    n_read_char = read(client->client_sd, recv_buff+dim_recv_messagge, BUF_SIZE-dim_recv_messagge-1);

    if(n_read_char < 0){
  		printf("\n Error receiveing message from the client \n");
  	}else if(n_read_char > 0){
      dim_recv_messagge += n_read_char;
      // If the space in the buffer has finished, or newline appears in the string then we can try to process the request
      if(dim_recv_messagge >= BUF_SIZE-1 || recv_buff[dim_recv_messagge-1] == '\n'){

        if(dim_recv_messagge < BUF_SIZE-1)
          recv_buff[dim_recv_messagge]='\0';
        else // buffer is full and we truncate the read string
          recv_buff[BUF_SIZE-1]='\0';

        // LOGGING A NEW REQUEST
        printf("\n-NEW REQUEST FROM CLIENT :\nNickname : %s\nSocket Descriptor : %d\nIP ADDRESS : %s\nRequest : %s",client->username,client->client_sd,client->IP_address,recv_buff);

        int request_type = parse_client_request(recv_buff);
        if (request_type<0){ // Invalid syntax
          if (request_type==-1){
            sprintf(send_buff, "\nThe request can't be executed by the server because of the wrong syntax !\nExpected : //command:<...> OR //command:JOIN<room name>\n");
            printf("The request can't be executed by the server because of the wrong syntax !\n");
          }
          /*if (request_type==-2){
            sprintf(send_buff, "\nThe request can't be executed by the server because there's no room with such name\n");
            printf("The request can't be executed by the server because there's no room with such name\n");
          }*/
          write(client->client_sd,send_buff,strlen(send_buff));
        } else if ( request_type == 0 ){ // Syntax is right but the command has not been found
          sprintf(send_buff, "\nThe request can't be executed by the server ! No command found !\n");
          write(client->client_sd,send_buff,strlen(send_buff));
          printf("The request can't be executed by the server ! No command found !\n");
        } else if (request_type == 1){ // command:<HELP> Help request, returns command list

          if (client->authenticated == false){
            sprintf(send_buff, "--- LISTA DEI COMANDI DISPONIBILI ---\n* Effettua il login                           : //command:<LOGIN> \n* Effettua la registrazione                   : //command:<SIGNIN> \n* Terminare il programma in esecuzione        : Ctrl+D or Ctrl-C \n\n");
            write(client->client_sd,send_buff,strlen(send_buff));
          }else{
            sprintf(send_buff, "--- LISTA DEI COMANDI DISPONIBILI ---\n* Crea una nuova stanza per chattare                    : //command:<CREATE ROOM> \n* Visualizza le stanze esistenti create da altri utenti : //command:<ROOMS> \n* Entra in una delle stanze disponibili                 : //command:<JOIN ROOM> \n* Esci                                                  : //command:<LOGOUT> \n\n* Terminare immediatamente il programma in esecuzione   : Ctrl+D or Ctrl-C \n\n");
            write(client->client_sd,send_buff,strlen(send_buff));
          }
        } else if (request_type == 2){ // if command:<LOGIN> check credentials into the database

          if (client->authenticated == false){
                char username[31], pswd[31];
                sprintf(send_buff, "\nLOGIN\nUsername :\n");
                write(client->client_sd,send_buff,strlen(send_buff));

                memset(username, '\0', 31);
                if ( (read(client->client_sd, username, 31)) > 30 )
                  username[30]='\0';
                else
                  username[strlen(username)-1]='\0';



                sprintf(send_buff, "\nPassword :\n");
                write(client->client_sd,send_buff,strlen(send_buff));

                memset(pswd, '\0', 31);
                if ( (read(client->client_sd, pswd, 31)) > 30 )
                  pswd[30]='\0';
                else
                  pswd[strlen(pswd)-1]='\0';



                if ( (client->authenticated = authenticate_user(username, pswd)) ){
                  strcpy(client->username, username);
                  sprintf(send_buff, "\nAUTENTICAZIONE AVVENUTA CON SUCCESSO !\n");
                }else{
                  sprintf(send_buff, "\nAUTENTICAZIONE FALLITA, RIPROVARE !\n");
                }
          }else{
            sprintf(send_buff, "\nATTENZIONE UTENTE GIA AUTENTICATO !\n");
          }

          write(client->client_sd,send_buff,strlen(send_buff));

        } else if (request_type == 3){ // if command:<SIGNIN> insert credentials into the database

          if (client->authenticated == false){
                char username[31], pswd[31];

                sprintf(send_buff, "\nSIGNIN - REGISTRAZIONE\n*Inserire i dati utente*\nUsername :\n");
                write(client->client_sd,send_buff,strlen(send_buff));

                memset(username, '\0', 31);
                if ( (read(client->client_sd, username, 31)) > 30 )
                  username[30]='\0';
                else
                  username[strlen(username)-1]='\0';

                sprintf(send_buff, "\nPassword :\n");
                write(client->client_sd,send_buff,strlen(send_buff));

                memset(pswd, '\0', 31);
                if ( (read(client->client_sd, pswd, 31)) > 30 )
                  pswd[30]='\0';
                else
                  pswd[strlen(pswd)-1]='\0';


                if ( register_user(username, pswd) ){
                  sprintf(send_buff, "\nREGISTRAZIONE AVVENUTA CON SUCCESSO !\n");
                }else{
                  sprintf(send_buff, "\nREGISTRAZIONE FALLITA, RIPROVARE !\n");
                }
          }else{
            sprintf(send_buff, "\nATTENZIONE UTENTE GIA AUTENTICATO ! DISCONNETTERSI PER POTER REGISTRARE UN NUOVO ACCOUNT !\n");
          }

          write(client->client_sd,send_buff,strlen(send_buff));
        } else if (request_type == 4){ // if command:<LOGOUT> set authenticated state to false

          if (client->authenticated == true){
                client->authenticated = false;
                sprintf(send_buff, "\nLOGOUT AVVENUTO CON SUCCESSO !\n");
          }else{
                sprintf(send_buff, "\nATTENZIONE, EFFETTUARE PRIMA L'ACCESSO !\n");
          }
          write(client->client_sd,send_buff,strlen(send_buff));

        } else if (request_type == 5){ // if command:<CREATE ROOM> creates a joinable room

          if (client->authenticated == true){

            char room_name[32];
            memset(room_name, '\0', 32);

            sprintf(send_buff, "\n*** INSERIRE IL NOME DELLA STANZA (max 32 caratteri ***)\nRoom name :\n");
            write(client->client_sd,send_buff,strlen(send_buff));

            if ( (read(client->client_sd, room_name, 32)) > 31 )
              room_name[31]='\0';
            else
              room_name[strlen(room_name)-1]='\0';

            room_info* new_room = NULL;

            if ( ( new_room=(room_info*)malloc(sizeof(room_info)) ) != NULL ){

              strcpy(new_room->room_name,room_name);
              new_room->owner_sd = client->client_sd ;
              new_room->isAdminBusy = false ;
              new_room->destroy_flag = false ;
              new_room->adminMessagesQueue = createANewMessagesQueue();
              pthread_mutex_init(&new_room->acceptance_semaphore,NULL) ;
              new_room->connected_clients = createANewLinkedListUsers(); // should be checked for null ptr
              insert_user(client,new_room->connected_clients);

              if ( ( insert_room(new_room,rooms_list) == true ) ){
                // lancia thread conversazione con new_room come argomento
                // Parameters for launching threads
                pthread_t tinfo;
                int err = 0;

                // Threads creation. They need to be detached so performance won't deteriorate during time cause of zombies threads after users disconnections
                if ( (err=pthread_create(&tinfo, NULL, manage_a_single_room, (void*)new_room) )!=0 ) {
                    printf("Error calling pthread_create with manage_a_single_room : %s\n", strerror(err));
                    // Rimuovere dalla lista globale e deallocare new_room
                    remove_room(new_room, rooms_list);
                    // Deallocare new_room
                    destroy_list_users(new_room->connected_clients);
                    new_room->connected_clients = NULL ;
                    destroy_queue(new_room->adminMessagesQueue);
                    new_room->adminMessagesQueue = NULL ;
                    pthread_mutex_destroy(&new_room->acceptance_semaphore);
                    free(new_room);
                    // Comunica all'utente
                    sprintf(send_buff, "\nATTENZIONE, ERRORE DURANTE LA CREAZIONE DELLA STANZA !\n RIPETERE IL PROCESSO DA CAPO !\n");
                    write(client->client_sd,send_buff,strlen(send_buff));
                }else{
                  sprintf(send_buff, "\nCREAZIONE ANDATA A BUON FINE ! Nome stanza creata : \"%s\"\n",room_name);
                  write(client->client_sd,send_buff,strlen(send_buff));
                  return 0; // exit this thread
                }
              }else{
                // Deallocare new_room
                destroy_list_users(new_room->connected_clients);
                new_room->connected_clients = NULL ;
                destroy_queue(new_room->adminMessagesQueue);
                new_room->adminMessagesQueue = NULL ;
                pthread_mutex_destroy(&new_room->acceptance_semaphore);
                free(new_room);
                // Comunica all'utente
                sprintf(send_buff, "\nATTENZIONE, ERRORE DURANTE LA CREAZIONE DELLA STANZA !\n RIPETERE IL PROCESSO DA CAPO !\n");
                write(client->client_sd,send_buff,strlen(send_buff));
              }



            }else{
              sprintf(send_buff, "\nATTENZIONE, ERRORE DURANTE LA CREAZIONE DELLA STANZA !\n RIPETERE IL PROCESSO DA CAPO !\n");
              write(client->client_sd,send_buff,strlen(send_buff));
            }


          }else{
            sprintf(send_buff, "\nATTENZIONE, EFFETTUARE PRIMA L'ACCESSO !\n");
            write(client->client_sd,send_buff,strlen(send_buff));
          }

        } else if (request_type == 6){ // if command:<ROOMS> returns the names of available rooms

          if (client->authenticated == true){
            if( sizeOfTheListRooms(rooms_list) > 0 ){
              char appendstring[128];
              sprintf(send_buff, "\n*** STANZE ATTIVE ***\n");

              linkedListRoomsNode* iterator ;
              pthread_mutex_lock(&rooms_list->semaphore);
              iterator = rooms_list->head ;
              do {
                sprintf(appendstring, "-> \"%s\" <-\n",iterator->data->room_name);
                sprintf(send_buff, "%s%s",send_buff,appendstring);
                iterator=iterator->next;
              } while(iterator!=NULL);
              pthread_mutex_unlock(&rooms_list->semaphore);

              write(client->client_sd,send_buff,strlen(send_buff));

            }else{
              sprintf(send_buff, "\n*** SPIACENTI MA NON CI SONO STANZE ATTIVE AL MOMENTO A CUI UNIRSI ! ***\n");
              write(client->client_sd,send_buff,strlen(send_buff));
            }
          }else{
            sprintf(send_buff, "\nATTENZIONE, EFFETTUARE PRIMA L'ACCESSO !\n");
            write(client->client_sd,send_buff,strlen(send_buff));
          }

        } else if (request_type == 7){ // if command:<JOIN ROOM>

          if (client->authenticated == true){
                char room_name[32];
                memset(room_name, '\0', 32);

                sprintf(send_buff, "\n*** INSERIRE IL NOME DELLA STANZA (max 32 caratteri ***)\nRoom name :\n");
                write(client->client_sd,send_buff,strlen(send_buff));

                if ( (read(client->client_sd, room_name, 32)) > 31 )
                  room_name[31]='\0';
                else
                  room_name[strlen(room_name)-1]='\0';

                room_info* choosen_room = accessByName(room_name,rooms_list); // Ricerco nella lista globale delle stanze la stanza con nome <room name>
                if (choosen_room!=NULL){ // LANCIA UN THREAD gestisci_richiesta CON INPUT client + room_info (manage_access_arg)
                  // Parameters for launching threads
                  pthread_t tinfo;
                  int err = 0;
                  manage_access_arg* access_thread_arg = (manage_access_arg*)malloc(sizeof(manage_access_arg));
                  access_thread_arg->client_to_accept = client ;
                  access_thread_arg->room = choosen_room ;

                  // Threads creation. They need to be detached so performance won't deteriorate during time cause of zombies threads after users disconnections
                  if ( (err=pthread_create(&tinfo, NULL, manage_access_to_a_room, (void*)access_thread_arg) )!=0 ){
                    printf("Error calling pthread_create with manage_access_to_a_room : %s\n", strerror(err));
                    free(access_thread_arg);
                    // Comunica all'utente
                    sprintf(send_buff, "\nATTENZIONE, ERRORE DURANTE LA RICHIESTA DI ACCESSO ALLA STANZA !\n RIPETERE IL PROCESSO DA CAPO !\n");
                    write(client->client_sd,send_buff,strlen(send_buff));
                  }else{
                    sprintf(send_buff, "\nRICHIESTA PER ENTRARE NELLA STANZA (\"%s\") AVVENUTA CON SUCCESSO ... ATTENDERE PREGO ...\n",room_name);
                    write(client->client_sd,send_buff,strlen(send_buff));
                    return 0; // exit this thread
                  }
                } else {
                  sprintf(send_buff, "\nATTENZIONE, STANZA (\"%s\") NON TROVATA ! CONTROLLARE CHE IL NOME INSERITO RISPETTI SPAZI E PUNTEGGIATURA(CASESENSITIVE) !\n",room_name);
                  write(client->client_sd,send_buff,strlen(send_buff));
                }
          }else{
                sprintf(send_buff, "\nATTENZIONE, EFFETTUARE PRIMA L'ACCESSO !\n");
                write(client->client_sd,send_buff,strlen(send_buff));
          }

        }

        dim_recv_messagge = 0;
      }

    }else if(n_read_char == 0){
      // If read returns 0 the socket with the client and the connection has been closed
      goto gone_client;
    }


  }

   gone_client:
   // LOGGING DISCONNECTIONS
   printf("\n-A CLIENT DISCONNECTED :\nNickname : %s\nSocket Descriptor : %d\nIP ADDRESS : %s\n",client->username,client->client_sd,client->IP_address);
   close(client->client_sd);
   free(client);
   return 0; // Implicit call to pthread_exit
}

// Entrypoint of the thread that will manage one chatroom at time
void *manage_a_single_room(void *arg){

  room_info* room = (room_info*)arg ;

  // Allocate buffer to hold incoming messages
  char recv_buff[BUF_SIZE];
  char send_buff[BUF_SIZE];
  int n_read_char;


  // Parameters for the select.
  int numfds ; // if maxd is the greatest socket descriptor, numfds will be maxd+1
  int num_descriptors_ready; // Returned by the select
  fd_set read_fds; // Set of socket descriptors to check for
  struct timeval timeout;
  timeout.tv_sec = 1;

  sprintf(send_buff, "\n*** BENVENUTI NELLA STANZA (\"%s\") ! ***\n",room->room_name);
  write(room->owner_sd,send_buff,strlen(send_buff));

  while(1){

    // Set numfds with the socket descriptors of users connected to the room
    numfds = fillSet(&read_fds, room);

    // Wait for data to be available on any of the connected client sockets
    num_descriptors_ready = select(numfds,&read_fds,NULL,NULL,&timeout);

    if (num_descriptors_ready<0){ // Error from select
      printf("Error calling select : %s\nConversation has ended\n", strerror(num_descriptors_ready));
      goto close_chatroom ;
    }else if (num_descriptors_ready>0){ // Some descriptors ready
        // Check which socket has data available and receive the message
        pthread_mutex_lock(&room->connected_clients->semaphore);

          linkedListUsersNode* iterator = room->connected_clients->head ;
          while (iterator!=NULL){

            int client_sd = iterator->data->client_sd;
            if ( FD_ISSET(client_sd,&read_fds) != 0 ){ // client_sd ha scritto qualcosa da leggere
                        if ( (n_read_char = read(client_sd, recv_buff, BUF_SIZE)) > 0){
                            recv_buff[n_read_char]='\0';
                            // Se non si tratta del comando di exit inoltra a tutti gli altri
                            int result_parsing_request = parse_client_request(recv_buff);
                            if ( result_parsing_request!=8 ){
                              sprintf(send_buff, "\n-- <%s> --\n%s",iterator->data->username,recv_buff);
                              broadcast_message(send_buff,client_sd,room);
                            }else{  // Si tratta del comando //command:<EXIT ROOM>
                              // The user wants to exit the room
                              if (client_sd!=room->owner_sd){
                                // Cercare il puntatore alla client_info di client_sd
                                client_info* client_to_manage = accessByClientSd(client_sd,room->connected_clients);
                                // Prima di rimuovere il nodo faccio avanzare iterator per evitare segmentation fault
                                iterator = iterator->next ;
                                // Rimuovere da connected clients
                                remove_user(client_to_manage,room->connected_clients);
                                // Salutiamo il client_to_manage
                                sprintf(send_buff, "\n-- DISCONNESSIONE DA \"<%s>\" AVVENUTA CON SUCCESSO --\n\n***** BENVENUTI IN MULTICHAT ! *****\n\n--- Digitare //command:<HELP> per conoscere i comandi disponibili ---\n\n",room->room_name);
                                write(client_to_manage->client_sd,send_buff,strlen(send_buff));
                                // Notifichiamo al resto della chat della sua disconnessione
                                sprintf(send_buff, "\n-- \"<%s>\" HA LASCIATO LA STANZA --\n",client_to_manage->username);
                                broadcast_message(send_buff,client_to_manage->client_sd,room);
                                // Lanciare manage_a_single_client
                                launch_manage_a_single_client_thread(client_to_manage);
                                // Continue in questo modo salto iterator = iterator->next che ho già eseguito
                                continue;
                              }
                              else{ // Chiudere la stanza mettendo tutti in manage_a_single_client

                                // Salutiamo l'admin
                                sprintf(send_buff, "\n-- DISCONNESSIONE DA \"<%s>\" AVVENUTA CON SUCCESSO --\n\n***** BENVENUTI IN MULTICHAT ! *****\n\n--- Digitare //command:<HELP> per conoscere i comandi disponibili ---\n\n",room->room_name);
                                write(room->owner_sd,send_buff,strlen(send_buff));
                                // Notifichiamo al resto della chat della chiusura della chatroom a causa della disconnessione dell'admin
                                sprintf(send_buff, "\n-- LA STANZA : \"<%s>\" È STATA CHIUSA DAL CREATORE --\n\n***** BENVENUTI IN MULTICHAT ! *****\n\n--- Digitare //command:<HELP> per conoscere i comandi disponibili ---\n\n",room->room_name);
                                broadcast_message(send_buff,room->owner_sd,room);
                                // Settare il flag di distruzione
                                room->destroy_flag = true ;
                                // Esci dal ciclo while iterator!=NULL
                                break;
                              }

                            }
                        }else{
                            if(n_read_char < 0){
                              sprintf(send_buff, "\nError sending the message. Try again !\n");
                              write(client_sd,send_buff,strlen(send_buff));
                            }else{ // n == 0
                              // Reading 0 means the client has disconnected
                                if (client_sd!=room->owner_sd){
                                  // Cercare il puntatore alla client_info di client_sd
                                  client_info* client_to_dealloc = accessByClientSd(client_sd,room->connected_clients);
                                  // Prima di rimuovere il nodo faccio avanzare iterator per evitare segmentation fault
                                  iterator = iterator->next ;
                                  // Rimuovere da connected clients
                                  remove_user(client_to_dealloc,room->connected_clients);
                                  // Notifichiamo al resto della chat della sua disconnessione
                                  sprintf(send_buff, "\n-- \"<%s>\" HA LASCIATO LA STANZA --\n",client_to_dealloc->username);
                                  broadcast_message(send_buff,client_to_dealloc->client_sd,room);
                                  // Deallocare risorse utente
                                  // LOGGING DISCONNECTIONS
                                  printf("\n-A CLIENT DISCONNECTED :\nNickname : %s\nSocket Descriptor : %d\nIP ADDRESS : %s\n",client_to_dealloc->username,client_to_dealloc->client_sd,client_to_dealloc->IP_address);
                                  close(client_to_dealloc->client_sd);
                                  free(client_to_dealloc);
                                  // Continue in questo modo salto iterator = iterator->next che ho già eseguito
                                  continue;
                                }
                                else{
                                  // Cercare il puntatore alla client_info di owner_sd
                                  client_info* client_to_dealloc = accessByClientSd(client_sd,room->connected_clients);
                                  // Rimuovere da connected clients
                                  remove_user(client_to_dealloc,room->connected_clients);
                                  // Deallocare risorse admin
                                  // LOGGING DISCONNECTIONS
                                  printf("\n-A CLIENT DISCONNECTED :\nNickname : %s\nSocket Descriptor : %d\nIP ADDRESS : %s\n",client_to_dealloc->username,client_to_dealloc->client_sd,client_to_dealloc->IP_address);
                                  close(client_to_dealloc->client_sd);
                                  free(client_to_dealloc);
                                  // Notifichiamo al resto della chat della chiusura della chatroom a causa del crash dell'admin
                                  sprintf(send_buff, "\n-- LA STANZA : \"<%s>\" È STATA CHIUSA A CAUSA DELLA DISCONNESSIONE DEL CREATORE --\n\n***** BENVENUTI IN MULTICHAT ! *****\n\n--- Digitare //command:<HELP> per conoscere i comandi disponibili ---\n\n",room->room_name);
                                  broadcast_message(send_buff,room->owner_sd,room);
                                  // Settare il flag di distruzione
                                  room->destroy_flag = true ;
                                  // Esci dal ciclo while iterator!=NULL
                                  break;
                                }
                            }
                        }
            }
            // Scorre sul prossimo client_sd
            iterator = iterator->next ;
          }

        pthread_mutex_unlock(&room->connected_clients->semaphore);

    }
    // else if num_descriptors_ready == 0 the timer of 3 seconds has expired and we do nothing except going back for updating users_list

    // destroy_flag equals true means that the chatroom has to be ended
    if (room->destroy_flag==true)
      goto close_chatroom;

  }

  close_chatroom:
  // Takes all the clients from connected_clients room's list and launches for each one a new thread "manage_a_single_client"
  pthread_mutex_lock(&room->connected_clients->semaphore);
    linkedListUsersNode* iterator = room->connected_clients->head ;
    while (iterator!=NULL){
        launch_manage_a_single_client_thread(iterator->data);
        iterator = iterator->next ;
    }
  pthread_mutex_unlock(&room->connected_clients->semaphore);
  // Rimuovere dalla lista globale e deallocare room
  remove_room(room, rooms_list);
  // Deallocare room
  destroy_list_users(room->connected_clients);
  room->connected_clients = NULL ;
  destroy_queue(room->adminMessagesQueue);
  room->adminMessagesQueue = NULL ;
  pthread_mutex_destroy(&room->acceptance_semaphore);
  free(room);
  // Esco dal thread manage_a_single_room
  return 0;

}

// Entrypoint of the thread that will manage an access request to a room
void *manage_access_to_a_room(void *arg){

    char send_buff[BUF_SIZE];
    char recv_buff[BUF_SIZE];
    manage_access_arg* argument = (manage_access_arg*)arg ;

    // Acquisisco mutex per evitare race conditions tra più client che fanno richiesta di accesso
    pthread_mutex_lock(&argument->room->acceptance_semaphore);

    argument->room->isAdminBusy = true ;

    sprintf(send_buff,"\n*** NUOVA RICHIESTA DI ACCESSO AL CANALE ***\n\"%s\" HA CHIESTO DI UNIRSI\nACCETTARE ? (S/s)/(Qualsiasi altra cosa per rifiutare)\n",argument->client_to_accept->username);
    write(argument->room->owner_sd,send_buff,strlen(send_buff));

    int n_read_byte;
    if( (n_read_byte=read(argument->room->owner_sd,recv_buff,BUF_SIZE)) > 0 ){

      recv_buff[n_read_byte-1]='\0';
      if (strcmp(recv_buff,"S")==0 || strcmp(recv_buff,"s")==0 ){ // Accetto il client
        insert_user(argument->client_to_accept, argument->room->connected_clients);
        sprintf(send_buff, "\nRICHIESTA ACCETTATA ! BENVENUTI NELLA STANZA (\"%s\") !\nDIGITARE //command:<EXIT ROOM> PER USCIRE DALLA STANZA\n",argument->room->room_name);
        write(argument->client_to_accept->client_sd,send_buff,strlen(send_buff));
        sprintf(send_buff, "\n(\"%s\") SI È UNITO ALLA STANZA !\n",argument->client_to_accept->username);
        broadcast_message(send_buff,argument->client_to_accept->client_sd,argument->room);
      }else{
        sprintf(send_buff, "\nATTENZIONE, ACCESSO RIFIUTATO !\n");
        write(argument->client_to_accept->client_sd,send_buff,strlen(send_buff));
        launch_manage_a_single_client_thread(argument->client_to_accept);
      }

    }else{
      argument->room->destroy_flag = true ;
    }

    argument->room->isAdminBusy = false ;
    dequeue_all_messages(argument->room->adminMessagesQueue, argument->room->owner_sd);

    //Rilascio mutex riguardante l'accettazione
    pthread_mutex_unlock(&argument->room->acceptance_semaphore);

    free(argument);
    return 0; // Chiamata implicita a pthread_exit;

}
