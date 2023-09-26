#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>

#include "nyush.h"
#include "list.h"

// global variables
char cwd[100];
char *line;
char cmd_line[200];

int main()
{
    // -- SIGNAL HANDLING --
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGTSTP, handler);

    // --- GET PROMPT ---
    // get full path of current directory
    getcwd(cwd, sizeof(cwd));
    char *prompt;
    prompt = get_prompt(cwd);

    // buffer that stores user input
    size_t len = 0;

    while (1)
    {
        // print prompt
        printf("%s", prompt);
        fflush(stdout);

        // // -- GET COMMAND--
        if (getline(&line, &len, stdin) < 0)
        {
            // exit if getline is -1 (ctrl+d, or EOF)
            exit(0);
        }

        char *tmp = line;
        int count = 0; // count number of spaces in command
        while (*tmp)
        {
            if (*tmp == ' ')
                count++;
            if (*tmp == '\n')
                // replace the ending new line char with null char
                *tmp = '\0';
            tmp++;
        }

        strcpy(cmd_line, line);

        // array to store args for exec
        char *argv[count + 2]; // +1 for the missed word (x spaces => x+1 words), +1 for NULL

        // get tokens from cmd separated by space, and store into argv
        count = 0;
        char *saveptr = line;
        char *token;
        while ((token = strtok_r(saveptr, " ", &saveptr)))
        {
            argv[count] = token;
            count++;
        }
        argv[count] = NULL; // argv must end with NULL (due to exec specification)

        // if input command is empty or just spaces, continue
        if (argv[0] == NULL)
            continue;

        // checking for invalid command (|, <, > occurs at the beginning or the end)
        if (strcmp(argv[0], "|") == 0 || strcmp(argv[0], "<") == 0 || strcmp(argv[0], ">") == 0)
        {
            fprintf(stderr, "Error: invalid command\n");
            continue;
        }
        else if (count > 0 && (strcmp(argv[count-1], "|") == 0 || strcmp(argv[count-1], "<") == 0 || strcmp(argv[count-1], ">") == 0))
        {
            fprintf(stderr, "Error: invalid command\n");
            continue;
        }

        // -- BUILT IN COMMANDS --
        if (strcmp(argv[0], "cd") == 0)
        {
            if (argv[1] == NULL || argv[2] != NULL)
            {
                fprintf(stderr, "Error: invalid command\n");
                continue;
            }

            // change directory
            if (chdir(argv[1]) == -1)
            {
                fprintf(stderr, "Error: invalid directory\n");
                continue;
            }
            else
            {
                getcwd(cwd, sizeof(cwd));
                prompt = get_prompt(cwd);
            }
            continue;
        }
        else if (strcmp(argv[0], "jobs") == 0)
        {
            if (argv[1] != NULL)
            {
                fprintf(stderr, "Error: invalid command\n");
                continue;
            }

            print_list();
            continue;
        }
        else if (strcmp(argv[0], "fg") == 0)
        {
            if (argv[1] == NULL || argv[2] != NULL)
            {
                fprintf(stderr, "Error: invalid command\n");
                continue;
            }
            int n = atoi(argv[1]);

            job_node *node = remove_node(n);

            if (!node) // if node is null (cannot find)
                fprintf(stderr, "Error: invalid job\n");
            else
            {
                int pid = node->pid;
                strcpy(cmd_line, node->cmd_line);
                free(node); // free up node space

                int status;
                kill(pid, SIGCONT);               // continue suspended process
                waitpid(pid, &status, WUNTRACED); // wait for process now that it continues
                if (WIFSTOPPED(status))
                    add_node_to_end(pid, cmd_line);
                continue;
            }

            continue;
        }
        else if (strcmp(argv[0], "exit") == 0)
        {
            if (argv[1] != NULL)
            {
                fprintf(stderr, "Error: invalid command\n");
                continue;
            }
            if (!listIsEmpty())
            {
                fprintf(stderr, "Error: there are suspended jobs\n");
                continue;
            }
            exit(0);
        }

        // -- PROGRAM EXECUTION --
        program_exec(argv);
    }
}

// signal handler for SIGINT, SIGQUIT, SIGTSTP
void handler()
{
    // printf("\n");
    ;
}

// get prompt from path
char *get_prompt(char *cwd)
{
    // loop through path until we arrive at last token after "/"
    char basename[100];
    strcpy(basename, cwd);
    char *token;
    char *ptr = basename;
    while ((token = strtok_r(ptr, "/", &ptr)))
        strcpy(basename, token);

    // stores prompt (i.e. [nyush /])
    char *prompt = (char *)malloc(100 * sizeof(char));
    strcpy(prompt, "[nyush ");
    strcat(prompt, basename);
    strcat(prompt, "]$ ");

    return prompt;
}

char *find_program(char *prog)
{
    // parsing "path" to command
    char *path = (char *)malloc(100 * sizeof(char));
    if (prog[0] == '/') // case 1: absolute path (begins with /)
        strcpy(path, prog);
    else if (strstr(prog, "/"))
    { // case 2: relative path (contains but does not begin with /)
        strcat(strcat(strcpy(path, cwd), "/"), prog);
    }
    else
    { // case 3: base name
        strcat(strcpy(path, "/usr/bin/"), prog);
    }
    return path;
}

int program_exec(char **argv)
{
    int status;
    // argv[0] = find_program(argv[0]);
    char *arg = find_program(argv[0]);

    int more_redirect = 0;    // 1 if remaining args have >, >>, or |
    char **next_argv;         // remaining args after <, >, >>, or |
    int re_input_index = -1;  // index of the token after <
    int re_output_index = -1; // index of the token after >
    int re_output_append = 0; // 0 if >, 1 if >>
    int i = 0;                // loop through argv
    while (argv[i])
    {
        if (strcmp(argv[i], "<") == 0)
        {
            re_input_index = i + 1;
            argv[i] = NULL;
        }
        else if (strcmp(argv[i], ">") == 0)
        {
            re_output_index = i + 1;
            argv[i] = NULL;
            more_redirect = 1;
        }
        else if (strcmp(argv[i], ">>") == 0)
        {
            re_output_index = i + 1;
            argv[i] = NULL;
            re_output_append = 1;
            more_redirect = 1;
        }
        else if (strcmp(argv[i], "|") == 0)
        {
            argv[i] = NULL;
            next_argv = &argv[i + 1];
            more_redirect = 1;
            break;
        }
        i++;
    }

    // run this if there is no more redirection (i.e. no more >, >>, or |)
    if (!more_redirect)
    {
        // fork and run program in child process
        int fork_id = fork();
        if (fork_id == 0)
        {
            // if there is input redirection, redirect stdinp to input file
            if (re_input_index >= 0)
            {
                int fd = open(argv[re_input_index], O_RDONLY, 0777);
                if (fd < 0)
                {
                    fprintf(stderr, "Error: invalid file\n");
                    exit(-1);
                }
                if (dup2(fd, STDIN_FILENO) == -1)
                    fprintf(stderr, "Something went wrong with pipe\n!");
                re_input_index = -1;
                close(fd);
            }

            if (execv(arg, argv) == -1)
                fprintf(stderr, "Error: invalid program\n");
            exit(-1);
        }

        // wait for process
        waitpid(fork_id, &status, WUNTRACED);
        if (WIFSTOPPED(status)) // if process is stopped, add to list of stopped jobs
            add_node_to_end(fork_id, cmd_line);
        return 0;
    }

    // piping for redirections
    int pipefd[2];
    if (pipe(pipefd) == -1)
        fprintf(stderr, "Something went wrong with pipe!\n");

    int fork_id_1 = fork();
    if (fork_id_1 == 0) // child process
    {
        close(pipefd[0]);

        // if there is input redirection, do that:
        if (re_input_index >= 0)
        {
            int fd = open(argv[re_input_index], O_RDONLY, 0777);
            if (fd < 0)
            {
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            }
            if (dup2(fd, STDIN_FILENO) == -1)
                fprintf(stderr, "Something went wrong with pipe\n!");
            re_input_index = -1;
            close(fd);
        }

        // redirect stdout to write end of pipefd
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
            fprintf(stderr, "Something went wrong with pipe\n!");

        close(pipefd[1]);

        // execute with args
        if (execv(arg, argv) == -1)
            fprintf(stderr, "Error: invalid program\n");

        exit(-1);
    }
    // fork to send output to stdinp without changing stdinp's fd in parent
    int fork_id_2 = fork();
    if (fork_id_2 == 0)
    {
        // child process

        close(pipefd[1]);

        // if there is output redirection, do that
        if (re_output_index >= 0)
        {
            // open/create new file at ptr (the token after ">")
            int fd;
            if (re_output_append) // >>
                fd = open(argv[re_output_index], O_WRONLY | O_CREAT | O_APPEND, 0777);
            else // >
                fd = open(argv[re_output_index], O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (fd < 0)
                fprintf(stderr, "something went wrong");

            char buff;
            while (read(pipefd[0], &buff, 1) > 0)
                write(fd, &buff, 1);

            re_output_index = -1;
            close(fd);
        }
        // otherwise, continue piping
        else
        {
            // stdinput becoms read end of pipfd
            if (dup2(pipefd[0], STDIN_FILENO) == -1)
                fprintf(stderr, "Something went wrong!");

            close(pipefd[0]);

            // execute with args
            program_exec(next_argv);
        }

        exit(0);
    }
    else
    {
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(fork_id_1, &status, WUNTRACED);
        if (WIFSTOPPED(status)) // if process is stopped, add to list of stopped jobs
            add_node_to_end(fork_id_1, cmd_line);

        waitpid(fork_id_2, &status, WUNTRACED);
        if (WIFSTOPPED(status)) // if process is stopped, add to list of stopped jobs
            add_node_to_end(fork_id_2, cmd_line);
        free(arg);
    }
    return 0;
}
