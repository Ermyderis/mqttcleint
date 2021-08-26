#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>

#define configfile "/etc/config/mqttconfig"


struct Node {
   char *data;
   struct Node *next;
};

struct Config{
    char *port;
    char *address;
    char *ussername;
    char *password;
    char *tsl;
};

//functions used to remove single quotes
void remov_unvanted_character(char* string, char character)
{
    int j, n = strlen(string);
    for (int i = j = 0; i < n; i++)
        if (string[i] != character)
            string[j++] = string[i];
 
    string[j] = '\0';
}


void get_topics(struct Node **head, struct Node *current){
    FILE *fp;
    int errnum;
    char *pos = NULL;
    char line[128];
    int linenum = 1;

    fp = fopen(configfile, "r");
    if(fp == NULL){
        errnum = errno;
        perror("Error: ");
    }
    else{
        while(fgets(line, sizeof(line), fp) != NULL){
            char firstColumn[256], secondColumn[256], thirdColumn[256];
            linenum++;
            if(sscanf(line, "%s%s%s", firstColumn, secondColumn, thirdColumn) != 3){
                    //fprintf(fp, "\nSyntax error, line%d\n", linenum);
            }
            else{
                pos = strstr(secondColumn, "topic");
                if(pos != NULL){
                struct Node *new = (struct Node *)malloc(sizeof(struct Node));
                remov_unvanted_character(thirdColumn, '\'');
                new->data = strdup(thirdColumn);
                if(*head)
                    new->next = *head;
                *head = new;
                }
            }
        }
        fclose(fp);
    }
}

struct Config Get_values_from_config(){
    struct Config configdata;
    FILE *fp;
    int errnum;
    char *pos = NULL;
    char line[128];
    int linenum = 1;
    configdata.port = NULL;
    configdata.address = NULL;
    configdata.ussername = NULL;
    configdata.password = NULL;
    configdata.tsl = NULL;

    fp = fopen(configfile, "r");
    if(fp == NULL){
        errnum = errno;
        perror("Error: ");
    }
    else{
        while(fgets(line, sizeof(line), fp) != NULL){
            char firstColumn[256], secondColumn[256], thirdColumn[256];
            linenum++;
            if(sscanf(line, "%s%s%s", firstColumn, secondColumn, thirdColumn) != 3){
                    //fprintf(fp, "\nSyntax error, line%d\n", linenum);
            }
            else{
                pos = strstr(secondColumn, "port");
                if(pos != NULL){
     
                    remov_unvanted_character(thirdColumn, '\'');
                    configdata.port = strdup(thirdColumn); 
                }
                else{
                    pos = strstr(secondColumn, "address");
                    if(pos != NULL){
                        remov_unvanted_character(thirdColumn, '\'');
                        configdata.address = strdup(thirdColumn);  
                    }
                    else{
                        pos = strstr(secondColumn, "username");
                        if(pos != NULL){
                            remov_unvanted_character(thirdColumn, '\'');
                            configdata.ussername = strdup(thirdColumn);
                        }
                        else{
                            pos = strstr(secondColumn, "password");
                            if(pos != NULL){
                                remov_unvanted_character(thirdColumn, '\'');
                                configdata.password = strdup(thirdColumn);
                            }
                            else{
                                pos = strstr(secondColumn, "enabletsl");
                                if(pos != NULL){
                                    remov_unvanted_character(thirdColumn, '\'');
                                    configdata.tsl = strdup(thirdColumn);
                                }
                            }
                        }
                    }
                }
            }
        }
        fclose(fp);
    }
    return configdata;
}
