#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
int clients[200];
int n = 0;

void test(char *expectedRes, char *currRes, const char *testName)
{
    if (expectedRes == currRes)
    {
        printf("\n%s Passed", testName);
    }
    else
    {
        printf("\n%s Failed. expected: %s, but actual result: %s", testName, expectedRes, currRes);
    }
}

void sendtoall(char *msg, int curr)
{
    int i;
    pthread_mutex_lock(&mutex);
    //change into switch
    //1. EB: ebMsg
    //2. fuel empty in 12km: lfMsg
    //3. check for dist
    //4. change of lane
    char ebMsg[500] = "EB msg recieved from ";
    char lfMsg[500] = "Low fuel alert recieved from ";
    char *ptr1 = strstr(msg, "EB");
    char *ptr2 = strstr(msg, "low fuel");
    char *ptr3 = strstr(msg, "disctance estimation");
    char *ptr4 = strstr(msg, "lane");

    if (ptr1 != NULL)
    {
        printf("Detected EB\nEB msg recieved from %s", msg);
        strcat(ebMsg, msg);
        msg = ebMsg;
        printf("Sending modified msg, '%s'", msg);
    }
    else if (ptr2 != NULL)
    {
        printf("Low fuel acknowledged\n");
        strcat(lfMsg, msg);
        msg = lfMsg;
        printf("Sending modified msg, '%s'", msg);
    }
    else if (ptr3 != NULL)
    {
        printf("Starting distance estimation\n");
    }
    else if (ptr4 != NULL)
    {
        printf("Adapting to lane change\n");
    }

    for (i = 0; i < n; i++)
    {
        if (clients[i] != curr)
        {
            if (send(clients[i], msg, strlen(msg), 0) < 0)
            {
                printf("sending failure \n");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void *recvmg(void *client_sock)
{
    int sock = *((int *)client_sock);
    char msg[500];
    int len;
    char *ptr3 = strstr(msg, "exit");
    while ((len = recv(sock, msg, 500, 0)) > 0)
    {
        msg[len] = '\0';
        printf("%s", msg);
        sendtoall(msg, sock);
    }
}

void *sendMsg(void *clientSock)
{
    int sock = *((int *)clientSock);
    char serverMsg[500];
    while (fgets(serverMsg, 500, stdin) > 0)
    {
        char catMsg[100] = "Server Msg: ";
        strcat(catMsg, serverMsg);
        sendtoall(catMsg, sock);
    }
}

int main()
{
    struct sockaddr_in ServerIp;
    pthread_t recvt;
    pthread_t sendt;
    int sock = 0, Client_sock = 0;
    char serverMsg[500];

    ServerIp.sin_family = AF_INET;
    ServerIp.sin_port = htons(1234);
    ServerIp.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    char bindMsg[500];
    if (bind(sock, (struct sockaddr *)&ServerIp, sizeof(ServerIp)) == -1)
    {
        printf("cannot bind, error!! \n");
        char thismsg[500] = "cannot bind, error!! \n";
    }
    else
    {
        printf("Server Started\n");
        char thismsg[500] = "Server Started\n";
    }

    if (listen(sock, 20) == -1)
        printf("listening failed \n");

    while (1)
    {
        if ((Client_sock = accept(sock, (struct sockaddr *)NULL, NULL)) < 0)
        {
            printf("accept failed  \n");
        }
        else
        {
            printf("Connection accepted from new truck\n");
        }
        pthread_mutex_lock(&mutex);
        clients[n] = Client_sock;
        n++;
        // creating a thread for each client
        pthread_create(&recvt, NULL, (void *)recvmg, &Client_sock);
        pthread_create(&sendt, NULL, (void *)sendMsg, &Client_sock);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}
