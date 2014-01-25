/************************************************
 *  example.cpp
 *  ESA++
 *
 *  Copyright (c) 2014, Chi-En Wu
 *  Distributed under The BSD 3-Clause License
 ************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "seg.hpp"

using namespace std;
using namespace esapp;

int main (void)
{
    wcout.imbue(locale("zh_tw.UTF-8"));

    vector<wstring> sequences;
    sequences.push_back(L"這是一隻可愛的小花貓");
    sequences.push_back(L"一隻貓");
    sequences.push_back(L"真可愛的貓");
    sequences.push_back(L"這是一隻花貓");
    sequences.push_back(L"小貓真可愛");

    Segmenter segmenter(2.0, 10, 30, 3);

    /*
    segmenter.fit(sequences);
    for (vector<wstring>::const_iterator it = sequences.begin();
         it != sequences.end(); ++it)
    {
        vector<wstring> words = segmenter.segment(*it);
        copy(words.begin(), words.end(), ostream_iterator<wstring, wchar_t>(wcout, L" "));
        wcout << endl;
    }
    //*/

    vector<vector<wstring> > words_list = segmenter.fit_and_segment(sequences);
    for (vector<vector<wstring> >::const_iterator it = words_list.begin();
         it != words_list.end(); ++it)
    {
        copy(it->begin(), it->end(), ostream_iterator<wstring, wchar_t>(wcout, L" "));
        wcout << endl;
    }

    return EXIT_SUCCESS;
}
