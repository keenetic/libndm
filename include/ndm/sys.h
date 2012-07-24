#ifndef __NDM_SYS_H__
#define __NDM_SYS_H__

#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "macro.h"

bool ndm_sys_init() NDM_WUR;

bool ndm_sys_set_default_signals() NDM_WUR;

int ndm_sys_rand() NDM_WUR;

const char *ndm_sys_strerror(const int error) NDM_WUR;

bool ndm_sys_sleep(const struct timespec *interval);
bool ndm_sys_sleep_msec(const int64_t msec);

const struct timespec *ndm_sys_sleep_granularity() NDM_WUR;

bool ndm_sys_is_interrupted() NDM_WUR;
void ndm_sys_set_interrupted();

#endif	/* __NDM_SYS_H__ */

