#pragma once

#include <util/uacpi/types.h>
#include <util/uacpi/registers.h>

uacpi_status uacpi_ininitialize_registers(void);
void uacpi_deinitialize_registers(void);
