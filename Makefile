include $(TOPDIR)/rules.mk

PKG_NAME:=DbusSmsForwardCPlus
PKG_RELEASE:=1.0.6
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/DbusSmsForwardCPlus
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=DbusSmsForwardCPlus
  DEPENDS:=+libdbus +libopenssl +libcurl +libstdcpp +modemmanager
endef

define Package/myprogram/description
  This is a smsforward program.
endef

define Build/Prepare
	$(MKDIR) -p $(PKG_BUILD_DIR)
	$(CP) ./DbusSmsForwardCPlus/* $(PKG_BUILD_DIR)
endef


define Package/DbusSmsForwardCPlus/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/DbusSmsForwardCPlus $(1)/bin
endef

$(eval $(call BuildPackage,DbusSmsForwardCPlus))
