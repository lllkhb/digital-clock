﻿#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#define F_CPU_MK 1000000UL 
#define BAUD 4800UL
#define UBRR_value ((F_CPU_MK/(BAUD*16)) - 1)

unsigned char time_val[3] = {0, 0, 0}; 
int digits[6] = {0, 0, 0, 0, 0, 0};
int current_digit = 0;
int button_number = 0;
char counter = 0;
char start = 0;
char off = 0;
int s = 0;
int result = -1;
char setting_phase = 0;
char blink_flag = 0;    
char blink_count = 0;   
char blink_state = 0;   
int blink_timer = 0;    
char buf[32];

void timer0_init_ctc(){
    TCCR0 |= (1 << WGM01);          
    TCCR0 |= (1 << CS02);           
    OCR0 = 12;                  
    TIMSK |= (1 << OCIE0); 
}

void timer1_init_ctc(){
    TCNT1 = 0;
    OCR1A = 15625; // 1 Гц
    TCCR1A = 0;
    TCCR1A |= (1 << FOC1A);
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS11) | (1 << CS10);
    TIMSK |= (1 << OCIE1A);
    sei();
}

void timer2_init_ctc(){
    TCCR2 = 0;
    TCCR2 |= (1 << WGM21); // CTC
    OCR2 = 12; 
    TIMSK |= (1 << OCIE2);
    TCCR2 |= (1 << CS22) | (1 << CS21);
}

void adc_init(){
    ADMUX |= (1 << REFS0) | (1 << MUX0) | (1 << MUX1) | (1 << MUX2);           
    ADCSRA |= (1 << ADEN) | (1 << ADIE); 
    SFIOR |= (1 << ADTS1) | (1 << ADTS0);
    sei();
}

void UART_Init(void){
    UBRRL = UBRR_value;
    UBRRH = (UBRR_value >> 8);
    UCSRB = (1 << TXEN);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

void UART_send(unsigned char value){
    while(!(UCSRA & (1 << UDRE)));
    UDR = value;
}

void UART_send_str(unsigned char *str){
    while(*str != '\0'){
        UART_send(*str++);
    }
}

const int digit_segments[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

void button() {
    switch(button_number){
        case 0: // нічого
			PORTD &= ~(1 << PD2);
            if(off == 1){
                PORTC = 0b00000000;
                PORTA = 0b00000000;
            }
            break;

        case 1: //встановлення годин/хвилин/секунд
                counter++;
                if(setting_phase == 0){
					UART_send_str("Adjust the hours\r\n"); 
                    if(counter > 23) counter = 0;
                    time_val[2] = counter;
                } else if(setting_phase == 1){ 
					UART_send_str("Adjust the minutes\r\n"); 
                    if(counter > 59) counter = 0;
                    time_val[1] = counter;
                } else if(setting_phase == 2){ 
					UART_send_str("Adjust the seconds\r\n"); 
                    if(counter > 59) counter = 0;
                    time_val[0] = counter;
                }

                digits[5] = time_val[0] % 10;
                digits[4] = time_val[0] / 10;
                digits[3] = time_val[1] % 10;
                digits[2] = time_val[1] / 10;
                digits[1] = time_val[2] % 10;
                digits[0] = time_val[2] / 10;
				
            break;

        case 2: //запуск секундоміра
			s++;
			if(s%2 == 1){
				UART_send_str("stopwatch ON\r\n");
				time_val[0] = time_val[1] = time_val[2] = 0;
				start = 1;
			} else if (s%2 == 0){
				UART_send_str("stopwatch OFF\r\n");
				start = 0;
				digits[5] = time_val[0] % 10;
				digits[4] = time_val[0] / 10;
				digits[3] = time_val[1] % 10;
				digits[2] = time_val[1] / 10;
				digits[1] = time_val[2] % 10;
				digits[0] = time_val[2] / 10;
				PORTD = (1 << PD2);
				sprintf(buf, "%d:%d:%d\r\n", time_val[2], time_val[1], time_val[0]);
				UART_send_str(buf);
			}						
            break;

        case 3: //фіксація та перехід до наступного налаштування
                setting_phase++;
                counter = 0;					
                if(setting_phase > 2){
                    setting_phase = 0;
                    start = 1;
                }
            break;

        case 4: //вимикання/вмикання
            off++;
            if(off % 2 == 1){
				UART_send_str("OFF\r\n");
                PORTC = 0b00000000;
                PORTA = 0b00000000;
                off = 1;
            } else{
				UART_send_str("ON\r\n");
                off = 0;
            }
            break;
    }
}

ISR(ADC_vect){
    result = ADC; 
    int old_button = button_number;

    if(result == 819)
        button_number = 0;
    else if(result == 0)
        button_number = 1;
    else if(result == 512)
        button_number = 2;    
    else if(result == 683)
        button_number = 3;
    else if(result == 768)
        button_number = 4;
    else
        button_number = -1;
	
    if(button_number != old_button && button_number != -1){
        button();
		result = 0;
    }
}

ISR(TIMER0_COMP_vect){
    ADCSRA |= (1 << ADSC);
	 if(blink_flag){
        blink_timer++;
        if(blink_timer >= 100){
            blink_timer = 0;
            if(blink_state == 0){
                PORTD |= (1 << PD2);
                blink_state = 1;
            } else{
                PORTD &= ~(1 << PD2);
                blink_state = 0;
                blink_count--;
                if(blink_count <= 0){
                    blink_flag = 0;
                }
            }
        }
    }
}

ISR(TIMER1_COMPA_vect){
    if(start == 1){
        time_val[0]++;
        if(time_val[0] == 60){
            time_val[0] = 0;
            time_val[1]++;
			if(time_val[1] < 60){
				sprintf(buf, "%d:%d\r\n", time_val[2], time_val[1]);
				UART_send_str(buf);
			}			
        }
        if(time_val[1] == 60){
            time_val[1] = 0;
            time_val[2]++;
			if(time_val[2] < 24){
				sprintf(buf, "%d:00\r\n", time_val[2]);
				UART_send_str(buf);				
				blink_flag = 1;
				blink_count = time_val[2]; 
				blink_timer = 0;
				blink_state = 0;
			}			
        }
        if(time_val[2] == 24){
            time_val[2] = 0;
			UART_send_str("00:00\r\n");
        }
        digits[5] = time_val[0] % 10;
        digits[4] = time_val[0] / 10;
        digits[3] = time_val[1] % 10;
        digits[2] = time_val[1] / 10;
        digits[1] = time_val[2] % 10;
        digits[0] = time_val[2] / 10;
    }
}

ISR(TIMER2_COMP_vect){
	if(off == 0){
		PORTA = 0x00;
		PORTC = digit_segments[digits[current_digit]];
		PORTA = (1 << current_digit);
		current_digit++;
		if(current_digit >= 6) 
			current_digit = 0;
	}	
	
}

int main(){
    DDRC = 0b11111111;
    DDRA = 0b00111111;
    DDRD = 0b00000100;
    PORTC = 0b00000000;
    PORTA = 0b00000000;
	PORTD = 0b00000000;
	
	timer0_init_ctc(); 
    timer1_init_ctc(); 
    timer2_init_ctc(); 
    adc_init();
	UART_Init();

	UART_send_str("System ready\r\n");
    while(1){
					
    }
}
