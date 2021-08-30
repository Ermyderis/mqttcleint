#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>
#include "getvaluesconfig.h"

#define DATABASE "/log/mqttmessages.db"

sqlite3 *data_base = NULL;


int database_and_initialize_mosquitto(){
    //open database
    int rc = 0;
    if (sqlite3_open(DATABASE, &data_base) != 0){
        printf("Failed to open databse\n");
        syslog(LOG_ERR, "Failed to open databse\n");
        rc = -1;
       
    }
    else{
        printf("Database opened\n");
    }
    //Initialize mosquitto
    if (rc == 0){
        rc = mosquitto_lib_init();
        if (rc != 0){
            syslog(LOG_ERR, "Failed to initialize mosquitto");
            rc = -2;
            
        }
        else{
            syslog(LOG_INFO, "Mosquitto initialize");
        }
    }
    return rc;
}
