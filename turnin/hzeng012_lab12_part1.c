 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #12  Exercise #1
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: Youtube URL>
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "timer.h"
#include "scheduler.h"

void transmit_data(unsigned char data, unsigned char side) {
    int i;
    for(i = 0; i < 8; ++i) {
        // Sets SRCLR to 1 allowing data to be set
        // Also clears SRCLK in preparation of sending data
        if(side == 1) {
            PORTC = 0x08;
            // set SER = next bit of data to be sent.
            PORTC |= ((data >> i) & 0x01);
            // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
            PORTC |= 0x02;
        } else {
            PORTD = 0x08;
            // set SER = next bit of data to be sent.
            PORTD |= ((data >> i) & 0x01);
            // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
            PORTD |= 0x02;
        }

    }
    if(side == 1) {
        // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
        PORTC |= 0x04;
    } else {
        // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
        PORTD |= 0x04;
    }
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
    PORTD = 0x00;
}

// shared task variables

//--------------------------------------
// LED Matrix Demo SynchSM
// Period: 100 ms
//--------------------------------------
enum Demo_States {shift};
int Demo_Tick(int state) {

    // Local Variables
    static unsigned char pattern = 0x80;    // LED pattern - 0: LED off; 1: LED on
    static unsigned char row = 0xFE;      // Row(s) displaying pattern. 
                            // 0: display pattern on row
                            // 1: do NOT display pattern on row
    // Transitions
    switch (state) {
        case shift:    
            break;
        default:    
            state = shift;
            break;
    }    
    // Actions
    switch (state) {
            case shift:    
            if (row == 0xEF && pattern == 0x01) { // Reset demo 
                pattern = 0x80;            
                row = 0xFE;
            } else if (pattern == 0x01) { // Move LED to start of next row
                pattern = 0x80;
                row = (row << 1) | 0x01;
            } else { // Shift LED one spot to the right on current row
                pattern >>= 1;
            }
            break;
        default:
    break;
    }
    transmit_data(pattern, 1);    // Pattern to display
    transmit_data(row, 2);        // Row(s) displaying pattern     
    return state;
}


enum Rows_States {rows_wait, rows_up, rows_down, rows_buffer};
int Rows_Tick(int state) {
    unsigned char up = (~PINA) & 0x02;
    unsigned char down = (~PINA) & 0x01;
    static unsigned char pattern = 0xFF;
    static unsigned char row = 0xFB;

    switch(state) {
        case rows_wait: 
            if(up) {
                state = rows_up;
            } else if(down) {
                state = rows_down;
            } else {
                state = rows_wait;
            }
            break;
        case rows_up: state = rows_buffer; break;
        case rows_down: state = rows_buffer; break;
        case rows_buffer:
            if(!up && !down) {
                state = rows_wait;
            } else {
                state = rows_buffer;
            }
            break;
        default: state = rows_wait; break;
    }
    switch(state) {
        case rows_wait: break;
        case rows_up:
            if(row <= 0xFE) {
                row = (row >> 1) | 0x10;
            }
            break;
        case rows_down: 
            if(row >= 0xEF) {
                row = (row << 1) | 0x01;
            }
            break;
        case rows_buffer: break;
    }
    transmit_data(pattern, 1);    // Pattern to display
    transmit_data(row, 2);        // Row(s) displaying pattern
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

    // LED Matrix
    task1.state = start;
    task1.period = 100;
    task1.elapsedTime = task1.period;
    task1.TickFct = &Demo_Tick;

    // Shift Rows
    task2.state = start;
    task2.period = 100;
    task2.elapsedTime = task2.period;
    task2.TickFct = &Rows_Tick;


    unsigned short i;

    unsigned long GCD = tasks[0]->period;
    for(i = 1; i < numTasks; i++) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();
    while (1) {
        for(i = 1; i < numTasks; i++) {
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
