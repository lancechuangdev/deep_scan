#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <filesystem>
#include <gtkmm.h>

class FileUtils
{
public:
    // Static method to get image files from the specified folder
    static std::vector<std::string> getImageFiles(const std::string &folder_path);
};

#endif // FILEUTILS_H