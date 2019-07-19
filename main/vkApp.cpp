#include "vkApp.h"
#include "vkRender.h"

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

    m_pRender = std::make_unique<vkRender>(m_pWindow, width, height);

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
                case SDL_MOUSEBUTTONDOWN:
                    break;
                case SDL_MOUSEMOTION:
                    break;
                case SDL_MOUSEWHEEL:
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

