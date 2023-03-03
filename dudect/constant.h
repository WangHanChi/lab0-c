#ifndef DUDECT_CONSTANT_H
#define DUDECT_CONSTANT_H

#include <stdbool.h>
#include <stdint.h>
#include "ttest.h"
/* Number of measurements per test */
#define N_MEASURES 150

/* Allow random number range from 0 to 65535 */
#define CHUNK_SIZE 16

#define DROP_SIZE 20

#define DUT_FUNCS  \
    _(insert_head) \
    _(insert_tail) \
    _(remove_head) \
    _(remove_tail)

#define DUT(x) DUT_##x

enum {
#define _(x) DUT(x),
    DUT_FUNCS
#undef _
};


/* Macro */
#define DUDECT_NUMBER_PERCENTILES (100)
#define DUDECT_TESTS (1 + DUDECT_NUMBER_PERCENTILES + 1)

/* struct */
typedef struct {
    size_t chunk_size;
    size_t number_measurements;
} dudect_config_t;

typedef struct {
    int64_t *before_ticks;
    int64_t *after_ticks;
    int64_t *exec_times;
    uint8_t *input_data;
    uint8_t *classes;
    dudect_config_t *config;
    ttest_ctx_t *ttest_ctxs[DUDECT_TESTS];
    int64_t *percentiles;
} dudect_ctx_t;


void init_dut();
void prepare_inputs(uint8_t *input_data, uint8_t *classes);
bool measure(dudect_ctx_t *ctx, int mode);

#endif
