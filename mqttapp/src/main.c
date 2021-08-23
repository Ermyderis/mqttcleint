#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>

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

void term_proc(int sigterm);
void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
void remov_unvanted_character(char* string, char character);
void get_topics(char *configfile, struct Node **head, struct Node *current);
struct Config Get_values_from_config(char *configfile);
void print_conf_values(char *configfile);
volatile sig_atomic_t deamonize = 1;


int main(void)
{
    char *configfile = "/etc/config/mqttconfig";
    int rc;
	struct sigaction action;
    struct Config configdata;
    struct mosquitto *mosq;
    struct Node *tmp;
    struct Node *head = NULL;
    struct Node *current = NULL;
    
	memset(&action, 0, sizeof(struct sigaction));
    memset(&configdata, 0, sizeof(struct Config));

	action.sa_handler = term_proc;

    get_topics(configfile, &head, current); 
    //testing
    print_conf_values(configfile);
    mosq = mosquitto_new("subscribe-test", true, head);

    configdata = Get_values_from_config(configfile);
    
    //log opening
    openlog("mqtt_app", LOG_PID, LOG_USER);
	mosquitto_lib_init();
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	mosquitto_username_pw_set(mosq, NULL, NULL); //Seting pass and username

	rc = mosquitto_connect(mosq, configdata.address, atoi(configdata.port), 10);
	if(rc) {
		printf("Could not connect to Broker with return code %d\n", rc);
		return -1;
	}

    mosquitto_loop_start(mosq);
	while(deamonize) {   
	}
    
    printf("Closing");
    //cleaning list
    while(head != NULL) { 
        free(head->data);                
        tmp= head;
        head = head->next;
        free (tmp);
    }
    syslog(LOG_INFO, "Program closed"); //just for testing
    mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
    mosquitto_loop_stop(mosq, true);
	mosquitto_lib_cleanup();
    free(configdata.port);
    free(configdata.address);
    free(configdata.ussername);
    free(configdata.password);
    free(configdata.tsl);
    closelog();
	return 0;
}
void term_proc(int sigterm) 
{
	deamonize = 0;
}


void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    struct Node *ptr = obj;
	if(rc) {
		printf("Error with result code: %d\n", rc);
	}
    else{
    while(ptr->data != NULL) {
            printf("%s:\n", ptr->data);
            mosquitto_subscribe(mosq, NULL, ptr->data, 0);
            ptr = ptr->next;
   }
}
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
	printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
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

void get_topics(char *configfile, struct Node **head, struct Node *current){
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

struct Config Get_values_from_config(char *configfile){
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
void print_conf_values(char *configfile){
    struct Config configdata;
    configdata = Get_values_from_config(configfile);
    printf("Data from configfile\n");
    printf("Port: %s\n", configdata.port);
    printf("Address: %s\n", configdata.address);
    printf("Username: %s\n", configdata.ussername);
    printf("Password: %s\n", configdata.password);
    printf("Tsl: %s\n", configdata.tsl);
}

