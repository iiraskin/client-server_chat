#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define maxSizeOfLogin 31
#define maxSizeOfPassword 31
#define messBufMaxSize 7000

struct Message
{
    char type;
    int length;
    int stringLength[messBufMaxSize];
    char* strings[messBufMaxSize];
    int k;
};

int sizeOfMes (char* mes)
{
    int i = 1;
    int res = 0;
    for (i; i < 5; i++) {
        res = res * 256 + (mes[i] + 256) % 256;
    }
    return res;
}

void Pars(char* mes, struct Message* parsMes) {
    parsMes->type = mes[0];
    int i = 1;
    int j = 0;
    parsMes->length = sizeOfMes(mes);
    int k = 0;
    i = 5;
    while (i < parsMes->length + 5) {
        j = 0;
        parsMes->stringLength[k] = 0;
        for (j; j < 4; ++j) {
            parsMes->stringLength[k] = parsMes->stringLength[k] * 256 + (mes[i] + 256) % 256;
            i++;
        }
        j = 0;
        parsMes->strings[k] = malloc(parsMes->stringLength[k]);
        parsMes->strings[k][0] = 0;
        strncat(parsMes->strings[k], mes + i, parsMes->stringLength[k]);
        i += parsMes->stringLength[k];
        k++;
    }
    parsMes->k = k;
    return;
}

void MakeMes(char* mes, struct Message* parsMes) {
    mes[0] = parsMes->type;
    int Length = parsMes->length;
    int i = 4;
    for (i; i >= 1; i--) {
        mes[i] = (char)(Length % 256);
        Length /= 256;
    }
    int j = 5;
    int k = 0;
    for (k; k < parsMes->k; k++) {
        Length = parsMes->stringLength[k];
        i = j + 3;
        for (i; i >= j; i--) {
            mes[i] = (char)(Length % 256);
            Length /= 256;
        }
        mes[j + 4] = 0;
        char* mes1 = mes + j + 4;
        mes1 = strcat(mes1, parsMes->strings[k]);
        j += 4 + parsMes->stringLength[k];
    }
    return;
}

void PrintTime( char* timeMic )
{
    char time[20];
    int i = 0;
    while (timeMic[i] != '$') {
        time[i] = timeMic[i];
        i++;
    }
    time[i] = 0;
    time_t timeInt = atol(time);
    struct tm* timeStruct = localtime(&timeInt);
    printf("[%d:%d:%d] ", timeStruct->tm_hour, (timeStruct->tm_min), timeStruct->tm_sec);
}

void* serverProcess(void * data) {
    int sockid = *(int*)data;
    
    while (1) {
        char messBuf[messBufMaxSize];
        int mess = recv(sockid, messBuf, 5, 0);
        if (mess == 0) {
            printf("Server break work\n");
            close(sockid);
            exit(0);
            return NULL;
        }
        mess = recv(sockid, messBuf + 5, sizeOfMes(messBuf), 0);
        struct Message parsMes;
        Pars(messBuf, &parsMes);
        switch (parsMes.type) {
            case 'r':
                if (parsMes.k != 3) {
                    printf("Error in regular message\n");
                c   ontinue;
                }
                PrintTime(parsMes.strings[0]);
                printf("%s: %s\n", parsMes.strings[1], parsMes.strings[2]);
                break;
            case 'm':
                if (parsMes.k != 2) {
                    printf("Error in server message\n");
                    continue;
                }
                PrintTime(parsMes.strings[0]);
                printf("--- %s ---\n", parsMes.strings[1]);
                break;
            case 'h':
                if (parsMes.k != 3) {
                    printf("Error in history message\n");
                    continue;
                }
                printf("History: ");
                PrintTime(parsMes.strings[0]);
                printf("%s: %s\n", parsMes.strings[1], parsMes.strings[2]);
                break;
                return NULL;
                break;
            case 'l':
                if (parsMes.k != 1) {
                    printf("Error in list message\n");
                    continue;
                }
                printf("--- %s is online ---\n", parsMes.strings[0]);
                break;
            case 'k':
                if (parsMes.k != 1) {
                    printf("Error in kick message\n");
                    continue;
                }
                printf("--- %s ---\n", parsMes.strings[0]);
                break;
            default:
                printf("Unknown message\n");
                break;
        }
    } 
}

void RMesSend(char* newMes) {
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'r';
    notif.k = 1;
    notif.strings[0][0] = 0;
    strcpy(notif.strings[0], text);
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
}

void IMesSend(char* newMes) {
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'i';
    notif.k = 2;
    char* s = malloc(messBufMaxSize);
    s[0] = 0;
    strcpy(s, text);
    notif.strings[0] = s;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    notif.strings[1] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    free(s);
}

void OMesSend(char* newMes) {
    struct Message notif;
    notif.type = 'o';
    notif.k = 0;
    notif.length = 0;
    MakeMes(newMes, &notif);
}

void HMesSend(char* newMes) {
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'h';
    notif.k = 1;
    notif.strings[0] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
}

void LMesSend(char* newMes) {
    struct Message notif;
    notif.type = 'l';
    notif.k = 0;
    notif.length = 0;
    MakeMes(newMes, &notif);
}

void KMesSend(char* newMes) {
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'k';
    notif.k = 2;
    char* s = malloc(messBufMaxSize);
    s[0] = 0;
    strcpy(s, text);
    notif.strings[0] = s;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    notif.strings[1] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    free(s);
}

void clientProcess(int sockid)
{
    int r;
    size_t len = 0;
    char* text = NULL;
    while ((r = getline(&text, &len, stdin)) != -1) {
        text[r - 1] = 0;
        if (!strcmp(text, "--regular")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            RMesSend(newMes);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            continue;
        } if (!strcmp(text, "--login")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            IMesSend(newMes);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            continue;
        } if (!strcmp(text, "--logout")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            OMesSend(newMes);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            continue;
        } if (!strcmp(text, "--history")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            HMesSend(newMes);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            continue;
        } if (!strcmp(text, "--list")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            LMesSend(newMes);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            continue;
        } if (!strcmp(text, "--kick")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            KMesSend(newMes);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            continue;
        }
        printf("Incorrect message\n");
    }
    return;
}

int main(int argc, char* argv[])
{
    int portNum = 1337;
    char* ip = malloc(50);
    strcpy(ip, "127.0.0.1");
    int option;
    while ((option = getopt(argc, argv, "-p-i")) != -1) {
        if (option == 'p') {
            portNum = atoi(optarg);
        } else if (option == 'i') {
            strcpy(ip, optarg);
        }
    }
    int sockid;
    if ((sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        fprintf(stderr, "Socket opening failed\n");
        return 1;
    }
    struct sockaddr_in port;
    port.sin_family = AF_INET;
    port.sin_port = htons((uint16_t)portNum);
    port.sin_addr.s_addr = inet_addr(ip);
    free(ip);
    if (connect(sockid, (struct sockaddr *) &port, sizeof(port)) == -1) {
        fprintf(stderr, "Server connecting failed\n");
        return 2;
    }
    pthread_t t;
    pthread_create(&t, NULL, serverProcess, (void *)&sockid);
    clientProcess(sockid);
    return 0;
}
