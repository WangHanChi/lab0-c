#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "constant.h"
#include "cpucycles.h"
#include "fixture.h"
#include "queue.h"
#include "random.h"

/* Maintain a queue independent from the qtest since
 * we do not want the test to affect the original functionality
 */
static struct list_head *l = NULL;

#define dut_new() ((void) (l = q_new()))

#define dut_size(n)                                \
    do {                                           \
        for (int __iter = 0; __iter < n; ++__iter) \
            q_size(l);                             \
    } while (0)

#define dut_insert_head(s, n)    \
    do {                         \
        int j = n;               \
        while (j--)              \
            q_insert_head(l, s); \
    } while (0)

#define dut_insert_tail(s, n)    \
    do {                         \
        int j = n;               \
        while (j--)              \
            q_insert_tail(l, s); \
    } while (0)

#define dut_free() ((void) (q_free(l)))

static char random_string[N_MEASURES][8];
static int random_string_iter = 0;

/* Implement the necessary queue interface to simulation */
void init_dut(void)
{
    l = NULL;
}

static char *get_random_string(void)
{
    random_string_iter = (random_string_iter + 1) % N_MEASURES;
    return random_string[random_string_iter];
}

void prepare_inputs(uint8_t *input_data, uint8_t *classes)
{
    randombytes(input_data, N_MEASURES * CHUNK_SIZE);
    for (size_t i = 0; i < N_MEASURES; i++) {
        classes[i] = randombit();
        if (classes[i] == 0) {
            memset(input_data, 'a', CHUNK_SIZE - 1);
            input_data[CHUNK_SIZE - 1] = '\0';
        } else {
            uint8_t rand;
            for (size_t i = 0; i < CHUNK_SIZE - 1; i++) {
                randombytes(&rand, 1);
                input_data[i] = (rand % 50) + 65;
            }
            input_data[CHUNK_SIZE - 1] = '\0';
        }
    }
}

bool measure(dudect_ctx_t *ctx, int mode)
{
    assert(mode == DUT(insert_head) || mode == DUT(insert_tail) ||
           mode == DUT(remove_head) || mode == DUT(remove_tail));

    switch (mode) {
    case DUT(insert_head):
        for (size_t i = DROP_SIZE; i < N_MEASURES - DROP_SIZE; i++) {
            char *s = get_random_string();
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (ctx->input_data + i * CHUNK_SIZE) % 10000);
            ctx->before_ticks[i] = cpucycles();
            dut_insert_head(s, 1);
            ctx->after_ticks[i] = cpucycles();
            dut_free();
        }
        break;
    case DUT(insert_tail):
        for (size_t i = DROP_SIZE; i < N_MEASURES - DROP_SIZE; i++) {
            char *s = get_random_string();
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (ctx->input_data + i * CHUNK_SIZE) % 10000);
            ctx->before_ticks[i] = cpucycles();
            dut_insert_tail(s, 1);
            ctx->after_ticks[i] = cpucycles();
            dut_free();
        }
        break;
    case DUT(remove_head):
        for (size_t i = DROP_SIZE; i < N_MEASURES - DROP_SIZE; i++) {
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (ctx->input_data + i * CHUNK_SIZE) % 10000 + 1);
            ctx->before_ticks[i] = cpucycles();
            element_t *e = q_remove_head(l, NULL, 0);
            ctx->after_ticks[i] = cpucycles();
            if (e)
                q_release_element(e);
            dut_free();
        }
        break;
    case DUT(remove_tail):
        for (size_t i = DROP_SIZE; i < N_MEASURES - DROP_SIZE; i++) {
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (ctx->input_data + i * CHUNK_SIZE) % 10000 + 1);
            ctx->before_ticks[i] = cpucycles();
            element_t *e = q_remove_tail(l, NULL, 0);
            ctx->after_ticks[i] = cpucycles();
            if (e)
                q_release_element(e);
            dut_free();
        }
        break;
    default:
        for (size_t i = DROP_SIZE; i < N_MEASURES - DROP_SIZE; i++) {
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (ctx->input_data + i * CHUNK_SIZE) % 10000);
            ctx->before_ticks[i] = cpucycles();
            dut_size(1);
            ctx->after_ticks[i] = cpucycles();
            dut_free();
        }
    }
    return true;
}
