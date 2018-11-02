#include "pi.h"
#include "strings.h"
#include "malloc.h"
#include "shell.h"
#include "keyboard.h"
#include "shell_commands.h"
#include "uart.h"

#define LINE_LEN 80

static int (*shell_printf)(const char * format, ...);

static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {},
    {},
    {},
};

int cmd_echo(int argc, const char *argv[]) 
{
    for (int i = 1; i < argc; ++i) 
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[]) 
{
    // TODO: your code here
    return 0;
}

void shell_init(formatted_fn_t print_fn)
{
    shell_printf = print_fn;
}

void shell_bell(void)
{
    uart_putchar('\a');
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
            if (i != 0) {
                uart_putchar('\b');
                uart_putchar(' ');
                uart_putchar('\b');
                i = i - 2;
            } else {
                shell_bell();
                i--;
            }
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
//        cmd_echo(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[1], "reboot") == 0) {
        pi_reboot();
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
