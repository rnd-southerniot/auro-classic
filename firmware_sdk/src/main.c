#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "quadrature_sample.pio.h"

#define LINE_BUF_SIZE 256
#define ENC_L_PIN_BASE 2u
#define ENC_R_PIN_BASE 4u
#define ENCODER_SAMPLE_HZ 2000.0f

typedef struct {
    bool armed;
    float pwm_l;
    float pwm_r;
    int32_t enc_l;
    int32_t enc_r;
} robot_state_t;

typedef struct {
    PIO pio;
    uint sm;
    uint pin_base;
    uint8_t prev_state;
    int32_t count;
} encoder_reader_t;

static uint32_t now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void emit_tele(const robot_state_t *s) {
    printf(
        "{\"type\":\"tele\",\"ts\":%lu,\"mode\":\"%s\",\"armed\":%s,"
        "\"pwm_l\":%.3f,\"pwm_r\":%.3f,\"enc_l\":%ld,\"enc_r\":%ld,"
        "\"rpm_l\":0.0,\"rpm_r\":0.0,\"yaw\":0.0,\"fault\":null}\n",
        (unsigned long)now_ms(),
        s->armed ? "ARMED" : "DISARMED",
        s->armed ? "true" : "false",
        s->pwm_l,
        s->pwm_r,
        (long)s->enc_l,
        (long)s->enc_r
    );
}

static void emit_ack_ok(const char *id, const char *data_json_or_null) {
    if (data_json_or_null) {
        printf("{\"type\":\"ack\",\"id\":\"%s\",\"ok\":true,\"data\":%s}\n", id, data_json_or_null);
    } else {
        printf("{\"type\":\"ack\",\"id\":\"%s\",\"ok\":true}\n", id);
    }
}

static void emit_ack_err(const char *id, const char *code, const char *message) {
    printf(
        "{\"type\":\"ack\",\"id\":\"%s\",\"ok\":false,"
        "\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}\n",
        id,
        code,
        message
    );
}

static bool extract_string_field(const char *line, const char *field, char *out, size_t out_size) {
    const char *p = strstr(line, field);
    const char *start;
    const char *end;
    size_t n;

    if (!p || out_size == 0) {
        return false;
    }
    p = strchr(p, ':');
    if (!p) {
        return false;
    }
    start = strchr(p, '"');
    if (!start) {
        return false;
    }
    start++;
    end = strchr(start, '"');
    if (!end) {
        return false;
    }
    n = (size_t)(end - start);
    if (n >= out_size) {
        n = out_size - 1;
    }
    memcpy(out, start, n);
    out[n] = '\0';
    return true;
}

static bool extract_number_field(const char *line, const char *field, float *value_out) {
    const char *p = strstr(line, field);
    char *endptr;
    float v;
    if (!p) {
        return false;
    }
    p = strchr(p, ':');
    if (!p) {
        return false;
    }
    p++;
    v = strtof(p, &endptr);
    if (endptr == p) {
        return false;
    }
    *value_out = v;
    return true;
}

static int quad_delta(uint8_t prev, uint8_t cur) {
    if (prev == cur) {
        return 0;
    }
    if (
        (prev == 0 && cur == 1) ||
        (prev == 1 && cur == 3) ||
        (prev == 3 && cur == 2) ||
        (prev == 2 && cur == 0)
    ) {
        return 1;
    }
    if (
        (prev == 0 && cur == 2) ||
        (prev == 2 && cur == 3) ||
        (prev == 3 && cur == 1) ||
        (prev == 1 && cur == 0)
    ) {
        return -1;
    }
    return 0;
}

static uint8_t read_encoder_state_from_gpio(uint pin_base) {
    uint8_t a = (uint8_t)gpio_get(pin_base);
    uint8_t b = (uint8_t)gpio_get(pin_base + 1u);
    return (uint8_t)((a << 1u) | b);
}

static void encoder_reader_init(encoder_reader_t *enc, PIO pio, uint sm, uint pin_base, uint offset) {
    pio_sm_config cfg = quadrature_sample_program_get_default_config(offset);

    enc->pio = pio;
    enc->sm = sm;
    enc->pin_base = pin_base;
    enc->count = 0;

    pio_gpio_init(pio, pin_base);
    pio_gpio_init(pio, pin_base + 1u);
    gpio_pull_up(pin_base);
    gpio_pull_up(pin_base + 1u);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, 2, false);

    sm_config_set_in_pins(&cfg, pin_base);
    sm_config_set_clkdiv(&cfg, 125000000.0f / ENCODER_SAMPLE_HZ);

    pio_sm_init(pio, sm, offset, &cfg);
    enc->prev_state = read_encoder_state_from_gpio(pin_base);
    pio_sm_set_enabled(pio, sm, true);
}

static void encoder_reader_poll(encoder_reader_t *enc) {
    while (!pio_sm_is_rx_fifo_empty(enc->pio, enc->sm)) {
        uint32_t raw = pio_sm_get(enc->pio, enc->sm);
        uint8_t cur = (uint8_t)((raw >> 30u) & 0x3u);
        enc->count += quad_delta(enc->prev_state, cur);
        enc->prev_state = cur;
    }
}

static void handle_line(const char *line, robot_state_t *s) {
    char id[64];
    float left = 0.0f;
    float right = 0.0f;

    if (!extract_string_field(line, "\"id\"", id, sizeof(id))) {
        strncpy(id, "?", sizeof(id));
        id[sizeof(id) - 1] = '\0';
    }

    if (!strstr(line, "\"type\":\"cmd\"")) {
        emit_ack_err(id, "bad_type", "expected cmd");
        return;
    }

    if (strstr(line, "\"cmd\":\"ping\"")) {
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"version\"")) {
        emit_ack_ok(id, "{\"fw\":\"sdk-v0\",\"robot\":\"airborne-classic\"}");
        return;
    }
    if (strstr(line, "\"cmd\":\"arm\"")) {
        s->armed = true;
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"disarm\"")) {
        s->armed = false;
        s->pwm_l = 0.0f;
        s->pwm_r = 0.0f;
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"stop\"")) {
        s->pwm_l = 0.0f;
        s->pwm_r = 0.0f;
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"set_pwm\"")) {
        if (!extract_number_field(line, "\"left\"", &left) || !extract_number_field(line, "\"right\"", &right)) {
            emit_ack_err(id, "bad_args", "expected left/right numbers");
            return;
        }
        if (!s->armed) {
            s->pwm_l = 0.0f;
            s->pwm_r = 0.0f;
        } else {
            s->pwm_l = left;
            s->pwm_r = right;
        }
        emit_ack_ok(id, NULL);
        return;
    }

    emit_ack_err(id, "bad_cmd", "unknown cmd");
}

int main(void) {
    absolute_time_t next_tele;
    uint offset;
    uint sm_l;
    uint sm_r;
    encoder_reader_t enc_l_reader;
    encoder_reader_t enc_r_reader;
    char line[LINE_BUF_SIZE];
    size_t line_len = 0;
    robot_state_t state = {.armed = false, .pwm_l = 0.0f, .pwm_r = 0.0f, .enc_l = 0, .enc_r = 0};

    stdio_init_all();
    sleep_ms(1000);

    offset = pio_add_program(pio0, &quadrature_sample_program);
    sm_l = pio_claim_unused_sm(pio0, true);
    sm_r = pio_claim_unused_sm(pio0, true);
    encoder_reader_init(&enc_l_reader, pio0, sm_l, ENC_L_PIN_BASE, offset);
    encoder_reader_init(&enc_r_reader, pio0, sm_r, ENC_R_PIN_BASE, offset);

    emit_tele(&state);
    next_tele = make_timeout_time_ms(100);

    while (true) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\r' || c == '\n') {
                line[line_len] = '\0';
                if (line_len > 0) {
                    handle_line(line, &state);
                }
                line_len = 0;
            } else if (line_len + 1 < LINE_BUF_SIZE) {
                line[line_len++] = (char)c;
            } else {
                line_len = 0;
            }
        }

        encoder_reader_poll(&enc_l_reader);
        encoder_reader_poll(&enc_r_reader);
        state.enc_l = enc_l_reader.count;
        state.enc_r = enc_r_reader.count;

        if (absolute_time_diff_us(get_absolute_time(), next_tele) <= 0) {
            emit_tele(&state);
            next_tele = make_timeout_time_ms(100);
        }

        sleep_ms(1);
    }
}
