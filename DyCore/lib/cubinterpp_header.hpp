/*
 * Cubinterpp: Single header library
 *
 *
 * SPDX-License-Identifier: MIT
 *
 *
 * MIT License
 *
 * Copyright (c) 2024 Sietze van Buuren
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <mdspan/mdspan.hpp>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace cip {

template <typename T>
class Indexer {
    using Vector = std::vector<T>;

   public:
    Indexer(const Vector &_x)
        : x(_x),
          index_back(x.size() - 2),
          x_front(x[index_front]),
          x_back(x[x.size() - 1]),
          x_delta((x_back - x_front) / (x.size() - 1)) {
    }
    ~Indexer() {
    }

    const std::size_t cell_index(const T xi) const {
        return (xi < x_back)
                   ? ((xi < x_front) ? index_front
                                     : (std::size_t)((xi - x_front) / x_delta))
                   : index_back;
    }

    const std::size_t sort_index(const T xi) const {
        if (xi < x_front) {
            return index_front;
        }
        if (xi >= x_back) {
            return index_back;
        }
        return std::distance(x.begin(),
                             std::upper_bound(x.begin(), x.end(), xi)) -
               1;
    }

   private:
    const Vector x;
    const size_t index_front = 0;
    const size_t index_back;
    const T x_front;
    const T x_back;
    const T x_delta;
};

constexpr inline int factorial(int n) {
    return n <= 1 ? 1 : (n * factorial(n - 1));
}

constexpr inline std::size_t binomial(std::size_t n, std::size_t k) noexcept {
    return (k > n) ? 0 :  // out of range
               (k == 0 || k == n) ? 1
                                  :  // edge
               (k == 1 || k == n - 1) ? n
                                      :  // first
               (k + k < n) ?             // recursive:
               (binomial(n - 1, k - 1) * n) / k
                           :                        //  path to k=1   is faster
               (binomial(n - 1, k) * n) / (n - k);  //  path to k=n-1 is faster
}

template <typename T>
T binomial_power_coefficient(const T y, const int n, const int k) {
    return binomial(n, k) * std::pow(y, n - k);
}

template <std::size_t Base, std::size_t Exp>
constexpr std::size_t power() {
    if constexpr (Exp <= 0) {
        return 1;
    } else {
        return Base * power<Base, Exp - 1>();
    }
}

template <typename T, typename Tx, typename Tf>
std::vector<T> monotonic_slopes(const Tx x, const Tf f) {
    // See https://en.wikipedia.org/wiki/Monotone_cubic_interpolation
    const auto nx = x.size();

    std::vector<T> secants(nx - 1);
    std::vector<T> tangents(nx);

    for (auto k = 0; k < nx - 1; ++k) {
        // secants[k] = (*(f_begin+k+1) - *(f_begin+k)) / (*(x_begin+k+1) -
        // *(x_begin+k));
        secants[k] = (f[k + 1] - f[k]) / (x[k + 1] - x[k]);
    }

    tangents[0] = secants[0];
    for (auto k = 1; k < nx - 1; ++k) {
        tangents[k] = 0.5 * (secants[k - 1] + secants[k]);
    }
    tangents[nx - 1] = secants[nx - 2];

    for (auto k = 0; k < nx - 1; ++k) {
        if (secants[k] == 0.0) {
            tangents[k] = 0.0;
            tangents[k + 1] = 0.0;
        } else {
            T alpha = tangents[k] / secants[k];
            T beta = tangents[k + 1] / secants[k];
            T h = std::hypot(alpha, beta);
            if (h > 3.0) {
                tangents[k] = 3.0 / h * alpha * secants[k];
                tangents[k + 1] = 3.0 / h * beta * secants[k];
            }
        }
    }
    return tangents;
}

template <typename T, typename Tx, typename Tf>
std::vector<T> akima_slopes(const Tx x, const Tf f) {
    /*
    Derivative values for Akima cubic Hermite interpolation

    Akima's derivative estimate at grid node x(i) requires the four finite
    differences corresponding to the five grid nodes x(i-2:i+2).
    For boundary grid nodes x(1:2) and x(n-1:n), append finite differences
    which would correspond to x(-1:0) and x(n+1:n+2) by using the following
    uncentered difference formula correspondin to quadratic extrapolation
    using the quadratic polynomial defined by data at x(1:3)
    (section 2.3 in Akima's paper):
    */

    const auto nx = x.size();

    std::vector<T> delta(nx - 1);
    for (auto i = 0; i < delta.size(); ++i) {
        delta[i] = (f[i + 1] - f[i]) / (x[i + 1] - x[i]);
    }

    T delta_0 = 2.0 * delta[0] - delta[1];
    T delta_m1 = 2.0 * delta_0 - delta[0];
    T delta_n = 2.0 * delta[nx - 2] - delta[nx - 3];
    T delta_n1 = 2.0 * delta_n - delta[nx - 2];

    std::vector<T> delta_new(delta.size() + 4);
    delta_new[0] = delta_m1;
    delta_new[1] = delta_0;
    delta_new[delta_new.size() - 2] = delta_n;
    delta_new[delta_new.size() - 1] = delta_n1;
    for (auto i = 0; i < delta.size(); ++i) {
        delta_new[i + 2] = delta[i];
    }

    /*
    Akima's derivative estimate formula (equation (1) in the paper):

            H. Akima, "A New Method of Interpolation and Smooth Curve Fitting
            Based on Local Procedures", JACM, v. 17-4, p.589-602, 1970.

        s(i) = (|d(i+1)-d(i)| * d(i-1) + |d(i-1)-d(i-2)| * d(i))
             / (|d(i+1)-d(i)|          + |d(i-1)-d(i-2)|)
    */

    std::vector<T> weights(delta_new.size() - 1);
    for (auto i = 0; i < weights.size(); ++i) {
        weights[i] = std::abs(delta_new[i + 1] - delta_new[i]) +
                     std::abs(delta_new[i] + delta_new[i + 1]) / 2.0;
    }

    std::vector<T> s(nx);

    for (auto i = 0; i < nx; ++i) {
        T weights1 = weights[i];      // |d(i-1)-d(i-2)|
        T weights2 = weights[i + 2];  // |d(i+1)-d(i)|
        T delta1 = delta_new[i + 1];  // d(i-1)
        T delta2 = delta_new[i + 2];  // d(i)
        T weights12 = weights1 + weights2;
        if (weights12 == 0.0) {
            // To avoid 0/0, Akima proposed to average the divided differences
            // d(i-1) and d(i) for the edge case of d(i-2) = d(i-1) and d(i) =
            // d(i+1):
            s[i] = 0.5 * (delta1 + delta2);
        } else {
            s[i] = (weights2 * delta1 + weights1 * delta2) / weights12;
        }
    }
    return s;
}

enum class BoundaryConditionType { Natural, Clamped, NotAKnot };

template <BoundaryConditionType BC, typename T, typename Tx, typename Tf>
struct setNaturalSplineBoundaryCondition;

template <typename T, typename Tx, typename Tf>
struct setNaturalSplineBoundaryCondition<BoundaryConditionType::Natural, T, Tx,
                                         Tf> {
    using Vector = std::vector<T>;
    constexpr void operator()(const Tx &x, const Tf &f, Vector &a, Vector &b,
                              Vector &c, Vector &d) const {
        T dx1 = x[1] - x[0];
        b[0] = 2.0 / dx1;
        c[0] = 1.0 / dx1;
        d[0] = 3.0 * (f[1] - f[0]) / (dx1 * dx1);

        const auto nx = x.size();
        T dxN = x[nx - 1] - x[nx - 2];
        a[nx - 1] = 1.0 / dxN;
        b[nx - 1] = 2.0 / dxN;
        d[nx - 1] = 3.0 * (f[nx - 1] - f[nx - 2]) / (dxN * dxN);
    }
};

template <typename T, typename Tx, typename Tf>
struct setNaturalSplineBoundaryCondition<BoundaryConditionType::NotAKnot, T, Tx,
                                         Tf> {
    using Vector = std::vector<T>;
    constexpr void operator()(const Tx &x, const Tf &f, Vector &a, Vector &b,
                              Vector &c, Vector &d) const {
        T dx1 = x[1] - x[0];
        T dx2 = x[2] - x[1];
        b[0] = 1.0 / (dx1 * dx1);
        c[0] = b[0] - 1.0 / (dx2 * dx2);
        d[0] = 2.0 * ((f[1] - f[0]) / (dx1 * dx1 * dx1) -
                      (f[2] - f[1]) / (dx2 * dx2 * dx2));

        // necessary conversion to maintain a tridiagonal matrix
        b[0] += a[1] / dx2;
        c[0] += b[1] / dx2;
        d[0] += d[1] / dx2;

        const auto nx = x.size();
        T dxN1 = x[nx - 1] - x[nx - 2];
        T dxN2 = x[nx - 2] - x[nx - 3];
        a[nx - 1] = 1.0 / (dxN1 * dxN1) - 1.0 / (dxN2 * dxN2);
        b[nx - 1] = -1.0 / (dxN2 * dxN2);
        d[nx - 1] = 2.0 * ((f[nx - 2] - f[nx - 3]) / (dxN2 * dxN2 * dxN2) -
                           (f[nx - 1] - f[nx - 2]) / (dxN1 * dxN1 * dxN1));

        // necessary conversion to maintain a tridiagonal matrix
        a[nx - 1] -= b[nx - 2] / dxN2;
        b[nx - 1] -= c[nx - 2] / dxN2;
        d[nx - 1] -= d[nx - 2] / dxN2;
    }
};

template <typename T, typename Tx, typename Tf>
struct setNaturalSplineBoundaryCondition<BoundaryConditionType::Clamped, T, Tx,
                                         Tf> {
    using Vector = std::vector<T>;
    constexpr void operator()(const Tx &x, const Tf &f, Vector &a, Vector &b,
                              Vector &c, Vector &d) const {
        // Demand the slopes to be zero at the boundaries
        b[0] = T{1};
        const auto nx = x.size();
        b[nx - 1] = T{1};
    }
};

template <typename T>
void thomas_algorithm(const std::vector<T> &a, const std::vector<T> &b,
                      std::vector<T> &c, std::vector<T> &d) {
    auto nd = d.size();

    c[0] /= b[0];
    d[0] /= b[0];

    nd--;
    for (auto i = 1; i < nd; i++) {
        c[i] /= b[i] - a[i] * c[i - 1];
        d[i] = (d[i] - a[i] * d[i - 1]) / (b[i] - a[i] * c[i - 1]);
    }

    d[nd] = (d[nd] - a[nd] * d[nd - 1]) / (b[nd] - a[nd] * c[nd - 1]);

    for (auto i = nd; i-- > 0;) {
        d[i] -= c[i] * d[i + 1];
    }
}

template <typename T, BoundaryConditionType BC = BoundaryConditionType::Natural,
          typename Tx, typename Tf>
std::vector<T> natural_spline_slopes(const Tx x, const Tf f) {
    // https://en.wikipedia.org/wiki/Spline_interpolation

    const auto nx = x.size();

    // vectors that fill up the tridiagonal matrix
    std::vector<T> a(nx, T{0});  // first value of a is not used
    std::vector<T> b(nx, T{0});
    std::vector<T> c(nx, T{0});  // last value of c is not used
    // right hand side
    std::vector<T> d(nx, T{0});

    // rows i = 1, ..., nx-2
    for (auto i = 1; i < nx - 1; ++i) {
        T dxi = x[i] - x[i - 1];
        T dxi1 = x[i + 1] - x[i];
        a[i] = 1.0 / dxi;
        b[i] = 2.0 * (1.0 / dxi + 1.0 / dxi1);
        c[i] = 1.0 / dxi1;
        d[i] = 3.0 * ((f[i] - f[i - 1]) / (dxi * dxi) +
                      (f[i + 1] - f[i]) / (dxi1 * dxi1));
    }

    setNaturalSplineBoundaryCondition<BC, T, Tx, Tf>{}(x, f, a, b, c, d);

    thomas_algorithm<T>(a, b, c, d);

    return d;
}

// VectorN definition template
template <typename T, std::size_t N>
class VectorN {
    using Mdspan =
        std::mdspan<T, std::dextents<std::size_t, N>, std::layout_stride>;
    using Mdspan1D =
        std::mdspan<T, std::dextents<std::size_t, 1>, std::layout_stride>;
    using IndexArray = std::array<std::size_t, N>;

   public:
    // Constructor from dimensions and initial value
    VectorN(const T &initial_value, const IndexArray &dimensions)
        : dimensions_(dimensions),
          data_(calculate_total_size(dimensions), initial_value),
          mdspan(std::mdspan(data_.data(), dimensions_)) {
    }

    // Copy constructor
    VectorN(const VectorN &other)
        : data_(other.data_),
          dimensions_(other.dimensions_),
          mdspan(std::mdspan(data_.data(), dimensions_)) {
    }

    // Constructor from nested vectors
    template <typename NestedVector>
    VectorN(const NestedVector &nested)
        : dimensions_(determine_dimensions<NestedVector, N>(nested)) {
        data_.reserve(calculate_total_size(dimensions_));
        Flatten<NestedVector, N>::apply(nested, data_);
        mdspan = std::mdspan(data_.data(), dimensions_);
    }

    // Constructor from just dimensions
    VectorN(const IndexArray &dimensions) : dimensions_(dimensions) {
        data_.reserve(calculate_total_size(dimensions));
        mdspan = std::mdspan(data_.data(), dimensions_);
    }

    // Access elements using variadic indices
    template <typename... Indices>
    T &operator()(Indices... indices) {
        static_assert(sizeof...(Indices) == N, "Incorrect number of indices");
        const IndexArray idx{static_cast<std::size_t>(indices)...};
        return mdspan[idx];
    }

    template <typename... Indices>
    const T &operator()(Indices... indices) const {
        static_assert(sizeof...(Indices) == N, "Incorrect number of indices");
        const IndexArray idx{static_cast<std::size_t>(indices)...};
        return mdspan[idx];
    }

    // Access elements using std::array indices
    T &operator()(const IndexArray &indices) {
        return mdspan[indices];
    }

    const T &operator()(const IndexArray &indices) const {
        return mdspan[indices];
    }

    template <typename... Args>
    void emplace_back(Args... args) {
        data_.emplace_back(std::forward<Args>(args)...);
    }

    Mdspan get_mdspan() {
        return mdspan;
    }

    const Mdspan get_mdspan() const {
        return mdspan;
    }

    template <typename... Pairs>
    Mdspan submdspan(Pairs &&...pairs) const {
        return std::submdspan(mdspan, std::forward<Pairs>(pairs)...);
    }

    template <typename... Pairs>
    Mdspan submdspan(Pairs &&...pairs) {
        return std::submdspan(mdspan, std::forward<Pairs>(pairs)...);
    }

    template <typename... SliceArgs>
    Mdspan1D submdspan_1d(SliceArgs &&...args) {
        return std::submdspan(mdspan, std::forward<SliceArgs>(args)...);
    }

    // This method takes an rvalue 1D vector and moves its contents into a
    // subview of the ND vector. The additional slice specifiers must yield a 1D
    // submdspan.
    template <typename... SliceSpecs>
    void move_into_submdspan(std::vector<T> &&source, SliceSpecs &&...specs) {
        // Obtain the subview using your existing submdspan function.
        auto subview =
            std::submdspan(mdspan, std::forward<SliceSpecs>(specs)...);
        static_assert(decltype(subview)::rank() == 1,
                      "The submdspan must be 1-dimensional.");

        // Check at runtime that the source vector has the same number of
        // elements as the extent of the 1D subview.
        if (subview.extent(0) != source.size()) {
            throw std::runtime_error(
                "Size mismatch: source vector size does not match subview "
                "extent");
        }
        // Use std::move to transfer the elements.
        // Due to strided layout, the elements are not contiguous in memory.
        for (std::size_t i = 0; i < subview.extent(0); ++i) {
            subview[std::array<std::size_t, 1>{i}] = std::move(source[i]);
        }
    }

    // Move a lower-dimensional VectorN into a submdspan of this VectorN, using
    // the source's operator()(const IndexArray&) to access its elements.
    template <std::size_t M, typename... SliceSpecs>
    void move_into_submdspan(VectorN<T, M> &&source, SliceSpecs &&...specs) {
        // Obtain the target subview.
        auto subview =
            std::submdspan(mdspan, std::forward<SliceSpecs>(specs)...);
        static_assert(
            decltype(subview)::rank() == M,
            "The submdspan must have rank equal to the source VectorN.");

        constexpr std::size_t rank = decltype(subview)::rank();

        // Get extents from the target subview.
        std::array<std::size_t, rank> targetExtents;
        std::size_t targetTotal = 1;
        for (std::size_t d = 0; d < rank; ++d) {
            targetExtents[d] = subview.extent(d);
            targetTotal *= targetExtents[d];
        }

        // Get extents from the source VectorN.
        const auto &srcDims = source.dimensions();
        std::array<std::size_t, M> srcExtents =
            srcDims;  // source.dimensions() returns an std::array<std::size_t,
                      // M>
        std::size_t srcTotal = 1;
        for (std::size_t d = 0; d < M; ++d) {
            srcTotal *= srcExtents[d];
        }

        if (srcTotal != targetTotal) {
            throw std::runtime_error(
                "Size mismatch: source VectorN size does not match target "
                "subview total size");
        }

        // Use a recursive lambda to iterate over all multi-index combinations.
        std::array<std::size_t, rank> indexArray{};
        auto recursiveFill = [&](auto &self, std::size_t dim) -> void {
            if (dim == rank) {
                // For each multi-index, use source.operator()(indexArray) to
                // access the element.
                subview[indexArray] = std::move(source(indexArray));
            } else {
                for (std::size_t i = 0; i < targetExtents[dim]; ++i) {
                    indexArray[dim] = i;
                    self(self, dim + 1);
                }
            }
        };
        recursiveFill(recursiveFill, 0);
    }

    const IndexArray &dimensions() const {
        return dimensions_;
    }
    const std::vector<T> &data() const {
        return data_;
    }

   private:
    IndexArray dimensions_;
    std::vector<T> data_;
    Mdspan mdspan;

    // Utility to calculate total size of the N-dimensional array.
    constexpr std::size_t calculate_total_size(
        const std::array<std::size_t, N> &dimensions) {
        std::size_t total_size = 1;
        for (std::size_t dim : dimensions) {
            total_size *= dim;
        }
        return total_size;
    }

    // Recursive template to handle nested std::vector flattening
    template <typename NestedVector, std::size_t Rank>
    struct Flatten {
        static void apply(const NestedVector &nested, std::vector<T> &flat) {
            for (const auto &inner : nested) {
                Flatten<typename NestedVector::value_type, Rank - 1>::apply(
                    inner, flat);
            }
        }
    };

    template <typename NestedVector>
    struct Flatten<NestedVector, 1> {
        static void apply(const NestedVector &nested, std::vector<T> &flat) {
            flat.insert(flat.end(), nested.begin(), nested.end());
        }
    };

    // General template to determine dimensions of a nested vector
    template <typename NestedVector, std::size_t Rank>
    std::array<std::size_t, Rank> determine_dimensions(
        const NestedVector &vec) {
        static_assert(Rank > 0, "Rank must be greater than 0");
        std::array<std::size_t, Rank> dimensions{};
        dimensions[0] = vec.size();
        if constexpr (Rank > 1) {
            if (!vec.empty()) {
                // Recursively call determine_dimensions on the first element of
                // the vector
                auto sub_dimensions =
                    determine_dimensions<typename NestedVector::value_type,
                                         Rank - 1>(vec[0]);
                std::copy(sub_dimensions.begin(), sub_dimensions.end(),
                          dimensions.begin() + 1);
            }
        }
        return dimensions;
    }
};

template <std::size_t N, std::size_t D, std::size_t current = 0, typename Func,
          typename XiArray, typename... Indices>
void iterate_over_indices(Func &&func, XiArray const &xi, Indices... indices) {
    if constexpr (current == N) {
        func(indices...);
    } else {
        if constexpr (current == D) {
            iterate_over_indices<N, D, current + 1>(std::forward<Func>(func),
                                                    xi, indices...);
        } else {
            for (std::size_t i = 0; i < xi[current].size(); ++i) {
                iterate_over_indices<N, D, current + 1>(
                    std::forward<Func>(func), xi, indices..., i);
            }
        }
    }
}

template <std::size_t I, std::size_t N, std::size_t D, typename Tuple,
          typename Accum>
constexpr auto build_coordinate_indices_impl(const Tuple &t, Accum acc) {
    if constexpr (I == N) {
        return acc;
    } else {
        if constexpr (I == D) {
            return build_coordinate_indices_impl<I + 1, N, D>(
                t, std::tuple_cat(acc, std::make_tuple(std::full_extent)));
        } else if constexpr (I < D) {
            return build_coordinate_indices_impl<I + 1, N, D>(
                t, std::tuple_cat(acc, std::make_tuple(std::get<I>(t))));
        } else {
            return build_coordinate_indices_impl<I + 1, N, D>(
                t, std::tuple_cat(acc, std::make_tuple(std::get<I - 1>(t))));
        }
    }
}

template <std::size_t N, std::size_t D, typename Tuple>
constexpr auto build_coordinate_indices(const Tuple &t) {
    return build_coordinate_indices_impl<0, N, D>(t, std::tuple<>());
}

template <typename FType, typename CoordTuple, typename DerivTuple>
auto call_submdspan_1d(FType &F, const CoordTuple &coord,
                       const DerivTuple &deriv) {
    return std::apply(
        [&F](const auto &...args) { return F.submdspan_1d(args...); },
        std::tuple_cat(coord, deriv));
}

template <typename FType, typename SlopeType, typename CoordTuple,
          typename DerivTuple>
void call_move_into_submdspan(FType &F, SlopeType &&slopes,
                              const CoordTuple &coord,
                              const DerivTuple &deriv) {
    std::apply(
        [&F, &slopes](const auto &...args) {
            F.move_into_submdspan(std::forward<SlopeType>(slopes), args...);
        },
        std::tuple_cat(coord, deriv));
}

constexpr std::size_t SIZE_T_ZERO = 0;

template <std::size_t N>
constexpr auto make_zero_tuple() {
    return []<std::size_t... I>(std::index_sequence<I...>) {
        return std::make_tuple(((void)I, SIZE_T_ZERO)...);
    }(std::make_index_sequence<N>{});
}

template <std::size_t D, typename Tuple, typename T>
constexpr auto update_tuple_element(const Tuple &tup, T new_value) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    return []<std::size_t... Is>(const Tuple &tup, T new_value,
                                 std::index_sequence<Is...>) {
        return std::make_tuple((Is == D ? new_value : std::get<Is>(tup))...);
    }(tup, new_value, std::make_index_sequence<N>{});
}

template <std::size_t N, typename Tuple, typename Func, std::size_t... Is>
constexpr void for_each_dimension_impl(const Tuple &tup, Func &&func,
                                       std::index_sequence<Is...>) {
    ((std::get<Is>(tup) == 0
          ? (func(std::integral_constant<std::size_t, Is>{}), 0)
          : 0),
     ...);
}

template <std::size_t N, typename Tuple, typename Func>
constexpr void for_each_dimension(const Tuple &tup, Func &&func) {
    for_each_dimension_impl<N>(tup, std::forward<Func>(func),
                               std::make_index_sequence<N>{});
}

template <std::size_t N, typename FType, typename XiArray, typename DerivTuple,
          typename CalcSlopesFunctor>
void compute_mixed_derivatives_impl(FType &F, XiArray const &xi,
                                    const DerivTuple &currentDeriv,
                                    CalcSlopesFunctor calcSlopes,
                                    std::size_t start = 0) {
    for_each_dimension<N>(currentDeriv, [&](const auto d_const) {
        constexpr std::size_t D = d_const.value;
        if (D < start) {
            return;
        }
        iterate_over_indices<N, D>(
            [&, d_const](auto... loopIndices) {
                constexpr std::size_t D = d_const.value;
                auto indicesTuple = std::make_tuple(loopIndices...);
                auto coord = build_coordinate_indices<N, D>(indicesTuple);
                auto extractionPattern =
                    update_tuple_element<D>(currentDeriv, 0);
                auto f_slice = call_submdspan_1d(F, coord, extractionPattern);
                auto newDeriv = update_tuple_element<D>(currentDeriv, 1);
                call_move_into_submdspan(F, calcSlopes(xi[D], f_slice), coord,
                                         newDeriv);
            },
            xi);
        auto newDeriv = update_tuple_element<D>(currentDeriv, 1);
        compute_mixed_derivatives_impl<N>(F, xi, newDeriv, calcSlopes, D + 1);
    });
}

template <std::size_t N, typename FType, typename XiArray,
          typename CalcSlopesFunctor>
void compute_mixed_derivatives(FType &F, XiArray const &xi,
                               CalcSlopesFunctor calcSlopes) {
    auto basePattern = make_zero_tuple<N>();
    compute_mixed_derivatives_impl<N>(F, xi, basePattern, calcSlopes, 0);
}

template <typename T, std::size_t N>
class LinearCellND {
    using Span = std::span<const T>;
    using Spans = std::array<Span, N>;
    using Mdspan =
        std::mdspan<const T, std::dextents<std::size_t, N>, std::layout_stride>;
    using NomArray = std::array<T, (1 << N)>;
    static constexpr std::size_t numCorners = 1 << N;

   public:
    explicit LinearCellND(const Mdspan &_f, const Spans &_x)
        : x(_x),
          f(_f),
          H(std::transform_reduce(
              x.begin(), x.end(), T{1}, std::multiplies<>{},
              [](const Span &xi) { return xi[1] - xi[0]; })),
          c(compute_coefficients()) {
    }

    template <typename... Args>
        requires(sizeof...(Args) == N)
    T eval(Args &&...args) const {
        return gather_corners({std::forward<Args>(args)...},
                              std::make_index_sequence<numCorners>{});
    }

   private:
    const Spans x;
    const Mdspan f;
    const T H;
    const NomArray c;

    NomArray compute_coefficients() const {
        NomArray c;
        for (std::size_t J = 0; J < numCorners; ++J) {
            c[J] = compute_c_J(J);
        }
        return c;
    }

    T compute_c_J(std::size_t J) const {
        T c_J = T{0};
        for (std::size_t mask = 0; mask < numCorners; ++mask) {
            T prod = T{1};
            std::array<std::size_t, N> indices{};
            for (std::size_t k = 0; k < N; ++k) {
                indices[k] = (mask & (1u << k)) ? 1 : 0;
                prod *= gamma(J, indices[k], k);
            }
            c_J += f[indices] * prod;
        }
        return c_J / H;
    }

    T gamma(std::size_t J, std::size_t i, std::size_t k) const {
        return (J & (1u << k)) ? (i == 0 ? -T{1} : T{1})
                               : (i == 0 ? x[k][1] : -x[k][0]);
    }

    template <std::size_t... Js>
    T gather_corners(const std::array<T, N> &xs,
                     std::index_sequence<Js...>) const {
        return (... + compute_corner<Js>(xs, std::make_index_sequence<N>{}));
    }

    template <std::size_t J, std::size_t... I>
    T compute_corner(const std::array<T, N> &xs,
                     std::index_sequence<I...>) const {
        return c[J] * (T{1} * ... * ((J & (1u << I)) ? xs[I] : T{1}));
    }

};  // class LinearCellND

template <typename T, std::size_t N>
class LinearInterpND {
    using Vector = std::vector<T>;
    using Array = std::array<Vector, N>;
    using F = VectorN<T, N>;
    using Cell = LinearCellND<T, N>;
    using Cells = VectorN<Cell, N>;
    using Span = std::span<const T>;
    using Pr = std::pair<std::size_t, std::size_t>;
    using Indexers = std::array<Indexer<T>, N>;

   public:
    template <typename... Args>
    LinearInterpND(const F &_f, const Args &..._xi)
        : xi{_xi...},
          f(_f),
          indexers{Indexer<T>(_xi)...},
          cells(build(_xi...)) {
    }

    ~LinearInterpND() {
    }

    template <typename... Args>
    T eval(const Args &...args) const {
        std::size_t dim = 0;
        std::array<size_t, N> indices = {indexers[dim++].sort_index(args)...};
        return cells(indices).eval(args...);
    }

    template <typename... Vectors>
    Vector evaln(const Vectors &...inputs) const {
        static_assert(sizeof...(inputs) > 0,
                      "At least one input vector is required");

        const std::size_t size =
            std::get<0>(std::tuple<const Vectors &...>(inputs...)).size();
        if (!((inputs.size() == size) && ...)) {
            throw std::invalid_argument(
                "All input vectors must have the same size");
        }

        Vector z(size);  // Output vector
        for (std::size_t i = 0; i < size; ++i) {
            z[i] = eval(inputs[i]...);
        }
        return z;
    }

   private:
    const Array xi;
    const F f;
    const Indexers indexers;
    const Cells cells;

    template <typename... Args>
    const Cells build(const Args &..._xi) const {
        Cells _cells({(_xi.size() - 1)...});
        build_cell(_cells);
        return _cells;
    }

    template <typename... Indices>
    void build_cell(Cells &_cells, Indices... indices) const {
        if constexpr (sizeof...(Indices) == N) {
            std::size_t index = 0;
            std::array<Span, N> spans = {Span(&xi[index++][indices], 2)...};
            _cells.emplace_back(f.submdspan(Pr{indices, indices + 1}...),
                                spans);
        } else {
            for (std::size_t i = 0; i < xi[sizeof...(indices)].size() - 1;
                 ++i) {
                build_cell(_cells, indices..., i);
            }
        }
    }

};  // class LinearInterpND

template <typename T>
class LinearCellND<T, 1> {
    using Span = std::span<const T>;

   public:
    explicit LinearCellND(Span x, Span f)
        : x0(x[0]),
          b0((x[1] * f[0] - x[0] * f[1]) / (x[1] - x[0])),
          a0((f[1] - f[0]) / (x[1] - x[0])) {
    }
    ~LinearCellND() {
    }

    T eval(const T &xi) const {
        return b0 + a0 * xi;
    }

   private:
    const T x0;
    const T a0;
    const T b0;
};

template <typename T>
class LinearInterpND<T, 1> {
    using Cell = LinearCellND<T, 1>;
    using Vector = std::vector<T>;
    using Span = std::span<const T>;

   public:
    LinearInterpND(const Vector &_f, const Vector &_x) : indexer(_x) {
        assert(_x.size() == _f.size());
        build(_x, _f);
    }
    ~LinearInterpND() {
    }

    void build(const Vector &x, const Vector &f) {
        cells.reserve(x.size() - 1);
        for (int i = 0; i < x.size() - 1; ++i) {
            cells.push_back(Cell(Span(&x[i], 2), Span(&f[i], 2)));
        }
    }

    T eval(const T xi) const {
        return cells[indexer.sort_index(xi)].eval(xi);
    }

    Vector evaln(const Vector &xi) const {
        auto xi_iter = xi.begin();
        Vector yi(xi.size());
        for (auto &yi_i : yi) {
            yi_i = eval(*xi_iter++);
        }
        return yi;
    }

   private:
    const Indexer<T> indexer;
    std::vector<Cell> cells;
};  // class LinearInterpND case N=1

template <typename T>
class LinearInterp1D : public LinearInterpND<T, 1> {
    using Vector = std::vector<T>;
    using Vector2 = VectorN<T, 1>;

   public:
    explicit LinearInterp1D(const Vector &x, const Vector &f)
        : LinearInterpND<T, 1>(f, x) {
    }

    ~LinearInterp1D() {
    }
};

template <typename T>
class LinearInterp2D : public LinearInterpND<T, 2> {
    using Vector = std::vector<T>;
    using Vector2 = VectorN<T, 2>;

   public:
    explicit LinearInterp2D(const Vector &x, const Vector &y, const Vector2 &f)
        : LinearInterpND<T, 2>(f, x, y) {
    }

    ~LinearInterp2D() {
    }
};

template <typename T>
class LinearInterp3D : public LinearInterpND<T, 3> {
    using Vector = std::vector<T>;
    using Vector3 = VectorN<T, 3>;

   public:
    explicit LinearInterp3D(const Vector &x, const Vector &y, const Vector &z,
                            const Vector3 &f)
        : LinearInterpND<T, 3>(f, x, y, z) {
    }

    ~LinearInterp3D() {
    }
};

template <typename T, std::size_t N>
class CubicCellND {
    static constexpr std::size_t order = 4;
    static constexpr std::size_t numCorners = 1 << N;
    static constexpr std::size_t numCoeffs = 1 << (2 * N);
    using CoeffsArray = std::array<T, numCoeffs>;
    using Array = std::array<T, order>;
    using Array2 = std::array<Array, order>;
    using ArrayN = std::array<T, N>;
    using Delta = std::array<Array2, N>;
    using Span = std::span<const T>;
    using Spans = std::array<Span, N>;
    using Mdspan = std::mdspan<const T, std::dextents<std::size_t, 2 * N>,
                               std::layout_stride>;

   public:
    explicit CubicCellND(const Mdspan &F, const Spans &x)
        : coeffs(calc_coeffs(x, F)) {
    }
    ~CubicCellND() = default;

    template <typename... Args>
        requires(sizeof...(Args) == N)
    T eval(Args &&...xi) const {
        return eval_poly<0>(0, {std::forward<Args>(xi)...});
    }

   private:
    const CoeffsArray coeffs;

    const ArrayN calc_h(const Spans &x) const {
        return std::apply(
            [](const auto &...xi) { return ArrayN{(xi[1] - xi[0])...}; }, x);
    }

    const T calc_h3(const ArrayN &h) const {
        T prod_hi = std::apply([](const auto &...hi) { return (hi * ...); }, h);
        return prod_hi * prod_hi * prod_hi;
    }

    const Array2 calc_delta_ij(const Span &xi) const {
        const T x0 = xi[0];
        const T x1 = xi[1];
        const T x02 = x0 * x0;
        const T x12 = x1 * x1;
        return {
            {{x12 * (x1 - 3.0 * x0), +6.0 * x0 * x1, -3.0 * (x0 + x1), +2.0},
             {-x0 * x12, x1 * (2.0 * x0 + x1), -(x0 + 2.0 * x1), +1.0},
             {x02 * (3.0 * x1 - x0), -6.0 * x0 * x1, +3.0 * (x0 + x1), -2.0},
             {-x1 * x02, x0 * (x0 + 2.0 * x1), -(2.0 * x0 + x1), +1.0}}};
    }

    const Delta calc_delta(const Spans &x) const {
        return std::apply(
            [this](const auto &...xi) { return Delta{calc_delta_ij(xi)...}; },
            x);
    }

    const CoeffsArray calc_coeffs(const Spans &x, const Mdspan &F) const {
        const ArrayN h = calc_h(x);
        const T h3 = calc_h3(h);
        const Delta delta = calc_delta(x);
        CoeffsArray _coeffs = {};
        std::array<std::size_t, 2 * N> indices = {};
        for (std::size_t m_idx = 0; m_idx < numCoeffs; ++m_idx) {
            for (std::size_t i = 0; i < numCorners; ++i) {
                for (std::size_t j = 0; j < numCorners; ++j) {
                    T product = T{1.0};
                    for (std::size_t k = 0; k < N; ++k) {
                        std::size_t i_k = (i >> k) & 1;
                        std::size_t j_k = (j >> k) & 1;
                        std::size_t ij_k = (i_k << 1) | j_k;
                        std::size_t m_k = (m_idx >> ((N - 1 - k) * 2)) & 3;
                        indices[k] = i_k;
                        indices[k + N] = j_k;
                        T h_factor = (j_k == 0) ? 1.0 : h[k];
                        product *= h_factor * delta[k][ij_k][m_k];
                    }
                    _coeffs[m_idx] += F[indices] * product;
                }
            }
            _coeffs[m_idx] /= h3;
        }
        return _coeffs;
    }

    template <std::size_t D>
    constexpr T eval_poly(std::size_t offset, const std::array<T, N> &x) const {
        if constexpr (D == N) {
            return coeffs[offset];
        } else {
            constexpr std::size_t stride = power<order, N - D - 1>();
            T c0 = eval_poly<D + 1>(offset, x);
            T c1 = eval_poly<D + 1>(offset + stride, x);
            T c2 = eval_poly<D + 1>(offset + 2 * stride, x);
            T c3 = eval_poly<D + 1>(offset + 3 * stride, x);
            return c0 + x[D] * (c1 + x[D] * (c2 + x[D] * c3));
        }
    }
};

template <typename T, std::size_t N>
class CubicInterpND {
    static constexpr std::size_t size_t_two = 2;
    using Vector = std::vector<T>;
    using Array = std::array<Vector, N>;
    using Cell = CubicCellND<T, N>;
    using Cells = VectorN<Cell, N>;
    using Span = std::span<const T>;
    using Mdspan =
        std::mdspan<T, std::dextents<std::size_t, N>, std::layout_stride>;
    using Mdspan1D =
        std::mdspan<T, std::dextents<std::size_t, 1>, std::layout_stride>;
    using Ff = VectorN<T, N>;
    using Ff2 = VectorN<T, 2 * N>;
    using Pr = std::pair<std::size_t, std::size_t>;
    using Indexers = std::array<Indexer<T>, N>;

   public:
    template <typename... Args>
    CubicInterpND(const Ff &_f, const Args &..._xi)
        : xi{_xi...},
          indexers{Indexer<T>(_xi)...},
          F(T{}, {_xi.size()..., ((void)_xi, size_t_two)...}),
          cells({(_xi.size() - 1)...}) {
    }
    virtual ~CubicInterpND() {
    }

    virtual Vector calc_slopes(const Vector &x, const Mdspan1D &f) const = 0;

    template <typename... Args>
    void build(const Ff &f, const Args &..._xi) {
        populate_F(f, _xi...);
        build_cell(cells);
    }

    template <typename... Args>
    T eval(const Args &...args) const {
        std::size_t dim = 0;
        std::array<size_t, N> indices = {indexers[dim++].sort_index(args)...};
        return cells(indices).eval(args...);
    }

   private:
    const Array xi;
    const Vector x;
    const Vector y;
    const Indexers indexers;
    Cells cells;
    Ff2 F;

    template <typename... Args>
    void populate_F(Ff f, const Args &..._xi) {  // NOTE: pass f by value, which
                                                 // will be moved into F
        F.move_into_submdspan(std::move(f), ((void)_xi, std::full_extent)...,
                              ((void)_xi, 0)...);
        auto slopesLambda = [this](const Vector &x,
                                   const Mdspan1D &f_slice) -> Vector {
            return this->calc_slopes(x, f_slice);
        };
        compute_mixed_derivatives<N>(F, xi, slopesLambda);
    }

    template <typename... Indices>
    void build_cell(Cells &_cells, Indices... indices) const {
        if constexpr (sizeof...(Indices) == N) {
            std::size_t index = 0;
            std::array<Span, N> spans = {Span(&xi[index++][indices], 2)...};
            _cells.emplace_back(
                F.submdspan(Pr{indices, indices + 1}...,
                            ((void)indices, std::full_extent)...),
                spans);
        } else {
            for (std::size_t i = 0; i < xi[sizeof...(indices)].size() - 1;
                 ++i) {
                build_cell(_cells, indices..., i);
            }
        }
    }
};

template <typename T>
class CubicCellND<T, 1> {
    static constexpr std::size_t order = 4;
    using Array = std::array<T, order>;
    using Alpha = std::array<Array, order>;
    using Span = std::span<const T>;
    using Mdspan =
        std::mdspan<const T, std::dextents<std::size_t, 2>, std::layout_stride>;

   public:
    explicit CubicCellND(const Span &x, const Mdspan &F)
        : coeffs(calc_coeffs(x, F)) {
    }
    ~CubicCellND() = default;

    const T eval(const T x) const {
        return coeffs[0] + (coeffs[1] + (coeffs[2] + coeffs[3] * x) * x) * x;
    }

   private:
    const Array coeffs;

    constexpr Array calc_coeffs(const Span &x, const Mdspan &F) noexcept {
        const T x0 = x[0];
        const T x1 = x[1];
        const T h = x1 - x0;
        const T h3 = h * h * h;
        const T x02 = x0 * x0;
        const T x12 = x1 * x1;
        const T f0 = F[std::array<std::size_t, 2>{0, 0}];
        const T f1 = F[std::array<std::size_t, 2>{1, 0}];
        const T df0 = F[std::array<std::size_t, 2>{0, 1}];
        const T df1 = F[std::array<std::size_t, 2>{1, 1}];
        const T diff = f0 - f1;
        return {(f0 * x12 * (x1 - 3.0 * x0) + f1 * x02 * (3.0 * x1 - x0) -
                 h * x0 * x1 * (df0 * x1 + df1 * x0)) /
                    h3,
                (+6.0 * x0 * x1 * diff + h * (df0 * x1 * (2.0 * x0 + x1) +
                                              df1 * x0 * (x0 + 2.0 * x1))) /
                    h3,
                (-3.0 * (x0 + x1) * diff -
                 h * (df0 * (x0 + 2.0 * x1) + df1 * (2.0 * x0 + x1))) /
                    h3,
                (+2.0 * diff + h * (df0 + df1)) / h3};
    }
};

template <typename T>
class CubicInterpND<T, 1> {
    using Vector = std::vector<T>;
    using Cell = CubicCellND<T, 1>;
    using Cells = std::vector<Cell>;
    using Span = std::span<const T>;
    using VectorN2 = VectorN<T, 2>;
    using Pr = std::pair<std::size_t, std::size_t>;

   public:
    CubicInterpND(const Vector &_x, const Vector &_f)
        : x(_x), indexer(_x), F(T{}, {x.size(), 2}) {
        assert(x.size() == _f.size());
    }
    virtual ~CubicInterpND() {
    }

    virtual Vector calc_slopes(const Vector &x, const Vector &f) const = 0;

    void build(
        Vector f)  // don't pass by reference but by value (to create a copy)!
    {
        const std::size_t n = x.size() - 1;
        F.move_into_submdspan(std::move(f), std::full_extent, 0);
        F.move_into_submdspan(calc_slopes(x, f), std::full_extent, 1);
        cells.reserve(n);
        for (auto i = 0; i < n; ++i) {
            cells.emplace_back(Span(&x[i], 2),
                               F.submdspan(Pr{i, i + 1}, std::full_extent));
        }
    }

    T eval(const T xi) const {
        return cells[indexer.sort_index(xi)].eval(xi);
    };

    Vector evaln(const Vector &xi) const {
        auto xi_iter = xi.begin();
        Vector yi(xi.size());
        for (auto &yi_i : yi) {
            yi_i = eval(*xi_iter++);
        }
        return yi;
    }

   private:
    const Vector x;
    const Indexer<T> indexer;
    Cells cells;
    VectorN2 F;
};

template <typename T, std::size_t N = 1>
class MonotonicSpline1D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;

   public:
    MonotonicSpline1D(const Vector &x, const Vector &f)
        : CubicInterpND<T, N>(x, f) {
        this->build(f);
    }

    ~MonotonicSpline1D() {
    }

    Vector calc_slopes(const Vector &x, const Vector &f) const override {
        return monotonic_slopes<T>(x, f);
    }
};

template <typename T, std::size_t N = 1>
class AkimaSpline1D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;

   public:
    AkimaSpline1D(const Vector &x, const Vector &f)
        : CubicInterpND<T, N>(x, f) {
        this->build(f);
    }

    ~AkimaSpline1D() {
    }

    Vector calc_slopes(const Vector &x, const Vector &f) const override {
        return akima_slopes<T>(x, f);
    }
};

template <typename T, std::size_t N = 1,
          BoundaryConditionType BC = BoundaryConditionType::Natural>
class NaturalSpline1D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;

   public:
    NaturalSpline1D(const Vector &x, const Vector &f)
        : CubicInterpND<T, N>(x, f) {
        this->build(f);
    }

    ~NaturalSpline1D() {
    }

    Vector calc_slopes(const Vector &x, const Vector &f) const override {
        return natural_spline_slopes<T, BC>(x, f);
    }
};

template <typename T, std::size_t N = 2>
class MonotonicSpline2D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;
    using Mdspan1D =
        std::mdspan<T, std::dextents<std::size_t, 1>, std::layout_stride>;
    using VectorN = VectorN<T, N>;

   public:
    MonotonicSpline2D(const Vector &_x, const Vector &_y, const VectorN &_f)
        : CubicInterpND<T, N>(_f, _x, _y) {
        this->build(_f, _x, _y);
    }
    ~MonotonicSpline2D() {
    }

    Vector calc_slopes(const Vector &x, const Mdspan1D &f) const override {
        return monotonic_slopes<T>(x, f);
    }
};

template <typename T, std::size_t N = 2>
class AkimaSpline2D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;
    using Mdspan1D =
        std::mdspan<T, std::dextents<std::size_t, 1>, std::layout_stride>;
    using VectorN = VectorN<T, N>;

   public:
    AkimaSpline2D(const Vector &_x, const Vector &_y, const VectorN &_f)
        : CubicInterpND<T, N>(_f, _x, _y) {
        this->build(_f, _x, _y);
    }
    ~AkimaSpline2D() {
    }

    Vector calc_slopes(const Vector &x, const Mdspan1D &f) const override {
        return akima_slopes<T>(x, f);
    }
};

template <typename T,
          BoundaryConditionType BC = BoundaryConditionType::NotAKnot,
          std::size_t N = 2>
class NaturalSpline2D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;
    using Mdspan1D =
        std::mdspan<T, std::dextents<std::size_t, 1>, std::layout_stride>;
    using VectorN = VectorN<T, N>;

   public:
    NaturalSpline2D(const Vector &_x, const Vector &_y, const VectorN &_f)
        : CubicInterpND<T, N>(_f, _x, _y) {
        this->build(_f, _x, _y);
    }
    ~NaturalSpline2D() {
    }

    Vector calc_slopes(const Vector &x, const Mdspan1D &f) const override {
        return natural_spline_slopes<T, BC>(x, f);
    }
};

template <typename T,
          BoundaryConditionType BC = BoundaryConditionType::NotAKnot,
          std::size_t N = 3>
class NaturalSpline3D : public CubicInterpND<T, N> {
    using Vector = std::vector<T>;
    using Mdspan1D =
        std::mdspan<T, std::dextents<std::size_t, 1>, std::layout_stride>;
    using VectorN = VectorN<T, N>;

   public:
    NaturalSpline3D(const Vector &_x, const Vector &_y, const Vector &_z,
                    const VectorN &_f)
        : CubicInterpND<T, N>(_f, _x, _y, _z) {
        this->build(_f, _x, _y, _z);
    }
    ~NaturalSpline3D() {
    }

    Vector calc_slopes(const Vector &x, const Mdspan1D &f) const override {
        return natural_spline_slopes<T, BC>(x, f);
    }
};

}  // namespace cip
