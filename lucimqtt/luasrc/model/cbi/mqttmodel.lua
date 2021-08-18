
require("luci.config")
local fw = require "luci.model.firewall"
local deathTrap = { }
local m, s, s2
local certs = require "luci.model.certificate"
local certificates = certs.get_certificates()
local keys = certs.get_keys()
local cas = certs.get_ca_files().certs

m = Map("mosquittocontroler")
m:chain("firewall")
fw.init(m.uci)

s = m:section(NamedSection, "mqttt", "mqttt", translate("MQTT Broker"), translate("The Broker will “listen” for connections on the specified Local port."))

enb = s:option(Flag, "enabled", translate("Enable"), translate("Select to enable MQTT"))
enb.rmempty = false

local_port = s:option( Value, "local_port", translate("Local Port"), translate("Specify local port which the MQTT will be listen to"))
local_port.default = "1883"
local_port.placeholder = "1883"
local_port.datatype = "port"
local_port.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: local port is empty"))
	end
	Value.parse(self, section, novld, ...)
end

ara = s:option(Flag, "enable_ra", translate("Enable Remote Access"), translate("Select to enable remote access"))
ara.rmempty = false
function ara.write(self, section)
	local fval = self:formvalue(section)
	local fport = local_port:formvalue(section)
	local needsPortUpdate = false
	local fwRuleInstName = "nil"

	if not deathTrap[1] then deathTrap[1] = true
	else return end

	if not fval then
		fval = "0"
	else
		fval = "1"
	end

	m.uci:foreach("firewall", "rule", function(z)
		if z.name == "Enable_MQTT_WAN" then
			fwRuleInstName = z[".name"]
			if z.dest_port ~= fport then
				needsPortUpdate = true
			end
			if z.enabled ~= fval then
				needsPortUpdate = true
			end
		end
	end)

	if needsPortUpdate == true then
		m.uci:set("firewall", fwRuleInstName, "dest_port", fport)
		m.uci:set("firewall", fwRuleInstName, "enabled", fval)
		m.uci:save("firewall")
	end

	if fwRuleInstName == "nil" then
		local wanZone = fw:get_zone("wan")
		if not wanZone then
			m.message = translate("Could not add firewall rule")
			return
		end
		local fw_rule = {
			name = "Enable_MQTT_WAN",
			target = "ACCEPT",
			proto = "tcp",
			dest_port = fport,
			enabled = fval
		}

		wanZone:add_rule(fw_rule)
		m.uci:save("firewall")
	end
end

function ara.cfgvalue(self, section)
	local fwRuleEn = false

	m.uci:foreach("firewall", "rule", function(z)
		if z.name == "Enable_MQTT_WAN" and z.enabled == "1" then
			fwRuleEn = true
		end
	end)
	if fwRuleEn then
		return self.enabled
	else
		return self.disabled
	end

end

---------------------------- Broker Settings ----------------------------

s2 = m:section(NamedSection, "mqttt", "mqttt", translate("Broker settings"), "")
s2:tab("security", translate("Security"))
s2:tab("client", translate("Bridge"))
s2:tab("misc",  translate("Miscellaneous"))
s2.template = "mqttt/tsectionn"
s2.anonymous = true
FileUpload.unsafeupload = true

function s2.cfgsections(self)
	return {"mqttt"}
end
---------------------------- Security Tab ----------------------------

use_tls_ssl = s2:taboption("security", Flag, "use_tls_ssl", translate("Use TLS/SSL"), translate("Mark to use TLS/SSL for connection"))
use_tls_ssl.rmempty = false
function use_tls_ssl.write(self, section, value)
	self.map:set("mqttt", self.option, value)
end

tls_type = s2:taboption("security", ListValue, "tls_type", translate("TLS Type"), translate("Select the type of TLS encryption"))
tls_type:depends("use_tls_ssl", "1")
tls_type:value("cert", translate("Certificate based"))
tls_type:value("psk", translate("Pre-Shared-Key based"))

local certificates_link = luci.dispatcher.build_url("admin", "system", "admin", "certificates")
o = s2:taboption("security", Flag, "_device_sec_files", translate("Certificate files from device"),
	translatef("Choose this option if you want to select certificate files from device.\
	Certificate files can be generated <a class=link href=%s>%s</a>", certificates_link, translate("here")))
o:depends({use_tls_ssl="1", tls_type = "cert"})

ca_file = s2:taboption("security", FileUpload, "ca_file", translate("CA File"), translate("Upload CA file"));
ca_file:depends({use_tls_ssl="1",_device_sec_files="", tls_type = "cert"})

cert_file = s2:taboption("security", FileUpload, "cert_file", translate("CERT File"), translate("Upload CERT file"));
cert_file:depends({use_tls_ssl="1",_device_sec_files="", tls_type = "cert"})

key_file = s2:taboption("security", FileUpload, "key_file", translate("Key File"), translate("Upload Key file"));
key_file:depends({use_tls_ssl="1",_device_sec_files="", tls_type = "cert"})

ca_file = s2:taboption("security", ListValue, "_device_ca_file", translate("CA File"), translate("Upload CA file"));
ca_file:depends({_device_sec_files = "1", tls_type = "cert"})

if #cas > 0 then
	for _,ca in pairs(cas) do
		ca_file:value("/etc/certificates/" .. ca.name, ca.name)
	end
else 
	ca_file:value("", translate("-- No file available --"))
end

function ca_file.write(self, section, value)
	m.uci:set(self.config, section, "ca_file", value)
end

ca_file.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "ca_file") or ""
end

cert_file = s2:taboption("security", ListValue, "_device_cert_file", translate("CERT File"), translate("Upload CERT file"));
cert_file:depends({_device_sec_files = "1", tls_type = "cert"})

if #certificates > 0 then
	for _,cert in pairs(certificates) do
		cert_file:value("/etc/certificates/" .. cert.name, cert.name)
	end
else 
	cert_file:value("", translate("-- No file available --"))
end

function cert_file.write(self, section, value)
	m.uci:set(self.config, section, "cert_file", value)
end

cert_file.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "cert_file") or ""
end

key_file = s2:taboption("security", ListValue, "_device_key_file", translate("Key File"), translate("Upload Key file"));
key_file:depends({_device_sec_files = "1", tls_type = "cert"})

if #keys > 0 then
	for _,key in pairs(keys) do
		key_file:value("/etc/certificates/" .. key.name, key.name)
	end
else 
	key_file:value("", translate("-- No file available --"))
end

function key_file.write(self, section, value)
	m.uci:set(self.config, section, "key_file", value)
end

key_file.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "key_file") or ""
end

tls_version = s2:taboption("security", ListValue, "tls_version", translate("TLS version"), translate("Used TLS version"));
tls_version:depends({tls_type = "cert"})
tls_version:value("tlsv1", "tlsv1");
tls_version:value("tlsv1.1", "tlsv1.1");
tls_version:value("tlsv1.2", "tlsv1.2");
tls_version:value("all", "Support all");
tls_version.default = "all"

o = s2:taboption("security", Value, "psk", translate("Pre-Shared-Key"), translate("The pre-shared-key in hex format with no leading “0x”"))
o.datatype = "lengthvalidation(0, 128)"
o.placeholder = "Key"
o:depends({use_tls_ssl = "1", tls_type = "psk"})

o = s2:taboption("security", Value, "identity", translate("Identity"), translate("Specify the Identity"))
o.datatype = "uciname"
o.placeholder = "Identity"
o:depends({use_tls_ssl = "1", tls_type = "psk"})



---------------------------- Bridge Tab ----------------------------

client_enabled = s2:taboption("client", Flag, "client_enabled", translate("Enable"), translate("Enable connection to remote bridge"))
client_enabled.rmempty = false

connection_name = s2:taboption("client", Value, "connection_name", translate("Connection Name"), "")
connection_name.datatype = "nospace"
connection_name.placeholder = translate("Name")
connection_name:depends("client_enabled", "1")
connection_name.parse = function(self, section, novld, ...)
	local bridgeEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.client_enabled")
	local value = self:formvalue(section)
	if bridgeEnabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: connection name is empty"))
	end
	Value.parse(self, section, novld, ...)
end

proto_version = s2:taboption("client", ListValue, "bridge_protocol_version", translate("Protocol Version"), translate("Version of the MQTT protocol"))
proto_version:value("mqttv31", translate("3.1"))
proto_version:value("mqttv311", translate("3.1.1"))
proto_version.default = "mqttv31"
proto_version:depends("client_enabled", "1")

remote_address = s2:taboption("client", Value, "remote_addr", translate("Remote Address"), translate("Select remote bridge address"))
remote_address.datatype = "host"
remote_address.placeholder = "0.0.0.0"
remote_address:depends("client_enabled", "1")
remote_address.parse = function(self, section, novld, ...)
	local bridgeEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.client_enabled")
	local value = self:formvalue(section)
	if bridgeEnabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: remote address is empty"))
	end
	Value.parse(self, section, novld, ...)
end

remote_port = s2:taboption("client", Value, "remote_port", translate("Remote Port"), translate("Select remote port"))
remote_port.datatype = "port"
remote_port.default = "1883"
remote_port.placeholder = "1883"
remote_port:depends("client_enabled", "1")
remote_port.parse = function(self, section, novld, ...)
	local bridgeEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.client_enabled")
	local value = self:formvalue(section)
	if bridgeEnabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: remote port is empty"))
	end
	Value.parse(self, section, novld, ...)
end

use_remote_tls = s2:taboption("client", Flag, "use_remote_tls", translate("Use Remote TLS/SSL"), translate("Select to use TLS/SSL for remote connection"))
use_remote_tls:depends("client_enabled", "1")

local certificates_link = luci.dispatcher.build_url("admin", "system", "admin", "certificates")
o = s2:taboption("client", Flag, "_device_brg_files", translate("Certificate files from device"),
	translatef("Choose this option if you want to select certificate files from device.\
	Certificate files can be generated <a class=link href=%s>%s</a>", certificates_link, translate("here")))
o:depends("use_remote_tls", "1")

bridge_ca_file = s2:taboption("client", FileUpload, "bridge_cafile", translate("Bridge CA File"), translate("Upload bridge CA file"))
bridge_ca_file:depends({use_remote_tls="1", _device_brg_files=""})

bridge_certfile = s2:taboption("client", FileUpload, "bridge_certfile", translate("Bridge CERT File"), translate("Upload bridge CERT file"))
bridge_certfile:depends({use_remote_tls="1", _device_brg_files=""})

bridge_keyfile = s2:taboption("client", FileUpload, "bridge_keyfile", translate("Bridge Key File"), translate("Upload bridge Key file"))
bridge_keyfile:depends({use_remote_tls="1", _device_brg_files=""})

ca_file = s2:taboption("client", ListValue, "_device_bridge_cafile", translate("Bridge CA File"), translate("Upload CA file"));
ca_file:depends("_device_brg_files", "1")

if #cas > 0 then
	for _,ca in pairs(cas) do
		ca_file:value("/etc/certificates/" .. ca.name, ca.name)
	end
else 
	ca_file:value("", translate("-- No file available --"))
end

function ca_file.write(self, section, value)
	m.uci:set(self.config, section, "bridge_cafile", value)
end

ca_file.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "bridge_cafile") or ""
end

cert_file = s2:taboption("client", ListValue, "_device_bridge_certfile", translate("Bridge CERT File"), translate("Upload CERT file"));
cert_file:depends("_device_brg_files", "1")

if #certificates > 0 then
	for _,cert in pairs(certificates) do
		cert_file:value("/etc/certificates/" .. cert.name, cert.name)
	end
else 
	cert_file:value("", translate("-- No file available --"))
end

function cert_file.write(self, section, value)
	m.uci:set(self.config, section, "bridge_certfile", value)
end

cert_file.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "bridge_certfile") or ""
end

key_file = s2:taboption("client", ListValue, "_device_bridge_keyfile", translate("Bridge Key File"), translate("Upload Key file"));
key_file:depends("_device_brg_files", "1")

if #keys > 0 then
	for _,key in pairs(keys) do
		key_file:value("/etc/certificates/" .. key.name, key.name)
	end
else 
	key_file:value("", translate("-- No file available --"))
end

function key_file.write(self, section, value)
	m.uci:set(self.config, section, "bridge_keyfile", value)
end

key_file.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "bridge_keyfile") or ""
end

bridge_tls_version = s2:taboption("client", ListValue, "bridge_tls_version", translate("Bridge TLS version"), translate("Used bridge TLS version"));
bridge_tls_version:depends("use_remote_tls", "1");
bridge_tls_version:value("tlsv1", "tlsv1");
bridge_tls_version:value("tlsv1.1", "tlsv1.1");
bridge_tls_version:value("tlsv1.2", "tlsv1.2");

use_bridge_login = s2:taboption("client", Flag, "use_bridge_login", translate("Use Remote Bridge Login"), translate("Select to use login for bridge"))
use_bridge_login:depends("client_enabled", "1")

remote_clientid = s2:taboption("client", Value, "remote_clientid", translate("Remote ID"), translate("Choose remote client ID"))
remote_clientid:depends("use_bridge_login", "1")
remote_clientid.rmempty = false
remote_clientid.datatype = "credentials_validate"
remote_clientid.validator_hint = translate("All characters are allowed except ` and space. Length must be between 1 and 256 characters")
remote_clientid.maxlength = "256"
remote_clientid.placeholder = "1"
remote_clientid.parse = function(self, section, novld, ...)
	local bridgeEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.client_enabled")
	local bridgeLoginEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.use_bridge_login")
	local value = self:formvalue(section)
	if bridgeEnabled and bridgeLoginEnabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: remote ID is empty"))
	end
	Value.parse(self, section, novld, ...)
end

remote_username = s2:taboption("client", Value, "remote_username", translate("Remote Username"),translate("Choose remote user name"))
remote_username:depends("use_bridge_login", "1")
remote_username.placeholder = translate("Username")
remote_username.rmempty = false
remote_username.datatype = "credentials_validate"
remote_username.validator_hint = translate("All characters are allowed except ` and space. Length must be between 1 and 256 characters")
remote_username.maxlength = "256"
remote_username.parse = function(self, section, novld, ...)
	local bridgeEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.client_enabled")
	local bridgeLoginEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.use_bridge_login")
	local value = self:formvalue(section)
	if bridgeEnabled and bridgeLoginEnabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: remote username is empty"))
	end
	Value.parse(self, section, novld, ...)
end

remote_password = s2:taboption("client", Value, "remote_password", translate("Remote Password"), translate("Choose remote password"))
remote_password:depends("use_bridge_login", "1")
remote_password.placeholder = translate("Password")
remote_password.rmempty = false
remote_password.datatype = "fieldvalidation('^[^`]+$',1)"
remote_password.validator_hint = translate("All characters are allowed except `. Length must be between 1 and 256 characters")
remote_password.maxlength = "256"
remote_password.parse = function(self, section, novld, ...)
	local bridgeEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.client_enabled")
	local bridgeLoginEnabled = luci.http.formvalue("cbid.mosquittocontroler.mqttt.use_bridge_login")
	local value = self:formvalue(section)
	if bridgeEnabled and bridgeLoginEnabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: remote password is empty"))
	end
	Value.parse(self, section, novld, ...)
end
remote_password.password = true

try_private = s2:taboption("client", Flag, "try_private", translate("Try Private"), translate("Check if remote broker is another instance of a daemon"))
try_private:depends("client_enabled", "1")


cleansession = s2:taboption("client", Flag, "cleansession", translate("Clean Session"), translate("Discard session state when connecting or disconnecting"))
cleansession:depends("client_enabled", "1")

---------------------------- Topic ----------------------------

st = m:section(TypedSection, "topic", translate("Topics"), translate("") )
st.addremove = true
st.anonymous = true
st.template = "mqttt/tblsectionn"
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

direction = st:option(ListValue, "direction", translate("Direction"), translate("The direction that the messages will be shared in"))
direction:value("out", "OUT")
direction:value("in", "IN")
direction:value("both", "BOTH")
direction.default = "out"

qos = st:option(ListValue, "qos", translate("QoS level"), translate("The publish/subscribe QoS level used for this topic"))
qos:value("0", "At most once (0)")
qos:value("1", "At least once (1)")
qos:value("2", "Exactly once (2)")
qos.rmempty=false
qos.default="0"

---------------------------- Misc Tab ----------------------------

acl_file_path = s2:taboption("misc", FileUpload, "acl_file_path", translate("ACL File"), translate("Select ACL file"))

password_file = s2:taboption("misc", FileUpload, "password_file", translate("Password File"), translate("Uploads passwords/users file"))

persistence = s2:taboption("misc", Flag, "persistence", translate("Persistence"),
						   translate("If true, connection, subscription and message data will be written to the disk"))

allow_anonymous = s2:taboption("misc", Flag, "anonymous_access", translate("Allow Anonymous"), translate("Allows anonymous access"))
allow_anonymous.default = "1"
allow_anonymous.rmempty = false

return m
