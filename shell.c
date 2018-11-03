#include "pi.h"
#include "strings.h"
#include "malloc.h"
#include "shell.h"
#include "keyboard.h"
#include "shell_commands.h"
#include "uart.h"

#define LINE_LEN 80
#define NUM_COMMANDS 5

static int (*shell_printf)(const char * format, ...);

static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboot the Raspberry Pi back to the bootloader", cmd_reboot},
    {"peek",   "<address> prints the contents (4 bytes) of memory at address", cmd_peek},
    {"poke",   "<address> <value> stores `value` into the memory at `address`", cmd_poke},
};

static int peek_and_poke_error_check(unsigned int address, unsigned int value, const char* argv[], char* callerName) {
    if(address % 4 != 0) {
        shell_printf("error: %s address must be 4-byte aligned\n", callerName);
        return 1;
    } else if (address == 0 && strcmp(argv[1], "0") != 0 && strcmp(argv[1], "0x0") != 0) {
        shell_printf("error: %s cannot convert '%s'.\n", callerName, argv[1]);
        return 1;
    }
    if (strcmp(callerName, "poke") == 0) {
        if (value == 0 && strcmp(argv[2], "0") != 0 && strcmp(argv[2], "0x0") != 0) {
            shell_printf("error: poke cannot convert '%s'.\n", argv[2]);
            return 1;
        }
    }
    return 0;
}

int cmd_poke(int argc, const char *argv[])
{
    if (argc <= 2) {
        shell_printf("error: poke requires 2 arguments [address] and [value]\n");
        return 1;
    } else { 
        const char** temp = (const char**)NULL;
        unsigned int address = strtonum(argv[1], temp);
        unsigned int value = strtonum(argv[2], temp);
        if (peek_and_poke_error_check(address, value, argv, "poke") == 1) return 1;
        *(unsigned int*)address = value;
        shell_printf("0x%08x: %08x\n", address, *(unsigned int*)address);
        return 0;
    }
    return 1;
}

int cmd_peek(int argc, const char *argv[])
{       
    if (argc == 1) {
        shell_printf("error: peek requires 1 argument [address]\n");
        return 1;
    } else {
        const char** temp = (const char**)NULL;
        unsigned int address = strtonum(argv[1], temp);
        if (peek_and_poke_error_check(address, 0, argv, "peek") == 1) return 1;
        shell_printf("0x%08x: %08x\n", address, *(unsigned int*)address);
        return 0;
    }
    return 1;
}

int cmd_reboot(int argc, const char* argv[]) {
    pi_reboot();
    return 0;
}

int cmd_echo(int argc, const char *argv[]) 
{
    for (int i = 1; i < argc; ++i) 
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[]) 
{
    if (argc == 1) {
        for (int i = 0; i < NUM_COMMANDS; i++) {
            shell_printf("%s: %s\n", commands[i].name, commands[i].description);
        }
        return 0;
    } else {
        for (int i = 0; i < NUM_COMMANDS; i++) {
            if (strcmp(argv[1], commands[i].name) == 0) {
                shell_printf("%s: %s\n", commands[i].name, commands[i].description);
                return 0;
            }
        }
        shell_printf("error: no such command '%s'.\n", argv[1]);
        return 1;
    }
    return 1;
}

void shell_init(formatted_fn_t print_fn)
{
    shell_printf = print_fn;
}

void shell_bell(void)
{
    uart_putchar('\a');
}

static int backspace(int placeOnLine) {
    if (placeOnLine != 0) {
        uart_putchar('\b');
        uart_putchar(' ');
        uart_putchar('\b');
        placeOnLine = placeOnLine - 2;
    } else {
        shell_bell();
        placeOnLine--;
    }
    return placeOnLine;
}

void shell_readline(char buf[], int bufsize)
{
    for (int i = 0; i < bufsize - 1; i++) {
        unsigned char userTyped = keyboard_read_next();
        if(userTyped == '\n') {
            uart_putchar(userTyped);
            buf[i] = '\0';
            return;
        } else if (userTyped == '\b') {
            i = backspace(i);
        } else {
            buf[i] = userTyped;
            uart_putchar(userTyped);
        }
    }
    buf[bufsize - 1] = '\0';
}

static int tokenizer(char* temp, char** tokens) {
    int charFromLine = 0;
    int tokenIndex = 0;
    while (1) {
        char* startOfWord = &temp[charFromLine];
        while(temp[charFromLine] != ' ' && temp[charFromLine] != '\0') {
            charFromLine++;
        }
        tokens[tokenIndex] = startOfWord;
        tokenIndex++;
        if (temp[charFromLine] == '\0') break;
        temp[charFromLine] = '\0';
        charFromLine++;
    }
    return tokenIndex;
}

int shell_evaluate(const char *line)
{
    if(line[0] == '\0') return 0;
    char* temp = malloc(strlen(line) + 1);
    memcpy(temp, line, strlen(line) + 1);
    char* tokens[strlen(temp)];
    int tokenIndex = tokenizer(temp, tokens);
    if (strcmp(tokens[0], "echo") == 0) {
        commands[1].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "reboot") == 0) {
        commands[2].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "help") == 0) {
        commands[0].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "peek") == 0) {
        commands[3].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "poke") == 0) {
        commands[4].fn(tokenIndex, (const char**)tokens);
    }
    free(temp);
    return 0;
}

void shell_run(void)
{
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1) 
    {
        char line[LINE_LEN];

        shell_printf("Pi> ");
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
    }
}
