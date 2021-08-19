#pragma once
#include <vector>
#include <Stratega/Representation/Vector2.h>

namespace SGA
{
    template<typename Type>
    class Map2D
    {
        size_t width;
        size_t height;


    public:
        std::vector<Type> grid;
        std::map<Vector2i, int> cellHits;
        std::vector<Vector2i> occupied;
        double incrementWidthVal = 1./width;
        double incrementHeightVal = 1./height;
        typedef typename std::vector<Type>::reference reference;
        typedef typename std::vector<Type>::const_reference const_reference;

        template<typename InputIterator>
        Map2D(size_t width, InputIterator begin, InputIterator end)
                : width(width), height(0), grid(begin, end)
        {
            if(grid.size() % width != 0)
            {
                throw std::runtime_error("Received a amount of values that is not a multiple of width.");
            }

            height = grid.size() / width;
        }


        Map2D(size_t width, size_t height, Type defaultValue = Type())
                : width(width), height(height), grid(width * height, defaultValue)
        {
        }

        reference operator[] (const Vector2i& pos) { return grid[pos.y * width + pos.x]; }
        const_reference operator[] (const Vector2i& pos) const { return grid[pos.y * width + pos.x]; }

        reference get(int x, int y) { return grid[y * width + x]; }
        const_reference get(int x, int y) const { return grid[y * width + x]; }

        [[nodiscard]] size_t getWidth() const { return width; }
        [[nodiscard]] size_t getHeight() const { return height; }

        [[nodiscard]] int getCellXPosition(double fitnessX)
        {
            double incrementWidth = 0;
            int x = -1;
            do {
                if(incrementWidth + incrementWidthVal > 1)
                    break;
                incrementWidth += incrementWidthVal;
                x++;
            }
            while (fitnessX > incrementWidth);

            return x;
        }

        [[nodiscard]] int getCellYPosition(double fitnessY)
        {
            double incrementHeight = 0;
            int y = -1;
            do {
                incrementHeight += incrementHeightVal;
                y++;
            }
            while (fitnessY > incrementHeight);

            return y;
        }

        void setCell(int x, int y, MapState newMap)
        {
            auto test = newMap;
            std::cout << "change\nat map" << y * width + x << "\n";
            grid[y * width + x] = newMap;
        }
    };
}