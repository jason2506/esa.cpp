/************************************************
 *  seg.hpp
 *  ESA++
 *
 *  Copyright (c) 2014, Chi-En Wu
 *  Distributed under The BSD 3-Clause License
 ************************************************/

#ifndef SEG_HPP_INCLUDEED
#define SEG_HPP_INCLUDEED

#include <cmath>
#include <string>
#include <vector>

#include "trie.hpp"

namespace esapp
{

/************************************************
 * Declaration: class Segmenter
 ************************************************/

class Segmenter
{
public: // Public Type(s)
    typedef FreqTrie::Sequence Sequence;

public: // Public Method(s)
    Segmenter(double lrv_exp, size_t max_iters = 10,
              size_t max_length = 30, double smooth = 0.0);

    std::vector<std::vector<Sequence> > fit_and_segment(std::vector<Sequence> const &sequences);
    void fit(std::vector<Sequence> const &sequences);
    std::vector<std::vector<Sequence> > segment(std::vector<Sequence> const &sequences) const;
    std::vector<Sequence> segment(Sequence const &sequence) const;

private: // Private Type(s)
    typedef std::vector<size_t> Seg;

private: // Private Method(s)
    Seg optimize_segment(Sequence const &sequence) const;
    void generate_segment(Seg &seg, size_t **fs, size_t i, size_t j) const;
    std::vector<Sequence> segment_sequence(Sequence const &sequence,
                                           Seg const &seg) const;

private: // Private Property(ies)
    FreqTrie trie_;
    double lrv_exp_;
    size_t max_iters_;
}; // class Segmenter

} // namespace esa

#endif // SEG_HPP_INCLUDEED
