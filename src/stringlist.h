#ifndef STRINGLIST_H
#define STRINGLIST_H

class StringList
{
	private:
		struct node
		{
            LPTSTR data;
            node *link;
		} * p;
		
	public:
		StringList();
		virtual ~StringList();
		void AddLast(LPCTSTR string);
		void AddFirst(LPCTSTR string);
		void InsertAfter(int index, LPCTSTR string);
		bool Contains(LPCTSTR string);
		void Remove(int index);
		void RemoveAll();
		void FromComplexString(LPTSTR string, TCHAR sep);
		int ToComplexString(LPTSTR buffer, int maxlen, TCHAR sep);
		int Size();
		LPCTSTR operator[](int index);
		
	protected:
		node * Get(int index);
};

#endif
