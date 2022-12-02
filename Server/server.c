#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   int i;
   char buffer[BUF_SIZE];
   char init_buffer[BUF_SIZE];
   char history[BUF_SIZE][BUF_SIZE];
   int actual_his = 0;
   char name[BUF_SIZE] = "", name_dest[BUF_SIZE] = "", name_group[BUF_SIZE] = "";
   /* the index for the array */
   int actual = 0;
   int actual_group = 0;
   int max = sock;

   int sock_dest = 0;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   /* an array for all groups */
   Group groups[MAX_GROUPS];

   fd_set rdfs;
   /* Charge history.txt to history[][] */
   restore_history(history, &actual_his);
   while (1)
   {
      i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, (socklen_t *)&sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }
         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;
         FD_SET(csock, &rdfs);
         Client c = {csock, sock_dest, "", "", ""};
         
         if (strstr(buffer, "--group"))
         {
            /* take name of a client */
            for (i = 0; i < strlen(buffer); i++)
            {
               if (buffer[i] == ' ')
                  break;
               name[i] = buffer[i];
            }
            name[i] = '\0';
            int j;
            /* get name of group chat */
            for (j = i + 9; j < strlen(buffer); j++)
            {
               name_group[j - i - 9] = buffer[j];
            }
            name_group[j - i - 9] = '\0';
            /* Find whether group exist */
            for (i = 0; i < actual_group; i++)
            {
               /* persist */
               if (strcmp(name_group, groups[i].name) == 0)
               {
                  int nb_clients = groups[i].nb_clients;
                  groups[i].clients[nb_clients] = c;
                  strncpy(groups[i].clients[nb_clients].name, name, strlen(name) + 1);
                  strncpy(groups[i].clients[nb_clients].name_group, name_group, strlen(name_group) + 1);
                  groups[i].clients[nb_clients].sock_dest = -2;
                  groups[i].nb_clients++;
                  strncpy(c.name, name, strlen(name));
                  strncpy(c.name_group, name_group, strlen(name_group));
                  c.sock_dest = -2;
                  break;
               }
            }
            /* If group does not exist */
            if (i == actual_group)
            {
               Group group;
               group.clients[0] = c;
               group.nb_mess = 0;
               strncpy(group.clients[0].name, name, strlen(name) + 1);
               strncpy(group.clients[0].name_group, name_group, strlen(name_group) + 1);
               group.clients[0].sock_dest = -2;
               group.nb_clients = 1;
               strncpy(group.name, name_group, strlen(name_group));
               c.sock_dest = -2;
               strncpy(c.name, name, strlen(name));
               strncpy(c.name_group, name_group, strlen(name_group));
               groups[actual_group] = group;
               actual_group++;
            }
            /* merge buffer */
            char message_his[BUF_SIZE];
            strncpy(message_his, "Last message of all server\n", 28);
            for (i = 0; i < actual_his; i++)
            {
               char s[100];
               strncpy(s, history[i], strlen(history[i]) + 1);
               strncat(s, "\n", 2);
               strncat(message_his, s, strlen(s) + 1);
            }
            char message_group[BUF_SIZE];
            strncpy(message_group, "Last message of group ", 24);
            strncat(message_group, name_group, strlen(name_group) + 1);
            strncat(message_group, "\n", 2);
            char mess[BUF_SIZE];
            restore_message_group(mess, name_group);
            strncat(message_group, mess, strlen(mess));
            strncpy(init_buffer, message_his, strlen(message_his));
            strncat(init_buffer, message_group, strlen(message_group));
            strncat(init_buffer, "\nYou are in the group chat ", 30);
            strncat(init_buffer, name_group, strlen(name_group));
            strncat(init_buffer, "\n", 2);
            write_client(csock, init_buffer);
         }
         else
         {
            for (i = 0; i < strlen(buffer); i++)
            {
               if (buffer[i] == '\n' || buffer[i] == '\0')
                  break;
               name[i] = buffer[i];
            }
            name[i] = '\0';
            int j;
            for (j = i + 1; j < strlen(buffer); j++)
            {
               if (buffer[j] == '\0')
                  break;
               name_dest[j - i - 1] = buffer[j];
            }
            name_dest[j - i - 1] = '\0';

            for (i = 0; i < actual; i++)
            { /* Take socket of the destinator */
               if (strcmp(name_dest, clients[i].name) == 0)
               {
                  sock_dest = clients[i].sock;
                  break;
               }
            }
            if (strlen(name_dest) == 0 && sock_dest == 0)
            { // chat to all server
               sock_dest = -1;
            }
            strncpy(c.name, name, strlen(name));
            strncpy(c.name_dest, name_dest, strlen(name_dest));
            /* merge buffer */
            char message_his[BUF_SIZE];
            strncpy(message_his, "Last message of all server\n", 28);
            for (i = 0; i < actual_his; i++)
            {
               char s[100];
               strncpy(s, history[i], strlen(history[i]) + 1);
               strncat(s, "\n", 2);
               strncat(message_his, s, strlen(s) + 1);
            }
            char message_private[BUF_SIZE] = "";
            if (strcmp(name_dest, "") != 0)
            {
               strncpy(message_private, "Last message with ", 20);
               strncat(message_private, name_dest, strlen(name_dest) + 1);
               strncat(message_private, "\n", 2);
               char mess[BUF_SIZE];
               restore_private_message(mess, name, name_dest);
               strncat(message_private, mess, strlen(mess) + 1);
            }
            strncpy(init_buffer, message_his, strlen(message_his) + 1);
            strncat(init_buffer, message_private, strlen(message_private) + 1);
            c.sock_dest = sock_dest;
            if (c.sock_dest == -1)
            {
               strncat(init_buffer, "\nYou are talking to all server\n", 33);
            }
            else if (c.sock_dest >= 0)
            {
               strncat(init_buffer, "\nYou are talking to client ", 30);
               strncat(init_buffer, name_dest, strlen(name_dest) + 1);
               strncat(init_buffer, "\n", 2);
            }
            
            strncpy(c.name_group, "", 1);
            write_client(csock, init_buffer);
         }
         clients[actual] = c;
         actual++;
      }
      else
      {
         i = 0;
         for (i = 0; i < actual; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int j;
               /* Take socket of the destinator */
               for (j = 0; j < actual; j++)
               {
                  if (strcmp(client.name_dest, clients[j].name) == 0)
                  {
                     client.sock_dest = clients[j].sock;
                     break;
                  }
               }
               if (j == actual && client.sock_dest != -1 && client.sock_dest != 0)
               {
                  client.sock_dest = 0;
               }
               int c = read_client(clients[i].sock, buffer);
               if (c == 0) /* client disconnected */
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  printf("%s\n", buffer);
                  strncpy(history[actual_his], buffer, BUF_SIZE - 1);
                  actual_his++;
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  /* Group chat */
                  if (strlen(client.name_group) > 0)
                  {
                     Group group;
                     int k;
                     /* Find group chat */
                     for (k = 0; k < actual_group; k++)
                     {
                        if (strcmp(client.name_group, groups[k].name) == 0)
                        {
                           group = groups[k];
                           /* Concatenate client's message */
                           strncpy(name, client.name, strlen(client.name) + 1);
                           strncat(name, " : ", 4);
                           strncat(name, buffer, strlen(buffer) + 1);
                           strncat(name, "\n", 2);
                           strncpy(groups[k].message[group.nb_mess], name, strlen(name) + 1);
                           groups[k].nb_mess++;
                           break;
                        }
                     }

                     /* Send to other clients in group */
                     int size = strlen(name);
                     name[size - 1] = '\0';
                     for (k = 0; k < group.nb_clients; k++)
                     {
                        if (strcmp(group.clients[k].name, client.name) != 0)
                        {
                           SOCKET sock_other = group.clients[k].sock;
                           write_client(sock_other, name);
                        }
                     }
                  }
                  else
                  {
                     if (client.sock_dest == -1)
                     { // chat to all server
                        send_message_to_all_clients(clients, client, actual, buffer, 0);
                        strncpy(name, "", BUF_SIZE - 1);
                        strncpy(name, client.name, strlen(client.name));
                        strncat(name, " : ", 4);
                        strncat(name, buffer, strlen(buffer));
                        strncpy(history[actual_his], name, BUF_SIZE - 1);
                        actual_his++;
                     }
                     else if (client.sock_dest == 0)
                     { // destinator is offline
                        char str[BUF_SIZE];
                        strncpy(str, "files/", 7);
                        strncat(str, client.name_dest, strlen(client.name_dest));
                        strncat(str, ".txt", 5);
                        FILE *read_file = fopen(str, "r");
                        if (read_file == NULL)
                        {
                           FILE *temp = fopen(str, "w");
                           fprintf(temp, "%s\n", client.name);
                           fprintf(temp, "%s : %s\n", client.name, buffer);
                           fclose(temp);
                        }
                        else
                        {
                           fclose(read_file);
                           FILE *write_file = fopen(str, "a");
                           fprintf(write_file, "%s : %s\n", client.name, buffer);
                           fclose(write_file);
                        }
                     }
                     else
                     { // chat to client
                        strncpy(name, "", BUF_SIZE - 1);
                        strncpy(name, client.name, strlen(client.name));
                        strncat(name, " : ", 4);
                        strncat(name, buffer, strlen(buffer));
                        write_client(client.sock_dest, name);
                     }
                  }
               }
               break;
            }
         }
      }

      if (actual_his >= BUF_SIZE)
      {
         break;
      }
   }

   save_history(history, actual_his);
   for (i = 0; i < actual_group; i++)
   {
      save_message_group(groups[i].message, groups[i].nb_mess, groups[i].name);
   }
   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

/* Restore history of conversation of all server */
static void restore_history(char history[][BUF_SIZE], int *actual_his)
{
   FILE *read_his = fopen("files/history.txt", "r");
   char c;
   int j;
   while (1)
   {
      c = fgetc(read_his);

      if (c == EOF)
      {
         break;
      }
      j = 0;
      while (c != '\n')
      {
         history[*actual_his][j] = c;
         j++;
         c = fgetc(read_his);
      }
      history[*actual_his][j] = '\0';
      (*actual_his)++;
   }
   fclose(read_his);
}

/* Save the history of conversation in file */
static void save_history(const char history[][BUF_SIZE], const int actual_his)
{
   int i;
   FILE *file_his = fopen("files/history.txt", "w");
   for (i = 0; i < actual_his; i++)
   {
      fprintf(file_his, "%s\n", history[i]);
   }
   fclose(file_his);
}

static void restore_message_group(char *message, const char *name_group)
{
   int i;
   char path[100];
   char c;
   sprintf(path, "files/group_%s.txt", name_group);
   FILE *file = fopen(path, "r");
   if (file != NULL)
   {
      i = 0;
      while (!feof(file))
      {
         c = fgetc(file);
         message[i] = c;
         i++;
      }
      message[i] = '\0';
      fclose(file);
   }
}

static void save_message_group(const char message[][30], const int nb_mess, const char *name_group)
{
   char path[100];
   char c;
   sprintf(path, "files/group_%s.txt", name_group);
   FILE *file = fopen(path, "w");
   int i;
   for (i = 0; i < nb_mess; i++)
   {
      fputs(message[i], file);
   }
   fclose(file);
}

static void restore_private_message(char *message, const char *name, const char *name_dest)
{
   char str[BUF_SIZE];
   strncpy(message, "", 1);
   sprintf(str, "files/%s.txt", name);
   FILE *file = fopen(str, "r");
   if (file != NULL)
   {
      fgets(str, BUF_SIZE, file);
      int size = strlen(str);
      str[size - 1] = '\0';
      if (strcmp(str, name_dest) == 0)
      {
         while (!feof(file))
         {
            fgets(str, BUF_SIZE, file);
            strncat(message, str, strlen(str) + 1);
         }
      }
      fclose(file);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
