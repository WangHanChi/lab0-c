#ifndef DUDECT_TTEST_H
#define DUDECT_TTEST_H

#include <stdint.h>

typedef struct {
    double mean[2];
    double m2[2];
    double n[2];
} ttest_ctx_t;

void t_push(ttest_ctx_t *ctx, double x, uint8_t class);
double t_compute(ttest_ctx_t *ctx);
void t_init(ttest_ctx_t *ctx);

#endif
