map = Map("mosquitto_client")

s = map:section(NamedSection, "mosq_sct_set", "mosqsm", "Messages");
s.template = "mqtt_htm/mqttmessages"
return map
