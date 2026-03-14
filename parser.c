/*
 * parser.c -- Command parser
 *
 * Splits a raw input string into three parts: cmd, key, and value.
 * The value field captures everything after the key (including spaces),
 * which allows multi-token values such as "Nam EX 60".
 *
 * Example:
 *   input  = "SET city Hanoi EX 60"
 *   cmd    = "SET"
 *   key    = "city"
 *   value  = "Hanoi EX 60"
 */

#include <stdio.h>
#include <string.h>

#define CMD_SIZE   16
#define KEY_SIZE   64
#define VALUE_SIZE 256

void parse_command(char *input, char *cmd, char *key, char *value)
{
    char *token;

    /* First token: command (SET, GET, DEL, ...) */
    token = strtok(input, " ");
    if (token) {
        strncpy(cmd, token, CMD_SIZE - 1);
        cmd[CMD_SIZE - 1] = '\0';
    } else {
        return;
    }

    /* Second token: key */
    token = strtok(NULL, " ");
    if (token) {
        strncpy(key, token, KEY_SIZE - 1);
        key[KEY_SIZE - 1] = '\0';
    } else {
        key[0] = '\0';
    }

    /*
     * Third token: the rest of the string (empty delimiter).
     * Using "" instead of " " makes strtok return everything remaining,
     * preserving spaces so storage.c can parse optional flags like "EX 60".
     */
    token = strtok(NULL, "");
    if (token) {
        strncpy(value, token, VALUE_SIZE - 1);
        value[VALUE_SIZE - 1] = '\0';
    } else {
        value[0] = '\0';
    }
}
