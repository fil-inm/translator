#include "trie.hpp"

Trie::Trie() {
    root = new TrieNode();
}

Trie::~Trie() {
    clear(root);
}

void Trie::clear(TrieNode* node) {
    for (auto& p : node->children)
        clear(p.second);
    delete node;
}

void Trie::insert(const std::string& word) {
    TrieNode* current = root;
    for (char c : word) {
        if (!current->children[c])
            current->children[c] = new TrieNode();
        current = current->children[c];
    }
    current->isEnd = true;
}

bool Trie::search(const std::string& word) const {
    const TrieNode* current = root;
    for (char c : word) {
        auto it = current->children.find(c);
        if (it == current->children.end())
            return false;
        current = it->second;
    }
    return current->isEnd;
}