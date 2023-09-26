typedef struct job_node{
    struct job_node *next;
    int pid;
    char cmd_line[200];
} job_node;

void print_list();
void add_node_to_end(int process_pid, char *cmd_line);
job_node *remove_node(int n); 
int listIsEmpty();