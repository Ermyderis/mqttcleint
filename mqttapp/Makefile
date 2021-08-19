include $(TOPDIR)/rules.mk

PKG_NAME:=mqttapp
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/mqttapp
	CATEGORY:=Base system
	TITLE:=mqttapp
	DEPENDS:=+libmosquitto
endef

define Package/mqttapp/description
	This is an example app package which uses an example library package
endef

define Package/mqttapp/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/mqttapp.init $(1)/etc/init.d/mqttapp
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqttapp $(1)/usr/bin
endef

$(eval $(call BuildPackage,mqttapp))
