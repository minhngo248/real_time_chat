#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

/* Struct Client */
typedef struct
{
   SOCKET sock;
   SOCKET sock_dest;
   char name[BUF_SIZE];
   char name_dest[BUF_SIZE];
   char name_group[BUF_SIZE];
}Client;


#endif /* guard */
