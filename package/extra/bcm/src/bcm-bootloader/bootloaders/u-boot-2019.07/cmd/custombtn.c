#include <common.h>
#include <command.h>
#include <button.h>
#include <linux/delay.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <led.h>
#include <dm/uclass-internal.h>


static int resetbtn_pressed = 0;

int recovery_btn_check(void)
{
	return resetbtn_pressed;
}

void led_config_control(const char *cmd, const char *name, const char *arg)
{
	const char *led = ofnode_conf_read_str(name);
	char buf[128];

	if (!led)
		return;

	sprintf(buf, "%s %s %s", cmd, led, arg);

	run_command(buf, 0);
}

#if 0
static void gpio_power_clr(void)
{
	ofnode node = ofnode_path("/config");
	char cmd[128];
	const u32 *val;
	int size, i;

	if (!ofnode_valid(node))
		return;

	val = ofnode_read_prop(node, "gpio_power_clr", &size);
	if (!val)
		return;

	for (i = 0; i < size / 4; i++) {
		sprintf(cmd, "gpio clear %u", fdt32_to_cpu(val[i]));
		run_command(cmd, 0);
	}
}
#endif

#if 0
static void gpio_power_set(void)
{
	ofnode node = ofnode_path("/config");
	char cmd[128];
	const u32 *val;
	int size, i;

	if (!ofnode_valid(node))
		return;

	val = ofnode_read_prop(node, "gpio_power_set", &size);
	if (!val)
		return;

	for (i = 0; i < size / 4; i++) {
		sprintf(cmd, "gpio set %u", fdt32_to_cpu(val[i]));
		run_command(cmd, 0);
	}
}
#endif

void led_control(const char *cmd, const char *name, const char *arg)
{
	char buf[128];

	sprintf(buf, "%s %s %s", cmd, name, arg);

	run_command(buf, 0);
}

int led_custom_init(void)
{
/*
	struct udevice *dev;

	printf("############led_custom_init\r\n");
	gpio_power_clr();
	for (uclass_find_first_device(UCLASS_LED, &dev);
	     dev;
	     uclass_find_next_device(&dev)) {
		struct led_uc_plat *plat = dev_get_uclass_platdata(dev);

		if (!plat->label)
			continue;
		led_control("led", plat->label, "on");
	}

	mdelay(500);

	for (uclass_find_first_device(UCLASS_LED, &dev);
		 dev;
		 uclass_find_next_device(&dev)) {
		struct led_uc_plat *plat = dev_get_uclass_platdata(dev);

		if (!plat->label)
			continue;
		led_control("led", plat->label, "off");
	}

	mdelay(500);
*/
	led_config_control("led", "system_led", "on");

	return 0;
	
}


static int do_custombtn(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *dev;	
	int ret, counter = 0;
	const char *button_label = NULL;
	ulong ts;
	

	button_label = "reset";

	ret = button_get_by_label(button_label, &dev);
	if (ret) 
	{
		printf("Button '%s' not found\n", button_label);
		return CMD_RET_SUCCESS;
	}

	if (!button_get_state(dev))
	{
		return CMD_RET_SUCCESS;
	}

	printf("RESET button is pressed for: %2d second(s)", counter++);

	ts = get_timer(0);

	while (button_get_state(dev) && counter < 3)
	{
		if (get_timer(ts) < 1000)
			continue;

		ts = get_timer(0);

		printf("\b\b\b\b\b\b\b\b\b\b\b\b%2d second(s)", counter++);
	}

	printf("\n");

	if (counter == 3) 
	{
		led_config_control("led", "reset_led", "blink 500");
		led_config_control("led", "hwdog_trigger", "blink 5000");
		resetbtn_pressed = 1;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	custombtn, 1, 0, do_custombtn,
	"custom button check",
	""
);
