#include "sm_common.h"
#include "logger.h"

static const char * TAG = "SM_COMMON";

static uint32_t sm_common_timeo;

void sm_common_reset_timeo(void)
{
  sm_common_timeo = 0;
}

bool sm_common_test_timeo(uint32_t timeo)
{
  NEO_LOGD(TAG, "sm_common_timeo %d timeo %d", sm_common_timeo, timeo);
  if(sm_common_timeo > timeo) {
    return true;
  }
  sm_common_timeo ++;
  return false;
}