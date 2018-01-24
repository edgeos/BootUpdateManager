#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include "EFIGlue.h"
#include "EFIGlue.h"
#include "BUMState.h"

static const char *usage = "<BUM state directory>";

int main(int argc, char** argv)
{
    int ret;
    BUM_state_t *BUM_state_p;
    char Config[BUMSTATE_CONFIG_MAXLEN];
    EFI_STATUS stat;

    if(argc != 2){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        ret = -1;
    }else{
        stat = BUMState_Get(argv[1], &BUM_state_p);
        if(EFI_ERROR(stat)){
            fprintf(stderr, "   BUMState_Get failed\n");
            ret = -1;
        }else{
            stat = BUMState_getNonCurrConfig(BUM_state_p, Config);
            if(EFI_ERROR(stat)){
                fprintf(stderr, "   BUMState_Get failed\n");
                ret = -1;
            }else{
                Config[BUMSTATE_CONFIG_MAXLEN-1] = '\0';
                BUMState_Free(BUM_state_p);
                ret = 0;
            }
        }
    }
    if(0 == ret)
        printf("%s", Config);
    else
        fputs("", stdout);
    return ret;
}

