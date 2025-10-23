#pragma once
#include <unordered_map>
#include <string>

class TrieNode {
public:
    bool isEnd = false;
    std::unordered_map<char, TrieNode*> children;
};

class Trie {
private:
    TrieNode* root;
public:
    Trie();
    ~Trie();
    void insert(const std::string& word);
    bool search(const std::string& word) const;
    void clear(TrieNode* node);
};