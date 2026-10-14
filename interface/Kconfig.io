menu "ZBook Interface - I/O"

config ZBOOK_INTERFACE_IO
    bool "Enable ZBook Interface I/O"
    default n
    help
      Enable the ZBook Interface I/O module.

config ZBOOK_INTERFACE_IO_PINS
    bool "Enable ZBook Interface I/O - runtime pin functions"
    default y
    depends on ZBOOK_INTERFACE_IO
    select PICOSDK_USE_PWM
    help
      Enable the header pin factory: assign a function (GPIO, UART, PWM, ADC)
      to a header pin at runtime and have it released/reconfigured on change.

if ZBOOK_INTERFACE_IO_PINS

module = ZBOOK_INTERFACE_IO_PINS
module-str = zbook pins
source "subsys/logging/Kconfig.template.log_config"

endif # ZBOOK_INTERFACE_IO_PINS

endmenu
