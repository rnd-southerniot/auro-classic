#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "quadrature_sample.pio.h"

#define LINE_BUF_SIZE 256
#define ENC_L_PIN_BASE 2u
#define ENC_R_PIN_BASE 4u
#define ENCODER_SAMPLE_HZ 2000.0f
#define MOTOR_L_A_PIN 8u
#define MOTOR_L_B_PIN 9u
#define MOTOR_R_A_PIN 10u
#define MOTOR_R_B_PIN 11u
#define MOTOR_PWM_HZ 20000u
#define MOTOR_PWM_WRAP 6249u
#define ENC_L_SIGN 1
#define ENC_R_SIGN -1
// Calibrated from three 1-rev hand-turn trials (Feb 22, 2026):
// left: 576, 586, 585 -> 582.33 ; right: 575, 591, 584 -> 583.33
#define COUNTS_PER_WHEEL_REV_L 582.0f
#define COUNTS_PER_WHEEL_REV_R 583.0f
// Open-loop RPM command range mapped to PWM [-1, 1].
#define OPEN_LOOP_MAX_ABS_RPM 200.0f

typedef struct {
    bool armed;
    float pwm_l;
    float pwm_r;
    int32_t enc_l;
    int32_t enc_r;
    float rpm_l;
    float rpm_r;
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
        "\"rpm_l\":%.3f,\"rpm_r\":%.3f,\"yaw\":0.0,\"fault\":null}\n",
        (unsigned long)now_ms(),
        s->armed ? "ARMED" : "DISARMED",
        s->armed ? "true" : "false",
        s->pwm_l,
        s->pwm_r,
        (long)s->enc_l,
        (long)s->enc_r,
        s->rpm_l,
        s->rpm_r
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

static float counts_to_rpm(int32_t delta_counts, uint32_t delta_ms, float counts_per_rev) {
    if (delta_ms == 0u || counts_per_rev <= 0.0f) {
        return 0.0f;
    }
    return ((float)delta_counts * 60000.0f) / (counts_per_rev * (float)delta_ms);
}

static float clamp_unit(float v) {
    if (v > 1.0f) {
        return 1.0f;
    }
    if (v < -1.0f) {
        return -1.0f;
    }
    return v;
}

static float rpm_to_pwm(float rpm) {
    if (OPEN_LOOP_MAX_ABS_RPM <= 0.0f) {
        return 0.0f;
    }
    return clamp_unit(rpm / OPEN_LOOP_MAX_ABS_RPM);
}

static uint16_t unit_to_pwm_level(float v) {
    float mag = fabsf(clamp_unit(v));
    return (uint16_t)(mag * (float)MOTOR_PWM_WRAP);
}

static void drive_hbridge(uint pin_a, uint pin_b, float v) {
    uint16_t level = unit_to_pwm_level(v);
    // Cytron truth table:
    // 00 brake
    // 10 forward
    // 01 backward
    // 11 coast (Hi-Z)
    if (v > 0.0f) {
        pwm_set_gpio_level(pin_a, level);
        pwm_set_gpio_level(pin_b, 0u);
    } else if (v < 0.0f) {
        pwm_set_gpio_level(pin_a, 0u);
        pwm_set_gpio_level(pin_b, level);
    } else {
        pwm_set_gpio_level(pin_a, 0u);
        pwm_set_gpio_level(pin_b, 0u);
    }
}

static void apply_motor_outputs(const robot_state_t *s) {
    float left = s->armed ? clamp_unit(s->pwm_l) : 0.0f;
    float right = s->armed ? clamp_unit(s->pwm_r) : 0.0f;
    drive_hbridge(MOTOR_L_A_PIN, MOTOR_L_B_PIN, left);
    drive_hbridge(MOTOR_R_A_PIN, MOTOR_R_B_PIN, right);
}

static void motor_pwm_init(void) {
    uint pins[] = {MOTOR_L_A_PIN, MOTOR_L_B_PIN, MOTOR_R_A_PIN, MOTOR_R_B_PIN};
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        gpio_set_function(pins[i], GPIO_FUNC_PWM);
    }

    uint slice_l = pwm_gpio_to_slice_num(MOTOR_L_A_PIN);
    uint slice_r = pwm_gpio_to_slice_num(MOTOR_R_A_PIN);
    pwm_set_wrap(slice_l, MOTOR_PWM_WRAP);
    pwm_set_clkdiv(slice_l, 1.0f);
    pwm_set_enabled(slice_l, true);

    pwm_set_wrap(slice_r, MOTOR_PWM_WRAP);
    pwm_set_clkdiv(slice_r, 1.0f);
    pwm_set_enabled(slice_r, true);

    pwm_set_gpio_level(MOTOR_L_A_PIN, 0u);
    pwm_set_gpio_level(MOTOR_L_B_PIN, 0u);
    pwm_set_gpio_level(MOTOR_R_A_PIN, 0u);
    pwm_set_gpio_level(MOTOR_R_B_PIN, 0u);
}

static void handle_line(
    const char *line,
    robot_state_t *s,
    encoder_reader_t *enc_l_reader,
    encoder_reader_t *enc_r_reader,
    int32_t *last_enc_l,
    int32_t *last_enc_r,
    uint32_t *last_tele_ts
) {
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
            s->pwm_l = clamp_unit(left);
            s->pwm_r = clamp_unit(right);
        }
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"set_rpm\"")) {
        if (!extract_number_field(line, "\"left\"", &left) || !extract_number_field(line, "\"right\"", &right)) {
            emit_ack_err(id, "bad_args", "expected left/right numbers");
            return;
        }
        if (!s->armed) {
            s->pwm_l = 0.0f;
            s->pwm_r = 0.0f;
        } else {
            s->pwm_l = rpm_to_pwm(left);
            s->pwm_r = rpm_to_pwm(right);
        }
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"cal_encoders\"")) {
        enc_l_reader->count = 0;
        enc_r_reader->count = 0;
        s->enc_l = 0;
        s->enc_r = 0;
        s->rpm_l = 0.0f;
        s->rpm_r = 0.0f;
        *last_enc_l = 0;
        *last_enc_r = 0;
        *last_tele_ts = now_ms();
        emit_ack_ok(id, NULL);
        return;
    }
    if (strstr(line, "\"cmd\":\"cal_imu\"")) {
        emit_ack_ok(id, "{\"status\":\"noop\"}");
        return;
    }

    emit_ack_err(id, "bad_cmd", "unknown cmd");
}

int main(void) {
    absolute_time_t next_tele;
    uint offset;
    uint sm_l;
    uint sm_r;
    uint32_t last_tele_ts;
    int32_t last_enc_l;
    int32_t last_enc_r;
    encoder_reader_t enc_l_reader;
    encoder_reader_t enc_r_reader;
    char line[LINE_BUF_SIZE];
    size_t line_len = 0;
    robot_state_t state = {
        .armed = false,
        .pwm_l = 0.0f,
        .pwm_r = 0.0f,
        .enc_l = 0,
        .enc_r = 0,
        .rpm_l = 0.0f,
        .rpm_r = 0.0f
    };

    stdio_init_all();
    sleep_ms(1000);
    motor_pwm_init();

    offset = pio_add_program(pio0, &quadrature_sample_program);
    sm_l = pio_claim_unused_sm(pio0, true);
    sm_r = pio_claim_unused_sm(pio0, true);
    encoder_reader_init(&enc_l_reader, pio0, sm_l, ENC_L_PIN_BASE, offset);
    encoder_reader_init(&enc_r_reader, pio0, sm_r, ENC_R_PIN_BASE, offset);

    last_tele_ts = now_ms();
    last_enc_l = 0;
    last_enc_r = 0;

    emit_tele(&state);
    next_tele = make_timeout_time_ms(100);

    while (true) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\r' || c == '\n') {
                line[line_len] = '\0';
                if (line_len > 0) {
                    handle_line(
                        line,
                        &state,
                        &enc_l_reader,
                        &enc_r_reader,
                        &last_enc_l,
                        &last_enc_r,
                        &last_tele_ts
                    );
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
        state.enc_l = ENC_L_SIGN * enc_l_reader.count;
        state.enc_r = ENC_R_SIGN * enc_r_reader.count;

        if (absolute_time_diff_us(get_absolute_time(), next_tele) <= 0) {
            uint32_t tele_ts = now_ms();
            uint32_t delta_ms = tele_ts - last_tele_ts;
            int32_t delta_l = state.enc_l - last_enc_l;
            int32_t delta_r = state.enc_r - last_enc_r;

            state.rpm_l = counts_to_rpm(delta_l, delta_ms, COUNTS_PER_WHEEL_REV_L);
            state.rpm_r = counts_to_rpm(delta_r, delta_ms, COUNTS_PER_WHEEL_REV_R);

            last_tele_ts = tele_ts;
            last_enc_l = state.enc_l;
            last_enc_r = state.enc_r;

            emit_tele(&state);
            next_tele = make_timeout_time_ms(100);
        }

        apply_motor_outputs(&state);
        sleep_ms(1);
    }
}
