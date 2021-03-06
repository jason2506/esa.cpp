/************************************************
 *  example.cpp
 *  ESA++
 *
 *  Copyright (c) 2014-2017, Chi-En Wu
 *  Distributed under The BSD 3-Clause License
 ************************************************/

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// include header file which contains the segmenter class.
#include <esapp/segmenter.hpp>

int main(void) {
    std::vector<std::string> sequences = {
        u8"這是一隻可愛的小花貓",
        u8"一隻貓",
        u8"真可愛的貓",
        u8"這是一隻花貓",
        u8"小貓真可愛"
    };

    // create a segmenter with parameter lrv_exp = 0.1, which is used to
    // balance insertion/deletion errors: character sequences will be
    // segmented into more parts as the value increases, and vice versa.
    esapp::segmenter seg(0.1);
    for (auto const &s : sequences) {
        // fit iterator pair of each sequence into segmenter.
        seg.fit(s.cbegin(), s.cend());
    }

    // perform 10 iterations of optimization after fitting sequences.
    seg.optimize(10);
    for (auto const &s : sequences) {
        // pass iterator pair of sequence you want to segment into segmenter.
        // segmentation result will be inserted with third parameter, which
        // must meet the requirements of OutputIterator.
        seg.segment(s.cbegin(), s.cend(),
                    std::ostream_iterator<std::string>(std::cout, " "));
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
