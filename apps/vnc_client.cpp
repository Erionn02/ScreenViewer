#include <rfb/rfbclient.h>
#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
bool frameUpdated = false;

int windowWidth = 800;
int windowHeight = 600;
                    // client, x, y, w, h
void updateRectangle(rfbClient*, int , int , int , int ) {
    frameUpdated = true;
}

void handleWindowEvent(SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT) {
        switch (event->window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                windowWidth = event->window.data1;
                windowHeight = event->window.data2;
                break;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    rfbClient *client = rfbGetClient(8, 3, 4);
    if (!client) {
        return 1;
    }

    client->canHandleNewFBSize = FALSE;
    client->GotFrameBufferUpdate = updateRectangle;

    if (!rfbInitClient(client, &argc, argv)) {
        fprintf(stderr, "Failed to connect to VNC server\n");
        return 1;
    }


    window = SDL_CreateWindow("VNC Client",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              windowWidth, windowHeight,
                              SDL_WINDOW_SHOWN  | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        rfbClientCleanup(client);
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        rfbClientCleanup(client);
        SDL_Quit();
        return 1;
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                client->width, client->height);
    if (!texture) {
        fprintf(stderr, "Could not create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        rfbClientCleanup(client);
        SDL_Quit();
        return 1;
    }

    fprintf(stderr, "Connected to VNC server\n");


    while (1) {
        SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight);
        spdlog::info("Width: {}, height: {}", windowWidth, windowHeight);
        if (WaitForMessage(client, 100000) < 0) {
            fprintf(stderr, "WaitForMessage failed\n");
            break;
        }
        if (!HandleRFBServerMessage(client)) {
            fprintf(stderr, "HandleRFBServerMessage failed\n");
            break;
        }

        if (frameUpdated) {
            SDL_UpdateTexture(texture, NULL, client->frameBuffer, client->width * 4);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            frameUpdated = false;
        }


        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(1);
            }
            handleWindowEvent(&event);
        }
    }
    rfbClientCleanup(client);
    return 0;
}