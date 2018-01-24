/* BUMState.h - BUM-State data structure definitions and headers for helper
 *              functions.
 *
 * Author: Safayet N Ahmed (GE Global Research) <Safayet.Ahmed@ge.com>
 *
 * Copyright (c) 2017, General Electric Company. All rights reserved.
 */

#ifndef __BUM_STATE__
#define __BUM_STATE__

#define BUMSTATE_VERSION (0)
#define BUMSTATE_CONFIG_MAXLEN (128)
#define BUMSTATE_CONFIG_SIZE (sizeof(CHAR8) * BUMSTATE_CONFIG_MAXLEN)

static inline BOOLEAN BUMState_configIsValid(   IN  CHAR8 *Config)
{
    UINTN len = AsciiStrnLenS(Config, BUMSTATE_CONFIG_MAXLEN);
    return (len > 0) && (len < BUMSTATE_CONFIG_MAXLEN);
}

static inline EFI_STATUS CopyConfig(OUT CHAR8 Dst[static BUMSTATE_CONFIG_MAXLEN],
                                    IN  CHAR8 *Src)
{
    EFI_STATUS ret;
    if(BUMState_configIsValid(Src)){
        AsciiStrCpy(Dst, Src);
        ret = EFI_SUCCESS;
    }else
        ret = EFI_INVALID_PARAMETER;
    return ret;
}

typedef struct {
    UINT64  StateUpdateCounter;
    UINT64  StateSize;
    UINT64  Version;
    union {
        struct {
            UINT64  CurrConfig      : 1;
            #define BUMSTATE_CONFIG_DFLT (0)
            #define BUMSTATE_CONFIG_ALTR (1)
            UINT64  UpdateAttempt   : 1;
            UINT64  Reserved        : 62;
        };
        UINT64  raw;
    } Flags;
    UINT64  DfltAttemptCount;
    UINT64  DfltAttemptsRemaining;
    CHAR8   DfltConfig[BUMSTATE_CONFIG_MAXLEN];
    CHAR8   AltrConfig[BUMSTATE_CONFIG_MAXLEN];
    UINT64  Checksum;
} BUM_state_t;

EFI_STATUS EFIAPI BUMState_Init(IN  CHAR8   *BootStatDirPath,
                                IN  CHAR8   *Config);

EFI_STATUS EFIAPI BUMState_Get( IN  CHAR8       *BootStatDirPath,
                                OUT BUM_state_t **BUM_state_pp);

EFI_STATUS EFIAPI BUMState_Put( IN  CHAR8       *BootStatDirPath,
                                IN  BUM_state_t *BUM_state_p);

EFI_STATUS EFIAPI BUMState_Free(IN  BUM_state_t *BUM_state_p);

VOID EFIAPI BUMStateNext_StartUpdate(IN  BUM_state_t *BUM_state_p);

EFI_STATUS EFIAPI BUMStateNext_CompleteUpdate(  IN  BUM_state_t *BUM_state_p,
                                                IN  UINT64      AttemptCount,
                                                IN  CHAR8       *UpdateConfig);

VOID EFIAPI BUMStateNext_BootTime(IN  BUM_state_t *BUM_state_p);

VOID EFIAPI BUMStateNext_RunTimeInit( IN  BUM_state_t *BUM_state_p);

EFI_STATUS EFIAPI BUMState_getCurrConfig(
                        IN  BUM_state_t  *BUM_state_p,
                        OUT CHAR8        Dst[static BUMSTATE_CONFIG_MAXLEN]);

EFI_STATUS EFIAPI BUMState_getNonCurrConfig(
                        IN  BUM_state_t *BUM_state_p,
                        OUT CHAR8 Dst[static BUMSTATE_CONFIG_MAXLEN]);

#endif
