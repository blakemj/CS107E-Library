#include "pi.h"
#include "strings.h"
#include "malloc.h"
#include "shell.h"
#include "keyboard.h"
#include "shell_commands.h"
#include "uart.h"

#define LINE_LEN 80
#define NUM_COMMANDS 6
#define MAX_COMMAND_HISTORY 10
#define MAX_COMMAND_STRING_SIZE 1024
#define MOVE_TO_START_CODE 0xfa
#define MOVE_TO_END_CODE 0xfe
#define DELETE_LINE_CODE 0xfd

//These global variables keep track of the history array
static int numHistoryInArray = 0;
static int searchingHistory = 0;
//This array of char* (or character arrays) keeps track of command history
static char commandHistory[MAX_COMMAND_HISTORY][MAX_COMMAND_STRING_SIZE];

static int (*shell_printf)(const char * format, ...);

/*
* This command will print out the previous 10 commands in order from oldest to newest.
* If there are not 10 previous commands, it will print out whatever commands there 
* have been.
*/
int cmd_history(int argc, const char *argv[])
{
    int counter = 1;
    for (int i = numHistoryInArray; i > 0; i--) {
        shell_printf("%d %s\n", counter, commandHistory[i - 1]);
        counter++;
    }
    return 0;
}

// This is the table of commands in the form of a struct
// Order is name, description, function
static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboot the Raspberry Pi back to the bootloader", cmd_reboot},
    {"peek",   "<address> prints the contents (4 bytes) of memory at address", cmd_peek},
    {"poke",   "<address> <value> stores `value` into the memory at `address`", cmd_poke},
    {"history", "displays the history of recent commands", cmd_history},
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
* placement index on the line from the shell_readline function. This function will 
* also move the characters backwards after where the cursor is deleting, if there
* are any characters to be moved. If deleting a whole line, this will not attempt to move
* any characters, but will simply delete everything.
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

/*
* This helper function is used to move the cursor to the end of the line.
*/
static int moveCursorToEnd(int placeholder, int size, char* buf) {
    for (int place = placeholder; place < size; place++) {
        uart_putchar(buf[place]);
    }
    return size;
}

/*
* This function is used to delete an entire line (control u). This will
* simply cover all of the line with backspaces using the backspace command, then
* will change the size.
*/
static int delete(int placeholder, int * size, char* buf) {
    placeholder = moveCursorToEnd(placeholder, *size, buf);
    while (placeholder != 0) {
        placeholder = backspace(placeholder, size, buf, 1);
        (*size)--;
    }
    return placeholder;
}

/*
* This helper function is used to move the cursor over to the left. If the cursor moves
* to the beginning of the line, then the shell_bell will sound and it cannot move left anymore.
*/
static int movingCursorLeft(int placeholder) {
    if (placeholder != 0) {
        placeholder = placeholder - 2;
        uart_putchar('\b');
    } else {
        shell_bell();
        placeholder--;
    }
    return placeholder;
}

/*
* This helper funtion is used to move the cursor over to the right. This is done by replacing
* the character on the screen with what is already there, moving the cursor each time. If the
* cursor attempts to move beyond the end of the line, the shell_bell will sound, and it will stop.
*/
static int movingCursorRight(char* buf, int placeholder, int size) {
    if (placeholder < size) {
        uart_putchar(buf[placeholder]);
    } else {
        shell_bell();
        placeholder--;
    }
    return placeholder;
}

/*
* This helper function is used to print out a new command when searching through the history.
* This function will also reset the placeholder and the size of the line accordingly.
*/
static int printNewCommand(char* buf, int placeholder, int* size, int bufsize) {
    placeholder = delete(placeholder, size, buf);
    shell_printf("%s", commandHistory[searchingHistory]);
    memcpy(buf, commandHistory[searchingHistory], bufsize);
    placeholder = strlen(commandHistory[searchingHistory]);
    *size = strlen(commandHistory[searchingHistory]);
    return placeholder;
}

/*
* This funtion is used to search up the history for previous commands (older). If the user reaches
* the top of the command history, then the shell_bell sound will play and the user will not be 
* allowed to move further.
*/
static int searchingUpHistory(char* buf, int placeholder, int* size, int bufsize) {
    if (searchingHistory == -1) searchingHistory++;
    if (searchingHistory < numHistoryInArray) {
        placeholder = printNewCommand(buf, placeholder, size, bufsize);
        searchingHistory++;
    } else {
        shell_bell();
        placeholder--;
    }
    return placeholder;
}

/*
* This funtion is used to search down the history for (newer) commands, or to leave the command
* history to write a new command. If the user tries to move beyond the point of writing their own
* command (after the newest command in the history), then the shell_bell will sound, and the user
* will no longer be able to move further.
*/
static int searchingDownHistory(char* buf, int placeholder, int* size, int bufsize) {
    if (searchingHistory == numHistoryInArray) searchingHistory--;
    if (searchingHistory > 0) {
        searchingHistory--;
        placeholder = printNewCommand(buf, placeholder, size, bufsize);
    } else if (searchingHistory == 0) {
        placeholder = delete(placeholder, size, buf);
        *size = 0;
        searchingHistory--;
        placeholder--;
    } else {
        shell_bell();
        placeholder--;
    }
    return placeholder;
}

/*
* This function will move through each character written to the line by the user.
* Each time the user presses a key on the keyboard, the screen will update accordingly
* (assuming the key pressed was a character). This also saves it in a buffer. This
* function will also check the arrow keys to move the cursor or search through the
* command history accordingly. This will also check for the shortcuts such as control
* a, control e, or control e, which moves the cursor to the beginning or the end of the
* line, or will delete the line (respectively).
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
            i = movingCursorLeft(i);
        } else if (userTyped == PS2_KEY_ARROW_RIGHT) {
            i = movingCursorRight(buf, i, size);
        } else if (userTyped == PS2_KEY_ARROW_UP) {
            i = searchingUpHistory(buf, i, &size, bufsize);
        } else if (userTyped == PS2_KEY_ARROW_DOWN) {
            i = searchingDownHistory(buf, i, &size, bufsize);
        } else if (userTyped == MOVE_TO_START_CODE) {
            for (int place = i; place > 0; place--) {
                uart_putchar('\b');
            }
            i = -1;
        } else if (userTyped == MOVE_TO_END_CODE) {
            i = moveCursorToEnd(i, size, buf) - 1;
        } else if (userTyped == DELETE_LINE_CODE) {
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

/*
* This function is used to place a line into the command history array. If 
* the command history reaches its max number of commands, then it will replace
* the older commands with the newer ones, while still keeping all the commands 
* in order.
*/
static void putLineInHistory(const char* line) {
    char temp[MAX_COMMAND_STRING_SIZE];
    if (numHistoryInArray < MAX_COMMAND_HISTORY) numHistoryInArray++;
    memcpy(temp, line, MAX_COMMAND_STRING_SIZE);
    for (int i = 0; i < numHistoryInArray; i++) {
        char nextTemp[MAX_COMMAND_STRING_SIZE];
        memcpy(nextTemp, commandHistory[i], MAX_COMMAND_STRING_SIZE);
        memcpy(commandHistory[i], temp, MAX_COMMAND_STRING_SIZE);
        memcpy(temp, nextTemp, MAX_COMMAND_STRING_SIZE);
    }
}

/*
* This helper function will check a prefix that the user puts in to find the 
* most recent command that begins with that prefix. If It cannot find a command
* with the prefix in the command history array, then it will return an error 
* message (it returns an int to demonstrate that prefixed string contains the
* error message).
*/
static int findPrefixInHistory(char* prefixedString, char* token) {
    int prefixSize = strlen(token);
    for (int commandNum = 0; commandNum < numHistoryInArray; commandNum++) {
        int correctCommand = 1;
        for (int prefixIndex = 1; prefixIndex < prefixSize; prefixIndex++) {
            if (token[prefixIndex] != commandHistory[commandNum][prefixIndex - 1]) {
                correctCommand = 0;
                break;
            }
        }
        if (correctCommand) {
            memcpy(prefixedString, commandHistory[commandNum], MAX_COMMAND_STRING_SIZE);
            return 1;
        }
    }
    memcpy(prefixedString, "Error: No command with given prefix!", MAX_COMMAND_STRING_SIZE);
    return 0;
}

/*
* This function takes in the line after the user hits return and evaluates the command
* that it should follow. First, it places the line as saved by the user into the heap
* so that the tokenizer can point to the different words. The words are separated into
* tokens, which are then used as arguments for the first word in the line which should
* be the command. If the first word is not any of the commands, it will simply ignore
* the line and start a new one. If it is a command, it will do the respective function
* for the command then return. This function will check the command history if the 
* user returns a shortcut (where the "command" is the first token, and all others are
* ignored). If "!!" is returned, then the previous command is evaluated, and if "!..."
* is returned (where the ... is a series of characters) then it will check the command
* history to evaluate the most recent command that starts with that series of characters.
*/
int shell_evaluate(const char *line)
{
    if(line[0] == '\0') return 0;
    char* temp = malloc(strlen(line) + 1);
    memcpy(temp, line, strlen(line) + 1);
    char* tokens[strlen(temp)];
    int errorReturned = 1;
    int tokenIndex = tokenizer(temp, tokens);
    if (strcmp(tokens[0], "!!") == 0) {
        char temp[MAX_COMMAND_STRING_SIZE];
        memcpy(temp, commandHistory[0], MAX_COMMAND_STRING_SIZE);
        errorReturned = shell_evaluate(temp);
    } else if (tokens[0][0] == '!') {
        char prefixedString[MAX_COMMAND_STRING_SIZE];
        int foundCommand = findPrefixInHistory(prefixedString, tokens[0]);
        if (foundCommand) {
            errorReturned = shell_evaluate(prefixedString);
        } else {
            shell_printf("%s\n", prefixedString);
        }
    } else {
        putLineInHistory(line);
    }
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
    if (strcmp(tokens[0], "history") == 0) {
        errorReturned = commands[5].fn(tokenIndex, (const char**)tokens);
    }
    free(temp);
    return errorReturned;
}

/*
* This funciton will run the shell, reading the line, then evaluating the line in an endless
* loop. This will number all of the commands.
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
