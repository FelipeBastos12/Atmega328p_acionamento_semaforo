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
#include "PCD8544\nokia5110.h"

// Novos tipos
typedef enum enum_parametros {Modo, Tempo_verde, Tempo_vermelho, Tempo_amarelo, Size_enum_parametros} enum_parametros;
typedef struct stc_semaforo
{
	uint8_t modo;
	uint16_t tempo_verde_ms;
	uint16_t tempo_vermelho_ms;
	uint16_t tempo_amarelo_ms;
}stc_semaforo;

// Variáveis globais
uint16_t modo = 0;
uint32_t tempo_ms = 0;
uint32_t tempo_carros=0;
uint32_t tempo_ADC=0;
uint16_t carros = 0;
uint16_t c_min = 0;
uint16_t ADC_aux = 0;
uint16_t pessoas = 0;
enum_parametros selecao_parametro = Modo;
stc_semaforo semaforo = {.modo=0, .tempo_verde_ms=5000, .tempo_vermelho_ms=3000, .tempo_amarelo_ms=1000};
	
// Protótipos
void anima_semaforo(stc_semaforo Semaforo, uint32_t Tempo_ms);
void anima_LCD(uint16_t Tempo_verde_ms , uint16_t Tempo_vermelho_ms, uint16_t Tempo_amarelo_ms);
void estima_carros();
void pwm ();
void Usart_trasmit (unsigned char data);
void HCSR04trigger ();

ISR (TIMER0_COMPA_vect) // Timer que faz uma interrupção a cada 1 ms
{
	tempo_ms++;
	tempo_carros++;
	tempo_ADC++;
}

ISR (INT0_vect) // Interrupção externa INT0 para o fluxo de veícuos 
{
	if (tempo_carros <=1000) // A cada pulso a variável carros é incrementada de 1
	carros++;
}

ISR (PCINT2_vect) //Interrupção PCINT2 para os botões -, + e s 
{	
	if(!(PIND&(1<<5))) // Apenas uma das bordas aciona o código da interrupção
	{
		switch (selecao_parametro) // Tratamento do botão -
		{
			case Modo: // Os botões de - e + mudam o modo entre manual e atuomático
			if (modo)
			{
				modo = 0;
				break;
			}
			else if (!modo)
			{
				modo = 1;
				break;
			}
			case Tempo_verde: //Tempo verde
			if(semaforo.tempo_verde_ms>=2000)
			semaforo.tempo_verde_ms -= 1000;
			break;
			case Tempo_vermelho: //Tempo vermelho
			if (semaforo.tempo_vermelho_ms>=2000)
			{
			if (semaforo.tempo_vermelho_ms > (semaforo.tempo_amarelo_ms+1000)) // Não permite o tempo vermelho menor que o amarelo
			{
				semaforo.tempo_vermelho_ms -= 1000;
			}
			}
			break;
			case Tempo_amarelo: //Tempo Amarelo
			if (semaforo.tempo_amarelo_ms>=2000)
			semaforo.tempo_amarelo_ms-=1000;
			break;
		}
	}
	if(!(PIND&(1<<4))) // Apenas uma das bordas aciona o código da interrupção
	{
		switch (selecao_parametro) // Tratamento do botão +
		{
			case Modo: // Os botões de - e + mudam o modo entre manual e atuomático
			if (modo)
			{
				modo = 0;
				break;
			}
			else if (!modo)
			{
				modo = 1;
				break;
			}
			case Tempo_verde: //Tempo verde
			if(semaforo.tempo_verde_ms<=8000)
			semaforo.tempo_verde_ms += 1000;
			break;
			case Tempo_vermelho: //Tempo vermelho
			if (semaforo.tempo_vermelho_ms<=8000)
			semaforo.tempo_vermelho_ms += 1000;
			break;
			case Tempo_amarelo: //Tempo amarelo
			if (semaforo.tempo_amarelo_ms<=8000)
			{
				if (semaforo.tempo_amarelo_ms<(semaforo.tempo_vermelho_ms-1000)) // Não permite o tempo vermelho menor que o amarelo
				{
					semaforo.tempo_amarelo_ms+=1000;
				}
			}
			break;
		}
	}
	if(!(PIND&(1<<6)))
	{
		if (selecao_parametro<(Size_enum_parametros-1) && modo==1) // Só é possível selecionar um parâmetro caso o modo esteja 0, que equivale ao modo manual
		{
			selecao_parametro++;
		}
		else
		{
			selecao_parametro=Modo;
		}
	}
}

int main(void)
{
	// Definições de GPIO
	DDRC &= ~(1<<6); // C6 como entrada
	PORTC |= (1<<6); // Pull up ativado
	DDRC &= ~(1<<0); // C0 e como entrada
	PORTC &= ~(1<<0); // Pull up desativado
	DDRB = 0b11111111; // Todos os pinos B como saída
	DDRD &= ~(1<<2); // D2 como entrada
	DDRD |= (1<<0);  //D0 como saída
	DDRD |= (1<<3); // D3 como saída
	DDRD &= ~(1<<7); // D7 como entrada
 
	for (int h=4;h<=6;h++)
	{
		DDRD &= ~(1<<h);
		PORTD |= (1<<h);
	} // D4, D5 e D6 como entrada e pull ups ativados
	
	// Configurações das interrupções externas
	EICRA = 0b00001010;
	EIMSK = 0b00000011;
	PCICR = 0b00000100;
	PCMSK2= 0b01110000;
	sei();
	
	// Definições do timer, que irá fazer interrupções a cada 1 ms
	TCCR0A = 0b00000010;
	TCCR0B = 0b00000011;
	OCR0A = 249;
	TIMSK0 = 0b00000010;
	
	// Definições do ADC
	ADMUX = 0b01000000;
	ADCSRA = 0b11100111;
	ADCSRB = 0b00000000;
	DIDR0 = 0b00000001;
	
	// Definições do PWM
	TCCR2A = 0b00100011;
	TCCR2B = 0b00000110;
	OCR2B = 128;
	
	//Configurações do USART
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char)MYUBRR;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	
	// Inicialização do LCD
	nokia_lcd_init();
	anima_LCD(semaforo.tempo_verde_ms,semaforo.tempo_vermelho_ms, semaforo.tempo_amarelo_ms);
	
	while (1)
	{
		estima_carros(); // Função para estimar o fluxo de carros
		pwm(); // Função para manipular o pwm
		anima_semaforo(semaforo, tempo_ms); // Função para animar o semáforo
	}
}

void anima_semaforo(stc_semaforo Semaforo, uint32_t Tempo_ms )
{	
	const uint16_t estados [9] = {0b00010010, 0b00000010, 0b00011100, 0b00001100, 0b00000100, 0b00011000, 0b00001000, 0b00110000, 0b00010100};
	static int8_t I_M = 0, I_E = 0;
	static uint32_t tempo_anterior_ms_M = 0 , tempo_anterior_ms_E = 0;
	
	PORTB = estados [I_M] & 0b11111111;
	
	if(I_M<=3) // Os 4 primeiros estados ligam os leds verdes
	{ 
		PORTB &= ~(1<<0); // Semáforo para pedestres vermelho
		if ((Tempo_ms-tempo_anterior_ms_M)>=(Semaforo.tempo_verde_ms/4))
		{
			I_M++;
			tempo_anterior_ms_M+=(Semaforo.tempo_verde_ms/4);
		}
	}
	else
	{
		if(I_M<=4) // Os 5 primeiros estados liga o led amarelo
		{
			if ((Tempo_ms-tempo_anterior_ms_M)>=(Semaforo.tempo_amarelo_ms))
			{
				I_M++;
				tempo_anterior_ms_M+=(Semaforo.tempo_amarelo_ms);
			}
		}
		else
		{
			if(I_M<=8) // Os últimos 4 estados ligam os leds vermelhos
			{
					PORTB |= (1<<0); // Semáforo para pedestres verde
					HCSR04trigger(); // Função para verificar se algum veículo avançou o semáforo vermelho
				if((Tempo_ms-tempo_anterior_ms_M)>=(Semaforo.tempo_vermelho_ms/4))
				{
					I_M++;
					tempo_anterior_ms_M+=(Semaforo.tempo_vermelho_ms/4);
				}
			}
			else
			{
				I_M=0; // Inicia o ciclo de novo
				tempo_anterior_ms_M = Tempo_ms;
				tempo_anterior_ms_E = Tempo_ms; // Tempos igualados para sincronizar o semáforo mestre e escravo
			}
		}
	}
	if (I_E<=3) // Estados referentes ao semáforo escravo 
	{
		if ((Tempo_ms-tempo_anterior_ms_E)>=(Semaforo.tempo_verde_ms+Semaforo.tempo_amarelo_ms)/4)
		{
			I_E++;
			tempo_anterior_ms_E += ((Semaforo.tempo_verde_ms+Semaforo.tempo_amarelo_ms)/4);
			Usart_trasmit('0'+I_E); // Função para enviar os dados pela porta COM
		}
	}
	else
	{
		if (I_E<=7) // Estados referentes ao semáforo escravo 
		{
			if ((Tempo_ms-tempo_anterior_ms_E)>=(Semaforo.tempo_vermelho_ms-Semaforo.tempo_amarelo_ms)/4)
			{
				I_E++;
				tempo_anterior_ms_E+=((Semaforo.tempo_vermelho_ms-Semaforo.tempo_amarelo_ms)/4);
				Usart_trasmit('0'+I_E); // Função para enviar os dados pela porta COM
			}
		}
		else
		{
			if (I_E<=8) // Estados referentes ao semáforo escravo  
			{
				if ((Tempo_ms - tempo_anterior_ms_E)>=(Semaforo.tempo_amarelo_ms))
				{
					I_E++;
					tempo_anterior_ms_E += (Semaforo.tempo_amarelo_ms);
					Usart_trasmit('0'+I_E); // Função para enviar os dados pela porta COM
				}
			}
			else
			{
				I_E=0; // inicia o ciclo de novo no semáforo escravo
				Usart_trasmit('0'+I_E); // Envia os dados para o semáforo escravo iniciar o ciclo de novo
			}
		}
	}
}
void HCSR04trigger ()
{
	uint32_t tempo_execucao;
	
	PORTB |= (1<<5);
	_delay_us(10);
	PORTB &= ~(1<<5); // Pulso de 10us no Trigger
	
	for (int i=0; i<600000; i++) // Laço para aguardar o ínicio da borda do echo
	{
		if (!(PIND&(1<<7)))
		continue;
		else
		break;
	}
	// Definições do Timer para contar a cada 1us		
	TCCR1A = 0x00;
	TCCR1B = (1<<CS11);
	TCNT1 = 0x00;
	
	for(int i=0;i<600000;i++) // Laço que funciona enquanto a borda do echo estiver em nível alto							
	{
		if(PIND & (1<<7))							
		{
			if(TCNT1 > 60000) 
			break;					
			else
			 continue;								
		}
		else
		break;											
	}
	
	tempo_execucao = TCNT1>>1; // Salva o valor da contagem na variável tempo_execucao
	
	TCCR1B = 0x00; // Zera a contagem 
	
	if(tempo_execucao < 11600) // Comparador para saber se o veículo está a 2 metros de distância do sensor 
	{
		PORTB |= (1<<7);
	}
	else
	PORTB &=~(1<<7);
	
	
}
void pwm()
{
	ADC_aux = 1023000/ADC-1000;
	if (ADC_aux>300)
	{
		OCR2B = 0;
	}
	else
	{
		if  ((!(PINC&(1<<6))) || (c_min>0))
		{
			OCR2B = 255;	
		}
		else
		{
			OCR2B = 85;
		}
	}
	anima_LCD(semaforo.tempo_verde_ms, semaforo.tempo_vermelho_ms, semaforo.tempo_amarelo_ms);
}

void estima_carros()
{
	if (tempo_carros >= 1000) // Quando a variável tempo_carros atinge 1 segundo é feito o tratamento de informações do fluxo de carros
	{
		c_min = carros *60;
		carros =0;
		tempo_carros =0;
		if (modo == 0)
		{
			semaforo.tempo_amarelo_ms = 1000;
			 uint16_t amarelo_aux = (semaforo.tempo_amarelo_ms/1000)+30;
			semaforo.tempo_verde_ms = ((c_min/60)+1)*1000;
			 uint16_t verde_aux = (semaforo.tempo_verde_ms/1000)+10;
			semaforo.tempo_vermelho_ms = (-(c_min/60)+9)*1000;
			 uint16_t vermelho_aux = (semaforo.tempo_vermelho_ms/1000)+20;
		}
	}
}

void Usart_trasmit(unsigned char data)
{
	while (!( UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void anima_LCD(uint16_t Tempo_verde_ms , uint16_t Tempo_vermelho_ms, uint16_t Tempo_amarelo_ms) //Função para manipular as informações mostradas no LCD
{
	unsigned char tempo_verde_s_string[2]; //Variáveis usadas para converter os números em strings
	unsigned char tempo_vermelho_s_string[2];
	unsigned char tempo_amarelo_s_string[2];
	unsigned char c_min_string[4];
	unsigned char pessoas_aux[2];
	unsigned char lux_LDR[4];
	
	sprintf (tempo_verde_s_string, "%u", Tempo_verde_ms/1000); //Funções para converter os números em strings
	sprintf(tempo_vermelho_s_string, "%u", Tempo_vermelho_ms/1000);
	sprintf (tempo_amarelo_s_string, "%u", Tempo_amarelo_ms/1000);
	sprintf (c_min_string, "%u", c_min);
	sprintf (pessoas_aux, "%u", pessoas);
	sprintf (lux_LDR, "%u", ADC_aux);
		
	nokia_lcd_clear(); //Funções para mostrar as informações no LCD
	
	nokia_lcd_set_cursor(0,10);
	
	nokia_lcd_set_cursor(0,10);
	nokia_lcd_write_string("Modo",1);
	nokia_lcd_set_cursor(32,10);
	if (modo)
	{
		nokia_lcd_write_string("M",1);
	}
	if (!modo)
	{
		nokia_lcd_write_string("A",1);
	}
	nokia_lcd_set_cursor(0,20);
	nokia_lcd_write_string("T.Vd",1);
	nokia_lcd_set_cursor(32,20);
	nokia_lcd_write_string(tempo_verde_s_string,1);
	nokia_lcd_set_cursor(0,30);
	nokia_lcd_write_string("T.Vm",1);
	nokia_lcd_set_cursor(32,30);
	nokia_lcd_write_string(tempo_vermelho_s_string,1);
	nokia_lcd_set_cursor(0,40);
	nokia_lcd_write_string("T.Am",1);
	nokia_lcd_set_cursor(32,40);
	nokia_lcd_write_string(tempo_amarelo_s_string,1);
	nokia_lcd_set_cursor(52,30);
	nokia_lcd_write_string(c_min_string,1);
	nokia_lcd_set_cursor(52,41);
	nokia_lcd_write_string("C/min",1);
	nokia_lcd_set_cursor(52,5);
	nokia_lcd_write_string(lux_LDR,1);
	nokia_lcd_set_cursor(52,15);
	nokia_lcd_write_string("Lux",1);
	
	nokia_lcd_set_cursor(40,10+selecao_parametro*10);
	nokia_lcd_write_string("<",1);
	
	nokia_lcd_render();
}