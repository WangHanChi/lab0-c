/** dude, is my code constant time?
 *
 * This file measures the execution time of a given function many times with
 * different inputs and performs a Welch's t-test to determine if the function
 * runs in constant time or not. This is essentially leakage detection, and
 * not a timing attack.
 *
 * Notes:
 *
 *  - the execution time distribution tends to be skewed towards large
 *    timings, leading to a fat right tail. Most executions take little time,
 *    some of them take a lot. We try to speed up the test process by
 *    throwing away those measurements with large cycle count. (For example,
 *    those measurements could correspond to the execution being interrupted
 *    by the OS.) Setting a threshold value for this is not obvious; we just
 *    keep the x% percent fastest timings, and repeat for several values of x.
 *
 *  - the previous observation is highly heuristic. We also keep the uncropped
 *    measurement time and do a t-test on that.
 *
 *  - we also test for unequal variances (second order test), but this is
 *    probably redundant since we're doing as well a t-test on cropped
 *    measurements (non-linear transform)
 *
 *  - as long as any of the different test fails, the code will be deemed
 *    variable time.
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../console.h"
#include "../random.h"

#include "constant.h"
#include "fixture.h"
#include "ttest.h"

#define ENOUGH_MEASURE 10000
#define TEST_TRIES 10

static ttest_ctx_t *t;
static dudect_ctx_t *ctx;
static dudect_config_t *conf;

/* threshold values for Welch's t-test */
enum {
    t_threshold_bananas = 500, /* Test failed with overwhelming probability */
    t_threshold_moderate = 10, /* Test failed */
};

static void __attribute__((noreturn)) die(void)
{
    exit(111);
}

static void differentiate(dudect_ctx_t *ctx)
{
    for (size_t i = 0; i < N_MEASURES; i++)
        ctx->exec_times[i] = ctx->after_ticks[i] - ctx->before_ticks[i];
}

static void update_statistics(dudect_ctx_t *ctx)
{
    for (size_t i = 10 /* discard the first few measurements */;
         i < (ctx->config->number_measurements - 1); i++) {
        int64_t difference = ctx->exec_times[i];

        if (difference < 0) {
            continue;  // the cpu cycle counter overflowed, just throw away the
                       // measurement
        }

        // t-test on the execution time
        t_push(ctx->ttest_ctxs[0], difference, ctx->classes[i]);

        // t-test on cropped execution times, for several cropping thresholds.
        for (size_t crop_index = 0; crop_index < DUDECT_NUMBER_PERCENTILES;
             crop_index++) {
            if (difference < ctx->percentiles[crop_index]) {
                t_push(ctx->ttest_ctxs[crop_index + 1], difference,
                       ctx->classes[i]);
            }
        }

        // second-order test (only if we have more than 10000 measurements).
        // Centered product pre-processing.
        if (ctx->ttest_ctxs[0]->n[0] > 10000) {
            double centered =
                (double) difference - ctx->ttest_ctxs[0]->mean[ctx->classes[i]];
            t_push(ctx->ttest_ctxs[1 + DUDECT_NUMBER_PERCENTILES],
                   centered * centered, ctx->classes[i]);
        }
    }
    // for (size_t i = 0; i < N_MEASURES; i++) {
    //     int64_t difference = exec_times[i];
    //     /* CPU cycle counter overflowed or dropped measurement */
    //     if (difference <= 0)
    //         continue;

    //     /* do a t-test on the execution time */
    //     t_push(t, difference, classes[i]);
    // }
}

static ttest_ctx_t *max_test(dudect_ctx_t *ctx)
{
    size_t ret = 0;
    double max = 0;
    for (size_t i = 0; i < DUDECT_TESTS; i++) {
        if (ctx->ttest_ctxs[i]->n[0] > ENOUGH_MEASURE) {
            double x = fabs(t_compute(ctx->ttest_ctxs[i]));
            if (max < x) {
                max = x;
                ret = i;
            }
        }
    }
    return ctx->ttest_ctxs[ret];
}

static bool report(dudect_ctx_t *ctx)
{
    ttest_ctx_t *t = max_test(ctx);
    double max_t = fabs(t_compute(t));
    double number_traces_max_t = t->n[0] + t->n[1];
    double max_tau = max_t / sqrt(number_traces_max_t);

    printf("\033[A\033[2K");
    printf("meas: %7.2lf M, ", (number_traces_max_t / 1e6));
    if (number_traces_max_t < ENOUGH_MEASURE) {
        printf("not enough measurements (%.0f still to go).\n",
               ENOUGH_MEASURE - number_traces_max_t);
        return false;
    }

    /* max_t: the t statistic value
     * max_tau: a t value normalized by sqrt(number of measurements).
     *          this way we can compare max_tau taken with different
     *          number of measurements. This is sort of "distance
     *          between distributions", independent of number of
     *          measurements.
     * (5/tau)^2: how many measurements we would need to barely
     *            detect the leak, if present. "barely detect the
     *            leak" = have a t value greater than 5.
     */
    printf("max t: %+7.2f, max tau: %.2e, (5/tau)^2: %.2e.\n", max_t, max_tau,
           (double) (5 * 5) / (double) (max_tau * max_tau));

    /* Definitely not constant time */
    if (max_t > t_threshold_bananas)
        return false;

    /* Probably not constant time. */
    if (max_t > t_threshold_moderate)
        return false;

    /* For the moment, maybe constant time. */
    return true;
}
static int cmp(const int64_t *a, const int64_t *b)
{
    return (int) (*a - *b);
}

static int64_t percentile(int64_t *a, double which, size_t size)
{
    qsort(a, size, sizeof(int64_t), (int (*)(const void *, const void *)) cmp);
    size_t array_position = (size_t) ((double) size * (double) which);
    assert(array_position >= 0);
    assert(array_position < size);
    return a[array_position];
}

static void prepare_percentiles(dudect_ctx_t *ctx)
{
    for (size_t i = 0; i < DUDECT_NUMBER_PERCENTILES; i++) {
        ctx->percentiles[i] = percentile(
            ctx->exec_times,
            1 - (pow(0.5, 10 * (double) (i + 1) / DUDECT_NUMBER_PERCENTILES)),
            ctx->config->number_measurements);
    }
}

static bool doit(dudect_ctx_t *ctx, int mode)
{
    if (!ctx->before_ticks || !ctx->after_ticks || !ctx->exec_times ||
        !ctx->classes || !ctx->input_data) {
        die();
    }

    prepare_inputs(ctx->input_data, ctx->classes);

    bool ret = measure(ctx, mode);
    differentiate(ctx);

    bool first_time = ctx->percentiles[DUDECT_NUMBER_PERCENTILES - 1] == 0;
    if (first_time) {
        prepare_percentiles(ctx);
    } else {
        update_statistics(ctx);
        ret &= report(ctx);
    }
    return ret;
}



static int dudect_init(dudect_ctx_t *ctx, dudect_config_t *conf)
{
    ctx->config = calloc(1, sizeof(*conf));
    ctx->config->number_measurements = conf->number_measurements;
    ctx->config->chunk_size = conf->chunk_size;
    ctx->after_ticks =
        calloc(ctx->config->number_measurements, sizeof(int64_t));
    ctx->before_ticks =
        calloc(ctx->config->number_measurements, sizeof(int64_t));
    ctx->exec_times = calloc(ctx->config->number_measurements, sizeof(int64_t));
    ctx->classes = calloc(ctx->config->number_measurements, sizeof(uint8_t));
    ctx->input_data =
        calloc(ctx->config->number_measurements * ctx->config->chunk_size,
               sizeof(uint8_t));

    for (int i = 0; i < DUDECT_TESTS; i++) {
        ctx->ttest_ctxs[i] = (ttest_ctx_t *) calloc(1, sizeof(ttest_ctx_t));
        assert(ctx->ttest_ctxs[i]);
        t_init(ctx->ttest_ctxs[i]);
    }

    ctx->percentiles = calloc(DUDECT_NUMBER_PERCENTILES, sizeof(int64_t));

    assert(ctx->after_ticks);
    assert(ctx->before_ticks);
    assert(ctx->exec_times);
    assert(ctx->classes);
    assert(ctx->input_data);
    assert(ctx->percentiles);

    return 0;
}

static void init_once(void)
{
    init_dut();
    t_init(t);
    dudect_init(ctx, conf);
}

static bool test_const(char *text, int mode)
{
    bool result = false;
    t = malloc(sizeof(ttest_ctx_t));
    ctx = malloc(sizeof(dudect_ctx_t));
    conf = malloc(sizeof(dudect_config_t));
    conf->chunk_size = CHUNK_SIZE;
    conf->number_measurements = N_MEASURES;

    for (int cnt = 0; cnt < TEST_TRIES; ++cnt) {
        printf("Testing %s...(%d/%d)\n\n", text, cnt, TEST_TRIES);

        init_once();
        for (int i = 0; i < ENOUGH_MEASURE / (N_MEASURES - DROP_SIZE * 2) + 1;
             ++i)
            result = doit(ctx, mode);
        printf("\033[A\033[2K\033[A\033[2K");
        if (result)
            break;
    }
    free(t);
    free(ctx);
    free(conf);
    return result;
}

#define DUT_FUNC_IMPL(op)                \
    bool is_##op##_const(void)           \
    {                                    \
        return test_const(#op, DUT(op)); \
    }

#define _(x) DUT_FUNC_IMPL(x)
DUT_FUNCS
#undef _
