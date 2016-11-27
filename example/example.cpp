/************************************************
 *  example.cpp
 *  ESA++
 *
 *  Copyright (c) 2014-2016, Chi-En Wu
 *  Distributed under The BSD 3-Clause License
 ************************************************/

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <esapp/segmenter.hpp>

int main(void) {
    std::vector<std::string> sequences = {
        u8"這是一隻可愛的小花貓",
        u8"一隻貓",
        u8"真可愛的貓",
        u8"這是一隻花貓",
        u8"小貓真可愛"
    };

    esapp::segmenter seg(0.1);

    for (auto const &s : sequences) {
        seg.fit(s.cbegin(), s.cend());
    }

    seg.optimize(10);
    for (auto const &s : sequences) {
        auto words = seg.segment(s.cbegin(), s.cend());
        std::copy(words.begin(), words.end(),
                  std::ostream_iterator<std::string>(std::cout, " "));
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
