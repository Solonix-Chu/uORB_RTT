/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include "uorb_demo_topics.h"

ORB_DEFINE(orb_test, struct orb_test_s, sizeof(struct orb_test_s), "uint64_t timestamp;int32 val;", 0);
ORB_DEFINE(sensor_demo, struct sensor_demo_s, sizeof(struct sensor_demo_s), "uint64_t timestamp;int32 x;int32 y;int32 z;", 1); 