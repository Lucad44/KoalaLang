#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdint.h>

#include "hashmap.h"

struct ListNode;

typedef enum {
    VAR_NUM,
    VAR_STR,
    VAR_LIST
} VarType;

typedef struct {
    VarType element_type;
    VarType nested_element_type; // Type of elements in nested lists
    bool is_nested;              // Whether this is a list of lists
    struct ListNode *head;
} NestedList;

typedef struct {
    VarType type;
    union {
        double num_val;
        char *str_val;
        NestedList nested_list;  // For lists of lists
    } value;
} ListElement;

typedef struct ListNode {
    ListElement element;
    struct ListNode *next;
} ListNode;

typedef struct {
    union {
        double num_val;
        char *str_val;
        struct {
            VarType element_type;
            VarType nested_element_type; // Type of elements in nested lists
            bool is_nested;              // Whether this is a list of lists
            ListNode *head;
        } list_val;
    };
} VariableValue;

typedef struct {
    char *name;
    VarType type;
    VariableValue value;
} Variable;

extern struct hashmap *variable_map;

int variable_compare(const void *a, const void *b, void *udata);

bool variable_iter(const void *item, void *udata);

uint64_t variable_hash(const void *item, uint64_t seed0, uint64_t seed1);

char *trim_double(double value);

ListNode *create_list_node(ListElement element);

void free_list(ListNode *head);

char *list_to_string(const ListNode *head, VarType element_type);

ListNode *deep_copy_list(const ListNode *head);

#endif