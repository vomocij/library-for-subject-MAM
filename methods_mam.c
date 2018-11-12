#include "methods_mam.h"

// popis metod (koment��e) najdete v methods_mam.h

static uint8_t numbers[] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
		0x80, 0x90 }; // definov�n� jednotliv�ch ��sel na 7. seg
static uint8_t segSel[] = { 0x01, 0x02, 0x04, 0x08 }; // definice jednotliv�ch segment�
static uint8_t interrupts[] = { EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
		EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
		EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
		EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn }; // pole slou��c� pro povolen� p�eru�en�

static struct pinGPIO *LCD_latch = 0; // ukazatel na struktruru, kter� se pln� v metod� 'LCD_Init'
static struct pinGPIO *LCD_clock = 0; // -||-
static struct pinGPIO *LCD_data = 0;  // -||-

static int getGate(GPIO_TypeDef *gate) { // jen pomocn� metoda pro zji�t�n� pozice bitu v registru 'RCC->AHB1ENR' (bohu�el v c�ku nejde pou��t switch nad GPIO_TypeDef)
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
	return -1; // pokud p�ijde n�co jin�ho vra� -1
}

void setUpGPIO(struct pinGPIO *a) {
	if (a->pinNum > 15 || a->pinNum < 0 || a->type > 2 || a->type < 0
			|| a->outputType > 1 || a->outputType < 0 || a->speed > 3
			|| a->speed < 0 || a->pullUpDown > 2 || a->pullUpDown < 0) {
		return;
	} // jen kontrola vstupn�ch dat
	if (a->type == 1 || a->type == 2) {
		if (a->outputType != 0 || a->speed != 0) {
			return;
		}
	} // jen kontrola zda 'outputType' a 'speed' jsou v "nule" pokud je pin zvolen jako vstupn�
	int myGate = getGate(a->gate);
	if (myGate == -1) {
		return;
	} else {
		setbit(RCC->AHB1ENR, myGate); // povolen� hodin pro p��slu�nou br�nu
	}

	switch (a->type) { // nastaven� typu pinu
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
	switch (a->pullUpDown) { // nastaven� pull up/down rezistor�
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

	if (a->type == 0) { // pokud je pin nastaven jako v�stup
		switch (a->outputType) { // nastaven� typu v�stupu
		case 0: // push-pull
			clearbit(a->gate->OTYPER, a->pinNum);
			break;
		case 1: // open drain
			setbit(a->gate->OTYPER, a->pinNum);
			break;
		default:
			return;
		}
		switch (a->speed) { // nastaven� rychlosti pinu
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
		setbit(a->gate->ODR, a->pinNum); // nastaven� defaultn� hodnoty v�stupu na log. 0
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

	setbit(RCC->APB2ENR, 14); //SYSCFGEN povolen� hodin pro syst�movou sb�rnici
	setbit(EXTI->IMR, a->pinNum); // povolen� p�eru�en� od pin� 1 (ale v�echny br�ny)
	if (rising == 1) {
		setbit(EXTI->RTSR, a->pinNum); // volan� p�eru�en� na n�b�nou hranu u pinu 1
	}
	if (falling == 1) {
		setbit(EXTI->FTSR, a->pinNum); // volan� p�eru�en� na n�b�nou hranu u pinu 1
	}
	uint8_t reg = a->pinNum / 4;
	uint8_t pos = a->pinNum % 4;
	if (a->gate == GPIOA) { // Podle br�ny nastav� p��slu�n� registr EXTICR
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

	NVIC_EnableIRQ(interrupts[a->pinNum]); // povolen� p�eru�en�
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
	// osc = 26 000 000 Hz  pro� ?
	setbit(RCC->APB1ENR, 3); // Povolen� hodin pro Timer 5 (Timer 2 by byl bit 0)
	TIM5->PSC = 26;			// p�edd�li�ka
	TIM5->ARR = 10000; // auto reload register do kolika ��ta� ��t�, ne� se resetuje
	NVIC_EnableIRQ(TIM5_IRQn); // povolen� IRQ pro TIM5 v NVIC ?? to nech�pu
	TIM5->DIER |= 0x0001; // DMA/IRQ Enable Register - �e timer muze volat p�eru�en�
	// setbit(TIM5->CR1, 0); // zapnut� ��ta�e TIM5
}

static void serOut(struct pinGPIO *data, struct pinGPIO *clock, uint8_t dataOut) { // samotn� zapsan� (vysl�n�) dat do posuvn�ch registr� (n�zev metody je jen zkratka serialOut :) )
	for (int i = 7; i >= 0; i--) {
		setOutputPin(LCD_data, (dataOut >> i) & 1);
		toggleOutputPin(clock);
		toggleOutputPin(clock);
	}
}

void LCD_Init(struct pinGPIO *latch, struct pinGPIO *data,
		struct pinGPIO *clock) {
	LCD_latch = latch; // p�i�azen� hodnoty ukazatele do statick�ch prom�n�ch pro dal�� obsluhu LCD
	LCD_data = data;	   	 // -||-
	LCD_clock = clock;       // -||-
	setUpGPIO(LCD_latch);	 // nastaven� samotn�ch pin� (z�pis do registr�)
	setUpGPIO(LCD_data);	 // -||-
	setUpGPIO(LCD_clock);    // -||-
	setOutputPin(LCD_latch, 1);  // nastaven� defaultn�ch stav� pinu
	setOutputPin(LCD_data, 0);  // -||-
	setOutputPin(LCD_clock, 0);   // -||-

	// prvotn� nulov�n� displeje
	setOutputPin(LCD_latch, 0);
	serOut(LCD_data, LCD_clock, 0);
	serOut(LCD_data, LCD_clock, 0);
	setOutputPin(LCD_latch, 1);
}

void LCD_WriteNum(uint8_t seg, uint8_t num, uint8_t stateDot) {
	if ((seg < 1 || seg > 4)) {
		return;
	}
	setOutputPin(LCD_latch, 0); // nastaven� Latch do log.0
	if (num >= 0 && num <= 9) {
		if (stateDot == 1) { // pokud m� b�t zobrazena te�ka
			serOut(LCD_data, LCD_clock, numbers[num] - 128); // vysl�n� ��sla
		} else {	// pokud nem� b�t zobrazena te�ka
			serOut(LCD_data, LCD_clock, numbers[num]);
		}
	} else {
		serOut(LCD_data, LCD_clock, 191); // 191 - poml�ka
	}
	serOut(LCD_data, LCD_clock, segSel[seg - 1]); // vysl�n� segmentu
	setOutputPin(LCD_latch, 1); // nastaven� Latch do log.1
}

