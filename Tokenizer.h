#ifndef TOKENIZER_H
#define TOKENIZER_H
#define TOKEN_ERR "End of data"
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

class Tokenizer
{
  public:
    int getLineCount();
    int getTokenOffset();
    string getToken();
    bool peekToken();
    Tokenizer(char *fileName);
    void close();

  private:
    int lineCount;
    int tokenOffset;
    string currentLine;
    string currentToken;
    ifstream input;
    istringstream iss;
};

#endif
