#include <iostream>

#include "Camera.h"
#include "vkRender.h"

#include "vkApp.h"

vkApp::vkApp()
{
    init(m_width, m_height);
}

vkApp::vkApp(int width, int height) : m_width(width), m_height(height)
{
    init(width, height);
}

vkApp::~vkApp()
{
}

int vkApp::init(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Could not initialize SDL." << std::endl;
        return -1;
    }
    m_pWindow = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_VULKAN);
    if (m_pWindow == NULL) {
        std::cout << "Could not create SDL window." << std::endl;
        return - 1;
    }
    SDL_SetWindowResizable(m_pWindow, SDL_TRUE);

    m_pCamera = std::make_shared<Camera>(100.0f);
    m_pRender = std::make_unique<vkRender>(m_pWindow, m_pCamera, width, height);

    return 0;
}

void vkApp::resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    m_pRender->resizeWindow(width, height);
}

int vkApp::run()
{
    if (m_pRender) {

        bool stillRunning = true;
        while (stillRunning) {

            SDL_Event event;
            while (SDL_PollEvent(&event)) {

                switch (event.type) {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        resize(event.window.data1, event.window.data2);
                    }
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if (event.key.state == 0 && event.key.keysym.sym == SDLK_ESCAPE) 
                        stillRunning = false;
                    else if (event.key.state == 1) {
                        switch (event.key.keysym.sym) {
                        case SDLK_w:
                            m_pCamera->forward();
                            break;
                        case SDLK_s:
                            m_pCamera->backward();
                            break;
                        case SDLK_a:
                            m_pCamera->left();
                            break;
                        case SDLK_d:
                            m_pCamera->right();
                            break;
                        }
                    }
                    //printf("type:%d, state:%d, scan_code:%d, syn:0x%x, mode:0x%x, repeat:%d\n", event.key.type, event.key.state, event.key.keysym.scancode, event.key.keysym.sym, event.key.keysym.mod, event.key.repeat);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    //Button down state: 1, up state: 1->:0
                    //Button: 1: Left, 2: Middle, 3: Right
                    //printf("type:%d, state:%d, button:%d, x:%d, y:%d, clicks:%d\n", event.button.type, event.button.state, event.button.button, event.button.x, event.button.y, event.button.clicks);
                    break;
                case SDL_MOUSEMOTION:
                    //State: 1: Left, 2: Middle, 4: Right
                    if (event.motion.state == 1) {
                        //Button down
                        m_pCamera->rotate(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, m_width, m_height);
                        //std::cout << glm::to_string(mat) << std::endl;
                        //printf("type:%d, state:%d, x:%d, y:%d, xrel:%d, yrel:%d\n", event.motion.type, event.motion.state, event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
                    } else if (event.motion.state == 4) {
                        m_pCamera->translate(event.motion.xrel, event.motion.yrel);
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    //wheel up, y+, wheel down, y-
                    //printf("type:%d, x:%d, y:%d, direction:%d\n", event.wheel.type, event.wheel.x, event.wheel.y, event.wheel.direction);
                    m_pCamera->zoom(event.wheel.y);
                    break;
                case SDL_QUIT:
                    stillRunning = false;
                    break;

                default:
                    // Do nothing.
                    break;
                }
            }
            m_pRender->drawFrame();

            SDL_Delay(10);
        }
        m_pRender->waitIdle();

    }
    clean();

    return 0;
}

void vkApp::clean()
{
    m_pRender.reset();

    SDL_DestroyWindow(m_pWindow);
    SDL_Quit();
}

