// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Timer.h
//
//  Created by Croc Dialer on 11/09/16.

#pragma once

#include "crocore.hpp"
#include "Area.hpp"

namespace crocore {

DEFINE_CLASS_PTR(Image);

class Image
{
public:
    enum class Type
    {
        UNKNOWN = 0, GRAY, RGB, BGR, RGBA, BGRA
    };

    [[nodiscard]] virtual uint32_t width() const = 0;

    [[nodiscard]] virtual uint32_t height() const = 0;

    [[nodiscard]] virtual uint32_t num_components() const = 0;

    virtual void *data() = 0;

    [[nodiscard]] virtual size_t num_bytes() const = 0;

    virtual void offsets(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) const = 0;

    [[nodiscard]] virtual ImagePtr resize(uint32_t the_width, uint32_t the_height) const = 0;

    //! kernel is interpreted col-major
    virtual ImagePtr convolve(const std::vector<float> &the_kernel) = 0;

    virtual ImagePtr blur() = 0;

    virtual void flip(bool horizontal) = 0;

    Type type = Type::UNKNOWN;

    Area_<uint32_t> roi = {};
};

template<class T>
class Image_ : public Image
{
public:

    using Ptr = std::shared_ptr<Image_<T>>;
    using ConstPtr = std::shared_ptr<const Image_<T>>;

    static Ptr create(T *theData, uint32_t the_width, uint32_t the_height,
                      uint32_t the_num_components = 0, bool not_dispose = false)
    {
        return Ptr(new Image_<T>(theData, the_width, the_height, the_num_components, not_dispose));
    };

    static Ptr create(uint32_t the_width, uint32_t the_height, uint32_t the_num_components = 0)
    {
        return Ptr(new Image_<T>(the_width, the_height, the_num_components));
    };

    inline T *at(uint32_t x, uint32_t y) const { return m_data + (x + y * m_width) * m_num_components * sizeof(T); };

    inline T *data_start_for_roi() const
    {
        return m_data + (roi.y * m_width + roi.x) * m_num_components * sizeof(T);
    }

    [[nodiscard]] inline size_t num_bytes() const override { return m_height * m_width * m_num_components * sizeof(T); }

    [[nodiscard]] ImagePtr resize(uint32_t the_width, uint32_t the_height) const override;

    //! kernel is interpreted col-major
    ImagePtr convolve(const std::vector<float> &the_kernel) override;

    ImagePtr blur() override;

    void flip(bool horizontal) override;

    void offsets(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) const override;

    [[nodiscard]] inline uint32_t width() const override { return m_width; };

    [[nodiscard]] inline uint32_t height() const override { return m_height; };

    inline void *data() override { return (void *)m_data; };

    [[nodiscard]] inline uint32_t num_components() const override { return m_num_components; };

    Image_(const Image_ &the_other) = delete;

    Image_(Image_ &&the_other) noexcept;

    Image_ &operator=(Image_ the_other);

    virtual ~Image_();

    template<class S>
    friend void swap(Image_<S> &lhs, Image_<S> &rhs);

private:

    Image_() = default;

    Image_(T *data,
           uint32_t width,
           uint32_t height,
           uint32_t the_num_components,
           bool not_dispose);

    Image_(uint32_t width,
           uint32_t height,
           uint32_t num_components);

    T *m_data = nullptr;
    uint32_t m_width = 0, m_height = 0;
    uint32_t m_num_components = 1;
    bool do_not_dispose = false;
};

class ImageLoadException : public std::runtime_error
{
public:
    ImageLoadException() : std::runtime_error("Got trouble decoding image file") {};
};

ImagePtr create_image_from_file(const std::string &the_path, int num_channels = 0);

ImagePtr create_image_from_data(const std::vector<uint8_t> &the_data, int num_channels = 0);

ImagePtr create_image_from_data(const uint8_t *the_data, size_t num_bytes, int num_channels = 0);

template<class T>
void copy_image(const typename Image_<T>::Ptr &src_img, typename Image_<T>::Ptr &dst_img);

bool save_image_to_file(const ImagePtr &the_img, const std::string &the_path);

std::vector<uint8_t> encode_png(const ImagePtr &img);

std::vector<uint8_t> encode_jpg(const ImagePtr &img);

ImagePtr compute_distance_field(const ImagePtr& the_img, float spread);
}
