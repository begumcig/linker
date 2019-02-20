#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <regex>
#include <vector>
#include <iomanip>
#include "Tokenizer.h"

using namespace std;

const int MEMORY_SIZE = 512;
const int MAX_COUNT = 16;
const int SYMBOL_LENGTH = 16;

bool eom = false;
bool PASS_2 = false;
int moduleNum = 0;
int instrNum = 0;
int instrCnt = 0;

map<string, int> defPairs;
map<string, bool> multiDef;
map<int, int> base;
map<string, bool> curUseListUsed;
map<string, int> globalUseListUsed;
map<string, int> globalUseList;
vector<string> curUseList;
vector<string> useList;

regex symbolname("([A-Z]|[a-z])([A-Z]|[a-z]|[0-9])*", regex_constants::extended);

//-- Error codes for parsing phase (pass one) --//
void __parseerror(int errcode, Tokenizer &t)
{
    static string errstr[] = {
        "NUM_EXPECTED",           // Number expect
        "SYM_EXPECTED",           // Symbol Expected
        "ADDR_EXPECTED",          // Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",           // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
    };
    cout << "Parse Error line " << t.getLineCount() << " offset " << t.getTokenOffset() << ": " << errstr[errcode] << endl;
    exit(1);
}

//-- Checking defcount, usecount and codecount for errors. --//
//-- Expecting 4,5,6 as category since they are the related error codes. --//
bool intErrors(int count, int category)
{
    if (category == 6)
    {
        instrNum += count;
        if (instrNum > MEMORY_SIZE)
            return false;
    }
    else if ((category == 4 || category == 5) && count > MAX_COUNT)
        return false;
    return true;
}

//-- Reading any integer related input. --//
int readInt(Tokenizer &t, int category)
{
    if (t.peekToken())
    {
        try
        {
            int dc = stoi(t.getToken());
            if (!(intErrors(dc, category)))
                __parseerror(category, t);
            else
                return dc;
        }
        catch (const invalid_argument &ia)
        {
            __parseerror(0, t);
        }
    }
    else //TOKEN_ERR (eof)
    {
        if (!eom)
            __parseerror(0, t);
        else
            return -1;
    }
}

//-- All symbols should follow the regex rule and must be shorter than 16 chars. --//
string readSymbol(Tokenizer &t)
{
    if (t.peekToken())
    {
        string symb = t.getToken();
        if (symb.length() > SYMBOL_LENGTH)
            __parseerror(3, t);
        else if (!regex_match(symb, symbolname))
            __parseerror(1, t);
        return symb;
    }
    else
        __parseerror(1, t);
}

//-- Instructions start with A,E,I,R follwed with a 4 digit instuction. --//
//-- inst / 1000 = op , inst % 1000 = operand. --------------------------//
void readInst(Tokenizer &t)
{
    if (t.peekToken())
    {
        string addr = t.getToken();
        if (!(addr == "A" || addr == "E" || addr == "I" || addr == "R"))
        {
            __parseerror(2, t);
        }
        else
        {
            int instr = readInt(t, 0);
            if (PASS_2)
            {
                cout << setfill('0') << setw(3);
                cout << instrCnt;
                cout << ": ";
                if (addr == "A")
                {
                    if (instr >= 10000)
                    {
                        cout << 9999;
                        cout << " Error: Illegal opcode; treated as 9999";
                    }
                    else if (instr % 1000 > MEMORY_SIZE)
                    {
                        int opcode = instr / 1000;
                        cout << (opcode * 1000)
                             << " Error: Absolute address exceeds machine size; zero used ";
                    }
                    else
                        cout << setfill('0') << setw(4) << instr;
                }
                else if (addr == "R")
                {
                    if (instr >= 10000)
                    {
                        cout << 9999;
                        cout << " Error: Illegal opcode; treated as 9999";
                    }
                    else if (instr % 1000 >= base[moduleNum + 1] - base[moduleNum])
                    {
                        int opcode = instr / 1000;
                        cout << (opcode * 1000) + base[moduleNum]
                             << " Error: Relative address exceeds module size; zero used ";
                    }
                    else
                        cout << setfill('0') << setw(4) << instr + base[moduleNum];
                }
                else if (addr == "I")
                {
                    if (instr >= 10000)
                    {
                        cout << 9999 << " Error: Illegal immediate value; treated as 9999";
                    }
                    else
                        cout << setfill('0') << setw(4) << instr;
                }
                else if (addr == "E")
                {
                    if (instr >= 10000)
                    {
                        cout << 9999;
                        cout << " Error: Illegal opcode; treated as 9999";
                    }
                    else if (instr % 1000 >= curUseList.size())
                    {

                        cout << setfill('0') << setw(4) << instr;
                        cout << " Error: External address exceeds length of uselist; treated as immediate";
                    }
                    else
                    {
                        int op = instr / 1000;
                        int operand = instr % 1000;
                        curUseListUsed[curUseList[operand]] = true;
                        if (defPairs.find(curUseList[operand]) == defPairs.end())
                            cout << op * 1000 << " Error: " << curUseList[operand] << " is not defined; zero used";
                        else
                        {
                            cout << (op * 1000) + defPairs[curUseList[operand]];
                            globalUseListUsed[curUseList[operand]] = moduleNum;
                        }
                    }
                }
                cout << endl;
            }
        }
    }
    else
        __parseerror(2, t);
}

void passOne(char *fileName)
{
    Tokenizer parser(fileName);
    while (parser.peekToken())
    {
        moduleNum++;
        map<string, int> curDefPairs;      //for warning messages.
        int defCount = readInt(parser, 4); //4 because defcount.
        if (defCount == -1)
        {
            parser.close();
            return;
        }

        //-- defCount = number of symbol address pairs we should parse for. --//
        for (int i = 0; i < defCount; i++)
        {
            string sym = readSymbol(parser);
            int relAdd = readInt(parser, 0); //0 because no error check.
            if (defPairs.find(sym) == defPairs.end())
            {
                defPairs[sym] = base[moduleNum] + relAdd;
                curDefPairs[sym] = relAdd;
            }
            else
                multiDef[sym] = true; // symbol is defined multiple times.
        }

        int useCount = readInt(parser, 5);
        for (int i = 0; i < useCount; i++)
        {
            string sym = readSymbol(parser);
        }

        int codeCount = readInt(parser, 6);

        //-- Warning Message --//
        for (map<string, int>::iterator iter = curDefPairs.begin(); iter != curDefPairs.end(); iter++)
        {
            if (iter->second >= codeCount)
            {
                cout << "Warning: Module " << moduleNum << ": " << iter->first
                     << " too big " << iter->second << " (max=" << codeCount - 1
                     << ") assume zero relative" << endl;
                defPairs[iter->first] = base[moduleNum];
            }
        }
        for (int i = 0; i < codeCount; i++)
        {
            readInst(parser);
        }

        base[moduleNum + 1] = base[moduleNum] + codeCount;
        eom = true;
    }
    parser.close();
}

void printSymbolTable()
{
    cout << "Symbol Table " << endl;
    for (map<string, int>::iterator iter = defPairs.begin(); iter != defPairs.end(); iter++)
    {
        cout << iter->first << "=" << iter->second;
        if (multiDef[iter->first])
        {
            cout << " Error: This variable is multiple times defined; first value used";
        }
        cout << endl;
    }
    cout << endl;
}

void passTwo(char *fileName)
{
    Tokenizer parser(fileName);
    cout << "Memory Map" << endl;
    moduleNum = 0;
    instrCnt = 0;
    while (parser.peekToken())
    {
        moduleNum++;
        int defCount = readInt(parser, 4); //4 because defcount.
        if (defCount == -1)
        {
            for (map<string, int>::iterator iter = defPairs.begin(); iter != defPairs.end(); iter++)
            {
                if (globalUseListUsed.find(iter->first) == globalUseListUsed.end())
                {
                    cout << endl << "Warning: Module " << globalUseList[iter->first] << ": " << iter->first
                         << " was defined but never used" << endl;
                }
            }
            parser.close();
            return;
        }

        //-- defCount = number of symbol address pairs we should parse for. --//
        for (int i = 0; i < defCount; i++)
        {
            string sym = readSymbol(parser);
            int relAdd = base[moduleNum] + readInt(parser, 0); //0 because no error check.
            if (defPairs[sym] == relAdd)
                globalUseList[sym] = moduleNum;
        }

        int useCount = readInt(parser, 5);
        curUseList.clear();
        curUseListUsed.clear();
        for (int i = 0; i < useCount; i++)
        {
            string sym = readSymbol(parser);
            useList.push_back(sym);
            curUseList.push_back(sym);
        }

        for (int i = 0; i < curUseList.size(); i++)
        {
            curUseListUsed[curUseList[i]] = false;
        }

        int codeCount = readInt(parser, 6);
        for (int i = 0; i < codeCount; i++)
        {
            readInst(parser);
            instrCnt++;
        }

        for (map<string, bool>::iterator iter = curUseListUsed.begin(); iter != curUseListUsed.end(); iter++)
        {
            if (iter->second == false)
                cout << "Warning: Module " << moduleNum << ": " << iter->first
                     << " appeared in the uselist but was not actually used" << endl;
        }
    }
    for (map<string, int>::iterator iter = defPairs.begin(); iter != defPairs.end(); iter++)
    {
        if (globalUseListUsed.find(iter->first) == globalUseListUsed.end())
        {
            cout << "Warning: Module " << globalUseList[iter->first] << ": " << iter->first
                 << " was defined but never used" << endl;
        }
    }
    parser.close();
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "You did not enter a file name.";
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            base[1] = 0;
            char *fileName = argv[i];
            Tokenizer parser(fileName);
            passOne(fileName);
            printSymbolTable();
            PASS_2 = true;
            passTwo(fileName);
        }
    }
    return 0;
}
