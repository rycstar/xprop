x_prop:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif

	if [ -f $(FV_APP_PATH)/x_prop/libxprop.so ]; then \
		cp $(FV_APP_PATH)/x_prop/libxprop.so $(IMAGE_INSTALL_DIR)/rootfs/lib/libxprop.so ; \
	fi;

	if [ -f $(FV_APP_PATH)/x_prop/libxpropctrl.so ]; then \
		cp $(FV_APP_PATH)/x_prop/libxpropctrl.so $(IMAGE_INSTALL_DIR)/rootfs/lib/libxpropctrl.so ; \
	fi;

	if [ -f $(FV_APP_PATH)/x_prop/xprop_srv ]; then \
		cp $(FV_APP_PATH)/x_prop/xprop_srv $(IMAGE_INSTALL_DIR)/rootfs/bin/xprop_srv ; \
		$(STRIP) --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/bin/xprop_srv; \
	fi;
	if [ -f $(FV_APP_PATH)/x_prop/prop_get ]; then \
		cp $(FV_APP_PATH)/x_prop/prop_get $(IMAGE_INSTALL_DIR)/rootfs/bin/prop_get ; \
		$(STRIP) --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/bin/prop_get; \
	fi;
	if [ -f $(FV_APP_PATH)/x_prop/prop_set ]; then \
		cp $(FV_APP_PATH)/x_prop/prop_set $(IMAGE_INSTALL_DIR)/rootfs/bin/prop_set ; \
		$(STRIP) --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/bin/prop_set; \
	fi;
