//
// Created by refexchange on 2018/5/3.
//

#ifndef CONTACTS_LIST_H
#define CONTACTS_LIST_H

#ifndef offset_of
#define offset_of(type, member) ((size_t)&(((type*)0)->member))
#endif

#ifndef container_of
#define container_of(pointer, type, member) ({ \
    const typeof(((type*)0)->member) *__mpointer = (pointer); \
    (type*)((char*)__mpointer - offset_of(type, member));})
#endif

typedef struct list_node_t {
    struct list_node_t *next;
    struct list_node_t *prev;
} list_node;

#define list_init_node(name) ((name).next = (name).prev = &(name))
#define list_check_end(eptr) (&((eptr)->node) == (eptr)->node.next)
#define list_check_begin(eptr) (&((eptr)->node) == (eptr)->node.prev)
#define list_next_entry(src) ((typeof(src))(src)->node.next)
#define list_prev_entry(src) ((typeof(src))(src)->node.prev)

#define list_unchain(target) ({ \
    list_node *__left = (target)->node.prev; \
    list_node *__right = (target)->node.next; \
    __left->next = list_check_end(target) ? __left : __right; \
    __right->prev = list_check_begin(target) ? __right : __left; \
    list_init_node((target)->node); \
})

#define list_insert_before(dest, src) ({ \
    list_node *__left = &((src)->node); \
    list_node *__right = &((dest)->node); \
    __left->next = __right; \
    __left->prev = __right->prev == __right ? __left : __right->prev; \
    __right->prev = __left; \
})

#define list_insert_after(dest, src) ({ \
    list_node *__left = &((dest)->node); \
    list_node *__right = &((src)->node); \
    __right->prev = __left; \
    __right->next = __left->next == __left ? __right : __left->next; \
    __left->next = __right; \
})

#define list_successor(eptr) ({\
	typeof(eptr) __entry = (eptr);\
    while (!list_check_end(__entry)) __entry = list_next_entry(__entry);\
    __entry;\
})

#define list_predecessor(eptr) ({\
	typeof(eptr) __entry = (eptr);\
    while (!list_check_begin(__entry)) __entry = list_prev_entry(__entry);\
    __entry;\
})

#define list_entry_new(type, x) ({ \
    type *__new_entry = (type*)malloc(sizeof(type)); \
    __new_entry->value = (x); \
    list_init_node(__new_entry->node);\
    __new_entry; \
})

#define list_instance_new(type_name) (type_name*)calloc(1, sizeof(type_name))
#define list_instance_append(instance, src) do { \
    if (!(instance)->head || !(instance)->tail) { \
        (instance)->head = (instance)->tail = (src); \
        (instance)->count = 1; \
    } else { \
        list_insert_after((instance)->head, src); \
        (instance)->head = src; \
        (instance)->count++; \
    } \
} while(0)

#define CLION_SHUT_THE_FUCK_UP (a, b) list_insert_before(a, b)
#endif //CONTACTS_LIST_H
