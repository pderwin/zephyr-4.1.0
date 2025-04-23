#pragma once

int  lr11xx_gpio_init                  (const struct device *device);
void lr11xx_gpio_radio_irq_disable     (struct lr11xx_data *dev_data);
void lr11xx_gpio_radio_irq_enable      (struct lr11xx_data *dev_data);
void lr11xx_gpio_reset                 (struct lr11xx_data *dev_data);
void lr11xx_gpio_reset_programming_mode(struct lr11xx_data *dev_data);
