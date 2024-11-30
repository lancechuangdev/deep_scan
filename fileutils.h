#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <iostream>
#include <filesystem>
#include <random>
#include <gtkmm.h>
#include <sys/stat.h> // For mkdir()
#include <regex>
#include <giomm.h>

class FileUtils
{
public:
    static std::string getCssFilePath();
    static std::string getGladeFilePath();
    static std::vector<std::string> getImageFiles(const std::string &folder_path, bool get_masks = false);
    static std::vector<std::string> findMatchingImages(const std::string& dirPath, const std::string& baseName, const std::regex& pattern);
    static std::string get_extension(const std::string &filename);
    static std::string constructMaskPath(const std::string &imagePath);
    static std::string constructMaskName(const std::string &imageName);
    static std::string constructPatchName(const std::string &imagePath, const std::string &extension);
    static std::string generateRandomString(size_t length);
    static std::string replaceExtension(const std::string &filename, const std::string &newExtension);
    static bool directoryExists(const std::string& parent, const std::string& sub);
    static bool createFile(const std::string& path);
    static bool createSubdirectory(const std::string& parent, const std::string& sub);
    static bool checkImagesDimensions(std::vector<std::string> images, int width, int height);
    static bool checkImagesHaveMasks(std::vector<std::string> images, std::vector<std::string> masks);
    static bool createTrainingDatasetDirs(const std::string &destination, std::filesystem::path &trainImagesPath, std::filesystem::path &trainMasksPath, std::filesystem::path &valImagesPath, std::filesystem::path &valMasksPath, std::filesystem::path &testImagesPath, std::filesystem::path &testMasksPath);
    static bool splitAndCopyImagesAndMasks(std::vector<std::string> images, std::vector<std::string> masks, const std::string& destination, double trainRatio, double valRatio, double testRatio);
};

#endif // FILEUTILS_H