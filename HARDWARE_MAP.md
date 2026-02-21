# Airborne Classic — HARDWARE_MAP (source of truth)

## Chassis / kinematics (LOCKED TO PRINTED CAD)

Source: 3MF CAD files in `cad/`.

- Wheel diameter (CAD bbox): **~54.875 mm** (treat as 55 mm)
- Wheel thickness (CAD bbox): **~9.125 mm**
- Track width (CAD assembly, wheel-center to wheel-center): **~155.0 mm** *(computed)*
- Wheelbase (measured): **62 mm** (drive axle center → caster contact center)

> Current CAD-derived frame overall extents (PCA-aligned bbox):
> - Length/width extents (two largest): ~223.78 mm and ~222.30 mm
> - Height extent (smallest): ~38.50 mm
>
> TODO (still needs extraction of axle center positions):
> - Track width (wheel-center to wheel-center)
> - Wheelbase (axle-center to axle-center)
> - Encoder mounting geometry

## Controller: Cytron Maker Pi RP2040 (Pico)

### Motor driver pins (locked)
- M1A = GP8
- M1B = GP9
- M2A = GP10
- M2B = GP11

Truth table:
- 00 brake
- 10 forward
- 01 backward
- 11 coast (Hi-Z)

PWM frequency target: ~20 kHz

### Onboard peripherals
- WS2812 RGB: GP18
- Buttons: GP20, GP21
- Buzzer: GP22
- Servo signals: GP12–GP15

## Encoders (quadrature, locked approach)

### Pin assignment (chosen)
- Left: ENC_L_A = GP2, ENC_L_B = GP3
- Right: ENC_R_A = GP4, ENC_R_B = GP5

### Decoder strategy
- MicroPython bring-up: IRQ-based counting (low speed)
- PicoSDK production: **PIO quadrature**

### Calibration
- counts-per-wheel-rev: measured by rotating wheel 1 turn (3 trials)
- wheel diameter: locked to CAD (~55 mm)

## IMU: ICM-20948

### Bus (chosen)
- I2C1: SDA=GP6, SCL=GP7

> TODO: confirm ICM-20948 I2C address on your breakout and whether AD0 is strapped.

## Power
- TBD (battery type/voltage for car)
