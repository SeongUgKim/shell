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

int (*builtin_func[]) (char **) = {
    &shell_cd,
    &shell_pwd,
    &shell_exit,
    &shell_help,
    &shell_history
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
    if (count_pipes(args) > 0) {
        return handle_pipes(args);
    }
    for (int i = 0; i < num_builtins(); ++i) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
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

int shell_cd(char **args) 
{
    if (args[1] == NULL) {
        // no argument provided, go to home directory
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "shell: cd: HOME not set\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("shell: cd");
        } 
    } else {
        if (chdir(args[1]) != 0) {
            perror("shell: cd");
        }
    }
    return 1; 
}

int shell_pwd(char **args) 
{
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        perror("shell: pwd");
    } else {
        printf("%s\n", cwd);
        free(cwd);
    }
    return 1;
}

int shell_exit(char **args) 
{
    return 0;
}

int shell_help(char **args)
{
    printf("Shell - Coding Challenges Shell\n");
    printf("Built-in commands:\n");
    for (int i = 0; i < num_builtins(); ++i) {
        printf("    %s\n", builtin_str[i]);
    }
    printf("\nFeatures supported:\n");
    printf("    - External program execution\n");
    printf("    - Command pipes (|)\n");
    printf("    - Command history\n");
    printf("    - Signal handling (Ctrl+C)\n");
    printf("\nUse 'man <command>' for help on external programs\n");
    return 1;
}

int shell_history(char **args)
{
}

int count_pipes(char **args)
{
    int count = 0;
    for (int i = 0; args[i] != NULL; ++i) {
        if (strcmp(args[i], "|") == 0) {
            ++count;
        }
    }
    return count;
}

int handle_pipes(char **args)
{
    int pipe_count = count_pipes(args);
    int pipefd[2 * pipe_count];
    int cmd_start = 0;
    int i, j;
    pid_t pid;
    for (i = 0; i < pipe_count; ++i) {
        if (pipe(pipefd + i * 2) < 0) {
            perror("shell: pipe");
            return 1;
        }
    }
    for (i = 0; i <= pipe_count; ++i) {
        int cmd_end = cmd_start;
        while (args[cmd_end] != NULL && strcmp(args[cmd_end], "|") != 0) {
            cmd_end++;
        }
        char **cmd_args = malloc((cmd_end - cmd_start + 1) * sizeof(char*));
        for (j = 0; j < cmd_end - cmd_start; j++) {
            cmd_args[j] = args[cmd_start + j];
        }
        cmd_args[j] = NULL;
        pid = fork();
        if (pid == 0) {
            if (i > 0) { // Not first command
                dup2(pipefd[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < pipe_count) { // Not last command
                dup2(pipefd[i * 2 + 1], STDOUT_FILENO);
            }
            // close all pipe file descriptors
            for (j = 0; j < 2 * pipe_count; ++j) {
                close(pipefd[j]);
            }
            if (execvp(cmd_args[0], cmd_args) == -1) {
                perror("shell");
            }
            exit(1);
        } else if (pid < 0) {
            perror("shell: fork");
            free(cmd_args);
            break;
        }
        free(cmd_args);
        cmd_start = cmd_end + 1;
    }
    for (i = 0; i < 2 * pipe_count; ++i) {
        close(pipefd[i]);
    }
    for (i = 0; i <= pipe_count; ++i) {
        wait(NULL);
    }
    return 1;
}