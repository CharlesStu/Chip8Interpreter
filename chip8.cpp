#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <cstdlib>
#include <time.h>

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
const int WIDTH = 64, HEIGHT = 32;

unsigned char font[80] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, //0
        0x20, 0x60, 0x20, 0x20, 0x70, //1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
        0x90, 0x90, 0xF0, 0x10, 0x10, //4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
        0xF0, 0x10, 0x20, 0x40, 0x40, //7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
        0xF0, 0x90, 0xF0, 0x90, 0x90, //A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
        0xF0, 0x80, 0x80, 0x80, 0xF0, //C
        0xE0, 0x90, 0x90, 0x90, 0xE0, //D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
        0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

uint8_t keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v,
};

bool SDL_init();
void SDL_kill();

class chip8
{
public:
    uint8_t memory[4096]; //memory
    uint16_t PC;          // program counter
    uint16_t I;           // index register
    uint16_t stack[16] = {0};   // stack for return addresses on branches
    uint16_t SP;          //stack pointer for top of stack
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t v_reg[16];
    uint16_t opcode;
    uint8_t keys[16];
    //bool v_reg[0xF];
    bool display[64 * 32];

    void init()
    {
        std::cout << "initializing" << std::endl;
        PC = 0x200;
        I = 0;
        delay_timer = 0;
        sound_timer = 0;
        SP = 0;

        for (int i = 0; i < 4096; i++)
        {
            memory[i] = 0;
        }
        for (int i = 0; i < 80; i++)
        {
            memory[i + 0x50] = font[i];
        }
        for (int i = 0; i< 16; i++){
            keys[i] = 0;
        }
        for (int i = 0; i < 64 * 32; i++)
        {
                    display[i] = 0;
        }
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

    }
    bool memory_load()
    {
        //get size of the file
        struct stat result;
        int size;
        if (stat("TETRIS", &result) == 0)
        {
            size = result.st_size;
            std::cout << size;
        }
        else
        {
            std::cout << "can't read the ROM size";
        }
        //open the file
        std::ifstream ROM("TETRIS", std::ios::in | std::ios::binary);
        if (ROM.is_open())
        {
            std::cout << "byte ROM being read" << std::endl;
            int i = 0x200;
            char *buffer;
            ROM.read((char *)&memory[512], size);
            ROM.close();
        }
        else
        {
            std::cout << "can't open the ROM";
            return 0;
        }
        return 1;
    }
    void printDisplay()
    {
        for (int i = 0; i < 32 * 64; i++)
        {
            if (i % 64 == 0)
            {
                std::cout << std::endl;
            }
            std::cout << display[i];
        }
    }
    void emulate()
    {
        //fetch
        opcode = memory[PC] << 8 | memory[PC + 1];
        std::cout<< PC<< std::endl;
        int opcodeprint = opcode;
        std::cout<< std::hex << opcodeprint << std::endl;
        PC = PC + 2;
        //execute
        switch (opcode & 0xF000)
        {
        case 0x0000:
            switch (opcode & 0x00FF)
            {
            case 0x00E0:
                std::cout << "clear screen" << std::endl;
                for (int i = 0; i < 64 * 32; i++)
                {
                    display[i] = 0;
                }
                SDL_SetRenderTarget(renderer, texture);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                break;
            case 0x00EE:
                std::cout << "return" << std::endl;
                for (int i = 0; i < 16; i++){
                    std::cout << stack [i] << std::endl;
                }
                PC = stack[SP - 1];
                stack[SP - 1] = 0;
                SP = SP - 1;
                break;
            case 0x0000:
                std::cout << "end of file" << std::endl;
                break;
            }
            break;
        case 0x1000:
            //std::cout << "jump" << std::endl;
            PC = (opcode & 0xFFF);
            break;
        case 0x2000: //subroutine call
            std::cout << "subroutine call" << std::endl;
            stack[SP] = PC;
            SP++;
            PC = (opcode & 0xFFF);
            break;
        case 0x3000: //skip if equal (reg,constant)
            std::cout << "skip if register" << ((opcode & 0x0F00) >> 8) << " "<< int(v_reg[((opcode & 0x0F00) >> 8)])<< "equals" << (opcode & 0x00FF) <<std::endl;
            if (v_reg[((opcode & 0x0F00) >> 8)] == (opcode & 0x00FF))
            {
                PC = PC + 2;
            }
            break;
        case 0x4000: //skip is not equal(reg, constant)
            std::cout << "skip if not equal" << std::endl;
            if (v_reg[((opcode & 0x0F00) >> 8)] != (opcode & 0x00FF))
            {
                PC = PC + 2;
            }
            break;
        case 0x5000: //skip if equal (reg,reg)
            std::cout << "skip equal reg" << std::endl;
            if (v_reg[((opcode & 0x0F00) >> 8)] == v_reg[((opcode & 0x00F0) >> 4)])
            {
                PC = PC + 2;
            }
            break;
        case 0x6000: //set reg
            std::cout << "set reg" << std::endl;
            v_reg[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            //std::cout << ((opcode & 0x0F00) >> 8) << std::endl;
            //std::cout << (opcode & 0x0FF) << std::endl;

            break;
        case 0x7000: //add (constant and reg)
            std::cout << "add constant" << std::endl;
            //std::cout << (opcode ) << std::endl;
            //std::cout << v_reg[((opcode & 0x0F00) >> 8)] << std::endl;

            v_reg[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            //std::cout << v_reg[((opcode & 0x0F00) >> 8)] << std::endl;
            //std::cout <<  (opcode & 0x00FF) << std::endl;
            break;
        case 0x8000:
            switch (opcode & 0x000F)
            {
            //set value 8XY0
            case 0:
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x00F0) >> 4];
                break;
            //binary or 8XY1
            case 1:
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x0F00) >> 8] | v_reg[(opcode & 0x00F0) >> 4];
                break;
            //binary and 8XY2
            case 2:
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x0F00) >> 8] & v_reg[(opcode & 0x00F0) >> 4];
                break;
            //logical xor 8XY3
            case 3:
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x0F00) >> 8] ^ v_reg[(opcode & 0x00F0) >> 4];
                break;
            //add 8XY4
            case 4:
                if (256 > v_reg[(opcode & 0x0F00) >> 8] + v_reg[(opcode & 0x00F0) >> 4])
                {
                    v_reg[0xF] = 0;
                }
                else
                {
                    v_reg[0xF] = 1;
                }
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x0F00) >> 8] + v_reg[(opcode & 0x00F0) >> 4];
                break;
            //subtract 8XY5 (x-y)
            case 5:
                if (0 < v_reg[(opcode & 0x0F00) >> 8] - v_reg[(opcode & 0x00F0) >> 4])
                {
                    v_reg[0xF] = 1;
                }
                else
                {
                    v_reg[0xF] = 0;
                }
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x0F00) >> 8] - v_reg[(opcode & 0x00F0) >> 4];
                break;
            //8XY6 shift right
            case 6:
                v_reg[0xF] = v_reg[(opcode & 0x00F0) >> 4] & 1;
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x00F0) >> 4] >> 1;
                break;
            case 7:
                if (0 < v_reg[(opcode & 0x00F0) >> 4] - v_reg[(opcode & 0x0F00) >> 8])
                {
                    v_reg[0xF] = 1;
                }
                else
                {
                    v_reg[0xF] = 0;
                }
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x00F0) >> 4] - v_reg[(opcode & 0x0F00) >> 8];
                break;
                //8XYe shift left
                case 0xE:
                v_reg[0xF] = (v_reg[(opcode & 0x00F0) >> 4] & 0x8000) >> 15;
                v_reg[(opcode & 0x0F00) >> 8] = v_reg[(opcode & 0x00F0) >> 4] << 1;
                break;
            }  
            break;
        case 0x9000: //skip if not equal (reg,reg)
            std::cout << "skip not equal " << int(v_reg[((opcode & 0x0F00) >> 8)])<<int(v_reg[((opcode & 0x00F0) >> 4)])<<  std::endl;
            if (v_reg[((opcode & 0x0F00) >> 8)] != v_reg[((opcode & 0x00F0) >> 4)])
            {
                std::cout << "skipping" << std::endl;
                PC = PC + 2;
            }
            break;
        case 0xA000: //set index reg
            std::cout << "set index reg" << std::endl;
            std::cout << (opcode) << std::endl;
            I = (opcode & 0x0FFF);
            break;
        case 0xB000: //jump with offset
            std::cout << "jump with offset" << std::endl;
            PC = (opcode & 0xFFF) + v_reg[0];
            break;
        case 0xC000: //random num
            std::cout << "random num" << std::endl;
            v_reg[((opcode & 0x0F00) >> 8)] = rand() & (opcode & 0x00FF);
            break;
        case 0xD000:
        {
            std::cout << "display" << std::endl;
            //std::cout << opcode << std::endl;
            int X = v_reg[((opcode & 0x0F00) >> 8)] % 64;
            int Y = v_reg[((opcode & 0x00F0) >> 4)] % 32;
            std::cout << X << std::endl;
            std::cout << Y << std::endl;
            v_reg[0xF] = 0;
            std::cout << (opcode & 0x000F) << std::endl;
            for (int i = 0; i < (opcode & 0x000F); i++)
            { //number of y lines
                if (i < 31)
                {
                    bool xbits[8];
                    for (int n = 0; n < 8; n++)
                    {
                        xbits[n] = (memory[I + i] >> (7 - n)) & 1;
                        if (X + n < 63)
                        { // dont write to display if past edge
                            if ((display[n + X + (i + Y) * 64] == 1) && xbits[n] == 1)
                            {
                                v_reg[0xF] = 1;
                                display[n + X + (i + Y) * 64] = 0;
                            }
                            else{
                            display[n + X + (i + Y) * 64] = display[n + X + (i + Y) * 64] ^ xbits[n];
                            }
                        }
                    }
                }
            }
            //printDisplay();
        }
        break;
        case 0xE000:
            switch (opcode & 0x00FF){
                case 0x9E:
                    if (keys[v_reg[((opcode & 0x0F00) >> 8)]]){
                        PC += 2;
                    }
                    break;
                case 0xA1:
                    if (keys[v_reg[((opcode & 0x0F00) >> 8)]] == 0){
                        PC += 2;
                    }
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF){
                case 0x07:
                    v_reg[((opcode & 0x0F00) >> 8)] = delay_timer;
                    break;
                case 0x15:
                    delay_timer = v_reg[((opcode & 0x0F00) >> 8)];
                    break;
                case 0x18:
                    sound_timer = v_reg[((opcode & 0x0F00) >> 8)];
                    break;
                case 0x1E:
                    I += v_reg[((opcode & 0x0F00) >> 8)];
                    break;
                case 0x0A:
                    for (int i = 0; i < 16; i++){
                        if (keys[i]){
                            v_reg[((opcode & 0x0F00) >> 8)] = i;
                        }
                        else{
                            PC -= 2;
                        }
                    }
                    break;
                case 0x29:
                    I = (v_reg[((opcode & 0x0F00) >> 8)] * 0x05) + 0x50;
                case 0x33:{
                    std::cout << "FX33" << std::endl;
                    int X = v_reg[((opcode & 0x0F00) >> 8)];
                    std::cout << X << std::endl;
                    memory[I+2] = X % 10;
                    X /= 10;
                    std::cout << X << std::endl;
                    memory[I+1] = X % 10;
                    X /= 10;
                    std::cout << X << std::endl;
                    memory[I] = X % 10;
                    break;
                }
                case 0x55:{
                    //int X = v_reg[((opcode & 0x0F00) >> 8)];
                    int X = ((opcode & 0x0F00) >> 8);
                    for (int i = 0; i < (X + 1); i++){
                        memory[I + i] = v_reg[i];
                    }
                    break;
                }
                case 0x65:{
                    //int X = v_reg[((opcode & 0x0F00) >> 8)];
                    int X = ((opcode & 0x0F00) >> 8);
                    for (int i = 0; i < (X + 1); i++){
                        v_reg[i] = memory[I + i];
                        std::cout << i << std::endl;
                    }
                    break;
                }
            }
            break;
        }
    }
    //}
    void mem_print(int start, int end)
    {
        std::cout << "Printing Memory" << std::endl;
        for (int i = start; i < end; i++)
        {
            int printer = memory[i];
            std::cout << std::hex << printer << std::endl;
        }
    }
};

int main(int argc, char *argv[])
{
    //initialize random number generation
    srand(time(NULL));
    //need to create object
    SDL_init();
    chip8 emulator;
    //init function
    emulator.init();
    //load rom
    emulator.memory_load();
    //run emulator
    //emulator.mem_print(0x239, 0x284);
    SDL_Event ev;
    bool running = true;
    while (running)
    {
        while (SDL_PollEvent(&ev))
        {
            switch (ev.type)
            {
            case SDL_QUIT:
                SDL_kill();
                running = false;
                break;
            case SDL_KEYDOWN:
                for (int i = 0; i < 16; i++){
                    if (ev.key.keysym.sym == keymap[i]) {
                        emulator.keys[i] = 1;
                        std::cout<< i << "is down" << std::endl;
                    }
                }
            break;
            case SDL_KEYUP:
                for (int i = 0; i < 16; i++){
                    if (ev.key.keysym.sym == keymap[i]) {
                        emulator.keys[i] = 0;
                        std::cout<< i << "is up" << std::endl;
                    }
                }
            break;
            }
        }
        if (emulator.sound_timer > 0){
        emulator.sound_timer -= 1;
        }
        if (emulator.delay_timer > 0){
        emulator.delay_timer -= 1;
        }
        int timer = emulator.delay_timer ;
        //std::cout << "delay timer "<< timer<< std::endl;
        SDL_Delay(10);
        emulator.emulate();
        int *pixel;
        int pitch = 32;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_LockTexture(texture, NULL, (void **)&pixel, &pitch);
        for (int x = 0; x < 63; x++)
        {
            for (int y = 0; y < 31; y++)
            {
                //std::cout<<emulator.display[x + y*64];
                if (emulator.display[x + y * 64])
                {
                    pixel[x + y * 64] = 0xFFFFFFFF;
                }
                else{
                    pixel[x + y * 64] = 0x0;
                }
            }
        }
        SDL_UnlockTexture(texture);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_Rect texture_rect;
        texture_rect.x = 0;   //the x coordinate
        texture_rect.y = 0;   // the y coordinate
        texture_rect.w = 640; //the width of the texture
        texture_rect.h = 320; //the height of the texture
        SDL_RenderCopy(renderer, texture, NULL, &texture_rect);
        SDL_RenderPresent(renderer);

        //std::cout << "Hello World" << std::endl;
    }
    SDL_kill();
    return 0;
}

bool SDL_init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
        system("pause");
        return false;
    }

    window = SDL_CreateWindow("Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * 10, HEIGHT * 10, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cout << "Error creating window: " << SDL_GetError() << std::endl;
        system("pause");
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cout << "Error creating renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create texture
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture)
    {
        std::cout << "Error creating texture: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}
void SDL_draw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int x; x < 63; x++)
    {
        for (int y; y < 31; y++)
        {
            //           if (display[x + y*64]){SDL_RenderDrawPoint( renderer, x, y );}
        }
    }
}

void SDL_kill()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    texture = NULL;
    window = NULL;
    renderer = NULL;
    SDL_Quit();
}