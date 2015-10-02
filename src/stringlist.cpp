#include <windows.h>
#include "common.h"
#include "stringlist.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
StringList::StringList()
{
     p = NULL;
}
 
StringList::~StringList()
{
	RemoveAll();
}

void StringList::AddLast(LPCTSTR string)
{
	TR_START

	node *q, *t;

	q = p;
	while(q != NULL && q->link != NULL)
	{
		q = q->link;
	}
	
	t = new node;
	t->data = (LPTSTR) malloc((_tcslen(string) + 1) * sizeof(TCHAR));
	_tcscpy(t->data, string);
	t->link = NULL;
	if (q != NULL)
	{
		q->link = t;
	}
	else
	{
		p = t;
	}
			
	TR_END
}
 
void StringList::AddFirst(LPCTSTR string)
{
	TR_START

	node *q;

	q = new node;
	q->data = (LPTSTR) malloc((_tcslen(string) + 1) * sizeof(TCHAR));
	_tcscpy(q->data, string);
	q->link = p;
	p = q;
			
	TR_END
}
 
void StringList::InsertAfter(int index, LPCTSTR string)
{
	TR_START

	node *q, *t;

	q = Get(index);
	
	if (q != NULL)
	{
		t = new node;
		t->data = (LPTSTR)malloc((_tcslen(string) + 1) * sizeof(TCHAR));
		_tcscpy(t->data, string);
		t->link = q->link;
		q->link = t;
	}
			
	TR_END
}

void StringList::Remove(int index)
{
	TR_START

	node *q, *r;
	
	r = Get(index - 1);
	q = Get(index);
	if (q == NULL)
	{
		return;
	}

	if (r != NULL)
	{
		r->link = q->link;
	}
	else
	{
		p = q->link;
	}

	free(q->data);
	delete q;
			
	TR_END
}
 
int StringList::Size()
{
	TR_START

	node *q;
	int count = 0;
	for(q = p; q != NULL; q = q->link)
	{
		count++;
	}

	return count;
			
	TR_END0
}
 
void StringList::RemoveAll()
{
	TR_START

	node *q;

	while(p != NULL)
	{
		q = p->link;

		free(p->data);
		delete p;
		p = q;
	}
			
	TR_END
}

bool StringList::Contains(LPCTSTR string)
{
	TR_START

	node *r = p;

	while(r != NULL)
	{
		if(_tcscmp(r->data, string) == 0)
			return true;

		r = r->link;
	}

	return false;
			
	TR_END0
}

LPCTSTR StringList::operator[](int index)
{
	TR_START

	node * r = Get(index);
	return (r == NULL ? NULL : r->data);
			
	TR_END0
}

StringList::node * StringList::Get(int index)
{
	TR_START

	if (index < 0)
	{
		return NULL;
	}

	node * r = p;
	for (int it = 0; it <= index && r != NULL; ++it)
	{
		if (it == index)
		{
			return r;
		}
		else
		{
			r = r->link;
		}
	}

	return NULL;
			
	TR_END0
}