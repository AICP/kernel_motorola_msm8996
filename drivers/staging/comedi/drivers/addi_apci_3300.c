#include <asm/i387.h>

#include "../comedidev.h"
#include "comedi_fc.h"

#include "addi-data/addi_common.h"
#include "addi-data/addi_amcc_s5933.h"

#define CONFIG_APCI_3300 1

#define ADDIDATA_DRIVER_NAME	"addi_apci_3300"

#include "addi-data/addi_common.c"
