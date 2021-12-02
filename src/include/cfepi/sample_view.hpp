#ifndef PROBABILITY_SAMPLE_VIEW_HPP
#define PROBABILITY_SAMPLE_VIEW_HPP

#include <ranges>
#include <concepts>
#include <iterator>
#include <random>

namespace probability {
// Replace me with the real concept when available
template<typename E>
concept uniform_random_number_engine = std::uniform_random_bit_generator<E> && requires(E e,
  E::result_type s,
  E &&v,
  const E x,
  const E y) {
  { E() };
  { E(s) };
  { E(x) };
  { E(e) };
  // {e.seed(s)};
  // {e.seed(x)};
  { x == y } -> std::same_as<bool>;
  { x != y } -> std::same_as<bool>;
};

template<std::ranges::input_range R, class Gen>
requires uniform_random_number_engine<std::remove_reference_t<Gen>> && std::ranges::view<R>
class sample_view : public std::ranges::view_interface<sample_view<R, Gen>> {
private:
  R base_;
  Gen gen_;
  std::geometric_distribution<> dist_;

  template<bool Const> struct iterator {
    template<class T> using constify = std::conditional_t<Const, const T, T>;
    using Base = constify<R>;
    using difference_type = std::ranges::range_difference_t<Base>;
    // using value_type = Base::value_type;
    using value_type = std::ranges::iterator_t<Base>::value_type;

    std::ranges::iterator_t<Base> current_{};
    std::ranges::sentinel_t<Base> end_{};
    Gen gen_{};
    std::geometric_distribution<> dist_;


    iterator() = default;
    constexpr iterator(std::ranges::iterator_t<Base> begin,
      Base *base,
      Gen gen,
      std::geometric_distribution<> dist)
      : current_(std::move(begin)), end_(std::ranges::end(*base)), gen_(gen), dist_(dist) {
      std::ranges::advance(current_, dist_(gen_), end_);
    }
    iterator &operator++() {
      std::ranges::advance(current_, dist_(gen_) + 1, end_);
      return (*this);
    }
    iterator operator++(int) {
      const iterator rc{ *this };
      std::ranges::advance(current_, dist_(gen_) + 1, end_);
      return (rc);
    }

    bool operator==(const iterator<Const> other) const { return (current_ == other.current_); }

    constexpr auto operator*() const { return (*current_); }
  };

  template<bool Const> struct sentinel {
    template<class T> using constify = std::conditional_t<Const, const T, T>;
    using Base = constify<R>;

    std::ranges::sentinel_t<Base> end_{};

    sentinel() = default;
    constexpr sentinel(const Base *base) : end_(std::ranges::end(*base)) {}

    friend bool operator==(const iterator<Const> it, const sentinel<Const> &st) {
      return (it.current_ == st.end_);
    }
  };

public:
  constexpr auto begin() const {
    return (iterator<true>{ std::ranges::begin(base_), &base_, gen_, dist_ });
  }
  constexpr auto end() const { return (sentinel<true>{ &base_ }); }

  sample_view() = default;
  sample_view(const R &base, double probability)
    : base_(std::move(base)), gen_(), dist_(probability) {}
  sample_view(const R &base, double probability, Gen gen)
    : base_(std::move(base)), gen_(gen), dist_(probability) {}
};

template<class V, class Gen>
sample_view(V &&, double, Gen) -> sample_view<std::ranges::views::all_t<V>, Gen>;


namespace views {
  namespace detail {
    class sample_fn_base {
    public:
      template<class R, uniform_random_number_engine Gen>
      constexpr auto operator()(R &&r, double probability, Gen gen) const {
        return (sample_view(std::forward<R>(r), probability, gen));
      }
    };

    struct sample_fn : sample_fn_base {
      using sample_fn_base::operator();

      template<uniform_random_number_engine Gen>
      constexpr auto operator()(double probability, Gen gen) const {
        return (
          [this, probability, gen](auto &&r) { return (this->operator()(r, probability, gen)); });
      }
    };

    template<std::ranges::viewable_range T> constexpr auto operator|(T &&t, auto s) {
      return (s(std::forward<T>(t)));
    }

  }// namespace detail
  // template<uniform_random_number_engine Gen>
  inline constexpr detail::sample_fn sample;
}// namespace views
}// namespace probability


namespace cfor {
template<auto Start, auto End, auto Inc, class F> constexpr void constexpr_for(F &&f) {
  if constexpr (Start < End) {
    f(std::integral_constant<decltype(Start), Start>());
    constexpr_for<Start + Inc, End, Inc>(f);
  }
}
template<class F, class Tuple> constexpr void constexpr_for_tuple(F &&f, Tuple &&tuple) {
  constexpr size_t cnt = std::tuple_size_v<std::decay_t<Tuple>>;

  constexpr_for<size_t(0), cnt, size_t(1)>([&](auto i) { f(std::get<i.value>(tuple)); });
}
}// namespace cfor

#endif
