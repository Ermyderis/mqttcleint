require("luci.config")

local s, s2, m

m = Map("mqttconfig")



section = m:section(NamedSection, "mqttconfig_sct", "mqttconfig", "Subscribe info")

flag = section:option(Flag, "enable", "Enable", "Enable program")

irgid = section:option( Value, "port", "Port")
irgid.datatype = "string"

typeid = section:option( Value, "address", "Address")
typeid.datatype = "string"

token = section:option( Value, "token", "Token")
token.datatype = [[ maxlength(25), "string"]]

tls_ssl = section:option(Flag, "enable", "TSL_SSL", "Enable program use TSL_SSL")





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
