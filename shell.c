#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"
char *builtin_str[] = {
    "cd",
    "pwd",
    "exit",
    "help",
    "history"
};

int main(int argc, char **argv)
{
    shell_loop();
}

void shell_loop(void)
{
    char *line;
    char **args;
    int status = 1;
    while (status) {
        printf(SHELL_PROMPT);
        fflush(stdout);
        line = read_line();
        if (line == NULL || strlen(trim_whitespace(line)) == 0) {
            free(line);
            continue;
        }
        args = tokenize_line(line);
        free(line);
        free_tokens(args);
    }
}

/*
 * Read a line of input from the user
 * Returns: dynamically allocated string containing the line 
 */
char *read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            printf("\n");
            exit(0); // EOF encoutered
        } else {
            perror("shell: getline");
            exit(1);
        }
    }

    return line;
}

/**
 * Trim leading and trailing whitespace
 */
char *trim_whitespace(char *str)
{
    char *end;
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    if (*str == '\0') {
        return str;
    }
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}

/**
 * Split a line into tokens (arguments)
 * Returns array of strings (tokens)
 */
char **tokenize_line(char *line) 
{
    int bufsize = MAX_NUM_TOKENS;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    const char *delim = " \t\r\n\a";
    if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(1);
    }
    token = strtok(line, delim);
    while (token != NULL) {
        tokens[position] = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(tokens[position++], token);
        if (position >= bufsize) {
            bufsize += MAX_NUM_TOKENS;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(1);
            }
        }
        token = strtok(NULL, delim);
    }
    tokens[position] = NULL;
    return tokens;
}

void free_tokens(char **tokens)
{
    if (tokens == NULL) {
        return;
    }
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}