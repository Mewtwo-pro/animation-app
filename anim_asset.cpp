#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <map>
#include <string>
#include <cmath>
#include <set>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>            
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace fs = std::filesystem;
void setMensajeTime (std:: stringstream& ss , sf::Text& timeMensaje,
    bool& viewTimeMensaje , sf::Clock& mensajeClock){
  timeMensaje.setString(ss.str());
  viewTimeMensaje = true;
  mensajeClock.restart();
}

sf::VertexArray appendThickVertex(sf::Vector2f p, sf::Vector2f lastP, float thickness, 
  sf::VertexArray outline, sf::Color color) {
    sf::Vector2f tangent = p - lastP;
    float length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    sf::Vector2f normal(0.f, 0.f);
    if (length != 0) {
        normal = sf::Vector2f(-tangent.y / length, tangent.x / length);
    }
    sf::Vector2f pOut = p + normal * (thickness / 2.f);
    sf::Vector2f pIn  = p - normal * (thickness / 2.f);
    outline.append(sf::Vertex(pOut, color));
    outline.append(sf::Vertex(pIn, color));
    return outline;
}

sf::Vector2f calculateQuadraticBezier(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, float t) {
    float u = 1.0f - t;
    float uu = u * u;
    float tt = t * t;

    sf::Vector2f p = uu * p0; // (1-t)^2 * P0
    p += 2.0f * u * t * p1;   // 2(1-t)t * P1
    p += tt * p2;             // t^2 * P2
    
    return p;
}    
struct CopiedControl {
    int bodyIndex;
    int controlType;
    sf::Vector2f position;
};
struct PinnedControl {
    int bodyIndex;
    int controlType; // 0: pUp, 1: pCenter, 2: pBottom
};
std::vector<PinnedControl> pinnedControls;
struct ControlPoint {
    sf::Vector2f pos;
    sf::RectangleShape visualNode;

    bool multiSelected = false;

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
class QuadraticBezier {
public:
    ControlPoint pUp;
    ControlPoint pCenter;
    ControlPoint pBottom;
    bool allFront = false;
    ControlPoint* selectedPoint = nullptr;
    sf::VertexArray outline;
    sf::VertexArray handlesLines;
    bool onion = false;
    sf::Color colorOnion = sf::Color(120,120,120,80);
    float thickness = 6.0f;
    int pointsPerSegment = 50;

    // Nuevos atributos de color
    sf::Color primaryColor;
    sf::Color secondaryColor;
    float splitRatio = 0.9f; // 90% principal, 10% final

    QuadraticBezier(sf::Vector2f pStart, sf::Vector2f pEnd, 
                    sf::Color pColor = sf::Color(79, 79, 79), 
                    sf::Color sColor = sf::Color(79, 79, 79), 
                    float ratio = 0.9f) 
    : outline(sf::TriangleStrip), handlesLines(sf::Lines),
      primaryColor(pColor), secondaryColor(sColor), splitRatio(ratio) {
        
        pUp.pos = pStart; 
        pBottom.pos = pEnd;
        pCenter.pos = { (pStart.x + pEnd.x) / 2 , (pStart.y + pEnd.y) / 2 };
        pUp.initVisual();
        pBottom.initVisual();
        pCenter.initVisual(sf::Color::Green);
        update();
    }

    void update() {
        outline.clear();
        handlesLines.clear();   

        pUp.updateVisualPosition();
        pCenter.updateVisualPosition();
        pBottom.updateVisualPosition();

        pUp.visualNode.setFillColor(
            selectedPoint == &pUp || pUp.multiSelected
                ? sf::Color(180,20,50)
                : sf::Color::Blue
        );

        pCenter.visualNode.setFillColor(
            selectedPoint == &pCenter || pCenter.multiSelected
                ? sf::Color(180,20,50)
                : sf::Color::Green
        );

        pBottom.visualNode.setFillColor(
            selectedPoint == &pBottom || pBottom.multiSelected
                ? sf::Color(180,20,50)
                : sf::Color::Blue
        );
        std::vector<sf::Vector2f> curvePoints;
        
        for (int i = 0; i <= pointsPerSegment; ++i) {
            float t = (float)i / pointsPerSegment;
            curvePoints.push_back(calculateQuadraticBezier(pUp.pos, pCenter.pos, pBottom.pos, t));
        }

        // Construir el contorno aplicando el cambio de color en el último 10%
        for (size_t i = 0; i < curvePoints.size(); ++i) {
            sf::Vector2f lastP = (i == 0) ? curvePoints[curvePoints.size() - 2] : curvePoints[i - 1]; 
            
            float progress = (float)i / pointsPerSegment;
            sf::Color currentColor = (progress >= splitRatio) ? secondaryColor : primaryColor;

            outline = appendThickVertex(curvePoints[i], lastP, thickness, outline, currentColor);
        }
    }

    void draw(sf::RenderTarget& target) {
      if (onion == false){
        target.draw(outline);
        if (selectedPoint == &pUp || pUp.multiSelected) {
            target.draw(pUp.visualNode);
        }
        if (selectedPoint == &pCenter || pCenter.multiSelected) {
            target.draw(pCenter.visualNode);
        }
        if (selectedPoint == &pBottom || pBottom.multiSelected) {
            target.draw(pBottom.visualNode);
        } 
      }else{
        sf::VertexArray onionOutline = outline;
        for (size_t i = 0 ; i < onionOutline.getVertexCount(); ++i){
          onionOutline[i].color = colorOnion;
        }
        target.draw(onionOutline);
      }
    }
};
struct dataElipce {
    float x;
    float y;
    float radius; // <--- Nuevo campo para la escala/radio
};
struct data_audio{
  std::string path_audio;
  sf::RectangleShape key_audio;
  sf::SoundBuffer soundBufferCache;
};
struct attr_body{
    float pUp_x;
    float pUp_y;
    float pCenter_x;
    float pCenter_y;
    float pBottom_x;
    float pBottom_y;
    bool allFront = false;
};
struct dataBodyKey{
    std::vector<attr_body> dataAttr;
    void set_key(const std::vector<std::unique_ptr<QuadraticBezier>>& dataRig){
        dataAttr.clear();
        for (const auto& ctrl : dataRig){
            dataAttr.push_back({
                ctrl->pUp.pos.x,      // <- Cambiado a ->
                ctrl->pUp.pos.y,
                ctrl->pCenter.pos.x,
                ctrl->pCenter.pos.y,
                ctrl->pBottom.pos.x,
                ctrl->pBottom.pos.y,
                ctrl->allFront
            }); 
        }     
    }
};

void updatePosbody(std::vector<std::unique_ptr<QuadraticBezier>>& datarig, const std::vector<attr_body>& dataAttr){
  int ix = 0;
  for (auto& ctrl : datarig){
    const auto localData = dataAttr[ix];
    ctrl->pUp.pos.x = localData.pUp_x;      // <- Cambiado a ->
    ctrl->pUp.pos.y = localData.pUp_y;
    ctrl->pCenter.pos.x = localData.pCenter_x;
    ctrl->pCenter.pos.y = localData.pCenter_y;
    ctrl->pBottom.pos.x = localData.pBottom_x; 
    ctrl->pBottom.pos.y = localData.pBottom_y; 
    ctrl->allFront = localData.allFront;
    ctrl->update();                        // <- Cambiado a ->
    ix += 1;
  }  
}
void selectCtrl(std::vector<std::unique_ptr<QuadraticBezier>>& fullBody , 
  QuadraticBezier*& preSelect, int idx, ControlPoint* tipeCtrl,int& selectAll){
    selectAll = 0;
    if (preSelect){
      preSelect->selectedPoint  = nullptr;
      preSelect->update();
    }
    fullBody[idx]->selectedPoint = tipeCtrl;
    fullBody[idx]->update();
    preSelect = fullBody[idx].get();
  }
void newKey(const sf::RectangleShape& currentFrame , sf::CircleShape& head,
  std::map<float , sf::RectangleShape>& frameKeylist,
  std::map <float,dataElipce>& dataHead,
  std::map <float,dataBodyKey>& bodyKeyList,
  std::vector<std::unique_ptr<QuadraticBezier>>& fullBody
){
  auto posCurrentk = currentFrame.getPosition();
  auto it = frameKeylist.find(posCurrentk.x);
  auto posHead = head.getPosition();
  float currentRadius = head.getRadius(); // <--- Obtener radio actual

  if (it != frameKeylist.end()){
    auto kh = dataHead.find(posCurrentk.x);
    if (kh != dataHead.end()){
      kh->second.x = posHead.x;
      kh->second.y = posHead.y;
      kh->second.radius = currentRadius; // <--- Actualizar radio
    }
    auto kb = bodyKeyList.find(posCurrentk.x); 
    if (kb != bodyKeyList.end()){
      kb->second.set_key(fullBody);
    } 
  }else{
    sf::RectangleShape key(sf::Vector2f(3,25));
    key.setPosition(posCurrentk.x , posCurrentk.y);
    key.setFillColor(sf::Color(255, 50 , 50));
    frameKeylist.insert({ posCurrentk.x, key});
    
    dataElipce k_head = {posHead.x, posHead.y, currentRadius}; // <--- Guardar con radio
    dataHead.insert({posCurrentk.x, k_head});
    
    dataBodyKey key_body;
    key_body.set_key(fullBody); 
    bodyKeyList.insert({posCurrentk.x, key_body}); 
  }
}
void drawOnion(
  sf::RenderWindow& window,
  std::vector<std::unique_ptr<QuadraticBezier>>& onionBody,
  const dataBodyKey& data,
  sf::Color& colorOnion
){
  updatePosbody(onionBody, data.dataAttr);
  for (auto& part : onionBody){
    part->onion = true;
    part->colorOnion = colorOnion;
    part->draw(window);
    part->onion = false;
  }
}
std::vector<std::string> loadAudioFiles() {
    std::vector<std::string> audioFiles;
    std::string path = "audio";

    if (fs::exists(path) && fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".wav") {
                    // Guardamos solo el nombre (ej: "salto.wav") para que se vea limpio en la UI
                    audioFiles.push_back(entry.path().filename().string());
                }
            }
        }
    } else {
        std::cout << "[AVISO]: La carpeta 'audio' no existe en la ruta actual.\n";
    }

    return audioFiles;
}

std::vector<std::string> loadAnimationFile() {
    std::vector<std::string> audioFiles;
    std::string path = "saveAnim";

    if (fs::exists(path) && fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".json") {
                    // Guardamos solo el nombre (ej: "salto.wav") para que se vea limpio en la UI
                    audioFiles.push_back(entry.path().filename().string());
                }
            }
        }
    } else {
        std::cout << "[AVISO]: La carpeta 'audio' no existe en la ruta actual.\n";
    }

    return audioFiles;
}
std::string toLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}
void moverFrameKeysDesde(
    std::map<float, sf::RectangleShape>& mapa,
    float desde,
    float cantidad)
{
    std::map<float, sf::RectangleShape> nuevoMapa;

    for (auto& [key, value] : mapa) {

        float nuevaKey = key;

        if (key >= desde) {
            nuevaKey += cantidad;

            value.setPosition(
                nuevaKey,
                value.getPosition().y
            );
        }

        nuevoMapa[nuevaKey] = value;
    }

    mapa = std::move(nuevoMapa);
}
template <typename T>
void retimeKeysRange(
    std::map<float, T>& mapa,
    float start_range,
    float end_range,
    float factor)
{
    float r_start = std::min(start_range, end_range);
    float r_end   = std::max(start_range, end_range);

    std::map<float, T> nuevoMapa;

    for (auto& [frame, value] : mapa) {

        // Fuera del rango: queda igual
        if (frame < r_start || frame > r_end) {
            nuevoMapa[frame] = value;
            continue;
        }

        // Dentro del rango: aplicar retime
        float nuevoFrame =
            r_start + (frame - r_start) * factor;

        nuevoMapa[nuevoFrame] = value;
    }

    mapa = std::move(nuevoMapa);
}
void retimeFrameKeysRange(
    std::map<float, sf::RectangleShape>& mapa,
    float start_range,
    float end_range,
    float factor)
{
    float r_start = std::min(start_range, end_range);
    float r_end   = std::max(start_range, end_range);

    std::map<float, sf::RectangleShape> nuevoMapa;

    for (auto& [frame, value] : mapa) {

        float nuevoFrame = frame;

        if (frame >= r_start && frame <= r_end) {

            nuevoFrame =
                r_start + (frame - r_start) * factor;

            value.setPosition(
                nuevoFrame,
                value.getPosition().y
            );
        }

        nuevoMapa[nuevoFrame] = value;
    }

    mapa = std::move(nuevoMapa);
}       
template <typename T>
void moverKeysDesde(
    std::map<float, T>& mapa,
    float desde,
    float cantidad)
{
    std::map<float, T> nuevoMapa;

    for (auto& [key, value] : mapa) {

        float nuevaKey = key;

        if (key >= desde) {
            nuevaKey += cantidad;
        }

        nuevoMapa[nuevaKey] = value;
    }

    mapa = std::move(nuevoMapa);
}
void sincronizarAudio(
    sf::Sound& activeSound,
    const std::map<float, data_audio>& audioKeyList,
    float currentFrameX)
{
    // Buscar el último audio que comenzó antes o exactamente
    // en el frame actual
    auto it = audioKeyList.upper_bound(currentFrameX);

    if (it == audioKeyList.begin()) {
        activeSound.stop();
        return;
    }

    --it;

    float audioStartFrame = it->first;
    float elapsedFrames = currentFrameX - audioStartFrame;

    float tiempoAudio = elapsedFrames /240;

    const auto& audio = it->second;

    activeSound.setBuffer(audio.soundBufferCache);

    if (tiempoAudio < audio.soundBufferCache.getDuration().asSeconds()) {
        activeSound.setPlayingOffset(
            sf::seconds(tiempoAudio)
        );
        //q este en play solo medio segundo y luego este en pause
        activeSound.play();
    } else {
        activeSound.stop();
    }
}
void actualizarTrayectoria(
    bool autoPath,
    float currentFrameX,
    const QuadraticBezier* preSelect,
    const std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    const std::map<float, dataBodyKey>& bodyKeyList,
    float start_range,
    float end_range,
    std::vector<sf::Vector2f>& trajectoryPoints,
    bool& viewPathPoint
) {
    trajectoryPoints.clear(); // Limpiar puntos anteriores

    if (preSelect == nullptr || preSelect->selectedPoint == nullptr) {
        viewPathPoint = false;
        return;
    }

    // 1. Encontrar el índice de la parte del cuerpo seleccionada
    int partIndex = -1;
    for (size_t i = 0; i < fullBody.size(); ++i) {
        if (fullBody[i].get() == preSelect) {
            partIndex = static_cast<int>(i);
            break;
        }
    }

    if (partIndex == -1) return;

    // 2. Determinar el tipo de punto (0: pUp, 1: pCenter, 2: pBottom)
    int pointType = -1;
    if (preSelect->selectedPoint == &preSelect->pUp) {
        pointType = 0;
    } else if (preSelect->selectedPoint == &preSelect->pCenter) {
        pointType = 1;
    } else if (preSelect->selectedPoint == &preSelect->pBottom) {
        pointType = 2;
    }

    if (pointType == -1) return;

    // 3. Evaluar si es ruta automática o por rango
    if (autoPath) {
        // Buscamos el keyframe más cercano por la izquierda del current frame
        auto itPre = bodyKeyList.lower_bound(currentFrameX);
        if (itPre != bodyKeyList.begin()) {
            --itPre;
            if (partIndex < static_cast<int>(itPre->second.dataAttr.size())) {
                const auto& attr = itPre->second.dataAttr[partIndex];
                sf::Vector2f posFound(0.f, 0.f);
                if (pointType == 0) posFound = {attr.pUp_x, attr.pUp_y};
                else if (pointType == 1) posFound = {attr.pCenter_x, attr.pCenter_y};
                else if (pointType == 2) posFound = {attr.pBottom_x, attr.pBottom_y};
                trajectoryPoints.push_back(posFound);
            }
        }

        // Buscamos el keyframe más cercano por la derecha del current frame
        auto itPost = bodyKeyList.upper_bound(currentFrameX);
        if (itPost != bodyKeyList.end()) {
            if (partIndex < static_cast<int>(itPost->second.dataAttr.size())) {
                const auto& attr = itPost->second.dataAttr[partIndex];
                sf::Vector2f posFound(0.f, 0.f);
                if (pointType == 0) posFound = {attr.pUp_x, attr.pUp_y};
                else if (pointType == 1) posFound = {attr.pCenter_x, attr.pCenter_y};
                else if (pointType == 2) posFound = {attr.pBottom_x, attr.pBottom_y};
                trajectoryPoints.push_back(posFound);
            }
        }
    } else {
        // Asegurar el orden correcto del rango
        float r_start = std::min(start_range, end_range);
        float r_end = std::max(start_range, end_range);

        // Recorrer el rango de keyframes
        for (auto& [framePos, bodyData] : bodyKeyList) {
            if (framePos >= r_start && framePos <= r_end) {
                if (partIndex < static_cast<int>(bodyData.dataAttr.size())) {
                    const auto& attr = bodyData.dataAttr[partIndex];
                    sf::Vector2f posFound(0.f, 0.f);

                    if (pointType == 0) {
                        posFound = {attr.pUp_x, attr.pUp_y};
                    } else if (pointType == 1) {
                        posFound = {attr.pCenter_x, attr.pCenter_y};
                    } else if (pointType == 2) {
                        posFound = {attr.pBottom_x, attr.pBottom_y};
                    }

                    trajectoryPoints.push_back(posFound);
                }
            }
        }
    }

    // Activamos la visualización si hay puntos
    if (!trajectoryPoints.empty()) {
        viewPathPoint = true;
    }
}
void flipPose(
    std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    sf::CircleShape& head
){
    // El eje vertical está definido por el pBottom de la spine
    float ejeX = fullBody[4]->pBottom.pos.x;

    // ---------------------------------
    // Reflejar todas las partes del cuerpo
    // ---------------------------------
    for (auto& part : fullBody){

        // pUp
        part->pUp.pos.x =
            2.f * ejeX - part->pUp.pos.x;

        // pCenter
        part->pCenter.pos.x =
            2.f * ejeX - part->pCenter.pos.x;

        // pBottom
        //
        // En la spine NO queremos mover el pBottom
        // porque él define el eje.
        if (part.get() != fullBody[4].get()){
            part->pBottom.pos.x =
                2.f * ejeX - part->pBottom.pos.x;
        }

        part->update();
    }

    // ---------------------------------
    // Reflejar cabeza
    // ---------------------------------
    // Obtener la esquina superior izquierda actual
    auto headPos = head.getPosition();
    float radio = head.getRadius(); // 50.f

    // Convertir a posición central, reflejar y volver a convertir a esquina superior izquierda
    float centroX = headPos.x + radio;
    float nuevoCentroX = 2.f * ejeX - centroX;
    float nuevaEsquinaX = nuevoCentroX - radio;

    head.setPosition(
        nuevaEsquinaX,
        headPos.y
    );
}
void mirrorPose(
    std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    sf::CircleShape& head
){
    // Eje vertical: pBottom de la spine
    float ejeX = fullBody[4]->pBottom.pos.x;

    // Guardar las posiciones originales de cada parte
    struct Pose {
        sf::Vector2f up;
        sf::Vector2f center;
        sf::Vector2f bottom;
    };

    std::vector<Pose> original;

    for (const auto& part : fullBody){
        original.push_back({
            part->pUp.pos,
            part->pCenter.pos,
            part->pBottom.pos
        });
    }

    // Reflejar un punto respecto al eje vertical
    auto mirrorPoint = [ejeX](sf::Vector2f p){
        return sf::Vector2f(
            2.f * ejeX - p.x,
            p.y
        );
    };

    // Intercambiar las poses reflejadas de los brazos
    // Brazo izquierdo recibe la pose reflejada del derecho
    fullBody[0]->pUp.pos =
        mirrorPoint(original[1].up);
    fullBody[0]->pCenter.pos =
        mirrorPoint(original[1].center);
    fullBody[0]->pBottom.pos =
        mirrorPoint(original[1].bottom);

    // Brazo derecho recibe la pose reflejada del izquierdo
    fullBody[1]->pUp.pos =
        mirrorPoint(original[0].up);
    fullBody[1]->pCenter.pos =
        mirrorPoint(original[0].center);
    fullBody[1]->pBottom.pos =
        mirrorPoint(original[0].bottom);

    // Intercambiar las poses reflejadas de las piernas
    // Pierna izquierda recibe la pose reflejada de la derecha
    fullBody[2]->pUp.pos =
        mirrorPoint(original[3].up);
    fullBody[2]->pCenter.pos =
        mirrorPoint(original[3].center);
    fullBody[2]->pBottom.pos =
        mirrorPoint(original[3].bottom);

    // Pierna derecha recibe la pose reflejada de la izquierda
    fullBody[3]->pUp.pos =
        mirrorPoint(original[2].up);
    fullBody[3]->pCenter.pos =
        mirrorPoint(original[2].center);
    fullBody[3]->pBottom.pos =
        mirrorPoint(original[2].bottom);

    // Reflejar la spine, conservando su pBottom como eje
    fullBody[4]->pUp.pos =
        mirrorPoint(original[4].up);
    fullBody[4]->pCenter.pos =
        mirrorPoint(original[4].center);
    // pBottom no cambia

    // Actualizar todas las curvas
    for (auto& part : fullBody){
        part->update();
    }

    // Reflejar la cabeza por su centro
    auto headPos = head.getPosition();
    float radio = head.getRadius();

    float centroX = headPos.x + radio;
    float nuevoCentroX = 2.f * ejeX - centroX;

    head.setPosition(
        nuevoCentroX - radio,
        headPos.y
    );
}
void scaleAllBody(
    std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    sf::CircleShape& head,
    float factor
){
    // Centro de escala:
    // pBottom de la spine
    sf::Vector2f centro = fullBody[4]->pBottom.pos;

    // Escalar todos los puntos del cuerpo
    for (auto& part : fullBody){

        auto escalar = [&](sf::Vector2f& p){

            p.x = centro.x + (p.x - centro.x) * factor;
            p.y = centro.y + (p.y - centro.y) * factor;
        };

        escalar(part->pUp.pos);
        escalar(part->pCenter.pos);

        // El pBottom de la spine es el centro/eje,
        // por eso no se mueve.
        if (part.get() != fullBody[4].get()){
            escalar(part->pBottom.pos);
        }

        part->update();
    }

    // Escalar la cabeza respecto al mismo centro
    auto headPos = head.getPosition();

    float radio = head.getRadius();

    // Centro real de la cabeza
    sf::Vector2f centroHead(
        headPos.x + radio,
        headPos.y + radio
    );

    // Mover el centro de la cabeza respecto
    // al centro de escala
    centroHead.x =
        centro.x + (centroHead.x - centro.x) * factor;

    centroHead.y =
        centro.y + (centroHead.y - centro.y) * factor;

    // Escalar radio
    float nuevoRadio = radio * factor;

    head.setRadius(nuevoRadio);

    // SFML mantiene el origen anterior; en tu caso
    // estamos usando la esquina como posición.
    head.setPosition(
        centroHead.x - nuevoRadio,
        centroHead.y - nuevoRadio
    );
}
void selectCtrlMulti(
    std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    int idx,
    ControlPoint* tipoCtrl
){
    ControlPoint* ctrl = tipoCtrl;

    // Alternar selección
    ctrl->multiSelected = !ctrl->multiSelected;

    // Actualizar visualmente
    fullBody[idx]->update();
}
void copySelectedPose(
    const std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    std::vector<CopiedControl>& copiedPose,
    const sf::CircleShape& head // <--- Necesitamos la cabeza para copiar su posición si se mueve la spine up
){
    copiedPose.clear();

    for (int i = 0; i < static_cast<int>(fullBody.size()); ++i){
        const auto& part = fullBody[i];

        bool isSpine = (i == 4);

        if (part->pUp.multiSelected || (isSpine && part->pUp.multiSelected)){
            copiedPose.push_back({ i, 0, part->pUp.pos });
        }

        if (part->pCenter.multiSelected){
            copiedPose.push_back({ i, 1, part->pCenter.pos });
        }

        if (part->pBottom.multiSelected || (isSpine && part->pBottom.multiSelected)){
            copiedPose.push_back({ i, 2, part->pBottom.pos });
        }
    }

    // EXCEPCIÓN: Si la espina superior (spine up) está seleccionada, aseguramos 
    // incluir también lógicamente los hombros/brazos y la cabeza si forman parte del contexto.
    // O si prefieres forzarlo siempre que la spine up esté multiseleccionada:
    if (fullBody.size() > 4 && fullBody[4]->pUp.multiSelected) {
        // Opcional: Podrías registrar datos adicionales o dejar que el pegado inteligente lo maneje.
    }
}
void pasteSelectedPose(
    std::vector<std::unique_ptr<QuadraticBezier>>& fullBody,
    const std::vector<CopiedControl>& copiedPose,
    sf::CircleShape& head // <--- Se pasa la cabeza para poder actualizarla
){
    // Guardamos de forma temporal las posiciones anteriores o calculamos los deltas si es necesario,
    // o aplicamos directamente la lógica de desplazamiento/actualización:
    for (const auto& item : copiedPose){

        if (item.bodyIndex < 0 ||
            item.bodyIndex >= static_cast<int>(fullBody.size())){
            continue;
        }

        auto& part = fullBody[item.bodyIndex];

        // Calculamos el desplazamiento (delta) si queremos mover de forma relativa o asignamos directo.
        // Aquí aplicamos la asignación directa y las excepciones de la Spine:
        if (item.controlType == 0){
            sf::Vector2f oldPos = part->pUp.pos;
            part->pUp.pos = item.position;
            sf::Vector2f delta = part->pUp.pos - oldPos;

            // EXCEPCIÓN SPINE UP: Si es la columna y se movió el pUp, arrastra brazos superiores y cabeza
            if (item.bodyIndex == 4) {
                head.setPosition(head.getPosition().x + delta.x, head.getPosition().y + delta.y);
                
                // Brazo izquierdo y derecho superiores (índices 0 y 1)
                if (fullBody.size() > 1) {
                    fullBody[0]->pUp.pos += delta;
                    fullBody[0]->update();
                    fullBody[1]->pUp.pos += delta;
                    fullBody[1]->update();
                }
            }

        }else if (item.controlType == 1){

            part->pCenter.pos = item.position;

        }else if (item.controlType == 2){
            sf::Vector2f oldPos = part->pBottom.pos;
            part->pBottom.pos = item.position;
            sf::Vector2f delta = part->pBottom.pos - oldPos;

            // EXCEPCIÓN SPINE DOWN / BOTTOM: Si es la columna y se movió el pBottom, arrastra piernas
            if (item.bodyIndex == 4) {
                if (fullBody.size() > 3) {
                    fullBody[2]->pUp.pos += delta; // Pierna izquierda
                    fullBody[2]->update();
                    fullBody[3]->pUp.pos += delta; // Pierna derecha
                    fullBody[3]->update();
                }
            }
        }

        part->update();
    }
}
struct dataAssetKeyframe {
    float x, y;
    float rotation;
    float scaleX, scaleY;
};

struct AssetControl {
    std::string name;
    sf::RectangleShape shape;
    
    // Lista de keyframes visuales en el timeline para este asset específico
    std::map<float, sf::RectangleShape> assetKeylist;
    
    // Datos guardados por posición de fotograma para este asset
    std::map<float, dataAssetKeyframe> assetDataList;
};
bool cargarAnimacion(
    const std::string& ruta,
    std::map<float, sf::RectangleShape>& frameKeylist,
    std::map<float, dataBodyKey>& bodyKeyList,
    std::map<float, dataElipce>& dataHead,
    std::vector<AssetControl>& globalAssets,
    std::map<float, data_audio>& audioKeyList,
    std::map<float, sf::RectangleShape>& loopKeylist,
    sf::RectangleShape& currentFrame,
    sf::CircleShape& head,
    float& maxCreatedOffset
) {
    try {
        // 1. Abrir y leer el archivo
        std::ifstream archivo(ruta);

        if (!archivo.is_open()) {
            std::cerr << "No se pudo abrir: " << ruta << '\n';
            return false;
        }

        json data;
        archivo >> data;
        archivo.close();

        // 2. Limpiar la animación anterior
        frameKeylist.clear();
        bodyKeyList.clear();
        dataHead.clear();
        globalAssets.clear();
        audioKeyList.clear();
        loopKeylist.clear();

        // 3. Restaurar el máximo de frames
        maxCreatedOffset =
            data.value("maxCreatedOffset", 0.0f);

        // 4. Reconstruir las definiciones de los assets
        if (data.contains("assetDefinitions")) {
            for (const auto& item : data["assetDefinitions"]) {
                AssetControl asset;

                asset.name = item.at("name").get<std::string>();

                float width = item.value("width", 20.0f);
                float height = item.value("height", 20.0f);

                asset.shape.setSize({width, height});

                asset.shape.setOrigin({
                    item.value("originX", 0.0f),
                    item.value("originY", 0.0f)
                });

                asset.shape.setFillColor(sf::Color(
                    item.value("colorR", 255),
                    item.value("colorG", 255),
                    item.value("colorB", 255),
                    item.value("colorA", 255)
                ));

                globalAssets.push_back(std::move(asset));
            }
        }

        // 5. Restaurar los keyframes
        if (data.contains("frames")) {
            for (const auto& frame : data["frames"]) {
                float pos = frame.at("position").get<float>();

                // Solo agregar al timeline principal si el frame
                // contiene datos del personaje.
                if (frame.contains("body") || frame.contains("head")) {
                    sf::RectangleShape key({3.0f, 25.0f});
                    key.setPosition(pos, 0.0f);
                    key.setFillColor(sf::Color(255, 50, 50));

                    frameKeylist[pos] = key;
                }
                // Cabeza
                if (frame.contains("head")) {
                    const auto& h = frame["head"];

                    dataHead[pos] = {
                        h.at("x").get<float>(),
                        h.at("y").get<float>(),
                        h.at("radius").get<float>()
                    };
                }

                // Cuerpo
                if (frame.contains("body")) {
                    dataBodyKey bodyKey;

                    for (const auto& part : frame["body"]) {
                        attr_body attr;

                        attr.pUp_x =
                            part.at("pUp_x").get<float>();
                        attr.pUp_y =
                            part.at("pUp_y").get<float>();

                        attr.pCenter_x =
                            part.at("pCenter_x").get<float>();
                        attr.pCenter_y =
                            part.at("pCenter_y").get<float>();

                        attr.pBottom_x =
                            part.at("pBottom_x").get<float>();
                        attr.pBottom_y =
                            part.at("pBottom_y").get<float>();

                        attr.allFront =
                            part.value("allFront", false);

                        bodyKey.dataAttr.push_back(attr);
                    }

                    bodyKeyList[pos] = bodyKey;
                }

                // Datos de los assets para este frame
                if (frame.contains("assets")) {
                    for (const auto& saved : frame["assets"]) {
                        std::string name =
                            saved.at("name").get<std::string>();

                        // Encontrar el asset por su nombre
                        auto itAsset = std::find_if(
                            globalAssets.begin(),
                            globalAssets.end(),
                            [&](const AssetControl& asset) {
                                return asset.name == name;
                            }
                        );

                        // Ignorar assets que no tengan definición
                        if (itAsset == globalAssets.end()) {
                            std::cerr
                                << "Asset sin definición: "
                                << name << '\n';
                            continue;
                        }

                        dataAssetKeyframe keyData;

                        keyData.x =
                            saved.at("x").get<float>();
                        keyData.y =
                            saved.at("y").get<float>();
                        keyData.rotation =
                            saved.at("rotation").get<float>();
                        keyData.scaleX =
                            saved.at("scaleX").get<float>();
                        keyData.scaleY =
                            saved.at("scaleY").get<float>();

                        itAsset->assetDataList[pos] = keyData;

                        // Crear el key visual del asset
                        sf::RectangleShape assetKey({3.0f, 25.0f});
                        assetKey.setPosition(pos, 0.0f);
                        assetKey.setFillColor(sf::Color(100, 200, 100));

                        itAsset->assetKeylist[pos] = assetKey;
                    }
                }
            }
        }

        // 6. Restaurar los audios
        if (data.contains("audios")) {
            for (const auto& saved : data["audios"]) {
                float pos =
                    saved.at("position").get<float>();

                data_audio audio;
                audio.path_audio =
                    saved.at("path").get<std::string>();

                std::string rutaAudio = audio.path_audio;

                // Si el JSON solo guarda el nombre del archivo,
                // buscarlo dentro de la carpeta audio.
                if (!fs::exists(rutaAudio)) {
                    rutaAudio = "audio/" + audio.path_audio;
                }

                if (!audio.soundBufferCache.loadFromFile(rutaAudio)) {
                    std::cerr
                        << "No se pudo cargar el audio: "
                        << rutaAudio << '\n';
                    continue;
                }

                audio.key_audio.setSize({3.0f, 15.0f});
                audio.key_audio.setPosition(pos, 15.0f);
                audio.key_audio.setFillColor(sf::Color(10,10,180));
                audioKeyList[pos] = std::move(audio);
            }
        }

        // 7. Restaurar los marcadores del loop
        // Este bloque supone que cada elemento de "markers"
        // contiene un campo "position".
        if (data.contains("markers")) {
            for (const auto& saved : data["markers"]) {
                float pos =
                    saved.at("position").get<float>();

                sf::RectangleShape marker({3.0f, 15.0f});
                marker.setPosition(pos, 25);
                marker.setFillColor(sf::Color(10,180 , 180));
                loopKeylist[pos] = marker;
            }
        }

        // 8. Posicionar el cursor en el primer keyframe
        if (!frameKeylist.empty()) {
            float firstFrame = frameKeylist.begin()->first;

            currentFrame.setPosition(
                firstFrame,
                currentFrame.getPosition().y
            );

            // Actualizar la cabeza visible
            auto itHead = dataHead.find(firstFrame);
            if (itHead != dataHead.end()) {
                head.setPosition(
                    itHead->second.x,
                    itHead->second.y
                );
                head.setRadius(itHead->second.radius);
            }
        }
        /*
        std::cout << "Animacion cargada correctamente: "
                  << ruta << '\n';

        std::cout << "Frames: " << frameKeylist.size()
                  << " | Cuerpo: " << bodyKeyList.size()
                  << " | Assets: " << globalAssets.size()
                  << " | Audios: " << audioKeyList.size()
                  << '\n';
        */
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error al cargar JSON: "
                  << e.what() << '\n';
        return false;
    }
}
void newKeyCurrentAsset(const sf::RectangleShape& currentFrame, 
  AssetControl* currentAsset) {
    
    if (!currentAsset) return;

    auto posCurrentk = currentFrame.getPosition();
    float framePos = posCurrentk.x;

    // Obter os dados atuais de transformação da forma (shape) do asset[cite: 1]
    dataAssetKeyframe kData = {
        currentAsset->shape.getPosition().x,
        currentAsset->shape.getPosition().y,
        currentAsset->shape.getRotation(),
        currentAsset->shape.getScale().x,
        currentAsset->shape.getScale().y
    };

    // Verificar se já existe um keyframe neste frame exato para este asset[cite: 1]
    auto it = currentAsset->assetDataList.find(framePos);

    if (it != currentAsset->assetDataList.end()) {
        // Se já existe, atualizamos os valores de posição, rotação e escala[cite: 1]
        it->second = kData;
    } else {
        // Se não existe, criamos um novo registro de dados[cite: 1]
        currentAsset->assetDataList[framePos] = kData;

        // Criar a barra visual indicadora no timeline do asset (cor laranja)[cite: 1]
        sf::RectangleShape key(sf::Vector2f(3, 25));
        key.setPosition(framePos, currentFrame.getPosition().y);
        key.setFillColor(sf::Color(255, 165, 0)); 
        currentAsset->assetKeylist[framePos] = key;
    }
}
void updateCurrentAsset(AssetControl* currentAsset, float framePos) {
    if (!currentAsset) return;

    // Busca os dados do asset no fotograma atual
    auto it = currentAsset->assetDataList.find(framePos);
    if (it != currentAsset->assetDataList.end()) {
        const auto& keyData = it->second;

        // Aplica os valores salvos de posição, rotação e escala ao shape do asset
        currentAsset->shape.setPosition(keyData.x, keyData.y);
        currentAsset->shape.setRotation(keyData.rotation);
        currentAsset->shape.setScale(keyData.scaleX, keyData.scaleY);
    }
}
void updatePosAllAssets(std::vector<AssetControl>& globalAssets, float framePos) {
    for (auto& asset : globalAssets) {
        // Busca si el asset tiene un keyframe registrado en esta posición exacta
        auto it = asset.assetDataList.find(framePos);
        if (it != asset.assetDataList.end()) {
            const auto& keyData = it->second;

            // Actualiza la posición, rotación y escala de la forma del asset
            asset.shape.setPosition(keyData.x, keyData.y);
            asset.shape.setRotation(keyData.rotation);
            asset.shape.setScale(keyData.scaleX, keyData.scaleY);
        }
    }
}
struct CopiedPoseRangeFrame {
    float frameOffset; // Distancia en frames respecto a start_range
    std::vector<CopiedControl> pose;
    sf::Vector2f headPos;
    float headRadius;
};

struct CopiedPoseRangeData {
    bool hasData = false;
    float originalDuration = 0.f;
    std::vector<CopiedPoseRangeFrame> frames;
};

// Variable global para guardar el rango copiado
CopiedPoseRangeData copiedPoseRange;
int main() {
  float altura_ventana= 750.f; 
  float ancho_ventana= 455.f; 
  float altura_scena = 640.f; 
  float ancho_scena = 360.f; 
  float altura_timeline= 60.f; 
  float ancho_timeline= 455.f; 
  float altura_info= 30.f; 
  float ancho_info= 455.f; 
  float altura_buscador= 200.f; 
  float ancho_buscador= 300.f; 
  float pagina = 450.f;
  float pageOffset = 0;
  bool volumenAudio = false;
  float maxCreatedOffset = 0;
  bool idxTimeline = false;
  sf::RenderWindow window(sf::VideoMode(ancho_ventana,altura_ventana), "Bézier: Rotación de Manecillas - SFML");
  sf::View viewEscena(sf::FloatRect(0, 0, ancho_scena, altura_scena));
  sf::View viewTimeline(sf::FloatRect(0, 0, ancho_timeline, altura_timeline));
  sf::View viewInfo(sf::FloatRect(0, 0, ancho_info, altura_info));
  sf::View viewBuscador(sf::FloatRect(0, 0, ancho_buscador, altura_buscador));
  viewEscena.setViewport(sf::FloatRect(0.f, 0.f,
     ancho_scena/ancho_ventana,altura_scena/altura_ventana)); 
  viewTimeline.setViewport(sf::FloatRect(0,645.f/altura_ventana,
      ancho_timeline / ancho_ventana , altura_timeline/altura_ventana
  ));
  viewInfo.setViewport(
    sf::FloatRect(0,720.f/altura_ventana,
    ancho_info / ancho_ventana , altura_info/altura_ventana
  ));
  viewBuscador.setViewport(
    sf::FloatRect(50.f/ancho_buscador,10.f/altura_ventana,
    ancho_buscador / ancho_ventana , altura_buscador/altura_ventana
  ));

  sf::RectangleShape drawScena(sf::Vector2f(ancho_scena, altura_scena));
  drawScena.setFillColor(sf::Color(255, 252, 252));
  drawScena.setOutlineThickness(-2.f);
  drawScena.setOutlineColor(sf::Color::Blue);
  drawScena.setPosition(0.f, 0.f);
  
  sf::RectangleShape drawTimeline(sf::Vector2f(ancho_timeline, altura_timeline));
  drawTimeline.setFillColor(sf::Color(92, 92, 92));
  drawTimeline.setOutlineThickness(-2.f);
  drawTimeline.setOutlineColor(sf::Color(120,120,120));
  drawTimeline.setPosition(0.f, 0.f);    
  
  sf::RectangleShape drawInfo(sf::Vector2f(ancho_info, altura_info));
  drawInfo.setFillColor(sf::Color(30, 30, 30));
  drawInfo.setOutlineThickness(-2.f);
  drawInfo.setOutlineColor(sf::Color(120,120,120));
  drawInfo.setPosition(0.f, 0.f);    
  sf::RectangleShape drawBuscador(sf::Vector2f(ancho_buscador, altura_buscador));
  drawBuscador.setFillColor(sf::Color(30, 30, 30));
  drawBuscador.setOutlineThickness(-2.f);
  drawBuscador.setOutlineColor(sf::Color(120,120,120));
  drawBuscador.setPosition(0.f, 0.f);    
  float forceValue = 5.f;
  int moveState = 1;
  bool mark_loop = false;
  float tangentStep = 0.05f;
  sf::CircleShape head;
  head.setRadius(50);
  head.setFillColor(sf::Color(92,92,92));
  head.setPosition(20, 20);
  sf::RectangleShape head_ctrl;
  head_ctrl.setOrigin(6.f, 6.f);
  head_ctrl.setSize(sf::Vector2f(10,10));
  head_ctrl.setFillColor(sf::Color(180, 20, 50));
  head_ctrl.setPosition(20, 20);
  sf::RectangleShape allBody_ctrl;
  allBody_ctrl.setOrigin(6.f, 6.f);
  allBody_ctrl.setSize(sf::Vector2f(10,10));
  allBody_ctrl.setFillColor(sf::Color(180, 20, 50));
  allBody_ctrl.setPosition(20, 20);
  bool other_ctrl = false;
  sf::CircleShape headOnion;
  headOnion.setRadius(50);
  headOnion.setFillColor(sf::Color(120,120,120,80));
  headOnion.setPosition(20, 20);
  sf::RectangleShape currentFrame(sf::Vector2f(10,30));
  float forceMove_frame = 20.f;
  currentFrame.setFillColor(sf::Color(80,250,80,100));
  currentFrame.setPosition(5,0);
  std::vector<std::unique_ptr<QuadraticBezier>> fullBody;
  std::vector<CopiedControl> copiedPose;
  std::vector<PinnedControl> pinnedControls;
  // Brazo Izquierdo (Gris con detalles naranjas en la mano)
  fullBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 120), sf::Vector2f(120, 180), sf::Color(79, 79, 79), sf::Color(220, 100, 0)));
  // Brazo Derecho (Gris con detalles azules en la mano)
  fullBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 120), sf::Vector2f(20, 180), sf::Color(79, 79, 79), sf::Color(0, 100, 220)));
  // Pierna Izquierda (Gris con detalles naranjas en el pie)
  fullBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 220), sf::Vector2f(120, 270), sf::Color(50, 50, 50), sf::Color(220, 100, 0)));
  // Pierna Derecha (Gris con detalles azules en el pie)
  fullBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 220), sf::Vector2f(20, 270), sf::Color(50, 50, 50), sf::Color(0, 100, 220)));
  // Torso / Columna (Gris neutro)
  fullBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 120), sf::Vector2f(70, 220), sf::Color(100, 100, 100), sf::Color(100, 100, 100)));
  QuadraticBezier* preSelect = fullBody[4].get();
  //actualizar el puntero cada ves q modificar globalAssets 
  //por ejemplo al agreagr un nuevo asset o eliminar un asset de globalAssets
  AssetControl* currentAsset= nullptr;
    
  fullBody[4]->selectedPoint = &fullBody[4]->pCenter;
  fullBody[4]->update();
  //creando una lista de keyframes
  std::vector<std::unique_ptr<QuadraticBezier>> onionBody;
  std::vector<AssetControl> globalAssets; // <--- Así de simple, sin punteros dinámicos
  std::string selectedAssetTarget = "";
  onionBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 120), sf::Vector2f(120, 180), sf::Color(100, 100, 100), sf::Color(100, 100, 100)));
  onionBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 120), sf::Vector2f(20, 180), sf::Color(100, 100, 100), sf::Color(100, 100, 100)));
  onionBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 220), sf::Vector2f(120, 270), sf::Color(100, 100, 100), sf::Color(100, 100, 100)));
  onionBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 220), sf::Vector2f(20, 270), sf::Color(100, 100, 100), sf::Color(100, 100, 100)));
  onionBody.push_back(std::make_unique<QuadraticBezier>(sf::Vector2f(70, 120), sf::Vector2f(70, 220), sf::Color(100, 100, 100), sf::Color(100, 100, 100)));
  std::map <float,dataBodyKey> bodyKeyList; 
  std::map <float,dataElipce> dataHead;
  std::map<float , sf::RectangleShape> frameKeylist;
  std::map<float , data_audio> audioKeyList;
  std::map<float ,sf::RectangleShape> loopKeylist;
  sf::Sound activeSound;
  bool isPause= false;
  //time animation play
  sf::Clock playClock;
  sf::Clock durationClock;
  sf::Clock mensajeClock;
  float frameTime = 1.f / 24.f;
  bool slowPlay = false;
  float start_loop = 1.f;
  float end_loop = 10.f;
  bool viewOnion = false;
  bool viewPathPoint = false;
  bool autoPath = false;
  sf::RectangleShape visualPoint;
  visualPoint.setOrigin(6.f, 6.f);
  visualPoint.setSize(sf::Vector2f(5,5));
  visualPoint.setFillColor(sf::Color(10,180,180));
  visualPoint.setOutlineThickness(-0.99f); 
  visualPoint.setOutlineColor(sf::Color(128, 128, 128)); // Color plomo / gris
    
  std::vector<sf::Vector2f> trajectoryPoints;
  bool viewOnionRange = false;
  bool isPlay = false;
  int selectMode = 0;
  float maxFramePosition = pageOffset + 450 - 5;
  bool isRender = false;
  float start_render = 5.f;
  float end_render = maxFramePosition;
  sf::Color grisColor(90,90,90,80);
  sf::Color redColor(100,10,10,80);
  sf::Color blueColor(10,10,180,80);
  bool rangeSelect = false;
  float start_range = 1;
  float end_range = 2;
  sf::Font font;
  if (!font.loadFromFile(
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
  {
      std::cout << "No se pudo cargar la fuente\n";
  }
  sf::Text timeMensaje;
  timeMensaje.setCharacterSize(20);
  timeMensaje.setFillColor(sf::Color(150, 150, 150));
  timeMensaje.setPosition(10, 0);
  timeMensaje.setFont(font);
  std::stringstream ss;
  ss << "select all  : " << forceValue; 
  bool viewTimeMensaje = false;
  //buscador_config 
  std::vector<std::string> names;

  std::string query = "";
  int selectAudio = 0;
  bool openBuscador = false;
  bool justOpenBuscador = false;
  int selectedResult = 0;
  std::vector<std::string> filteredResults;
  std::string select_action = "none";
  std::string current_file_save = "none";
  bool ejecutar_action = false;
  bool update_lista_buscador = true;
  bool zoomMode = false;
  bool multiSelectMode = false;

  //circle bezier variables 

    // 3. Variables de Estado RGB independientes para Relleno y Borde
    int fillR = 255, fillG = 100, fillB = 150;
    int outlineR = 255, outlineG = 255, outlineB = 255;

    // Variables actuales que mueven los sliders (inicializadas con Relleno)
    int r = fillR, g = fillG, b = fillB;
    bool draggingR = false, draggingG = false, draggingB = false;

    // Modo actual: 0 para Relleno (Fill), 1 para Borde (Outline)
    int currentMode = 0; 

    // Propiedades del círculo central
    float circleRadius = 75.f;
    sf::Vector2f circleCenter(100.f, 550.f);

    // Configuración de los sliders
    float trackX = 60.f;
    float trackWidth = 256.f;
    float trackHeight = 12.f;
    bool create_circle_bezier = false;
  while (window.isOpen()) {
    sf::Event event;
    filteredResults.clear();
    std::string lowerQuery = toLower(query);
    // Si estamos reproduciendo el preview por los saltos de frame, evaluamos si ya pasaron 0.5s
    if (update_lista_buscador){
      update_lista_buscador = false;
      if (selectAudio == 1){
        names = loadAudioFiles();
      }else if (selectAudio == 2 ){
        names = loadAnimationFile();
      }else if (selectAudio == 4 ){
        names.clear();
        for (const auto& asset : globalAssets) {
            // Evitamos agregar nombres repetidos a la lista de sugerencias
            if (std::find(names.begin(), names.end(), asset.name) == names.end()) {
                names.push_back(asset.name);
            }
        }
      }else if (selectAudio == 3 ){
        names = loadAnimationFile();
      }else if (selectAudio == 5){
            names.clear();
            // Agregamos la opción por defecto para regresar al personaje
            names.push_back("[ Personaje Principal ]");
            
            // Agregamos los nombres de todos los assets creados
            for (const auto& asset : globalAssets) {
                names.push_back(asset.name);
            }
      }else{
        names = {
            "play Render", "load animation", "save animation","aMid pos", 
            "Scene render", "audio keyframe","add frames",
            "paste range" , "aOnion","rOnion", "delete range", "right move",
            "volumen audio","left move", "marks key","delete mark","Mid range",
            "loop play" , "path ctrl", "zoom x4","front part", "range render",
            "aPath" , "aZoom x2", "flip pose", "mirror pose","multiseleccion",
            "copy pose" , "paste pose" , "paste mirror" , "extend audio",
            "remove bloque frames" , "slow x2", "remove key","move 10",
            "retime x2", "retime /2" , "add assets" , "select asset", 
            "pin controles" , "pin move range", "copy pose range", "paste pose range",
            "paste pose range mirror", "head pin", "circle bezier"
        };
      }
    }
    if (!query.empty()){
        for (const auto& name : names){

            if (toLower(name).find(lowerQuery) != std::string::npos){
                filteredResults.push_back(name);
            }
        }
    }
    
    if (selectedResult >= (int)filteredResults.size()){
        selectedResult = 0;
    }
    // --- Cálculo dinámico de la altura ---
    size_t limit = std::min(filteredResults.size(), (size_t)4);
    float dynamicHeight = 65.0f;

    if (!filteredResults.empty()) {
        dynamicHeight = 75.0f + (limit * 35.0f);
    }
    // Tamaño del rectángulo
    drawBuscador.setSize(
        sf::Vector2f(ancho_buscador, dynamicHeight)
    );
    // Tamaño de la vista
    viewBuscador.setSize(
        ancho_buscador,
        dynamicHeight
    );
    // Mantener el origen de la vista en 0,0
    viewBuscador.setCenter(
        ancho_buscador / 2.f,
        dynamicHeight / 2.f
    );
    // Posición y tamaño del viewport
    viewBuscador.setViewport(sf::FloatRect(
        50.f / ancho_ventana,
        10.f / altura_ventana,
        ancho_buscador / ancho_ventana,
        dynamicHeight / altura_ventana
    ));

    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed){
          window.close();
      }
      if (create_circle_bezier){
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldMouse(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                // Delimitadores de los sliders
                sf::FloatRect rectR(trackX, 150.f, trackWidth, trackHeight);
                sf::FloatRect rectG(trackX, 230.f, trackWidth, trackHeight);
                sf::FloatRect rectB(trackX, 310.f, trackWidth, trackHeight);

                if (rectR.contains(worldMouse)) draggingR = true;
                if (rectG.contains(worldMouse)) draggingG = true;
                if (rectB.contains(worldMouse)) draggingB = true;

                // Delimitador para cambiar de modo
                
                sf::FloatRect modeToggleRect(90.f, 420.f, 240.f, 40.f);
                if (modeToggleRect.contains(worldMouse)) {
                    // 1. Guardar valores actuales antes de cambiar
                    if (currentMode == 0) {
                        fillR = r; fillG = g; fillB = b;
                    } else {
                        outlineR = r; outlineG = g; outlineB = b;
                    }

                    // 2. Alternar modo
                    currentMode = 1 - currentMode; 

                    // 3. Cargar valores correspondientes al nuevo modo en los sliders
                    if (currentMode == 0) {
                        r = fillR; g = fillG; b = fillB;
                    } else {
                        r = outlineR; g = outlineG; b = outlineB;
                    }
                }
            }
        }

        if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                draggingR = false;
                draggingG = false;
                draggingB = false;
            }
        }
      }
      if (openBuscador && event.type == sf::Event::TextEntered){
          if (justOpenBuscador){
            justOpenBuscador = false;
          }else if (event.text.unicode < 128 && event.text.unicode >= 32){
              query += static_cast<char>(event.text.unicode);
              selectedResult = 0;
          }
      }
      if (event.type == sf::Event::KeyPressed) {
        if (openBuscador){
          if (event.key.code == sf::Keyboard::Escape){
              openBuscador = false;
              query.clear();
              selectedResult = 0;
          }else if (event.key.code == sf::Keyboard::Backspace){
              if (!query.empty()){
                  query.pop_back();
                  selectedResult = 0;
              }

          }else if (event.key.code == sf::Keyboard::Enter){

              // =========================================
              // GUARDAR ANIMACIÓN
              // =========================================
              if (selectAudio == 2){
                    if (!query.empty()){

                        std::string nombreArchivo = query;

                        if (nombreArchivo.size() < 5 ||
                            nombreArchivo.substr(nombreArchivo.size() - 5) != ".json"){
                            nombreArchivo += ".json";
                        }

                        std::string rutaCompleta = "saveAnim/" + nombreArchivo;

                        if (!fs::exists("saveAnim")){
                            fs::create_directory("saveAnim");
                        }

                        try {

                            json data;
                            data["maxCreatedOffset"] = maxCreatedOffset;
                            
                            // 1. Guardar Audios
                            for (const auto& pair : audioKeyList){
                                float framePos = pair.first;
                                json audioJson;
                                audioJson["position"] = framePos;
                                audioJson["path"] = pair.second.path_audio;
                                data["audios"].push_back(audioJson);
                            }
                            
                            // 2. Guardar Markers (Loop)
                            for (const auto& pair : loopKeylist){
                                float framePos = pair.first;
                                json markerJson;
                                markerJson["position"] = framePos;
                                data["markers"].push_back(markerJson);
                            }

                    // =========================================
                    // 3. RECOPILAR TODOS LOS FOTOGRAMAS
                    // =========================================
                    std::set<float> allFrames;

                    // Timeline principal
                    for (const auto& [framePos, key] : frameKeylist) {
                        allFrames.insert(framePos);
                    }

                    // Keyframes del cuerpo
                    for (const auto& [framePos, key] : bodyKeyList) {
                        allFrames.insert(framePos);
                    }

                    // Keyframes de la cabeza
                    for (const auto& [framePos, key] : dataHead) {
                        allFrames.insert(framePos);
                    }

                    // Keyframes de cada asset
                    for (const auto& asset : globalAssets) {
                        for (const auto& [framePos, key] : asset.assetDataList) {
                            allFrames.insert(framePos);
                        }
                    }

                    // =========================================
                    // 4. GUARDAR TODOS LOS KEYFRAMES
                    // =========================================
                    data["frames"] = json::array();

                    for (float framePos : allFrames) {
                        json frame;
                        frame["position"] = framePos;

                        // -------------------------------------
                        // CABEZA
                        // -------------------------------------
                        auto itHead = dataHead.find(framePos);

                        if (itHead != dataHead.end()) {
                            frame["head"] = {
                                {"x", itHead->second.x},
                                {"y", itHead->second.y},
                                {"radius", itHead->second.radius}
                            };
                        }

                        // -------------------------------------
                        // CUERPO COMPLETO
                        // -------------------------------------
                        auto itBody = bodyKeyList.find(framePos);

                        if (itBody != bodyKeyList.end()) {
                            frame["body"] = json::array();

                            for (const auto& attr : itBody->second.dataAttr) {
                                frame["body"].push_back({
                                    {"pUp_x", attr.pUp_x},
                                    {"pUp_y", attr.pUp_y},

                                    {"pCenter_x", attr.pCenter_x},
                                    {"pCenter_y", attr.pCenter_y},

                                    {"pBottom_x", attr.pBottom_x},
                                    {"pBottom_y", attr.pBottom_y},

                                    // Importante: conservar el orden de dibujo
                                    {"allFront", attr.allFront}
                                });
                            }
                        }

                        // -------------------------------------
                        // ASSETS
                        // -------------------------------------
                        frame["assets"] = json::array();

                        for (const auto& asset : globalAssets) {
                            auto itAsset = asset.assetDataList.find(framePos);

                            if (itAsset == asset.assetDataList.end()) {
                                continue;
                            }

                            const auto& attr = itAsset->second;

                            frame["assets"].push_back({
                                {"name", asset.name},
                                {"x", attr.x},
                                {"y", attr.y},
                                {"rotation", attr.rotation},
                                {"scaleX", attr.scaleX},
                                {"scaleY", attr.scaleY}
                            });
                        }

                        data["frames"].push_back(frame);
                    }
                            // =========================================
                            // DEFINICIÓN DE LOS ASSETS
                            // =========================================
                            data["assetDefinitions"] = json::array();

                            for (const auto& asset : globalAssets) {
                                auto size = asset.shape.getSize();
                                auto origin = asset.shape.getOrigin();
                                auto color = asset.shape.getFillColor();

                                data["assetDefinitions"].push_back({
                                    {"name", asset.name},

                                    {"width", size.x},
                                    {"height", size.y},

                                    {"originX", origin.x},
                                    {"originY", origin.y},

                                    {"colorR", color.r},
                                    {"colorG", color.g},
                                    {"colorB", color.b},
                                    {"colorA", color.a}
                                });
                            }
                            std::ofstream jsonFile(rutaCompleta);

                            if (jsonFile.is_open()){

                                jsonFile << data.dump(4);
                                jsonFile.close();

                                ss.str("");
                                ss.clear();
                                current_file_save = nombreArchivo;
                                ss << "Guardado: " << nombreArchivo << " :)";

                                setMensajeTime(
                                    ss,
                                    timeMensaje,
                                    viewTimeMensaje,
                                    mensajeClock
                                );

                            }else{

                                throw std::runtime_error(
                                    "No se pudo abrir el archivo"
                                );
                            }

                        }catch (const std::exception& e){

                            std::cout << e.what() << std::endl;

                            ss.str("");
                            ss.clear();

                            ss << "Error al guardar JSON :(";

                            setMensajeTime(
                                ss,
                                timeMensaje,
                                viewTimeMensaje,
                                mensajeClock
                            );
                        }

                        // Cerrar el input
                        openBuscador = false;
                        selectAudio = 0;
                        query.clear();
                        selectedResult = 0;

                    }
              }else if (selectAudio == 5){
                if (!filteredResults.empty()){
                    // Obtenemos el nombre seleccionado de la lista filtrada
                    std::string elegido = filteredResults[selectedResult];

                    if (elegido == "[ Personaje Principal ]"){
                        moveState = 1;
                        selectMode = 1;
                        // Volvemos a editar el cuerpo/personaje principal
                        selectedAssetTarget = ""; 
                        
                        ss.str("");
                        ss.clear();
                        ss << "Modo activo: Personaje Principal";
                    } else {
                        // Seleccionamos el asset específico para editar sus keyframes y posición
                        selectMode = 4;
                        moveState = 3;
                        selectedAssetTarget = elegido; 
                        auto it = std::find_if(globalAssets.begin(), globalAssets.end(), [selectedAssetTarget](const AssetControl& ac){
                             return ac.name == selectedAssetTarget;
                        });
                        if (it != globalAssets.end()) {
                             currentAsset= &(*it);
                        }
                        ss.str("");
                        ss.clear();
                        ss << "Modo activo Asset: " << it->name;
                    }

                    // Mostramos el mensaje en pantalla
                    setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);

                    // Cerramos el buscador y reseteamos estados
                    openBuscador = false;
                    selectAudio = 0;
                    query.clear();
                    selectedResult = 0;
                }
              }else if (selectAudio == 4){ // Estado para nombrar el asset
                    if (!query.empty()){
                        std::string assetName = query;

                        // Verificar si ya existe un asset con ese nombre
                        auto it = std::find_if(globalAssets.begin(), globalAssets.end(), [&assetName](const AssetControl& ac){
                            return ac.name == assetName;
                        });

                        if (it != globalAssets.end()){
                            // El asset ya existe: mostramos mensaje y no dejamos crearlo
                            ss.str("");
                            ss.clear();
                            ss << "Error: El asset '" << assetName << "' ya existe :(";
                            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                        } else {
                            // El asset no existe: lo creamos
                            sf::RectangleShape newRect(sf::Vector2f(20.f, 30.f));
                            newRect.setOrigin(10.f, 15.f); // Centro para rotar/escalar mejor
                            newRect.setFillColor(sf::Color(92, 92, 50)); // Color distintivo para assets
                            newRect.setPosition(180.f, 320.f); // Posición fija inicial en la escena

                            selectMode = 4;
                            moveState = 3;
                            AssetControl nuevoAsset;
                            nuevoAsset.name = assetName;
                            nuevoAsset.shape = newRect;

                            // Crear automáticamente su primer keyframe en el fotograma actual
                            float framePos = currentFrame.getPosition().x;
                            
                            dataAssetKeyframe kData = {
                                newRect.getPosition().x,
                                newRect.getPosition().y,
                                newRect.getRotation(),
                                newRect.getScale().x,
                                newRect.getScale().y
                            };
                            nuevoAsset.assetDataList[framePos] = kData;

                            // Crear la barrita visual en el timeline del asset
                            sf::RectangleShape key(sf::Vector2f(3, 25));
                            key.setPosition(framePos, currentFrame.getPosition().y);
                            key.setFillColor(sf::Color(255, 165, 0)); // Color naranja para los keys de assets
                            nuevoAsset.assetKeylist[framePos] = key;

                            // Guardarlo en tu lista global de assets activos en la escena
                            globalAssets.push_back(nuevoAsset);

                            // Opcionalmente, puedes hacer que se seleccione automáticamente al crearlo
                            selectedAssetTarget = assetName;
                            currentAsset = &globalAssets.back(); 

                            ss.str("");
                            ss.clear();
                            ss << "Asset '" << assetName << "' creado con exito :)";
                            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                        }

                        openBuscador = false;
                        selectAudio = 0;
                        query.clear();
                        selectedResult = 0;
                    }
              }else if (selectAudio == 1){

                  if (!filteredResults.empty()){

                      select_action = filteredResults[selectedResult];

                      openBuscador = false;
                      selectAudio = 0;

                      std::string rutaCompleta =
                          "audio/" + select_action;

                      auto posCurrentk = currentFrame.getPosition();

                      auto kau = audioKeyList.find(posCurrentk.x);

                      if (kau != audioKeyList.end()){

                          kau->second.path_audio = rutaCompleta;

                          kau->second.soundBufferCache
                              .loadFromFile(rutaCompleta);

                      }else{

                          sf::RectangleShape keyAudio(
                              sf::Vector2f(3,15)
                          );

                          keyAudio.setPosition(
                              posCurrentk.x,
                              posCurrentk.y + 15
                          );

                          keyAudio.setFillColor(
                              sf::Color(10,10,180)
                          );

                          sf::SoundBuffer newBuffer;

                          if (newBuffer.loadFromFile(rutaCompleta)){

                              data_audio k_audio = {
                                  rutaCompleta,
                                  keyAudio,
                                  newBuffer
                              };

                              audioKeyList.insert({
                                  posCurrentk.x,
                                  k_audio
                              });

                          }
                      }
                  }

              //cargar scena
              }else if (selectAudio == 3){
                openBuscador = false;
                selectAudio = 0;
                if (!filteredResults.empty()){
                  select_action = filteredResults[selectedResult];
                  
                  // Corrección: Añadir la ruta completa de la carpeta "saveAnim/"
                  std::string ruta ="saveAnim/" + select_action;

                    if (cargarAnimacion(
                            ruta,
                            frameKeylist,
                            bodyKeyList,
                            dataHead,
                            globalAssets,
                            audioKeyList,
                            loopKeylist,
                            currentFrame,
                            head,
                            maxCreatedOffset
                        )) {

                      pageOffset = maxCreatedOffset;
                      int paginaIndex = static_cast<int>(pageOffset);
                      if ((paginaIndex / 450) % 2 == 0) {
                          idxTimeline = false;
                          maxFramePosition = pageOffset + 450 - 5;
                      } else {
                          idxTimeline = true;
                          maxFramePosition = pageOffset + 450 - 15;
                      }
                        // Actualizar el personaje con la pose del frame actual

                        auto pos = currentFrame.getPosition();
                        currentFrame.setPosition(maxFramePosition,pos.y);
                    }
                }
              }else{

                  if (!filteredResults.empty()){

                      select_action = filteredResults[selectedResult];

                      openBuscador = false;

                      ejecutar_action = true;
                  }
              }
          }
        }else{
          if (event.key.code == sf::Keyboard::S){
            newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
          }else if (event.key.code == sf::Keyboard::Q){
            
            std::string nombreArchivo = current_file_save;

            if (nombreArchivo.size() < 5 ||
                nombreArchivo.substr(nombreArchivo.size() - 5) != ".json"){
                nombreArchivo += ".json";
            }

            std::string rutaCompleta = "saveAnim/" + nombreArchivo;

            if (!fs::exists("saveAnim")){
                fs::create_directory("saveAnim");
            }

            try {

                json data;
                data["maxCreatedOffset"] = maxCreatedOffset;
                
                // 1. Guardar Audios
                for (const auto& pair : audioKeyList){
                    float framePos = pair.first;
                    json audioJson;
                    audioJson["position"] = framePos;
                    audioJson["path"] = pair.second.path_audio;
                    data["audios"].push_back(audioJson);
                }
                
                // 2. Guardar Markers (Loop)
                for (const auto& pair : loopKeylist){
                    float framePos = pair.first;
                    json markerJson;
                    markerJson["position"] = framePos;
                    data["markers"].push_back(markerJson);
                }

            // =========================================
            // 3. RECOPILAR TODOS LOS FOTOGRAMAS
            // =========================================
            std::set<float> allFrames;

            // Timeline principal
            for (const auto& [framePos, key] : frameKeylist) {
                allFrames.insert(framePos);
            }

            // Keyframes del cuerpo
            for (const auto& [framePos, key] : bodyKeyList) {
                allFrames.insert(framePos);
            }

            // Keyframes de la cabeza
            for (const auto& [framePos, key] : dataHead) {
                allFrames.insert(framePos);
            }

            // Keyframes de cada asset
            for (const auto& asset : globalAssets) {
                for (const auto& [framePos, key] : asset.assetDataList) {
                    allFrames.insert(framePos);
                }
            }

            // =========================================
            // 4. GUARDAR TODOS LOS KEYFRAMES
            // =========================================
            data["frames"] = json::array();

            for (float framePos : allFrames) {
                json frame;
                frame["position"] = framePos;

                // -------------------------------------
                // CABEZA
                // -------------------------------------
                auto itHead = dataHead.find(framePos);

                if (itHead != dataHead.end()) {
                    frame["head"] = {
                        {"x", itHead->second.x},
                        {"y", itHead->second.y},
                        {"radius", itHead->second.radius}
                    };
                }

                // -------------------------------------
                // CUERPO COMPLETO
                // -------------------------------------
                auto itBody = bodyKeyList.find(framePos);

                if (itBody != bodyKeyList.end()) {
                    frame["body"] = json::array();

                    for (const auto& attr : itBody->second.dataAttr) {
                        frame["body"].push_back({
                            {"pUp_x", attr.pUp_x},
                            {"pUp_y", attr.pUp_y},

                            {"pCenter_x", attr.pCenter_x},
                            {"pCenter_y", attr.pCenter_y},

                            {"pBottom_x", attr.pBottom_x},
                            {"pBottom_y", attr.pBottom_y},

                            // Importante: conservar el orden de dibujo
                            {"allFront", attr.allFront}
                        });
                    }
                }

                // -------------------------------------
                // ASSETS
                // -------------------------------------
                frame["assets"] = json::array();

                for (const auto& asset : globalAssets) {
                    auto itAsset = asset.assetDataList.find(framePos);

                    if (itAsset == asset.assetDataList.end()) {
                        continue;
                    }

                    const auto& attr = itAsset->second;

                    frame["assets"].push_back({
                        {"name", asset.name},
                        {"x", attr.x},
                        {"y", attr.y},
                        {"rotation", attr.rotation},
                        {"scaleX", attr.scaleX},
                        {"scaleY", attr.scaleY}
                    });
                }

                data["frames"].push_back(frame);
            }
                    // =========================================
                    // DEFINICIÓN DE LOS ASSETS
                    // =========================================
                    data["assetDefinitions"] = json::array();

                    for (const auto& asset : globalAssets) {
                        auto size = asset.shape.getSize();
                        auto origin = asset.shape.getOrigin();
                        auto color = asset.shape.getFillColor();

                        data["assetDefinitions"].push_back({
                            {"name", asset.name},

                            {"width", size.x},
                            {"height", size.y},

                            {"originX", origin.x},
                            {"originY", origin.y},

                            {"colorR", color.r},
                            {"colorG", color.g},
                            {"colorB", color.b},
                            {"colorA", color.a}
                        });
                    }
                    std::ofstream jsonFile(rutaCompleta);

                    if (jsonFile.is_open()){

                        jsonFile << data.dump(4);
                        jsonFile.close();

                        ss.str("");
                        ss.clear();
                        current_file_save = nombreArchivo;
                        ss << "Guardado: " << nombreArchivo << " :)";

                        setMensajeTime(
                            ss,
                            timeMensaje,
                            viewTimeMensaje,
                            mensajeClock
                        );

                    }else{

                        throw std::runtime_error(
                            "No se pudo abrir el archivo"
                        );
                    }

                }catch (const std::exception& e){

                    std::cout << e.what() << std::endl;

                    ss.str("");
                    ss.clear();

                    ss << "Error al guardar JSON :(";

                    setMensajeTime(
                        ss,
                        timeMensaje,
                        viewTimeMensaje,
                        mensajeClock
                    );
                }

                // Cerrar el input
                openBuscador = false;
                selectAudio = 0;
                query.clear();
                selectedResult = 0;

          }else if (event.key.code == sf::Keyboard::V){
            moveState = 3; 
            selectMode = 3;
            ss.str("");
            ss.clear();
            ss << "ESCALA ON";
            setMensajeTime(ss,timeMensaje,viewTimeMensaje,mensajeClock);
          }else if (event.key.code == sf::Keyboard::Space){
            if (moveState == 1){ // move timeline
                auto pos = currentFrame.getPosition();
                currentFrame.setPosition(pos.x + forceMove_frame, pos.y);
                auto kh = dataHead.find(pos.x+forceMove_frame);
                auto kb = bodyKeyList.find(pos.x+forceMove_frame);
                if (kh != dataHead.end()){
                  head.setPosition(kh->second.x,kh->second.y);
                  head.setRadius(kh->second.radius); // <--- Restaurar la escala de la cabeza
                }
                if (kb != bodyKeyList.end()){
                  updatePosbody(fullBody , kb->second.dataAttr); 
                }
                for (auto& asset : globalAssets) {
                    auto itAssetData = asset.assetDataList.find(pos.x + forceMove_frame);
                    if (itAssetData != asset.assetDataList.end()) {
                        const auto& keyData = itAssetData->second;
                        
                        // Restaurar posição, rotação e escala salvas no keyframe[cite: 1]
                        asset.shape.setPosition(keyData.x, keyData.y);
                        asset.shape.setRotation(keyData.rotation);
                        asset.shape.setScale(keyData.scaleX, keyData.scaleY);
                    }
                }
                //actuaizar view timeline
                int frameInt = static_cast<int>(pos.x);
                pageOffset = (static_cast<float>(frameInt / 450)) * 450.f;
                int paginaIndex = static_cast<int>(pageOffset);
                if ((paginaIndex / 450) % 2 == 0) {
                    idxTimeline = false;
                } else {
                    idxTimeline = true;
                }
            }else if (moveState == 3 ){ //move eje X
              if (selectMode== 1){
                for (auto& part : fullBody){
                  part->pUp.pos.x += forceValue;
                  part->pCenter.pos.x += forceValue;
                  part->pBottom.pos.x += forceValue;
                  part->update();
                }
                auto pos = head.getPosition();
                head.setPosition(pos.x + forceValue ,pos.y);
                pos = allBody_ctrl.getPosition();
                allBody_ctrl.setPosition(pos.x + forceValue , pos.y);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode== 2){
                auto pos = head.getPosition();
                head.setPosition(pos.x + forceValue ,pos.y);
                pos = head_ctrl.getPosition();
                head_ctrl.setPosition(pos.x + forceValue , pos.y);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode== 3){
                scaleAllBody(fullBody, head, 0.95f);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode == 4){
                auto pos = currentAsset->shape.getPosition();
                currentAsset->shape.setPosition(pos.x + forceValue,pos.y);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else if (selectMode == 6){
                auto pos = currentAsset->shape.getRotation();
                currentAsset->shape.setRotation(pos + forceValue);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else if (selectMode == 5){
                auto pos = currentAsset->shape.getScale();
                currentAsset->shape.setScale(pos.x + forceValue,pos.y);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else{
                if(preSelect->selectedPoint  != nullptr){
                  preSelect->selectedPoint->pos.x += forceValue;
                  preSelect->update();
                  if (preSelect == fullBody[4].get()){
                    if (preSelect->selectedPoint == &preSelect->pUp){
                      auto pos = head.getPosition();
                      head.setPosition(pos.x + forceValue ,pos.y);
                      fullBody[1]->pUp.pos.x += forceValue;
                      fullBody[1]->update();
                      fullBody[0]->pUp.pos.x += forceValue;
                      fullBody[0]->update();
                    }else if (preSelect->selectedPoint == &preSelect->pBottom){
                      fullBody[2]->pUp.pos.x += forceValue;
                      fullBody[2]->update();
                      fullBody[3]->pUp.pos.x += forceValue;
                      fullBody[3]->update();
                    }
                  }
                  newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
                }
              }
              if (viewPathPoint) {
                actualizarTrayectoria(autoPath, currentFrame.getPosition().x, preSelect,fullBody, bodyKeyList, start_range, end_range, trajectoryPoints, viewPathPoint);
              }
            }else if (moveState == 2 ){ //move eje Y
              if (selectMode == 1){
                for (auto& part : fullBody){
                  part->pUp.pos.y += forceValue;
                  part->pCenter.pos.y += forceValue;
                  part->pBottom.pos.y += forceValue;
                  part->update();
                }
                auto pos = head.getPosition();
                head.setPosition(pos.x , pos.y + forceValue);
                pos = allBody_ctrl.getPosition();
                allBody_ctrl.setPosition(pos.x , pos.y + forceValue);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode== 2){
                auto pos = head.getPosition();
                head.setPosition(pos.x , pos.y + forceValue);
                pos = head_ctrl.getPosition();
                head_ctrl.setPosition(pos.x,pos.y + forceValue);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode == 4){
                auto pos = currentAsset->shape.getPosition();
                currentAsset->shape.setPosition(pos.x ,pos.y +forceValue);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else if (selectMode == 5){
                auto pos = currentAsset->shape.getScale();
                currentAsset->shape.setScale(pos.x,pos.y + forceValue);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else{
                if(preSelect->selectedPoint  != nullptr){
                  preSelect->selectedPoint->pos.y += forceValue;
                  preSelect->update();
                  if (preSelect == fullBody[4].get()){
                    if (preSelect->selectedPoint == &preSelect->pUp){
                      auto pos = head.getPosition();
                      head.setPosition(pos.x ,pos.y+ forceValue);
                      fullBody[1]->pUp.pos.y += forceValue;
                      fullBody[1]->update();
                      fullBody[0]->pUp.pos.y += forceValue;
                      fullBody[0]->update();
                    }else if (preSelect->selectedPoint == &preSelect->pBottom){
                      fullBody[2]->pUp.pos.y += forceValue;
                      fullBody[2]->update();
                      fullBody[3]->pUp.pos.y += forceValue;
                      fullBody[3]->update();
                    }
                  }
                  newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
                }
              }
              if (viewPathPoint) {
                actualizarTrayectoria(autoPath, currentFrame.getPosition().x, preSelect,fullBody, bodyKeyList, start_range, end_range, trajectoryPoints, viewPathPoint);
              }
            }else if (moveState == 4 ){ //force change
              forceValue += 1.f;
              ss.str("");
              ss.clear();
              ss << "force : " << forceValue; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }
          }else if (event.key.code == sf::Keyboard::Backspace){
            if (moveState == 1){
                auto pos = currentFrame.getPosition();
                currentFrame.setPosition(pos.x - forceMove_frame , pos.y);
                auto kh = dataHead.find(pos.x - forceMove_frame);
                if (kh != dataHead.end()){
                  head.setPosition(kh->second.x,kh->second.y);
                  head.setRadius(kh->second.radius); // <--- Restaurar la escala de la cabeza
                }
                auto kb = bodyKeyList.find(pos.x-forceMove_frame);
                if (kb != bodyKeyList.end()){
                  updatePosbody(fullBody , kb->second.dataAttr); 
                }
                for (auto& asset : globalAssets) {
                    auto itAssetData = asset.assetDataList.find(pos.x - forceMove_frame);
                    if (itAssetData != asset.assetDataList.end()) {
                        const auto& keyData = itAssetData->second;
                        
                        // Restaurar posição, rotação e escala salvas no keyframe[cite: 1]
                        asset.shape.setPosition(keyData.x, keyData.y);
                        asset.shape.setRotation(keyData.rotation);
                        asset.shape.setScale(keyData.scaleX, keyData.scaleY);
                    }
                }
                //actuaizar view timeline
                int frameInt = static_cast<int>(pos.x);
                pageOffset = (static_cast<float>(frameInt / 450)) * 450.f;
                int paginaIndex = static_cast<int>(pageOffset);
                if ((paginaIndex / 450) % 2 == 0) {
                    idxTimeline = false;
                } else {
                    idxTimeline = true;
                }
            }else if (moveState == 3){
              if (selectMode == 1){
                for (auto& part : fullBody){
                  part->pUp.pos.x -= forceValue;
                  part->pCenter.pos.x -= forceValue;
                  part->pBottom.pos.x -= forceValue;
                  part->update();
                }
                auto pos = head.getPosition();
                head.setPosition(pos.x - forceValue ,pos.y);              
                pos = allBody_ctrl.getPosition();
                allBody_ctrl.setPosition(pos.x - forceValue , pos.y);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode== 2){
                auto pos = head.getPosition();
                head.setPosition(pos.x - forceValue ,pos.y);              
                pos = head_ctrl.getPosition();
                head_ctrl.setPosition(pos.x - forceValue ,pos.y);              
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode== 3){
                scaleAllBody(fullBody,head,1.1f);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode == 4){
                auto pos = currentAsset->shape.getPosition();
                currentAsset->shape.setPosition(pos.x - forceValue,pos.y);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else if (selectMode == 6){
                auto pos = currentAsset->shape.getRotation();
                currentAsset->shape.setRotation(pos - forceValue);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else if (selectMode == 5){
                auto pos = currentAsset->shape.getScale();
                currentAsset->shape.setScale(pos.x -forceValue,pos.y);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else{
                if(preSelect->selectedPoint  != nullptr){
                  preSelect->selectedPoint->pos.x -= forceValue;
                  preSelect->update();
                  if (preSelect == fullBody[4].get()){
                    if (preSelect->selectedPoint == &preSelect->pUp){
                      auto pos = head.getPosition();
                      head.setPosition(pos.x - forceValue ,pos.y);
                      fullBody[1]->pUp.pos.x -= forceValue;
                      fullBody[1]->update();
                      fullBody[0]->pUp.pos.x -= forceValue;
                      fullBody[0]->update();
                    }else if (preSelect->selectedPoint == &preSelect->pBottom){
                      fullBody[2]->pUp.pos.x -= forceValue;
                      fullBody[2]->update();
                      fullBody[3]->pUp.pos.x -= forceValue;
                      fullBody[3]->update();
                    }
                  }
                  newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
                }
              }
              if (viewPathPoint) {
                actualizarTrayectoria(autoPath, currentFrame.getPosition().x, preSelect,fullBody, bodyKeyList, start_range, end_range, trajectoryPoints, viewPathPoint);
              }
            }else if (moveState == 2 ){
              if (selectMode == 1){
                for (auto& part : fullBody){
                  part->pUp.pos.y -= forceValue;
                  part->pCenter.pos.y -= forceValue;
                  part->pBottom.pos.y -= forceValue;
                  part->update();
                }
                auto pos = head.getPosition();
                head.setPosition(pos.x , pos.y - forceValue);            
                pos = allBody_ctrl.getPosition();
                allBody_ctrl.setPosition(pos.x , pos.y - forceValue);
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode== 2){
                auto pos = head.getPosition();
                head.setPosition(pos.x , pos.y - forceValue);            
                pos = head_ctrl.getPosition();
                head_ctrl.setPosition(pos.x , pos.y - forceValue);            
                newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
              }else if (selectMode == 4){
                auto pos = currentAsset->shape.getPosition();
                currentAsset->shape.setPosition(pos.x ,pos.y -forceValue);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else if (selectMode == 5){
                auto pos = currentAsset->shape.getScale();
                currentAsset->shape.setScale(pos.x ,pos.y - forceValue);
                newKeyCurrentAsset(currentFrame, currentAsset);
              }else{
                if(preSelect->selectedPoint  != nullptr){
                  preSelect->selectedPoint->pos.y -= forceValue;
                  preSelect->update();
                  if (preSelect == fullBody[4].get()){
                    if (preSelect->selectedPoint == &preSelect->pUp){
                      auto pos = head.getPosition();
                      head.setPosition(pos.x ,pos.y- forceValue);
                      fullBody[1]->pUp.pos.y -= forceValue;
                      fullBody[1]->update();
                      fullBody[0]->pUp.pos.y -= forceValue;
                      fullBody[0]->update();
                    }else if (preSelect->selectedPoint == &preSelect->pBottom){
                      fullBody[2]->pUp.pos.y -= forceValue;
                      fullBody[2]->update();
                      fullBody[3]->pUp.pos.y -= forceValue;
                      fullBody[3]->update();
                    }
                  }
                  newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
                }
              }
              if (viewPathPoint) {
                actualizarTrayectoria(autoPath, currentFrame.getPosition().x, preSelect,fullBody, bodyKeyList, start_range, end_range, trajectoryPoints, viewPathPoint);
              }
            }else if (moveState == 4 ){
              forceValue -= 1.f;
              ss.str("");
              ss.clear();
              ss << "force : " << forceValue; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }
          }else if (event.key.code == sf::Keyboard::Semicolon){
              moveState = 1;
              volumenAudio = false;
              forceMove_frame = 20.f;
          }else if (event.key.code == sf::Keyboard::N){
            if (multiSelectMode){
                selectCtrlMulti(fullBody,1,&fullBody[1]->pBottom);
            }else{
                selectCtrl(fullBody,preSelect,1,&fullBody[1]->pBottom,selectMode);
            }
          }else if (event.key.code == sf::Keyboard::Tab){
            if (multiSelectMode){
                selectCtrlMulti(fullBody,1,&fullBody[1]->pCenter);
            }else{
                selectCtrl(fullBody,preSelect,1,&fullBody[1]->pCenter,selectMode);
            }
          }else if (event.key.code == sf::Keyboard::B){
            if (multiSelectMode){
                selectCtrlMulti(fullBody,4,&fullBody[4]->pBottom);
            }else{
                selectCtrl(fullBody,preSelect,4,&fullBody[4]->pBottom,selectMode);
            }
          }else if (event.key.code == sf::Keyboard::P){
            if (selectedAssetTarget == ""){ 
                if (multiSelectMode){
                    selectCtrlMulti(fullBody,4,&fullBody[4]->pCenter);
                }else{
                    selectCtrl(fullBody,preSelect,4,&fullBody[4]->pCenter,selectMode);
                }
            }else{
              moveState = 2;
              selectMode = 4;
              forceValue = 5;
              ss.str("");
              ss.clear();
              ss << "asset mode : Move"; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }
          }else if (event.key.code == sf::Keyboard::O){
            if (selectedAssetTarget == ""){ 
                if (multiSelectMode){
                    selectCtrlMulti(fullBody,4,&fullBody[4]->pUp);
                }else{
                    selectCtrl(fullBody,preSelect,4,&fullBody[4]->pUp,selectMode);
                }
            }else{
              moveState = 3;
              selectMode = 6;
              forceValue = 1;
              ss.str("");
              ss.clear();
              ss << "asset mode : Rotate"; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }
               
          }else if (event.key.code == sf::Keyboard::U){
            if (selectedAssetTarget == ""){ 
              if (multiSelectMode){
                    selectCtrlMulti(fullBody,0,&fullBody[0]->pCenter);
              }else{
                    selectCtrl(fullBody,preSelect,0,&fullBody[0]->pCenter,selectMode);
              }
            }else{
              moveState = 2;
              selectMode = 5;
              forceValue = 1;
              ss.str("");
              ss.clear();
              ss << "asset mode : Scale"; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }
               
          }else if (event.key.code == sf::Keyboard::L){
                if (multiSelectMode){
                     selectCtrlMulti(fullBody,0,&fullBody[0]->pBottom);
                 }else{
                     selectCtrl(fullBody,preSelect,0,&fullBody[0]->pBottom,selectMode);
                 }
          }else if (event.key.code == sf::Keyboard::K){
            if (rangeSelect){
              auto pos = currentFrame.getPosition();
              start_range = pos.x;
              ss.str("");
              ss.clear();
              ss << "start : " << start_range; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }else {
                if (multiSelectMode){
                    selectCtrlMulti(fullBody,2,&fullBody[2]->pCenter);
                }else{
                    selectCtrl(fullBody,preSelect,2,&fullBody[2]->pCenter,selectMode);
                }
            }
          }else if (event.key.code == sf::Keyboard::J){
            if (rangeSelect){
              rangeSelect = false;
              auto pos = currentFrame.getPosition();
              end_range = pos.x;
              ss.str("");
              ss.clear();
              ss << "end : " << end_range; 
              setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            }else {
                if (multiSelectMode){
                    selectCtrlMulti(fullBody,2,&fullBody[2]->pBottom);
                }else{
                    selectCtrl(fullBody,preSelect,2,&fullBody[2]->pBottom,selectMode);
                }
                   }
          }else if (event.key.code == sf::Keyboard::G){

            if (multiSelectMode){
                selectCtrlMulti(fullBody,3,&fullBody[3]->pCenter);
            }else{
                selectCtrl(fullBody,preSelect,3,&fullBody[3]->pCenter,selectMode);
            }
                         
          }else if (event.key.code == sf::Keyboard::H){
            if (multiSelectMode){
                selectCtrlMulti(fullBody,3,&fullBody[3]->pBottom);
            }else{
                selectCtrl(fullBody,preSelect,3,&fullBody[3]->pBottom,selectMode);
            }
               
          }else if (event.key.code == sf::Keyboard::I){
            moveState = 3;
          }else if (event.key.code == sf::Keyboard::T){
            moveState = 2;
          }else if (event.key.code == sf::Keyboard::Enter){
            if (create_circle_bezier){
                create_circle_bezier = false;
                std::cout << fillR << fillG << fillB << std::endl; 
                std::cout <<outlineR<<  outlineG<<outlineB<< std::endl; 
                
            }else{
                selectMode = 1;
                if (preSelect){
                  preSelect->selectedPoint  = nullptr;
                  preSelect->update();
                }
                auto posHips = fullBody[4]->pBottom.pos;
                allBody_ctrl.setPosition(posHips.x , posHips.y + 20);
                ss.str("");
                ss.clear();
                ss << "select all  : " << "on"; 
                setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
             }
          }else if (event.key.code == sf::Keyboard::E){
            isPlay = true;
            playClock.restart();
            durationClock.restart();
            viewOnion = false;
            activeSound.pause(); 
            auto pos = currentFrame.getPosition();
            //actuaizar view timeline
            int frameInt = static_cast<int>(pos.x);
            pageOffset = (static_cast<float>(frameInt / 450)) * 450.f;
            int paginaIndex = static_cast<int>(pageOffset);
            if ((paginaIndex / 450) % 2 == 0) {
                idxTimeline = false;
                currentFrame.setPosition(pageOffset + 5, pos.y);
            } else {
                idxTimeline = true;
                currentFrame.setPosition(pageOffset + 15, pos.y);
            }
            sincronizarAudio(
                activeSound,
                audioKeyList,
                currentFrame.getPosition().x
            );
            pos = currentFrame.getPosition();
            auto machHead= dataHead.find(pos.x);
            if (machHead != dataHead.end()){
              head.setPosition(machHead->second.x, machHead->second.y);
              head.setRadius(machHead->second.radius); // <--- Restaurar la escala de la cabeza 
            }
            auto kb = bodyKeyList.find(pos.x);
            if (kb != bodyKeyList.end()){
              updatePosbody(fullBody , kb->second.dataAttr); 
            }
                
          }else if (event.key.code == sf::Keyboard::Escape){
            ejecutar_action = true;
          }else if (event.key.code == sf::Keyboard::A){
            rangeSelect = true;
            ss.str("");
            ss.clear();
            ss << "range -> start (k) " << " - end (j)"; 
            setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
          }else if (event.key.code == sf::Keyboard::M){
            openBuscador = true;
            query.clear();
            selectedResult = 0;
            justOpenBuscador = true;
            //modo acciones
            selectAudio = 0;
            update_lista_buscador = true;
          }else if (event.key.code == sf::Keyboard::D){
            ss.str("");
            ss.clear();
            ss << "force : " << "on"; 
            moveState = 4;
            setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
          }else if (event.key.code == sf::Keyboard::F){
            ss.str("");
            ss.clear();
            mark_loop = false;
            ss << "play loop : " << "off"; 
            setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
            
          }else if (event.key.code == sf::Keyboard::Up){
            if (volumenAudio){
              float currentVol = activeSound.getVolume();
              if (currentVol > 0.f) {
                  float newVol = std::max(0.f, currentVol - 5.f); // Baja de 5 en 5
                  activeSound.setVolume(newVol);
                  
                  ss.str("");
                  ss.clear();
                  ss << "Volumen: " << static_cast<int>(newVol) << "%";
                  setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
              }
            }else{
              // Solo retroceder si hay una página anterior (es decir, pageOffset > 0)
              if (pageOffset >= 450) {
                  pageOffset -= 450;
                  auto pos = currentFrame.getPosition();
                  // Recalcular los valores visuales para esta página existente
                  int paginaIndex = static_cast<int>(pageOffset);
                  if ((paginaIndex / 450) % 2 == 0) {
                      idxTimeline = false;
                      currentFrame.setPosition(pageOffset + 5, pos.y);
                  } else {
                      idxTimeline = true;
                      currentFrame.setPosition(pageOffset + 15, pos.y);
                  }
                  ss.str("");
                  ss.clear();
                  ss << "Vista anterior <-";
                  setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
              }
            }
          }else if (event.key.code == sf::Keyboard::Down){
            if (volumenAudio){
              float currentVol = activeSound.getVolume();
              if (currentVol < 100.f) {
                  float newVol = std::min(100.f, currentVol + 5.f); // Sube de 5 en 5
                  activeSound.setVolume(newVol);
                  
                  ss.str("");
                  ss.clear();
                  ss << "Volumen: " << static_cast<int>(newVol) << "%";
                  setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
              }
            }else{
              // Solo avanzar si la siguiente página ya fue creada previamente
              if (pageOffset + 450 <= maxCreatedOffset) {
                  pageOffset += 450;
                  auto pos = currentFrame.getPosition();
                  // Recalcular los valores visuales para esta página existente
                  int paginaIndex = static_cast<int>(pageOffset);
                  if ((paginaIndex / 450) % 2 == 0) {
                      idxTimeline = false;
                      currentFrame.setPosition(pageOffset + 5, pos.y);
                  } else {
                      idxTimeline = true;
                      currentFrame.setPosition(pageOffset + 15, pos.y);
                  }

                  ss.str("");
                  ss.clear();
                  ss << "Vista siguiente ->";
                  setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
              }
            }
          }else if (event.key.code == sf::Keyboard::R){
            isPlay = false;
            if (activeSound.getStatus() == sf::Sound::Playing) {
              activeSound.pause();
            }
            isPause = true;
          }else if (event.key.code == sf::Keyboard::C){
            auto posHead = head.getPosition();
            head_ctrl.setPosition(posHead.x+ 50, posHead.y);
            selectMode = 2;
            if (preSelect){
              preSelect->selectedPoint  = nullptr;
              preSelect->update();
            }
          }
        }
      }
    }
    if (ejecutar_action){
      ejecutar_action = false;
      window.setTitle("Anim: " + select_action);
      if (select_action == "play Render"){
        system("ffplay -autoexit /home/manuel/Documents/cppApps/animation/verb/animacion.mp4");
      }else if (select_action == "delete mark"){
        auto currFrame = currentFrame.getPosition();  
        auto it = loopKeylist.find(currFrame.x);
        if (it != loopKeylist.end()){
            it = loopKeylist.erase(it);
        }
      }else if (select_action == "Mid range"){
            // 1. Definir o recibir tus extremos de rango (ej. por variables de la UI, selección o parámetros)
            float startRange = start_range;
            float endRange = end_range;

            // 2. Verificar si existen keyframes exactamente en esos frames extremos
            auto startIt = bodyKeyList.find(startRange);
            auto endIt = bodyKeyList.find(endRange);

            if (startIt == bodyKeyList.end()) {
                ss.str("");
                ss.clear();
                ss << "mid pos: falta key en start range (" << startRange << ")";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else if (endIt == bodyKeyList.end()) {
                ss.str("");
                ss.clear();
                ss << "mid pos: falta key en end range (" << endRange << ")";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                // =========================================
                // Tenemos keys válidos en startRange y endRange
                // =========================================
                
                // Buscar parte y CTRL seleccionado
                if (preSelect == nullptr || preSelect->selectedPoint == nullptr) {
                    ss.str("");
                    ss.clear();
                    ss << "mid pos: no hay ctrl";
                    setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                } else {
                    int partIndex = -1;
                    for (size_t i = 0; i < fullBody.size(); ++i) {
                        if (fullBody[i].get() == preSelect) {
                            partIndex = static_cast<int>(i);
                            break;
                        }
                    }

                    int ctrlType = -1;
                    if (preSelect->selectedPoint == &preSelect->pUp) {
                        ctrlType = 0;
                    } else if (preSelect->selectedPoint == &preSelect->pCenter) {
                        ctrlType = 1;
                    } else if (preSelect->selectedPoint == &preSelect->pBottom) {
                        ctrlType = 2;
                    }

                    if (partIndex == -1 || ctrlType == -1) {
                        ss.str("");
                        ss.clear();
                        ss << "mid pos: ctrl invalido";
                        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                    } else {
                        // =====================================
                        // Obtener posiciones de los extremos
                        // =====================================
                        const auto& attrStart = startIt->second.dataAttr[partIndex];
                        const auto& attrEnd = endIt->second.dataAttr[partIndex];

                        sf::Vector2f posStart;
                        sf::Vector2f posEnd;

                        if (ctrlType == 0) {
                            posStart = sf::Vector2f(attrStart.pUp_x, attrStart.pUp_y);
                            posEnd   = sf::Vector2f(attrEnd.pUp_x, attrEnd.pUp_y);
                        } else if (ctrlType == 1) {
                            posStart = sf::Vector2f(attrStart.pCenter_x, attrStart.pCenter_y);
                            posEnd   = sf::Vector2f(attrEnd.pCenter_x, attrEnd.pCenter_y);
                        } else {
                            posStart = sf::Vector2f(attrStart.pBottom_x, attrStart.pBottom_y);
                            posEnd   = sf::Vector2f(attrEnd.pBottom_x, attrEnd.pBottom_y);
                        }

                        // =====================================
                        // Calcular la mitad entre el rango
                        // =====================================
                        sf::Vector2f midPos;
                        midPos.x = (posStart.x + posEnd.x) / 2.f;
                        midPos.y = (posStart.y + posEnd.y) / 2.f;

                        sf::Vector2f currentPos = preSelect->selectedPoint->pos;

                        float dx = midPos.x - currentPos.x;
                        float dy = midPos.y - currentPos.y;

                        // Mover CTRL seleccionado
                        preSelect->selectedPoint->pos.x += dx;
                        preSelect->selectedPoint->pos.y += dy;
                        preSelect->update();

                        // =====================================
                        // CASO SPINEUP (si aplica a tu estructura)
                        // =====================================
                        if (preSelect == fullBody[4].get()) {
                            if (preSelect->selectedPoint == &preSelect->pUp) {
                                auto posHead = head.getPosition();
                                head.setPosition(posHead.x + dx, posHead.y + dy);

                                fullBody[1]->pUp.pos.x += dx;
                                fullBody[1]->pUp.pos.y += dy;
                                fullBody[1]->update();

                                fullBody[0]->pUp.pos.x += dx;
                                fullBody[0]->pUp.pos.y += dy;
                                fullBody[0]->update();
                            } else if (preSelect->selectedPoint == &preSelect->pBottom) {
                                fullBody[2]->pUp.pos.x += dx;
                                fullBody[2]->pUp.pos.y += dy;
                                fullBody[2]->update();

                                fullBody[3]->pUp.pos.x += dx;
                                fullBody[3]->pUp.pos.y += dy;
                                fullBody[3]->update();
                            }
                        }

                        // Guardar resultado como keyframe en el frame actual
                        newKey(currentFrame, head, frameKeylist, dataHead, bodyKeyList, fullBody);

                        // Mensaje de éxito
                        ss.str("");
                        ss.clear();
                        ss << "mid pos (rango): " << startRange << " -> " << currentFrame.getPosition().x << " -> " << endRange;
                        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                    }
                }
            }
      }else if (select_action == "aMid pos"){
            float currentX =currentFrame.getPosition().x;
            auto rightIt = bodyKeyList.upper_bound(currentX);

            // 2. Buscar el keyframe por la izquierda (lower_bound y luego retroceder)
            auto leftIt = bodyKeyList.lower_bound(currentX);
            if (leftIt != bodyKeyList.begin()) {
                --leftIt;
                // Si por alguna razón el lower_bound cayó exactamente en currentX, retrocedemos uno más
                if (leftIt->first == currentX && leftIt != bodyKeyList.begin()) {
                    --leftIt;
                }
            }

            // Validar si encontramos un keyframe válido a la izquierda
            if (leftIt == bodyKeyList.begin() && leftIt->first >= currentX) {
                ss.str("");
                ss.clear();
                ss << "mid pos: falta key izquierda";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                // Verificar que exista un keyframe a la derecha
                if (rightIt == bodyKeyList.end()) {
                    ss.str("");
                    ss.clear();
                    ss << "mid pos: falta key derecha";
                    setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                } else {
                    // =========================================
                    // Tenemos:
                    // leftIt  = key izquierda válida
                    // rightIt = key derecha válida
                    // =========================================

                    float frameLeft = leftIt->first;
                    float frameRight = rightIt->first;
                    // -----------------------------------------
                    // Buscar parte y CTRL seleccionado
                    // -----------------------------------------

                    if (preSelect == nullptr ||
                        preSelect->selectedPoint == nullptr){

                        ss.str("");
                        ss.clear();

                        ss << "mid pos: no hay ctrl";

                        setMensajeTime(
                            ss,
                            timeMensaje,
                            viewTimeMensaje,
                            mensajeClock
                        );

                    }else{

                        int partIndex = -1;

                        for (size_t i = 0;
                             i < fullBody.size();
                             ++i){

                            if (fullBody[i].get() ==
                                preSelect){

                                partIndex =
                                    static_cast<int>(i);

                                break;
                            }
                        }

                        // -----------------------------------------
                        // Determinar CTRL
                        // -----------------------------------------

                        int ctrlType = -1;

                        if (preSelect->selectedPoint ==
                            &preSelect->pUp){

                            ctrlType = 0;

                        }else if (
                            preSelect->selectedPoint ==
                            &preSelect->pCenter){

                            ctrlType = 1;

                        }else if (
                            preSelect->selectedPoint ==
                            &preSelect->pBottom){

                            ctrlType = 2;
                        }

                        if (partIndex == -1 ||
                            ctrlType == -1){

                            ss.str("");
                            ss.clear();

                            ss << "mid pos: ctrl invalido";

                            setMensajeTime(
                                ss,
                                timeMensaje,
                                viewTimeMensaje,
                                mensajeClock
                            );

                        }else{

                            // =====================================
                            // Obtener posición izquierda
                            // =====================================

                            const auto& attrLeft =
                                leftIt->second.dataAttr[partIndex];

                            const auto& attrRight =
                                rightIt->second.dataAttr[partIndex];

                            sf::Vector2f posLeft;
                            sf::Vector2f posRight;

                            if (ctrlType == 0){

                                posLeft = sf::Vector2f(
                                    attrLeft.pUp_x,
                                    attrLeft.pUp_y
                                );

                                posRight = sf::Vector2f(
                                    attrRight.pUp_x,
                                    attrRight.pUp_y
                                );

                            }else if (ctrlType == 1){

                                posLeft = sf::Vector2f(
                                    attrLeft.pCenter_x,
                                    attrLeft.pCenter_y
                                );

                                posRight = sf::Vector2f(
                                    attrRight.pCenter_x,
                                    attrRight.pCenter_y
                                );

                            }else{

                                posLeft = sf::Vector2f(
                                    attrLeft.pBottom_x,
                                    attrLeft.pBottom_y
                                );

                                posRight = sf::Vector2f(
                                    attrRight.pBottom_x,
                                    attrRight.pBottom_y
                                );
                            }

                            // =====================================
                            // Calcular mitad
                            // =====================================

                            sf::Vector2f midPos;

                            midPos.x =
                                (posLeft.x + posRight.x) / 2.f;

                            midPos.y =
                                (posLeft.y + posRight.y) / 2.f;

                            // =====================================
                            // Posición actual del CTRL
                            // =====================================

                            sf::Vector2f currentPos =
                                preSelect->selectedPoint->pos;

                            // -------------------------------------
                            // Diferencia necesaria
                            // -------------------------------------

                            float dx =
                                midPos.x - currentPos.x;

                            float dy =
                                midPos.y - currentPos.y;

                            // =====================================
                            // Mover CTRL seleccionado
                            // =====================================

                            preSelect->selectedPoint->pos.x += dx;
                            preSelect->selectedPoint->pos.y += dy;

                            preSelect->update();

                            // =====================================
                            // CASO SPINEUP
                            // =====================================

                            if (preSelect == fullBody[4].get()){

                                if (preSelect->selectedPoint ==
                                    &preSelect->pUp){

                                    // Cabeza
                                    auto posHead =
                                        head.getPosition();

                                    head.setPosition(
                                        posHead.x + dx,
                                        posHead.y + dy
                                    );

                                    // Spine / partes relacionadas
                                    fullBody[1]->pUp.pos.x += dx;
                                    fullBody[1]->pUp.pos.y += dy;
                                    fullBody[1]->update();

                                    fullBody[0]->pUp.pos.x += dx;
                                    fullBody[0]->pUp.pos.y += dy;
                                    fullBody[0]->update();

                                }else if (
                                    preSelect->selectedPoint ==
                                    &preSelect->pBottom){

                                    fullBody[2]->pUp.pos.x += dx;
                                    fullBody[2]->pUp.pos.y += dy;
                                    fullBody[2]->update();

                                    fullBody[3]->pUp.pos.x += dx;
                                    fullBody[3]->pUp.pos.y += dy;
                                    fullBody[3]->update();
                                }
                            }

                            // =====================================
                            // Guardar resultado como keyframe
                            // =====================================

                            newKey(
                                currentFrame,
                                head,
                                frameKeylist,
                                dataHead,
                                bodyKeyList,
                                fullBody
                            );

                            // =====================================
                            // Mensaje
                            // =====================================

                            ss.str("");
                            ss.clear();

                            ss << "mid pos: "
                               << frameLeft
                               << " -> "
                               << currentX
                               << " -> "
                               << frameRight;

                            setMensajeTime(
                                ss,
                                timeMensaje,
                                viewTimeMensaje,
                                mensajeClock
                            );
                        }
                    }
                }
            } 
      }else if (select_action == "load animation"){
        openBuscador = true;
        query.clear();
        selectedResult = 0;
        justOpenBuscador = true;
        //cargamos la lista de json animacion de la carpeta saveAnimacion
        selectAudio = 3;
        update_lista_buscador = true;
      }else if (select_action == "save animation"){
        //primero abrimos el buscardor pero en modo de seleccion de audio
        openBuscador = true;
        query.clear();
        selectedResult = 0;
        justOpenBuscador = true;
        //cargamos la lista de json animacion de la carpeta saveAnimacion
        selectAudio = 2;
        update_lista_buscador = true;
      }else if (select_action == "Scene render"){
        if (fs::exists("render") && fs::is_directory("render")) {
            for (const auto& entry : fs::directory_iterator("render")) {
                fs::remove_all(entry.path());
            }
        } else {

            fs::create_directory("render");
        }
        isRender = true;
        start_render = 5.f;   
        end_render = maxFramePosition; 
      }else if (select_action == "range render"){
        start_render = start_range;
        end_render = end_range;
        if (fs::exists("render") && fs::is_directory("render")) {
            for (const auto& entry : fs::directory_iterator("render")) {
                fs::remove_all(entry.path());
            }
        } else {

            fs::create_directory("render");
        }
        isRender = true;
      }else if (select_action == "audio keyframe"){
        //primero abrimos el buscardor pero en modo de seleccion de audio
        openBuscador = true;
        query.clear();
        selectedResult = 0;
        justOpenBuscador = true;
        //cargamos la lista de audio
        selectAudio = 1;
        update_lista_buscador = true;
      }else if (select_action == "pin controles") {
            pinnedControls.clear();
            
            // Capturar todos los controles que estén multiseleccionados
            for (size_t i = 0; i < fullBody.size(); ++i) {
                if (fullBody[i]->pUp.multiSelected) {
                    pinnedControls.push_back({static_cast<int>(i), 0});
                    fullBody[i]->pUp.multiSelected = false;
                }
                if (fullBody[i]->pCenter.multiSelected) {
                    pinnedControls.push_back({static_cast<int>(i), 1});
                    fullBody[i]->pCenter.multiSelected = false;
                }
                if (fullBody[i]->pBottom.multiSelected) {
                    pinnedControls.push_back({static_cast<int>(i), 2});
                    fullBody[i]->pBottom.multiSelected = false;
                }
                fullBody[i]->update();
            }
            
            multiSelectMode = false;
            
            ss.str("");
            ss.clear();
            ss << "Pin controles: " << pinnedControls.size()<< " controles fijados";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);

      } else if (select_action == "pin move range") {
            if (pinnedControls.empty()) {
                ss.str("");
                ss.clear();
                ss << "No hay controles pineados :(";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                // Verificar que haya exactamente un control seleccionado como padre
                int activeSelections = 0;
                int parentBodyIndex = -1;
                int parentCtrlType = -1;

                // Comprobar selección individual (preSelect)
                if (preSelect != nullptr && preSelect->selectedPoint != nullptr) {
                    activeSelections++;
                    for (size_t i = 0; i < fullBody.size(); ++i) {
                        if (fullBody[i].get() == preSelect) {
                            parentBodyIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    if (preSelect->selectedPoint == &preSelect->pUp) parentCtrlType = 0;
                    else if (preSelect->selectedPoint == &preSelect->pCenter) parentCtrlType = 1;
                    else parentCtrlType = 2;
                }

                // Comprobar si hay multiselecciones activas por error
                for (size_t i = 0; i < fullBody.size(); ++i) {
                    if (fullBody[i]->pUp.multiSelected) { activeSelections++; parentBodyIndex = i; parentCtrlType = 0; }
                    if (fullBody[i]->pCenter.multiSelected) { activeSelections++; parentBodyIndex = i; parentCtrlType = 1; }
                    if (fullBody[i]->pBottom.multiSelected) { activeSelections++; parentBodyIndex = i; parentCtrlType = 2; }
                }

                // Si hay más de uno o ninguno seleccionado, mostrar el mensaje requerido
                if (activeSelections != 1) {
                    ss.str("");
                    ss.clear();
                    ss << "solo seleccione uno";
                    setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                } else {
                    float r_start = std::min(start_range, end_range);
                    float r_end = std::max(start_range, end_range);

                    // Buscar el keyframe exacto o inicial en start_range dentro de bodyKeyList
                    auto startIt = bodyKeyList.find(r_start);
                    if (startIt == bodyKeyList.end()) {
                        ss.str("");
                        ss.clear();
                        ss << "Falta key en start range (" << r_start << ")";
                        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                    } else {
                        // Obtener posición del padre en start_range
                        const auto& attrStart = startIt->second.dataAttr[parentBodyIndex];
                        sf::Vector2f parentPosStart;
                        if (parentCtrlType == 0) parentPosStart = {attrStart.pUp_x, attrStart.pUp_y};
                        else if (parentCtrlType == 1) parentPosStart = {attrStart.pCenter_x, attrStart.pCenter_y};
                        else parentPosStart = {attrStart.pBottom_x, attrStart.pBottom_y};

                        // Calcular el offset inicial de cada hijo respecto al padre en start_range
                        struct ChildOffset {
                            int bodyIndex;
                            int controlType;
                            sf::Vector2f offset;
                        };
                        std::vector<ChildOffset> childOffsets;
                        for (const auto& pin : pinnedControls) {
                            const auto& childAttrStart = startIt->second.dataAttr[pin.bodyIndex];
                            sf::Vector2f childPosStart;
                            if (pin.controlType == 0) childPosStart = {childAttrStart.pUp_x, childAttrStart.pUp_y};
                            else if (pin.controlType == 1) childPosStart = {childAttrStart.pCenter_x, childAttrStart.pCenter_y};
                            else childPosStart = {childAttrStart.pBottom_x, childAttrStart.pBottom_y};

                            childOffsets.push_back({pin.bodyIndex, pin.controlType, childPosStart - parentPosStart});
                        }

                        // Modificar los frames hacia adelante en el rango especificado
                        for (auto& [framePos, bodyData] : bodyKeyList) {
                            if (framePos >= r_start && framePos <= r_end) {
                                auto& attrCurrent = bodyData.dataAttr[parentBodyIndex];
                                sf::Vector2f parentPosCurrent;
                                if (parentCtrlType == 0) parentPosCurrent = {attrCurrent.pUp_x, attrCurrent.pUp_y};
                                else if (parentCtrlType == 1) parentPosCurrent = {attrCurrent.pCenter_x, attrCurrent.pCenter_y};
                                else parentPosCurrent = {attrCurrent.pBottom_x, attrCurrent.pBottom_y};

                                // Aplicar la variación de posición al hijo respetando su offset inicial
                                for (const auto& co : childOffsets) {
                                    sf::Vector2f newChildPos = parentPosCurrent + co.offset;
                                    auto& childAttrCurrent = bodyData.dataAttr[co.bodyIndex];
                                    if (co.controlType == 0) {
                                        childAttrCurrent.pUp_x = newChildPos.x;
                                        childAttrCurrent.pUp_y = newChildPos.y;
                                    } else if (co.controlType == 1) {
                                        childAttrCurrent.pCenter_x = newChildPos.x;
                                        childAttrCurrent.pCenter_y = newChildPos.y;
                                    } else {
                                        childAttrCurrent.pBottom_x = newChildPos.x;
                                        childAttrCurrent.pBottom_y = newChildPos.y;
                                    }
                                }
                            }
                        }

                        // Refrescar el cuerpo en pantalla con el frame actual si corresponde
                        float currentX = currentFrame.getPosition().x;
                        auto kb = bodyKeyList.find(currentX);
                        if (kb != bodyKeyList.end()) {
                            updatePosbody(fullBody, kb->second.dataAttr);
                        }

                        ss.str("");
                        ss.clear();
                        ss << "Pin move range aplicado con exito :)";
                        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                    }
                }
            }
      }else if (select_action == "delete range"){
          // Asegurar el orden correcto del rango por si se seleccionan de manera invertida
          float r_start = std::min(start_range, end_range);
          float r_end = std::max(start_range, end_range);
          // 1. Eliminar keyframes visuales del timeline
          for (auto it = frameKeylist.begin(); it != frameKeylist.end(); ) {
              if (it->first >= r_start && it->first <= r_end) {
                  it = frameKeylist.erase(it);
              } else {
                  ++it;
              }
          }
          // 2. Eliminar datos de posición de la cabeza
          for (auto it = dataHead.begin(); it != dataHead.end(); ) {
              if (it->first >= r_start && it->first <= r_end) {
                  it = dataHead.erase(it);
              } else {
                  ++it;
              }
          }
          // 3. Eliminar datos de posición del cuerpo completo (fullbody)
          for (auto it = bodyKeyList.begin(); it != bodyKeyList.end(); ) {
              if (it->first >= r_start && it->first <= r_end) {
                  it = bodyKeyList.erase(it);
              } else {
                  ++it;
              }
          }
          /*
          // Opcional: Limpiar también los audios asociados en ese rango
          for (auto it = audioKeyList.begin(); it != audioKeyList.end(); ) {
              if (it->first >= r_start && it->first <= r_end) {
                  it = audioKeyList.erase(it);
              } else {
                  ++it;
              }
          }
          */
          ss.str("");
          ss.clear();
          ss << "Rango eliminado: " << r_start << " - " << r_end;
          setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
      }else if (select_action == "paste range"){

        // Posición donde comenzará el rango pegado
        auto pastePos = currentFrame.getPosition();
        float pasteStart = pastePos.x;

        for (const auto& keyframe : bodyKeyList){

            float sourceFrame = keyframe.first;

            // Solo tomar los keyframes que están dentro del rango copiado
            if(sourceFrame >= start_range && sourceFrame <= end_range){

                // Offset que tenía el keyframe respecto al inicio del rango
                float offset = sourceFrame - start_range;

                // Nueva posición donde será pegado
                float targetFrame = pasteStart + offset;

                // -----------------------------
                // Cargar cabeza
                // -----------------------------
                auto machHead = dataHead.find(sourceFrame);

                if(machHead != dataHead.end()){
                    head.setPosition(
                        machHead->second.x,
                        machHead->second.y
                    );
                    head.setRadius(machHead->second.radius); // <--- Restaurar la escala de la cabeza
                }

                // -----------------------------
                // Cargar cuerpo
                // -----------------------------
                auto kb = bodyKeyList.find(sourceFrame);

                if(kb != bodyKeyList.end()){
                    updatePosbody(
                        fullBody,
                        kb->second.dataAttr
                    );
                }

                // -----------------------------
                // Colocar currentFrame en
                // la posición destino
                // -----------------------------
                auto pos = currentFrame.getPosition();

                currentFrame.setPosition(
                    targetFrame,
                    pos.y
                );

                // -----------------------------
                // Crear el nuevo keyframe
                // -----------------------------
                newKey(
                    currentFrame,
                    head,
                    frameKeylist,
                    dataHead,
                    bodyKeyList,
                    fullBody
                );
            }
        }

        // Dejar currentFrame al final del rango pegado
        auto pos = currentFrame.getPosition();
        currentFrame.setPosition(
            pasteStart + (end_range - start_range),
            pos.y
        );
      }else if (select_action == "aOnion"){
        if (viewOnion == true){
          viewOnion= false;
        }else{
          viewOnion= true;
        }
        viewOnionRange= false;
      }else if (select_action == "rOnion"){
        if (viewOnion == true){
          viewOnion= false;
        }else{
          viewOnion= true;
        }
        viewOnionRange= true;
      }else if (select_action == "right move"){
        
        float desde = currentFrame.getPosition().x;
        float cantidad = 20.f;
        moverFrameKeysDesde(frameKeylist, desde, cantidad);
        moverKeysDesde(bodyKeyList, desde, cantidad);
        moverKeysDesde(dataHead, desde, cantidad);

        ss.str("");
        ss.clear();
        ss<< "move ->";
        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);

      }else if (select_action == "left move"){
        float desde = currentFrame.getPosition().x;
        float cantidad = -20.f;
        moverFrameKeysDesde(frameKeylist, desde, cantidad);
        moverKeysDesde(bodyKeyList, desde, cantidad);
        moverKeysDesde(dataHead, desde, cantidad);
        ss.str("");
        ss.clear();
        ss<< "move <-";
        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);

      }else if (select_action == "add frames"){
        pageOffset = pageOffset + 450;
        // Guardamos o actualizamos hasta qué página máxima se ha extendido
        if (pageOffset > maxCreatedOffset) {
            maxCreatedOffset = pageOffset;
        }
        int paginaIndex = static_cast<int>(pageOffset);
        if ((paginaIndex/450)% 2 == 0){
          idxTimeline = false;
          maxFramePosition = pageOffset + 450 - 5;
        }else{
          idxTimeline = true;
          maxFramePosition = pageOffset + 450 - 15;
        }
      }else if (select_action == "remove key"){
        float currentX = currentFrame.getPosition().x;
        if (selectedAssetTarget == ""){ 
            // Buscar el keyframe exactamente en la posición actual
            auto frameIt = frameKeylist.find(currentX);
            if (frameIt != frameKeylist.end()){

                // ---------------------------------
                // Eliminar representación visual
                // ---------------------------------
                frameKeylist.erase(frameIt);

                // ---------------------------------
                // Eliminar datos de la cabeza
                // ---------------------------------
                dataHead.erase(currentX);

                // ---------------------------------
                // Eliminar datos del cuerpo
                // ---------------------------------
                bodyKeyList.erase(currentX);

                // ---------------------------------
                // Mensaje
                // ---------------------------------
                ss.str("");
                ss.clear();

                ss << "Keyframe eliminado: "
                   << currentX;

                setMensajeTime(
                    ss,
                    timeMensaje,
                    viewTimeMensaje,
                    mensajeClock
                );

            }else{

                // No existe keyframe en esa posición
                ss.str("");
                ss.clear();
                ss << "No hay keyframe en "<< currentX;
                setMensajeTime(
                    ss,
                    timeMensaje,
                    viewTimeMensaje,
                    mensajeClock
                );
            }
        }else{
          auto it = currentAsset->assetDataList.find(currentX);
          if (it != currentAsset->assetDataList.end()) {
                currentAsset->assetDataList.erase(currentX);
                currentAsset->assetKeylist.erase(currentX);
          } else {
                ss.str("");
                ss.clear();
                ss << "No hay keyframe en "<< currentX;
                setMensajeTime(ss,timeMensaje,viewTimeMensaje,mensajeClock);
          }
        }
      }else if (select_action == "add assets"){
            openBuscador = true;
            query.clear();
            selectedResult = 0;
            justOpenBuscador = true;
            selectAudio = 4; // Activamos el modo de captura de nombre de asset
            update_lista_buscador = true;
      }else if (select_action == "select asset"){
            openBuscador = true;
            query.clear();
            selectedResult = 0;
            justOpenBuscador = true;
            selectAudio = 5; // Activamos el selector de assets/personaje
            update_lista_buscador = true;
      }else if (select_action == "extend audio"){
            float currentX = currentFrame.getPosition().x;

            // Buscar un audio EXACTAMENTE en el current frame
            auto audioIt = audioKeyList.find(currentX);

            if (audioIt == audioKeyList.end()){

                ss.str("");
                ss.clear();

                ss << "No hay audio en frame "
                   << currentX;

                setMensajeTime(
                    ss,
                    timeMensaje,
                    viewTimeMensaje,
                    mensajeClock
                );

            }else{
                float duracionAudio =audioIt->second.soundBufferCache.getDuration().asSeconds();
                float framesAudio = duracionAudio * 240.f;
                int cantidadBloques =
                    static_cast<int>(
                        std::ceil(framesAudio / 450.f)
                    );
                // Agregar bloques : 

                for (int i = 0; i < cantidadBloques; ++i){

                    pageOffset += 450;

                    if (pageOffset > maxCreatedOffset) {
                        maxCreatedOffset = pageOffset;
                    }

                    int paginaIndex =
                        static_cast<int>(pageOffset);

                    if ((paginaIndex / 450) % 2 == 0){

                        idxTimeline = false;

                        maxFramePosition =
                            pageOffset + 450 - 5;

                    }else{

                        idxTimeline = true;

                        maxFramePosition =
                            pageOffset + 450 - 15;
                    }
                }

                // -----------------------------------------
                // Mensaje
                // -----------------------------------------

                ss.str("");
                ss.clear();

                ss << "time: "
                   << duracionAudio
                   << "/fr: "
                   << framesAudio
                   << "/bl: "
                   << cantidadBloques;

                setMensajeTime(
                    ss,
                    timeMensaje,
                    viewTimeMensaje,
                    mensajeClock
                );
            }
      }else if (select_action == "marks key"){
        auto pos = currentFrame.getPosition();        
        sf::RectangleShape keyLoop(
            sf::Vector2f(3,15)
        );

        keyLoop.setPosition(
            pos.x,
            pos.y + 30
        );

        keyLoop.setFillColor(
            sf::Color(10,180,180)
        );
        loopKeylist.insert({pos.x,keyLoop});
      }else if (select_action == "remove bloque frames"){

            // -----------------------------------------
            // Nunca eliminar el primer bloque
            // -----------------------------------------

            if (pageOffset <= 0){

                ss.str("");
                ss.clear();

                ss << "No hay bloques para eliminar";

                setMensajeTime(
                    ss,
                    timeMensaje,
                    viewTimeMensaje,
                    mensajeClock
                );

            }else{

                // -----------------------------------------
                // Quitar el ultimo bloque de 450
                // -----------------------------------------

                pageOffset -= 450;

                // -----------------------------------------
                // Actualizar limite maximo
                // -----------------------------------------

                maxCreatedOffset = pageOffset;

                // -----------------------------------------
                // Actualizar pagina / timeline
                // -----------------------------------------

                int paginaIndex =
                    static_cast<int>(pageOffset);

                if ((paginaIndex / 450) % 2 == 0){

                    idxTimeline = false;

                    maxFramePosition =
                        pageOffset + 450 - 5;

                }else{

                    idxTimeline = true;

                    maxFramePosition =
                        pageOffset + 450 - 15;
                }

                // -----------------------------------------
                // Mensaje
                // -----------------------------------------

                ss.str("");
                ss.clear();

                ss << "remove frames: bloque eliminado";

                setMensajeTime(
                    ss,
                    timeMensaje,
                    viewTimeMensaje,
                    mensajeClock
                );
            }
      }else if (select_action == "paste pose"){
        if (copiedPose.empty()){
            ss.str("");
            ss.clear();

            ss << "No hay pose copiada";

            setMensajeTime(
                ss,
                timeMensaje,
                viewTimeMensaje,
                mensajeClock
            );
        }else{
            pasteSelectedPose(fullBody,copiedPose, head);

            // Guardar solamente el resultado
            // dentro del keyframe actual
            newKey(
                currentFrame,
                head,
                frameKeylist,
                dataHead,
                bodyKeyList,
                fullBody
            );

            ss.str("");
            ss.clear();

            ss << "paste pose : "
               << copiedPose.size()
               << " ctrl";

            setMensajeTime(
                ss,
                timeMensaje,
                viewTimeMensaje,
                mensajeClock
            );
        }
      }else if (select_action == "copy pose range") {
            float r_start = std::min(start_range, end_range);
            float r_end = std::max(start_range, end_range);

            copiedPoseRange.frames.clear();
            copiedPoseRange.originalDuration = r_end - r_start;
            copiedPoseRange.hasData = false;

            // Recorrer todos los keyframes del cuerpo dentro del rango
            for (const auto& [framePos, bodyData] : bodyKeyList) {
                if (framePos >= r_start && framePos <= r_end) {
                    
                    // Simular temporalmente el cuerpo en este frame para extraer los controles multiseleccionados
                    updatePosbody(fullBody, bodyData.dataAttr);

                    // Obtener la cabeza de este frame si existe
                    auto itHead = dataHead.find(framePos);
                    sf::Vector2f hPos = head.getPosition();
                    float hRad = head.getRadius();
                    if (itHead != dataHead.end()) {
                        hPos = {itHead->second.x, itHead->second.y};
                        hRad = itHead->second.radius;
                    }

                    std::vector<CopiedControl> frameCopiedPose;
                    for (int i = 0; i < static_cast<int>(fullBody.size()); ++i) {
                        const auto& part = fullBody[i];
                        bool isSpine = (i == 4);

                        if (part->pUp.multiSelected || (isSpine && part->pUp.multiSelected)) {
                            frameCopiedPose.push_back({i, 0, part->pUp.pos});
                        }
                        if (part->pCenter.multiSelected) {
                            frameCopiedPose.push_back({i, 1, part->pCenter.pos});
                        }
                        if (part->pBottom.multiSelected || (isSpine && part->pBottom.multiSelected)) {
                            frameCopiedPose.push_back({i, 2, part->pBottom.pos});
                        }
                    }

                    if (!frameCopiedPose.empty()) {
                        copiedPoseRange.frames.push_back({
                            framePos - r_start,
                            frameCopiedPose,
                            hPos,
                            hRad
                        });
                    }
                }
            }

            // Restaurar el cuerpo a la posición del frame actual
            float currentX = currentFrame.getPosition().x;
            auto kb = bodyKeyList.find(currentX);
            if (kb != bodyKeyList.end()) {
                updatePosbody(fullBody, kb->second.dataAttr);
            }
            auto machHead = dataHead.find(currentX);
            if (machHead != dataHead.end()) {
                head.setPosition(machHead->second.x, machHead->second.y);
                head.setRadius(machHead->second.radius);
            }

            if (!copiedPoseRange.frames.empty()) {
                copiedPoseRange.hasData = true;
            }

            ss.str("");
            ss.clear();
            ss << "copy pose range: " << copiedPoseRange.frames.size() << " frames copiados";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
      }else if (select_action == "copy pose"){
            copySelectedPose(fullBody, copiedPose, head);
            ss.str("");
            ss.clear();

            ss << "copy pose : "
               << copiedPose.size()
               << " ctrl";

            setMensajeTime(
                ss,
                timeMensaje,
                viewTimeMensaje,
                mensajeClock
            );
      }else if (select_action == "multiseleccion"){
        if (preSelect){
          preSelect->selectedPoint  = nullptr;
          preSelect->update();
        }
        multiSelectMode = !multiSelectMode;

        if (!multiSelectMode){

            // Limpiar selección múltiple
            for (auto& part : fullBody){

                part->pUp.multiSelected = false;
                part->pCenter.multiSelected = false;
                part->pBottom.multiSelected = false;

                part->update();
            }

            ss.str("");
            ss.clear();
            ss << "multiseleccion OFF";

        }else{

            ss.str("");
            ss.clear();
            ss << "multiseleccion ON";
        }

        setMensajeTime(
            ss,
            timeMensaje,
            viewTimeMensaje,
            mensajeClock
        );
      }else if (select_action == "paste mirror"){

            auto pos = currentFrame.getPosition();
            float pasteStart = pos.x;

            float rangeLength = end_range - start_range;

            for (const auto& keyframe : bodyKeyList){

                float sourceFrame = keyframe.first;

                if(sourceFrame >= start_range &&
                   sourceFrame <= end_range){

                    // Distancia normal desde el inicio
                    float offset = sourceFrame - start_range;

                    // Invertir la distancia
                    float mirrorOffset = rangeLength - offset;

                    // Nueva posición
                    float targetFrame = pasteStart + mirrorOffset;


                    // -----------------------------
                    // Cabeza
                    // -----------------------------
                    auto machHead = dataHead.find(sourceFrame);

                    if(machHead != dataHead.end()){
                        head.setPosition(
                            machHead->second.x,
                            machHead->second.y
                        );
                    }


                    // -----------------------------
                    // Cuerpo
                    // -----------------------------
                    auto kb = bodyKeyList.find(sourceFrame);

                    if(kb != bodyKeyList.end()){
                        updatePosbody(
                            fullBody,
                            kb->second.dataAttr
                        );
                    }


                    // -----------------------------
                    // Mover currentFrame
                    // -----------------------------
                    auto currentPos = currentFrame.getPosition();

                    currentFrame.setPosition(
                        targetFrame,
                        currentPos.y
                    );


                    // -----------------------------
                    // Crear keyframe
                    // -----------------------------
                    newKey(
                        currentFrame,
                        head,
                        frameKeylist,
                        dataHead,
                        bodyKeyList,
                        fullBody
                    );
                }
            }

            // Dejar currentFrame al final del rango
            auto finalPos = currentFrame.getPosition();

            currentFrame.setPosition(
                pasteStart + rangeLength,
                finalPos.y
            );
      }else if (select_action == "move 10"){
        forceMove_frame = 10.f;
        ss.str("");
        ss.clear();
        ss << "move 10";
        setMensajeTime(
            ss,
            timeMensaje,
            viewTimeMensaje,
            mensajeClock
        );
      }else if (select_action == "head pin"){
    
        auto spineUpx= fullBody[4]->pUp.pos.x;
        auto spineUpy = fullBody[4]->pUp.pos.y;
        head.setPosition(spineUpx - 50,spineUpy - 100);
        newKey(currentFrame, head, frameKeylist,dataHead,bodyKeyList,fullBody);
        std::cout << spineUpx << " , " << spineUpy << std::endl; 
      }else if (select_action == "mirror pose"){

            mirrorPose(fullBody, head);

            newKey(
                currentFrame,
                head,
                frameKeylist,
                dataHead,
                bodyKeyList,
                fullBody
            );

            ss.str("");
            ss.clear();
            ss << "mirror pose";
            setMensajeTime(
                ss,
                timeMensaje,
                viewTimeMensaje,
                mensajeClock
            );
      }else if (select_action == "flip pose"){
            flipPose(fullBody, head);

            // Guardar el cambio en el keyframe actual
            newKey(
                currentFrame,
                head,
                frameKeylist,
                dataHead,
                bodyKeyList,
                fullBody
            );

            ss.str("");
            ss.clear();
            ss << "flip pose";
            setMensajeTime(
                ss,
                timeMensaje,
                viewTimeMensaje,
                mensajeClock
            );
      }else if (select_action == "slow x2"){
        ss.str("");
        ss.clear();
        if (slowPlay){
          slowPlay = false;
          frameTime = 1/24.f;
          activeSound.setPitch(1.0f); // <--- Reproduce el audio más lento (a la mitad)
          ss << "Reproduccion Slow x2 OFF";
        }else{ 
          slowPlay = true;
          frameTime = 1/12.f;
          activeSound.setPitch(0.5f); // <--- Reproduce el audio más lento (a la mitad)
          ss << "Reproduccion Slow x2 ON";
        } 
        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
      }else if (select_action == "retime /2"){

           float factor = 0.5f;

           float r_start = std::min(start_range, end_range);
           float r_end   = std::max(start_range, end_range);

           // -----------------------------------------
           // FRAME KEYS
           // -----------------------------------------
           retimeFrameKeysRange(
               frameKeylist,
               r_start,
               r_end,
               factor
           );

           // -----------------------------------------
           // CABEZA
           // -----------------------------------------
           retimeKeysRange(
               dataHead,
               r_start,
               r_end,
               factor
           );

           // -----------------------------------------
           // CUERPO
           // -----------------------------------------
           retimeKeysRange(
               bodyKeyList,
               r_start,
               r_end,
               factor
           );

           // Actualizar rango visual
           end_range =
               r_start + (r_end - r_start) * factor;

           ss.str("");
           ss.clear();

           ss << "Retime /2: "
              << r_start
              << " - "
              << r_end;

           setMensajeTime(
               ss,
               timeMensaje,
               viewTimeMensaje,
               mensajeClock
           );

      }else if (select_action == "retime x2"){

           float factor = 2.0f;

           float r_start = std::min(start_range, end_range);
           float r_end   = std::max(start_range, end_range);

           // -----------------------------------------
           // FRAME KEYS
           // -----------------------------------------
           retimeFrameKeysRange(
               frameKeylist,
               r_start,
               r_end,
               factor
           );

           // -----------------------------------------
           // CABEZA
           // -----------------------------------------
           retimeKeysRange(
               dataHead,
               r_start,
               r_end,
               factor
           );

           // -----------------------------------------
           // CUERPO
           // -----------------------------------------
           retimeKeysRange(
               bodyKeyList,
               r_start,
               r_end,
               factor
           );

           // Actualizar rango visual
           end_range =
               r_start + (r_end - r_start) * factor;

           ss.str("");
           ss.clear();

           ss << "Retime *2: "
              << r_start
              << " - "
              << r_end;

           setMensajeTime(
               ss,
               timeMensaje,
               viewTimeMensaje,
               mensajeClock
           );
             }else if (select_action == "aZoom x2"){
                 if (zoomMode){
                   zoomMode = false;
                   viewEscena.setSize(ancho_scena, altura_scena);
                   // Centramos la vista de nuevo en el medio de la pantalla original
                   viewEscena.setCenter(ancho_scena / 2.f, altura_scena / 2.f);
                   ss.str("");
                   ss.clear();
                   ss << "Vista restaurada por defecto :)";
                   setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                 }else {
                   if (preSelect != nullptr && preSelect->selectedPoint != nullptr) {
                       zoomMode = true;
                       // 1. Obtener la posición exacta del control seleccionado
                       sf::Vector2f targetPos = preSelect->selectedPoint->pos;

                       // 2. Centrar la vista de la escena en ese punto
                       viewEscena.setCenter(targetPos);

                       // 3. Aplicar zoom 4x reduciendo el tamaño del rectángulo de la vista (cuarta parte)
                       viewEscena.setSize(ancho_scena / 2.f, altura_scena / 2.f);

                       ss.str("");
                       ss.clear();
                       ss << "Zoom 4x aplicado en control :)";
                       setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                   } else {
                       ss.str("");
                       ss.clear();
                       ss << "Selecciona un punto de control primero :(";
                       setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
                   }
                }
      }else if (select_action == "zoom x4"){
          if (zoomMode){
            zoomMode = false;
            viewEscena.setSize(ancho_scena, altura_scena);
            // Centramos la vista de nuevo en el medio de la pantalla original
            viewEscena.setCenter(ancho_scena / 2.f, altura_scena / 2.f);
            ss.str("");
            ss.clear();
            ss << "Vista restaurada por defecto :)";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
          }else {
            if (preSelect != nullptr && preSelect->selectedPoint != nullptr) {
                zoomMode = true;
                // 1. Obtener la posición exacta del control seleccionado
                sf::Vector2f targetPos = preSelect->selectedPoint->pos;

                // 2. Centrar la vista de la escena en ese punto
                viewEscena.setCenter(targetPos);

                // 3. Aplicar zoom 4x reduciendo el tamaño del rectángulo de la vista (cuarta parte)
                viewEscena.setSize(ancho_scena / 4.f, altura_scena / 4.f);

                ss.str("");
                ss.clear();
                ss << "Zoom 4x aplicado en control :)";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                ss.str("");
                ss.clear();
                ss << "Selecciona un punto de control primero :(";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            }
         }
      }else if (select_action == "path ctrl"){
        if (viewPathPoint){
          viewPathPoint = false;
          autoPath  = false;
        }else{
            if (preSelect != nullptr && preSelect->selectedPoint != nullptr) {
                viewPathPoint = true;
                autoPath = false;
                actualizarTrayectoria(autoPath, currentFrame.getPosition().x, preSelect,fullBody, bodyKeyList, start_range, end_range, trajectoryPoints, viewPathPoint);
                ss.str("");
                ss.clear();
                ss << "Path Ctrl: " << trajectoryPoints.size() << " puntos encontrados";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                ss.str("");
                ss.clear();
                ss << "Selecciona un punto de control primero";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            }
        }
      }else if (select_action == "aPath"){
        if (viewPathPoint){
          viewPathPoint = false;
          autoPath  = false;
        }else{
            if (preSelect != nullptr && preSelect->selectedPoint != nullptr) {
                viewPathPoint = true;
                autoPath = true;
                actualizarTrayectoria(autoPath, currentFrame.getPosition().x, preSelect,fullBody, bodyKeyList, start_range, end_range, trajectoryPoints, viewPathPoint);
                ss.str("");
                ss.clear();
                ss << "Path Ctrl: " << trajectoryPoints.size() << " puntos encontrados";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                ss.str("");
                ss.clear();
                ss << "Selecciona un punto de control primero";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            }
        }
      }else if (select_action == "loop play"){
        ss.str("");
        ss.clear();
        mark_loop = true;
        start_loop = start_range;
        end_loop = end_range;
        ss << "(start) : " <<start_loop<<"(end) : "<<end_loop; 
        setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
      }else if (select_action == "front part"){
            if (preSelect != nullptr) {
                for (auto& part : fullBody) {
                    if (part.get() == preSelect) {
                        part->allFront = true;
                    } else {
                        part->allFront = false;
                    }
                }
                ss.str("");
                ss.clear();
                ss << "Parte al frente configurada :)";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            } else {
                ss.str("");
                ss.clear();
                ss << "Selecciona una parte primero :(";
                setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
            }
        
      }else if (select_action == "circle bezier"){
        if (create_circle_bezier){
          create_circle_bezier = false; 
        }else {
          create_circle_bezier = true;
        }
      }else if (select_action == "volumen audio"){
        volumenAudio = true;
        ss.str("");
        ss.clear();
        ss<< "(Up) subir - (Down) bajar - volumen audio";
        setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
      } else if (select_action == "paste pose range") {
        if (!copiedPoseRange.hasData || copiedPoseRange.frames.empty()) {
            ss.str("");
            ss.clear();
            ss << "No hay rango de poses copiado";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
        } else {
            float pasteStart = currentFrame.getPosition().x;

            for (const auto& copFrame : copiedPoseRange.frames) {
                float targetFrame = pasteStart + copFrame.frameOffset;

                // Mover temporalmente el currentFrame al frame destino
                auto pos = currentFrame.getPosition();
                currentFrame.setPosition(targetFrame, pos.y);

                // ========================================================
                // FUSIÓN INTELIGENTE: Verificar si ya existe un keyframe en el destino
                // ========================================================
                auto itExistingBody = bodyKeyList.find(targetFrame);
                if (itExistingBody != bodyKeyList.end()) {
                    // Si ya existe, cargamos primero ese estado en fullBody para no perder a los demás controles
                    updatePosbody(fullBody, itExistingBody->second.dataAttr);
                } else {
                    // Si no existe, buscamos el keyframe más cercano anterior para tener una base coherente
                    auto itPrevBody = bodyKeyList.lower_bound(targetFrame);
                    if (itPrevBody != bodyKeyList.begin()) {
                        --itPrevBody;
                        updatePosbody(fullBody, itPrevBody->second.dataAttr);
                    }
                }

                // Manejar cabeza: si ya hay datos en el destino, conservarlos; si no, usar los copiados
                auto itExistingHead = dataHead.find(targetFrame);
                sf::Vector2f targetHeadPos = copFrame.headPos;
                float targetHeadRad = copFrame.headRadius;
                if (itExistingHead != dataHead.end()) {
                    // Si no se seleccionó la cabeza/spine para pegar, respetamos la cabeza original del destino
                    // (Opcional: Si prefieres aplicar siempre la cabeza copiada, déjalo con copFrame.headPos)
                    targetHeadPos = sf::Vector2f(itExistingHead->second.x, itExistingHead->second.y);
                    targetHeadRad = itExistingHead->second.radius;
                }
                head.setPosition(targetHeadPos);
                head.setRadius(targetHeadRad);

                // Aplicar únicamente los controles copiados sobre la pose actual del cuerpo
                pasteSelectedPose(fullBody, copFrame.pose, head);

                // Guardar el resultado combinado como keyframe en esa posición del timeline
                newKey(currentFrame, head, frameKeylist, dataHead, bodyKeyList, fullBody);
            }

            // Dejar el currentFrame al final del rango pegado
            auto pos = currentFrame.getPosition();
            currentFrame.setPosition(pasteStart + copiedPoseRange.originalDuration, pos.y);

            // Volver a cargar el frame actual original para restaurar la escena visualmente
            float currentX = currentFrame.getPosition().x;
            auto kb = bodyKeyList.find(currentX);
            if (kb != bodyKeyList.end()) {
                updatePosbody(fullBody, kb->second.dataAttr);
            }
            auto machHead = dataHead.find(currentX);
            if (machHead != dataHead.end()) {
                head.setPosition(machHead->second.x, machHead->second.y);
                head.setRadius(machHead->second.radius);
            }

            ss.str("");
            ss.clear();
            ss << "paste pose range aplicado con exito :)";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
        }
      } else if (select_action == "paste pose range mirror") {
        if (!copiedPoseRange.hasData || copiedPoseRange.frames.empty()) {
            ss.str("");
            ss.clear();
            ss << "No hay rango de poses copiado";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
        } else {
            float pasteStart = currentFrame.getPosition().x;
            float rangeLength = copiedPoseRange.originalDuration;

            for (const auto& copFrame : copiedPoseRange.frames) {
                float invertedOffset = rangeLength - copFrame.frameOffset;
                float targetFrame = pasteStart + invertedOffset;

                // Mover temporalmente el currentFrame al frame destino
                auto pos = currentFrame.getPosition();
                currentFrame.setPosition(targetFrame, pos.y);

                // ========================================================
                // FUSIÓN INTELIGENTE: Verificar si ya existe un keyframe en el destino
                // ========================================================
                auto itExistingBody = bodyKeyList.find(targetFrame);
                if (itExistingBody != bodyKeyList.end()) {
                    updatePosbody(fullBody, itExistingBody->second.dataAttr);
                } else {
                    auto itPrevBody = bodyKeyList.lower_bound(targetFrame);
                    if (itPrevBody != bodyKeyList.begin()) {
                        --itPrevBody;
                        updatePosbody(fullBody, itPrevBody->second.dataAttr);
                    }
                }

                auto itExistingHead = dataHead.find(targetFrame);
                sf::Vector2f targetHeadPos = copFrame.headPos;
                float targetHeadRad = copFrame.headRadius;
                if (itExistingHead != dataHead.end()) {
                    targetHeadPos = sf::Vector2f(itExistingHead->second.x, itExistingHead->second.y);
                    targetHeadRad = itExistingHead->second.radius;
                }
                head.setPosition(targetHeadPos);
                head.setRadius(targetHeadRad);

                // Aplicar los controles copiados de forma invertida en el tiempo
                pasteSelectedPose(fullBody, copFrame.pose, head);

                // Guardar como keyframe combinado en esa posición del timeline
                newKey(currentFrame, head, frameKeylist, dataHead, bodyKeyList, fullBody);
            }

            // Dejar el currentFrame al final del rango pegado
            auto pos = currentFrame.getPosition();
            currentFrame.setPosition(pasteStart + rangeLength, pos.y);

            // Volver a cargar el frame actual original para restaurar la escena visualmente
            float currentX = currentFrame.getPosition().x;
            auto kb = bodyKeyList.find(currentX);
            if (kb != bodyKeyList.end()) {
                updatePosbody(fullBody, kb->second.dataAttr);
            }
            auto machHead = dataHead.find(currentX);
            if (machHead != dataHead.end()) {
                head.setPosition(machHead->second.x, machHead->second.y);
                head.setRadius(machHead->second.radius);
            }

            ss.str("");
            ss.clear();
            ss << "paste pose range mirror (temporal) aplicado con exito :)";
            setMensajeTime(ss, timeMensaje, viewTimeMensaje, mensajeClock);
        }
      }           
    }
    if (viewTimeMensaje && mensajeClock.getElapsedTime().asSeconds() > 2.f){
        viewTimeMensaje = false;
    }
    if (isPlay){
      if (playClock.getElapsedTime().asSeconds() >= frameTime){
        if (preSelect){
          preSelect->selectedPoint  = nullptr;
          preSelect->update();
        }
        playClock.restart();
        auto pos = currentFrame.getPosition();
        //actualizo las posicines
        auto machHead= dataHead.find(pos.x);
        if (machHead != dataHead.end()){
          head.setPosition(machHead->second.x, machHead->second.y);
          head.setRadius(machHead->second.radius); // <--- Restaurar la escala de la cabeza
        }
        auto kb = bodyKeyList.find(pos.x);
        if (kb != bodyKeyList.end()){
          updatePosbody(fullBody , kb->second.dataAttr); 
        }
        auto audioIt = audioKeyList.find(pos.x);
        if (audioIt != audioKeyList.end()) {
          // Asociar el buffer al reproductor y reproducir
          activeSound.setBuffer(audioIt->second.soundBufferCache);
          activeSound.play();
        }
        updatePosAllAssets(globalAssets, pos.x);       
        //actuaizar view timeline
        int frameInt = static_cast<int>(pos.x);
        pageOffset = (static_cast<float>(frameInt / 450)) * 450.f;
        int paginaIndex = static_cast<int>(pageOffset);
        if ((paginaIndex / 450) % 2 == 0) {
            idxTimeline = false;
        } else {
            idxTimeline = true;
        }
        pos.x += 10.f;
        if (mark_loop){
            if (pos.x > end_loop){
              currentFrame.setPosition(start_loop, pos.y);
              sincronizarAudio(
                activeSound,
                audioKeyList,
                start_loop
              );
            }else{
              currentFrame.setPosition(pos.x , pos.y);
            }
        }else{
            if (pos.x > maxFramePosition){
              isPlay=false;
              activeSound.stop();
              currentFrame.setPosition(maxFramePosition, pos.y);
            }else{
              currentFrame.setPosition(pos.x , pos.y);
            }
         }
      }
    }else{ 
        if(isPause){
            isPause = false;
            auto kf = currentFrame.getPosition();
            int keyfInt = static_cast<int>(kf.x);
            if ( keyfInt% 20== 15){
              currentFrame.setPosition(kf.x+10 , kf.y);
          }
        }
    }
    if(create_circle_bezier){
        // Actualizar valores de los sliders según el movimiento del mouse
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float mouseX = static_cast<float>(mousePos.x);

        if (draggingR) {
            r = static_cast<int>(std::clamp(mouseX - trackX, 0.f, trackWidth));
            if (currentMode == 0) fillR = r; else outlineR = r;
        }
        if (draggingG) {
            g = static_cast<int>(std::clamp(mouseX - trackX, 0.f, trackWidth));
            if (currentMode == 0) fillG = g; else outlineG = g;
        }
        if (draggingB) {
            b = static_cast<int>(std::clamp(mouseX - trackX, 0.f, trackWidth));
            if (currentMode == 0) fillB = b; else outlineB = b;
        }
    }
    window.clear(sf::Color(30, 30, 30));
    window.setView(viewInfo);
    window.draw(drawInfo);

    if (viewTimeMensaje){
      window.draw(timeMensaje);
    }

    viewTimeline.setCenter(
      (ancho_timeline / 2.f) + pageOffset,
      altura_timeline / 2.f
    );
    drawTimeline.setPosition(pageOffset, 0.f);
    window.setView(viewTimeline);
    window.draw(drawTimeline);
    for (int i = 0 ; i< 23; i++){
        sf::RectangleShape lineFrame(sf::Vector2f(2,20));
        lineFrame.setFillColor(sf::Color(180, 180 ,180));
        if (idxTimeline){
          lineFrame.setPosition(pageOffset + 15+20*i,0);
          window.draw(lineFrame);
        }else{
          lineFrame.setPosition(pageOffset + 5+20*i,0);
          window.draw(lineFrame);
        }
    }

    if (selectedAssetTarget.empty()) {
        // Mostrar keys normales del cuerpo
        for (const auto& par : frameKeylist){
            window.draw(par.second);
        }
    } else {
        // Buscar el asset seleccionado y dibujar SOLO sus keys
        auto it = std::find_if(globalAssets.begin(), globalAssets.end(), [selectedAssetTarget](const AssetControl& ac){
            return ac.name == selectedAssetTarget;
        });
        if (it != globalAssets.end()) {
            for (const auto& par : it->assetKeylist){
                window.draw(par.second);
            }
        }
    }
    for (const auto& par : audioKeyList){
        window.draw(par.second.key_audio);
    }
    for (const auto& par : loopKeylist){
        //markers
        window.draw(par.second);
    }
    
    window.draw(currentFrame);
    window.setView(viewEscena);
    window.draw(drawScena);

    if (isRender){
      sf::RenderTexture render;
      render.create(ancho_scena, altura_scena);
      if (preSelect){
        preSelect->selectedPoint  = nullptr;
        preSelect->update();
      }
      int frameCounter = 1;
      drawScena.setOutlineThickness(0.f);
      
      for (float framePos = start_render;
          framePos <= end_render;
          framePos += 10.f)
      {
          auto dh = dataHead.find(framePos);
          if (dh != dataHead.end()){
              head.setPosition(dh->second.x, dh->second.y);
              head.setRadius(dh->second.radius); // <--- Restaurar la escala de la cabeza
          }

          auto db = bodyKeyList.find(framePos);
          if (db != bodyKeyList.end()){
              updatePosbody(fullBody, db->second.dataAttr);
          }
          updatePosAllAssets(globalAssets, framePos);
            
          render.clear(sf::Color::White);
          render.draw(drawScena);
          render.draw(head);

          for (const auto& partbody : fullBody){
              partbody->draw(render);   
          }
          for (const auto& asset : globalAssets) {
            render.draw(asset.shape);
          }
          render.display();

          sf::Image img = render.getTexture().copyToImage();

          std::ostringstream nombreRender;
          nombreRender << "/home/manuel/Documents/"
                      << "cppApps/animation/verb/render/frame"
                      << std::setw(4)
                      << std::setfill('0')
                      << frameCounter
                      << ".png";

          img.saveToFile(nombreRender.str());
          frameCounter++;
      }

      // --- RECOPILAR AUDIOS Y SUS TIEMPOS ---
      std::string audioInputs = "";
      std::string filterComplex = "";
      int audioIndex = 1; // El índice 0 es el video (-i frame%04d.png)

      for (const auto& pair : audioKeyList) {
          float framePos = pair.first;
          // Calcular el tiempo en segundos basado en la posición del frame (24 fps, paso de 10.f)
          double frameIndex = (framePos - 5.f) / 10.f;
          double timeSec = frameIndex / 24.0;
          int delayMs = static_cast<int>(timeSec * 1000.0);

          audioInputs += " -i \"" + pair.second.path_audio + "\"";
          
          // Crear los flujos de delay para cada audio
          std::ostringstream filterStream;
          filterStream << "[" << audioIndex << ":a]adelay=" << delayMs << "|" << delayMs << "[a" << audioIndex << "];";
          filterComplex += filterStream.str();
          audioIndex++;
      }

      // Construir el comando FFmpeg final dinámicamente
      std::ostringstream ffmpegCmd;
      ffmpegCmd << "ffmpeg -y -framerate 24 "
                << "-i /home/manuel/Documents/cppApps/animation/verb/render/frame%04d.png";

      if (!audioKeyList.empty()) {
          ffmpegCmd << audioInputs << " -filter_complex \"";
          ffmpegCmd << filterComplex;
          
          // Mezclar todos los audios procesados con amix
          for (int i = 1; i < audioIndex; ++i) {
              ffmpegCmd << "[a" << i << "]";
          }
          ffmpegCmd << "amix=inputs=" << (audioIndex - 1) << "[outa]\" "
                    << "-map 0:v -map \"[outa]\" ";
      }

      ffmpegCmd << "-c:v libx264 -pix_fmt yuv420p "
                << "-shortest "
                << "/home/manuel/Documents/cppApps/animation/verb/animacion.mp4";

      int resultado = system(ffmpegCmd.str().c_str());

      if (resultado == 0){
        ss.str("");
        ss.clear();
        ss << "video generado" << ".  :)"; 
        setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
      }else{
        ss.str("");
        ss.clear();
        ss << "error al generar video" << ".  :("; 
        setMensajeTime(ss, timeMensaje , viewTimeMensaje, mensajeClock);
      }
      drawScena.setOutlineThickness(-2.f);
      isRender = false;
    }
    window.draw(head);
    //draw_ctrls
    if (selectMode == 2){
      window.draw(head_ctrl);
    }else if (selectMode == 1){
      window.draw(allBody_ctrl);
    }
    // 1ra Pasada: Dibujar todas las partes que NO estén al frente
    for (const auto& partbody : fullBody) {
        if (!partbody->allFront) {
            partbody->draw(window);
        }
    }
    // 2da Pasada: Dibujar al final la parte que SÍ esté al frente (para que tape a las demás)
    for (const auto& partbody : fullBody) {
        if (partbody->allFront) {
            partbody->draw(window);
        }
    }
    for (const auto& asset : globalAssets) {
        window.draw(asset.shape);
    }   
    if (viewOnion){
      if (viewOnionRange){
        for (int i = start_range; i <= end_range; i++){
          // Frames pre
          auto kon = bodyKeyList.find(i);
          auto khead = dataHead.find(i);
          if (kon != bodyKeyList.end()){
            drawOnion(window, onionBody, kon->second,grisColor);
          }
          if (khead != dataHead.end()){
            headOnion.setPosition(khead->second.x,khead->second.y);
            window.draw(headOnion);
          }
        }
      }else{
          auto pos = currentFrame.getPosition();

          // 1. Buscar el keyframe anterior más cercano (Izquierda)
          auto itPre = bodyKeyList.lower_bound(pos.x);
          if (itPre != bodyKeyList.begin()) {
              --itPre; // Retrocedemos al elemento inmediatamente anterior
              float preX = itPre->first;

              // Dibujar el cuerpo del keyframe anterior (rojo)
              drawOnion(window, onionBody, itPre->second, redColor);

              // Dibujar la cabeza del keyframe anterior
              auto kheadPre = dataHead.find(preX);
              if (kheadPre != dataHead.end()) {
                  headOnion.setPosition(kheadPre->second.x, kheadPre->second.y);
                  headOnion.setFillColor(redColor);
                  window.draw(headOnion);
              }
          }

          // 2. Buscar el keyframe posterior más cercano (Derecha)
          auto itPost = bodyKeyList.upper_bound(pos.x);
          if (itPost != bodyKeyList.end()) {
              float postX = itPost->first;

              // Dibujar el cuerpo del keyframe posterior (azul)
              drawOnion(window, onionBody, itPost->second, blueColor);

              // Dibujar la cabeza del keyframe posterior
              auto kheadPost = dataHead.find(postX);
              if (kheadPost != dataHead.end()) {
                  headOnion.setPosition(kheadPost->second.x, kheadPost->second.y);
                  headOnion.setFillColor(blueColor);
                  window.draw(headOnion);
              }
          }
      }
    }
    if (viewPathPoint){
        for (const auto& pt : trajectoryPoints){
          visualPoint.setPosition(pt);
          window.draw(visualPoint); 
        }
    }
    // --- SOLO DIBUJAR EL BUSCADOR SI openBuscador ES TRUE ---
    if (openBuscador) {
        window.setView(viewBuscador);
        window.draw(drawBuscador);
        
        // Dibujar barra de búsqueda
        sf::Text queryText;
        queryText.setFont(font);
        queryText.setCharacterSize(18);
        queryText.setFillColor(sf::Color::White);
        queryText.setPosition(20, 20);
        if (selectAudio == 1){
          queryText.setString("Audio: " + query + "_");
        }else if (selectAudio == 2){
          queryText.setString("Save: " + query + "_");
        }else if (selectAudio == 4){
          queryText.setString("Name: " + query + "_");
        }else if (selectAudio == 5){
          queryText.setString("Asset: " + query + "_");
        }else if (selectAudio == 3){
          queryText.setString("Load: " + query + "_");
        }else{
          queryText.setString("Accion: " + query + "_");
        }
        window.draw(queryText);

        // Línea separadora
        sf::RectangleShape separator(sf::Vector2f(410, 2));
        separator.setPosition(20, 55);
        separator.setFillColor(sf::Color(80, 80, 80));
        window.draw(separator);

        // --- Dibujar resultados (Máximo 4 elementos) ---
        float startY = 75.0f;
        
        size_t limit = std::min(filteredResults.size(), (size_t)4);

        for (size_t i = 0; i < limit; ++i) {
            sf::Text itemText;
            itemText.setFont(font);
            itemText.setCharacterSize(16);
            
            if (i == 0) {
                itemText.setFillColor(sf::Color::Green);
                itemText.setString("> " + filteredResults[i]);
            } else {
                itemText.setFillColor(sf::Color(92,92,92));
                itemText.setString("  " + filteredResults[i]);
            }

            itemText.setPosition(30, startY + (i * 35));
            window.draw(itemText);
        }

    }
    if (create_circle_bezier){
        // --- 1. SLIDER ROJO ---

        sf::RectangleShape trackR(sf::Vector2f(trackWidth, trackHeight));
        trackR.setPosition(trackX, 150.f);
        trackR.setFillColor(sf::Color(80, 40, 40));
        window.draw(trackR);

        sf::RectangleShape handleR(sf::Vector2f(10.f, trackHeight + 10.f));
        handleR.setPosition(trackX + r - 5.f, 145.f);
        handleR.setFillColor(sf::Color::Red);
        window.draw(handleR);

        // --- 2. SLIDER VERDE ---
        sf::RectangleShape trackG(sf::Vector2f(trackWidth, trackHeight));
        trackG.setPosition(trackX, 230.f);
        trackG.setFillColor(sf::Color(40, 80, 40));
        window.draw(trackG);

        sf::RectangleShape handleG(sf::Vector2f(10.f, trackHeight + 10.f));
        handleG.setPosition(trackX + g - 5.f, 225.f);
        handleG.setFillColor(sf::Color::Green);
        window.draw(handleG);

        // --- 3. SLIDER AZUL ---

        sf::RectangleShape trackB(sf::Vector2f(trackWidth, trackHeight));
        trackB.setPosition(trackX, 310.f);
        trackB.setFillColor(sf::Color(40, 40, 80));
        window.draw(trackB);

        sf::RectangleShape handleB(sf::Vector2f(10.f, trackHeight + 10.f));
        handleB.setPosition(trackX + b - 5.f, 305.f);
        handleB.setFillColor(sf::Color::Blue);
        window.draw(handleB);

        // --- 4. SIMULADOR DE COMBO BOX (SELECTOR DE MODO) ---
        sf::RectangleShape comboBg(sf::Vector2f(240.f, 40.f));
        comboBg.setPosition(90.f, 410.f);
        comboBg.setFillColor(sf::Color(50, 50, 60));
        comboBg.setOutlineColor(sf::Color(100, 100, 120));
        comboBg.setOutlineThickness(2.f);
        window.draw(comboBg);

        std::string modeText = (currentMode == 0) ? "Relleno" : "Borde";
        sf::Text comboLabel("Modo: " + modeText + " (Clic)", font, 14);
        comboLabel.setPosition(100.f, 420.f);
        comboLabel.setFillColor(sf::Color::White);
        window.draw(comboLabel);

        // --- 5. MODELO CIRCULAR (VISTA PREVIA) ---
        sf::CircleShape previewCircle(circleRadius);
        previewCircle.setOrigin(circleRadius, circleRadius);
        previewCircle.setPosition(circleCenter);

        sf::Color fillColor(static_cast<sf::Uint8>(fillR), static_cast<sf::Uint8>(fillG), static_cast<sf::Uint8>(fillB));
        sf::Color outlineColor(static_cast<sf::Uint8>(outlineR), static_cast<sf::Uint8>(outlineG), static_cast<sf::Uint8>(outlineB));

        previewCircle.setFillColor(fillColor);
        previewCircle.setOutlineColor(outlineColor);
        previewCircle.setOutlineThickness(8.f);
        window.draw(previewCircle);

        // --- 6. VALORES NUMÉRICOS Y HEXADECIMALES (del modo activo) ---
        int activeR = (currentMode == 0) ? fillR : outlineR;
        int activeG = (currentMode == 0) ? fillG : outlineG;
        int activeB = (currentMode == 0) ? fillB : outlineB;

        sf::Text infoText;
        infoText.setFont(font);
        infoText.setCharacterSize(15);
        infoText.setFillColor(sf::Color::Black);
        infoText.setPosition(100.f, 380.f);
        infoText.setString("RGB: (" + std::to_string(activeR) + ", " + std::to_string(activeG) + ", " + std::to_string(activeB) + ")");
        window.draw(infoText);
    }
    window.display();
  }
  return 0;
}
