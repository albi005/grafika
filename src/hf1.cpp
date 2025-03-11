#include "framework.h"

const char* vertSource = R"(
    #version 330
    precision highp float;

    layout(location = 0) in vec2 cP;	// 0. bemeneti regiszter

    void main() {
        gl_Position = vec4(cP.x, cP.y, 0, 1); 	// bemenet már normalizált eszközkoordinátákban
    }
)";

const char* fragSource = R"(
    #version 330
    precision highp float;

    uniform vec3 color;
    out vec4 fragmentColor;

    void main() {
        fragmentColor = vec4(color, 1);
    }
)";

class PointCollection final : Geometry<vec2> {
    GPUProgram* gpuProgram = nullptr;

  public:
    explicit PointCollection(GPUProgram* gpu_program) :
        gpuProgram(gpu_program) {
    }

    void addPoint(const vec2& p) {
        vtx.push_back(p);
        printf("Point %.1f, %.1f added\n", p.x, p.y);
        updateGPU();
    }

    vec2 getClosestPoint(const vec2& p) {
        if (vtx.empty())
            return {0, 0};

        auto closest = vtx[0];
        for (auto& point : vtx) {
            if (length(point - p) < length(closest - p)) {
                closest = point;
            }
        }
        return closest;
    }

    void draw(const vec3 color) {
        glPointSize(10);
        Draw(gpuProgram, GL_POINTS, color);
    }
};

struct Line {
    vec2 p1;
    vec2 p2;
    Line(const vec2& p1, const vec2& p2) : p1(p1), p2(p2) {
    }

    vec2 getNormalVector() const {
        vec2 v = p2 - p1;
        return {-v.y, v.x};
    }

    float getD() const {
        const vec2 n = getNormalVector();
        const vec2 p = p1;
        return -n.x * p.x - n.y * p.y;
    }

    vec2 getIntersection(const Line other) const {
        const vec2 a = getNormalVector();
        const float ad = getD();
        const vec2 b = other.getNormalVector();
        const float bd = other.getD();

        // we do a bit of trolling
        if (a.x == 0)
            return other.getIntersection(*this);

        const float y = ((ad * b.x) / a.x - bd) / ((b.x * -a.y) / a.x + b.y);
        const float x = (-a.y * y - ad) / a.x;
        return {x, y};
    }

    float getDistance(const vec2 p) const {
        const vec2 n = getNormalVector();
        const float eP = n.x * p.x + n.y * p.y + -(n.x * p1.x + n.y * p1.y);
        return fabs(eP / length(n));
    }

    bool contains(const vec2 p) const {
        const float d = getDistance(p);
        return d < 0.01;
    }

    void print() const {
        const vec2 n = getNormalVector();
        printf("LINE: {{%.9f, %.9f}, {%.9f, %.9f}}\n", p1.x, p1.y, p2.x, p2.y);
        printf(
            "     Implicit: %.1f x + %.1f y + %.1f = 0\n",
            n.x,
            n.y,
            -(n.x * p1.x + n.y * p1.y)
        );
        printf(
            "   Parametric: r(t) = (%.1f, %.1f) + (%.1f, %.1f)t\n",
            p1.x,
            p1.y,
            p2.x - p1.x,
            p2.y - p1.y
        );
    }
};

class LineCollection final : Geometry<vec2> {
    GPUProgram* gpuProgram = nullptr;
    std::vector<Line> lines;

  public:
    explicit LineCollection(GPUProgram* gpuProgram) : gpuProgram(gpuProgram) {
    }

    void draw(const vec3 color) {
        glLineWidth(3);
        Draw(gpuProgram, GL_LINES, color);
    }

    void add(const Line line) {
        lines.push_back(line);
        updateVertexBuffer();

        printf("Line added\n");
        line.print();
    }

    Line* getClosestLine(const vec2& p) {
        if (lines.empty())
            return nullptr;

        Line* closest = &lines[0];
        for (Line& line : lines) {
            if (line.getDistance(p) < closest->getDistance(p)) {
                closest = &line;
            }
        }
        return closest;
    }

    void updateVertexBuffer() {
        vtx.clear();
        vtx.reserve(lines.size() * 2);
        for (const auto& line : lines) {
            const vec2 dir = normalize(line.p2 - line.p1);
            vtx.push_back(line.p1 - (dir * 10.0f));
            vtx.push_back(line.p2 + (dir * 10.0f));
        }
        updateGPU();
    }
};

constexpr int winWidth = 600, winHeight = 600;

class AppState {
  public:
    virtual void onMousePressed(float x, float y) {
    }
    virtual void onMouseReleased(float x, float y) {
    }
    virtual void onMouseMoved(float x, float y) {
    }
};

class PointState : public AppState {
    PointCollection* points = nullptr;

  public:
    explicit PointState(PointCollection* points) : points(points) {
    }

    void onMousePressed(float x, float y) override {
        points->addPoint({x, y});
    }
};

class LineState : public AppState {
    PointCollection* points = nullptr;
    LineCollection* lines = nullptr;
    std::vector<vec2> selectedPoints;

  public:
    LineState(PointCollection* points, LineCollection* lines) :
        points(points), lines(lines) {
    }

    void onMousePressed(float x, float y) override {
        const auto point = points->getClosestPoint({x, y});
        selectedPoints.push_back(point);
        if (selectedPoints.size() == 2) {
            const auto line = Line(selectedPoints[0], selectedPoints[1]);
            lines->add(line);
            selectedPoints.clear();
        }
    }
};

class MoveState : public AppState {
    LineCollection* lines = nullptr;
    vec2 startingMouse = {};
    Line originalLine = Line({0, 0}, {0, 0});
    Line* selectedLine = nullptr;
    bool isMoving = false;

  public:
    explicit MoveState(LineCollection* lines) : lines(lines) {
    }

    void onMousePressed(float x, float y) override {
        startingMouse = {x, y};
        selectedLine = lines->getClosestLine({x, y});
        printf("%p\n", selectedLine);
        originalLine = *selectedLine;
        isMoving = true;
    }

    void onMouseMoved(const float x, const float y) override {
        if (!isMoving)return;
        const vec2 delta = vec2(x, y) - startingMouse;
        *selectedLine = Line(originalLine.p1 + delta, originalLine.p2 + delta);
        lines->updateVertexBuffer();
    }

    void onMouseReleased(float x, float y) override {
        isMoving = false;
        const vec2 delta = vec2(x, y) - startingMouse;
        printf("delta: %.2f, %.2f\n", delta.x, delta.y);
    }
};

class IntersectionState : public AppState {
    PointCollection* points = nullptr;
    LineCollection* lines = nullptr;
    std::vector<Line> selectedLines;

  public:
    IntersectionState(PointCollection* points, LineCollection* lines) :
        points(points), lines(lines) {
    }

    void onMousePressed(float x, float y) override {
        printf("IntersectionState::onMousePressed\n");
        printf("x: %.2f, y: %.2f\n", x, y);
        const Line* line = lines->getClosestLine({x, y});
        printf("line: %p\n", line);
        line->print();
        selectedLines.push_back(*line);
        if (selectedLines.size() == 2) {
            const vec2 intersection =
                selectedLines[0].getIntersection(selectedLines[1]);
            points->addPoint(intersection);
            selectedLines.clear();
        }
    }
};

class GrafikaApp final : public glApp {
    PointCollection* points = nullptr;
    LineCollection* lines = nullptr;
    AppState* state = nullptr;

  public:
    GrafikaApp() : glApp("GrafikaApp") {
    }

    void onInitialization() override {
        auto* gpu_program = new GPUProgram(vertSource, fragSource);
        points = new PointCollection(gpu_program);
        lines = new LineCollection(gpu_program);
        state = new PointState(points);
    }

    void onDisplay() override {
        constexpr float g = static_cast<float>(0x4C) / 0xFF;
        glClearColor(g, g, g, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, winWidth, winHeight);
        lines->draw({0, 1, 1});
        points->draw(vec3(1, 0, 0));
    }

    void onKeyboard(const int key) override {
        AppState* newState = nullptr;
        switch (key) {
            case 'p':
                newState = new PointState(points);
                printf("Define points\n");
                break;
            case 'l':
                newState = new LineState(points, lines);
                printf("Define lines\n");
                break;
            case 'i':
                newState = new IntersectionState(points, lines);
                printf("Intersect\n");
                break;
            case 'm':
                newState = new MoveState(lines);
                printf("Move");
                break;
            default:
                break;
        }
        if (newState != nullptr) {
            delete state;
            state = newState;
        }
    }

    void onMousePressed(MouseButton but, const int pX, const int pY) override {
        float x = pX;
        float y = pY;
        x = 2 * x / winWidth - 1;
        y = 1 - 2 * y / winHeight;
        if (state != nullptr)
            state->onMousePressed(x, y);
        refreshScreen();
    }

    void onMouseReleased(MouseButton but, int pX, int pY) override {
        float x = pX;
        float y = pY;
        x = 2 * x / winWidth - 1;
        y = 1 - 2 * y / winHeight;
        if (state != nullptr)
            state->onMouseReleased(x, y);
        refreshScreen();
    }

    void onMouseMotion(const int pX, const int pY) override {
        float x = pX;
        float y = pY;
        x = 2 * x / winWidth - 1;
        y = 1 - 2 * y / winHeight;
        if (state != nullptr)
            state->onMouseMoved(x, y);
        refreshScreen();
    }
} app;
