#include <iostream>
#include "Tokenizer.h"

Tokenizer::Tokenizer(char *fileName)
{
    input.open(fileName);
    lineCount = 0;
    tokenOffset = 1;
}

int Tokenizer::getLineCount()
{
    return lineCount;
}

int Tokenizer::getTokenOffset()
{
    return tokenOffset;
}

string Tokenizer::getToken()
{
    if (input.is_open() && peekToken())
    {

        tokenOffset += currentToken.length();
        iss >> currentToken;
        //cout << "....Currently at word offset: " << tokenOffset << " ....." << endl;
        //cout << ".... The token is: " << currentToken << " ....." << endl;
        return currentToken;
    }
    else
    {
        cout << "There is an error with the input file." << endl;
        return TOKEN_ERR;
    }
}

void Tokenizer::close()
{
    input.close();
}

bool Tokenizer::peekToken()
{
    if (tokenOffset + currentToken.length() > currentLine.length())
    {
        if (lineCount != 0){
            //cout << ".....Line ending offset: " << tokenOffset + currentToken.length() << " ....." << endl;
            tokenOffset += currentToken.length();
        }
            
        if (getline(input, currentLine))
        {
            lineCount++;
            tokenOffset = 1;
            currentToken = "";
            //cout << ".........Currently processing line: " << lineCount << " ........." << endl;
            iss.clear();
            iss.str(currentLine);
        }
        else
        {
            return false;
        }
    }
    
    //---Handling white spaces at the beginning of a line.--//
    while (iss.peek() == 32 || iss.peek() == 9)
    {
        iss.get();
        tokenOffset++;
        //--The case that a line is completely white space or ends with white space.--//
        if (tokenOffset > currentLine.length())
        {
            tokenOffset -= currentToken.length();
            return peekToken();
        }
    }
    return true;
}
