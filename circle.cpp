#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <vector>
sf::VertexArray appendThickVertex(sf::Vector2f p, sf::Vector2f lastP, float thickness, sf::VertexArray outline) {
    sf::Vector2f tangent = p - lastP;
    float length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    
    sf::Vector2f normal(0.f, 0.f);
    if (length != 0) {
        normal = sf::Vector2f(-tangent.y / length, tangent.x / length);
    }

    sf::Vector2f pOut = p + normal * (thickness / 2.f);
    sf::Vector2f pIn  = p - normal * (thickness / 2.f);

    outline.append(sf::Vertex(pOut, sf::Color::Black));
    outline.append(sf::Vertex(pIn, sf::Color::Black));
    return outline;
}

sf::Vector2f rotateVector(sf::Vector2f v, float angle) {
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    return sf::Vector2f(v.x * cosA - v.y * sinA, v.x * sinA + v.y * cosA);
}

sf::Vector2f calculateBezier(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    sf::Vector2f p = uuu * p0;
    p += 3.0f * uu * t * p1;
    p += 3.0f * u * tt * p2;
    p += ttt * p3;
    return p;
}

struct ControlPoint {
    sf::Vector2f pos;
    sf::RectangleShape visualNode;
    void initVisual(sf::Color color = sf::Color::Blue) {
        visualNode.setOrigin(6.f, 6.f);
        visualNode.setSize(sf::Vector2f(10,10));
        visualNode.setFillColor(color);
        updateVisualPosition();
    }
    void updateVisualPosition() {
        visualNode.setPosition(pos);
    }
};

class CircleBezier4Points {
public:
    ControlPoint pTop;
    ControlPoint pRight;
    ControlPoint pBottom;
    ControlPoint pLeft;

    ControlPoint* selectedPoint = nullptr;
    
    sf::VertexArray outline;
    sf::VertexArray fillShape; // Relleno interior plomo
    sf::VertexArray handlesLines;
    std::vector<sf::RectangleShape> handleDots;
    
    float radius = 100.0f;
    float thickness = 4.0f;
    int pointsPerSegment = 25;
    
    // Constantes k independientes para cada nodo
    float kTop = 0.552284f;
    float kRight = 0.552284f;
    float kBottom = 0.552284f;
    float kLeft = 0.552284f;
    
    // Rotaciones individuales de las manecillas por punto
    float rotTop = 0.f;
    float rotRight = 0.f;
    float rotBottom = 0.f;
    float rotLeft = 0.f;

    CircleBezier4Points(sf::Vector2f center) 
    : outline(sf::TriangleStrip), fillShape(sf::TrianglesFan), handlesLines(sf::Lines) {
        pTop.pos    = sf::Vector2f(center.x, center.y - radius);
        pRight.pos  = sf::Vector2f(center.x + radius, center.y);
        pBottom.pos = sf::Vector2f(center.x, center.y + radius);
        pLeft.pos   = sf::Vector2f(center.x - radius, center.y);

        pTop.initVisual();
        pRight.initVisual();
        pBottom.initVisual();
        pLeft.initVisual();

        selectedPoint = &pTop;
        pTop.visualNode.setFillColor(sf::Color::Red);

        update();
    }

    void update() {
        outline.clear();
        fillShape.clear();
        handlesLines.clear();
        handleDots.clear();

        pTop.updateVisualPosition();
        pRight.updateVisualPosition();
        pBottom.updateVisualPosition();
        pLeft.updateVisualPosition();

        // Tangentes individuales usando su respectivo valor de 'k'
        sf::Vector2f tangentTop1   = rotateVector(sf::Vector2f(-(radius * kTop), 0.f), rotTop);
        sf::Vector2f tangentTop2   = rotateVector(sf::Vector2f( (radius * kTop), 0.f), rotTop);

        sf::Vector2f tangentRight1 = rotateVector(sf::Vector2f(0.f, -(radius * kRight)), rotRight);
        sf::Vector2f tangentRight2 = rotateVector(sf::Vector2f(0.f,  (radius * kRight)), rotRight);

        sf::Vector2f tangentBottom1= rotateVector(sf::Vector2f( (radius * kBottom), 0.f), rotBottom);
        sf::Vector2f tangentBottom2= rotateVector(sf::Vector2f(-(radius * kBottom), 0.f), rotBottom);

        sf::Vector2f tangentLeft1  = rotateVector(sf::Vector2f(0.f,  (radius * kLeft)), rotLeft);
        sf::Vector2f tangentLeft2  = rotateVector(sf::Vector2f(0.f, -(radius * kLeft)), rotLeft);

        sf::Vector2f hTopLeft     = pTop.pos + tangentTop1;
        sf::Vector2f hTopRight    = pTop.pos + tangentTop2;

        sf::Vector2f hRightUp     = pRight.pos + tangentRight1;
        sf::Vector2f hRightDown   = pRight.pos + tangentRight2;

        sf::Vector2f hBottomRight = pBottom.pos + tangentBottom1;
        sf::Vector2f hBottomLeft  = pBottom.pos + tangentBottom2;

        sf::Vector2f hLeftDown    = pLeft.pos + tangentLeft1;
        sf::Vector2f hLeftUp      = pLeft.pos + tangentLeft2;

        addHandleVisual(pTop.pos, hTopLeft);
        addHandleVisual(pTop.pos, hTopRight);
        addHandleVisual(pRight.pos, hRightUp);
        addHandleVisual(pRight.pos, hRightDown);
        addHandleVisual(pBottom.pos, hBottomRight);
        addHandleVisual(pBottom.pos, hBottomLeft);
        addHandleVisual(pLeft.pos, hLeftDown);
        addHandleVisual(pLeft.pos, hLeftUp);

        std::vector<sf::Vector2f> curvePoints;

        // Curva 1: Top -> Right
        for (int i = 0; i <= pointsPerSegment; i++) {
            float t = (float)i / pointsPerSegment;
            curvePoints.push_back(calculateBezier(pTop.pos, hTopRight, hRightUp, pRight.pos, t));
        }

        // Curva 2: Right -> Bottom
        for (int i = 1; i <= pointsPerSegment; i++) {
            float t = (float)i / pointsPerSegment;
            curvePoints.push_back(calculateBezier(pRight.pos, hRightDown, hBottomRight, pBottom.pos, t));
        }

        // Curva 3: Bottom -> Left
        for (int i = 1; i <= pointsPerSegment; i++) {
            float t = (float)i / pointsPerSegment;
            curvePoints.push_back(calculateBezier(pBottom.pos, hBottomLeft, hLeftDown, pLeft.pos, t));
        }

        // Curva 4: Left -> Top
        for (int i = 1; i <= pointsPerSegment; i++) {
            float t = (float)i / pointsPerSegment;
            curvePoints.push_back(calculateBezier(pLeft.pos, hLeftUp, hTopLeft, pTop.pos, t));
        }

        // Relleno interior plomo (usando un centro aproximado con el promedio de los puntos)
        sf::Vector2f center(
            (pTop.pos.x + pRight.pos.x + pBottom.pos.x + pLeft.pos.x) / 4.f,
            (pTop.pos.y + pRight.pos.y + pBottom.pos.y + pLeft.pos.y) / 4.f
        );
        fillShape.append(sf::Vertex(center, sf::Color(160, 160, 160, 200))); // Color plomo translúcido
        for (const auto& pt : curvePoints) {
            fillShape.append(sf::Vertex(pt, sf::Color(160, 160, 160, 200)));
        }
        fillShape.append(sf::Vertex(curvePoints[0], sf::Color(160, 160, 160, 200))); // Cerrar abanico

        // Contorno grueso
        for (size_t i = 0; i < curvePoints.size(); i++) {
            sf::Vector2f lastP = (i == 0) ? (curvePoints[0] + (curvePoints[0] - curvePoints[1])) 
                                          : curvePoints[i - 1];
            outline = appendThickVertex(curvePoints[i], lastP, thickness, outline);
        }
    }

    void addHandleVisual(sf::Vector2f origin, sf::Vector2f handlePos) {
        handlesLines.append(sf::Vertex(origin, sf::Color(150, 150, 150, 200)));
        handlesLines.append(sf::Vertex(handlePos, sf::Color(150, 150, 150, 200)));
        
        sf::RectangleShape dot;
        dot.setOrigin(4.f, 4.f);
        dot.setSize(sf::Vector2f(5.f, 5.f));
        dot.setPosition(handlePos);
        dot.setFillColor(sf::Color(0, 180, 255));
        handleDots.push_back(dot);
    }

    void selectPoint(ControlPoint* pt) {
        pTop.visualNode.setFillColor(sf::Color::Blue);
        pRight.visualNode.setFillColor(sf::Color::Blue);
        pBottom.visualNode.setFillColor(sf::Color::Blue);
        pLeft.visualNode.setFillColor(sf::Color::Blue);

        selectedPoint = pt;
        if (selectedPoint) {
            selectedPoint->visualNode.setFillColor(sf::Color::Red);
        }
    }

    void draw(sf::RenderTarget& target) {
        target.draw(fillShape);
        target.draw(outline);
        target.draw(handlesLines);
        for (auto& dot : handleDots) target.draw(dot);
        target.draw(pTop.visualNode);
        target.draw(pRight.visualNode);
        target.draw(pBottom.visualNode);
        target.draw(pLeft.visualNode);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(700, 620), "Circulo Relleno Plomo con 4 Controles Independientes");
    sf::View viewEscena(sf::FloatRect(0, 0, 640, 480));
    viewEscena.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 480.f / 620.f));
    
    sf::RectangleShape drawScena(sf::Vector2f(640, 480));
    drawScena.setFillColor(sf::Color(255, 252, 252));
    drawScena.setOutlineThickness(-2.f);
    drawScena.setOutlineColor(sf::Color::Red);
    drawScena.setPosition(0.f, 0.f);
    
    CircleBezier4Points circle(sf::Vector2f(320.f, 240.f));

    float forceValue = 3.0f;
    float tangentStep = 0.05f;
    float rotationStep = 0.05f; 

    int movMode = 1; 
    bool movEjeX = true;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::W) {
                    circle.selectPoint(&circle.pTop);
                } else if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::D) {
                    circle.selectPoint(&circle.pRight);
                } else if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::S) {
                    circle.selectPoint(&circle.pBottom);
                } else if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::A) {
                    circle.selectPoint(&circle.pLeft);
                }
                else if (event.key.code == sf::Keyboard::R) {
                    movMode = 1; 
                } else if (event.key.code == sf::Keyboard::T) {
                    movMode = 2; 
                } else if (event.key.code == sf::Keyboard::Y) {
                    movMode = 3; 
                }
                else if (event.key.code == sf::Keyboard::J) {
                    movEjeX = true;
                } else if (event.key.code == sf::Keyboard::C) {
                    movEjeX = false; 
                }
                else if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::BackSpace) {
                    float multiplier = (event.key.code == sf::Keyboard::Space) ? 1.0f : -1.0f;

                    if (movMode == 1) {
                        if (movEjeX) circle.selectedPoint->pos.x += forceValue * multiplier;
                        else         circle.selectedPoint->pos.y += forceValue * multiplier;
                    } 
                    else if (movMode == 2) {
                        if (circle.selectedPoint == &circle.pTop)         circle.rotTop += rotationStep * multiplier;
                        else if (circle.selectedPoint == &circle.pRight)  circle.rotRight += rotationStep * multiplier;
                        else if (circle.selectedPoint == &circle.pBottom) circle.rotBottom += rotationStep * multiplier;
                        else if (circle.selectedPoint == &circle.pLeft)   circle.rotLeft += rotationStep * multiplier;
                    } 
                    else if (movMode == 3) {
                        // Modificar únicamente el factor k del nodo seleccionado en rojo
                        if (circle.selectedPoint == &circle.pTop)         circle.kTop = std::max(0.f, circle.kTop + tangentStep * multiplier);
                        else if (circle.selectedPoint == &circle.pRight)  circle.kRight = std::max(0.f, circle.kRight + tangentStep * multiplier);
                        else if (circle.selectedPoint == &circle.pBottom) circle.kBottom = std::max(0.f, circle.kBottom + tangentStep * multiplier);
                        else if (circle.selectedPoint == &circle.pLeft)   circle.kLeft = std::max(0.f, circle.kLeft + tangentStep * multiplier);
                    }
                    
                    circle.update();
                }
            }
        }
        
        window.clear(sf::Color(30, 30, 30));
        window.setView(viewEscena);
        window.draw(drawScena);
        circle.draw(window);
        window.display();
    }
    return 0;
}
