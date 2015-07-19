#include <stdlib.h>
#include "list.h"

pNode new_node()
{
	pNode _node = NULL;
	
	_node = malloc(sizeof(struct Node));
	
	if(_node)
		_node->elem = _node->next = NULL;
	
	return _node;
}

pList new_list()
{
	pList _list = NULL;
	
	_list = malloc(sizeof(struct Head));
	
	if (_list) 
		_list->first = _list->last = NULL;
	
	return _list;
}

/*Insert after pos
 * Insert first when _list == pos
 * Append if pos is NULL*/
int add_list_node(pList _list, void *elem, pNode pos)
{
	pNode _node = NULL;
	
	_node = new_node();
	
	if(_node && _list) {
		_node->elem = elem;
		
		if(pos) {
			if( (void *)pos == (void *)_list ) {
				_node->next = _list->first;
				_list->first = _node;
				if(!_list->last)
					_list->last = _list->first;
			}
			else if(_list->last == pos) {
				_list->last->next = _node;
			}
			else {
				_node->next = pos->next;
				pos->next = _node;
			}
		}
		else {
			if(_list->first)
				if(_list->first->next) {
					_list->last->next = _node;
					_list->last = _node;
				}
				else {
					_list->last = _node;
					_list->first->next = _node;
				}
			else
				_list->last = _list->first = _node;
		}
	}
	else
		return -1;
	
	return 0;
}

/* Return NULL if there is no prev node(inconsistent list) or node is first */
pNode get_prev_node(pList _list, pNode _node)
{
	pNode prev_node = NULL;
	
	if(_list && _node) {
		
		if (_node != _list->first) {
			prev_node = _list->first;
		}
		else {
			return NULL;
		}
		
		while(prev_node->next != _node)
			
			if(prev_node->next != NULL) {
				prev_node = prev_node->next;
			}
			else {
				prev_node = NULL;
				break;
			}
	}
	
	return prev_node;
}

pNode get_node_by_index(pList _list, int index)
{
	pNode _node;
	int i = 0;
	
	_node = _list->first;
	
	for(i=0; i<index; i++) {
		if(_node->next)
			_node = _node->next;
		else {
			_node = NULL;
			break;
		}
	}
	
	return _node;
}

int delete_list_node(pList _list, pNode _node)
{
	int ret = -1;

	if(_list && _node) {
		if(_node == _list->first) {
			if(_node != _list->last)
				_list->first = _node->next;
			else
				_list->first = _list->last = NULL;
		}
		else {
			pNode prev_node = get_prev_node(_list, _node);
			
			if(prev_node) {
				prev_node->next = _node->next;
				if(_node->next == NULL)
					_list->last = prev_node;
			}
			else {
				/*ERROR: inconsistent list*/
				return ret;
			}
		}
		delete_node(_node);
		ret = 0;
	}
	return ret;
}

void destroy_list(pList _list)
{
	if(_list) {
		empty_list(_list);
		free(_list);
	}
}

void empty_list(pList _list)
{
	if(_list) {
		if(_list->first) {
			pNode nextNode = _list->first;
			while(nextNode->next) {
				pNode thisNode = nextNode;
				nextNode = nextNode->next;
				free(thisNode);
			}
			free(nextNode);
		}
		_list->first = _list->last = NULL;
	}
}

void delete_node(pNode _node)
{
	if(_node) {
		if(_node->elem)
			free(_node->elem);
		free(_node);
	}
}

