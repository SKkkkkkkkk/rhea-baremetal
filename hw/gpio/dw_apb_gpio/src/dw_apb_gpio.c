#include <assert.h>
#include "dw_apb_gpio.h"
#include "dw_apb_gpio_regs.h"
#include "memmap.h"

#define GPIO0 ((DW_APB_GPIO_TypeDef *)GPIO_BASE)

void gpio_init(gpio_init_config_t const *const gpio_init_config)
{
	assert(gpio_init_config != NULL);
	assert(gpio_init_config->pin < 32);

	DW_APB_GPIO_TypeDef *gpio = GPIO0;
	uint8_t pin = gpio_init_config->pin;
	volatile uint32_t* SWPORTX_CTL;
	volatile uint32_t* SWPORTX_DDR;
	switch (gpio_init_config->port) {
	case PORTA:
		SWPORTX_CTL = &gpio->SWPORTA_CTL;
		SWPORTX_DDR = &gpio->SWPORTA_DDR;
		break;
	case PORTB:
		SWPORTX_CTL = &gpio->SWPORTB_CTL;
		SWPORTX_DDR = &gpio->SWPORTB_DDR;
		break;
	case PORTC:
		SWPORTX_CTL = &gpio->SWPORTC_CTL;
		SWPORTX_DDR = &gpio->SWPORTC_DDR;
		break;
	default:
		SWPORTX_CTL = &gpio->SWPORTD_CTL;
		SWPORTX_DDR = &gpio->SWPORTD_DDR;
	}

	//software or hardware mode
	uint32_t tmp = *SWPORTX_CTL;
	if (gpio_init_config->gpio_control_mode == Software_Mode)
		tmp &= ~(1 << pin);
	else
		tmp |= 1 << pin;
	*SWPORTX_CTL = tmp;

	//set direction
	uint32_t gpio_mode = gpio_init_config->gpio_mode;
	switch (gpio_mode) {
	case GPIO_Output_Mode:
		*SWPORTX_DDR |= 1 << pin;
		break;
	case GPIO_Rising_Int_Mode:
		gpio->INTTYPE_LEVEL |= (1 << pin);
		gpio->INT_POLARITY |= (1 << pin);
		gpio->INT_BOTHEDGE &= ~(1 << pin);
		break;
	case GPIO_Falling_Int_Mode:
		gpio->INTTYPE_LEVEL |= (1 << pin);
		gpio->INT_POLARITY &= ~(1 << pin);
		gpio->INT_BOTHEDGE &= ~(1 << pin);
		break;
	case GPIO_Low_Int_Mode:
		gpio->INTTYPE_LEVEL &= ~(1 << pin);
		gpio->INT_POLARITY &= ~(1 << pin);
		gpio->INT_BOTHEDGE &= ~(1 << pin);
		break;
	case GPIO_High_Int_Mode:
		gpio->INTTYPE_LEVEL &= ~(1 << pin);
		gpio->INT_POLARITY |= (1 << pin);
		gpio->INT_BOTHEDGE &= ~(1 << pin);
		break;
	case GPIO_Edge_Int_Mode:
		gpio->INT_BOTHEDGE |= (1 << pin);
		break;
	case GPIO_Input_Mode:
		*SWPORTX_DDR &= ~(1 << pin);
		break;
	default:
		return;
	}

	if ((gpio_mode != GPIO_Input_Mode) && (gpio_mode != GPIO_Output_Mode)) {
		*SWPORTX_DDR &= ~(1 << pin);
		gpio->DEBOUNCE |= (1 << pin);
		gpio->INTMASK &= ~(1 << pin);
		gpio->INTEN |= (1 << pin);
	}
}

gpio_pin_state_t gpio_read_pin(gpio_port_t port, uint8_t pin)
{
	assert(pin < 32);
	DW_APB_GPIO_TypeDef *gpio = GPIO0;
	volatile uint32_t* EXT_PORTX;
	switch (port) {
	case PORTA:
		EXT_PORTX = &gpio->EXT_PORTA;
		break;
	case PORTB:
		EXT_PORTX = &gpio->EXT_PORTB;
		break;
	case PORTC:
		EXT_PORTX = &gpio->EXT_PORTC;
		break;
	default:
		EXT_PORTX = &gpio->EXT_PORTD;
	}
	return (((*EXT_PORTX) & (1 << pin)) == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

void gpio_write_pin(gpio_port_t port, uint8_t pin, gpio_pin_state_t pin_state)
{
	assert(pin < 32);
	DW_APB_GPIO_TypeDef *gpio = GPIO0;
	volatile uint32_t* SWPORTX_DR;
	switch (port) {
	case PORTA:
		SWPORTX_DR = &gpio->SWPORTA_DR;
		break;
	case PORTB:
		SWPORTX_DR = &gpio->SWPORTB_DR;
		break;
	case PORTC:
		SWPORTX_DR = &gpio->SWPORTC_DR;
		break;
	default:
		SWPORTX_DR = &gpio->SWPORTD_DR;
	}

	uint32_t tmp = *SWPORTX_DR;
	if (pin_state == GPIO_PIN_RESET)
		tmp &= ~(1 << pin);
	else
		tmp |= 1 << pin;
	*SWPORTX_DR = tmp;
}

void gpio_clear_interrput(uint8_t pin)
{
	assert(pin < 32);
	GPIO0->PORTA_EOI = (1 << pin);
}

void pinmux_select(gpio_port_t port, uint8_t pin, uint8_t function)
{
	assert(pin < 32);
	assert(function < 8);
	uint8_t index = ((uint8_t)port * 32) + pin;
	volatile uint32_t* pinmux = (volatile uint32_t*)(SYSCTRL_CFG_BASE + 0x800UL);
	pinmux[index] |= function<<4;
	pinmux[index] |= 1;
}