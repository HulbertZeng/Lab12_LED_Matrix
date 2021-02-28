 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #12  Exercise #4 and 5
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: https://youtu.be/-wI6l-twOUM
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


// move hollow rectangle
enum Move_Rect_States{ wait, up_state, down_state, left_state, right_state, buffer };

int MoveRect(int state) {
    unsigned char up = (~PINA) & 0x08;
    unsigned char down = (~PINA) & 0x04;
    unsigned char left = (~PINA) & 0x02;
    unsigned char right = (~PINA) & 0x01;
    static signed char x = 0;
    static signed char y = 0;

    switch(state) {
        case wait:
            if(up) {
                state = up_state;
            } else if(down) {
                state = down_state;
            } else if(left) {
                state = left_state;
            } else if(right) {
                state = right_state;
            } else {
                state = wait;
            }
            break;
        case up_state: state = buffer; break;
        case down_state: state = buffer; break;
        case left_state: state = buffer; break;
        case right_state: state = buffer; break;
        case buffer:
            if(!up && !down && !left && !right) {
                state = wait;
            } else {
                state = buffer;
            }
            break;
        default: state = wait; break;
    }
    switch(state) {
        case wait: break;
        case up_state:
            if(y < 1) {
                ++y;
                for(unsigned char j = 0; j < 3; ++j) {
                    rows[j] = (rows[j] >> 1) | 0x80;
                }
            }
            break;
        case down_state:
            if(y > -1) {
                --y;
                for(unsigned char j = 0; j < 3; ++j) {
                    rows[j] = (rows[j] << 1) | 0x01;
                }
            }
            break;
        case left_state:
            if(x > -2) {
                --x;
                for(unsigned char j = 0; j < 3; ++j) {
                    patterns[j] = patterns[j] << 1;
                }
            }
            break;
        case right_state:
            if(x < 2) {
                ++x;
                for(unsigned char j = 0; j < 3; ++j) {
                    patterns[j] = patterns[j] >> 1;
                }
            }
            break;
        case buffer: break;
    }
    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    /* Insert your solution below */
    static task task1, task2; //static task variables
    task *tasks[] = { &task1, &task2 };
    const unsigned short numTasks = sizeof(tasks)/sizeof(*tasks);
    const char start = -1;

    // Hollow rectangle
    task1.state = start;
    task1.period = 1;
    task1.elapsedTime = task1.period;
    task1.TickFct = &Rect;

    // Move hollow rectangle
    task2.state = start;
    task2.period = 50;
    task2.elapsedTime = task2.period;
    task2.TickFct = &MoveRect;

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
