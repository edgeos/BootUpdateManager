#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <uchar.h>
#include <string.h>
#include <errno.h>
#include "EFIGlue.h"

#include "EFIGlue.h"
#include "BUMState.h"

static int updateComplete(  char        *statedir_name,
                            uint64_t    attemptcount,
                            char        *updateconfig)
{
    int ret;
    BUM_state_t       *BUM_state_p;
    EFI_STATUS stat;
    /*  Assume success */
    ret = 0;
    /*  Acquire the BUM state */
    stat = BUMState_Get(statedir_name, &BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Get failed\n");
        ret = -1;
        goto exit0;
    }
    /*  Perform the complete-update logic. */
    stat = BUMStateNext_CompleteUpdate( BUM_state_p,
                                        attemptcount,
                                        updateconfig);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMStateNext_CompleteUpdate failed\n");
        ret = -1;
        goto exit1;
    }
    /*  Save the BUM state */
    stat = BUMState_Put(statedir_name, BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Put failed\n");
        ret = -1;
        /*goto exit1;*/
    }
    /*  Free the state variable */
exit1:
    stat = BUMState_Free(BUM_state_p);
    if(EFI_ERROR(stat)){
        fprintf(stderr, "    BUMState_Free failed\n");
        ret = -1;
        /*goto exit0;*/
    }
exit0:
    return ret;
}

static int validateargs(char        *attemptcount_str,
                        char        *updateconfig,
                        uint64_t    *attemptcount_p)
{
    size_t len;
    int ret;
    char *endptr;
    unsigned long long attemptcount;
    /*  Check the attempt count*/
    len = strlen(attemptcount_str);
    if(0 == len){
        fprintf(stderr, "    validateargs: "\
                        "attempt count can not be an empty string\n");
        ret = -1;
    }else{
        errno = 0;
        attemptcount = strtoull(attemptcount_str, &endptr, 0);
        if(0 != errno){
            perror( "    validateargs: "\
                    "strtoull failed for attempt count");
            ret = -1;
        }else{
            if('\0' != *endptr){
                fprintf(stderr, "    validateargs: "\
                                "attempt count contains non-digit characters\n");
                ret = -1;
            }else{
                if(0 == attemptcount){
                    fprintf(stderr, "    validateargs: "\
                                    "attempt count must be greater than zero\n");
                    ret = -1;
                }else{
                    /*  Check the new-configuration name */
                    len = strlen(updateconfig);
                    if(0 == len){
                        fprintf(stderr, "    validateargs: "\
                                        "new configuration name must be"\
                                        "a non-empty string.\n");
                        ret = -1;
                    }else{
                        *attemptcount_p = (uint64_t)attemptcount;
                        ret = 0;
                    }
                }
            }
        }
    }
    return ret;
}

static const char *usage =  "<BUM state directory> <default attempt count> "\
                            "<new configuration>";

int main(int argc, char** argv)
{
    int ret;
    char *statedir_str;
    char *attemptcount_str;
    char *updateconfig;
    uint64_t attemptcount;
    if(argc != 4){
        fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
        ret = -1;
    }else{
        statedir_str = argv[1];
        attemptcount_str = argv[2];
        updateconfig = argv[3];
        ret = validateargs( attemptcount_str,
                            updateconfig,
                            &attemptcount);
        if(0 != ret)
            fprintf(stderr, "    validateargs failed\n");
        else{
            ret = updateComplete(   statedir_str,
                                    attemptcount,
                                    updateconfig);
            if(0 != ret)
                fprintf(stderr, "    updateComplete failed\n");
        }
    }
    return ret;
}

