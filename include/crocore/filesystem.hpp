// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "crocore/crocore.hpp"
#include <filesystem>

namespace crocore
{
namespace filesystem
{

enum class FileType
{
    IMAGE, MODEL, AUDIO, MOVIE, DIRECTORY, FONT, OTHER, NOT_A_FILE
};

size_t get_file_size(const std::filesystem::path &the_file_name);

// manage known file locations
std::set<std::filesystem::path> search_paths();

void add_search_path(const std::filesystem::path &path, int recursion_depth = 0);

void clear_search_paths();

std::vector<std::string> get_directory_entries(const std::filesystem::path &thePath,
                                               const std::string &theExtension = "",
                                               int the_recursion_depth = 0);

std::vector<std::string> get_directory_entries(const std::filesystem::path &thePath, FileType the_type,
                                               int the_recursion_depth = 0);

bool exists(const std::filesystem::path &the_file_name);

bool is_uri(const std::string &str);

bool is_directory(const std::filesystem::path &the_file_name);

bool is_absolute(const std::filesystem::path &the_file_name);

bool is_relative(const std::filesystem::path &the_file_name);

bool create_directory(const std::filesystem::path &the_file_name);

std::string join_paths(const std::filesystem::path &p1, const std::filesystem::path &p2);

std::string path_as_uri(const std::filesystem::path &p);

std::string read_file(const std::filesystem::path &theUTF8Filename);

std::vector<uint8_t> read_binary_file(const std::filesystem::path &theUTF8Filename);

bool write_file(const std::filesystem::path &the_file_name, const std::string &the_data);

bool write_file(const std::filesystem::path &the_file_name, const std::vector<uint8_t> &the_data);

bool append_to_file(const std::filesystem::path &the_file_name, const std::string &the_data);

std::string get_filename_part(const std::filesystem::path &the_file_name);

std::string get_directory_part(const std::filesystem::path &the_file_name);

std::filesystem::path search_file(const std::filesystem::path &file_name);

std::string get_extension(const std::filesystem::path &thePath);

std::string remove_extension(const std::filesystem::path &the_file_name);

FileType get_file_type(const std::filesystem::path &file_name);

/************************ Exceptions ************************/

class FileNotFoundException : public std::runtime_error
{
private:
    std::string m_file_name;
public:
    explicit FileNotFoundException(const std::string &the_file_name) :
            std::runtime_error(std::string("File not found: ") + the_file_name),
            m_file_name(the_file_name){}

    std::string file_name() const{ return m_file_name; }
};

class OpenDirectoryFailed : public std::runtime_error
{
public:
    explicit OpenDirectoryFailed(const std::string &theDir) :
            std::runtime_error(std::string("Could not open directory: ") + theDir){}
};

class OpenFileFailed : public std::runtime_error
{
public:
    explicit OpenFileFailed(const std::string &the_file_name) :
            std::runtime_error(std::string("Could not open file: ") + the_file_name){}
};
}// namespace filesystem

namespace fs = filesystem;

}// namespace crocore
