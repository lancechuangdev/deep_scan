#include "fileutils.h"

std::string FileUtils::getGladeFilePath()
{
    const std::filesystem::path dev_path = "../ui.glade";
    const std::filesystem::path install_path = "/usr/local/share/deep-scan/ui.glade";

    if (std::filesystem::exists(dev_path))
    {
        return dev_path;
    }
    else if (std::filesystem::exists(install_path))
    {
        return install_path;
    }
    else
    {
        std::cerr << "UI file not found!" << std::endl;
        return "";
    }
}

std::vector<std::string> FileUtils::getImageFiles(const std::string &folder_path)
{
    std::vector<std::string> image_files;
    Glib::Dir dir(folder_path);

    // Supported image file extensions
    std::vector<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".bmp"};

    // Iterate through files in the folder
    for (const auto &file : dir)
    {
        std::string file_path = folder_path + "/" + file;

        // Get the file extension by extracting the base name and finding the dot
        std::string basename = Glib::path_get_basename(file);

        // Skip files that end with '_mask'
        if (basename.find("_mask") != std::string::npos)
        {
            continue; // Skip mask files
        }

        std::string::size_type idx = basename.rfind('.');

        if (idx != std::string::npos)
        {
            std::string extension = basename.substr(idx); // Extract extension
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            // Check if the extension matches a supported image format
            if (std::find(image_extensions.begin(), image_extensions.end(), extension) != image_extensions.end())
            {
                image_files.push_back(file_path);
            }
        }
    }

    return image_files;
}

std::vector<std::string> FileUtils::findMatchingImages(const std::string &dirPath, const std::string &baseName, const std::regex &pattern)
{
    std::vector<std::string> matchingFiles;

    for (const auto &entry : std::filesystem::directory_iterator(dirPath))
    {
        if (entry.is_regular_file())
        {
            std::string fileName = entry.path().filename().string();
            if (std::regex_match(fileName, pattern))
            {
                matchingFiles.push_back(entry.path().string());
            }
        }
    }

    return matchingFiles;
}

std::string FileUtils::get_extension(const std::string &filename)
{
    size_t dot_pos = filename.find_last_of(".");
    if (dot_pos == std::string::npos)
    {
        return ""; // No extension found
    }
    return filename.substr(dot_pos); // Includes the dot (e.g., ".jpg")
}

std::string FileUtils::constructMaskPath(const std::string &imagePath)
{
    // Get the base name (filename with extension)
    std::string baseName = Glib::path_get_basename(imagePath); // Get filename with extension
    std::string extension = get_extension(baseName);           // Get extension (e.g., .jpg)

    // Remove the extension from base name
    baseName = baseName.substr(0, baseName.length() - extension.length());

    // Construct the new name by appending '_mask'
    std::string maskName = baseName + "_mask" + extension;

    // Return the new full path with '_mask' appended
    std::string directory = Glib::path_get_dirname(imagePath);

    return Glib::build_filename(directory, maskName);
}

std::string FileUtils::constructPatchName(const std::string &imagePath, const std::string &extension)
{
    // Get the base name (filename with extension)
    std::string baseName = Glib::path_get_basename(imagePath); // Get filename with extension

    // Remove the extension from base name
    baseName = baseName.substr(0, baseName.length() - get_extension(baseName).length());

    // Get current time and format it as YYYYMMDD_HHMMSS
    char timestamp[20];
    std::time_t now = std::time(nullptr);
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));

    // Append timestamp to the folder path
    std::string timestampStr(timestamp);

    // Construct the new name by appending the random string
    std::string patchName = baseName + "_" + timestampStr + "_" + generateRandomString(8) + extension;

    return patchName;
}

std::string FileUtils::constructMaskName(const std::string &imageName)
{
    std::string extension = get_extension(imageName);
    std::string baseName = imageName.substr(0, imageName.length() - extension.length());
    std::string maskName = baseName + "_mask" + extension;

    return maskName;
}

std::string FileUtils::generateRandomString(size_t length)
{
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string randomStr;
    for (size_t i = 0; i < length; ++i)
    {
        randomStr += chars[distribution(generator)];
    }

    return randomStr;
}

bool FileUtils::directoryExists(const std::string &parent, const std::string &sub)
{
    // Build the full path
    std::string path = Glib::build_filename(parent, sub);

    // Check if the directory already exists
    return Glib::file_test(path, Glib::FILE_TEST_IS_DIR);
}

bool FileUtils::createSubdirectory(const std::string &parent, const std::string &sub)
{
    // Check if the directory already exists
    if (directoryExists(parent, sub))
    {
        // If the directory exists, do nothing
        return true;
    }

    // Build the full path
    std::string path = Glib::build_filename(parent, sub);

    // Create the sub-directory (if it doesn't exist)
    if (mkdir(path.c_str(), 0755) == 0)
    {
        // Directory created successfully
        return true;
    }
    else
    {
        // Check if the failure was because the directory already exists
        if (errno == EEXIST)
        {
            return true; // Directory exists
        }
        else
        {
            // Some other error occurred
            std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
            return false;
        }
    }
}

std::string FileUtils::replaceExtension(const std::string &filename, const std::string &newExtension)
{
    // Find the last occurrence of the dot character
    size_t dotPos = filename.find_last_of('.');

    // If there is no dot, just return the filename with the new extension
    if (dotPos == std::string::npos)
    {
        return filename + "." + newExtension;
    }

    // Replace the existing extension with the new one
    return filename.substr(0, dotPos) + "." + newExtension;
}

bool FileUtils::checkImagesDimensions(std::vector<std::string> images, int width, int height)
{
    for (const std::string &imagePath : images)
    {
        try
        {
            // Load the image into a Gdk::Pixbuf
            auto pixbuf = Gdk::Pixbuf::create_from_file(imagePath);

            // Get the width and height of the image
            int actualWidth = pixbuf->get_width();
            int actualHeight = pixbuf->get_height();

            // Check if the width and height are 512
            if (actualWidth != width || actualHeight != height)
            {
                std::cerr << "Image " << imagePath << " has incorrect dimensions: "
                          << width << "x" << height << std::endl;
                return false; // Return false if any image does not match
            }
        }
        catch (const Glib::FileError &e)
        {
            std::cerr << "File error: " << e.what() << std::endl;
            return false;
        }
        catch (const Gdk::PixbufError &e)
        {
            std::cerr << "Pixbuf error: " << e.what() << std::endl;
            return false;
        }
    }

    return true;
}

bool FileUtils::checkImagesHaveMasks(std::vector<std::string> images, std::vector<std::string> masks)
{
    for (const std::string &imagePath : images)
    {
        // Get the filename without extension
        std::filesystem::path imageFile(imagePath);
        std::string imageStem = imageFile.stem();           // Filename without extension
        std::string imageExtension = imageFile.extension(); // Get the extension (e.g., .png)

        // Generate the corresponding mask filename
        std::string expectedMask = imageStem + "_mask" + imageExtension;

        // Check if the mask exists in m_selectedMasks
        auto it = std::find_if(masks.begin(), masks.end(),
                               [&expectedMask](const std::string &maskPath)
                               {
                                   return std::filesystem::path(maskPath).filename() == expectedMask;
                               });

        if (it == masks.end())
        {
            std::cerr << "Mask for image " << imagePath << " not found. Expected: "
                      << expectedMask << std::endl;
            return false; // If the mask isn't found, return false
        }
    }

    // All images have corresponding masks
    return true;
}