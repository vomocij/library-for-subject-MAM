// Knihovna pro p�edm�t MAM

#include <stdint.h>
#include "stm32f4xx.h"

#define setbit(reg, bit) ((reg) |= (1U << (bit)))
#define clearbit(reg, bit) ((reg) &= (~(1U << (bit))))
#define togglebit(reg, bit) ((reg) ^= (1U << (bit)))
#define getbit(reg, bit) ((reg & (1U << bit)) >> bit)

// makra pro p�ehlednost definovan� parametr� struktury pinGPIO
#define OUTPUT			0
#define INPUT			1
#define ANALOG_INPUT	2
#define PUSH_PULL		0
#define OPEN_DRAIN		1
#define LOW				0
#define MEDIUM  		1
#define FAST  			2
#define HIGH			3
#define NO_PULLS		0
#define PULL_UP			1
#define PULL_DOWN		2

//struktura pinGPIO slou�� k pops�n� jednotliv�ch GPIO pin�
typedef struct pinGPIO {
	GPIO_TypeDef *gate; // = GPIOA, GPIOB...
	uint8_t pinNum;     // = pin number (0-15)
	uint8_t type;       // = 0 - output, 1 - input, 2 - analog input
	uint8_t outputType; // = 0 - push-pull, 1 - open drain
	uint8_t speed;      // = 0 - low, 1 - medium 2 - fast, 3 - high
	uint8_t pullUpDown; // = 0 - no Pull-up/Pull-down, 1 - pull-up, 2 - pull-down

// Pokud je 'type' nastaven jako 'input' nebo jako 'analog input' (1,2)
// pot� 'outputType' a 'speed' mus� b�t nastaveno na 0!

} pinGPIO;

void setUpGPIO(struct pinGPIO *a); // tato metoda nastavuje samotn� pin (z�pis do p��slu�n�ch registr�)
// *a - ukazatel na struktruru pinGPIO

void setInterrupt(struct pinGPIO *a, uint8_t rising, uint8_t falling); // Tato metoda nastav� p�eru�en� pro p��slu�n� pin
// *a - ukazatel na struktruru pinGPIO
// rising = 1 - p�eru�en� od n�b�n� hrany, 0 - ��dn� p�eru�en� od n�b�n� hrany
// falling = 1 - p�eru�en� od sp�dov� hrany, 0 - ��dn� p�eru�en� od sp�dov� hrany

void setOutputPin(struct pinGPIO *a, uint8_t state); // tato metoda nastav� na pinu 'a' v�stup podle 'state' (log. 0 or log. 1)
// *a - ukazatel na struktruru pinGPIO
// state = 0 nebo 1

// Pin mus� b�t nastaven jako v�stupn� p�ed vol�n�m t�to metody!

uint8_t getStateOfPin(struct pinGPIO *a); // tato metoda vrac� stav aktu�ln�ho vstupn�ho pinu (log.1 nebo log. 0)
// *a - ukazatel na struktruru pinGPIO

// Pin mus� b�t nastaven jako vstupn� p�ed vol�n�m t�to metody!

void toggleOutputPin(struct pinGPIO *a); // tato metoda prohod� 'state' pinu GPIO pin (log.1 nebo log. 0)
// *a - give here pointer to pinGPIO

//Pin mus� b�t nastaven jako v�stupn� p�ed vol�n�m t�to metody!

// P��klad main.c:

/*

 #include "methods_mam\methods_mam.h"
 struct pinGPIO led1 = { GPIOA, 5, OUTPUT, PUSH_PULL, MEDIUM, NO_PULLS };  - definice pinu led1;
 int main(void) {
 setUpGPIO(&led1);                                - snastaven� GPIO podle instance struktury 'led1'
 setOutputPin(&led1, 1);                          - nastaven� GPIO pinu 'led1' na log. 1
 }

 */

// Metody pro ovl�d�n� LCD:
void LCD_Init(struct pinGPIO *Latch, struct pinGPIO *Data,
		struct pinGPIO *Clock); // Tato metoda inicializuje 7 seg. displej (p�ed� se j� ukazatele na jednotliv� (piny) struktury pinGPIO)

void LCD_WriteNum(uint8_t seg, uint8_t num, uint8_t stateDot); // tato metoda zap�e ��slo 'num' na segment 'seg' a m��e zobrazit te�ku nastaven�m 'stateDot' do log. 1
// seg = 1-4, num = 0-9, stateDot = 0-1

void TIM5_Init(); // do�asn� metoda pro nastaven� �asova�e TIM5 a p�eru�en�

