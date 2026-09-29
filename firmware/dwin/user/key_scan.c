#include "key_scan.h"
#include "dgus_tools.h"
#include "log.h"

void scan_key_func(void)
{
    u16 key_value;
    key_value = Read_Vp(KEY_SCAN_VP);

    if(key_value)
    {
        LOG_DEBUG("scan key value = 0x%h04x", key_value);

        switch(key_value)
        {
            case 0x0001:
                // key1
                break;

            default:
                break;
        }

        Write_Vp(KEY_SCAN_VP, 0);
    }
}