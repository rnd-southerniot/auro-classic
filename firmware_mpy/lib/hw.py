import json
import machine

from machine import Pin, PWM, I2C


class Hardware:
    def __init__(self, cfg: dict):
        self.cfg = cfg
        self.robot_name = cfg.get("robot", {}).get("name", "airborne-classic")
        self.fw_version = cfg.get("robot", {}).get("fw", "mpy-v0")
        self.telemetry_hz = int(cfg.get("control", {}).get("telemetry_hz", 20))

        p = cfg["pins"]

        self._armed = False
        self._pwm_l = 0.0
        self._pwm_r = 0.0

        # Motor driver pins
        self.m1a = PWM(Pin(p["m1a"]))
        self.m1b = PWM(Pin(p["m1b"]))
        self.m2a = PWM(Pin(p["m2a"]))
        self.m2b = PWM(Pin(p["m2b"]))

        pwm_hz = int(cfg.get("control", {}).get("pwm_hz", 20000))
        for m in (self.m1a, self.m1b, self.m2a, self.m2b):
            m.freq(pwm_hz)
            m.duty_u16(0)

        # I2C
        i2c_id = int(p.get("i2c_id", 1))
        self.i2c = I2C(i2c_id, sda=Pin(p["i2c_sda"]), scl=Pin(p["i2c_scl"]), freq=400_000)

        # Encoders (quadrature)
        self.enc_l_a = Pin(p["enc_l_a"], Pin.IN, Pin.PULL_UP)
        self.enc_l_b = Pin(p["enc_l_b"], Pin.IN, Pin.PULL_UP)
        self.enc_r_a = Pin(p["enc_r_a"], Pin.IN, Pin.PULL_UP)
        self.enc_r_b = Pin(p["enc_r_b"], Pin.IN, Pin.PULL_UP)

        self._enc_l = 0
        self._enc_r = 0
        self._enc_l_state = self._read_enc_l_state()
        self._enc_r_state = self._read_enc_r_state()

        irq_trigger = Pin.IRQ_RISING | Pin.IRQ_FALLING
        self.enc_l_a.irq(trigger=irq_trigger, handler=self._on_enc_l)
        self.enc_l_b.irq(trigger=irq_trigger, handler=self._on_enc_l)
        self.enc_r_a.irq(trigger=irq_trigger, handler=self._on_enc_r)
        self.enc_r_b.irq(trigger=irq_trigger, handler=self._on_enc_r)

    @classmethod
    def from_config(cls, path: str) -> "Hardware":
        with open(path, "r") as f:
            cfg = json.load(f)
        return cls(cfg)

    @property
    def armed(self) -> bool:
        return self._armed

    def arm(self, on: bool):
        self._armed = bool(on)
        if not self._armed:
            self.stop()

    def stop(self):
        for m in (self.m1a, self.m1b, self.m2a, self.m2b):
            m.duty_u16(0)
        self._pwm_l = 0.0
        self._pwm_r = 0.0

    def _set_hbridge(self, a: PWM, b: PWM, val: float) -> float:
        # val in [-1,1]
        v = max(-1.0, min(1.0, float(val)))
        mag = int(abs(v) * 65535)

        if not self._armed:
            a.duty_u16(0)
            b.duty_u16(0)
            return 0.0

        # Cytron truth table:
        # 00 brake
        # 10 forward
        # 01 backward
        # 11 coast (Hi-Z)
        if v > 0:
            a.duty_u16(mag)
            b.duty_u16(0)
            return v
        elif v < 0:
            a.duty_u16(0)
            b.duty_u16(mag)
            return v
        else:
            a.duty_u16(0)
            b.duty_u16(0)
            return 0.0

    def set_pwm(self, left: float, right: float):
        self._pwm_l = self._set_hbridge(self.m1a, self.m1b, left)
        self._pwm_r = self._set_hbridge(self.m2a, self.m2b, right)

    def get_pwm(self):
        return self._pwm_l, self._pwm_r

    def i2c_scan(self):
        return [hex(a) for a in self.i2c.scan()]

    @staticmethod
    def _quad_delta(prev: int, cur: int) -> int:
        if prev == cur:
            return 0
        if (
            (prev == 0 and cur == 1)
            or (prev == 1 and cur == 3)
            or (prev == 3 and cur == 2)
            or (prev == 2 and cur == 0)
        ):
            return 1
        if (
            (prev == 0 and cur == 2)
            or (prev == 2 and cur == 3)
            or (prev == 3 and cur == 1)
            or (prev == 1 and cur == 0)
        ):
            return -1
        return 0

    def _read_enc_l_state(self) -> int:
        return (self.enc_l_a.value() << 1) | self.enc_l_b.value()

    def _read_enc_r_state(self) -> int:
        return (self.enc_r_a.value() << 1) | self.enc_r_b.value()

    def _on_enc_l(self, _pin):
        cur = self._read_enc_l_state()
        self._enc_l += self._quad_delta(self._enc_l_state, cur)
        self._enc_l_state = cur

    def _on_enc_r(self, _pin):
        cur = self._read_enc_r_state()
        self._enc_r += self._quad_delta(self._enc_r_state, cur)
        self._enc_r_state = cur

    def get_encoder_counts(self):
        irq_state = machine.disable_irq()
        try:
            return self._enc_l, self._enc_r
        finally:
            machine.enable_irq(irq_state)

    def reset_encoder_counts(self):
        irq_state = machine.disable_irq()
        try:
            self._enc_l = 0
            self._enc_r = 0
        finally:
            machine.enable_irq(irq_state)
