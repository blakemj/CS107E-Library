#include "pi.h"
#include "strings.h"
#include "malloc.h"
#include "shell.h"
#include "keyboard.h"
#include "shell_commands.h"
#include "uart.h"

#define LINE_LEN 80
#define NUM_COMMANDS 5

static int numHistoryInArray = 0;
static int searchingHistory = 0;
static char commandHistory[10][1024];
static int (*shell_printf)(const char * format, ...);

// This is the table of commands in the form of a struct
// Order is name, description, function
static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboot the Raspberry Pi back to the bootloader", cmd_reboot},
    {"peek",   "<address> prints the contents (4 bytes) of memory at address", cmd_peek},
    {"poke",   "<address> <value> stores `value` into the memory at `address`", cmd_poke},
};

/*
* This helper function serves to help check for errors in the peek and poke arguments. 
* For the addresses in both peek and poke, it will check to see if the arguments are
* in the form of a four-byte aligned address, and are an actual addresses. Also, if
* being called by the poke function, it will check if the value to be placed at the 
* address is convertable into an unsigned int. If any of these fail, the function returns
* 1 and prints an error message.
*/
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

/*
* This function is used to "poke" a given memory address. The function will take in 
* an array of arguments and an int of the number of arguments in the array. The 
* function ensures that there are at least 3 arguments (including 'poke'), and if not
* it will print an error message and return 1. If there are, it will convert the first
* argument (after the word 'poke') into an address and the second into an integer value.
* If the error checks pass from peek_and_poke_error_check, then it will palce the
* value into the address and print it to the monitor and return 0. Else it returns
* 1. The function ignores all arguments after the value.
*/
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

/*
* This function is used to "peek" at a given memory address. The function will take
* an array of arguments and an int of the number of arguments in the array. The 
* function ensures that there are at least 2 arguments (including 'peek', and if not
* it will print an error message and return 1. If there are, it will convert the
* argument (after the word 'peek') into an address. If the error checks pass from
* peek_and_poke_error_check, then it will print the address and its value to the
* monitor and return 0. Else it returns 1. The function ignores all arguments after
* the address.
*/
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

/*
* This function will reboot the pi.
*/
int cmd_reboot(int argc, const char* argv[]) {
    pi_reboot();
    return 0;
}

/*
* This function takes in an array of tokens and will display all of the tokens one
* after the other onto the monitor to echo what the user passes in.
*/
int cmd_echo(int argc, const char *argv[]) 
{
    for (int i = 1; i < argc; ++i) 
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

/*
* This function serves two different purposes. If the user only types the word 'help'
* (therfore there is only one argument--argc=1) then it will loop through all of the
* commands in the command struct and print out their description. If the user types
* 'help' and then the name of the command, it will print out the description of just
* that command. If the user types help and then something that is not a command, the
* funtion prints out an error message and returns 1. The function ignores all arguments
* after the command.
*/
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

//This function initializes the shell
void shell_init(formatted_fn_t print_fn)
{
    shell_printf = print_fn;
}

//This function sounds a bell noise
void shell_bell(void)
{
    uart_putchar('\a');
}

/*
* This function will print out a backspace on the monitor (or will essentially cover
* the previous character on the screen with a space). If there is no character before
* the backspace, then it will sound the bell sound. This function will also adjust the
* placement index on the line from the shell_readline function.
*/
static int backspace(int placeOnLine, int* size, char* buf, int delete) {
    if (placeOnLine != 0) {
        uart_putchar('\b');
        uart_putchar(' ');
        uart_putchar('\b');
        placeOnLine--;
    } else {
        shell_bell();
        placeOnLine--;
    }
    if (placeOnLine > -1 && !delete) {
        placeOnLine--;
        for (int moveBack = placeOnLine + 1; moveBack < *size + 1; moveBack++) {
            buf[moveBack] = buf[moveBack + 1];
        }
        (*size)--;
        for (int putBack = placeOnLine + 1; putBack < *size; putBack++) uart_putchar(buf[putBack]);
        uart_putchar(' ');
        for (int bringCursorBack = placeOnLine + 1; bringCursorBack < *size + 1; bringCursorBack++) uart_putchar('\b');
        }
    return placeOnLine;
}

static int moveCursorToEnd(int placeholder, int size, char* buf) {
    for (int place = placeholder; place < size; place++) {
        uart_putchar(buf[place]);
    }
    return size;
}

static int delete(int placeholder, int * size, char* buf) {
    placeholder = moveCursorToEnd(placeholder, *size, buf);
    while (placeholder != 0) {
        placeholder = backspace(placeholder, size, buf, 1);
        (*size)--;
    }
    return placeholder;
}

/*
* This function will move through each character written to the line by the user.
* Each time the user presses a key on the keyboard, the screen will update accordingly
* (assuming the key pressed was a character). This also saves it in a buffer.
*/
void shell_readline(char buf[], int bufsize)
{
    int size = 0;
    for (int i = 0; i < bufsize - 1; i++) {
        unsigned char userTyped = keyboard_read_next();
        if(userTyped == '\n') {
            i = moveCursorToEnd(i, size, buf);
            uart_putchar(userTyped);
            buf[i] = '\0';
            return;
        } else if (userTyped == '\b') {
            i = backspace(i, &size, buf, 0);
        } else if (userTyped == PS2_KEY_ARROW_LEFT) {
            if (i != 0) {
                i = i-2;
                uart_putchar('\b');
            } else {
                shell_bell();
                i--;
            }
        } else if (userTyped == PS2_KEY_ARROW_RIGHT) {
            if (i < size) {
                uart_putchar(buf[i]);
            } else {
                shell_bell();
                i--;
            }
        } else if (userTyped == PS2_KEY_ARROW_UP) {
            if (searchingHistory < numHistoryInArray) {
                i = delete(i, &size, buf);
                if (searchingHistory >= 10) {
                    searchingHistory--;
                    shell_bell();
                }
                shell_printf("%s", commandHistory[searchingHistory]);
                memcpy(buf, commandHistory[searchingHistory], bufsize);
                i = strlen(commandHistory[searchingHistory]);
                size = strlen(commandHistory[searchingHistory]);
                searchingHistory++;
            } else {
                shell_bell();
                i--;
            }
        } else if (userTyped == 0xfa) {
            for (int place = i; place > 0; place--) {
                uart_putchar('\b');
            }
            i = -1;
        } else if (userTyped == 0xfe) {
            i = moveCursorToEnd(i, size, buf) - 1;
        } else if (userTyped == 0xfd) {
            i = delete(i, &size, buf);
            i--;    
        } else {
            char nextTemp = buf[i];
            for (int move = i; move < size; move++) {
                char curTemp = buf[move+1];
                buf[move+1] = nextTemp;
                nextTemp = curTemp;
            }
            buf[i] = userTyped;
            uart_putchar(userTyped);
            size++;
            moveCursorToEnd(i + 1, size, buf);
            for (int bringCursorBack = i+1; bringCursorBack < size; bringCursorBack++) uart_putchar('\b');
        }
    }
    buf[bufsize - 1] = '\0';
}

/*
* This function will taken in a string of characters, and an array of char pointers.
* The function will use the space characer (' ') as the delimiter between words. As
* the function moves along the string, it will replace all spaces with '\0' and 
* place a pointer to the beginning of the word into the tokens array. Since the last
* word ends with '\0', once it reaches this point, it will place the final word in
* the array and return the number of tokens placed in the array.
*/
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

static void putLineInHistory(const char* line) {
    char temp[1024];
    if (numHistoryInArray < 10) numHistoryInArray++;
    memcpy(temp, line, 1024);
    for (int i = 0; i < numHistoryInArray; i++) {
        char nextTemp[1024];
        memcpy(nextTemp, commandHistory[i], 1024);
        memcpy(commandHistory[i], temp, 1024);
        memcpy(temp, nextTemp, 1024);
    }
}

/*
* This function takes in the line after the user hits return and evaluates the command
* that it should follow. First, it places the line as saved by the user into the heap
* so that the tokenizer can point to the different words. The words are separated into
* tokens, which are then used as arguments for the first word in the line which should
* be the command. If the first word is not any of the commands, it will simply ignore
* the line and start a new one. If it is a command, it will do the respective function
* for the command then return.
*/
int shell_evaluate(const char *line)
{
    putLineInHistory(line);
    if(line[0] == '\0') return 0;
    char* temp = malloc(strlen(line) + 1);
    memcpy(temp, line, strlen(line) + 1);
    char* tokens[strlen(temp)];
    int errorReturned = 1;
    int tokenIndex = tokenizer(temp, tokens);
    if (strcmp(tokens[0], "echo") == 0) {
        errorReturned = commands[1].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "reboot") == 0) {
        errorReturned = commands[2].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "help") == 0) {
        errorReturned = commands[0].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "peek") == 0) {
        errorReturned = commands[3].fn(tokenIndex, (const char**)tokens);
    }
    if (strcmp(tokens[0], "poke") == 0) {
        errorReturned = commands[4].fn(tokenIndex, (const char**)tokens);
    }
    free(temp);
    return errorReturned;
}

/*
* This funciton will run the shell, reading the line, then evaluating the line in an endless
* loop.
*/
void shell_run(void)
{
    int commandNum = 0;
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1) 
    {
        commandNum++;
        char line[LINE_LEN];

        shell_printf("[%d] Pi> ", commandNum);
        shell_readline(line, sizeof(line));
        searchingHistory = 0;
        shell_evaluate(line);
    }
}
