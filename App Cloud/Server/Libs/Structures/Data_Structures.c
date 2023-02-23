#include "Data_Structures.h"

// LIST OF USERS FUNCTIONS

/* Initializes the list like a default constructor would, allocating the needed resources. To be called one time, only
 * when we declare and allocate a linked list to avoid seg_fault
*/
linkedListUsers* createANewLinkedListUsers(){ // OK
  linkedListUsers* ret_list ;
  if ( (ret_list=(linkedListUsers*)malloc(sizeof(linkedListUsers)))!=NULL ){
    ret_list->head = NULL ;
    ret_list->size = 0 ;
    pthread_mutexattr_t mutex_attr ;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ret_list->semaphore,&mutex_attr) ;
    pthread_mutexattr_destroy(&mutex_attr);
  }
  return ret_list;
}
// Insert on top of the list the new user. Thread safe.
bool insert_user(client_info* client, linkedListUsers* list){ // OK
  if (client!=NULL && list!=NULL){

      linkedListUsersNode* new_node = NULL;
      if ( (new_node=(linkedListUsersNode*)malloc(sizeof(linkedListUsersNode))) != NULL ){
        new_node->data = client ;
        new_node->next = NULL ;

        pthread_mutex_lock(&list->semaphore);
        new_node->next = list->head;
        list->head = new_node ;
        list->size++ ;
        pthread_mutex_unlock(&list->semaphore);

        return true ;

      } else {
        return false ;
      }

  } else {
    return false ;
  }
}
// Removes the record from the list if present. Thread safe.
bool remove_user(client_info* client_to_remove, linkedListUsers* users_list){ // OK
  bool result = false ;
  if (client_to_remove!=NULL && users_list!=NULL)  {
    pthread_mutex_lock(&users_list->semaphore);

    linkedListUsersNode* iterator = users_list->head ;
    linkedListUsersNode* last_visited = NULL ;
    while (iterator!=NULL && iterator->data!=client_to_remove){
      last_visited = iterator ;
      iterator = iterator->next ;
    }
    if (iterator!=NULL){
      if(last_visited!=NULL){
        //rimuovi dal centro
        last_visited->next = iterator->next ;
      }else{
        //rimuovi dalla testa
        users_list->head = users_list->head->next;
      }
      iterator->data = NULL ;
      iterator->next = NULL ;
      free(iterator);
      users_list->size-- ;
      result = true ;
    }else{ // Non ho trovato nulla
      result = false ;
    }
    pthread_mutex_unlock(&users_list->semaphore);
  }

  return result ;
}
/* Access to the ith element of the linkedList. If index is bigger than the dimension or smaller than zero, it returns NULL.
 * Thread Safe.
*/
client_info* accessByClientSd(int client_sd, linkedListUsers* users_list){ // OK

  client_info* ret_client = NULL ;
  if (users_list!=NULL){
    pthread_mutex_lock(&users_list->semaphore);
      linkedListUsersNode* iterator = users_list->head ;
      while (iterator->data->client_sd!=client_sd && iterator!=NULL){
        iterator = iterator->next ;
      }
      if(iterator!=NULL)
        ret_client = iterator->data ;

    pthread_mutex_unlock(&users_list->semaphore);
  }
  return ret_client ;

}
// Like an object oriented destructor (it doesn't destroy the client_info data payload of the linkedListUsersNode)
void destroy_list_users(linkedListUsers* list){ //OK
  if (list!=NULL){
    pthread_mutex_lock(&list->semaphore);
    linkedListUsersNode* iterator = list->head ;
    while (iterator!=NULL){
      list->head = list->head->next ;
      iterator->next = NULL;
      iterator->data = NULL;
      free(iterator);
      iterator = list->head ;
    }
    pthread_mutex_unlock(&list->semaphore);
    pthread_mutex_destroy(&list->semaphore);
    free(list) ;
  }
}

// FUNCTIONS FOR THE QUEUE

queueMessages* createANewMessagesQueue(){
  queueMessages* queue_to_return;
  if (( queue_to_return = (queueMessages*)malloc(sizeof(queueMessages)) )!=NULL){
    queue_to_return->head = NULL ;
    queue_to_return->tail = NULL ;
  }
  return queue_to_return;
}

void enqueue_message(char* message, queueMessages* queue){
  if (queue != NULL){

    queueNode* node = (queueNode*)malloc(sizeof(queueNode));
    strcpy(node->message,message);
    node->next = NULL ;

    if (queue->tail != NULL){
      queue->tail->next = node ;
    }else{
      queue->head = node ;
    }

    queue->tail = node ;

  }
}

void dequeue_all_messages(queueMessages* queue, int client_sd){
  if (queue != NULL){
    while (queue->head != NULL){
      queueNode* tmp_ref = queue->head ;
      queue->head = queue->head->next ;
      write(client_sd,tmp_ref->message,strlen(tmp_ref->message));
      free(tmp_ref);
    }
    queue->tail = NULL ;
  }
}

void destroy_queue(queueMessages* queue_to_destroy){
  if (queue_to_destroy != NULL){
    while (queue_to_destroy->head != NULL){
      queueNode* tmp_ref = queue_to_destroy->head ;
      queue_to_destroy->head = queue_to_destroy->head->next ;
      free(tmp_ref);
    }
    queue_to_destroy->tail = NULL ;
    free (queue_to_destroy);
  }
}

// LIST OF ROOMS FUNCTIONS

/* Initializes the list like a default constructor would, allocating the needed resources. To be called one time,
 * only when we declare and allocate a linked list to avoid seg_fault.
*/
linkedListRooms* createANewLinkedListRooms(){ // OK
  linkedListRooms* ret_list ;
  if ( (ret_list=(linkedListRooms*)malloc(sizeof(linkedListRooms)))!=NULL ){
    ret_list->head = NULL ;
    ret_list->size = 0 ;
    pthread_mutex_init(&ret_list->semaphore,NULL);
  }
  return ret_list;
}
// Insert on top of the list the new node. Thread safe.
bool insert_room(room_info* new_room, linkedListRooms* list){ // OK
  if (new_room!=NULL && list!=NULL){

          linkedListRoomsNode* new_node = NULL;
          if ( (new_node=(linkedListRoomsNode*)malloc(sizeof(linkedListRoomsNode))) != NULL ){
            new_node->data = new_room ;
            new_node->next = NULL ;

            pthread_mutex_lock(&list->semaphore);
            new_node->next = list->head;
            list->head = new_node ;
            list->size++ ;
            pthread_mutex_unlock(&list->semaphore);

            return true ;

          } else {
            return false ;
          }

  } else {
    return false ;
  }
}
// Removes the record from the list if present. The record itself must be deallocated after the function execution. Thread safe.
bool remove_room(room_info* room_to_remove, linkedListRooms* rooms_list){ // OK
  bool result = false ;
  if (room_to_remove!=NULL && rooms_list!=NULL)  {
    pthread_mutex_lock(&rooms_list->semaphore);

    linkedListRoomsNode* iterator = rooms_list->head ;
    linkedListRoomsNode* last_visited = NULL ;
    while (iterator!=NULL && iterator->data!=room_to_remove){
      last_visited = iterator ;
      iterator = iterator->next ;
    }
    if (iterator!=NULL){
      if(last_visited!=NULL){
        //rimuovi dal centro
        last_visited->next = iterator->next ;
      }else{
        //rimuovi dalla testa
        rooms_list->head = rooms_list->head->next;
      }
      iterator->data = NULL ;
      iterator->next = NULL ;
      free(iterator);
      rooms_list->size-- ;
      result = true ;
    }else{ // Non ho trovato nulla
      result = false ;
    }
    pthread_mutex_unlock(&rooms_list->semaphore);
  }

  return result ;
}
/* Access to the ith element of the linkedList. If index is bigger than the dimension or smaller than zero, it returns NULL.
 * Thread Safe.
*/
room_info* accessByName(char* room_name, linkedListRooms* list){ // OK
  room_info* ret_room = NULL ;
  if (list!=NULL){
    pthread_mutex_lock(&list->semaphore);
      linkedListRoomsNode* iterator = list->head ;
      while (iterator!=NULL){
        if ( ( strcmp(iterator->data->room_name,room_name) == 0 ) ){
          ret_room = iterator->data ;
          break;
        }
        iterator = iterator->next ;
      }
    pthread_mutex_unlock(&list->semaphore);
  }
  return ret_room ;
}
// Returns the size of a list. Thread safe.
int sizeOfTheListRooms(linkedListRooms* list){ // OK
  int ret_value=-1;
  if (list!=NULL){
    pthread_mutex_lock(&list->semaphore);
    ret_value = list->size ;
    pthread_mutex_unlock(&list->semaphore);
  }
  return ret_value;
}
// Like an object oriented destructor deallocates the entire structure
void destroy_list_rooms(linkedListRooms* list){ // OK
  if (list!=NULL){
    linkedListRoomsNode* iterator_rooms = list->head ;
    while (iterator_rooms!=NULL){
      list->head = list->head->next ;
      iterator_rooms->next = NULL;

          linkedListUsersNode* iterator_users = iterator_rooms->data->connected_clients->head ;
          while (iterator_users!=NULL){
            iterator_rooms->data->connected_clients->head = iterator_rooms->data->connected_clients->head->next ;
            iterator_users->next = NULL;
            // eliminare client_info
            free(iterator_users->data);
            iterator_users->data = NULL;
            // eliminare nodo lista di tipo linkedListUsersNode
            free(iterator_users);
            iterator_users = iterator_rooms->data->connected_clients->head ;
          }
          pthread_mutex_destroy(&iterator_rooms->data->connected_clients->semaphore);
          // eliminare lista connected_clients di tipo linkedListUsers presente all'interno di room_info
          free(iterator_rooms->data->connected_clients) ;

      // eliminare singola room_info
      free(iterator_rooms->data) ;
      iterator_rooms->data = NULL;
      // eliminare nodo dalla lista delle stanze di tipo linkedListRoomsNode
      free(iterator_rooms);
      iterator_rooms = list->head ;
    }
    pthread_mutex_destroy(&list->semaphore);
    // elimina la vera e propria lista di stanze
    free(list) ;
  }
}

// CHATROOM FUNCTIONS WHICH USE DATA STRUCTURES

/* Add the socket descriptors from connected_clients list inside room into the set read_fds
 * Returns the greatest socket descriptor plus one
*/
int fillSet(fd_set* read_fds, room_info* room){ // OK

  int maxSD = 0; // The greatest socket descriptor(plus 1) to be returned

  // Clear the file descriptor set
  FD_ZERO(read_fds);

  pthread_mutex_lock(&room->connected_clients->semaphore);

  linkedListUsersNode* iterator = room->connected_clients->head ;

  // Add the socket descriptors for all connected clients to the set
  while (iterator!=NULL){
      if (iterator->data->client_sd != room->owner_sd || room->isAdminBusy != true)
        FD_SET(iterator->data->client_sd,read_fds);
      // Keeps track of the gratest socket descriptor (which will be returned)
      if ( iterator->data->client_sd > maxSD ){
        maxSD = iterator->data->client_sd ;
      }

      iterator = iterator->next ;
  }
  pthread_mutex_unlock(&room->connected_clients->semaphore);

  return (maxSD+1);

}
/* Takes a string and a linkedListUsers as input and broadcast the message inside buffer to all the users inside
 * the list 'connected_clients'
*/
void broadcast_message(char* buffer, int author_sd, room_info* room){ // OK

  pthread_mutex_lock(&room->connected_clients->semaphore);

  linkedListUsersNode* iterator = room->connected_clients->head ;

  while (iterator!=NULL){

      client_info* client = iterator->data ;

      if (client->client_sd != author_sd){ // EVITA DI INVIARE IL MESSAGGIO A CHI L'HA SCRITTO

        if (client->client_sd!=room->owner_sd){ // CONTROLLA CHE IL DESTINATARIO NON SIA IL PROPIETARIO
          write(client->client_sd, buffer, strlen(buffer)) ;
        }else{
          if ( room->isAdminBusy == false ){
            write(client->client_sd, buffer, strlen(buffer)) ;
          }else{
            enqueue_message(buffer,room->adminMessagesQueue);
          }
        }
      }

      iterator = iterator->next ;
  }

  pthread_mutex_unlock(&room->connected_clients->semaphore);

}

/*// LIST FUNCTIONS
// Initializes the list like a default constructor would, allocating the needed resources. To be called one time, only when we declare and allocate a linked list to avoid seg_fault
linkedList* createANewLinkedList(){
    linkedList* ret_list ;
    if ( (ret_list=(linkedList*)malloc(sizeof(linkedList)))!=NULL ){
      ret_list->head = NULL ;
      ret_list->size = 0 ;
      pthread_mutex_init(&ret_list->semaphore,NULL);
    }
    return ret_list;
}
// Insert on top of the list the new node. Thread safe.
void insert_element(linkedListNode* record, linkedList* list){
  if (record!=NULL && list!=NULL)  {
    pthread_mutex_lock(&list->semaphore);
    record->next = list->head;
    list->head = record ;
    list->size++ ;
    pthread_mutex_unlock(&list->semaphore);
  }
}
// Removes the record (found with accessByIndex function) from the list if still present. Thread safe.
void remove_element(linkedListNode* record, linkedList* list){
  if (record!=NULL && list!=NULL)  {
      pthread_mutex_lock(&list->semaphore);
      linkedListNode* iterator = list->head ;
      linkedListNode* last_visited = NULL ;
      while (iterator!=NULL && iterator!=record){
        last_visited = iterator ;
        iterator = iterator->next ;
      }
      if (iterator!=NULL){
        if(last_visited!=NULL){
          //rimuovi dal centro
          last_visited->next = iterator->next ;
        }else{
          //rimuovi dalla testa
          list->head = iterator->next ;
        }
        iterator->data = NULL ;
        iterator->next = NULL ;
        free(iterator);
        list->size-- ;
      }
      pthread_mutex_unlock(&list->semaphore);
  }
}
// Access to the ith element of the linkedList. If index is bigger than the dimension or smaller than zero, it returns NULL. Thread Safe.
linkedListNode* accessByIndex(int index, linkedList* list){
  linkedListNode* ret_value = NULL ;
  if (list!=NULL){
    pthread_mutex_lock(&list->semaphore);
    linkedListNode* iterator = list->head ;
    if (index < list->size && index>=0){
      for (int i = 0; i < index; i++) {
        iterator = iterator->next ;
      }
      ret_value = iterator ;
    }
    pthread_mutex_unlock(&list->semaphore);
  }
  return ret_value;
}
// Returns the size of a list. Thread safe.
int sizeOfTheList(linkedList* list){
  int ret_value=-1;
  if (list!=NULL){
    pthread_mutex_lock(&list->semaphore);
    ret_value = list->size ;
    pthread_mutex_unlock(&list->semaphore);
  }
  return ret_value;
}
// Like an object oriented destructor
void destroy_list(linkedList* list){
  if (list!=NULL){
    linkedListNode* iterator = list->head ;
    while (iterator!=NULL){
      list->head = list->head->next ;
      iterator->next = NULL;
      free(iterator->data);
      iterator->data = NULL;
      free(iterator);
      iterator = list->head ;
    }
    list->head = NULL;
    pthread_mutex_destroy(&list->semaphore);
    free(list) ;
  }
}
*/
