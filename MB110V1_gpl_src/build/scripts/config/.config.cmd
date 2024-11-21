deps_config := \
	../../sysdeps/linux/Config.in

.config include/config.h: $(deps_config)

$(deps_config):
