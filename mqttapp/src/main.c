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
#include "on_connectmesages.h"


volatile int interrupt;
sqlite3 *data_base;



int main(void)
{
    int errnum;
    int rc;
	struct sigaction action;
    struct Config configdata;
    struct mosquitto *mosq;
    struct Node *temporarily;
    struct Node *head = NULL;

	memset(&action, 0, sizeof(struct sigaction));
    memset(&configdata, 0, sizeof(struct Config));

	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    sigaction(SIGTERM, &action, NULL);

    //log opening
    openlog("mqttapp", LOG_PID, LOG_USER);
    rc = uci_read_config_data(&head, &configdata); 
    if(rc == -1){
        goto logclose;
    }

    rc = database_and_initialize_mosquitto();
    if (rc == -1){
        goto dataBaseNotCreated;
    }
    else if (rc == -2){
        goto endOfTheProgram;
    }

    mosq = mosquitto_new("subscribe-test", true, head);
    //check if tsl not equal NULL and if pasw and usr fields are not empty
    rc = set_ussername_password_tsl(mosq, &configdata);
    if(rc != 0){
        goto endOfTheProgram;
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
    syslog(LOG_INFO, "Working\n");
    printf("Working\n");
    //Program won't shut down while interupt = 1
	while(!interrupt) {   
        
	}

    mosquitto_loop_stop(mosq, true);
    endOfTheProgram:
    //Disconecting
    mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
    dataBaseNotCreated:
    sqlite3_close(data_base);
    //cleaning list  
    while(head != NULL) { 
        free(head->datatopics);                
        temporarily= head;
        head = head->next;
        free (temporarily);
    }
    //free memory
    free(configdata.port);
    free(configdata.address);
    free(configdata.ussername);
    free(configdata.password);
    free(configdata.tsl);
    logclose:
    closelog();
	return rc;
}


