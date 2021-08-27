require("luci.config")

local s, s2, m

m = Map("mqttconfig")

section = m:section(NamedSection, "mqttconfigsct", "mqttconfig", "Subscribe info")

flag = section:option(Flag, "enable", "Enable", "Enable program")
tls_ssl = section:option(Flag, "enabletsl", "TSL_SSL", "Enable program use TSL_SSL")

port = section:option( Value, "port", "Port")
port.datatype = "string"
port.placeholder = translate("1883 if using TSL/SSL use 8883 port")
port.rmempty = false


address = section:option( Value, "address", "Address")
address.datatype = [[ maxlength(100), "string"]]
address.rmempty = false



username = section:option(Value, "username","Username")
username.datatype = "credentials_validate"
username.placeholder = translate("Username")



password = section:option(Value, "password", translate"Password")
password.password = true
password.datatype = "credentials_validate"
password.placeholder = translate("Password")




---------------------------- Topic ----------------------------

st = m:section(TypedSection, "topic", translate("Topics"), translate("") )
st.addremove = true
st.anonymous = true
st.template = "cbi/tblsection"
st.novaluetext = translate("There are no topics created yet.")

topic = st:option(Value, "topic", translate("Topic name"), translate(""))
topic.datatype = "string"
topic.maxlength = 65536
topic.placeholder = translate("Topic")
topic.rmempty = false
topic.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	if value == nil or value == "" then
		self.map:error_msg(translate("Topic name can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

return m
