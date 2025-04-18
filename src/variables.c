#include <stdio.h>
#include <string.h>
#include "variables.h"

#include <stdlib.h>

#include "hashmap.h"
#include "memory.h"

struct hashmap *variable_map = NULL;

static void __attribute__((constructor)) init_variable_map() {
    variable_map = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);
}

int variable_compare(const void *a, const void *b, void *udata) {
    const Variable *va = a;
    const Variable *vb = b;
    return strcmp(va->name, vb->name);
}

bool variable_iter(const void *item, void *udata) {
    const Variable *variable = item;
    if (variable->type == VAR_NUM) {
        printf("%s = %f\n", variable->name, variable->value.num_val);
    } else if (variable->type == VAR_STR) {
        printf("%s = %s\n", variable->name, variable->value.str_val);
    }
    return true;
}

uint64_t variable_hash(const void *item, const uint64_t seed0, const uint64_t seed1) {
    const Variable *variable = item;
    return hashmap_sip(variable->name, strlen(variable->name), seed0, seed1);
}

char *trim_double(const double value) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%.15f", value);

    const char *dot = strchr(buffer, '.');
    if (dot) {
        char *end = buffer + strlen(buffer) - 1;
        while (end > dot && *end == '0') {
            *end-- = '\0';
        }
        if (end == dot) {
            *end = '\0';
        }
    }
    return strdup(buffer);
}

ListNode *create_list_node(const ListElement element) {
    ListNode *node = safe_malloc(sizeof(struct ListNode));
    if (!node) {
        perror("Failed to allocate memory for list node");
        return NULL;
    }
    node->element = element;
    node->next = NULL;
    return node;
}

void free_list(ListNode *head) {
    ListNode *current = head;

    while (current != NULL) {
        ListNode *next_node = current->next;
        if (current->element.type == VAR_STR && current->element.value.str_val != NULL) {
            free(current->element.value.str_val);
            current->element.value.str_val = NULL; // Good practice
        }
        free(current);

        current = next_node;
    }
}

char *list_to_string(const ListNode *head, const VarType element_type) {
    char buffer[4096] = {0};
    const ListNode *current = head;
    strcat(buffer, "[");
    while (current != NULL) {
        if (element_type == VAR_NUM) {
            const char *trimmed = trim_double(current->element.value.num_val);
            strncat(buffer, trimmed, strlen(trimmed));
        } else if (element_type == VAR_STR) {
            strncat(buffer, current->element.value.str_val, strlen(current->element.value.str_val));
        }
        if (current->next != NULL) {
            strcat(buffer, ", ");
        }
        current = current->next;
    }
    strcat(buffer, "]\n");
    return strdup(buffer);
}

ListNode *copy_list(const ListNode *head, const VarType element_type) {
    if (!head) {
        return NULL;
    }

    ListNode *new_head = NULL;
    ListNode *new_tail = NULL;
    const ListNode *current = head;

    while (current != NULL) {
        ListElement new_element;
        new_element.type = element_type;

        if (element_type == VAR_NUM) {
            new_element.value.num_val = current->element.value.num_val;
        } else if (element_type == VAR_STR) {
            if (current->element.value.str_val != NULL) {
                new_element.value.str_val = strdup(current->element.value.str_val);
                if (!new_element.value.str_val) {
                    fprintf(stderr, "Error: Failed to duplicate string during list copy.\n");
                    free_list(new_head);
                    return NULL;
                }
            } else {
                new_element.value.str_val = NULL;
            }
        } else {
            fprintf(stderr, "Internal Error: Unsupported element type %d during list copy.\n", element_type);
             free_list(new_head);
             return NULL;
        }

        ListNode *new_node = create_list_node(new_element);
        if (!new_node) {
            fprintf(stderr, "Error: Failed to allocate node during list copy.\n");
             if (element_type == VAR_STR && new_element.value.str_val != NULL) {
                 free(new_element.value.str_val);
             }
            free_list(new_head);
            return NULL;
        }

        if (new_head == NULL) {
            new_head = new_node;
            new_tail = new_node;
        } else if (new_tail != NULL) {
            new_tail->next = new_node;
            new_tail = new_node;
        }

        current = current->next;
    }

    return new_head;
}
