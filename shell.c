#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"

static volatile sig_atomic_t interrupted = 0;
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
        status = execute_command(args);
        free(line);
        free_tokens(args);
        interrupted = 0;
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

int execute_command(char **args)
{
    if (args[0] == NULL) {
        return 1;
    }
    if (interrupted) {
        return 1;
    }
    // for (int i = 0; i < num_builtins(); ++i) {
    //     if (strcmp(args[0], builtin_str[i]) == 0) {
    //         return (*builtin_func[i])(args);
    //     }
    // }
    return launch_program(args); 
}

int num_builtins(void)
{
    return sizeof(builtin_str) / sizeof(char *);
}

int launch_program(char **args)
{
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(1);
    } else if (pid < 0) {
        perror("shell: fork");
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}
