/*
 * Ethan Chen
 * UCLA CS M213A
 * 2011 Fall
 */

#include "mbed.h"

Serial pc(USBTX, USBRX);

PwmOut l1(LED1);
PwmOut l2(LED2);
PwmOut l3(LED3);
PwmOut l4(LED4);

int main() {
    pc.baud(9600);
    pc.printf("Ethan Chen\n");
    
    l1.period_ms(100);
    l2.period_ms(100);
    l3.period_ms(100);
    l4.period_ms(100);
    
    l1.pulsewidth_ms(10);
    l2.pulsewidth_ms(25);
    l3.pulsewidth_ms(50);
    l4.pulsewidth_ms(75);
}
