menu "ZBook Interface - I/O"

  config ZBOOK_INTERFACE_IO
    bool "Enable ZBook Interface I/O"
    default n
    help
      Enable the ZBook Interface I/O module.

  config ZBOOK_INTERFACE_IO_PINS
    bool "Enable ZBook Interface I/O - runtime pin functions"
    depends on ZBOOK_INTERFACE_IO
    help
      Enable the header pin factory: assign a function (GPIO, UART, PWM, ADC)
      to a header pin at runtime and have it released/reconfigured on change.

  menu "ZBook Interface I/O - runtime pin functions"
    depends on ZBOOK_INTERFACE_IO_PINS

    config ZBOOK_INTERFACE_IO_ADC
      bool "Enable ZBook Interface I/O - ADC"
      default n
      select ADC
      help
        Enable the ZBook Interface I/O - ADC module. This config must be enable 
        using "select CONFIG_ZBOOK_INTERFACE_IO_ADC" in the ADC module Kconfig.

    config ZBOOK_INTERFACE_IO_PWM
      bool "Enable ZBook Interface I/O - PWM"
      select PICOSDK_USE_PWM
      default n
      help
        Enable the ZBook Interface I/O - PWM module. This config must be enable 
        using "select CONFIG_ZBOOK_INTERFACE_IO_PWM" in the PWM module Kconfig.

    config ZBOOK_INTERFACE_IO_UART
      bool "Enable ZBook Interface I/O - UART"
      default n
      help
        Enable the ZBook Interface I/O - ADC module. This config must be enable 
        using "select CONFIG_ZBOOK_INTERFACE_IO_UART" in the UART module Kconfig.

    config ZBOOK_INTERFACE_IO_GPIO
      bool "Enable ZBook Interface I/O - GPIO"
      default y

  endmenu

  menu "ZBook Interface - Buttons"
    depends on ZBOOK_INTERFACE_IO

    config ZBOOK_INTERFACE_IO_BTN
      bool "Enable ZBook Interface Buttons"
      default y
      depends on ZBOOK_INTERFACE_IO
      depends on $(dt_alias_enabled,button0) && $(dt_alias_enabled,button1)
      depends on $(dt_alias_enabled,button2) && $(dt_alias_enabled,button3)
      select INPUT

    config BTN_LONG_PRESS_MS
      int "Long-press threshold in milliseconds"
      default 1000
      depends on ZBOOK_INTERFACE_IO_BTN
      help
        Minimum hold duration, in milliseconds, for a button press to be
        reported as ZBOOK_BTN_EVT_LONG_PRESSED.

  endmenu

  menu "ZBook Interface - IR"
    depends on ZBOOK_INTERFACE_IO

    config ZBOOK_INTERFACE_IO_IR
      bool "Enable ZBook Interface IR"
      default y
      depends on ZBOOK_INTERFACE_IO
      depends on $(dt_nodelabel_enabled,ir_emitter) && $(dt_nodelabel_enabled,ir_receiver)
      select GPIO
      help
        Enable the ZBook IR interface: the onboard IR emitter as an output and
        the IR phototransistor receiver as an input with event callbacks.

    config ZBOOK_INTERFACE_IO_IR_PULSE_MAX_MS
      int "Maximum IR emitter pulse duration in milliseconds"
      default 10000
      range 1 3600000
      depends on ZBOOK_INTERFACE_IO_IR
      help
        Longest duration, in milliseconds, accepted by zbook_ir_emitter_pulse().

    config ZBOOK_INTERFACE_IO_IR_SETTLE_MS
      int "IR receiver settle time in milliseconds"
      default 5
      range 0 1000
      depends on ZBOOK_INTERFACE_IO_IR
      help
        Time the receiver level must stay stable before an event is reported,
        filtering the slow RC edges of the phototransistor and ambient flicker.

  endmenu

endmenu

if ZBOOK_INTERFACE_IO_PINS
	module = ZBOOK_INTERFACE_IO_PINS
	module-str = zbook pins
	source "subsys/logging/Kconfig.template.log_config"
endif

if ZBOOK_INTERFACE_IO_BTN
	module = BTN
	module-str = btn
	source "subsys/logging/Kconfig.template.log_config"
endif

if ZBOOK_INTERFACE_IO_IR
	module = ZBOOK_INTERFACE_IO_IR
	module-str = zbook ir
	source "subsys/logging/Kconfig.template.log_config"
endif
