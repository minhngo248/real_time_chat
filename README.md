# TP Programmation Réseaux
4IF NGO Ngoc Minh, EL BOU Aicha
## Execute 
- Execute Client.c `make client`
- Execute Server.c `make server`
## Run the excutables
- Run the server `make run_server`
- You have to run the server before running the client `Client/client [address] [pseudo]` for group chatting or `Client/client [address] [pseudo] [pseudo_of_destinator]` for private chat with `pseudo_of_destinator`
For example
```
Client/client 127.0.0.1 Minh
```
I am Minh and I would to do group chat in localhost.
```
Client/client 127.0.0.1 Minh a
```
I am Minh and I would to chat with client `a` in localhost.
```
Note: pseudos of clients have to be 2-to-2 identical.
```
## Functionnalities we have developed
1. Server is informed when a client is disconnected.
![image](./img/Server.png)
2. Restore the history of group chat when a client connects to do group chat and Save history of group chat. Save the conversation in `files/history.txt`
3. Private discussion of 2 clients in a server.
4. Save the private chat when destinator is offline. For example, `sender` want to talk to `destinator` but `destinator` is offline. The conversation will be saved in `files/[destinator].txt`. When `destinator` connects, the private conversation and then the conversation of all server is restored. They will be shown on the `destinator`'s screen.
![image](./img/private_chat.png)