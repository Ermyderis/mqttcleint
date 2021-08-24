module("luci.controller.mqttcontroller", package.seeall)

function index()
    entry( { "admin", "services", "mqttcontroller"}, firstchild(), _("MQTT Client"), 150)
    entry( { "admin", "services", "mqttcontroller", "mqttmodel" }, cbi("mqttmodel"), _("Subscriber"), 1).leaf = true
    entry( { "admin", "services", "mqttcontroller", "messagemodel" }, cbi("messagemodel"), _("Messages"), 2).leaf = true
end
