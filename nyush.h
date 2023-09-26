// signal handling
void handler();
// suspension signal handling
void suspension_handler();

// get prompt from path
char *get_prompt(char *cwd);
// find program given user input
char *find_program(char *prog);
// execute program
int program_exec(char **argv);