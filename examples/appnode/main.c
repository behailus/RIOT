#include <stdio.h>

#include "kernel.h"
#include "thread.h"
#include "net_if.h"
#include "posix_io.h"
#include "shell.h"
#include "shell_commands.h"
#include "board_uart0.h"

#include "Interface.h"
#include "Rshell.h"


const shell_command_t shell_commands[] = {
    {"ping", "Send an ICMPv6 ping request to another node", nesb_send_ping},
    {"na", "Sample NESB based application node", nesb_app_commands},
    {"ns", "Sample NESB based service node", nesb_service},
    {"nm", "NESB manager node", nesb_manager},
    {NULL, NULL, NULL}
};

int main(void)
{
    /* Open the UART0 for the shell */
    posix_open(uart0_handler_pid, 0);
    /* initialize the shell */
    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);
    /* start the shell loop */
    shell_run(&shell);

    return 0;
}
