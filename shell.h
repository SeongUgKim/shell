#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_HISTORY_SIZE 1000
#define SHELL_PROMPT "shell> "

void shell_loop(void);
char *read_line(void);
char *trim_whitespace(char *str);
char **tokenize_line(char *line);
void free_tokens(char **tokens);
int execute_command(char **args);
int num_builtins(void);
int launch_program(char **args);
int shell_cd(char **args);
int shell_pwd(char **args);
int shell_exit(char **args);
int shell_help(char **args);
int shell_history(char **args);
int count_pipes(char **args);
int handle_pipes(char **args);
#endif /* SHELL_H */