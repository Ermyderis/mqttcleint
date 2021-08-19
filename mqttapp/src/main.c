#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <uci.h>
#include <mosquitto.h>

volatile sig_atomic_t deamonize = 1;

void term_proc(int sigterm) 
{
	deamonize = 0;
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
	printf("ID: %d\n", * (int *) obj);
	if(rc) {
		printf("Error with result code: %d\n", rc);
	}
    else{
	mosquitto_subscribe(mosq, NULL, "test/BSME2", 0);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
	printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
    syslog(LOG_INFO, "New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
    FILE* fp;
    fp = fopen("/tmp/messages.txt","a");
    fprintf(fp, "New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
    fclose(fp);
}

int main(void)
{
	FILE* fp;
	int i = 1;
    int rc;


	struct sigaction action;
    struct mosquitto *mosq;
	memset(&action, 0, sizeof(struct sigaction));
    
	action.sa_handler = term_proc;
	mosq = mosquitto_new("subscribe-test", true, NULL);

    sigaction(SIGTERM, &action, NULL);
    //log opening
    openlog("mqtt_app", LOG_PID, LOG_USER);
	mosquitto_lib_init();
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	mosquitto_username_pw_set(mosq, NULL, NULL);

	rc = mosquitto_connect(mosq, "test.mosquitto.org", 1883, 10);
	if(rc) {
		printf("Could not connect to Broker with return code %d\n", rc);
		return -1;
	}

    mosquitto_loop_start(mosq);
	while(deamonize) {
        
        syslog(LOG_INFO, "Line number: %d\n", i);
		i++;
        sleep(20);
	}
    syslog(LOG_INFO, "Program closed"); //just for testing
    mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
    mosquitto_loop_stop(mosq, true);
	mosquitto_lib_cleanup();
    closelog();
	return 0;
}