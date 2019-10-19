//
//  main.cpp
//  3DGE _SDL2
//
//  Created by Михаил Ковалевский on 19.10.2019.
//  Copyright © 2019 Mikhail Kavaleuski. All rights reserved.
//

#include <iostream>
#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#include <SDL.h>
#elif __APPLE__
	//define something for MacOS
	#include <SDL2/SDL.h>
#endif

const int FRAMES_PER_SECOND = 30;
int WIDTH = 800, HEIGHT = 600;


int check_window(SDL_Window *window){
    if ( window == NULL )
    {
        printf( "Error creating window! SDL_Error: %s\n", SDL_GetError() );
        return 1;
    } else {
        printf( "Created window(%dx%d)!\n", WIDTH, HEIGHT);
        return 0;
    }
}

void Axis_XY(SDL_Renderer *renderer){
    static int kw=0;
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, 0, HEIGHT/2, WIDTH, HEIGHT/2);
    SDL_RenderDrawLine(renderer, WIDTH/2, 0, WIDTH/2, HEIGHT);
    
    if(kw==0){
        printf("Created axises!\n");
        kw=1;
    }
}





int main(int argc, char *argv[])
{
    SDL_Init( SDL_INIT_VIDEO );
    
    SDL_Window *window;
    
    window = SDL_CreateWindow(
                              "^_^",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WIDTH,
                              HEIGHT,
                              SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
                              );
    
    // Check that the window was successfully created
    if(check_window(window)==1){
        return 1;
    };
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    
    Uint32 start;
    SDL_Event windowEvent;
    while ( true )
    {
        if ( SDL_PollEvent( &windowEvent ) )
        {
            if ( SDL_QUIT == windowEvent.type )
            {
                break;
            }
        }
        start = SDL_GetTicks();
        // Gets real size of the window(Fix for MacOS/Resizing)
        SDL_GetRendererOutputSize(renderer, &WIDTH, &HEIGHT);
        
        //Background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        //Axises
        Axis_XY(renderer);
        
        // Renders window
        SDL_RenderPresent(renderer);
        if (1000/FRAMES_PER_SECOND > SDL_GetTicks()-start) {
            SDL_Delay(1000/FRAMES_PER_SECOND - (SDL_GetTicks()-start));
        }
    }
    
    
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}






