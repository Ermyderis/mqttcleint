#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>

#define configfile "/etc/config/mqttconfig"
#define databasefile "/log/mqttmessages.db"

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
void remov_unvanted_character(char* string, char character);
void get_topics(struct Node **head, struct Node *current);
struct Config Get_values_from_config();
void print_conf_values();
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
    //testing
    print_conf_values();

    configdata = Get_values_from_config();
    if((configdata.address == NULL) || (configdata.port == NULL)){
        goto endOfTheProgram;
    }
    
    //log opening
    openlog("mqttapp", LOG_PID, LOG_USER);
    if (sqlite3_open(databasefile, &data_base) != 0){
        printf("Failed to open databse\n");
        syslog(LOG_ERR, "Failed to open databse\n");
        goto endOfTheProgram;
    }
    else{
        printf("Database opened\n");
    }
	rc = mosquitto_lib_init();
    if (rc != 0){
        syslog(LOG_ERR, "Failed to initialize mosquitto");
        goto endOfTheProgram;
    }
    else{
        syslog(LOG_INFO, "Mosquitto initialize");
    }
    mosq = mosquitto_new("subscribe-test", true, head);

      if(configdata.tsl != NULL){
        printf("TSL/SSL ON\n");
        if(configdata.password != NULL && configdata.ussername != NULL){
                if (mosquitto_username_pw_set(mosq, configdata.ussername ,configdata.password)==MOSQ_ERR_SUCCESS){
                syslog(LOG_INFO, "User name and password added successfuly\n");
                printf("User name and password added successfuly\n");
            }else{
                syslog(LOG_WARNING, "Failed to add username or password\n");
                printf("Failed to add username or password\n");
            }
        }
        else{
            printf("Empty usr or pasw\n");
            goto endOfTheProgram;
        }
    }



	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
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
    syslog(LOG_INFO, "Program closed\n"); //just for testing
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
    else{
        printf("Data was inserted: ");
        syslog(LOG_ERR, "Data was inserted: ");
        printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
    }
    
    //syslog(LOG_INFO, "New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
}
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

//just for testing at the end it need to be deleted
void print_conf_values(){
    struct Config configdata;
    configdata = Get_values_from_config(configfile);
    printf("Data from configfile\n");
    printf("Port: %s\n", configdata.port);
    printf("Address: %s\n", configdata.address);
    printf("Username: %s\n", configdata.ussername);
    printf("Password: %s\n", configdata.password);
    printf("Tsl: %s\n", configdata.tsl);
}

