// ======================= TIME-TRAVEL DEBUGGER - SERVER TEMPLATE =======================

// Pipeline this file implements, top to bottom:
//   0. Receive  -- stream the client's .trace bytes straight to source.bin on disk
//   1. Pass 0X0   -- validity check (FUNC/FUNC_END matching)
//   2. Pass 0X1   -- resolve(): copy EVERY source line into resolve.bin as [offset][size][string], then patch CALL targets.
//   3. Pass 0X2   -- execute resolve.bin: tokenize ONE line at a time, update the call stack, take a snapshot -> Timeline
//   4. Pass 0X3   -- serialize Timeline -> session.tdbg(header + snapshot records + dense index)


#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstdio>
using namespace std;

// ---- Constants ----
const int32_t MAX_VARS_PER_FRAME = 16;
const int32_t MAX_LINES = 1024;
const int32_t MAX_STACK_DEPTH = 64;
const int32_t MAX_FUNCS = 128;
const int32_t MAX_TOKENS = MAX_VARS_PER_FRAME + 2; // kW + func_name + upto 16 params/args
const int32_t MAX_PATCHES = MAX_FUNCS * 4;
const uint64_t MAX_SOURCE_BYTES = 15ULL * 1024 * 1024; // sanity cap on the declared file length
const int32_t IO_BUFFER_SIZE = 64 * 1024;                  // fixed buffer for streaming to/from disk
const int32_t SOCKET_TIMEOUT_SEC = 5;                      // TODO: apply as SO_RCVTIMEO so a deadclient can't hang the server forever

// ---- Custom data structures

// Stack: back the live Call Stack during execution
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

// Timeline : doubly linked list of Snapshots
struct Snapshot; // fwd declaration;
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


// Core structs
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
struct TTDBHeader
{
    char magic[4]; // "TTDB"
    int32_t version;
    int32_t stepCount;
    int64_t indexOffset;
};
void writeHeader(FILE *f, const TTDBHeader &h)
{
    fwrite(h.magic, 1, 4, f);
    fwrite(&h.version, sizeof(int32_t), 1, f);

    // placeholder for other two data members
}

// resolve.bin - bookkeeping
struct FuncEntry
{
    string funcName;
    int64_t byteOffsetInResolveBin; // where this function's FUNC header record sits
};
struct PendingPatch
{
    int64_t byteOffsetOfOffsetField; // where in resolve.bin to seek back and overwrite
    string targetFuncName;
};



// PASS 0x0: READING source.bin + VALIDITY CHECK
bool readSourceLine(ifstream &in, string &out)
{
    while(getline(in, out))
    {
        if(!out.empty() && out.back() == '\r')
        {
            out.pop_back();
        }

        bool blank = true;
        for(int32_t i =0; i < out.length(); i++)
        {
            if(out[i] != ' ' && out[i] != '\t')
            {
                blank = false;
                break;
            }
        }

        if(blank == false)
        {
            return true;
        }
    }
    return false;
    // reads the next nonblank line
}
string firstWord(const string &line)
{
    string word = "";
    int32_t idx = 0;

    //skiping leading space or tab
    while(idx < line.length() && (line[idx] == ' ' || line[idx] == '\t'))
    {
        idx++;
    }

    //Add up all letters before a space or tab is hit
    while(idx < line.length() && line[idx] != ' ' && line[idx] != '\t')
    {
        word = word + line[idx];
        idx++;
    }
    return word;
    // returns first word from the input string
}
string secondWord(const string &line)
{
    string word = "";
    int32_t idx = 0;

    //skipping leading space or tab
    while(idx < line.length() && (line[idx] == ' ' || line[idx] == '\t'))
    {
        idx++;
    }

    //skipping first word
    while(idx < line.length() && line[idx] != ' ' && line[idx] != '\t')
    {
        idx++;
    }

    //skipping space/tab between first and 2nd word
    while(idx < line.length() && (line[idx] == ' ' || line[idx] == '\t'))
    {
        idx++;
    }

    //Collecting the 2nd word
    while(idx < line.length() && line[idx] != ' ' && line[idx] != '\t')
    {
        word = word + line[idx];
        idx++;
    }

    return word;

    // returns the second word
}
bool validateProgram(const char *sourcePath)
{
    ifstream fin(sourcePath);

    if(!fin)
    {
        cout << "Error: Could not open the file"<<endl;
        return false;
    }

    string line;
    bool insideFunction = false;

    while(readSourceLine(fin, line))
    {
        string keyword = firstWord(line);

        if(keyword == "func")
        {
            if(insideFunction)
            {
                cout << "Error: Nested function not allowed"<<endl;
                fin.close();
                return false;
            }
            insideFunction = true;
        }
        else if(keyword == "func_end")
        {
            if(!insideFunction)
            {
                cout << "Error: found 'func_end' without matching 'func' "<<endl;
                fin.close();
                return false;
            }
            insideFunction = false;
        }
    }

    fin.close();

    if(insideFunction)
    {
        cout << "Error: Unclosed function at the end of file"<<endl;
        return false;
    }
    return true;
    // for each func defined there should be exactly one func_end and no nested funcs allowed - 
}

// PASS 0x1: RESOLVE() -> resolve.bin

int64_t writeResolveRecord(FILE *f, int64_t offsetField, const string &text)
{
    int64_t recordStart = ftell(f);
    fwrite(&offsetField, sizeof(int64_t), 1, f);

    int32_t strSize = text.length();
    fwrite(&strSize, sizeof(int32_t), 1, f);

    fwrite(text.c_str(), sizeof(char), strSize, f);
    return recordStart;

    // writes one [offset(8B)][size(4B)][string] record at the current file position
    // returns this record's own starting byte position
}
int64_t readResolveRecord(FILE *f, string &outText)
{
    int32_t strSize = 0;
    int64_t offsetField = -1;

    if(fread(&offsetField, sizeof(int64_t), 1 ,f) != 1)
    {
        return -1;
    }

    if(fread(&strSize, sizeof(int32_t), 1 ,f) != 1)
    {
        return -1;
    }

    char * newArr = new char[strSize + 1];
    fread(newArr, sizeof(char), strSize, f);
    newArr[strSize] = '\0';

    outText = string(newArr);
    delete[] newArr;

    return offsetField;

    // reads one record at the current position and advances past it, returns the offset field - the raw line text comes back untouched in outText.
}
int64_t resolveProgram(const char *sourcePath, const char *resolveBinPath)
{
    ifstream fin(sourcePath);

    if(!fin)
    {
        cout << "Error: Could not open source path file"<< endl;
        return -1;
    }

    string lines[MAX_LINES];
    int64_t offsets[MAX_LINES];
    int32_t lineCount = 0;

    FuncEntry funcTable[MAX_FUNCS];
    int32_t funcCt = 0;

    int32_t patchLineIndex[MAX_PATCHES];
    string patchFuncName[MAX_PATCHES];
    int32_t patchCt = 0;

    int64_t mainOffset = -1;
    int64_t offsetTotal = 0;
    string line;


    while(readSourceLine(fin, line))
    {
        string keyword = firstWord(line);
        string name = secondWord(line);

        lines[lineCount] = line;
        offsets[lineCount] = offsetTotal;

        if(keyword == "func")
        {
            funcTable[funcCt].funcName = name;
            funcTable[funcCt].byteOffsetInResolveBin = offsetTotal;
            funcCt++;

            if(name== "main")
            {
                mainOffset = offsetTotal;
            }
        }
        else if (keyword == "call")
        {
            patchLineIndex[patchCt]  = lineCount;
            patchFuncName[patchCt] = name;
            patchCt++;

            offsets[lineCount] = -1; //place holder 
        }

        offsetTotal = offsetTotal + 8 + 4 + line.size();

        lineCount++;

    }

    fin.close();

    //resolving every call
    for(int i =0; i < patchCt; i++)
    {
        int64_t realOffset = -1;

        for (int j =0; j < funcCt; j++)
        {
            if(funcTable[j].funcName == patchFuncName[i])
            {
                realOffset = funcTable[j].byteOffsetInResolveBin;
                break;
            }
        }

        if(realOffset == -1)
        {
            cout << "Error: Function "<< patchFuncName[i] << " is not defined."<<endl;
            return -1;
        }

        offsets[patchLineIndex[i]] = realOffset;

    }

    if(mainOffset == -1)
    {
        cout << "Error: main function not found"<<endl;
        return -1;
    }

    FILE* fout = fopen(resolveBinPath, "wb");

    if(!fout)
    {
        cout << "Error: could not make binary file"<<endl;
        return -1;
    }

    for(int i = 0; i < lineCount; i++)
    {
        writeResolveRecord(fout, offsets[i], lines[i]);
    }
    fclose(fout);
    return mainOffset;

    
    // Every source line becomes one record holding the raw line, as-is.
    // resolve() only PEEKS at the leading word(s) -- enough to spot FUNC
    // (remember its position) and CALL (remember which function it needs
    // and where its offset field sits).
    // Once the whole file is written, every CALL's offset field is patched
    // with its target's position. Patching happens after the full write
    // Returns the byte offset of main's FUNC header record.
    // if there is no main return the error 
}

// PASS 0x2: EXECUTION (tokenization happens here)
enum TokenType
{
    KEYWORD,
    IDENTIFIER,
    PARAM
};
struct Token
{
    TokenType type;
    string text;
};
int32_t tokenizeLine(const string &line, Token tokens[], int32_t maxTokens)
{
    int32_t tokenCount = 0;
    int32_t pos = 0;

    while(pos < line.length() && tokenCount <  maxTokens) //scanning whole line
    {
        while( pos < line.length() && (line[pos] == ' ' || line[pos] == '\t')) //skiping any spaces/tabs
        {
            pos++;
        }

        if(pos >= line.length())
        {
            break;
        }

        string word = "";
        while(pos < line.length() && line[pos] != ' ' && line[pos] != '\t')
        {
            word += line[pos];
            pos++;
        }

        if(tokenCount == 0)
        {
            tokens[tokenCount].type = KEYWORD;
        }
        else if (tokenCount == 1)
        {
            tokens[tokenCount].type = IDENTIFIER;
        }
        else{
            tokens[tokenCount].type = PARAM;
        }

        tokens[tokenCount].text = word; //  save the word
        tokenCount++;
    }
    return tokenCount;
}

string typeToString(TokenType t)
{
    if( t == KEYWORD)
    {
        return "KEYWORD";
    }
    if ( t== IDENTIFIER)
    {
        return "IDENTIFIER";
    }
    return "PARAM";
}

Snapshot* buildSnapshot(Stack<Frame> &callStack)
{
    Snapshot * snap = new Snapshot();
    snap->stackDepth = callStack.snapshot_into(snap->callStack, MAX_STACK_DEPTH);
    return snap;
}
void executeProgram(const char *resolveBinPath, int64_t mainOffset, Timeline &timeline)
{
    // initialize the call stack
    // make the main frame
    // push main frame on the call stack

    // implementation:
    // execute line by line, and according to the keyword perform action
}

// PASS 0x3: SERIALIZE TIMELINE
void writeTdbg(Timeline &timeline, const char *tdbgPath)
{
    // placeholder for header
    // index array of the size of stepcount from the timeline
    // placing each snapshot in the file while maintaining the index(starting point of each nth snapshot)
    // after timeline add the index array i the file
    // update the header
}
// main section
int32_t main()
{

    if (!validateProgram("source.bin"))
    {
        // send an error response instead of a .tdbg file
        return 1;
    }

    int64_t mainOffset = resolveProgram("source.bin", "resolve.bin");

    Timeline timeline;
    executeProgram("resolve.bin", mainOffset, timeline);

    writeTdbg(timeline, "session.tdbg");

    return 0;
}