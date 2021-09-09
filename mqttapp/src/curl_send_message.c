#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <sqlite3.h>
#include <json-c/json.h>
#include <uci.h>
#include <curl/curl.h>

#include "database.h"
#include "getvaluesconfig.h"

#define CONFIGFILE "/etc/config/mqttconfig"

#define FROM    "dovydas0000@gmail.com"
#define TO		"dovydas0000@gmail.com"

char *payload_text = "";

 
struct upload_status {
  size_t bytes_read;
};

/*
Fucntion prepares data for curl to use
*/
static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;
  size_t room = size * nmemb;
  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }
  data = &payload_text[upload_ctx->bytes_read];
  if(data) {
    size_t len = strlen(data);
    if(room < len)
      len = room;
    memcpy(ptr, data, len);
    upload_ctx->bytes_read += len;
    return len;
  }
 
  return 0;
}

void mail_sending(char *payload){
	size_t sizeneed;
	sizeneed = snprintf(NULL, 0, "Subject: Case\r\n\r\nJson \"%s\": \r\n\r\n", payload+1); 
	payload_text = (char*) malloc(sizeneed);
	sprintf(payload_text, "Subject: Case\r\n\r\nJson \"%s\": \r\n\r\n", payload);  
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx = { 0 };
	

	curl = curl_easy_init();
	if(curl) {
		/* This is the URL for your mailserver */ 
		curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");


		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);
		

		recipients = curl_slist_append(recipients, TO);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
		
			/* We're using a callback function to specify the payload (the headers and
			* body of the message). You could just use the CURLOPT_READDATA option to
			* specify a FILE pointer to read from. */ 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		
		curl_easy_setopt(curl, CURLOPT_USERNAME, FROM);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, "cgqbvuncyiqlfluu");
		
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		
			/* Send the message */ 

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		res = curl_easy_perform(curl);
		
			/* Check for errors */ 
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			syslog(LOG_ERR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		
			/* Free the list of recipients */ 
		curl_slist_free_all(recipients);

		curl_easy_cleanup(curl);
	}
	free(payload_text);
}
 


void check_comparison_type(char *comparisontype, char *key, char *casetype, char *casevalue, char *payload){
	if(atoi(casetype) == 0){
		if(atoi(comparisontype) == 0){
			printf("!=\n");
			if(strcmp(key, casevalue) != 0){
				printf("%s\n", payload);
				mail_sending(payload);
				
			}
		}
		else{
			if(atoi(comparisontype) == 1){
				printf("==\n");
				if(strcmp(key, casevalue) == 0){
				printf("%s\n", payload);
				mail_sending(payload);
				}
			}
		}
	}
	if(atoi(casetype) == 1){
		if(atoi(comparisontype) == 0){
				printf("!=\n");
				if(atoi(key) != casevalue){
					printf("%s\n", payload);
					mail_sending(payload);
				}
			}
			else{
				if(atoi(comparisontype) == 1){
					printf("==\n");
					if(atoi(key) == casevalue){
						printf("%s\n", payload);
						mail_sending(payload);
				}
				}
				else{
					if(atoi(comparisontype) == 2){
						printf("<=\n");
						if(atoi(key) <= casevalue){
							printf("%s\n", payload);
							mail_sending(payload);
						}
					}
					else{
						if(atoi(comparisontype) == 3){
							printf(">=\n");
							if(atoi(key) >= casevalue){
									printf("%s\n", payload);
									mail_sending(payload);
								}
						}
						else{
							if(atoi(comparisontype) == 4){
								printf("<\n");
								if(atoi(key) < casevalue){
									printf("%s\n", payload);
									mail_sending(payload);
								}
							}
							else{
								if(atoi(comparisontype) == 5){
									printf(">\n");
									if(atoi(key) > casevalue){
										printf("%s\n", payload);
										mail_sending(payload);
									}
								}
							}
						}
					}
				}
			}
		} 	
}


void curl_send_message(struct Node *head, char *topic, char *payload){
	
	int rc = 0;
	struct Node *ptr = head;
	struct json_object *parsed_json;
	struct json_object *string;

    while(ptr != NULL) //all topics
        {
			if(strcmp(ptr->datatopics, topic) == 0){
				struct Case *ptr2 = ptr->chead;
				while(ptr2 != NULL){
					parsed_json = json_tokener_parse(payload);

					json_object_object_get_ex(parsed_json, ptr2->casekey, &string);
					check_comparison_type(ptr2->casecomparisontype, json_object_get_string(string), ptr2->casetype, ptr2->casevalue, payload);

					ptr2 = ptr2->next;
				}
			}
			
            ptr = ptr->next;
        }
    
}
