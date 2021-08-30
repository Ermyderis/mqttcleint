#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>
#include "getvaluesconfig.h"
#include "database.h"
#include "usr_pasw_tsl.h"

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



sqlite3 *data_base;

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    struct Node *ptr = obj;
    printf("Connect\n");
	if(rc) {
		printf("Error with result code: %d\n", rc);
        syslog(LOG_ERR, "Error with result code: %d\n", rc);
	}
    else{
        while(ptr->datatopics != NULL) 
        {
            printf("%s:\n", ptr->datatopics);
            mosquitto_subscribe(mosq, NULL, ptr->datatopics, 0);
            ptr = ptr->next;
        }
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    char *errMSG = NULL;
    const char *table_query = "CREATE TABLE Messages ( \ id integer primary key autoincrement not null,\ Topic varchar(250),\ Message varchar(250),\ Time timestamp default (datetime('now', 'localtime')) not null);";
    char  *query;
    sqlite3_exec(data_base, table_query, 0, NULL, errMSG);     //create database table 
    if (errMSG != NULL)
        syslog(LOG_WARNING ,"Failed to insert data into database\n"); 
    query = sqlite3_mprintf("INSERT INTO Messages(Topic, Message) values ('%q', '%q');",(char *) msg->topic, (char *) msg->payload);
    sqlite3_exec(data_base, query, 0, NULL, errMSG);
    sqlite3_free(query);
    if (errMSG != NULL){
        printf("Failed to insert data into database\n");
        syslog(LOG_ERR, "Failed to insert data into database\n");
    }
}
