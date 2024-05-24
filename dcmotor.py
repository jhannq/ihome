import RPi.GPIO as GPIO
import time

GPIO.setwarnings(False)

GPIO.setmode(GPIO.BCM)
enable = 26
pin1 = 19
pin2 = 13

GPIO.setup(enable, GPIO.OUT)
GPIO.setup(pin1, GPIO.OUT)
GPIO.setup(pin2, GPIO.OUT)

cycle = 100

if cycle >= 0 and cycle <= 100:
    GPIO.output(pin1, 1)
    GPIO.output(pin2, 0)
    pwm = GPIO.PWM(enable, 100)
    pwm.start(0)
    pwm.ChangeDutyCycle(cycle)
    GPIO.output(enable, 1)
    time.sleep(5)
    GPIO.output(enable, 0)
    GPIO.cleanup()
else:
    cycle = 50
