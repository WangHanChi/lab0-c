#ifndef DUDECT_FIXTURE_H
#define DUDECT_FIXTURE_H

#include <stdbool.h>
#include "constant.h"
#include "ttest.h"



/* func */
// int dudect_init(dudect_ctx_t *ctx, dudect_config_t *conf);
// static int64_t percentile(int64_t *a, double which, size_t size);
// static void prepare_percentiles(dudect_ctx_t *ctx);
// static t_context_t *max_test(dudect_ctx_t *ctx);

/* Interface to test if function is constant */
#define _(x) bool is_##x##_const(void);
DUT_FUNCS
#undef _

#endif
