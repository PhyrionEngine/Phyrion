#include "Engine.hpp"
#include "Fyrion/Core/Logger.hpp"
#include "Fyrion/Platform/PlatformTypes.hpp"
#include "Fyrion/Platform/Platform.hpp"
#include "Fyrion/Graphics/GraphicsTypes.hpp"
#include "Fyrion/Graphics/Graphics.hpp"
#include "TypeRegister.hpp"
#include "Fyrion/ImGui/ImGui.hpp"
#include "Fyrion/Resource/ResourceAssets.hpp"
#include "Fyrion/IO/FileSystem.hpp"
#include "Fyrion/IO/Path.hpp"

namespace Fyrion
{
    void PlatformInit();
    void PlatformShutdown();

    void            GraphicsInit();
    void            GraphicsCreateDevice(GPUAdapter adapter);
    RenderCommands& GraphicsBeginFrame();
    void            GraphicsEndFrame(Swapchain swapchain);
    void            GraphicsShutdown();

    void RegistryShutdown();
    void EventShutdown();

    void RepositoryInit();
    void RepositoryShutdown();

    void ResourceAssetsInit();
    void ResourceAssetsShutdown();

    void RegisterAssets();


    namespace
    {
        Logger&     logger = Logger::GetLogger("Fyrion::Engine");
        bool        running = true;
        Window      window{};
        Swapchain   swapchain{};
        Vec4        clearColor = Vec4{0, 0, 0, 1};
        f64         lastTime{};
        f64         deltaTime{};
        u64         frame{0};
    }

    void Engine::Init()
    {
        Init(0, nullptr);
    }

    void Engine::Init(i32 argc, char** argv)
    {
        RepositoryInit();
        TypeRegister();
        ResourceAssetsInit();
        RegisterAssets();
    }

    void Engine::CreateContext(const EngineContextCreation& contextCreation)
    {
        ResourceAssets::LoadAssetsFromDirectory("Fyrion", Path::Join(FileSystem::AssetFolder(), "Fyrion"));

        PlatformInit();

        WindowFlags windowFlags = WindowFlags::None;
        if (contextCreation.maximize)
        {
            windowFlags |= WindowFlags::Maximized;
        }

        if (contextCreation.fullscreen)
        {
            windowFlags |= WindowFlags::Fullscreen;
        }

        GraphicsInit();
        GraphicsCreateDevice(GPUAdapter{});

        window = Platform::CreateWindow(contextCreation.title, contextCreation.resolution, windowFlags);

        swapchain = Graphics::CreateSwapchain(SwapchainCreation{
            .window = window,
            .vsync = true
        });

        ImGui::Init(window, swapchain);
    }

    void Engine::Run()
    {
        logger.Info("Fyrion Engine {} Initialized", FY_VERSION);

        while (running)
        {
            f64 currentTime = Platform::GetTime();
            deltaTime = currentTime - lastTime;
            lastTime  = currentTime;

            Platform::ProcessEvents();

            ImGui::BeginFrame(window, deltaTime);


            if (Platform::UserRequestedClose(window))
            {
                Engine::Shutdown();
                if (running)
                {
                    Platform::SetWindowShouldClose(window, false);
                }
            }

            ImGui::ShowDemoWindow(0);

            Extent extent = Platform::GetWindowExtent(window);

            RenderCommands& cmd = GraphicsBeginFrame();
            cmd.Begin();

            RenderPass renderPass = Graphics::AcquireNextRenderPass(swapchain);

            cmd.BeginRenderPass(BeginRenderPassInfo{
                .renderPass = renderPass,
                .clearValues = {&clearColor, 1}
            });

            ViewportInfo viewportInfo{};
            viewportInfo.x = 0.;
            viewportInfo.y = 0.;
            viewportInfo.width = (f32) extent.width;
            viewportInfo.height = (f32) extent.height;
            viewportInfo.maxDepth = 0.;
            viewportInfo.minDepth = 1.;
            cmd.SetViewport(viewportInfo);
            cmd.SetScissor(Rect{.x= 0, .y = 0, .width = extent.width, .height = extent.height});

            ImGui::Render(cmd);

            cmd.EndRenderPass();
            cmd.End();

            GraphicsEndFrame(swapchain);
            frame++;
        }

        Graphics::DestroySwapchain(swapchain);
        Platform::DestroyWindow(window);

        GraphicsShutdown();
        PlatformShutdown();
    }

    void Engine::Shutdown()
    {
        running = false;
    }

    void Engine::Destroy()
    {
        ResourceAssetsShutdown();
        RepositoryShutdown();
        RegistryShutdown();
        EventShutdown();
    }
}