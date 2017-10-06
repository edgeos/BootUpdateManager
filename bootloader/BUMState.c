/* LogPrint.c - Implement most BUMState logic.
 *              NOTE:   This code is meant to be compiled as a part of both
 *                      an EFI application and user-space utilities.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 */

/*  Need to include __BUMState.h as <> rather than "" to ensure the correct
    header file is included when compiling for user space instead of EFI space
*/
#include <__BUMState.h>

#define ASTATE_FILENAME L"A.state"
#define BSTATE_FILENAME L"B.state"

static EFI_STATUS CopyConfig(   OUT CHAR8 Dst[static BUMSTATE_CONFIG_MAXLEN],
                                IN  CHAR8 *Src)
{
    EFI_STATUS ret;
    UINTN len = AsciiStrnLenS(Src, BUMSTATE_CONFIG_MAXLEN);
    if(BUMSTATE_CONFIG_MAXLEN == len){
        ret = EFI_INVALID_PARAMETER;
    }else{
        AsciiStrCpy(Dst, Src);
        ret = EFI_SUCCESS;
    }
    return ret;
}

static UINT64 BUMState_GetSum(  IN  BUM_state_t *state_p)
{
    UINT64 *buffer, len, i, sum;
    len = state_p->StateSize / sizeof(UINT64);
    buffer = (UINT64*)state_p;
    for(i=0, sum=0; i<len; i++) sum += buffer[i];
    return sum;
}

static VOID BUMState_SetSum(IN  BUM_state_t *state_p)
{
    UINT64 sum;
    state_p->Checksum=0;
    sum = BUMState_GetSum(state_p);
    state_p->Checksum = -sum;
}

#define BUMState_SumInvalid(state_p) (0 != BUMState_GetSum(state_p))

EFI_STATUS EFIAPI BUMState_Init(IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                IN  CHAR8               *Config)
{
    EFI_STATUS ret;
    BUM_state_t state;
    /*  Zero-out state*/
    ZeroMem((VOID*)&state, sizeof(state));
    /*  Write to the B.state file */
    ret = Common_CreateWriteCloseFile(  BootStatDir,
                                        BSTATE_FILENAME,
                                        (VOID*)&state,
                                        0);
    if(EFI_ERROR(ret))
        goto exit0;
    /*  Populate the A.state file */
    state.StateUpdateCounter = 1;
    state.StateSize = sizeof(BUM_state_t);
    state.Version   = BUMSTATE_VERSION;
    /* state.Flags.raw = 0 */;
    /*  Always initializing to an unsafe boot state
            so we're in alternate
            and attempt count is 0
    */
    state.Flags.CurrConfig  = BUMSTATE_CONFIG_ALTR;
    state.DfltAttemptCount  = 0;
    /* state.DfltConfig[128] = {0}; */
    ret = CopyConfig(state.AltrConfig, Config);
    if(EFI_ERROR(ret))
        goto exit0;
    /*  Set state.AtomicityChecksum; */
    BUMState_SetSum(&state);
    /*  Write to the A.state file */
    ret = Common_CreateWriteCloseFile(  BootStatDir,
                                        ASTATE_FILENAME,
                                        (VOID*)&state,
                                        sizeof(state));
exit0:
    return ret;
}

static EFI_STATUS BUMState_ParseFile(   IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                        IN  CHAR16              *FileName,
                                        OUT BUM_state_t         **BUM_state_pp)
{
    EFI_STATUS ret;
    VOID    *buffer;
    UINTN   buffer_size;
    BUM_state_t *BUM_state_p;
    /*  Read the file */
    ret = Common_OpenReadCloseFile( BootStatDir,
                                    FileName,
                                    &buffer,
                                    &buffer_size );
    if(EFI_ERROR(ret))
        goto exit0;
    /*  Check the size */
    BUM_state_p = (BUM_state_t*)buffer;
    if(buffer_size != BUM_state_p->StateSize){
        ret = EFI_LOAD_ERROR;
        goto exit1;
    }
    /*  Check the checksum */
    if(BUMState_SumInvalid(BUM_state_p)){
        ret = EFI_LOAD_ERROR;
        goto exit1;
    }
    /*  May need to check version number in the future. We're good for now. */
    ret = EFI_SUCCESS;
exit1:
    if(EFI_ERROR(ret)){
        /*  Free the buffer on error */
        Common_FreeReadBuffer(  buffer,
                                buffer_size);
    }else{
        /*  Set the output buffer */
        *BUM_state_pp = BUM_state_p;
    }
exit0:
    return ret;
}

typedef struct {
    BUM_state_t *state[2];
    UINT8       curr;
    UINT8       next;
    #define BUMSTATE_A_IDX (0)
    #define BUMSTATE_B_IDX (1)
} BUM_state_pair_t;

#define BUMStatePair_Invalid(pair)  (((pair)->state[BUMSTATE_A_IDX] == NULL)\
                                    && ((pair)->state[BUMSTATE_B_IDX] == NULL))

static VOID BUMStatePair_Get(   IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                OUT BUM_state_pair_t    *BUM_state_pair_p)
{
    EFI_STATUS ret;
    /*  Get A.state */
    ret = BUMState_ParseFile(   BootStatDir,
                                ASTATE_FILENAME,
                                &(BUM_state_pair_p->state[BUMSTATE_A_IDX]));
    if(EFI_ERROR(ret))
        BUM_state_pair_p->state[BUMSTATE_A_IDX] = NULL;
    /*  Get B.state */
    ret = BUMState_ParseFile(   BootStatDir,
                                BSTATE_FILENAME,
                                &(BUM_state_pair_p->state[BUMSTATE_B_IDX]));
    if(EFI_ERROR(ret))
        BUM_state_pair_p->state[BUMSTATE_B_IDX] = NULL;
    /*  Set curr and next */
    if(NULL == BUM_state_pair_p->state[BUMSTATE_A_IDX]){
        if(NULL == BUM_state_pair_p->state[BUMSTATE_B_IDX]){
            /*  Neither state files is valid */
            BUM_state_pair_p->curr = BUMSTATE_A_IDX;
            BUM_state_pair_p->next = BUMSTATE_A_IDX;
        }else{
            /*  Only the B state file is valid */
            BUM_state_pair_p->curr = BUMSTATE_B_IDX;
            BUM_state_pair_p->next = BUMSTATE_A_IDX;
        }
    }else{
        if(NULL == BUM_state_pair_p->state[BUMSTATE_B_IDX]){
            /*  Only the A state file is valid */
            BUM_state_pair_p->curr = BUMSTATE_A_IDX;
            BUM_state_pair_p->next = BUMSTATE_B_IDX;
        }else{
            /*  Both A and B state files valid */
            UINT64 Acounter =
                BUM_state_pair_p->state[BUMSTATE_A_IDX]->StateUpdateCounter;
            UINT64 Bcounter =
                BUM_state_pair_p->state[BUMSTATE_B_IDX]->StateUpdateCounter;
            if(Acounter < Bcounter){
                /* B state file is newer than A state file */
                BUM_state_pair_p->curr = BUMSTATE_B_IDX;
                BUM_state_pair_p->next = BUMSTATE_A_IDX;
            }else{
                /* B state file is not newer than A state file */
                BUM_state_pair_p->curr = BUMSTATE_A_IDX;
                BUM_state_pair_p->next = BUMSTATE_B_IDX;
            }
        }
    }
}

EFI_STATUS EFIAPI BUMState_Free(IN  BUM_state_t *BUM_state_p)
{
    /*  Free the buffer on error */
    return  Common_FreeReadBuffer(  (VOID*)BUM_state_p,
                                    BUM_state_p->StateSize);
}

EFI_STATUS EFIAPI BUMState_Get( IN  EFI_FILE_PROTOCOL   *BootStatDir,
                                OUT BUM_state_t         **BUM_state_pp)
{
    EFI_STATUS ret;
    BUM_state_pair_t    BUM_state_pair;
    BUMStatePair_Get(   BootStatDir,
                        &BUM_state_pair);
    if(BUMStatePair_Invalid(&BUM_state_pair)){
        ret = EFI_NOT_FOUND;
    }else{
        *BUM_state_pp = BUM_state_pair.state[BUM_state_pair.curr];
        if(NULL != BUM_state_pair.state[BUM_state_pair.next])
            BUMState_Free(BUM_state_pair.state[BUM_state_pair.next]);
        ret = EFI_SUCCESS;
    }
    return ret;
}

#if 0

EFI_STATUS BUMState_Put(IN  EFI_FILE_PROTOCOL   *BootStatDir,
                        IN  BUM_state_t         *BUM_state_p);

EFI_STATUS BUMStateNext_BootTime(   IN  BUM_state_t *BUM_state_p);

EFI_STATUS BUMStateNext_RunTimeInit(IN  BUM_state_t *BUM_state_p);

EFI_STATUS BUMStateNext_StartUpdate(IN  BUM_state_t *BUM_state_p);

EFI_STATUS BUMStateNext_CompleteUpdate( IN  BUM_state_t *BUM_state_p,
                                        IN  UINTN       AttemptCount,
                                        IN  CHAR8       *UpdateConfig);

EFI_STATUS BUMState_getCurrConfig(  IN  BUM_state_t *BUM_state_p,
                                    OUT CHAR8       **CurrConfig_pp);

EFI_STATUS BUMState_getNonCurrConfig(   IN  BUM_state_t *BUM_state_p,
                                        OUT CHAR8       **CurrConfig_pp);
#endif

