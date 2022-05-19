// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Daniil Goncharov <neargye@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Changes:
// * added constexpr modifiers to all functions, except iosteam operators
// * memset in bitset::set/bitset::reset
// * memcmp in bitset::operator==
// * reinterpret_cast in bitset::count

#pragma once

#include <bit>
#include <cassert>
#include <climits>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>

namespace nstd {

template <size_t _Bits>
class bitset {  // store fixed-length sequence of Boolean elements
 public:
    using _Ty = std::conditional_t<_Bits <= sizeof(unsigned long) * CHAR_BIT, unsigned long, unsigned long long>;

    class reference {  // proxy for an element
        friend bitset<_Bits>;

     public:
        constexpr ~reference() noexcept {}  // TRANSITION, ABI

        constexpr reference& operator=(bool _Val) noexcept {
            _Pbitset->_Set_unchecked(_Mypos, _Val);
            return *this;
        }

        constexpr reference& operator=(const reference& _Bitref) noexcept {
            _Pbitset->_Set_unchecked(_Mypos, static_cast<bool>(_Bitref));
            return *this;
        }

        constexpr reference& flip() noexcept {
            _Pbitset->_Flip_unchecked(_Mypos);
            return *this;
        }

        constexpr bool operator~() const noexcept {
            return !_Pbitset->_Subscript(_Mypos);
        }

        constexpr operator bool() const noexcept {
            return _Pbitset->_Subscript(_Mypos);
        }

     private:
        constexpr reference() noexcept : _Pbitset(nullptr), _Mypos(0) {}

        constexpr reference(bitset<_Bits>& _Bitset, size_t _Pos) : _Pbitset(&_Bitset), _Mypos(_Pos) {}

        bitset<_Bits>* _Pbitset;
        size_t _Mypos;  // position of element in bitset
    };

    static constexpr void _Validate(size_t __attribute__((unused)) _Pos) {  // verify that _Pos is within bounds
        assert(_Pos < _Bits && "bitset index outside range");
    }

    constexpr bool _Subscript(size_t _Pos) const {
        return (_Array[_Pos / _Bitsperword] & (_Ty{1} << _Pos % _Bitsperword)) != 0;
    }

    constexpr bool operator[](size_t _Pos) const {
        _Validate(_Pos);
        return _Subscript(_Pos);
    }

    constexpr reference operator[](size_t _Pos) {
        _Validate(_Pos);
        return reference(*this, _Pos);
    }

    constexpr bitset() noexcept : _Array() {}  // construct with all false values

    static constexpr bool _Need_mask = _Bits < CHAR_BIT * sizeof(unsigned long long);

    static constexpr unsigned long long _Mask = (1ULL << (_Need_mask ? _Bits : 0)) - 1ULL;

    constexpr bitset(unsigned long long _Val) noexcept : _Array{static_cast<_Ty>(_Need_mask ? _Val & _Mask : _Val)} {}

    template <class _Traits, class _Elem>
    constexpr void _Construct(const _Elem* const _Ptr, size_t _Count, const _Elem _Elem0, const _Elem _Elem1) {
        if (_Count > _Bits) {
            for (size_t _Idx = _Bits; _Idx < _Count; ++_Idx) {
                const auto _Ch = _Ptr[_Idx];
                if (!_Traits::eq(_Elem1, _Ch) && !_Traits::eq(_Elem0, _Ch)) {
                    _Xinv();
                }
            }

            _Count = _Bits;
        }

        size_t _Wpos = 0;
        if (_Count != 0) {
            size_t _Bits_used_in_word = 0;
            auto _Last                = _Ptr + _Count;
            _Ty _This_word            = 0;
            do {
                --_Last;
                const auto _Ch = *_Last;
                _This_word |= static_cast<_Ty>(_Traits::eq(_Elem1, _Ch)) << _Bits_used_in_word;
                if (!_Traits::eq(_Elem1, _Ch) && !_Traits::eq(_Elem0, _Ch)) {
                    _Xinv();
                }

                if (++_Bits_used_in_word == _Bitsperword) {
                    _Array[_Wpos] = _This_word;
                    ++_Wpos;
                    _This_word         = 0;
                    _Bits_used_in_word = 0;
                }
            } while (_Ptr != _Last);

            if (_Bits_used_in_word != 0) {
                _Array[_Wpos] = _This_word;
                ++_Wpos;
            }
        }

        for (; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] = 0;
        }
    }

    template <class _Elem, class _Traits, class _Alloc>
    constexpr explicit bitset(
        const std::basic_string<_Elem, _Traits, _Alloc>& _Str,
        typename std::basic_string<_Elem, _Traits, _Alloc>::size_type _Pos   = 0,
        typename std::basic_string<_Elem, _Traits, _Alloc>::size_type _Count = std::basic_string<_Elem, _Traits, _Alloc>::npos,
        _Elem _Elem0                                                         = static_cast<_Elem>('0'),
        _Elem _Elem1                                                         = static_cast<_Elem>('1')) {
        // construct from [_Pos, _Pos + _Count) elements in string
        if (_Str.size() < _Pos) {
            _Xran();  // _Pos off end
        }

        if (_Str.size() - _Pos < _Count) {
            _Count = _Str.size() - _Pos;  // trim _Count to size
        }

        _Construct<_Traits>(_Str.data() + _Pos, _Count, _Elem0, _Elem1);
    }

    template <class _Elem>
    constexpr explicit bitset(const _Elem* _Ntcts,
                              typename std::basic_string<_Elem>::size_type _Count = std::basic_string<_Elem>::npos,
                              _Elem _Elem0                                        = static_cast<_Elem>('0'),
                              _Elem _Elem1                                        = static_cast<_Elem>('1')) {
        if (_Count == std::basic_string<_Elem>::npos) {
            _Count = std::char_traits<_Elem>::length(_Ntcts);
        }

        _Construct<std::char_traits<_Elem>>(_Ntcts, _Count, _Elem0, _Elem1);
    }

    constexpr bitset& operator&=(const bitset& _Right) noexcept {
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] &= _Right._Array[_Wpos];
        }

        return *this;
    }

    constexpr bitset& operator|=(const bitset& _Right) noexcept {
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] |= _Right._Array[_Wpos];
        }

        return *this;
    }

    constexpr bitset& operator^=(const bitset& _Right) noexcept {
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] ^= _Right._Array[_Wpos];
        }

        return *this;
    }

    constexpr bitset& operator<<=(size_t _Pos) noexcept {  // shift left by _Pos, first by words then by bits
        const auto _Wordshift = static_cast<ptrdiff_t>(_Pos / _Bitsperword);
        if (_Wordshift != 0) {
            for (ptrdiff_t _Wpos = _Words; 0 <= _Wpos; --_Wpos) {
                _Array[_Wpos] = _Wordshift <= _Wpos ? _Array[_Wpos - _Wordshift] : 0;
            }
        }

        if ((_Pos %= _Bitsperword) != 0) {  // 0 < _Pos < _Bitsperword, shift by bits
            for (ptrdiff_t _Wpos = _Words; 0 < _Wpos; --_Wpos) {
                _Array[_Wpos] = (_Array[_Wpos] << _Pos) | (_Array[_Wpos - 1] >> (_Bitsperword - _Pos));
            }

            _Array[0] <<= _Pos;
        }
        _Trim();
        return *this;
    }

    constexpr bitset& operator>>=(size_t _Pos) noexcept {  // shift right by _Pos, first by words then by bits
        const auto _Wordshift = static_cast<ptrdiff_t>(_Pos / _Bitsperword);
        if (_Wordshift != 0) {
            for (ptrdiff_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
                _Array[_Wpos] = _Wordshift <= _Words - _Wpos ? _Array[_Wpos + _Wordshift] : 0;
            }
        }

        if ((_Pos %= _Bitsperword) != 0) {  // 0 < _Pos < _Bitsperword, shift by bits
            for (ptrdiff_t _Wpos = 0; _Wpos < _Words; ++_Wpos) {
                _Array[_Wpos] = (_Array[_Wpos] >> _Pos) | (_Array[_Wpos + 1] << (_Bitsperword - _Pos));
            }

            _Array[_Words] >>= _Pos;
        }
        return *this;
    }

    constexpr bitset& set() noexcept {  // set all bits true

        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] = std::numeric_limits<_Ty>::max();
        }
        // std::memset(&_Array, 0xFF, sizeof(_Array));

        _Trim();
        return *this;
    }

    constexpr bitset& set(size_t _Pos, bool _Val = true) {  // set bit at _Pos to _Val
        if (_Bits <= _Pos) {
            _Xran();  // _Pos off end
        }

        return _Set_unchecked(_Pos, _Val);
    }

    constexpr bitset& reset() noexcept {  // set all bits false
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] = 0;
        }
        // std::memset(&_Array, 0, sizeof(_Array));

        return *this;
    }

    constexpr bitset& reset(size_t _Pos) {  // set bit at _Pos to false
        return set(_Pos, false);
    }

    constexpr bitset operator~() const noexcept {  // flip all bits
        bitset _Tmp = *this;
        _Tmp.flip();
        return _Tmp;
    }

    constexpr bitset& flip() noexcept {  // flip all bits
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            _Array[_Wpos] = ~_Array[_Wpos];
        }

        _Trim();
        return *this;
    }

    constexpr bitset& flip(size_t _Pos) {  // flip bit at _Pos
        if (_Bits <= _Pos) {
            _Xran();  // _Pos off end
        }

        return _Flip_unchecked(_Pos);
    }

    constexpr unsigned long to_ulong() const {
        constexpr bool _Bits_zero  = _Bits == 0;
        constexpr bool _Bits_small = _Bits <= 32;
        constexpr bool _Bits_large = _Bits > 64;
        if constexpr (_Bits_zero) {
            return 0;
        } else if constexpr (_Bits_small) {
            return static_cast<unsigned long>(_Array[0]);
        } else {
            if constexpr (_Bits_large) {
                for (size_t _Idx = 1; _Idx <= _Words; ++_Idx) {
                    if (_Array[_Idx] != 0) {
                        _Xoflo();  // fail if any high-order words are nonzero
                    }
                }
            }

            if (_Array[0] > ULONG_MAX) {
                _Xoflo();
            }

            return static_cast<unsigned long>(_Array[0]);
        }
    }

    constexpr unsigned long long to_ullong() const {
        constexpr bool _Bits_zero  = _Bits == 0;
        constexpr bool _Bits_large = _Bits > 64;
        if constexpr (_Bits_zero) {
            return 0;
        } else {
            if constexpr (_Bits_large) {
                for (size_t _Idx = 1; _Idx <= _Words; ++_Idx) {
                    if (_Array[_Idx] != 0) {
                        _Xoflo();  // fail if any high-order words are nonzero
                    }
                }
            }

            return _Array[0];
        }
    }

    template <class _Elem = char, class _Tr = std::char_traits<_Elem>, class _Alloc = std::allocator<_Elem>>
    constexpr std::basic_string<_Elem, _Tr, _Alloc> to_string(_Elem _Elem0 = static_cast<_Elem>('0'),
                                                              _Elem _Elem1 = static_cast<_Elem>('1')) const {
        // convert bitset to string
        std::basic_string<_Elem, _Tr, _Alloc> _Str;
        _Str.reserve(_Bits);

        for (auto _Pos = _Bits; 0 < _Pos;) {
            _Str.push_back(_Subscript(--_Pos) ? _Elem1 : _Elem0);
        }

        return _Str;
    }

    constexpr size_t count() const noexcept {  // count number of set bits
        size_t result = 0;
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            result += std::popcount(_Array[_Wpos]);
        }

        return result;
        // const char* const _Bitsperbyte = "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4"
        //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
        //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
        //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
        //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
        //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
        //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
        //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
        //                                 "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
        //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
        //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
        //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
        //                                 "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
        //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
        //                                 "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
        //                                 "\4\5\5\6\5\6\6\7\5\6\6\7\6\7\7\x8";
        // const unsigned char* _Ptr       = &reinterpret_cast<const unsigned char&>(_Array);
        // const unsigned char* const _End = _Ptr + sizeof(_Array);
        // size_t _Val                     = 0;
        // for (; _Ptr != _End; ++_Ptr) {
        //    _Val += _Bitsperbyte[*_Ptr];
        //}

        // return _Val;
    }

    constexpr size_t size() const noexcept {
        return _Bits;
    }

    constexpr bool operator==(const bitset& _Right) const noexcept {
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            if (_Array[_Wpos] != _Right._Array[_Wpos]) {
                return false;
            }
        }
        return true;
        // return std::memcmp(&_Array[0], &_Right._Array[0], sizeof(_Array)) == 0;
    }

#if !_HAS_CXX20
    constexpr bool operator!=(const bitset& _Right) const noexcept {
        return !(*this == _Right);
    }
#endif  // !_HAS_CXX20

    constexpr bool test(size_t _Pos) const {
        if (_Bits <= _Pos) {
            _Xran();  // _Pos off end
        }

        return _Subscript(_Pos);
    }

    constexpr bool any() const noexcept {
        for (size_t _Wpos = 0; _Wpos <= _Words; ++_Wpos) {
            if (_Array[_Wpos] != 0) {
                return true;
            }
        }

        return false;
    }

    constexpr bool none() const noexcept {
        return !any();
    }

    constexpr bool all() const noexcept {
        constexpr bool _Zero_length = _Bits == 0;
        if constexpr (_Zero_length) {  // must test for this, otherwise would count one full word
            return true;
        }

        constexpr bool _No_padding = _Bits % _Bitsperword == 0;
        for (size_t _Wpos = 0; _Wpos < _Words + _No_padding; ++_Wpos) {
            if (_Array[_Wpos] != ~static_cast<_Ty>(0)) {
                return false;
            }
        }

        return _No_padding || _Array[_Words] == (static_cast<_Ty>(1) << (_Bits % _Bitsperword)) - 1;
    }

    constexpr bitset operator<<(size_t _Pos) const noexcept {
        bitset _Tmp = *this;
        _Tmp <<= _Pos;
        return _Tmp;
    }

    constexpr bitset operator>>(size_t _Pos) const noexcept {
        bitset _Tmp = *this;
        _Tmp >>= _Pos;
        return _Tmp;
    }

    constexpr _Ty _Getword(size_t _Wpos) const noexcept {  // nonstandard extension; get underlying word
        return _Array[_Wpos];
    }

 private:
    friend std::hash<bitset<_Bits>>;

    static constexpr ptrdiff_t _Bitsperword = CHAR_BIT * sizeof(_Ty);
    static constexpr ptrdiff_t _Words       = _Bits == 0 ? 0 : (_Bits - 1) / _Bitsperword;  // NB: number of words - 1

    constexpr void _Trim() noexcept {  // clear any trailing bits in last word
        constexpr bool _Work_to_do = _Bits == 0 || _Bits % _Bitsperword != 0;
        if constexpr (_Work_to_do) {
            _Array[_Words] &= (_Ty{1} << _Bits % _Bitsperword) - 1;
        }
    }

    constexpr bitset& _Set_unchecked(size_t _Pos, bool _Val) noexcept {  // set bit at _Pos to _Val, no checking
        auto& _Selected_word = _Array[_Pos / _Bitsperword];
        const auto _Bit      = _Ty{1} << _Pos % _Bitsperword;
        if (_Val) {
            _Selected_word |= _Bit;
        } else {
            _Selected_word &= ~_Bit;
        }

        return *this;
    }

    constexpr bitset& _Flip_unchecked(size_t _Pos) noexcept {  // flip bit at _Pos, no checking
        _Array[_Pos / _Bitsperword] ^= _Ty{1} << _Pos % _Bitsperword;
        return *this;
    }

    [[noreturn]] constexpr void _Xinv() const {
        throw std::invalid_argument("invalid bitset char");
    }

    [[noreturn]] constexpr void _Xoflo() const {
        throw std::overflow_error("bitset overflow");
    }

  constexpr void _Xran() const {
    // throw std::out_of_range(_Pos);
    //throw std::out_of_range("invalid bitset position");
    }

    _Ty _Array[_Words + 1UL];
};

template <size_t _Bits>
constexpr bitset<_Bits> operator&(const bitset<_Bits>& _Left, const bitset<_Bits>& _Right) noexcept {
    bitset<_Bits> _Ans = _Left;
    _Ans &= _Right;
    return _Ans;
}

template <size_t _Bits>
constexpr bitset<_Bits> operator|(const bitset<_Bits>& _Left, const bitset<_Bits>& _Right) noexcept {
    bitset<_Bits> _Ans = _Left;
    _Ans |= _Right;
    return _Ans;
}

template <size_t _Bits>
constexpr bitset<_Bits> operator^(const bitset<_Bits>& _Left, const bitset<_Bits>& _Right) noexcept {
    bitset<_Bits> _Ans = _Left;
    _Ans ^= _Right;
    return _Ans;
}

}  // namespace nstd

template <size_t _Bits>
struct std::hash<nstd::bitset<_Bits>> {
    constexpr size_t operator()(const nstd::bitset<_Bits>& _BitSet) const noexcept {
        // EXAMPLE ONLY
        std::size_t result = 0;
        for (size_t i = 0; i <= _BitSet._Words; ++i) {
            result ^= _BitSet._Array[i];
        }
        return result;
    }
};
