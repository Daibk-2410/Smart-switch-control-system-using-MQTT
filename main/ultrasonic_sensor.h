// main/ultrasonic_sensor.h
#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

void ultrasonic_init(int trigger_pin, int echo_pin);
float ultrasonic_get_distance_cm();

#endif