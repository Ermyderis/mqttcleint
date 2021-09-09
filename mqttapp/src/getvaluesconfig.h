struct Case{
    char *casetopic;
    char *casekey;
    char *casetype;
    char *casevalue;
    char *casecomparisontype;
    struct Case *next;
};


struct Node {
   char *datatopics;
   struct Case *chead;
   struct Node *next;
};

struct Config{
    char *port;
    char *address;
    char *ussername;
    char *password;
    char *tsl;
};

void signal_handler(int signo);
int uci_read_config_data(struct Node **head, struct Config *configdata);
