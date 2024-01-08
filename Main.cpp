#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

constexpr const auto windowSizeX = 960;
constexpr const auto windowSizeY = 540;

template<class T>
inline const T& clamp(const T& v, const T& lo, const T& hi)
{
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

/* For some reason sfml dont clamp the sf::Mouse::GetPosition(window), and when is off screen get negative */
template <typename T>
inline sf::Vector2<T> clampMouseVector(const sf::Vector2<T>& vector)
{
    return sf::Vector2<T>(clamp(vector.x, 0, windowSizeX), clamp(vector.y, 0, windowSizeY));
}

// THE POWER OF BATCH RENDERING //
class BatchCells : public sf::Drawable, public sf::Transformable
{
public:
    BatchCells(const sf::Vector2i& cellSize, const sf::Vector2i& nCell) :
        m_CellSize(cellSize), m_NumCell(nCell)
    {
        m_Vertices.setPrimitiveType(sf::Quads);
        m_Vertices.resize(m_NumCell.x * m_NumCell.y * 4);
    }
    ~BatchCells() = default;

    void batch(int x, int y, const sf::Color& color)
    {
        sf::Vertex* quad = &m_Vertices[(y * m_NumCell.x + x) * 4];

        quad[0].position = sf::Vector2f(x * m_CellSize.x, y * m_CellSize.y);
        quad[1].position = sf::Vector2f((x + 1) * m_CellSize.x, y * m_CellSize.y);
        quad[2].position = sf::Vector2f((x + 1) * m_CellSize.x, (y + 1) * m_CellSize.y);
        quad[3].position = sf::Vector2f(x * m_CellSize.x, (y + 1) * m_CellSize.y);
        quad[0].color = color;
        quad[1].color = color;
        quad[2].color = color;
        quad[3].color = color;
    }

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        states.transform *= getTransform();

        states.texture = &m_Texture;

        target.draw(m_Vertices, states);
    }

    sf::Vector2i m_NumCell;
    sf::Vector2i m_CellSize;

    sf::VertexArray m_Vertices;

    // textures?
    sf::Texture m_Texture;
};

struct CellInfo
{
    sf::Vector2i nCell;
    sf::Vector2i cellSize;
};

int main()
{
    // Window
    sf::RenderWindow window(sf::VideoMode(windowSizeX, windowSizeY), "Game of Life");
    window.setFramerateLimit(60);

    // Cells info
    CellInfo cellInfo = { {96 * 5, 54 * 5}, {windowSizeX / cellInfo.nCell.x, windowSizeY / cellInfo.nCell.y} };

    // Init the state matrix
    std::vector<int8_t> wordState;
    wordState.resize(cellInfo.nCell.x * cellInfo.nCell.y);
    for (auto& v : wordState)
        v = 0;

    // Init the pre-state matrix
    std::vector<int8_t> preWordState;
    preWordState.resize(cellInfo.nCell.x * cellInfo.nCell.y);
    for (auto& v : preWordState)
        v = 0;

    // random cells in the state matrix
    srand(static_cast<unsigned int>(time(nullptr)));
    for (int i = 0; i < cellInfo.nCell.x * cellInfo.nCell.y; i++)
        wordState[i] = rand() % 2;

    BatchCells cells(cellInfo.cellSize, cellInfo.nCell);

    auto getCell = [&](int x, int y)
        {
            return preWordState[y * cellInfo.nCell.x + x];
        };

    auto setCell = [&](int x, int y, int value)
        {
            return wordState[y * cellInfo.nCell.x + x] = value;
        };

    // Zoom and Drag variables
    bool isDragging = false;
    sf::Vector2f prevMousePos;
    float zoomFactor = 1.0f;

    while (window.isOpen())
    {
        sf::Event events;
        while (window.pollEvent(events))
        {
            if (events.type == sf::Event::Closed)
                window.close();
            if (events.type == sf::Event::MouseButtonPressed && events.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mousePos = clampMouseVector(sf::Mouse::getPosition(window));
                setCell(mousePos.x / cellInfo.cellSize.x, mousePos.y / cellInfo.cellSize.y, 1);
            }
            if (events.type == sf::Event::MouseButtonPressed && events.mouseButton.button == sf::Mouse::Middle)
            {
                isDragging = true;
                prevMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            }
            if (events.type == sf::Event::MouseButtonReleased && events.mouseButton.button == sf::Mouse::Middle)
            {
                isDragging = false;
            }
            if (events.type == sf::Event::MouseWheelScrolled)
            {
                if (events.mouseWheelScroll.delta > 0)
                    zoomFactor *= 1.1f;
                else if (events.mouseWheelScroll.delta < 0)
                    zoomFactor /= 1.1f;

                zoomFactor = clamp(zoomFactor, 0.5f, 2.0f);
            }
        }

        if (isDragging)
        {
            sf::Vector2f currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            sf::Vector2f offset = currentMousePos - prevMousePos;
            cells.move(offset);
            prevMousePos = currentMousePos;
        }

        window.clear(sf::Color(31, 31, 31, 255));

        preWordState = wordState;

        for (int x = 1; x < cellInfo.nCell.x - 1; x++)
        {
            for (int y = 1; y < cellInfo.nCell.y - 1; y++)
            {
                auto vecinos = getCell((x - 1) % cellInfo.nCell.x, (y - 1) % cellInfo.nCell.y) + getCell((x) % cellInfo.nCell.x, (y - 1) % cellInfo.nCell.y) +
                    getCell((x + 1) % cellInfo.nCell.x, (y - 1) % cellInfo.nCell.y) + getCell((x + 1) % cellInfo.nCell.x, (y) % cellInfo.nCell.y) +
                    getCell((x + 1) % cellInfo.nCell.x, (y + 1) % cellInfo.nCell.y) + getCell((x) % cellInfo.nCell.x, (y + 1) % cellInfo.nCell.y) +
                    getCell((x - 1) % cellInfo.nCell.x, (y + 1) % cellInfo.nCell.y) + getCell((x - 1) % cellInfo.nCell.x, (y) % cellInfo.nCell.y);
                if (getCell(x, y) == 1)
                    setCell(x, y, vecinos == 2 || vecinos == 3);
                else
                    setCell(x, y, vecinos == 3);

                if (getCell(x, y) == 1)
                    cells.batch(x, y, sf::Color::Green);
                else
                    cells.batch(x, y, sf::Color::Black);
            }
        }

        // Apply zoom
        cells.setScale(zoomFactor, zoomFactor);

        window.draw(cells);

        window.display();
    }

    return 0;
}
