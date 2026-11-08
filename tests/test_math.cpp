#include <iostream>
#include <string>
#include <cstdint>
using namespace std;

const int32_t MAX_VARS_PER_FRAME = 16;
const int32_t MAX_TOKENS = MAX_VARS_PER_FRAME + 2;

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

void initFrame(Frame&f, const string & name)
{
    f.func_name = name;
    f.argc = 0;
    f.returnLine = -1;
    f.localCount = 0;
}

//will return pos in f.locals or -1 
int32_t findLocal(Frame&f, const string& name)
{
    for(int32_t i = 0; i < f.localCount; i++){
        if(f.locals[i].name == name)
        {
            return i;
        }
    }
    return -1;
}

// return pos in f.argv or -1;
int32_t findArg(Frame& f, const string& name)
{
    for(int32_t i =0; i < f.argc; i++)
    {
        if(f.argv[i].name == name)
        {
            return i;
        }
    }
    return -1;
}

//search in locals, then args. If not found return false
bool getVariable(Frame&f, const string& name, int32_t& outVal)
{
    int32_t idx = findLocal(f, name);

    if(idx != -1)
    {
        outVal = f.locals[idx].value;
        return true;
    }

    idx = findArg(f, name);

    if(idx!= -1)
    {
        outVal = f.argv[idx].value;
        return true;
    }
    return false;
}

// change an exisitng var or create a new local. false returned on err
bool setVariable(Frame&f, const string& name, int32_t& val)
{
    int32_t idx  = findLocal(f, name);
    if(idx != -1)
    {
        f.locals[idx].value = val;
        return true;
    }

    idx = findArg(f,name);
    if(idx != -1)
    {
        f.argv[idx].value = val;
        return true;
    }

    if(f.localCount >= MAX_VARS_PER_FRAME)
    {
        cout << "Err: too many variables are in function " << f.func_name <<endl;
        return false;
    }

    f.locals[f.localCount].name = name;
    f.locals[f.localCount].value = val;
    f.localCount++;
    return true;

}

bool isANumber(const string& text)
{
    if(text.empty())
    {
        return false;
    }

    int32_t start = 0;
    if(text[0] == '-')
    {
        if(text.length() == 1)
        {
            return false;
        }
        start = 1;
    }

    for(int32_t i = start; i < text.length(); i++)
    {
        if(text[i] < '0' || text[i] > '9')
        {
            return false;
        }
    }

    return true;
}

bool getOperandValue(Frame& f, const string& text, int32_t& outVal)
{
    if(isANumber(text))
    {
        outVal = stoi(text);
        return true;
    }

    if(getVariable(f, text, outVal))
    {
        return true;
    }

    cout << "Error: variable "<< text << " is not defined"<<endl;
    return false;
}

bool executeMath(Frame& f, Token tokens[], int32_t tokenCount)
{
    string keyword = tokens[0].text;

    if(tokenCount < 0)
    {
        cout << "Error: " << keyword << " needs a variable and a value"<<endl;
        return false;
    }

    string target = tokens[1].text;

    int32_t operand = 0;

    if(!getOperandValue(f, tokens[2].text, operand))
    {
        return false;
    }

    if(keyword == "set")
    {
        return setVariable(f, target, operand);
    }

    int32_t current = 0;

    if(!getVariable(f, target, current))
    {
        cout << "Error: variable " << target << " is not defined"<<endl;
        return false;
    }

    int32_t result = 0;

    if(keyword == "add")
    {
        result = current + operand;
    }
    else if(keyword == "sub")
    {
        result = current - operand;
    }
    else if(keyword == "mul")
    {
        result = current * operand;
    }
    else if(keyword == "div")
    {
        if(operand == 0)
        {
            cout << "Error: Division by zero"<<endl;
            return false;
        }
        result = current / operand;
    }
    else{
        cout << "Error: unknown instruction '" << keyword <<"'"<<endl;
        return false;
    }

    return setVariable(f, target, result);
}

bool run(Frame &f, const string &line)
{
    Token tokens[MAX_TOKENS];
    int32_t n = tokenizeLine(line, tokens, MAX_TOKENS);
    return executeMath(f, tokens, n);
}

void show(Frame &f, const string &name)
{
    int32_t value = 0;
    if (getVariable(f, name, value))
    {
        cout << name << "=" << value << endl;
    }
    else
    {
        cout << name << " is not defined" << endl;
    }
}

int main()
{
    Frame f;
    initFrame(f, "foo");

    f.argc = 1;
    f.argv[0].name = "b";
    f.argv[0].value = 10;

    run(f, "set a 10");
    show(f, "a");
    run(f, "add b a");
    show(f, "b");
    run(f, "sub b 5");
    show(f, "b");
    run(f, "mul a 3");
    show(f, "a");
    run(f, "div a 4");
    show(f, "a");
    run(f, "set c b");
    show(f, "c");

    cout << run(f, "div a 0") << endl;
    show(f, "a");
    cout << run(f, "add zzz 1") << endl;
    cout << run(f, "add a") << endl;
    cout << run(f, "set a 1") << endl;

    cout << f.localCount << " " << f.argc << endl;
    return 0;
}