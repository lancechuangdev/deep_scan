#include "fileutils.h"

std::string FileUtils::getCssFilePath()
{
    const std::filesystem::path dev_path = "../style.css";
    const std::filesystem::path install_path = "/usr/local/share/deep-scan/style.css";

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
        std::cerr << "CSS file not found!" << std::endl;
        return "";
    }
}

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
    std::vector<std::string> files;
    Glib::Dir dir(folder_path);

    // Supported image file extensions
    std::vector<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".bmp"};

    // Iterate through files in the folder
    for (const auto &file : dir)
    {
        std::string file_path = folder_path + "/" + file;

        // Get the file extension by extracting the base name and finding the dot
        std::string basename = Glib::path_get_basename(file);

        std::string::size_type idx = basename.rfind('.');

        if (idx != std::string::npos)
        {
            std::string extension = basename.substr(idx); // Extract extension
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            // Check if the extension matches a supported image format
            if (std::find(image_extensions.begin(), image_extensions.end(), extension) != image_extensions.end())
            {
                files.push_back(file_path);
            }
        }
    }

    // Sort files alphabetically by filename
    std::sort(files.begin(), files.end());

    return files;
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

bool FileUtils::createFile(const std::string& path)
{
    try
    {
        // Create the file object
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
        
        // Check if the file already exists
        if (file->query_exists())
        {
            std::cout << "File already exists: " << path << std::endl;
            return true;
        }

        // Get the parent directory of the file
        Glib::RefPtr<Gio::File> parentDir = file->get_parent();
        
        // Check if the parent directory exists
        if (!parentDir->query_exists())
        {
            // Create the directory and any missing parent directories
            parentDir->make_directory_with_parents();
        }

        // Now create the file
        Glib::RefPtr<Gio::FileOutputStream> outputStream = file->create_file();
        if (outputStream)
        {
            std::cout << "File created: " << path << std::endl;
            return true;
        }
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "Error creating file: " << ex.what() << std::endl;
    }

    return false;
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

bool FileUtils::checkImagesAspectRatios(const std::vector<std::string> &images, int targetWidth, int targetHeight)
{
    // Calculate the target aspect ratio
    double targetAspectRatio = static_cast<double>(targetWidth) / targetHeight;

    for (const std::string &imagePath : images)
    {
        try
        {
            // Load the image into a Gdk::Pixbuf
            auto pixbuf = Gdk::Pixbuf::create_from_file(imagePath);

            // Get the width and height of the image
            int actualWidth = pixbuf->get_width();
            int actualHeight = pixbuf->get_height();

            // Calculate the actual aspect ratio
            double actualAspectRatio = static_cast<double>(actualWidth) / actualHeight;

            // Check if the aspect ratios are different (allowing for small floating-point precision differences)
            if (std::abs(actualAspectRatio - targetAspectRatio) > 1e-6)
            {
                std::cerr << "Image " << imagePath << " has an incorrect aspect ratio: "
                          << actualWidth << "x" << actualHeight
                          << " (expected aspect ratio: " << targetAspectRatio << ")" << std::endl;
                return false; // Return false if any image does not match the aspect ratio
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

        // Generate possible mask filenames
        std::string expectedMask1 = imageStem + imageExtension;       // Same name as image
        std::string expectedMask2 = imageStem + "_mask" + imageExtension; // With _mask suffix

        // Check if the mask exists in m_selectedMasks
        auto it = std::find_if(masks.begin(), masks.end(),
                               [&expectedMask1, &expectedMask2](const std::string &maskPath)
                               {
                                   std::string maskFilename = std::filesystem::path(maskPath).filename();
                                   return maskFilename == expectedMask1 || maskFilename == expectedMask2;
                               });

        if (it == masks.end())
        {
            std::cerr << "Mask for image " << imagePath << " not found" << std::endl;
            return false; // If the mask isn't found, return false
        }
    }

    // All images have corresponding masks
    return true;
}

bool FileUtils::createTrainingDatasetDirs(const std::string &destination,
                                          std::filesystem::path &trainImagesPath,
                                          std::filesystem::path &trainMasksPath,
                                          std::filesystem::path &valImagesPath,
                                          std::filesystem::path &valMasksPath,
                                          std::filesystem::path &testImagesPath,
                                          std::filesystem::path &testMasksPath)
{
    // Create directories for train, val, and test if they don't exist
    trainImagesPath = std::filesystem::path(destination) / "train" / "images";
    trainMasksPath = std::filesystem::path(destination) / "train" / "masks";
    valImagesPath = std::filesystem::path(destination) / "val" / "images";
    valMasksPath = std::filesystem::path(destination) / "val" / "masks";
    testImagesPath = std::filesystem::path(destination) / "test" / "images";
    testMasksPath = std::filesystem::path(destination) / "test" / "masks";

    if (!std::filesystem::exists(trainImagesPath) && !std::filesystem::create_directories(trainImagesPath))
    {
        return false;
    }

    if (!std::filesystem::exists(trainMasksPath) && !std::filesystem::create_directories(trainMasksPath))
    {
        return false;
    }

    if (!std::filesystem::exists(valImagesPath) && !std::filesystem::create_directories(valImagesPath))
    {
        return false;
    }

    if (!std::filesystem::exists(valMasksPath) && !std::filesystem::create_directories(valMasksPath))
    {
        return false;
    }

    if (!std::filesystem::exists(testImagesPath) && !std::filesystem::create_directories(testImagesPath))
    {
        return false;
    }

    if (!std::filesystem::exists(testMasksPath) && !std::filesystem::create_directories(testMasksPath))
    {
        return false;
    }

    return true;
}

bool FileUtils::splitAndCopyImagesAndMasks(std::vector<std::string> images, std::vector<std::string> masks, const std::string &destination, double trainRatio, double valRatio, double testRatio)
{
    // Create directories for train, val, and test if they don't exist
    std::filesystem::path trainImagesPath, trainMasksPath;
    std::filesystem::path valImagesPath, valMasksPath;
    std::filesystem::path testImagesPath, testMasksPath;

    if (!createTrainingDatasetDirs(destination, trainImagesPath, trainMasksPath, valImagesPath, valMasksPath, testImagesPath, testMasksPath))
    {
        return false;
    }

    // Combine images and masks into pairs for easier shuffling
    std::vector<std::pair<std::string, std::string>> imageMaskPairs;
    for (size_t i = 0; i < images.size(); ++i)
    {
        imageMaskPairs.emplace_back(images[i], masks[i]);
    }

    // Shuffle the pairs
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(imageMaskPairs.begin(), imageMaskPairs.end(), g);

    // Calculate the indices for splitting
    size_t totalCount = imageMaskPairs.size();
    size_t trainCount = static_cast<size_t>(trainRatio * totalCount);
    size_t valCount = static_cast<size_t>(valRatio * totalCount);

    // Copy to train, val, and test directories
    for (size_t i = 0; i < totalCount; ++i)
    {
        const auto &pair = imageMaskPairs[i];
        const std::string &imagePath = pair.first;
        const std::string &maskPath = pair.second;

        if (i < trainCount)
        {
            if (!std::filesystem::copy_file(imagePath, (trainImagesPath / std::filesystem::path(imagePath).filename()), std::filesystem::copy_options::overwrite_existing) ||
                !std::filesystem::copy_file(maskPath, (trainMasksPath / std::filesystem::path(maskPath).filename()), std::filesystem::copy_options::overwrite_existing))
            {
                return false;
            }
        }
        else if (i < trainCount + valCount)
        {
            if (!std::filesystem::copy_file(imagePath, (valImagesPath / std::filesystem::path(imagePath).filename()), std::filesystem::copy_options::overwrite_existing) ||
                !std::filesystem::copy_file(maskPath, (valMasksPath / std::filesystem::path(maskPath).filename()), std::filesystem::copy_options::overwrite_existing))
            {
                return false;
            }
        }
        else
        {
            if (!std::filesystem::copy_file(imagePath, (testImagesPath / std::filesystem::path(imagePath).filename()), std::filesystem::copy_options::overwrite_existing) ||
                !std::filesystem::copy_file(maskPath, (testMasksPath / std::filesystem::path(maskPath).filename()), std::filesystem::copy_options::overwrite_existing))
            {
                return false;
            }
        }
    }

    return true;
}