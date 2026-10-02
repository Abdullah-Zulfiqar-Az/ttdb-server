#include <iostream>
#include <cstdint>
using namespace std;

struct Snapshot
{
    int id; // Just foR testing
};

struct TimelineNode
{
    Snapshot *data;
    TimelineNode *next;
    TimelineNode *prev;
};
class Timeline
{
    TimelineNode *head, *tail;
    int32_t stepCount;

public:
    // Implement these functions
    Timeline()
    {
        head = tail = nullptr;
        stepCount = 0;
    }
    void record(Snapshot *s)
    {
        TimelineNode* newNode = new TimelineNode();
        newNode->data = s;
        newNode->next = nullptr;
        newNode->prev = tail;

        if(head == nullptr)
        {
            head =tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
        stepCount++;
        // add record in the timeline
    }
    TimelineNode *begin()
    {
        return head;
    }
    int32_t getStepCount()
    {
        return stepCount;
    }
};

int main()
{
    Timeline t;
    cout << t.getStepCount() << endl;        
    cout << (t.begin() == nullptr) << endl;  

    for (int i = 1; i <= 3; i++)
    {
        Snapshot *s = new Snapshot;
        s->id = i;
        t.record(s);
    }
    cout << t.getStepCount() << endl;        

    
    TimelineNode *cur = t.begin();
    TimelineNode *last = nullptr;
    while (cur != nullptr)
    {
        cout << cur->data->id << " ";        
        last = cur;
        cur = cur->next;
    }
    cout << endl;

    
    cur = last;
    while (cur != nullptr)
    {
        cout << cur->data->id << " ";        
        cur = cur->prev;
    }
    cout << endl;

    return 0;
}