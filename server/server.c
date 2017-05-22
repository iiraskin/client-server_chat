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


#define maxUsersNum 10
#define maxConnectNum 15
#define maxSizeOfLogin 31
#define maxSizeOfPassword 31
#define bufSize 50
#define messBufMaxSize 7000

struct User
{
   char* login;
   char* password;
   int sockid;
   int isOnline;
};

struct User Users[maxUsersNum];
int usersLen;

char* hisBuffer[bufSize];
int bufPointer;

struct UserProcDate
{
    int sockid;
    pthread_mutex_t* mut;
};

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

void MMesMake(char* newMes, char* text)
{
    struct Message notif;
    notif.type = 'm';
    notif.k = 2;
    struct timeval tv;
    gettimeofday(&tv, NULL);    
    notif.strings[0] = malloc(16);
    sprintf(notif.strings[0],"%ld$%ld", (size_t)tv.tv_sec, (size_t)tv.tv_usec);
    notif.strings[1] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    free(notif.strings[0]);
    return;
}

void RMesMake(char* newMes, int sockid, char* text)
{
    struct Message notif;
    notif.type = 'r';
    notif.k = 3;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    notif.strings[0] = malloc(16);
    sprintf(notif.strings[0],"%ld$%ld", (size_t)tv.tv_sec, (size_t)tv.tv_usec);
    int i = 0;
    for (i; i < usersLen; i++) {
        if (Users[i].sockid == sockid) {
            notif.strings[1] = Users[i].login;
            break;
        }
    }
    notif.strings[2] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.stringLength[2] = strlen(notif.strings[2]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + notif.stringLength[2] + 12;
    MakeMes(newMes, &notif);
    free(notif.strings[0]);
    return;
}

void LMesMake(char* newMes, char* text) {
    struct Message notif;
    notif.type = 'l';
    notif.k = 1;
    notif.strings[0] = malloc(messBufMaxSize);
    strcpy(notif.strings[0], text);
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
    free(notif.strings[0]);
    return;
}

void KMesMake(char* newMes, char* login, char* text) {
    struct Message notif;
    notif.type = 'k';
    notif.k = 2;
    notif.strings[0] = login;
    notif.strings[1] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    return;
}

void IMess(int sockid, struct Message *parsMes, pthread_mutex_t* mut) {
    if (parsMes->k != 2) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        MMesMake(newMes, "Error 6");
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        free(newMes);
        return;
    }
    
    char* newLogin = malloc(maxSizeOfLogin);
    newLogin[0] = 0;
    newLogin = strcat(newLogin, parsMes->strings[0]);
    char* newPassword = malloc(maxSizeOfPassword);
    newPassword[0] = 0;
    newPassword = strcat(newPassword, parsMes->strings[1]);
    int i = 0;
    
    for (i; i < usersLen; ++i ) {
        if (sockid == Users[i].sockid) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            MMesMake(newMes, "You already logined");
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            free(newLogin);
            free(newPassword);
        }
        if (strcmp(newLogin, Users[i].login) == 0) {
            if (strcmp(newPassword, Users[i].password) == 0) {
                char* newMes;
                newMes = malloc(messBufMaxSize);
                if (Users[i].isOnline == 0) {
                    Users[i].isOnline = 1;
                    Users[i].sockid = sockid;
                    MMesMake(newMes, strcat(newLogin, " came"));
                    pthread_mutex_lock(mut);
                    int a = 0;
                    for (a; a < usersLen; a++) {;
                        if (Users[a].isOnline == 1) {
                            send(Users[a].sockid, newMes, sizeOfMes(newMes) + 5, 0);
                        }
                    }
                    pthread_mutex_unlock(mut);
                } else {
                    MMesMake(newMes, strcat(newLogin, " already online"));
                    pthread_mutex_lock(mut);
                    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                    pthread_mutex_unlock(mut);
                }
                free(newMes);
                free(newLogin);
                free(newPassword);
                return;
            }

            char* newMes;
            newMes = malloc(messBufMaxSize);
            MMesMake(newMes, strcat(newLogin, " has already used"));
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
            free(newLogin);
            free(newPassword);
            return;
        }
    }
    struct User newUser;
    newUser.login = malloc(maxSizeOfLogin);
    newUser.login[0] = 0;
    newUser.login = strcat(newUser.login, newLogin);
    newUser.password = malloc(maxSizeOfPassword);
    newUser.password[0] = 0;
    newUser.password = strcat(newUser.password, newPassword);
    newUser.isOnline = 1;
    newUser.sockid = sockid;
    Users[usersLen] = newUser;
    usersLen++;

    char* newMes;
    newMes = malloc(messBufMaxSize);
    MMesMake(newMes, strcat(newLogin, " joined"));
    pthread_mutex_lock(mut);
    int a = 0;
    for (a; a < usersLen; a++) {
        if (Users[a].isOnline == 1) {
            send(Users[a].sockid, newMes, sizeOfMes(newMes) + 5, 0);
        }
    }
    pthread_mutex_unlock(mut);
    free(newMes);
    free(newLogin);
    free(newPassword);
    return;
}

void OMess(int sockid, pthread_mutex_t* mut) {
    int i = 0;
    for (i; i < usersLen; ++i ) {
        if (sockid == Users[i].sockid) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            char* loginCopy = malloc(maxSizeOfLogin);
            strcpy(loginCopy, Users[i].login);
            MMesMake(newMes, strcat(loginCopy, " logout"));
            pthread_mutex_lock(mut);
            int a = 0;
            for (a; a < usersLen; a++) {
                if (Users[a].isOnline == 1) {
                    if (Users[a].isOnline == 1) {
                        send(Users[a].sockid, newMes, sizeOfMes(newMes) + 5, 0);
                    }
                }
            }
            Users[i].isOnline = 0;
            pthread_mutex_unlock(mut);
            free(loginCopy);
            free(newMes);
            return;
        }
    }
}

void RMess(int sockid, struct Message *parsMes, pthread_mutex_t* mut) {
    if (parsMes->k != 1) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        MMesMake(newMes, "Error 6");
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        return;
    }
    char* newMes;
    newMes = malloc(messBufMaxSize);
    RMesMake(newMes, sockid, parsMes->strings[0]);
    pthread_mutex_lock(mut);
    int a = 0;
    for (a; a < usersLen; a++) {
        send(Users[a].sockid, newMes, sizeOfMes(newMes) + 5, 0);
    }
    hisBuffer[bufPointer][0] = 'h';
    a = 1;
    for (a; a < sizeOfMes(newMes) + 5; a++) {
        hisBuffer[bufPointer][a] = newMes[a];
    }
    bufPointer = (bufPointer + 1) % bufSize;
    pthread_mutex_unlock(mut);
    free(newMes);
    return;
}

void HMess(int sockid, struct Message *parsMes, pthread_mutex_t* mut) {
    if (parsMes->k != 1) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        MMesMake(newMes, "Error 6");
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        return;
    }
    int count = atoi(parsMes->strings[0]) % bufSize;
    int i = count;
    pthread_mutex_lock(mut);
    for(i; i > 0; i--) {
        if (hisBuffer[(bufPointer - i) % bufSize][0] == 0) {
            break;
        }
        send(sockid, hisBuffer[(bufPointer - i) % bufSize], 
             sizeOfMes(hisBuffer[(bufPointer - i) % bufSize]) + 5, 0);
    }
    pthread_mutex_unlock(mut);
    return;
}

void LMess(int sockid, struct Message *parsMes) {
    if (parsMes->k != 0) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        MMesMake(newMes, "Error 6");
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        free(newMes);
        return;
    }
    int i = 0;
    for(i; i < usersLen; i++) {
        if (Users[i].isOnline == 1) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            LMesMake(newMes, Users[i].login);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            free(newMes);
        }
    }
    return;
}

void KMess(int sockid, struct Message *parsMes, pthread_mutex_t* mut) {
    if (parsMes->k != 2) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        MMesMake(newMes, "Error 6");
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        return;
    }
    if (Users[0].sockid != sockid) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        MMesMake(newMes, "Error 5");
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        return;
    }
    int i = 0;
    for (i; i < usersLen; i++) {
        if (strcmp(parsMes->strings[0], Users[i].login) == 0) {
            pthread_mutex_lock(mut);
            char* newMes;
            newMes = malloc(messBufMaxSize);
            KMesMake(newMes, parsMes->strings[0], parsMes->strings[1]);
            int a = 0;
            for (a; a < usersLen; a++) {
                if (Users[a].isOnline == 1) {
                    if (Users[a].isOnline == 1) {
                        send(Users[a].sockid, newMes, sizeOfMes(newMes) + 5, 0);
                    }
                }
            }
            
            close(Users[i].sockid);
            Users[i] = Users[usersLen - 1];
            pthread_mutex_unlock(mut);
            free(newMes);
            return;
        }
    }
    char* newMes;
    newMes = malloc(messBufMaxSize);
    MMesMake(newMes, strcat(parsMes->strings[0], " - no such user"));
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
    free(newMes);
    return;
}

void* userProcess(void * data) {
    struct UserProcDate newUserProcDate = *((struct UserProcDate*)data);
    int sockid = newUserProcDate.sockid;
    pthread_mutex_t* mut = newUserProcDate.mut;
    char* newMes;

    while (1) {
        char messBuf[messBufMaxSize];
        int mess = recv(sockid, messBuf, 5, 0);
        if (mess <= 0) { 
            OMess(sockid, mut);
            close(sockid);
            return NULL;
        }
        int size = sizeOfMes(messBuf);
        if (size != 0) {
            mess = recv(sockid, messBuf + 5, size, 0);
        }
        struct Message parsMes;
        Pars(messBuf, &parsMes);
        if (parsMes.type == 'i') {;
            IMess(sockid, &parsMes, mut);
        } else {
            int a;
            for (a; a < usersLen; a++) {
                if (Users[a].sockid == sockid && Users[a].isOnline == 1) {
                    a = -1;
                    break;
                }
            }
            if (a != -1) {
                char* newMes;
                newMes = malloc(messBufMaxSize);
                MMesMake(newMes, "Login, please");
                send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                free(newMes);
                continue;
            }
            switch (parsMes.type) {
                case 'r':
                    RMess(sockid, &parsMes, mut);
                    break;
                case 'i':
                    IMess(sockid, &parsMes, mut);
                    break;
                case 'o':
                    OMess(sockid, mut);
                    break;
                case 'h':
                    HMess(sockid, &parsMes, mut);
                    break;
                case 'l':
                    LMess(sockid, &parsMes);
                    break;
                case 'k':
                    KMess(sockid, &parsMes, mut);
                    break;
                default:
                        break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    usersLen = 1;
    bufPointer = 0;
    
    int a = 0;
    for (a; a < bufSize; a++) {
        hisBuffer[a] = malloc(messBufMaxSize);
        hisBuffer[a][0] = 0;
    }
    
    char option;
    
    int portNum = 1337;
    while ((option = getopt(argc, argv, "-p")) != -1) {
        if (option == 'p') {
            portNum = atoi(optarg);
        }
    }    

    pthread_mutex_t mut;
    if (pthread_mutex_init(&mut, NULL) != 0) {
        printf("mutex init failed\n");
        return 1;
    }

    int sockid = socket(PF_INET, SOCK_STREAM, 0);
    if (sockid == -1) {
        fprintf(stderr, "Socket opening failed.\n");
        return 1;
    }

    struct sockaddr_in port;
    port.sin_family = AF_INET;
    port.sin_port = htons((uint16_t)portNum);
    port.sin_addr.s_addr = htonl(INADDR_ANY);
    int status = bind(sockid, (struct sockaddr *) &port, sizeof(port));
    if (status == -1) {
        fprintf(stderr, "Socket binding failed.\n");
        return 2;
    }

    status = listen(sockid, maxConnectNum);
    if (status == -1) {
        fprintf(stderr, "Socket listening failed\n");
        return 3;
    }

    Users[0].login = alloca(maxSizeOfLogin);
    Users[0].login = "ilya";
    Users[0].isOnline = 0;
    printf("Input password\n");
    size_t len = 0;
    int r;
    Users[0].password = alloca(maxSizeOfPassword);
    while ( (r = getline(&Users[0].password, &len, stdin)) == -1) {
	    fprintf(stderr, "Error. Try once more\n");
    }
    Users[0].password[r - 1] = 0;

    while (1) {
        struct sockaddr client_addr;
        int addr_len = sizeof(client_addr);
        int newSock = accept(sockid, &client_addr, &addr_len);
        if (newSock == -1) {
            fprintf(stderr, "Socket accepting failed\n");
            return 4;
        }

        pthread_mutex_lock(&mut);
        struct UserProcDate newUserProcDate;
        newUserProcDate.sockid = newSock;
        newUserProcDate.mut = &mut;
        pthread_t t;
        int b = pthread_create(&t, NULL, userProcess, (void *)&newUserProcDate);
        if (b) {
            fprintf(stderr, "Thread creating error\n");
        }
        pthread_mutex_unlock(&mut);
    }
    
    return 0;
}