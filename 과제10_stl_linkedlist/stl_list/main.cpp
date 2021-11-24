#include "list"
#include <iostream>

using std::cout; using std::endl;
int main()
{
	list<int> l;
	l.push_back(10);
	l.push_front(20);

	for (auto iter = l.begin(); iter != l.end(); ++iter)
	{
		printf("%d ", *iter);
	}
	printf("\n");
	l.erase(l.begin());
	list<int>::iterator iter(l.begin());
	list<int>::iterator iter2(l.begin());

	cout << (iter == iter2) << endl;
	for (auto iter = l.begin(); iter != l.end(); ++iter)
	{
		printf("%d", *iter);
	}
	return 0;
}