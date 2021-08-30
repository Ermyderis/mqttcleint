#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>
#include "getvaluesconfig.h"

#define CRTFILE "/usr/share/mqttappcrt/mosquitto.org.crt"

struct Node {
   char *datatopics;
   struct Node *next;
};

struct Config{
    char *port;
    char *address;
    char *ussername;
    char *password;
    char *tsl;
};

int set_ussername_password_tsl(struct mosquitto *mosq, struct Config *configdata){
    int rc = 0;
    if(configdata->tsl != NULL){
        if(configdata->password != NULL && configdata->ussername != NULL){
                if (mosquitto_username_pw_set(mosq, configdata->ussername ,configdata->password)==MOSQ_ERR_SUCCESS){
                syslog(LOG_INFO, "User name and password added successfuly\n");
                printf("User name and password added successfuly\n");
                rc = 0;
            }else{
                syslog(LOG_ERR, "Failed to add username or password\n");
                printf("Failed to add username or password\n");
                rc = -1;
            }
        }
        else{
            printf("Empty usr or pasw\n");
            rc = -1;
        }
    }
    //use tsl
    if(configdata->tsl != NULL){
        if (!mosquitto_tls_set(mosq,CRTFILE,NULL, NULL, NULL, NULL)== MOSQ_ERR_SUCCESS){ 
                        syslog(LOG_ERR, "Failed to set tls"); 
                        printf("Failed to set tls\n");
                    }
        else{
            syslog(LOG_INFO, "Tls seted\n"); 
            printf("Tls seted\n");
        }
    }

    return rc;
}
