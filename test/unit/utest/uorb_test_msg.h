/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#ifndef _TEST_MESSAGES_H_
#define _TEST_MESSAGES_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Test message structure for simple data
struct test_simple_s {
    uint64_t timestamp;
    int32_t value;
    float data;
    bool flag;
};

// Test message structure for complex data
struct test_complex_s {
    uint64_t timestamp;
    int32_t array[5];
    float matrix[3][3];
    char name[16];
    bool enabled;
};

// Test constants for test data
#define TEST_SIMPLE_VALUE_1  123
#define TEST_SIMPLE_DATA_1   45.67f
#define TEST_SIMPLE_FLAG_1   true

#define TEST_SIMPLE_VALUE_2  456
#define TEST_SIMPLE_DATA_2   78.90f
#define TEST_SIMPLE_FLAG_2   false

// Test message metadata definitions
static const struct orb_metadata test_simple_meta = {
    .o_name = "test_simple",
    .o_size = sizeof(struct test_simple_s),
    .o_size_no_padding = sizeof(struct test_simple_s),
    .o_fields = "uint64_t timestamp;int32_t value;float data;bool flag",
    .o_id = 101
};

static const struct orb_metadata test_complex_meta = {
    .o_name = "test_complex", 
    .o_size = sizeof(struct test_complex_s),
    .o_size_no_padding = sizeof(struct test_complex_s),
    .o_fields = "uint64_t timestamp;int32_t array[5];float matrix[3][3];char name[16];bool enabled",
    .o_id = 102
};

#ifdef __cplusplus
}
#endif

#endif // _TEST_MESSAGES_H_ 