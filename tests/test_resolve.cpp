#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <cstdint>
#include <cstdio>
using namespace std;


const int32_t MAX_FUNCS = 128;
const int32_t MAX_PATCHES = MAX_FUNCS * 4;
const int32_t MAX_LINES = 1024;

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


int main()
{
    int64_t mainOffset = resolveProgram("valid.txt", "resolve.bin");

    cout << "Main is at: "<< mainOffset <<endl;

    FILE* fin = fopen("resolve.bin", "rb");
    if(fin)
    {
        string text;

        int64_t off = readResolveRecord(fin, text);
        while(off != -1)
        {
            cout << off << " " << text << endl;
            off = readResolveRecord(fin , text);
        }
        fclose(fin);
    }

    cout << resolveProgram("nomain.txt", "resolve.bin") <<endl;
    cout << resolveProgram("wrongcall.txt", "resolve.bin") << endl;
    return 0;
}


