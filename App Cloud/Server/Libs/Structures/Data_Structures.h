#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include<stdlib.h> // Needed for malloc, free and signals
#include<pthread.h> // Needed for multithreading
#include<stdbool.h> // Needed for boolean values
#include<unistd.h> // Needed for read, write, sleep and close
#include<string.h> // Needed for strncmp
#include<arpa/inet.h> // Needed for socket net programming
#include<sys/select.h> // Da controllare
#include<signal.h>
#include<netinet/tcp.h>

#define BUF_SIZE 1024

//#include<netinet/ip.h>
//#include<sys/socket.h>
// Headers in eccesso
//#include<stdio.h>
//#include<errno.h>
//#include<time.h>

// DATA STRUCTURES FOR USERS
// Client informations
typedef struct client_inf {
    char IP_address[32]; // Holds the client IP address
    int client_sd ; // The socket_descriptor opened between client and server
    bool authenticated ;
    char username[32]; // Holds the nickname chosen by the the user
} client_info ;

// Node used by the linked list contained into room_info
typedef struct userNode {
  client_info* data ;
  struct userNode* next ;
} linkedListUsersNode ;

/* A simple thread_safe data structure which will holds the different rooms' clients that are waiting to chat
 * with a random stranger. Can't be allocated statically, and the pointer must be initialized with createANewLinkedList()
 * function defined below.
*/
typedef struct linked_l_u {
  linkedListUsersNode* head ;
  int size ;
  pthread_mutex_t semaphore ;
} linkedListUsers ;

typedef struct mexNode{
  char message[BUF_SIZE];
  struct mexNode* next;
} queueNode;

typedef struct queue_mex {
  queueNode* head;
  queueNode* tail;
} queueMessages;

// DATA STRUCTURES FOR ROOMS
// Room informations
typedef struct room_inf {
    char room_name[32];
    int owner_sd ; // Room Owner/Admin's socket descriptor.
    linkedListUsers* connected_clients ; // Room connected clients
    bool isAdminBusy ; // Flag for signaling when an admin can't receive messages because is accepting a request to join
    queueMessages* adminMessagesQueue ; // Queue for storing messages coming while the admin is busy
    pthread_mutex_t acceptance_semaphore ; // We can process one request to join a time. Acquired and released by manage_access_to_a_room thread
    bool destroy_flag ; // Signal when it's time to destroy the room
} room_info ;

// Node used by the linked list containing rooms
typedef struct roomNode {
  room_info* data ;
  struct roomNode* next ;
} linkedListRoomsNode ;

// The global linkedlist of rooms
typedef struct linked_l_r {
  linkedListRoomsNode* head ;
  int size ;
  pthread_mutex_t semaphore ;
} linkedListRooms ;

// ARGUMENT FOR THE THREAD manage_access_to_a_room
typedef struct manage_access_targ {
  client_info* client_to_accept ;
  room_info* room ;
} manage_access_arg ;

// LIST OF LINKEDLIST FUNCTIONS

/* Initializes the list like a default constructor would, allocating the needed resources. To be called one time, only
 * when we declare and allocate a linked list to avoid seg_fault
*/
linkedListUsers* createANewLinkedListUsers();
// Insert on top of the list the new user. Thread safe.
bool insert_user(client_info* client, linkedListUsers* list);
// Removes the record from the list if present. Thread safe.
bool remove_user(client_info* client_to_remove, linkedListUsers* users_list);
/* Access to the ith element of the linkedList. If index is bigger than the dimension or smaller than zero, it returns NULL.
 * Thread Safe.
*/
client_info* accessByClientSd(int client_sd, linkedListUsers* users_list);
// Like an object oriented destructor (it doesn't destroy the client_info data payload of the linkedListUsersNode)
void destroy_list_users(linkedListUsers* list);


// FUNCTIONS FOR THE QUEUE

queueMessages* createANewMessagesQueue();
void enqueue_message(char* message, queueMessages* queue);
void dequeue_all_messages(queueMessages* queue, int client_sd);
void destroy_queue(queueMessages* queue_to_destroy);


// LIST OF ROOMS FUNCTIONS

/* Initializes the list like a default constructor would, allocating the needed resources. To be called one time,
 * only when we declare and allocate a linked list to avoid seg_fault.
*/
linkedListRooms* createANewLinkedListRooms();
// Insert on top of the list the new node. Thread safe.
bool insert_room(room_info* new_room, linkedListRooms* list);
// Removes the record from the list if present. The record itself must be deallocated after the function execution. Thread safe.
bool remove_room(room_info* room_to_remove, linkedListRooms* rooms_list);
/* Access to the ith element of the linkedList. If index is bigger than the dimension or smaller than zero, it returns NULL.
 * Thread Safe.
*/
room_info* accessByName(char* room_name, linkedListRooms* list);
// Returns the size of a list. Thread safe.
int sizeOfTheListRooms(linkedListRooms* list);
// Like an object oriented destructor deallocates the entire structure
void destroy_list_rooms(linkedListRooms* list);




// CHATROOM FUNCTIONS WHICH USE DATA STRUCTURES

/* Add the socket descriptors from connected_clients list inside room into the set read_fds
 * Returns the greatest socket descriptor plus one
*/
int fillSet(fd_set* read_fds, room_info* connected_clients);
/* Takes a string and a linkedListUsers as input and broadcast the message inside buffer to all the users inside
 * the list 'connected_clients'
*/
void broadcast_message(char* buffer, int author_sd, room_info* connected_clients);

#endif
