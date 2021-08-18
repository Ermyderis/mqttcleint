module("luci.controller.mqttcontroller", package.seeall)

function index()
	entry({"admin", "services", "mqttmodel"}, cbi("mqttmodel"), _("Mqtt subsriber"),105)
end
