#include <iostream>
#include <cstdint>
using namespace std;

const int32_t MAX_STACK_DEPTH = 64;


template <typename T>
class Stack
{
    struct Node
    {
        T data;
        Node *next;
    };
    Node *top;
    int32_t count;

public:
    // Implement these functions:
    Stack()
    { 
        top = nullptr;
        count = 0;
    }
    void push(const T &val)
    {
        if(count >= MAX_STACK_DEPTH)
        {
            return;
        }

        Node* newNode = new Node();
        newNode->data = val;
        newNode->next = top;
        top = newNode;
        count++;

        // pushes the value on the stack if max limit is not reached yet.
    }
    T pop()
    {
        if(isEmpty())
        {
            return T();
        }
        Node* temp = top;
        T value = temp->data;
        top = top->next;
        delete temp;
        count--;

        return value;
        // pop the top value on the stack
    }
    T &peek()
    {
        return top->data;
        // returns the top value on the stack
    }
    bool isEmpty()
    {
        return top == nullptr;
    }
    int32_t depth()
    {
        return count;
    }
    int32_t snapshot_into(T out[], int32_t maxLen)
    {
        int32_t writeCt = 0;
        Node* current = top;

        while(current != nullptr && writeCt < maxLen)
        {
            out[writeCt] = current->data;
            writeCt++;
            current = current->next;
        }

        return writeCt;
        // copies every frame, top to bottom in the array given as a parameter
        // this is what buildSnapshot() call, returns count written
    }
};

int main()
{
Stack<int> s;
    cout << s.isEmpty() << endl;      

    s.push(10);
    s.push(20);
    s.push(30);
    cout << s.depth() << endl;        
    cout << s.peek() << endl;         

    int arr[5];
    int n = s.snapshot_into(arr, 5);
    cout << n << ": ";                
    for (int i = 0; i < n; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;

    cout << s.pop() << endl;          
    cout << s.pop() << endl;          
    cout << s.depth() << endl;        

    s.pop();
    cout << s.isEmpty() << endl;      

    for (int i = 0; i < 70; i++)
    {
        s.push(i);
    }
    cout << s.depth() << endl;        

    return 0;
}