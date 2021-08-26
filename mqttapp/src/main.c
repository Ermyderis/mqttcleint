#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>
#include "getvalues.h"


#define databasefile "/log/mqttmessages.db"
#define crtfile "/usr/share/mqttappcrt/mosquitto.org.crt"

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

void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

volatile int interrupt = 0;
sqlite3 *data_base = NULL;


void signal_handler(int signo) {
    signal(SIGINT, NULL);
    syslog(LOG_INFO, "Received signal: %d", signo);
    closelog();
    interrupt = 1;
}


int main(void)
{
    int errnum;
    int rc;
	struct sigaction action;
    struct Config configdata;
    struct mosquitto *mosq;
    struct Node *tmp;
    struct Node *head = NULL;
    struct Node *current = NULL;

    
	memset(&action, 0, sizeof(struct sigaction));
    memset(&configdata, 0, sizeof(struct Config));

	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    sigaction(SIGTERM, &action, NULL);

    get_topics(&head, current); 
   

    configdata = Get_values_from_config();
    if((configdata.address == NULL) || (configdata.port == NULL)){
        goto endOfTheProgram;
    }
    
    //log opening
    openlog("mqttapp", LOG_PID, LOG_USER);
    //open database
    if (sqlite3_open(databasefile, &data_base) != 0){
        printf("Failed to open databse\n");
        syslog(LOG_ERR, "Failed to open databse\n");
        goto endOfTheProgram;
    }
    else{
        printf("Database opened\n");
    }
    //Initialize mosquitto
	rc = mosquitto_lib_init();
    if (rc != 0){
        syslog(LOG_ERR, "Failed to initialize mosquitto");
        goto endOfTheProgram;
    }
    else{
        syslog(LOG_INFO, "Mosquitto initialize");
    }

    mosq = mosquitto_new("subscribe-test", true, head);
    //check if tsl not equal NULL and if pasw and usr fields are not empty
    if(configdata.tsl != NULL){
        if(configdata.password != NULL && configdata.ussername != NULL){
                if (mosquitto_username_pw_set(mosq, configdata.ussername ,configdata.password)==MOSQ_ERR_SUCCESS){
                syslog(LOG_INFO, "User name and password added successfuly\n");
                printf("User name and password added successfuly\n");
            }else{
                syslog(LOG_WARNING, "Failed to add username or password\n");
                printf("Failed to add username or password\n");
                goto endOfTheProgram;
            }
        }
        else{
            printf("Empty usr or pasw\n");
            goto endOfTheProgram;
        }
    }

    //use tsl
    if(configdata.tsl != NULL){
        if (!mosquitto_tls_set(mosq,crtfile,NULL, NULL, NULL, NULL)== MOSQ_ERR_SUCCESS){ 
                        syslog(LOG_WARNING, "Failed to set tls"); 
                        printf("Failed to set tls\n");
                    }
        else{
            syslog(LOG_INFO, "Tls seted"); 
                        printf("Tls seted\n");
        }
    }
    
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
    //Conecting to broker
	rc = mosquitto_connect(mosq, configdata.address, atoi(configdata.port), 10);
	if(rc) {
		printf("Could not connect to Broker with return code %d\n", rc);
        syslog(LOG_ERR, "Could not connect to Broker with return code %d\n", rc);
		goto endOfTheProgram;
	}
    mosquitto_loop_start(mosq);
	while(!interrupt) {   
        
	}
    printf("Closing\n");
    mosquitto_loop_stop(mosq, true);

    endOfTheProgram:
    printf("Closing\n");
    //free memory
    //cleaning list
    while(head != NULL) { 
        free(head->data);                
        tmp= head;
        head = head->next;
        free (tmp);
    }
    mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
    free(configdata.port);
    free(configdata.address);
    free(configdata.ussername);
    free(configdata.password);
    free(configdata.tsl);
    sqlite3_close(data_base);
    closelog();
	return rc;
}
///////////// Main end

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    struct Node *ptr = obj;
    printf("Connect\n");
	if(rc) {
		printf("Error with result code: %d\n", rc);
        syslog(LOG_ERR, "Error with result code: %d\n", rc);
	}
    else{
        while(ptr->data != NULL) 
        {
            printf("%s:\n", ptr->data);
            mosquitto_subscribe(mosq, NULL, ptr->data, 0);
            ptr = ptr->next;
        }
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    char *errMSG = NULL;
    const char *table_query = "CREATE TABLE Messages ( \ id integer primary key autoincrement not null,\ Topic varchar(250),\ Message varchar(250),\ Time timestamp default (datetime('now', 'localtime')) not null);";
    char  *query;
    sqlite3_exec(data_base, table_query, 0, NULL, errMSG);     //creates database table if there is none
    if (errMSG != NULL)
        syslog(LOG_WARNING ,"Failed to insert data into database\n"); 
    query = sqlite3_mprintf("INSERT INTO Messages(Topic, Message) values ('%q', '%q');",(char *) msg->topic, (char *) msg->payload);
    sqlite3_exec(data_base, query, 0, NULL, errMSG);
    sqlite3_free(query);
    if (errMSG != NULL){
        printf("Failed to insert data into database\n");
        syslog(LOG_ERR, "Failed to insert data into database\n");
    }
    
    //syslog(LOG_INFO, "New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
}

