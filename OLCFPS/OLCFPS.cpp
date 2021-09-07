// OLCFPS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//TODO: find problem with player controls and why wall shading doesn't work
//github project works, but mine doesn't
//timestamp: ~18:54

#include <iostream>
#include <chrono>
#include <Windows.h>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

//unable to use std::cout because it causes scrolling
//need a buffer

int nScreenWidth = 140;
int nScreenHeight = 60;

//basic player x and y
float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
//player angle
float fPlayerA = 0.0f;

int nMapHeight = 16;
int nMapWidth = 16;

float fFOV = 3.14159f / 4.0f;
//for the raycasting loop to make sure the ray does not go on forever
float fDepth = 16.0f;

int main()
{
    //create a 2d array representation of the screen
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    //uses the console buffer previous created
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    std::wstring map;

    map += L"################";
    map += L"#..............#";
    map += L"#...........#..#";
    map += L"#...........#..#";
    map += L"#..............#";
    map += L"#..####........#";
    map += L"#.....#........#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#...#######....#";
    map += L"#......#.......#";
    map += L"#......#.......#";
    map += L"#..............#";
    map += L"################";

    auto tp1 = std::chrono::steady_clock::now();
    auto tp2 = std::chrono::steady_clock::now();

    //game loop
    while (1)
    {
        //for delta time
        tp2 = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        //Controls
        //ccw rotation
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
        {
            fPlayerA -= (1.5f) * fElapsedTime;

            //just added these to reset the value because I didn't like seeing big values for player angle
            //is just a rough estimation of the circle but is good enough
            if (fPlayerA < -6.3)
                fPlayerA = 0;
        }

        //cw rotation
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
        {
            fPlayerA += (1.5f) * fElapsedTime;

            if (fPlayerA > 6.3)
                fPlayerA = 0;
        }

        //forwards movement
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += -sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;

            //collision detection
            //just undoes what the player done and moves back when colliding in wall
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX -= -sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }

        //backwards movement
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= -sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;

            //collision detection
            //just undoes what the player done and moves back when colliding in wall
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX += -sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }

        std::wstring minimap = map;

        for (int x = 0; x < nScreenWidth; x++)
        {
            //for each column, calculate the projected ray angle into world space
            // 
            //(fPlayerA - fFOV / 2.0f) tries to find the starting angle for the fov
            //this part bisects the fov
            //
            //((float)x / (float)nScreenWidth) cuts it up into nScreenWidth parts
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            float fDistanceToWall = 0.0f;
            bool bHitWall = false;

            //boundary is if it is edge of cell
            //basically if ray has hit edge of cube
            //
            //the ray will find the center of the cell,
            //then it will find the perfect corners of the cell
            //when it finds the corners, it will find the two closest corners
            bool bBoundary = false;

            //basic raytracing

            //unit vector for ray in player space to calculate the test point
            float fEyeX = -sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1f;

                //creates a line of a certain distance
                //only need integers since walls are in an array
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                //test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    //set the distance to max depth
                    fDistanceToWall = fDepth;
                }
                else
                {
                    //ray is inbounds so test if ray cell is wall block
                    if (map[nTestY * nMapWidth + nTestX] == '#')
                    {
                        bHitWall = true;

                        std::vector<std::pair<float, float>> p; //distance to corner, dot

                        //tx, ty are perfect integer corners
                        for (int tx = 0; tx < 2; tx++)
                            for (int ty = 0; ty < 2; ty++)
                            {
                                //offset by fPlayer coordinate
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;

                                //dot product
                                //is between ray unit vector and unit vector of corner
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(std::make_pair(d, dot));
                            }
                        //sort pairs from closest to farthest
                        //takes two pairs and compares based on first element of pair, distance
                        sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {return left.first < right.first; });

                        //radians
                        float fBound = 0.005;

                        //most you can see of a cube is three corners in 3d space
                        //however, would have to do extra work to make it so you don't see the third corner through
                        // the wall if looking square at a face of the cube
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                    }
                    else if (map[nTestY * nMapWidth + nTestX] == '.')
                    {
                        minimap[nTestY * nMapWidth + nTestX] = '|';
                    }
                }
            }

            //calculate distance to ceiling and floor
            //this is done to create the illusion of closer and farther walls
            //when a wall is closer, it will look bigger by having the floor and ceiling of the wall be larger
            
            //takes the midpoint, and subtracts a proportion of screen height relative to fDistanceToWall
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            //mirrors ceiling
            int nFloor = nScreenHeight - nCeiling;

            //shades wall based on distance
            //uses extended unicode for blocks
            short nShade = ' ';

            if (fDistanceToWall <= fDepth / 4.0f)       //closer
                nShade = 0x2588;
            else if (fDistanceToWall < fDepth / 3.0f)
                nShade = 0x2593;
            else if (fDistanceToWall < fDepth / 2.0f)
                nShade = 0x2592;
            else if (fDistanceToWall < fDepth)
                nShade = 0x2591;
            else
                nShade = ' ';                           //farther

            if (bBoundary)
                nShade = ' ';                           //shade boundary

            for (int y = 0; y < nScreenHeight; y++)
            {
                //if y is less than nCeiling, fill with "sky" with blank space
                //else if less than nCeiling and less than nFloor, then fill with wall
                if (y <= nCeiling)
                    //ceiling
                    screen[y * nScreenWidth + x] = ' ';
                else if (y > nCeiling && y <= nFloor)
                    //wall
                    screen[y * nScreenWidth + x] = nShade;
                else
                {
                    //floor

                    //floor shading based on distance
                    //can be done here because the floor does not change distance from player
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25)
                        nShade - '#';
                    else if (b < 0.5)
                        nShade = 'x';
                    else if (b < 0.75)
                        nShade = '.';
                    else if (b < .9)
                        nShade = '-';
                    else
                        nShade = ' ';

                    screen[y * nScreenWidth + x] = nShade;
                }
            }
        }

        //display stats
        swprintf_s(screen, 40, LR"(X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f )", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        //display map
        //
        //replaces corner with map
        //can be done since all the things in termial are viewed with characters
        //offset by one to not overwrite stats
        for (int nx = 0; nx < nMapHeight; nx++)
            for (int ny = 0; ny < nMapWidth; ny++)
            {
                screen[(ny + 1) * nScreenWidth + nx] = minimap[ny * nMapWidth + nx];
            }

        //player position to be written over the map
        screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

        //give escape character to know when to stop outputing string
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        //give handle, buffer, how many bytes, the coordinates of where text is written, not sure what the last one is for but isn't needed
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
