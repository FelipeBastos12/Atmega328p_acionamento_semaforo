/*
Sprint 10
Lógica de acionamento de um semáforo
Matrícula: 119210837
Nome: Felipe Bastos Meneses
*/

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Variáveis globais
uint8_t I= 0;

// Protótipos
void anima_semaforo(uint8_t i);
void Usart_transmit (unsigned char data);

ISR (USART_RX_vect) // Timer que faz uma interrupção a cada 1 ms
{
	anima_semaforo(UDR0 - '0'); 
}
int main(void)
{
	DDRB = 0b11111111;
	DDRD = 0b10000000;
	
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char)MYUBRR;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	sei (); 
	
	while (1)
	{
		//estima_carros();
		//anima_semaforo();
	}
}

void anima_semaforo(uint8_t i)
{
	const uint16_t estados [9] = {0b011110000, 0b001110000, 0b000110000, 0b000010000, 0b000001111, 0b000000111, 0b000000011, 0b000000001, 0b100000000};
	PORTB = estados[i] & 0b011111111;
	if (estados[i] & 0b100000000)
	PORTD |= 0b10000000;
	else
	PORTD &= 0b01111111;	
}
void Usart_trasmit(unsigned char data)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}



void Usart_transmit(unsigned char data)
{
	while (!( UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}



