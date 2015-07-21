#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "cli.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_CONN (10)

#define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

////////////////////////////////////////////////////////////////////////////////

enum
{
    // mode id shall be unique. but being continuous is not necessary
    ENABLE_MODE = 100,
    CONFIG_MODE = 50,
};

static int _running = 1;

static const char _banner[] =  "+---------------------+\n\r"
                               "|  This is My Banner  |\n\r"
                               "+---------------------+";

static const char _config_filepath[] = "test.conf";

////////////////////////////////////////////////////////////////////////////////

static void _sigIntHandler(int sig_num)
{
    _running = 0;
    dprint("stop main()");
}

static int _regular(struct cli* cli, int argc, char* argv[])
{
    cli_error(cli, "invalid input : %s", argv[0]);
    return CLI_OK;
}

////////////////////////////////////////////////////////////////////////////////

DEFUN(_get_info,                   // some name ?
      _get_info_cmd,               // the parameter name which be installed
      "get info",                  // input command strings
      "[HELP] get something",      // description for 1st string - here is 'get'
      "[HELP] self information")   // description for 2nd string - here is 'info'
{
    int money = 100;
    cli_print(cli, "this is a get info cmd");
    cli_print(cli, "my money = %d dollars", money);
    return CLI_OK;
}

DEFUN(_change_to_config,
      _change_to_config_cmd,
      "config terminal",
      "[HELP] change to config mode - part 1",
      "[HELP] change to config mode - part 2")
{
    cli_change_mode(cli, CONFIG_MODE); // you could change mode freely by your self
    return CLI_OK;
}

DEFUN(_exit_mode,
      _exit_cmd,
      "exit",
      "[HELP] exit current mode")
{
    if (cli->mode_id == CONFIG_MODE)
    {
        cli_change_mode(cli, ENABLE_MODE);
    }
    else if (cli->mode_id == ENABLE_MODE)
    {
        // if you return CLI_BREAK, this connection will be closed
        return CLI_BREAK;
    }

    return CLI_OK;
}

DEFUN(_set_integer,
      _set_integer_cmd,
      "set int <-100-5000>",
      "[HELP] set something")   // description num could be less than string number
{
    cli_print(cli, "this is a set int cmd");
    cli_print(cli, "argc = %d", argc);

    if (argc <= 0)
    {
        // in normal case, system will check parameter for you
        // so this error case should never happen
        cli_error(cli, "argc <= 0 is invalid");
        // CLI_OK or CLI_FAIL are the same for CLI system. system do nothing
        return CLI_FAIL;
    }
    cli_print(cli, "argv[0] = %s", argv[0]);

    int money = atoi(argv[0]);
    cli_print(cli, "my money = %d dollars", money);
    return CLI_OK;
}

DEFUN(_set_ipv4,
      _set_ipv4_cmd,
      "set ipv4 A.B.C.D/M", // it must be A.B.C.D or A.B.C.D/M
      "[HELP] set something",
      "[HELP] ipv4 token",
      "[HELP] ipv4 value with or without mask")
{
    if (argc <= 0)
    {
        cli_error(cli, "argc <= 0 is invalid");
        return CLI_FAIL;
    }
    cli_print(cli, "argc = %d, argv[0] = %s", argc, argv[0]);

    int ipv4[4];
    int mask;
    int num = sscanf(argv[0], "%d.%d.%d.%d/%d", &ipv4[0], &ipv4[1], &ipv4[2], &ipv4[3], &mask);

    cli_print(cli, "num = %d", num);

    // you could use sscanf return value to check if input contains netmask or not
    if (num == 4)
    {
        cli_print(cli, "ipv4 value = %d.%d.%d.%d", ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
    }
    else if (num == 5)
    {
        cli_print(cli, "ipv4 value = %d.%d.%d.%d", ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
        cli_print(cli, "mask = %d", mask);
    }
    else
    {
        cli_error(cli, "input is wrong?");
        return CLI_FAIL;
    }
    return CLI_OK;
}

DEFUN(_set_string,
      _set_string_cmd,
      "set string LALALALA LULULU", // token of all upcase letters
      "[HELP] set something",
      "[HELP] string token",
      "[HELP] your input string")
{
    if (argc <= 0)
    {
        cli_error(cli, "argc <= 0 is invalid");
        return CLI_FAIL;
    }
    cli_print(cli, "argc = %d, LALALA = %s, LULULU = %s", argc, argv[0], argv[1]);
    return CLI_OK;
}

// (on on | off off) is invalid. each alternative option should be a single word
DEFUN(_set_alternative,
      _set_alternative_cmd,
      "set alternative (on   |off |  fuzzy  )", // redundant spaces will be trimmed by system
      "[HELP] set something",
      "[HELP] alternative token",
      "[HELP] input")
{
    if (argc <= 0)
    {
        cli_error(cli, "argc <= 0 is invalid");
        return CLI_FAIL;
    }

    cli_print(cli, "argv[0] = %s", argv[0]);

    if (0 == strcmp(argv[0], "on"))
    {
        cli_print(cli, "turn on");
    }
    else if (0 == strcmp(argv[0], "off"))
    {
        cli_print(cli, "turn off");
    }
    else if (0 == strcmp(argv[0], "fuzzy"))
    {
        cli_print(cli, "fuzzy mode");
    }

    return CLI_OK;
}

DEFUN(_set_option,
      _set_option_cmd,
      "set option [<1-100>]",
      "[HELP] set something",
      "[HELP] option token",
      "[HELP] option input")
{
    cli_print(cli, "set option : argc = %d", argc);
    if (argc > 0)
    {
        int value = atoi(argv[0]);
        cli_print(cli, "set option value = %d", value);
    }
    return CLI_OK;
}

DEFUN(_set_mix,
      _set_mix_cmd,
      "set int <1-65535> ipv4 A.B.C.D/M string LALALA LULULU alternative (|on|off)",
      "[HELP] set something",
      "[HELP] i am too lazy to write descriptions...")
{
    cli_print(cli, "argc = %d", argc);
    cli_print(cli, "integer = %s", argv[0]);
    cli_print(cli, "ipv4 = %s", argv[1]);
    cli_print(cli, "LALALA = %s", argv[2]);
    cli_print(cli, "LULULU = %s", argv[3]);

    if (argc == 4)
    {
        cli_print(cli, "no alternative");
    }
    else if (argc == 5)
    {
        cli_print(cli, "alternative = %s", argv[4]);
    }
    return CLI_OK;
}

DEFUN(_write_file,
      _write_file_cmd,
      "write",
      "[HELP] write history to a config file")
{
    int chk = cli_save_histories(cli, (char*)_config_filepath);
    if (chk != CLI_OK)
    {
        cli_error(cli, "cli_save_histories to file : %s failed", _config_filepath);
        return CLI_FAIL;
    }
    return CLI_OK;
}

DEFUN(_read_file,
      _read_file_cmd,
      "read",
      "[HELP] read history from a config file")
{
    int chk = cli_execute_file(cli, (char*)_config_filepath);
    if (chk != CLI_OK)
    {
        cli_error(cli, "cli_execute_file : %s failed", _config_filepath);
        return CLI_FAIL;
    }
    return CLI_OK;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    struct cli_server server = {};
    struct cli cli[MAX_CONN] = {0};
    int chk;

    signal(SIGINT, _sigIntHandler);

    ////////////////////////////////////////////////////////////////////////

    chk = cli_server_init(&server, NULL, 5000, MAX_CONN, NULL, NULL);
    // chk = cli_server_init(&server, NULL, 5000, MAX_CONN, "5566", "i am sad");
    CHECK_IF(chk != CLI_OK, return -1, "cli_server_init failed");

    chk = cli_server_banner(&server, (char*)_banner);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_banner failed");

    chk = cli_server_regular_func(&server, _regular);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_regular_func failed");

    chk = cli_server_install_mode(&server, ENABLE_MODE, "[taco]# ");
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_mode failed");

    chk = cli_server_install_mode(&server, CONFIG_MODE, "[taco](config)# ");
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_mode failed");

    chk = cli_server_default_mode(&server, ENABLE_MODE);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_default_mode failed");

    ////////////////////////////////////////////////////////////////////////

    chk = cli_server_install_cmd(&server, ENABLE_MODE, &_get_info_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _get_info_cmd failed");

    chk = cli_server_install_cmd(&server, ENABLE_MODE, &_change_to_config_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _change_to_config_cmd failed");

    chk = cli_server_install_cmd(&server, ENABLE_MODE, &_exit_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _exit_cmd failed");

    chk = cli_server_install_cmd(&server, ENABLE_MODE, &_read_file_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _read_file_cmd failed");

    ////////////////////////////////////////////////////////////////////////

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_set_integer_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _set_integer_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_set_ipv4_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _set_ipv4_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_set_string_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _set_string_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_set_alternative_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _set_alternative_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_set_option_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _set_option_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_set_mix_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _set_mix_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_write_file_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _write_file_cmd failed");

    chk = cli_server_install_cmd(&server, CONFIG_MODE, &_exit_cmd);
    CHECK_IF(chk != CLI_OK, goto _END, "cli_server_install_cmd _exit_cmd failed");

    ////////////////////////////////////////////////////////////////////////

    fd_set readset;
    int select_ret;
    struct timeval timeout;
    int i;

    while (_running)
    {
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(server.fd, &readset);

        for (i=0; i<MAX_CONN; i++)
        {
            if (cli[i].is_init == 1) FD_SET(cli[i].fd, &readset);
        }

        select_ret = select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
        if (select_ret < 0)
        {
            derror("select_ret = %d", select_ret);
            break;
        }
        else if (select_ret == 0)
        {
            continue;
        }
        else
        {
            if (FD_ISSET(server.fd, &readset))
            {
                for (i=0; i<MAX_CONN; i++)
                {
                    if (cli[i].is_init == 0) // find an empty cli to accept this connection
                    {
                        chk = cli_server_accept(&server, &cli[i]);
                        CHECK_IF(chk != CLI_OK, goto _END, "cli_server_accept failed, i = %d", i);
                        break;
                    }
                }
            }

            for (i=0; i<MAX_CONN; i++)
            {
                if (cli[i].is_init && FD_ISSET(cli[i].fd, &readset))
                {
                    chk = cli_process(&cli[i]);
                    // you need to do uninit cli by yourself when return CLI_BREAK
                    if (chk == CLI_BREAK)
                    {
                        cli_uninit(&cli[i]);
                    }
                }
            }
        }
    }

_END:
    for (i=0; i<MAX_CONN; i++)
    {
        if (cli[i].is_init) cli_uninit(&cli[i]);
    }
    cli_server_uninit(&server);

    return 0;
}
