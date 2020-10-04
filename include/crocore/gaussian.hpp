//
// Created by crocdialer on 9/25/20.
//
#pragma once

#include <array>
#include <cmath>

namespace crocore
{

/**
 * @brief   Create a one-dimensional gaussian kernel with provided size
 *          and standard deviation (sigma)
 *
 * @tparam  Size    the size of the created kernel
 * @tparam  Real    the floating point type to use
 *
 * @param   sigma   the standard deviation for the kernel
 * @return  a newly created std::array containing the kernel
 */
template<std::size_t Size = 5, typename Real = float>
std::array<Real, Size> createGaussianKernel_1D(double sigma)
{
    static_assert(Size % 2, "gaussian kernel-size must be odd");

    std::array<Real, Size> kernel_one_dim;
    double sigmaX = sigma > 0 ? sigma : ((Size - 1) * 0.5 - 1) * 0.3 + 0.8;
    double scale2X = -0.5 / (sigmaX * sigmaX);
    double sum = 0;

    for (uint32_t i = 0; i < Size; i++)
    {
        double x = i - (Size - 1) * 0.5;
        kernel_one_dim[i] = std::exp(scale2X * x * x);
        sum += kernel_one_dim[i];
    }
    sum = 1. / sum;
    for (uint32_t i = 0; i < Size; i++) { kernel_one_dim[i] *= sum; }
    return kernel_one_dim;
}

/**
 * @brief   Create a two-dimensional gaussian kernel with provided size
 *          and standard deviation (sigma)
 *
 * @tparam  Size    the size of the created kernel
 * @tparam  Real    the floating point type to use
 *
 * @param   sigma_x the standard deviation for the kernel in direction of x
 * @param   sigma_y the standard deviation for the kernel in direction of y
 * @return  a newly created std::array containing a two-dimensional column-major kernel
 */
template<std::size_t Size = 5, typename Real = float>
std::array<Real, Size * Size> createGaussianKernel_2D(double sigma_x, double sigma_y)
{
    auto kernel_x = createGaussianKernel_1D<Size, Real>(sigma_x);
    auto kernel_y = createGaussianKernel_1D<Size, Real>(sigma_y);

    // col-major quadratic matrix, containing the 2-dimensional blur-kernel
    std::array<Real, Size * Size> combinedKernel;

    // perform an outer product
    for (uint32_t c = 0; c < Size; c++)
    {
        for (uint32_t r = 0; r < Size; r++)
        {
            combinedKernel[c * Size + r] = kernel_x[c] * kernel_y[r];
        }
    }
    return combinedKernel;
}

}// namespace crocore
