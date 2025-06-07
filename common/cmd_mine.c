#include <common.h>
#include <command.h>
#include <g_dnl.h>
#include <usb.h>

static void busy_indicator(void)
{
    static int state;

    switch (state)
    {
    case 0:
        puts("\r|");
        break;
    case 1:
        puts("\r/");
        break;
    case 2:
        puts("\r-");
        break;
    case 3:
        puts("\r\\");
        break;
    case 4:
        puts("\r|");
        break;
    case 5:
        puts("\r/");
        break;
    case 6:
        puts("\r-");
        break;
    case 7:
        puts("\r\\");
        break;
    default:
        state = 0;
    }
    if (state++ == 8)
        state = 0;
}

int do_mine(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{
    int rc = 0;
    int i = 0, k = 0;

    const char *usb_controller;
    unsigned int controller_index;

    if (argc < 2)
        return CMD_RET_USAGE;

    usb_controller = argv[1];
    controller_index = (unsigned int)(simple_strtoul(
        usb_controller, NULL, 0));

    if (board_usb_init(controller_index, USB_INIT_DEVICE))
    {
        error("Couldn't init USB controller.");
        return CMD_RET_FAILURE;
    }

    rc = g_dnl_register("usb_mine_k");
    if (rc)
    {
        error("g_dnl_register failed");
        return CMD_RET_FAILURE;
    }
    while (1)
    {
        usb_gadget_handle_interrupts(controller_index);
        if (++i == 20000)
        {
            busy_indicator();
            i = 0;
            k++;
        }
        if (k == 10)
        {
            /* Handle CTRL+C */
            if (ctrlc())
                break;
            /* Check cable connection */
            if (!g_dnl_board_usb_cable_connected())
                break;
            k = 0;
        }
    }

    g_dnl_unregister();
    board_usb_cleanup(controller_index, USB_INIT_DEVICE);
    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mine, 2, 1, do_mine, "my cmd", "<USB_controller> e.g. mine 0");
