#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
using namespace std;

#include <Windows.h>

int nScreenWidth = 120;
int nScreenHeight = 40;

// World dimensions
int nMapHeight = 16;
int nMapWidth = 16;

float fPlayerX = 8.f;
float fPlayerY = 8.f;
float fPlayerA = 0.f;

float fFOV = 3.1459f / 4.0f; // Field of view
float fDepth = 16.f; // Max rendering distance
float fSpeed = 5.0f; // Walk speed

int main()
{
    // Create screen buffer
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;
    
    // Create Map of world space # = wall block, . = space
    wstring map;
    map += L"################";
    map += L"#..............#";
    map += L"#.......########";
    map += L"#..............#";
    map += L"#......##......#";
    map += L"#......##......#";
    map += L"###............#";
    map += L"##.............#";
    map += L"#......####..###";
    map += L"#......#.......#";
    map += L"#......#.......#";
    map += L"#..............#";
    map += L"#......#########";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    // Game loop
    while (1)
    {
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();
        
        // Controls
        // Handle CCW Rotation
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
        {
            fPlayerA -= (0.6f) * fElapsedTime;
        }

        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
        {
            fPlayerA += (0.6f) * fElapsedTime;
        }

        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;

            if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
            {
                fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;

            if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
            {
                fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        for (int x = 0; x < nScreenWidth; x++)
        {
            // For each column, calculate the projected ray angle into the world space
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            // Find distance from wall
            float fStepSize = 0.1f;
            float fDistanceToWall = 0.0f;
            bool bHitWall = false;
            bool bBoundary = false;

            // Unit vector for ray in player space
            float fEyeX = sinf(fRayAngle); 
            float fEyeY = cosf(fRayAngle); 

            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += fStepSize;

                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    // Just set distance to the maximum depth
                    bHitWall = true;    
                    fDistanceToWall = fDepth;
                }
                else
                {
                    // Ray is inbounds so test to see if the ray cell is a wall block
                    if (map.c_str()[nTestX * nMapWidth + nTestY] == '#')
                    {
                        bHitWall = true;

                        vector<pair<float, float>> p; // Distance, dot

                        for (int tx = 0; tx < 2; tx++)
                        {
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx*vx + vy*vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }
                        }

                        // Sort pairs from closest to farthest
                        sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

                        // First two are closest (we will never see all four)
                        float fBound = 0.01;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            // Shader walls based on distance
            short nShade = ' ';
            if (fDistanceToWall <= fDepth / 4.0f)
            {
                nShade = 0x2588; // Very close
            }
            else if (fDistanceToWall < fDepth / 3.0f) 
            {
                nShade = 0x2593;
            }
            else if (fDistanceToWall < fDepth / 2.0f)
            {
                nShade = 0x2592;
            }
            else if (fDistanceToWall < fDepth)
            {
                nShade = 0x2591;
            }
            else
            {
                nShade = ' '; // Too far away
            }

            if (bBoundary)
            {
                nShade = ' '; // Black it out
            }

            for (int y = 0; y < nScreenHeight; y++)
            {
                if (y <= nCeiling)
                {
                    screen[y * nScreenWidth + x] = ' ';
                }
                else if (y > nCeiling && y <= nFloor)
                {
                    screen[y * nScreenWidth + x] = nShade;
                }
                else
                {
                    // Shade floor based on distance
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25)
                    {
                        nShade = '#';
                    }
                    else if (b < 0.5)
                    {
                        nShade = 'X';
                    }
                    else if (b < 0.75)
                    {
                        nShade = '.';
                    }
                    else if (b < 0.9)
                    {
                        nShade = '-';
                    }
                    else
                    {
                        nShade = ' ';
                    }
                    screen[y * nScreenWidth + x] = nShade;
                }
            }

        }

        // Display stats
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        // Display map
        for (int nx = 0; nx < nMapWidth; nx++)
        {
            for (int ny = 0; ny < nMapHeight; ny++)
            {
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        }

        screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

        // Display Frame
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }

    return 0;
}