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

Variable *get_variable(struct hashmap *scope, char *name) {
    Variable *variable = (Variable *) hashmap_get(scope, &(Variable) {
        .name = name
    });

    if (!variable) {
        variable = (Variable *) hashmap_get(variable_map, &(Variable) {
            .name = name
        });
    }
    return variable == NULL ? NULL : variable;
}

void deep_copy_variable(void *dest, const void *src, void *udata) {
    const Variable *src_var = (const Variable *)src;
    Variable *dest_var = (Variable *)dest;

    // Copy name
    dest_var->name = src_var->name ? strdup(src_var->name) : NULL;

    dest_var->type = src_var->type;

    switch (src_var->type) {
        case VAR_NUM:
            dest_var->value.num_val = src_var->value.num_val;
            break;
        case VAR_STR:
            dest_var->value.str_val = src_var->value.str_val ? strdup(src_var->value.str_val) : NULL;
            break;
        case VAR_LIST:
            dest_var->value.list_val.element_type = src_var->value.list_val.element_type;
            dest_var->value.list_val.nested_element_type = src_var->value.list_val.nested_element_type;
            dest_var->value.list_val.is_nested = src_var->value.list_val.is_nested;
            dest_var->value.list_val.head = deep_copy_list(src_var->value.list_val.head);
            break;
    }
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
        ListNode *next = current->next;

        if (current->element.type == VAR_STR && current->element.value.str_val) {
            free(current->element.value.str_val);
        } else if (current->element.type == VAR_LIST && current->element.value.nested_list.head) {
            free_list(current->element.value.nested_list.head);
        }

        free(current);
        current = next;
    }
}

char *list_to_string(const ListNode *head, VarType element_type) {
    size_t buffer_size = 256;
    char *buffer = safe_malloc(buffer_size);
    size_t length = 0;

    buffer[length++] = '[';

    const ListNode *current = head;
    while (current != NULL) {
        if (current != head) {
            buffer[length++] = ',';
            buffer[length++] = ' ';
        }

        // Ensure we have enough space in the buffer
        if (length + 128 >= buffer_size) {
            buffer_size *= 2;
            buffer = safe_realloc(buffer, buffer_size);
        }

        if (element_type == VAR_NUM) {
            char *num_str = trim_double(current->element.value.num_val);
            size_t num_len = strlen(num_str);

            // Ensure we have enough space for the number
            if (length + num_len + 1 >= buffer_size) {
                buffer_size = length + num_len + 128;
                buffer = safe_realloc(buffer, buffer_size);
            }

            strcpy(buffer + length, num_str);
            length += num_len;
            free(num_str);

        } else if (element_type == VAR_STR) {
            char *str_val = current->element.value.str_val;
            size_t str_len = strlen(str_val);

            // Ensure we have enough space for the string
            if (length + str_len + 3 >= buffer_size) {
                buffer_size = length + str_len + 128;
                buffer = safe_realloc(buffer, buffer_size);
            }

            buffer[length++] = '"';
            strcpy(buffer + length, str_val);
            length += str_len;
            buffer[length++] = '"';

        } else if (element_type == VAR_LIST) {
            // Handle nested list by recursively converting it to string
            char *nested_str = list_to_string(current->element.value.nested_list.head,
                                           current->element.value.nested_list.element_type);
            size_t nested_len = strlen(nested_str);

            // Ensure we have enough space for the nested list
            if (length + nested_len + 1 >= buffer_size) {
                buffer_size = length + nested_len + 128;
                buffer = safe_realloc(buffer, buffer_size);
            }

            strcpy(buffer + length, nested_str);
            length += nested_len;
            free(nested_str);
        }

        current = current->next;
    }

    // Ensure we have space for the closing bracket
    if (length + 2 >= buffer_size) {
        buffer_size = length + 32;
        buffer = safe_realloc(buffer, buffer_size);
    }

    buffer[length++] = ']';
    buffer[length] = '\0';

    return buffer;
}


ListNode *deep_copy_list(const ListNode *head) {
    if (!head) return NULL;

    ListNode *new_head = NULL;
    ListNode *current = NULL;

    const ListNode *src = head;
    while (src != NULL) {
        ListElement new_element;
        new_element.type = src->element.type;

        if (new_element.type == VAR_NUM) {
            new_element.value.num_val = src->element.value.num_val;
        } else if (new_element.type == VAR_STR) {
            new_element.value.str_val = strdup(src->element.value.str_val);
        } else if (new_element.type == VAR_LIST) {
            // Handle nested lists with recursive deep copy
            new_element.value.nested_list.element_type = src->element.value.nested_list.element_type;
            new_element.value.nested_list.nested_element_type = src->element.value.nested_list.nested_element_type;
            new_element.value.nested_list.is_nested = src->element.value.nested_list.is_nested;
            new_element.value.nested_list.head = deep_copy_list(src->element.value.nested_list.head);
        }

        ListNode *new_node = create_list_node(new_element);

        if (new_head == NULL) {
            new_head = new_node;
            current = new_head;
        } else {
            current->next = new_node;
            current = new_node;
        }

        src = src->next;
    }

    return new_head;
}