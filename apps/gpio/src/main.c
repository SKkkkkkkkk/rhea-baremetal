#include <stdio.h>
#include "dw_apb_gpio.h"

int main()
{
	printf("GPIO Demo\n\r");
	pinmux_select(PORTB, 1, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTB,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode,
		.pin = 1
	};
	gpio_init(&gpio_init_config);

	while (1)
		printf("%u\n\r", gpio_read_pin(PORTB, 1));
}