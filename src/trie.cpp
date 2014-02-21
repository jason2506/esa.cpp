/************************************************
 *  trie.cpp
 *  ESA++
 *
 *  Copyright (c) 2014, Chi-En Wu
 *  Distributed under The BSD 3-Clause License
 ************************************************/

#include "trie.hpp"

namespace esapp
{

/************************************************
 * Implementation: class FreqTrie
 ************************************************/

FreqTrie::FreqTrie(size_t max_depth, double smooth, Char boundary)
    : freq_avg_(NULL), max_depth_(max_depth), smooth_(smooth), boundary_(boundary)
{
    root_ = new FreqTrieNode();
}

FreqTrie::FreqTrie(FreqTrie const &trie)
    : freq_avg_(NULL), max_depth_(trie.max_depth_),
      smooth_(trie.smooth_), boundary_(trie.boundary_)
{
    root_ = new FreqTrieNode(*(trie.root_));
    if (trie.freq_avg_)
    {
        size_t trie_depth = trie.depth();
        freq_avg_ = new double[trie_depth];
        memcpy(freq_avg_, trie.freq_avg_, sizeof(double) * trie_depth);
    }
}

FreqTrie::~FreqTrie(void)
{
    clear_fm();
    delete root_;
}

FreqTrie &FreqTrie::operator=(FreqTrie const &trie)
{
    root_ = trie.root_;

    clear_fm();
    if (trie.freq_avg_)
    {
        size_t trie_depth = trie.depth();
        freq_avg_ = new double[trie_depth];
        memcpy(freq_avg_, trie.freq_avg_, sizeof(double) * trie_depth);
    }

    return *this;
}

void FreqTrie::increase(Sequence const &sequence, bool include_self)
{
    size_t n = sequence.size();
    for (size_t i = 0; i < n; ++i)
    {
        FreqTrieNode *node = root_;
        for (size_t j = i; j < n && j - i < max_depth_; ++j)
        {
            if (!include_self && i == 0 && j == n - 1) { continue; }

            node = node->get(sequence[j], true);
            node->f++;

            Char x = (i > 0)     ? sequence[i - 1] : boundary_;
            Char y = (j < n - 1) ? sequence[j + 1] : boundary_;
            node->sp1l[x]++;
            node->sp1r[y]++;
        }
    }
}

void FreqTrie::increase(std::vector<Sequence> const &sequences, bool include_self)
{
    for (std::vector<Sequence>::const_iterator it = sequences.begin();
         it != sequences.end(); ++it)
    {
        increase(*it, include_self);
    }
}

void FreqTrie::decrease(Sequence const &sequence, bool include_self)
{
    size_t n = sequence.size();
    for (size_t i = 0; i < n; ++i)
    {
        FreqTrieNode *node = root_;
        for (size_t j = i; j < n && j - i < max_depth_; ++j)
        {
            if (!include_self && i == 0 && j == n - 1) { continue; }

            node = node->children[sequence[j]];
            node->f--;
        }
    }
}

void FreqTrie::decrease(std::vector<Sequence> const &sequences, bool include_self)
{
    for (std::vector<Sequence>::const_iterator it = sequences.begin();
         it != sequences.end(); ++it)
    {
        decrease(*it, include_self);
    }
}

void FreqTrie::update_hsp1(void)
{
    size_t trie_depth = depth();
    size_t num_alphabets = root_->children.size() + 1;

    size_t *nums = new size_t[trie_depth];
    double *hl_avg = new double[trie_depth];
    double *hr_avg = new double[trie_depth];

    memset(nums, 0, sizeof(size_t) * trie_depth);
    memset(hl_avg, 0, sizeof(double) * trie_depth);
    memset(hr_avg, 0, sizeof(double) * trie_depth);

    for (Iterator it = begin(); it != end(); ++it)
    {
        size_t node_depth = it->first - 1;
        FreqTrieNode *node = it->second;

        node->hl = entropy(node->sp1l, num_alphabets);
        node->hr = entropy(node->sp1r, num_alphabets);
        hl_avg[node_depth] += node->hl;
        hr_avg[node_depth] += node->hr;
        nums[node_depth]++;
    }

    for (size_t i = 0; i < trie_depth; ++i)
    {
        hl_avg[i] /= nums[i];
        hr_avg[i] /= nums[i];
    }

    for (Iterator it = begin(); it != end(); ++it)
    {
        size_t node_depth = it->first - 1;
        FreqTrieNode *node = it->second;

        node->hl /= hl_avg[node_depth];
        node->hr /= hr_avg[node_depth];
    }

    delete [] nums;
    delete [] hl_avg;
    delete [] hr_avg;
}

void FreqTrie::update_fm(void)
{
    clear_fm();

    size_t trie_depth = depth();
    size_t *nums = new size_t[trie_depth];
    freq_avg_ = new double[trie_depth];

    memset(nums, 0, sizeof(size_t) * trie_depth);
    memset(freq_avg_, 0, sizeof(double) * trie_depth);

    for (Iterator it = begin(); it != end(); ++it)
    {
        size_t node_depth = it->first - 1;
        FreqTrieNode *node = it->second;

        freq_avg_[node_depth] += node->f;
        nums[node_depth]++;
    }

    for (size_t i = 0; i < trie_depth; ++i)
    {
        freq_avg_[i] /= nums[i];
    }

    delete [] nums;
}

void FreqTrie::update_iv(void)
{
    if (freq_avg_ == NULL) { throw std::exception(); }

    for (Iterator it = begin(); it != end(); ++it)
    {
        size_t node_depth = it->first - 1;
        FreqTrieNode *node = it->second;

        node->iv = pow(node->f / freq_avg_[node_depth], node_depth + 1);
    }
}

double FreqTrie::get_hl(Sequence const &sequence) const
{
    return get_hl(sequence.begin(), sequence.end());
}

double FreqTrie::get_hl(Sequence::const_iterator const &begin,
                        Sequence::const_iterator const &end) const
{
    FreqTrieNode const *node = find(begin, end);
    if (node == NULL) { return -1; }

    return node->hl;
}

double FreqTrie::get_hr(Sequence const &sequence) const
{
    return get_hr(sequence.begin(), sequence.end());
}

double FreqTrie::get_hr(Sequence::const_iterator const &begin,
                        Sequence::const_iterator const &end) const
{
    FreqTrieNode const *node = find(begin, end);
    if (node == NULL) { return -1; }

    return node->hr;
}

double FreqTrie::get_iv(Sequence const &sequence) const
{
    return get_iv(sequence.begin(), sequence.end());
}

double FreqTrie::get_iv(Sequence::const_iterator const &begin,
                        Sequence::const_iterator const &end) const
{
    FreqTrieNode const *node = find(begin, end);
    if (node == NULL) { return -1; }

    return node->iv;
}

size_t FreqTrie::depth(void) const
{
    return root_->depth();
}

size_t FreqTrie::max_depth(void) const
{
    return max_depth_;
}

FreqTrie::Iterator FreqTrie::begin(void)
{
    return ++Iterator(root_);
}

FreqTrie::ConstIterator FreqTrie::begin(void) const
{
    return ++ConstIterator(root_);
}

FreqTrie::Iterator FreqTrie::end(void)
{
    return Iterator();
}

FreqTrie::ConstIterator FreqTrie::end(void) const
{
    return ConstIterator();
}

FreqTrie::FreqTrieNode const *FreqTrie::find(Sequence const &sequence) const
{
    return find(sequence.begin(), sequence.end());
}

FreqTrie::FreqTrieNode const *FreqTrie::find(
    Sequence::const_iterator const &begin,
    Sequence::const_iterator const &end) const
{
    NodeCollection::const_iterator result_it;
    FreqTrieNode *node = root_;
    for (Sequence::const_iterator it = begin; it != end; ++it)
    {
        node = node->get(*it);
        if (node == NULL) { break; }
    }

    return node;
}

double FreqTrie::entropy(CharCounts counts, size_t num_events)
{
    double n = num_events * smooth_;
    for (CharCounts::const_iterator it = counts.begin();
         it != counts.end(); ++it)
    {
        n += it->second;
    }

    double h = 0;
    for (CharCounts::const_iterator it = counts.begin();
         it != counts.end(); ++it)
    {
        double p = (it->second + smooth_) / n;
        h -= p * log(p);
    }

    if (smooth_ > 0)
    {
        size_t num_zeros = num_events - counts.size();
        double p = smooth_ / n;
        h -= num_zeros * (p * log(p));
    }

    return h;
}

void FreqTrie::clear_fm(void)
{
    if (freq_avg_ != NULL)
    {
        delete [] freq_avg_;
    }

    freq_avg_ = NULL;
}

void FreqTrie::clear(void)
{
    root_->clear();
    clear_fm();
}

/************************************************
 * Implementation: class FreqTrie::BaseIterator
 ************************************************/

template<typename T>
FreqTrie::BaseIterator<T>::BaseIterator(void)
    : stack_()
{
    // do nothing
}

template<typename T>
FreqTrie::BaseIterator<T>::BaseIterator(T *node)
    : stack_()
{
    stack_.push(std::make_pair(0, node));
}

template<typename T>
FreqTrie::BaseIterator<T> &FreqTrie::BaseIterator<T>::operator++(void)
{
    Value top = stack_.top();
    stack_.pop();

    size_t depth = top.first + 1;
    T *node = top.second;
    for (NodeCollection::const_iterator it = node->children.begin();
         it != node->children.end(); ++it)
    {
        stack_.push(std::make_pair(depth, it->second));
    }

    return *this;
}

template<typename T>
FreqTrie::BaseIterator<T> FreqTrie::BaseIterator<T>::operator++(int)
{
    BaseIterator it(this);
    ++(*this);
    return it;
}

template<typename T>
typename FreqTrie::BaseIterator<T>::Value const &
FreqTrie::BaseIterator<T>::operator*(void) const
{
    return stack_.top();
}

template<typename T>
typename FreqTrie::BaseIterator<T>::Value const *
FreqTrie::BaseIterator<T>::operator->(void) const
{
    return &(stack_.top());
}

template<typename T>
bool FreqTrie::BaseIterator<T>::operator==(BaseIterator const &it) const
{
    if (it.stack_.empty())      { return stack_.empty(); }
    else if (stack_.empty())    { return false; }

    return stack_.top() == it.stack_.top();
}

template<typename T>
bool FreqTrie::BaseIterator<T>::operator!=(BaseIterator const &it) const
{
    return !(*this == it);
}

/************************************************
 * Implementation: struct FreqTrie::FreqTrieNode
 ************************************************/

FreqTrie::FreqTrieNode::FreqTrieNode(void) : f(0), hl(0), hr(0), iv(0)
{
    // do nothing
}

FreqTrie::FreqTrieNode::FreqTrieNode(FreqTrie::FreqTrieNode const &node)
    : sp1l(node.sp1l), sp1r(node.sp1r), f(node.f), hl(node.hl), hr(node.hr), iv(node.iv)
{
    for (NodeCollection::const_iterator it = node.children.begin();
         it != node.children.end(); ++it)
    {
        FreqTrieNode *node = new FreqTrieNode(*(it->second));
        children[it->first] = node;
    }
}

FreqTrie::FreqTrieNode::~FreqTrieNode(void)
{
    clear();
}

FreqTrie::FreqTrieNode &FreqTrie::FreqTrieNode::operator=(FreqTrie::FreqTrieNode const &node)
{
    if (&node != this) { return *this; }

    clear();

    sp1l = node.sp1l;
    sp1r = node.sp1r;
    f = node.f;
    hl = node.hl;
    hr = node.hr;
    iv = node.iv;

    for (NodeCollection::const_iterator it = node.children.begin();
         it != node.children.end(); ++it)
    {
        FreqTrieNode *node = new FreqTrieNode(*(it->second));
        children[it->first] = node;
    }

    return *this;
}

FreqTrie::FreqTrieNode const *FreqTrie::FreqTrieNode::get(Char key) const
{
    NodeCollection::const_iterator it = children.find(key);
    if (it != children.end())
    {
        return it->second;
    }

    return NULL;
}

FreqTrie::FreqTrieNode *FreqTrie::FreqTrieNode::get(Char key, bool create)
{
    NodeCollection::iterator it = children.find(key);
    if (it != children.end())   { return it->second; }
    else if (!create)           { return NULL; }

    children[key] = new FreqTrieNode();
    return children[key];
}

size_t FreqTrie::FreqTrieNode::depth(void) const
{
    if (children.empty()) { return 0; }

    size_t max_depth = 0;
    for (NodeCollection::const_iterator it = children.begin();
         it != children.end(); ++it)
    {
        size_t depth = it->second->depth();
        if (depth > max_depth)
        {
            max_depth = depth;
        }
    }

    return max_depth + 1;
}

void FreqTrie::FreqTrieNode::clear(void)
{
    for (NodeCollection::const_iterator it = children.begin();
         it != children.end(); ++it)
    {
        delete it->second;
    }

    children.clear();
    f = hl = hr = iv = 0;
}

} // namespace esa
