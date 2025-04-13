#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pwd.h>

#define BUFFER_SIZE 1024

// macro to move cursor using ansi escape codes
#define moveto(x,y) printf("\033[%d;%dH",y,x)

// list containing the tasks
typedef struct tasks_list {
    // name of the task
    char *name;
    // next in the list
    struct tasks_list *next;
}tasks_list_t;

// file linked to .tasks
char tasks_path[BUFFER_SIZE];

// adds the new task name to the list specified
tasks_list_t *add_task_to_list_tail(tasks_list_t *head, char *name) {
    if (name == NULL)
        return head;
    tasks_list_t* new_task = malloc(sizeof(tasks_list_t));
    // failed to allocate
    if (!new_task)
        return NULL;
    char *task_name = strdup(name);

    if (head == NULL) {
        // no elements
        new_task -> next = NULL;
        new_task -> name = task_name;
        head = new_task;
    } else if (head -> next == NULL) {
        // only one element
        head -> next = new_task;
        new_task -> next = NULL;
        new_task -> name = task_name;
    }
    else {
        // general case: more than one element
        tasks_list_t *current = head;
        while (current -> next != NULL) {
            current = current -> next;
        }
        // set node
        current -> next = new_task;
        new_task -> name = task_name;
        new_task -> next = NULL;
    }

    // return new head
    return head;
}

// gets the size of the list of given head
int get_task_list_size(tasks_list_t *head) {
    tasks_list_t* current = head;
    int size = 0;
    while (current != NULL) {
        size++;
        current = current -> next;
    }
    return size;
}

// removes the name from the task list specified; removes the first occurrence of said name
tasks_list_t* remove_task_from_list(tasks_list_t *head, char *name) {

    tasks_list_t* current = head;
    tasks_list_t* prev = NULL;
    while (current != NULL) {

        if (!strcmp(current -> name, name)) {
            // found item
            if (prev == NULL) {
                head = current -> next;
            } else {
                prev -> next = current -> next;
            }
            free(current -> name);
            free(current);
            break;
        }
        // update
        prev = current;
        current = current -> next;
    }
    return head;
}

// deletes the task list (frees memory)
void free_tasks(tasks_list_t *head) {
    while (head != NULL) {
        tasks_list_t *next = head -> next;
        free(head -> name);
        free(head);
        head = next;
    }

}

bool is_task_in_list(tasks_list_t *head, char *name) {
    if (name == NULL)
        return false;
    tasks_list_t *current = head;
    while (current != NULL) {
        if (!strcmp(current -> name, name))
            return true;
        current = current -> next;
    }
    return false;
}

void flush_tasks_from_screen() {
    moveto(0, 3);
    for (int i = 0; i < 20; i++)
        printf("                                                              \n");
}

void print_tasks(const tasks_list_t *head) {
    moveto(0, 3);
    while (head != NULL) {
        printf("%s\n", head -> name);
        head = head -> next;
    }
}

int main(void) {

    system("clear");
    printf("Input:\n");
    char input[BUFFER_SIZE];

    tasks_list_t *todo_head = NULL;
    tasks_list_t *done_head = NULL;

    // get home directory
    const struct passwd *pw = getpwuid(getuid());
    const char* homedir = pw -> pw_dir;
    strncpy(tasks_path, homedir, BUFFER_SIZE);
    strncat(tasks_path, "/.tasks", BUFFER_SIZE - strlen("/.tasks"));

    // open file
    FILE* file_start = fopen(tasks_path, "rw");
    if (file_start) {
        char task[BUFFER_SIZE];
        // read file contents
        while (!feof(file_start)) {
            if (fscanf(file_start, " %[^\n]", task) == EOF)
                break;
            char* type = strtok(task, " ");
            char* name = strtok(NULL, "\n");
            if (!strcmp(type, "TODO")) {
                todo_head = add_task_to_list_tail(todo_head, name);
            } else if (!strcmp(type, "DONE")) {
                done_head = add_task_to_list_tail(done_head, name);
            }
        }
        fclose(file_start);
    }

    // print tasks in file
    print_tasks(todo_head);

    moveto(8, 0);
    while (scanf(" %[^\n]", input) != EOF) {
        // flush stdin
        fflush(stdin);

        // clear console
        moveto(0, 2);
        printf("                              ");

        // get instruction and value
        char* instruction = strtok(input, " ");
        char* value = strtok(NULL, "\n");

        if (!strcmp(instruction, "add")) {
            todo_head = add_task_to_list_tail(todo_head, value);
            moveto(0, 2);
            printf("Added task");
            // write to file
            FILE *file = fopen(tasks_path, "a");
            if (file) {
                fprintf(file, "TODO %s\n", value);
                fclose(file);
            }
            // print
            flush_tasks_from_screen();
            print_tasks(todo_head);
        } else if (!strcmp(instruction, "com")) {
            if (is_task_in_list(todo_head, value)) {
                todo_head = remove_task_from_list(todo_head, value);
                done_head = add_task_to_list_tail(done_head, value);

                // write to file
                FILE *file = fopen(tasks_path, "r+");
                if (file) {
                    char to_check[BUFFER_SIZE];
                    strcpy(to_check, "TODO ");
                    strncat(to_check, value, BUFFER_SIZE - strlen("TODO "));
                    char current[BUFFER_SIZE];
                    // save beginning of line so that it can be overwritten
                    long pos = ftell(file);
                    while (fscanf(file, " %[^\n]", current) != EOF) {
                        if (!strcmp(to_check, current)) {
                            // found it: overwrite line with DONE
                            fseek(file, pos, SEEK_SET);
                            fprintf(file, "\nDONE %s\n", value);
                            break;
                        }
                        pos = ftell(file);
                    }
                    fclose(file);
                }
                moveto(0, 2);
                printf("Marked task as complete");
            } else {
                // invalid command
                moveto(0, 2);
                printf("Not found.");
            }

            // print
            flush_tasks_from_screen();
            print_tasks(todo_head);
        } else if (!strcmp(instruction, "todo")) {
            moveto(0, 2);
            printf("Printing TODO");
            // print
            flush_tasks_from_screen();
            print_tasks(todo_head);

        } else if (!strcmp(instruction, "done")) {
            moveto(0, 2);
            printf("Printing DONE");
            // print
            flush_tasks_from_screen();
            print_tasks(done_head);
        } else {
            // invalid command
            moveto(0, 2);
            printf("Not a valid command!");
        }

        // return cursor to input
        moveto(8, 0);
        printf("                                ");
        moveto(8, 0);
    }

    // free memory
    if (todo_head)
        free_tasks(todo_head);
    if (done_head)
        free_tasks(done_head);

    // clear terminal
    system("clear");
    return 0;
}