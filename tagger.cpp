// Usage:
//   album_cover_tagger <audio_file> <image_file>

#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/tvariant.h>
#include <taglib/tbytevector.h>
#include <taglib/tstring.h>
#include <taglib/tlist.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

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

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <audio_file> <image_file>" << std::endl;
        return 1;
    }

    const std::string audioPath = argv[1];
    const std::string imagePath = argv[2];

    std::string ext = getLowerExtension(imagePath);
    std::string mimeType = getMimeType(ext);

    if (mimeType.empty()) {
        std::cerr << "Error: Image type is not supported. Supported types: jpg png" << std::endl;
        return 1;
    }

    std::vector<char> imageData;
    if (!readFileBytes(imagePath, imageData)) {
        std::cerr << "Error: Cannot read image file: " << imagePath << std::endl;
        return 1;
    }

    TagLib::FileRef fileRef(audioPath.c_str());
    if (fileRef.isNull() || !fileRef.file()) {
        std::cerr << "Error: Cannot open audio file: " << audioPath << std::endl;
        return 1;
    }

    TagLib::ByteVector pictureBytes(imageData.data(),
                                     static_cast<unsigned int>(imageData.size()));

    TagLib::VariantMap picture;
    picture.insert("data", pictureBytes);
    picture.insert("mimeType", TagLib::String(mimeType));
    picture.insert("description", TagLib::String("Cover"));
    picture.insert("pictureType", TagLib::String("Front Cover"));

    TagLib::List<TagLib::VariantMap> pictures;
    pictures.append(picture);

    bool ok = fileRef.file()->setComplexProperties("PICTURE", pictures);
    if (!ok) {
        std::cerr << "Error: File format doesnt support cover art, or image was rejected" << std::endl;
        return 1;
    }

    if (!fileRef.save()) {
        std::cerr << "Error: Cannot write changes to: " << audioPath << std::endl;
        return 1;
    }

    std::cout << "Success. Cover art added to: " << audioPath << std::endl;
    return 0;
}
