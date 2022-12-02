#ifndef GROUP_H
#define GROUP_H

#include "server2.h"

/* Group chat */
typedef struct
{   
   int nb_clients;
   char clients[20][20];
   char name[BUF_SIZE];
   int nb_mess;
   char message[BUF_SIZE][30];
}Group;


#endif /* guard */