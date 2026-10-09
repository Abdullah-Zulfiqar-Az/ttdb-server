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

void testLine(const string& line)
{
    Token tokens[MAX_TOKENS];
    int32_t num = tokenizeLine(line, tokens, MAX_TOKENS);

    cout << line << " is " << num << " tokens: ";
    for(int32_t i = 0; i < num; i++)
    {
        cout << typeToString(tokens[i].type) << ": " << tokens[i].text  << " ";
    }
    cout << endl;
}


int main()
{
    testLine("func_end");
    testLine("func fee a b");
    testLine("  add  a  b  ");

    return 0;
}