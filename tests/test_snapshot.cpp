#include <iostream>
#include <string>
#include <cstdint>
using namespace std;

const int32_t MAX_VARS_PER_FRAME = 16;
const int32_t MAX_STACK_DEPTH = 64;

struct Variable
{
    string name;
    int32_t value;
};
struct Frame
{
    string func_name;
    int32_t argc;
    Variable argv[MAX_VARS_PER_FRAME];
    int32_t returnLine;
    Variable locals[MAX_VARS_PER_FRAME];
    int32_t localCount;
};
struct Snapshot
{
    Frame callStack[MAX_STACK_DEPTH];
    int32_t stackDepth;
};

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


Snapshot* buildSnapshot(Stack<Frame> &callStack)
{
    Snapshot * snap = new Snapshot();
    snap->stackDepth = callStack.snapshot_into(snap->callStack, MAX_STACK_DEPTH);
    return snap;
}

int main()
{
    Stack<Frame> st;
    //pushing main frame
    Frame f1;
    f1.func_name = "main";
    f1.locals[0] = {"k", 10};
    f1.localCount = 1;
    st.push(f1);

    Frame f2;
    f2.func_name = "foo";
    st.push(f2);


    Snapshot* snap1 = buildSnapshot(st);
    cout << snap1->stackDepth <<endl; //2
    cout << snap1->callStack[0].func_name <<endl; //foo

    cout << snap1->callStack[1].func_name << endl; //main
    cout << snap1 -> callStack[1].locals[0].name << " = " << snap1->callStack[1].locals[0].value <<endl; // k = 10

    st.peek().func_name = "changed";
    cout << snap1->callStack[0].func_name <<endl; //foo

    cout << st.peek().func_name << endl; //changed

    st.pop();
    Snapshot * snap2 = buildSnapshot(st);
    cout << snap2 -> stackDepth << endl; //1
    cout << snap1->stackDepth << endl; // 2

    delete snap1;
    delete snap2;

    return 0;
}