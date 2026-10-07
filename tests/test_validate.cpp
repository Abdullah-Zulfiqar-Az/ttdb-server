#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
using namespace std;

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


int main()
{
    cout << "[" <<  firstWord("  set a 10") << "]" << endl; 
    cout << "[" << secondWord("  set a 10") << "]" << endl; 
    cout << "[" << secondWord("func_end") << "]" << endl;
    cout << "[" << firstWord("  ") << "]" << endl;  

    cout << validateProgram("valid.txt")<<endl;
    cout << validateProgram("invalid1.txt")<<endl;
    cout << validateProgram("invalid2.txt")<<endl;
    cout << validateProgram("invalid3.txt")<<endl;

    return 0;
}