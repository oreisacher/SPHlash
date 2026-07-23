//
// Created by ruut on 6/6/26.
//

#ifndef SPHLASH_TEXTURE_H
#define SPHLASH_TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include <string>

class Texture {
public:
    Texture(const std::string& path);
    ~Texture();

    void bind() const;
    unsigned int getId() const;

    const int& getWidth() const;
    void setWidth(const int& width);

    const int& getHeight() const;
    void setHeight(const int& height);

    const int& getNrChannels() const;
    void setNrChannels(const int& nrChannels);

protected:
    int width = 0;
    int height = 0;
    int nrChannels = 0;

    unsigned int textureID = 0;
};

#endif //SPHLASH_TEXTURE_H
