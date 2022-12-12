// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <shared_mutex>
#include <fstream>
#include <mutex>
#include "crocore/filesystem.hpp"

namespace crocore::filesystem
{

/////////// implementation internal /////////////

std::string expand_user(std::string path)
{
    path = trim(path);

    if(!path.empty() && path[0] == '~')
    {
        if(path.size() != 1 && path[1] != '/'){ return path; } // or other error handling ?
        char const *home = getenv("HOME");
        if(home || ((home = getenv("USERPROFILE"))))
        {
            path.replace(0, 1, home);
        }
        else
        {
            char const *hdrive = getenv("HOMEDRIVE"),
                    *hpath = getenv("HOMEPATH");
            if(!(hdrive && hpath)){ return path; } // or other error handling ?
            path.replace(0, 1, std::string(hdrive) + hpath);
        }
    }
    return path;
}

/////////// end implemantation internal /////////////

///////////////////////////////////////////////////////////////////////////////

std::vector<std::string> get_directory_entries(const std::filesystem::path &thePath, const std::string &theExtension,
                                               int the_recursion_depth)
{
    std::vector<std::string> ret;
    std::filesystem::path p(expand_user(thePath.string()));

    auto check_file_status = [](const std::filesystem::file_status &s) -> bool
    {
        return std::filesystem::is_regular_file(s) ||
               std::filesystem::is_symlink(s) ||
               std::filesystem::is_character_file(s) ||
               std::filesystem::is_block_file(s);
    };

    try
    {
        if(std::filesystem::exists(p))    // does p actually exist?
        {
            if(the_recursion_depth)
            {
                std::filesystem::recursive_directory_iterator it(p), end;

                while(it != end)
                {
                    if(check_file_status(it->status()))
                    {
                        if(theExtension.empty())
                        {
                            ret.push_back(it->path().string());
                        }
                        else
                        {
                            std::string ext = it->path().extension().string();
                            if(!ext.empty()){ ext = ext.substr(1); }
                            if(theExtension == ext){ ret.push_back(it->path().string()); }
                        }
                    }

                    try{ ++it; }
                    catch(std::exception &e)
                    {
                        // e.g. no permission
                        it.disable_recursion_pending();
                        ++it;
                    }
                }
            }
            else
            {
                std::filesystem::directory_iterator it(p), end;

                while(it != end)
                {
                    if(check_file_status(it->status()))
                    {
                        if(theExtension.empty())
                        {
                            ret.push_back(it->path().string());
                        }
                        else
                        {
                            std::string ext = it->path().extension().string();
                            if(!ext.empty()){ ext = ext.substr(1); }
                            if(theExtension == ext){ ret.push_back(it->path().string()); }
                        }
                    }

                    try{ ++it; }
                    catch(std::exception &e)
                    {
//                        LOG_ERROR << e.what();
                    }
                }
            }
        }
        else
        {
//            LOG_TRACE << p << " does not exist";
        }
    }
    catch(const std::exception &e)
    {
//        LOG_ERROR << e.what();
    }
    std::sort(ret.begin(), ret.end());
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

std::vector<std::string> get_directory_entries(const std::filesystem::path &thePath, FileType the_type,
                                               int the_recursion_depth)
{
    auto ret = get_directory_entries(thePath, "", the_recursion_depth);
    ret.erase(std::remove_if(ret.begin(), ret.end(), [the_type](const std::string &f)
    {
        return get_file_type(f) != the_type;
    }), ret.end());
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

size_t get_file_size(const std::filesystem::path &the_file_name)
{
    return std::filesystem::file_size(expand_user(the_file_name.string()));
}

///////////////////////////////////////////////////////////////////////////////

/// read a complete file into a string
std::string read_file(const std::filesystem::path &theUTF8Filename)
{
    auto path = expand_user(theUTF8Filename.string());
    std::ifstream inStream(path);

    if(!inStream.is_open()){ throw OpenFileFailed(path); }
    return {std::istreambuf_iterator<char>(inStream), std::istreambuf_iterator<char>()};
}

///////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> read_binary_file(const std::filesystem::path &theUTF8Filename)
{
    std::filesystem::path path = expand_user(theUTF8Filename.string());
    std::ifstream inStream(path, std::ios::in | std::ios::binary | std::ios::ate);

    if(!inStream.good()){ throw OpenFileFailed(theUTF8Filename.string()); }
    std::vector<uint8_t> content;
    content.reserve(inStream.tellg());
    inStream.seekg(0);

    content.insert(content.end(), (std::istreambuf_iterator<char>(inStream)),
                   std::istreambuf_iterator<char>());
    return content;
}

///////////////////////////////////////////////////////////////////////////////

bool write_file(const std::filesystem::path &the_file_name, const std::string &the_data)
{
    std::ofstream file_out(expand_user(the_file_name.string()));
    if(!file_out){ return false; }
    file_out << the_data;
    file_out.close();
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool write_file(const std::filesystem::path &the_file_name, const std::vector<uint8_t> &the_data)
{
    std::ofstream file_out(expand_user(the_file_name.string()), std::ios::out | std::ios::binary);
    if(!file_out){ return false; }
    file_out.write(reinterpret_cast<const char *>(&the_data[0]), the_data.size());
    file_out.close();
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool append_to_file(const std::filesystem::path &the_file_name, const std::string &the_data)
{
    std::ofstream file_out(expand_user(the_file_name.string()), std::ios::out | std::ios::app);
    if(!file_out){ return false; }
    file_out << the_data;
    file_out.close();
    return true;
}

///////////////////////////////////////////////////////////////////////////////

std::string get_filename_part(const std::filesystem::path &the_file_name)
{
    return std::filesystem::path(expand_user(the_file_name.string())).filename().string();
}

///////////////////////////////////////////////////////////////////////////////

bool is_uri(const std::string &str)
{
    auto result = str.find("://");

    if(result == std::string::npos || result == 0){ return false; }

    for(size_t i = 0; i < result; ++i)
    {
        if(!::isalpha(str[i])){ return false; }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool is_directory(const std::filesystem::path &the_file_name)
{
    return std::filesystem::is_directory(expand_user(the_file_name.string()));
}

///////////////////////////////////////////////////////////////////////////////

bool is_absolute(const std::filesystem::path &the_file_name)
{
    return std::filesystem::path(the_file_name).is_absolute();
}

///////////////////////////////////////////////////////////////////////////////

bool is_relative(const std::filesystem::path &the_file_name)
{
    return std::filesystem::path(the_file_name).is_relative();
}

///////////////////////////////////////////////////////////////////////////////

bool exists(const std::filesystem::path &the_file_name)
{
    return std::filesystem::exists(expand_user(the_file_name.string()));
}

///////////////////////////////////////////////////////////////////////////////

bool create_directory(const std::filesystem::path &the_file_name)
{
    if(!std::filesystem::exists(the_file_name))
    {
        try{ return std::filesystem::create_directory(the_file_name); }
        catch(std::exception &e){ return false; }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

std::string join_paths(const std::filesystem::path &p1, const std::filesystem::path &p2)
{
    return (p1 / p2).string();
}

///////////////////////////////////////////////////////////////////////////////

std::string path_as_uri(const std::filesystem::path &p)
{
    if(is_uri(p.string())){ return p.string(); }
    return "file://" + std::filesystem::canonical(expand_user(p.string())).string();
}

///////////////////////////////////////////////////////////////////////////////

std::string get_directory_part(const std::filesystem::path &the_file_name)
{
    auto expanded_path = expand_user(the_file_name.string());

    if(is_directory(expanded_path))
    {
        return std::filesystem::path(expanded_path).string();
    }
    else
    {
        return std::filesystem::path(expanded_path).parent_path().string();
    }
}

///////////////////////////////////////////////////////////////////////////////

std::string get_extension(const std::filesystem::path &the_file_name)
{
    std::filesystem::path p = expand_user(the_file_name.string());
    return p.extension().string();
}

///////////////////////////////////////////////////////////////////////////////

std::string remove_extension(const std::filesystem::path &the_file_name)
{
    return std::filesystem::path(expand_user(the_file_name.string())).replace_extension().string();
}

///////////////////////////////////////////////////////////////////////////////

FileType get_file_type(const std::filesystem::path &file_name)
{
    if(!std::filesystem::exists(file_name)){ return FileType::NOT_A_FILE; }
    if(std::filesystem::is_directory(file_name)){ return FileType::DIRECTORY; }
    std::string ext = get_extension(file_name);
    ext = ext.empty() ? ext : crocore::to_lower(ext.substr(1));

    const std::set<std::string>
            image_exts{"png", "jpg", "jpeg", "gif", "bmp", "tga", "hdr"},
            audio_exts{"wav", "m4a", "mp3"},
            model_exts{"obj", "dae", "3ds", "ply", "md5mesh", "fbx", "gltf", "glb"},
            movie_exts{"mpg", "mov", "avi", "mp4", "m4v", "mkv"},
            font_exts{"ttf", "otf", "ttc"};

    if(crocore::contains(image_exts, ext)){ return FileType::IMAGE; }
    else if(crocore::contains(model_exts, ext)){ return FileType::MODEL; }
    else if(crocore::contains(audio_exts, ext)){ return FileType::AUDIO; }
    else if(crocore::contains(movie_exts, ext)){ return FileType::MOVIE; }
    else if(crocore::contains(font_exts, ext)){ return FileType::FONT; }

    return FileType::OTHER;
}

///////////////////////////////////////////////////////////////////////////////

}// namespace crocore::filesystem
