// Usage:
// tag <audio_file>... <image_file>   (image: .jpg .jpeg .png)


#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/tvariant.h>
#include <taglib/tbytevector.h>
#include <taglib/tstring.h>
#include <taglib/tlist.h>

#include <glob.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

std::string getLowerExtension(const std::string &path)
{
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    std::string ext = path.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

std::string getMimeType(const std::string &ext)
{
    if (ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    }
    if (ext == "png") {
        return "image/png";
    }
    return "";
}

bool readFileBytes(const std::string &path, std::vector<char> &outData)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    std::streamsize size = file.tellg();
    if (size <= 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    outData.resize(static_cast<size_t>(size));
    if (!file.read(outData.data(), size)) {
        return false;
    }
    return true;
}

void expandArgument(const std::string &arg, std::vector<std::string> &out)
{
    glob_t results;
    int rc = glob(arg.c_str(), GLOB_NOCHECK, nullptr, &results);
    if (rc == 0) {
        for (size_t i = 0; i < results.gl_pathc; ++i) {
            out.emplace_back(results.gl_pathv[i]);
        }
    } else {
        out.push_back(arg);
    }
    globfree(&results);
}

enum class TagResult { Ok, Skipped, Failed };

TagResult tagFile(const std::string &audioPath,
                  const TagLib::ByteVector &pictureBytes,
                  const std::string &mimeType)
{
    TagLib::FileRef fileRef(audioPath.c_str());
    if (fileRef.isNull() || !fileRef.file()) {
        std::cerr << "Skipped (not a supported audio file): " << audioPath << std::endl;
        return TagResult::Skipped;
    }

    TagLib::VariantMap picture;
    picture.insert("data", pictureBytes);
    picture.insert("mimeType", TagLib::String(mimeType));
    picture.insert("description", TagLib::String("Cover"));
    picture.insert("pictureType", TagLib::String("Front Cover"));

    TagLib::List<TagLib::VariantMap> pictures;
    pictures.append(picture);

    if (!fileRef.file()->setComplexProperties("PICTURE", pictures)) {
        std::cerr << "Error: File format doesnt support cover art, or image was rejected: "
                  << audioPath << std::endl;
        return TagResult::Failed;
    }

    if (!fileRef.save()) {
        std::cerr << "Error: Cannot write changes to: " << audioPath << std::endl;
        return TagResult::Failed;
    }

    std::cout << "Success. Cover art added to: " << audioPath << std::endl;
    return TagResult::Ok;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <audio_file>... <image_file>" << std::endl;
        return 1;
    }

    const std::string imagePath = argv[argc - 1];

    std::string ext = getLowerExtension(imagePath);
    std::string mimeType = getMimeType(ext);
    if (mimeType.empty()) {
        std::cerr << "Error: Image type is not supported. Supported types: jpg png" << std::endl;
        std::cerr << "Note: The image must be the LAST argument." << std::endl;
        return 1;
    }

    std::vector<char> imageData;
    if (!readFileBytes(imagePath, imageData)) {
        std::cerr << "Error: Cannot read image file: " << imagePath << std::endl;
        return 1;
    }

    TagLib::ByteVector pictureBytes(imageData.data(),
                                    static_cast<unsigned int>(imageData.size()));

    std::vector<std::string> audioPaths;
    for (int i = 1; i < argc - 1; ++i) {
        expandArgument(argv[i], audioPaths);
    }

    int okCount = 0;
    int skipCount = 0;
    int failCount = 0;

    for (const std::string &path : audioPaths) {
        std::error_code ec;

        if (fs::is_directory(path, ec)) {
            continue;
        }

        if (fs::equivalent(path, imagePath, ec)) {
            continue;
        }

        switch (tagFile(path, pictureBytes, mimeType)) {
            case TagResult::Ok:      ++okCount;   break;
            case TagResult::Skipped: ++skipCount; break;
            case TagResult::Failed:  ++failCount; break;
        }
    }

    if (audioPaths.size() > 1) {
        std::cout << "Done. Tagged: " << okCount
                  << ", skipped: " << skipCount
                  << ", failed: " << failCount << std::endl;
    }

    return failCount > 0 ? 1 : 0;
}
