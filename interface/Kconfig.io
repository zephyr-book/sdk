menu "ZBook Interface - I/O"

config ZBOOK_INTERFACE_IO
	bool "Enable ZBook Interface I/O"
	default n
	help
	  Enable the ZBook Interface I/O module.

menu "ZBook Interface - Buttons"
	depends on ZBOOK_INTERFACE_IO

config ZBOOK_INTERFACE_IO_BTN
	bool "Enable ZBook Interface Buttons"
	default y
	depends on ZBOOK_INTERFACE_IO
	select INPUT

config BTN_LONG_PRESS_MS
	int "Long-press threshold in milliseconds"
	default 1000
	depends on ZBOOK_INTERFACE_IO_BTN
	help
	  Minimum hold duration, in milliseconds, for a button press to be
	  reported as ZBOOK_BTN_EVT_LONG_PRESSED.

endmenu
endmenu

if ZBOOK_INTERFACE_IO_BTN
	module = BTN
	module-str = btn
	source "subsys/logging/Kconfig.template.log_config"
endif
