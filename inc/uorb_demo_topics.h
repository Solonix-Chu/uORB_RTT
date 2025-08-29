/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#ifndef __UORB_DEMO_TOPICS_H__
#define __UORB_DEMO_TOPICS_H__

#include "uORB.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct orb_test_s {
    uint64_t timestamp;
    int32_t  val;
};

struct sensor_demo_s {
    uint64_t timestamp;
    int32_t  x;
    int32_t  y;
    int32_t  z;
};

ORB_DECLARE(orb_test);
ORB_DECLARE(sensor_demo);

#ifdef __cplusplus
}
#endif

#endif /* __UORB_DEMO_TOPICS_H__ */ 