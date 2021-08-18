module("luci.controller.mosquittocontroler", package.seeall)

function index()

  entry( { "admin", "services", "mqttt"}, firstchild(), _("MQTT client"), 150)
  entry( { "admin", "services", "mqttt", "Model" }, cbi("mqttmodel"), _("Model"), 1).leaf = true
end
