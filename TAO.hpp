#include <bits/stdc++.h>
#include "tokens.hpp"
using namespace std;

struct SItem {
    Token::Type kind;
    string value;
};

class SemStack {
private:
    stack<SItem> st;
public:
    void push(Token::Type &t, const string &val);
    SItem pop();
    Token::Type check_bin();
    Token::Type check_uno();
};