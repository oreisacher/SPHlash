//
// Created by ruut on 6/6/26.
//

#ifndef SPHLASH_TEXTURE_H
#define SPHLASH_TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include <string>

class Texture {
public:
    enum Filter {
        NEAREST,
        LINEAR
    };

    enum Wrap {
        REPEAT,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE
    };

    Texture(const std::string& path);
    ~Texture();

    void bind() const;

    const int& getWidth() const;
    void setWidth(const int& width);

    const int& getHeight() const;
    void setHeight(const int& height);

    const int& getNrChannels() const;
    void setNrChannels(const int& nrChannels);

    const bool& isTransparent() const;
    void setTransparent(const bool& transparent);

    const Filter& getFilter() const;
    void setFilter(const Filter& filter);

    const Wrap& getWrap() const;
    void setWrap(const Wrap& wrap);

protected:
    int width = 0;
    int height = 0;
    int nrChannels = 0;
    bool transparent = false;

    Filter filter = NEAREST;
    Wrap wrap = REPEAT;

    unsigned int textureID = 0;
};

#endif //SPHLASH_TEXTURE_H
