/************************************************
 *  with_segments.hpp
 *  ESA++
 *
 *  Copyright (c) 2014-2017, Chi-En Wu
 *  Distributed under The BSD 3-Clause License
 ************************************************/

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

#include "freq_trie.hpp"

#ifndef ESAPP_INTERNAL_WITH_SEGMENTS_HPP_
#define ESAPP_INTERNAL_WITH_SEGMENTS_HPP_

namespace esapp {

namespace internal {

/************************************************
 * Declaration: struct with_segments<N>
 ************************************************/

template <std::size_t N = 30>
struct with_segments {
    template <typename TextIndex, typename Trait>
    class policy;
};  // class with_segments<N>

/************************************************
 * Declaration: class with_segments<N>::policy<LCP, T>
 ************************************************/

template <std::size_t N>
template <typename LCP, typename Trait>
class with_segments<N>::policy {
 public:  // Public Type(s)
    using host_type = LCP;
    using size_type = typename Trait::size_type;
    using term_type = std::uint16_t;
    using seg_pos_vec_type = std::vector<size_type>;

 public:  // Public Method(s)
    policy();
    void optimize(double lrv_exp, size_type num_iters);

    template <typename Sequence>
    seg_pos_vec_type segment(Sequence const &s, double lrv_exp) const;

 private:  // Private Type(s)
    using event = typename Trait::event;
    using seq_type = std::vector<term_type>;

 protected:  // Protected Method(s)
    template <typename Sequence>
    void update(typename event::template after_inserting_lcp<Sequence> const &info);

 private:  // Private Method(s)
    template <typename Sequence>
    void update_counts(Sequence const &s, size_type n, size_type lcp_lf);

    // NOLINTNEXTLINE(runtime/references)
    void recover_sequence(size_type &i, seq_type &s) const;
    std::vector<size_type> segment_sequence(
        seq_type const &s,
        seg_pos_vec_type &seg_pos_vec,  // NOLINT(runtime/references)
        double lrv_exp) const;
    void increase_counts(seq_type const &s, seg_pos_vec_type const &seg_pos_vec);
    void decrease_counts(seq_type const &s, seg_pos_vec_type const &seg_pos_vec);
    void generate_seg_pos_vec(seg_pos_vec_type &seg_pos_vec,  // NOLINT(runtime/references)
                              std::vector<size_type> const &fs) const;

 private:  // Private Property(ies)
    size_type lcp_;
    freq_trie<term_type> trie_;
    std::array<size_type, N> sum_f_;
    std::array<size_type, N> sum_av_;
    std::array<size_type, N> num_str_;
    std::vector<seg_pos_vec_type> seg_pos_vecs_;
};  // class with_segments<N>::policy<LCP, T>

/************************************************
 * Implementation: class with_segments<N>::policy<LCP, T>
 ************************************************/

template <std::size_t N>
template <typename LCP, typename T>
with_segments<N>::policy<LCP, T>::policy()
    : lcp_(0), trie_(), sum_f_(), sum_av_(), num_str_(), seg_pos_vecs_() {
    // do nothing
}

template <std::size_t N>
template <typename LCP, typename T>
template <typename Sequence>
void with_segments<N>::policy<LCP, T>::update(
        typename event::template after_inserting_lcp<Sequence> const &info) {
    if (info.num_inserted == 0) {
        assert(info.lcp == 0);
        assert(info.lcp_next == 0);
        lcp_ = 0;

        seg_pos_vecs_.emplace_back();
        return;
    }

    auto lcp_lf = std::max(info.lcp, info.lcp_next);
    update_counts(info.s, info.num_inserted - 1, lcp_lf);
    lcp_ = lcp_lf;

    if (info.num_inserted == info.s.size()) {
        // handle last inserted term
        update_counts(info.s, info.num_inserted, 0);
        lcp_ = 0;
    }
}

template <std::size_t N>
template <typename LCP, typename T>
void with_segments<N>::policy<LCP, T>::optimize(double lrv_exp, size_type num_iters) {
    size_type i = 0;
    seq_type s;

    auto n = seg_pos_vecs_.size();
    for (decltype(num_iters) count = 0; count < num_iters; count++) {
        for (decltype(n) j = 0; j < n; j++) {
            recover_sequence(i, s);

            auto fs = segment_sequence(s, seg_pos_vecs_[j], lrv_exp);
            if (!seg_pos_vecs_[j].empty()) {
                increase_counts(s, seg_pos_vecs_[j]);
            }

            generate_seg_pos_vec(seg_pos_vecs_[j], fs);
            decrease_counts(s, seg_pos_vecs_[j]);
        }

        assert(i == 0);
    }
}

template <std::size_t N>
template <typename LCP, typename T>
template <typename Sequence>
typename with_segments<N>::template policy<LCP, T>::seg_pos_vec_type
with_segments<N>::policy<LCP, T>::segment(Sequence const &s, double lrv_exp) const {
    decltype(segment(s, lrv_exp)) seg_pos_vec;
    auto fs = segment_sequence(s, seg_pos_vec, lrv_exp);
    generate_seg_pos_vec(seg_pos_vec, fs);
    return seg_pos_vec;
}

template <std::size_t N>
template <typename LCP, typename T>
template <typename Sequence>
void with_segments<N>::policy<LCP, T>::update_counts(
        Sequence const &s, size_type n, size_type lcp_lf) {
    assert(lcp_ <= n);
    if (lcp_ > 0) {
        auto it = rbegin(s) + n - 1;
        auto node = trie_.get_root();
        auto max_i = std::min(lcp_, N);
        for (decltype(max_i) i = 0; i < max_i; i++) {
            auto c = *it;
            node = node->get(c, true);
            node->f++;
            sum_f_[i]++;
            if (i + 1 >= lcp_lf) {
                node->avl++;
            }

            --it;
        }

        if (lcp_ <= N) {
            node->avr++;
            sum_av_[max_i - 1]++;
        }
    }

    auto max_i = std::min(n, N);
    for (auto i = lcp_; i < max_i; i++) {
        sum_f_[i]++;
        sum_av_[i]++;
        num_str_[i]++;
    }
}

template <std::size_t N>
template <typename LCP, typename T>  // NOLINTNEXTLINE(runtime/references)
void with_segments<N>::policy<LCP, T>::recover_sequence(size_type &i, seq_type &s) const {
    using ti_ptr_type = typename host_type::host_type const *;

    s.clear();

    // recover sequence in reverse order
    i = static_cast<ti_ptr_type>(this)->lf(i);
    auto c = static_cast<ti_ptr_type>(this)->f(i);
    do {
        s.push_back(c);
        i = static_cast<ti_ptr_type>(this)->lf(i);
        c = static_cast<ti_ptr_type>(this)->f(i);
    } while (c != 0);

    std::reverse(s.begin(), s.end());
}

template <std::size_t N>
template <typename LCP, typename T>
std::vector<typename with_segments<N>::template policy<LCP, T>::size_type>
with_segments<N>::policy<LCP, T>::segment_sequence(  // NOLINTNEXTLINE(runtime/references)
        seq_type const &s, seg_pos_vec_type &seg_pos_vec, double lrv_exp) const {
    auto n = s.size();
    std::vector<size_type> fs(n);
    std::vector<double> fv(n, -std::numeric_limits<double>::infinity());

    auto seg_pos_it = seg_pos_vec.begin();
    typename decltype(seg_pos_it)::value_type seg_pos = 0;
    for (decltype(n) i = 0; i < n; ++i) {
        decltype(i) min_len = 0;
        if (!seg_pos_vec.empty()) {
            if (i == seg_pos) {
                seg_pos = *seg_pos_it;
                ++seg_pos_it;

                min_len = seg_pos - i - 1;
            } else {
                min_len = seg_pos - i;
            }

            assert(seg_pos > i);
        }

        auto s_it = s.begin() + i;
        auto node = trie_.get_root();
        auto max_m = std::min(n - i, N);
        for (decltype(i) m = 0; m < max_m; ++m) {
            if (node) {
                node = node->get(*s_it);
                ++s_it;
            }

            double f, avl, avr;
            if (node) {
                f = static_cast<double>(node->f);
                avl = static_cast<double>(node->avl);
                avr = static_cast<double>(node->avr);
            } else if (m >= min_len) {
                avl = avr = f = 1.0;
            } else { continue; }

            auto num_str = static_cast<double>(num_str_[m]);
            f *= num_str / static_cast<double>(sum_f_[m]);
            avl *= num_str / static_cast<double>(sum_av_[m]);
            avr *= num_str / static_cast<double>(sum_av_[m]);

            auto score = (m + 1) * log(f) + lrv_exp * log(avl * avr);
            if (i == 0) {
                fv[m] = score;
            } else if (fv[i - 1] + score > fv[i + m]) {
                fv[i + m] = fv[i - 1] + score;
                fs[i + m] = i;
            }
        }
    }

    return fs;
}

template <std::size_t N>
template <typename LCP, typename T>
void with_segments<N>::policy<LCP, T>::increase_counts(
        seq_type const &s, seg_pos_vec_type const &seg_pos_vec) {
    auto it = s.begin();
    typename seg_pos_vec_type::value_type prev_pos = 0;
    for (auto pos : seg_pos_vec) {
        trie_.increase(it + prev_pos, it + pos);
        prev_pos = pos;
    }
}

template <std::size_t N>
template <typename LCP, typename T>
void with_segments<N>::policy<LCP, T>::decrease_counts(
        seq_type const &s, seg_pos_vec_type const &seg_pos_vec) {
    auto it = s.begin();
    typename seg_pos_vec_type::value_type prev_pos = 0;
    for (auto pos : seg_pos_vec) {
        trie_.decrease(it + prev_pos, it + pos);
        prev_pos = pos;
    }
}

template <std::size_t N>
template <typename LCP, typename T>
void with_segments<N>::policy<LCP, T>::generate_seg_pos_vec(  // NOLINTNEXTLINE(runtime/references)
        seg_pos_vec_type &seg_pos_vec, std::vector<size_type> const &fs) const {
    auto n = fs.size();
    seg_pos_vec.clear();
    seg_pos_vec.push_back(n);
    for (auto i = fs[n - 1]; i > 0; i = fs[i - 1]) {
        seg_pos_vec.push_back(i);
    }

    std::reverse(seg_pos_vec.begin(), seg_pos_vec.end());
}

}  // namespace internal

}  // namespace esapp

#endif  // ESAPP_INTERNAL_WITH_SEGMENTS_HPP_
