#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <iostream>
#include <filesystem>
#include <random>
#include <gtkmm.h>
#include <sys/stat.h> // For mkdir()
#include <regex>

class FileUtils
{
public:
    // Static method to get image files from the specified folder
    static std::vector<std::string> getImageFiles(const std::string &folder_path);
    static std::vector<std::string> findMatchingImages(const std::string& dirPath, const std::string& baseName, const std::regex& pattern);
    static std::string get_extension(const std::string &filename);
    static std::string constructMaskPath(const std::string &imagePath);
    static std::string constructMaskName(const std::string &imageName);
    static std::string constructPatchName(const std::string &imagePath, const std::string &extension);
    static std::string generateRandomString(size_t length);
    static std::string replaceExtension(const std::string &filename, const std::string &newExtension);
    static bool directoryExists(const std::string& parent, const std::string& sub);
    static bool createSubdirectory(const std::string& parent, const std::string& sub);
};

#endif // FILEUTILS_H