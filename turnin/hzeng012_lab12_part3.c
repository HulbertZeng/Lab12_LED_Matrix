 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #12  Exercise #3
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: https://youtu.be/5SU5GZOhIdw
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "timer.h"
#include "scheduler.h"

void transmit_data(unsigned char data, unsigned char gnds) {
    int i;
    for (i = 0; i < 8 ; ++i) {
         // Sets SRCLR to 1 allowing data to be set
         // Also clears SRCLK in preparation of sending data
         PORTC = 0x08;
         PORTD = 0x08;
         // set SER = next bit of data to be sent.
         PORTC |= ((data >> i) & 0x01);
         PORTD |= ((gnds >> i) & 0x01);
         // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
         PORTC |= 0x02;
         PORTD |= 0x02;
    }
    // set RCLK = 1. Rising edge copies data from â€œShiftâ€ register to â€œStorageâ€ register
    PORTC |= 0x04;
    PORTD |= 0x04;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
    PORTD = 0x00;
}

// shared task variables
unsigned short patterns[] = {0x3C, 0x24, 0x3C};
unsigned short rows[] = {0xEF, 0xDF, 0xBF}; //ground displays only the 5 most significant bits
unsigned char i = 0;

// display hollow rectangle
enum Rect_States{ display };
int Rect(int state) {

    switch(state) {
        case display: state = display; break;
        default: state = display; break;
    }
    switch(state) {
        case display:
            if(i >= 3) {
                i = 0;
            }
            transmit_data(patterns[i], rows[i]);
            ++i;
            break;
    }
    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    /* Insert your solution below */
    static task task1; //static task variables
    task *tasks[] = { &task1 };
    const unsigned short numTasks = sizeof(tasks)/sizeof(*tasks);
    const char start = -1;

    // Hollow rectangle
    task1.state = start;
    task1.period = 1;
    task1.elapsedTime = task1.period;
    task1.TickFct = &Rect;


    unsigned short i;

    unsigned long GCD = tasks[0]->period;
    for(i = 1; i < numTasks; i++) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();
    while (1) {
        for(i = 0; i < numTasks; i++) {
            if(tasks[i]->elapsedTime == tasks[i]->period) {
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += GCD;
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
