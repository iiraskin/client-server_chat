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
#define maxOneLoginNum 5


struct User
{
   char* login;
   char* password;
   int* sockid;
   int isOnline;
   int sockidLen;
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

void Pars(char* mes, struct Message* parsMes)
{
    int a = 0;
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
        parsMes->strings[k] = malloc(parsMes->stringLength[k] + 1);
        parsMes->strings[k][0] = 0;
        for (j; j <  parsMes->stringLength[k]; j++) {
            parsMes->strings[k][j] = mes[i];
            i++;
        }
        parsMes->strings[k][j] = 0;
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
        j += 4;
        int a = 0;
        for (a; a < parsMes->stringLength[k]; a++) {
            mes[j] = parsMes->strings[k][a];
            j++;
        }
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
    notif.strings[0] = malloc(18);
    long t1 = (size_t)tv.tv_sec;
    long t2 = (size_t)tv.tv_usec;
    int a = 3;
    for (a; a >= 0; a--) {
        notif.strings[0][a] = (char)(t1 % 256);
        t1 /= 256;
    }
    a = 7;
    for (a; a >= 3; a--) {
        notif.strings[0][a] = (char)(t2 % 256);
        t2 /= 256;
    };
    notif.strings[1] = text;
    notif.stringLength[0] = 8;
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    free(notif.strings[0]);
    return;
}

void SMesMake(char* newMes, int text)
{
    struct Message notif;
    notif.type = 's';
    notif.k = 1;
    notif.strings[0] = malloc(messBufMaxSize);
    notif.strings[0][0] = 0;
    int a = 3;
    for (a; a >= 0; a--) {
        notif.strings[0][a] = (char)(text % 256);
        text /= 256;
    }
    notif.stringLength[0] = 4;
    notif.length = notif.stringLength[0] + 4;
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
    notif.strings[0] = malloc(18);
    long t1 = (size_t)tv.tv_sec;
    long t2 = (size_t)tv.tv_usec;
    int a = 3;
    for (a; a >= 0; a--) {
        notif.strings[0][a] = (char)(t1 % 256);
        t1 /= 256;
    }
    a = 7;
    for (a; a >= 3; a--) {
        notif.strings[0][a] = (char)(t2 % 256);
        t2 /= 256;
    }
    int i = 0, j = 0;
    for (i; i < usersLen; i++) {
        j = 0;
        for (j; j < Users[i].sockidLen; j++) {
            if (Users[i].sockid[j] == sockid) {
                notif.strings[1] = Users[i].login;
                break;
            }
        }
    }
    notif.strings[2] = text;
    notif.stringLength[0] = 8;
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
    notif.strings[0][0] = 0;
    strcpy(notif.strings[0], text);
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
    free(notif.strings[0]);
    return;
}

void KMesMake(char* newMes, char* text) {
    struct Message notif;
    notif.type = 'k';
    notif.k = 1;
    notif.strings[0] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
    return;
}

void IMess(int sockid, struct Message *parsMes, pthread_mutex_t* mut) {
    if (parsMes->k != 2) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        SMesMake(newMes, 6);
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
    int i = 0, j = 0;
    
    for (i; i < usersLen; ++i) {
        for (j; j < Users[i].sockidLen; ++j ) {
            if (sockid == Users[i].sockid[j] && Users[i].isOnline == 1) {
                char* newMes;
                newMes = malloc(messBufMaxSize);
                SMesMake(newMes, 3);
                send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                MMesMake(newMes, "You already logined");
                send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                free(newMes);
                free(newLogin);
                free(newPassword);
                return;
            }
        }
        if (strcmp(newLogin, Users[i].login) == 0) {
            if (strcmp(newPassword, Users[i].password) == 0) {
                char* newMes;
                newMes = malloc(messBufMaxSize);
                if (Users[i].isOnline == 0) {
                    pthread_mutex_lock(mut);
                    Users[i].isOnline = 1;
                    Users[i].sockid = (int*)malloc(maxOneLoginNum * sizeof(int));
                    Users[i].sockid[0] = sockid;
                    Users[i].sockidLen = 1;
                    SMesMake(newMes, 0);
                    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                    MMesMake(newMes, strcat(newLogin, " came"));
                    int a = 0, b = 0;
                    for (a; a < usersLen; a++) { 
                        if (Users[a].isOnline == 1) {
                            b = 0;
                            for (b; b < Users[a].sockidLen; b++) {
                                b = 0;
                                send(Users[a].sockid[b], newMes, sizeOfMes(newMes) + 5, 0);
                            }
                        }
                    }
                    pthread_mutex_unlock(mut);
                } else {
                    pthread_mutex_lock(mut);
                    Users[i].sockid[Users[i].sockidLen] = sockid;
                    Users[i].sockidLen++;
                    SMesMake(newMes, 0);
                    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                    MMesMake(newMes, "You logined another time");
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
            SMesMake(newMes, 4);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
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
    newUser.sockid = (int*)malloc(maxOneLoginNum * sizeof(int));
    newUser.sockid[0] = sockid;
    newUser.sockidLen = 1;
    Users[usersLen] = newUser;
    usersLen++;

    char* newMes;
    newMes = malloc(messBufMaxSize);
    SMesMake(newMes, 0);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
    MMesMake(newMes, strcat(newLogin, " joined"));
    pthread_mutex_lock(mut);
    int a = 0, b = 0;
    for (a; a < usersLen; a++) { 
        if (Users[a].isOnline == 1) {
            b = 0;
            for (b; b < Users[a].sockidLen; b++) {
                send(Users[a].sockid[b], newMes, sizeOfMes(newMes) + 5, 0);
            }
        }
    }
    pthread_mutex_unlock(mut);
    free(newMes);
    free(newLogin);
    free(newPassword);
    return;
}

void OMess(int sockid, pthread_mutex_t* mut) {
    int i = 0, j = 0;
    for (i; i < usersLen; ++i ) {
        j = 0;
        for (j; j < Users[i].sockidLen; j++) {
            if (Users[i].sockid[j] == sockid) {
                char* newMes;
                newMes = malloc(messBufMaxSize);
                SMesMake(newMes, 0);
                send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                pthread_mutex_lock(mut);
                Users[i].sockid[Users[i].sockidLen - 1] = Users[i].sockid[j];
                Users[i].sockidLen--;
                if (Users[i].sockidLen == 0) {
                    Users[i].isOnline = 0;
                    free(Users[i].sockid);;
                    char* loginCopy = malloc(maxSizeOfLogin);
                    strcpy(loginCopy, Users[i].login);
                    MMesMake(newMes, strcat(loginCopy, " logout"));
                    int a = 0, b = 0;
                    for (a; a < usersLen; a++) { 
                        if (Users[a].isOnline == 1) {
                            b = 0;
                            for (b; b < Users[a].sockidLen; b++) {
                                send(Users[a].sockid[b], newMes, sizeOfMes(newMes) + 5, 0);
                            }
                        }
                    }
                    free(loginCopy);
                }
                free(newMes);
                pthread_mutex_unlock(mut);
                return;
            }
        }
    }
    char* newMes;
    newMes = malloc(messBufMaxSize);
    SMesMake(newMes, 3);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void RMess(int sockid, struct Message *parsMes, pthread_mutex_t* mut) {
    if (parsMes->k != 1) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        SMesMake(newMes, 6);
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        free(newMes);
        return;
    }
    char* newMes;
    newMes = malloc(messBufMaxSize);
    RMesMake(newMes, sockid, parsMes->strings[0]);
    pthread_mutex_lock(mut);
    int a = 0, b = 0;
    for (a; a < usersLen; a++) { 
        if (Users[a].isOnline == 1) {
            b = 0;
            for (b; b < Users[a].sockidLen; b++) {
                send(Users[a].sockid[b], newMes, sizeOfMes(newMes) + 5, 0);
            }
        }
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
        SMesMake(newMes, 6);
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        free(newMes);
        return;
    }
    int i = 0;
    int j = 0;
    time_t timeInt = 0;
    for (j; j < 4; j++) {
        i = i * 256 + (parsMes->strings[0][j] + 256) % 256;
    }
    pthread_mutex_lock(mut);
    for(i; i > 0; i--) {
        if (hisBuffer[((bufPointer - i) + bufSize) % bufSize][0] == 0) {
            continue;
        }
        send(sockid, hisBuffer[((bufPointer - i) + bufSize) % bufSize], 
             sizeOfMes(hisBuffer[((bufPointer - i) + bufSize) % bufSize]) + 5, 0);
    }
    pthread_mutex_unlock(mut);
    return;
}

void LMess(int sockid, struct Message *parsMes) {
    if (parsMes->k != 0) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        SMesMake(newMes, 6);
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
        SMesMake(newMes, 6);
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        free(newMes);
        return;
    }
    int a = 0;
    for (a; a < Users[0].sockidLen; a++) {
        if (Users[0].sockid[a] == sockid) {
            a = -1;
            break;
        }
    }
    if (a != -1) {
        char* newMes;
        newMes = malloc(messBufMaxSize);
        SMesMake(newMes, 5);
        send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
        free(newMes);
        return;
    }
    int i = 0;
    for (i; i < usersLen; i++) {
        if (strcmp(parsMes->strings[0], Users[i].login) == 0) {
            pthread_mutex_lock(mut);
            char* newMes1;
            newMes1 = malloc(messBufMaxSize);
            KMesMake(newMes1, parsMes->strings[1]);
            int b = 0;
            for (b; b < Users[i].sockidLen; b++) {
                send(Users[i].sockid[b], newMes1, sizeOfMes(newMes1) + 5, 0);
            }
            Users[i].isOnline = 0;
            b = 0;
            for (b; b < Users[i].sockidLen; b++) {
                close(Users[i].sockid[b]);
            }
            free(Users[i].sockid);
            Users[i] = Users[usersLen - 1];
            usersLen--;
            a = 0;
            b = 0;
            char* newMes = malloc(messBufMaxSize);
            char* text = malloc(messBufMaxSize);
            SMesMake(newMes, 0);
            send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
            text[0] = 0;
            sprintf(text, "%s was kicked because '%s'", parsMes->strings[0], parsMes->strings[1]);
            MMesMake(newMes, text);
            for (a; a < usersLen; a++) { 
                if (Users[a].isOnline == 1) {
                    b = 0;
                    for (b; b < Users[a].sockidLen; b++) {
                        send(Users[a].sockid[b], newMes, sizeOfMes(newMes) + 5, 0);
                    }
                }
            }
            pthread_mutex_unlock(mut);
            free(newMes);
            return;
        }
    }
    char* newMes;
    newMes = malloc(messBufMaxSize);
    SMesMake(newMes, 2);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
    MMesMake(newMes, strcat(parsMes->strings[0], " - no such user"));
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
    free(newMes);
    return;
}

void* userProcess(void * data) {
    struct UserProcDate newUserProcDate = *((struct UserProcDate*)data);
    int sockid = newUserProcDate.sockid;
    pthread_mutex_t* mut = newUserProcDate.mut;

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
            int a = 0, b = 0;
            for (a; a < usersLen; a++) {
                b = 0;
                for (b; b < Users[a].sockidLen; b++) {
                    if (Users[a].sockid[b] == sockid && Users[a].isOnline == 1) {
                        a = -1;
                        break;
                    }
                }
                if (a == -1) {
                    break;
                }
            }
            if (a != -1) {
                char* newMes = malloc(messBufMaxSize);
                SMesMake(newMes, 2);
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
                {
                    char* newMes = malloc(messBufMaxSize);
                    SMesMake(newMes, 1);
                    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
                    free(newMes);
                    break;
                }
            }
        }
        int a = 0;
        for(a; a < parsMes.k; a++) {
            free(parsMes.strings[a]);
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