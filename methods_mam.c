#include "methods_mam.h"

// popis metod (komentáøe) najdete v methods_mam.h

static uint8_t numbers[] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
		0x80, 0x90 }; // definování jednotlivých èísel na 7. seg
static uint8_t segSel[] = { 0x01, 0x02, 0x04, 0x08 }; // definice jednotlivých segmentù
static uint8_t interrupts[] = { EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
		EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
		EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
		EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn }; // pole sloužící pro povolení pøerušení

static struct pinGPIO *LCD_latch = 0; // ukazatel na struktruru, který se plní v metodì 'LCD_Init'
static struct pinGPIO *LCD_clock = 0; // -||-
static struct pinGPIO *LCD_data = 0;  // -||-

static int getGate(GPIO_TypeDef *gate) { // jen pomocná metoda pro zjištìní pozice bitu v registru 'RCC->AHB1ENR' (bohužel v cèku nejde použít switch nad GPIO_TypeDef)
	if (gate == GPIOA) {
		return 0;
	}
	if (gate == GPIOB) {
		return 1;
	}
	if (gate == GPIOC) {
		return 2;
	}
	if (gate == GPIOD) {
		return 3;
	}
	if (gate == GPIOE) {
		return 4;
	}
	if (gate == GPIOH) {
		return 7;
	}
	return -1; // pokud pøijde nìco jiného vra -1
}

void setUpGPIO(struct pinGPIO *a) {
	if (a->pinNum > 15 || a->pinNum < 0 || a->type > 2 || a->type < 0
			|| a->outputType > 1 || a->outputType < 0 || a->speed > 3
			|| a->speed < 0 || a->pullUpDown > 2 || a->pullUpDown < 0) {
		return;
	} // jen kontrola vstupních dat
	if (a->type == 1 || a->type == 2) {
		if (a->outputType != 0 || a->speed != 0) {
			return;
		}
	} // jen kontrola zda 'outputType' a 'speed' jsou v "nule" pokud je pin zvolen jako vstupní
	int myGate = getGate(a->gate);
	if (myGate == -1) {
		return;
	} else {
		setbit(RCC->AHB1ENR, myGate); // povolení hodin pro pøíslušnou bránu
	}

	switch (a->type) { // nastavení typu pinu
	case 0: // output
		setbit(a->gate->MODER, (2 * a->pinNum));
		clearbit(a->gate->MODER, ((2 * a->pinNum) + 1));
		break;
	case 1: // input
		clearbit(a->gate->MODER, (2 * a->pinNum));
		clearbit(a->gate->MODER, ((2 * a->pinNum) + 1));
		break;
	case 2: // analog input
		setbit(a->gate->MODER, (2 * a->pinNum));
		setbit(a->gate->MODER, ((2 * a->pinNum) + 1));
		break;
	default:
		return;
	}
	switch (a->pullUpDown) { // nastavení pull up/down rezistorù
	case 0: // no pull-up no pull-down
		clearbit(a->gate->PUPDR, (2 * a->pinNum));
		clearbit(a->gate->PUPDR, ((2 * a->pinNum) + 1));
		break;
	case 1: // pull-up
		setbit(a->gate->PUPDR, (2 * a->pinNum));
		clearbit(a->gate->PUPDR, ((2 * a->pinNum) + 1));
		break;
	case 2: // pull-down
		clearbit(a->gate->PUPDR, (2 * a->pinNum));
		setbit(a->gate->PUPDR, ((2 * a->pinNum) + 1));
		break;
	default:
		return;
	}

	if (a->type == 0) { // pokud je pin nastaven jako výstup
		switch (a->outputType) { // nastavení typu výstupu
		case 0: // push-pull
			clearbit(a->gate->OTYPER, a->pinNum);
			break;
		case 1: // open drain
			setbit(a->gate->OTYPER, a->pinNum);
			break;
		default:
			return;
		}
		switch (a->speed) { // nastavení rychlosti pinu
		case 0: // low speed
			clearbit(a->gate->OSPEEDR, (2 * a->pinNum));
			clearbit(a->gate->OSPEEDR, ((2 * a->pinNum) + 1));
			break;
		case 1: // medium speed
			setbit(a->gate->OSPEEDR, (2 * a->pinNum));
			clearbit(a->gate->OSPEEDR, ((2 * a->pinNum) + 1));
			break;
		case 2: // fast speed
			clearbit(a->gate->MODER, (2 * a->pinNum));
			setbit(a->gate->MODER, ((2 * a->pinNum) + 1));
			break;
		case 3: // high speed
			setbit(a->gate->MODER, (2 * a->pinNum));
			setbit(a->gate->MODER, ((2 * a->pinNum) + 1));
			break;
		default:
			return;
		}
		setbit(a->gate->ODR, a->pinNum); // nastavení defaultní hodnoty výstupu na log. 0
	}
}

void setInterrupt(struct pinGPIO *a, uint8_t rising, uint8_t falling) {
	// EXTI0_IRQn		EXTI0_IRQHandler		Handler for pins connected to line 0
	// EXTI1_IRQn		EXTI1_IRQHandler		Handler for pins connected to line 1
	// EXTI2_IRQn		EXTI2_IRQHandler		Handler for pins connected to line 2
	// EXTI3_IRQn		EXTI3_IRQHandler		Handler for pins connected to line 3
	// EXTI4_IRQn		EXTI4_IRQHandler		Handler for pins connected to line 4
	// EXTI9_5_IRQn		EXTI9_5_IRQHandler		Handler for pins connected to line 5 to 9
	// EXTI15_10_IRQn	EXTI15_10_IRQHandler	Handler for pins connected to line 10 to 15

	setbit(RCC->APB2ENR, 14); //SYSCFGEN povolení hodin pro systémovou sbìrnici
	setbit(EXTI->IMR, a->pinNum); // povolení pøerušení od pinù 1 (ale všechny brány)
	if (rising == 1) {
		setbit(EXTI->RTSR, a->pinNum); // volaní pøerušení na nábìžnou hranu u pinu 1
	}
	if (falling == 1) {
		setbit(EXTI->FTSR, a->pinNum); // volaní pøerušení na nábìžnou hranu u pinu 1
	}
	uint8_t reg = a->pinNum / 4;
	uint8_t pos = a->pinNum % 4;
	if (a->gate == GPIOA) { // Podle brány nastaví pøíslušný registr EXTICR
	}
	if (a->gate == GPIOB) {
		setbit(SYSCFG->EXTICR[reg], pos * 4);
	}
	if (a->gate == GPIOC) {
		setbit(SYSCFG->EXTICR[reg], (pos * 4) + 1);
	}
	if (a->gate == GPIOD) {
		setbit(SYSCFG->EXTICR[reg], pos * 4);
		setbit(SYSCFG->EXTICR[reg], (pos * 4) + 1);
	}
	if (a->gate == GPIOE) {
		setbit(SYSCFG->EXTICR[reg], (pos * 4) + 2);
	}
	if (a->gate == GPIOH) {
		setbit(SYSCFG->EXTICR[reg], pos * 4);
		setbit(SYSCFG->EXTICR[reg], (pos * 4) + 1);
		setbit(SYSCFG->EXTICR[reg], (pos * 4) + 2);
	}

	NVIC_EnableIRQ(interrupts[a->pinNum]); // povolení pøerušení
}

void setOutputPin(struct pinGPIO *a, uint8_t state) {
	if (state == 0) {
		clearbit(a->gate->ODR, a->pinNum);
	}
	if (state == 1) {
		setbit(a->gate->ODR, a->pinNum);
	}
}

void toggleOutputPin(struct pinGPIO *a) {
	togglebit(a->gate->ODR, a->pinNum);
}

uint8_t getStateOfPin(struct pinGPIO *a) {
	if (a->pinNum > 15 || a->pinNum < 0) {
		return -1;
	}
	return getbit(a->gate->IDR, a->pinNum);
}

void TIM5_Init() {
	// osc = 26 000 000 Hz  proè ?
	setbit(RCC->APB1ENR, 3); // Povolení hodin pro Timer 5 (Timer 2 by byl bit 0)
	TIM5->PSC = 26;			// pøeddìlièka
	TIM5->ARR = 10000; // auto reload register do kolika èítaè èítá, než se resetuje
	NVIC_EnableIRQ(TIM5_IRQn); // povolení IRQ pro TIM5 v NVIC ?? to nechápu
	TIM5->DIER |= 0x0001; // DMA/IRQ Enable Register - že timer muze volat pøerušení
	// setbit(TIM5->CR1, 0); // zapnutí èítaèe TIM5
}

static void serOut(struct pinGPIO *data, struct pinGPIO *clock, uint8_t dataOut) { // samotné zapsaní (vyslání) dat do posuvných registrù (název metody je jen zkratka serialOut :) )
	for (int i = 7; i >= 0; i--) {
		setOutputPin(LCD_data, (dataOut >> i) & 1);
		toggleOutputPin(clock);
		toggleOutputPin(clock);
	}
}

void LCD_Init(struct pinGPIO *latch, struct pinGPIO *data,
		struct pinGPIO *clock) {
	LCD_latch = latch; // pøiøazení hodnoty ukazatele do statických promìných pro další obsluhu LCD
	LCD_data = data;	   	 // -||-
	LCD_clock = clock;       // -||-
	setUpGPIO(LCD_latch);	 // nastavení samotných pinù (zápis do registrù)
	setUpGPIO(LCD_data);	 // -||-
	setUpGPIO(LCD_clock);    // -||-
	setOutputPin(LCD_latch, 1);  // nastavení defaultních stavù pinu
	setOutputPin(LCD_data, 0);  // -||-
	setOutputPin(LCD_clock, 0);   // -||-

	// prvotní nulování displeje
	setOutputPin(LCD_latch, 0);
	serOut(LCD_data, LCD_clock, 0);
	serOut(LCD_data, LCD_clock, 0);
	setOutputPin(LCD_latch, 1);
}

void LCD_WriteNum(uint8_t seg, uint8_t num, uint8_t stateDot) {
	if ((seg < 1 || seg > 4)) {
		return;
	}
	setOutputPin(LCD_latch, 0); // nastavení Latch do log.0
	if (num >= 0 && num <= 9) {
		if (stateDot == 1) { // pokud má být zobrazena teèka
			serOut(LCD_data, LCD_clock, numbers[num] - 128); // vyslání èísla
		} else {	// pokud nemá být zobrazena teèka
			serOut(LCD_data, LCD_clock, numbers[num]);
		}
	} else {
		serOut(LCD_data, LCD_clock, 191); // 191 - pomlèka
	}
	serOut(LCD_data, LCD_clock, segSel[seg - 1]); // vyslání segmentu
	setOutputPin(LCD_latch, 1); // nastavení Latch do log.1
}

