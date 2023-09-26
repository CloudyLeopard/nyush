#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"
job_node *headp;

void print_list()
{
    job_node *p = headp;
    int i = 1;
    while (p)
    {
        printf("[%d] %s\n", i, p->cmd_line); // each line should end with a \n on its own
        i++;
        p = p->next;
    }
}

void add_node_to_end(int process_pid, char *cmd_line)
{
    job_node *n = (job_node *)malloc(sizeof(job_node));
    strcpy(n->cmd_line, cmd_line);
    n->pid = process_pid;
    n->next = NULL;

    if (headp == NULL) {
        headp = n;
        return;
    }

    job_node *p = headp;
    while (p->next)
        p = p->next;
    p->next = n;
    
}

job_node *find(job_node *head, int num, job_node **predp)
{
    job_node *n = head;
    job_node *pred = NULL;
    int i = 1;

    while (n)
    {
        if (i == num)
            break;
        pred = n;
        n = n->next;
        i++;
    }
    *predp = pred;
    return n;
}

job_node *remove_node(int n)
{
    job_node *curr;
    job_node *pred;
    curr = find(headp, n, &pred);
    
    if (!curr) // if node DNE
        return NULL;
    if (!pred) // if node is first node
        headp = curr->next;
    else
        pred->next = curr->next;
    return curr;
}

int listIsEmpty() {
    return headp == NULL;
}

// int main()
// {
//     printf("%d", listIsEmpty());
//     char *message = "blah blah blah";
//     for (int i = 10; i < 20; i++)
//     {
//         add_node_to_end(i, message);
//     }
//     print_list(headp);
//     printf("--------");
//     int num = 5;
//     int pid = remove_node(num);
//     printf("REMOVED: %d, pid=%d\n", num, pid);
//     print_list(headp);
// }