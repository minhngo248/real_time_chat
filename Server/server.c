#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "server2.h"
#include "client2.h"

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
   char buffer[BUF_SIZE];
   char history[BUF_SIZE][BUF_SIZE];
   int actual_his = 0;
   char name[BUF_SIZE], name_dest[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   int i;
   int sock_dest = 0;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;
   /* Charge history.txt to history[][] */
   FILE *read_his = fopen("files/history.txt", "r");
   char *str_temp = malloc(BUF_SIZE * sizeof(char));
   fgets(str_temp, BUF_SIZE, read_his);
   while (!feof(read_his))
   {
      strncpy(history[actual_his], str_temp, strlen(str_temp) - 1);
      actual_his++;
      free(str_temp);
      str_temp = malloc(BUF_SIZE * sizeof(char));
      fgets(str_temp, BUF_SIZE, read_his);
   }
   fclose(read_his);
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
         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         for (i = 0; i < actual; i++)
         { /* Take socket of the destinator */
            if (strcmp(name_dest, clients[i].name) == 0)
            {
               sock_dest = clients[i].sock;
               break;
            }
         }
         if (strlen(name_dest) == 0 && sock_dest == 0)
         { // chat to all
            sock_dest = -1;
         }
         Client c = {csock, sock_dest, "", ""};
         strncpy(c.name, name, strlen(name));
         strncpy(c.name_dest, name_dest, strlen(name_dest));
         for (i = 0; i < actual_his; i++)
         {
            write_client(c.sock, history[i]);
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
               for (j = 0; j < actual; j++)
               { /* Take socket of the destinator */
                  if (strcmp(client.name_dest, clients[j].name) == 0)
                  {
                     client.sock_dest = clients[j].sock;
                     break;
                  }
               }
               if ( j == actual && client.sock_dest != -1 && client.sock_dest != 0)
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
                  if (client.sock_dest == -1)
                  { // chat to all
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
               break;
            }
         }
      }

      if (actual_his >= BUF_SIZE)
      {
         break;
      }
   }

   /* Save the history of conversation in file */
   FILE *file_his = fopen("files/history.txt", "w");
   for (i = 0; i < actual_his; i++)
   {
      fprintf(file_his, "%s\n", history[i]);
   }
   fclose(file_his);

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

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
