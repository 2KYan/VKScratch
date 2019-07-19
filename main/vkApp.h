#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include "vkRender.h"

class vkApp
{
public:
    vkApp();
    vkApp(int width, int height);
    virtual ~vkApp();

    vkApp(vkApp const &) = delete;
    vkApp& operator=(vkApp const&) = delete;

public:
    int run();
    void resize(uint32_t width, uint32_t height);

protected:
    int init(int width, int height);
    void clean();

protected:
    int m_width = 1200;
    int m_height = 960;

    SDL_Window* m_pWindow;
    std::unique_ptr<vkRender> m_pRender;
};

