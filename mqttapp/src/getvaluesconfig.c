#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>
#include <uci.h>


#define CONFIGFILE "/etc/config/mqttconfig"


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


volatile int interrupt = 0;

void signal_handler(int signo) {
    signal(SIGINT, NULL);
    syslog(LOG_INFO, "Received signal: %d", signo);
    closelog();
    interrupt = 1;
}

int uci_read_config_data(struct Node **head, struct Node *current, struct Config *configdata){
    int rc = 0;
    struct uci_element *element = NULL;
    struct uci_package *package = NULL;
    struct uci_context *ctx;
    ctx = uci_alloc_context();

    if (uci_load(ctx, CONFIGFILE, &package) != UCI_OK) {
            syslog (LOG_ERR,"Failed to load uci.");
            printf("Failed to load uci\n");
            rc = -1;
            goto delete;
        }
    else{
        syslog (LOG_ERR,"Uci loaded");
        printf("Uci loaded for config\n");
    }
    uci_foreach_element(&package->sections, element) {
        struct uci_section *section = uci_to_section(element);

         if (strcmp(section->type, "mqttconfig") == 0) {
                if (uci_lookup_option_string(ctx, section, "port")!= NULL){
                    configdata->port = malloc(strlen((char*)uci_lookup_option_string(ctx, section, "port")) + 1);
                    strcpy(configdata->port,(char*)uci_lookup_option_string(ctx, section, "port"));
                }
                 if (uci_lookup_option_string(ctx, section, "address")!= NULL){
                    configdata->address = malloc(strlen((char*)uci_lookup_option_string(ctx, section, "address")) + 1);
                    strcpy(configdata->address,(char*)uci_lookup_option_string(ctx, section, "address"));
                }
                if (uci_lookup_option_string(ctx, section, "username")!= NULL){
                    configdata->ussername = malloc(strlen((char*)uci_lookup_option_string(ctx, section, "username")) + 1);
                    strcpy(configdata->ussername,(char*)uci_lookup_option_string(ctx, section, "username"));
                }
                if (uci_lookup_option_string(ctx, section, "password")!= NULL){
                    configdata->password = malloc(strlen((char*)uci_lookup_option_string(ctx, section, "password")) + 1);
                    strcpy(configdata->password,(char*)uci_lookup_option_string(ctx, section, "password"));
                }
                if (uci_lookup_option_string(ctx, section, "enabletsl")!= NULL){
                    configdata->tsl = malloc(strlen((char*)uci_lookup_option_string(ctx, section, "enabletsl")) + 1);
                    strcpy(configdata->tsl,(char*)uci_lookup_option_string(ctx, section, "enabletsl"));
                }
        }

        if (strcmp(section->type, "topic") == 0) {
            char *topic =(char*)uci_lookup_option_string(ctx, section, "topic");
            struct Node *new = (struct Node *)malloc(sizeof(struct Node));
            new->datatopics = strdup(topic);
            if(*head)
                new->next = *head;
            *head = new;
        }
    }

delete:
    uci_free_context(ctx);
    return rc;
}


