#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#define LINE_BUF_SIZE 256

typedef struct {
    bool armed;
    float pwm_l;
    float pwm_r;
} robot_state_t;

static uint32_t now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void emit_tele(const robot_state_t *s) {
    printf(
        "{\"type\":\"tele\",\"ts\":%lu,\"mode\":\"%s\",\"armed\":%s,"
        "\"pwm_l\":%.3f,\"pwm_r\":%.3f,\"enc_l\":0,\"enc_r\":0,"
        "\"rpm_l\":0.0,\"rpm_r\":0.0,\"yaw\":0.0,\"fault\":null}\n",
        (unsigned long)now_ms(),
        s->armed ? "ARMED" : "DISARMED",
        s->armed ? "true" : "false",
        s->pwm_l,
        s->pwm_r
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
    char line[LINE_BUF_SIZE];
    size_t line_len = 0;
    robot_state_t state = {.armed = false, .pwm_l = 0.0f, .pwm_r = 0.0f};

    stdio_init_all();
    sleep_ms(1000);

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

        if (absolute_time_diff_us(get_absolute_time(), next_tele) <= 0) {
            emit_tele(&state);
            next_tele = make_timeout_time_ms(100);
        }

        sleep_ms(1);
    }
}
