#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include "GuiApp.hpp"

void GlfwError(int code, const char* description)
{
    std::fprintf(stderr, "GLFW (%d): %s\n", code, description);
}

int main(int, char** argv)
{
    @autoreleasepool
    {
        glfwSetErrorCallback(GlfwError);
        if (!glfwInit()) { return 1; }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(1180, 800, "SparseVector — Лабораторная работа 2", nullptr, nullptr);
        if (window == nullptr) { glfwTerminate(); return 1; }
        glfwSetWindowSizeLimits(window, 840, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (device == nil || queue == nil)
        {
            std::fprintf(stderr, "Metal is unavailable\n");
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0;
        style.FrameRounding = 5;
        style.GrabRounding = 5;
        style.ChildRounding = 6;
        style.FramePadding = ImVec2(10, 7);
        style.ItemSpacing = ImVec2(12, 10);
        style.WindowPadding = ImVec2(20, 16);
        style.FontScaleDpi = ImGui_ImplGlfw_GetContentScaleForWindow(window);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.085f, 0.105f, 1);
        style.Colors[ImGuiCol_Header] = ImVec4(0.16f, 0.25f, 0.37f, 1);
        style.Colors[ImGuiCol_Button] = ImVec4(0.17f, 0.29f, 0.44f, 1);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.23f, 0.39f, 0.58f, 1);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.46f, 0.69f, 1);

        int exitCode = 0;
        bool platformReady = ImGui_ImplGlfw_InitForOther(window, true);
        bool rendererReady = platformReady && ImGui_ImplMetal_Init(device);
        try
        {
            if (!rendererReady) { throw std::runtime_error("Cannot initialize ImGui backends"); }
            std::filesystem::path executable = std::filesystem::canonical(argv[0]);
            std::filesystem::path font = executable.parent_path() / "../gui/assets/DroidSans.ttf";
            NSString* bundle = [[NSBundle mainBundle] bundlePath];
            if ([bundle hasSuffix:@".app"])
            {
                font = std::filesystem::path([[[NSBundle mainBundle] resourcePath] fileSystemRepresentation]) / "DroidSans.ttf";
                std::filesystem::path directory = std::filesystem::path([bundle fileSystemRepresentation]).parent_path();
                if (directory.filename() == "build" && std::filesystem::is_directory(directory.parent_path() / "lab2"))
                {
                    directory = directory.parent_path();
                }
                std::filesystem::current_path(directory);
            }
            if (!std::filesystem::exists(font) || io.Fonts->AddFontFromFileTTF(font.c_str(), 17.0f) == nullptr)
            {
                throw std::runtime_error("Cannot load gui/assets/DroidSans.ttf");
            }

            NSWindow* nativeWindow = glfwGetCocoaWindow(window);
            CAMetalLayer* layer = [CAMetalLayer layer];
            layer.device = device;
            layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            nativeWindow.contentView.layer = layer;
            nativeWindow.contentView.wantsLayer = YES;
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];
            glfwShowWindow(window);
            glfwFocusWindow(window);

            GuiApp app;
            while (!app.ShouldClose())
            {
                @autoreleasepool
                {
                    glfwPollEvents();
                    if (glfwWindowShouldClose(window))
                    {
                        app.RequestClose();
                        glfwSetWindowShouldClose(window, GLFW_FALSE);
                    }
                    if (app.ShouldClose()) { break; }
                    int width = 0, height = 0;
                    glfwGetFramebufferSize(window, &width, &height);
                    if (width == 0 || height == 0) { glfwWaitEventsTimeout(0.05); continue; }
                    layer.drawableSize = CGSizeMake(width, height);
                    id<CAMetalDrawable> drawable = [layer nextDrawable];
                    if (drawable == nil) { glfwWaitEventsTimeout(0.05); continue; }
                    id<MTLCommandBuffer> command = [queue commandBuffer];
                    if (command == nil) { throw std::runtime_error("Cannot create a Metal command buffer"); }
                    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
                    pass.colorAttachments[0].texture = drawable.texture;
                    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
                    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
                    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.075, 0.085, 0.105, 1);
                    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:pass];
                    if (encoder == nil) { throw std::runtime_error("Cannot create a Metal render encoder"); }
                    ImGui_ImplMetal_NewFrame(pass);
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();
                    app.Draw();
                    ImGui::Render();
                    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), command, encoder);
                    [encoder endEncoding];
                    [command presentDrawable:drawable];
                    [command commit];
                }
            }
        }
        catch (const std::exception& exception)
        {
            std::fprintf(stderr, "%s\n", exception.what());
            NSAlert* alert = [[NSAlert alloc] init];
            alert.messageText = @"Не удалось запустить SparseVector";
            alert.informativeText = [NSString stringWithUTF8String:exception.what()];
            [alert runModal];
            exitCode = 1;
        }
        if (rendererReady) { ImGui_ImplMetal_Shutdown(); }
        if (platformReady) { ImGui_ImplGlfw_Shutdown(); }
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return exitCode;
    }
}
