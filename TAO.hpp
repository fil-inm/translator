#include <bits/stdc++.h>
using namespace std;

struct SItem {
    enum Kind { TYPE, OP } kind;
    string value;
};

class SemStack {
private:
    stack<SItem> st;
public:
    void pushType(const string &t);
    void pushOp(const string &op);
    SItem pop();
    SItem top() const;
};

string check_bin(const string &a, const string &op, const string &b);
string check_uno(const string &op, const string &a);