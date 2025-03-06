#if CONFIG_RHEA_D2D_SELF_ID == 0
#include "pcie_ep_host.c"
#elif D2D_C2C_CHIP == 0
#include "pcie_rc.c"
#elif D2D_C2C_CHIP == 1
#include "pcie_ep.c"
#endif