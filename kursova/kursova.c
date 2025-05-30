
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>  //Перетворення строки в число або навпаки
/*
#define F_CPU_MK 1000000 //частота проца
#define BAUD 9600UL //швидкість передачі даних UL-unsine long - тип данних
#define UBRR_value (F_CPU_MK/(BAUD*16)) - 1  //12
*/
unsigned char time_val[3] = {0, 0, 0}; 
int digits[6] = {0, 0, 0, 0, 0, 0};
int current_digit = 0;
int button_number = 0;
char counter = 0;
char start = 0;
char off = 0;
int s = 0;
int result = -1;

char setting = 1;
char setting_phase = 0;

/*
void UART_Init(){ //Налаштування ЮАРТ
  UBRRL = UBRR_value; 
  UBRRH = UBRR_value>>8;
  UCSRB = (1<<TXEN); //st 160
  UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
}

void UART_send(char value)//Функція для передачі даних
{
  while(!(UCSRA&(1<<UDRE)));
  UDR = value;
  
}

*/
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

void timer1_init_ctc() {
    TCNT1 = 0;
    OCR1A = 15625; // 1Hz
    TCCR1A = 0;
    TCCR1A |= (1 << FOC1A);
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS11) | (1 << CS10); // prescaler 64
    TIMSK |= (1 << OCIE1A);
    sei();
}



ISR(TIMER1_COMPA_vect) {
    if (start == 1) {
        time_val[0]++;
        if (time_val[0] == 60) {
            time_val[0] = 0;
            time_val[1]++;
        }
        if (time_val[1] == 60) {
            time_val[1] = 0;
            time_val[2]++;	
			for(int i=0; i<time_val[2]; i++){
			led();	
			}			
        }
        if (time_val[2] == 24) {
            time_val[2] = 0;
        }
        digits[5] = time_val[0] % 10;
        digits[4] = time_val[0] / 10;
        digits[3] = time_val[1] % 10;
        digits[2] = time_val[1] / 10;
        digits[1] = time_val[2] % 10;
        digits[0] = time_val[2] / 10;
    }
}

void led(){
		PORTB |= (1 << PB7);
			_delay_ms(300);
			PORTB &= ~(1 << PB7);
			_delay_ms(300);
}

void adc_init() {
    ADMUX |= (1 << REFS0) | (1 << MUX0) | (1 << MUX1) | (1 << MUX2);           
    ADCSRA |= (1 << ADEN) | (1 << ADIE); 
    SFIOR |= (1 << ADTS1) | (1 << ADTS0);
    sei();
}

void button() {
    switch (button_number) {
        case 0: // нічого
            if (off == 1) {
                PORTC = 0b00000000;
                PORTA = 0b00000000;
            }
            break;

        case 1: // встановлення годин/хвилин/секунд
            if (setting) {
                counter++;
                if (setting_phase == 0) { // години
                    if (counter > 23) counter = 0;
                    time_val[2] = counter;
                } else if (setting_phase == 1) { // хвилини
                    if (counter > 59) counter = 0;
                    time_val[1] = counter;
                } else if (setting_phase == 2) { // секунди
                    if (counter > 59) counter = 0;
                    time_val[0] = counter;
                }

                digits[5] = time_val[0] % 10;
                digits[4] = time_val[0] / 10;
                digits[3] = time_val[1] % 10;
                digits[2] = time_val[1] / 10;
                digits[1] = time_val[2] % 10;
                digits[0] = time_val[2] / 10;
				
            }
		
            break;

        case 2: // запуск секундоміра
			s++;
			if(s%2 == 1){
            time_val[0] = time_val[1] = time_val[2] = 0;
            start = 1;
			} else if (s%2 == 0){
				start = 0;
			 digits[5] = time_val[0] % 10;
			digits[4] = time_val[0] / 10;
			digits[3] = time_val[1] % 10;
			digits[2] = time_val[1] / 10;
			digits[1] = time_val[2] % 10;
			digits[0] = time_val[2] / 10;

			}						
            break;

        case 3: // фіксація та перехід до наступного етапу
            if (setting) {
                setting_phase++;
                counter = 0;
                if (setting_phase > 2) {
                    setting = 0;
                    setting_phase = 0;
                    start = 1;
                }
            }
			
            break;

        case 4: // вимикання/вмикання
            off++;
            if (off % 2 == 1) {
                PORTC = 0b00000000;
                PORTA = 0b00000000;
                off = 1;
            } else {
                off = 0;
            }
            break;
    }
}

ISR(ADC_vect) {
    result = ADC; 
    int old_button = button_number;

    if (result == 819)
        button_number = 0;
    else if (result == 0)
        button_number = 1;
    else if (result == 512)
        button_number = 2;    
    else if (result == 683)
        button_number = 3;
    else if (result == 768)
        button_number = 4;
    else
        button_number = -1;
	
    if (button_number != old_button && button_number != -1) {
        button();
		result = 0;
    }
	/*
	  char buf[1];
        itoa(result, buf, 10);   // Перетворення числа на рядок
        for (int i = 0; buf[i] != '\0'; i++) {
            UART_send(buf[i]);
        }
        UART_send('\n');  // Перехід на новий рядок для зручності
    */
}

void timer0_init_ctc() {
    TCCR0 |= (1 << WGM01);          
    TCCR0 |= (1 << CS02);           
    OCR0 = 12;                  
    TIMSK |= (1 << OCIE0); 
}

void timer2_init_ctc() {
    TCCR2 = 0;
    TCCR2 |= (1 << WGM21); // CTC
    OCR2 = 12; 
    TIMSK |= (1 << OCIE2);
    TCCR2 |= (1 << CS22) | (1 << CS21);
}

ISR(TIMER0_COMP_vect) {
    ADCSRA |= (1 << ADSC);
}

ISR(TIMER2_COMP_vect) {
	if(off == 0){
    PORTA = 0x00;
    PORTC = digit_segments[digits[current_digit]];
    PORTA = (1 << current_digit);
    current_digit++;
    if (current_digit >= 6) current_digit = 0;
	}	
	
}

int main() {
    DDRC = 0b11111111;
    DDRA = 0b00111111;
    DDRB = 0b10000000;
    PORTC = 0b00000000;
    PORTA = 0b00000000;
	PORTB = 0b00000000;
	//UART_Init();
    timer1_init_ctc(); 
    timer0_init_ctc(); 
    timer2_init_ctc(); 
    adc_init();


    while (1) {
		
    }
}
