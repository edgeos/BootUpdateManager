#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include "EFIGlue.h"
#include "EFIGlue.h"
#include "BUMState.h"

static const char *usage = "<BUM status directory>";

void BUMState_Print(BUM_state_t *BUM_state_p,
                    char        *statusdir)
{
    printf("    BUM State: \n");
    printf("        Status directory:       %s\n", statusdir);
    printf("        Current Configuration:  ");
    if(BUM_state_p->Flags.CurrConfig == BUMSTATE_CONFIG_DFLT)
        printf("Default\n");
    else
        printf("Alternate\n");
    printf("        Update Attempt:         ");
    if(BUM_state_p->Flags.UpdateAttempt == 1)
        printf("Yes\n");
    else
        printf("No\n");
    printf("        Default Attempt Count:  %" PRIu64 "\n",
            BUM_state_p->DfltAttemptCount);
    printf("        Default Attempts Rem.:  %" PRIu64 "\n",
            BUM_state_p->DfltAttemptsRemaining);
    printf("        Default Configurtaion:      \"%s\"\n",
            BUM_state_p->DfltConfig);
    printf("        Alternate Configurtaion:    \"%s\"\n",
            BUM_state_p->AltrConfig);
}

int main(int argc, char** argv)
{
    int ret;
    EFI_FILE_PROTOCOL *statusdir;
    BUM_state_t *BUM_state_p;
    EFI_STATUS stat;

    if(argc != 2){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        ret = -1;
    }else{
        statusdir = GetDirFileProtocol(argv[1]);
        stat = BUMState_Get(statusdir, &BUM_state_p);
        if(EFI_ERROR(stat)){
            fprintf(stderr, "   BUMState_Get failed\n");
            ret = -1;
        }else{
            BUMState_Print(BUM_state_p, argv[1]);
            BUMState_Free(BUM_state_p);
            ret = 0;
        }
    }
    return ret;
}

