#ifndef __list_h
#define __list_h

struct Node;
struct Head;
typedef struct Node *pNode;
typedef struct Head *pList;

struct Node {
	void *elem;
	pNode next;
};

struct Head {
	pNode first;
	pNode last;
};

pList new_list();
void empty_list(pList _list);
void destroy_list(pList _list);
int add_list_node(pList _list, void *elem, pNode pos);
int delete_list_node(pList _list, pNode _node);

pNode new_node();
pNode get_prev_node(pList _list, pNode _node);
pNode get_node_by_index(pList _list, int index);
void delete_node(pNode _node);

#endif /* __list_h */
