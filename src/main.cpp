#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#define CAMERA_IMPLEMENTATION
#include "rcamera.h"

// #include "tsl/hopscotch_growth_policy.h"
// #include "tsl/hopscotch_map.h"

template<>
struct std::hash<Vector2> {
    size_t operator()(Vector2 p) const {
        return *(size_t*)&p;
        // size_t l = std::hash<float>()(p.x);
        // size_t r = std::hash<float>()(p.y);
        // return l ^ (r + 0x9e3779b9 + (l << 6) + (l >> 2));
    }
};

bool operator==(Vector2 l, Vector2 r) {
    return l.x == r.x && l.y == r.y;
}

template<>
struct std::hash<Vector3> {
    size_t operator()(Vector3 p) const {
        size_t l = std::hash<Vector2>()({p.x, p.y});
        size_t r = std::hash<float>()(p.z);
        return l ^ (r + 0x9e3779b9 + (l << 6) + (l >> 2));
    }
};

bool operator==(Vector3 l, Vector3 r) {
    return l.x == r.x && l.y == r.y && l.z == r.z;
}

struct Triangle {
    Vector3 A, B, C;
};

Mesh GenMeshTriangles(std::vector<Triangle> triangles) {
    Vector3 min = { 0 },
            max = { 0 },
            space = { 0 };
    for (const auto& t : triangles) {
        if (t.A.x > max.x) { max.x = t.A.x; }
        if (t.A.y > max.y) { max.y = t.A.y; }
        if (t.A.z > max.z) { max.z = t.A.z; }

        if (t.B.x > max.x) { max.x = t.B.x; }
        if (t.B.y > max.y) { max.y = t.B.y; }
        if (t.B.z > max.z) { max.z = t.B.z; }

        if (t.C.x > max.x) { max.x = t.C.x; }
        if (t.C.y > max.y) { max.y = t.C.y; }
        if (t.C.z > max.z) { max.z = t.C.z; }

        if (t.A.x < min.x) { min.x = t.A.x; }
        if (t.A.y < min.y) { min.y = t.A.y; }
        if (t.A.z < min.z) { min.z = t.A.z; }

        if (t.B.x < min.x) { min.x = t.B.x; }
        if (t.B.y < min.y) { min.y = t.B.y; }
        if (t.B.z < min.z) { min.z = t.B.z; }

        if (t.C.x < min.x) { min.x = t.C.x; }
        if (t.C.y < min.y) { min.y = t.C.y; }
        if (t.C.z < min.z) { min.z = t.C.z; }
    }
    space = { max.x - min.x, max.y - min.y, max.z - min.z };

    Mesh mesh = { 0 };
    mesh.triangleCount = triangles.size();
    mesh.vertexCount = mesh.triangleCount * 3;

    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.colors = NULL;

    int vCounter = 0;
    int tcCounter = 0;
    int nCounter = 0;

    Vector3 vN = { 0 };

    for (const auto& t : triangles) {
        mesh.vertices[vCounter] = t.A.x;
        mesh.vertices[vCounter + 1] = t.A.y;
        mesh.vertices[vCounter + 2] = t.A.z;

        mesh.vertices[vCounter + 3] = t.B.x;
        mesh.vertices[vCounter + 4] = t.B.y;
        mesh.vertices[vCounter + 5] = t.B.z;

        mesh.vertices[vCounter + 6] = t.C.x;
        mesh.vertices[vCounter + 7] = t.C.y;
        mesh.vertices[vCounter + 8] = t.C.z;
        vCounter += 9;

        mesh.texcoords[tcCounter] = (t.A.x - min.x) / space.x;
        mesh.texcoords[tcCounter + 1] = (t.A.z - min.z) / space.z;

        mesh.texcoords[tcCounter + 2] = (t.B.x - min.x) / space.x;
        mesh.texcoords[tcCounter + 3] = (t.B.z - min.z) / space.z;

        mesh.texcoords[tcCounter + 4] = (t.C.x - min.x) / space.x;
        mesh.texcoords[tcCounter + 5] = (t.C.z - min.z) / space.z;
        tcCounter += 6;

        vN = Vector3Normalize(
            Vector3CrossProduct(Vector3Subtract(t.B, t.A), 
                                Vector3Subtract(t.C, t.A)));
        mesh.normals[nCounter] = vN.x;
        mesh.normals[nCounter + 1] = vN.y;
        mesh.normals[nCounter + 2] = vN.z;

        mesh.normals[nCounter + 3] = vN.x;
        mesh.normals[nCounter + 4] = vN.y;
        mesh.normals[nCounter + 5] = vN.z;

        mesh.normals[nCounter + 6] = vN.x;
        mesh.normals[nCounter + 7] = vN.y;
        mesh.normals[nCounter + 8] = vN.z;
        nCounter += 9;
    }

    UploadMesh(&mesh, false);

    TraceLog(LOG_INFO, "Mesh generated from triangles successfully");

    return mesh;
}

void Smooth(Mesh* mesh) {
    std::unordered_map<Vector3, Vector3> linkedNormals;
    size_t idx = 0;

    for (int i = 0; i < mesh->vertexCount * 3; i += 9) {
        float* tri = &mesh->vertices[i];
        Vector3 v1 = Vector3{tri[0], tri[1], tri[2]};
        Vector3 v2 = Vector3{tri[3], tri[4], tri[5]};
        Vector3 v3 = Vector3{tri[6], tri[7], tri[8]};

        Vector3 normalV1 = Vector3CrossProduct(Vector3Subtract(v2,v1), Vector3Subtract(v3,v1));
        Vector3 normalV2 = Vector3CrossProduct(Vector3Subtract(v3,v2), Vector3Subtract(v1,v2));
        Vector3 normalV3 = Vector3CrossProduct(Vector3Subtract(v1,v3), Vector3Subtract(v2,v3));

        Vector3 sum = Vector3Add(Vector3Add(normalV1, normalV2), normalV3);

        if (!linkedNormals.contains(v1)) linkedNormals[v1] = Vector3Zero();
        if (!linkedNormals.contains(v2)) linkedNormals[v2] = Vector3Zero();
        if (!linkedNormals.contains(v3)) linkedNormals[v3] = Vector3Zero();

        linkedNormals[v1] = Vector3Add(linkedNormals[v1], sum);
        linkedNormals[v2] = Vector3Add(linkedNormals[v2], sum);
        linkedNormals[v3] = Vector3Add(linkedNormals[v3], sum);

        idx+=9;
    }

    idx = 0;
    for (float* v = mesh->vertices; v < mesh->vertices + mesh->vertexCount*3; v += 3) {
        Vector3 cur = Vector3{v[0], v[1], v[2]};
        Vector3 nor = Vector3Normalize(linkedNormals[cur]);

        mesh->normals[idx + 0] = nor.x;
        mesh->normals[idx + 1] = nor.y;
        mesh->normals[idx + 2] = nor.z;

        idx += 3;
    }

    UpdateMeshBuffer(*mesh, 0, mesh->vertices, sizeof(float) * mesh->vertexCount * 3, 0);

    TraceLog(LOG_INFO, "Vertices smoothed successfully");
}

struct Terrain {
    using F = std::function<float(Vector2)>;

    F f;
    // tsl::hopscotch_map<
    //     Vector2, float,
    //     std::hash<Vector2>,
    //     std::equal_to<Vector2>,
    //     std::allocator<std::pair<Vector2, float>>,
    //     62,
    //     false,
    //     tsl::hh::prime_growth_policy
    // > cache;
    float max_val = 0;
    float min_val = 0;
    float scale = 1;

    Terrain(F f, float scale): f(f), scale(scale) { }

    float operator[](Vector2 p) {
        return f(p);
    }
    // float operator[](Vector2 p) {
    //     auto iter = cache.find(p);
    //     if (iter != cache.end()) {
    //         return iter->second;
    //     }

    //     float v = f(p);
    //     cache.insert({p, v});
    //     if (v > max_val) {
    //         max_val = v;
    //     }
    //     if (v < min_val) {
    //         min_val = v;
    //     }
    //     return v;
    // }

    float remap(float v, float start, float end) {
        return (v - min_val)/(max_val - min_val)*(end - start) + start;
    }

    std::vector<Triangle> triangles(Vector2 position, size_t chunkSize, float detail) {
        std::vector<Triangle> triangles;
        Terrain& terrain = *this;
        for (float x = position.x; x < position.x + chunkSize - detail; x += detail) {
            Vector2 a, b,
                    c, d;
            Vector3 A, B,
                    C, D;
            c = { x, position.y };
            d = { x + detail, position.y };
            C = { x, terrain[c], position.y };
            D = { x + detail, terrain[d], position.y };
            for (float z = position.y; z < position.y + chunkSize - detail; z += detail) {
                a = c;
                b = d;
                c = { x, z + detail };
                d = { x + detail, z + detail };

                A = C;
                B = D;
                C = { x, terrain[c], z + detail };
                D = { x + detail, terrain[d], z + detail };

                triangles.push_back({C, B, A});
                triangles.push_back({B, C, D});
            }
        }

        TraceLog(LOG_INFO, "Terrain triangles calculated successfully");
        return triangles;
    }

    Model model(Vector2 position, size_t chunkSize, float detail) {
        Mesh mesh = GenMeshTriangles(triangles(position, chunkSize, detail));
        Smooth(&mesh);
        Model model = LoadModelFromMesh(mesh);
        TraceLog(LOG_INFO, "Loaded model from mesh successfully");

        return model;
    }
};

struct LightSrc {
    float angle;
    float deltaAngle;
    float locusRadius;
    Shader shader;
    Light light;

    LightSrc(
        float angle,
        float deltaAngle,
        float locusRadius,
        Color color,
        Shader shader) :
    angle(angle), deltaAngle(deltaAngle), locusRadius(locusRadius),
    shader(shader), light(CreateLight(
        LIGHT_DIRECTIONAL,
        {locusRadius * sinf(angle), locusRadius * cosf(angle), 0},
        Vector3Zero(),
        color,
        shader))
    { }

    void update(Camera camera) {
        angle += deltaAngle;
        light.position.x = locusRadius * sinf(angle);
        light.position.y = locusRadius * cosf(angle);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        Vector3 viewVec = Vector3Subtract(camera.position, camera.target);
        SetShaderValue(
            shader,
            shader.locs[SHADER_LOC_VECTOR_VIEW],
            (float*)&viewVec,
            SHADER_UNIFORM_VEC3);
        UpdateLightValues(shader, light);
    }
};

struct Agent {
    float size;
    Model model;
    Vector3 position;

    Agent(float size, Shader shader): size(size){
        model = LoadModelFromMesh(GenMeshCube(size, size, size));
        model.materials[0].shader = shader;
    }
    ~Agent() {
        UnloadModel(model);
    }

    void update(const Terrain& terrain, Camera* camera) {
        float y = terrain.f({
            camera->target.x / terrain.scale,
            camera->target.z / terrain.scale});
        y *= terrain.scale;
        y += size / 2;

        float diff = y - camera->target.y;
        camera->target.y += diff;
        camera->position.y += diff;

        position = camera->target;
    }
};

int main() {
    size_t count = 0;

    const int w = 600, h = 600;
    SetConfigFlags(
        FLAG_WINDOW_RESIZABLE |
        FLAG_WINDOW_MAXIMIZED);
    InitWindow(w, h, "Terrain");
    SetTargetFPS(60);
    rlEnableSmoothLines();
    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();

    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 13.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    DisableCursor();

    Terrain terrain = Terrain(
        [](Vector2 p){
            auto noise = [](Vector2 p) {
                return sinf(p.x) + sinf(p.y);
            };
            auto tanh = [](Vector2 p) {
                return std::tanh(p.x) + std::tanh(p.y);
            };
            auto fbm = [=](Vector2 p) {
                constexpr float N_OCTAVES = 8;
                float res = 0.f;
                float amp = 0.5;
                float freq = 1.95;
                for (int i = 0; i < N_OCTAVES; i++) {
                    res += amp * noise(p);
                    amp *= 0.5;
                    p = Vector2Scale(p, freq);
                    p = Vector2Rotate(p, PI / 4.f);
                    p = Vector2AddValue(p, res * 0.6);
                }
                return res;
            };

            p = Vector2Scale(p, .5f);
            float d = 0;
            d += 5 * fbm(Vector2Scale(p, .2f));
            d += 0.2 * fbm(Vector2Scale(p, 5));
            d -= 2.3 * noise(Vector2Scale(p, 0.02));
            d += 4.5 * noise(Vector2Scale(p, 0.1));
            return d;
        }, 2.f);

    Shader shader = LoadShader("shaders/lighting.vert", "shaders/lighting.frag");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.01f, 0.01f, 0.01f, 1.0f }, SHADER_UNIFORM_VEC4);

    float halfwidth = 30;
    Model model = terrain.model({-halfwidth, -halfwidth}, halfwidth*2, .12f);
    model.materials[0].shader = shader;

    Agent agent = Agent(0.4, shader);
    LightSrc sun = LightSrc(0, .005f, 100, WHITE, shader);
    LightSrc moon = LightSrc(PI, .005f, 70, {50, 50, 50, 255}, shader);

    while (!WindowShouldClose()) {
        float dy = .2f;
        if (IsKeyDown(KEY_SPACE)) {
            camera.position.y += dy;
            camera.target.y += dy;
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            camera.position.y -= dy;
            camera.target.y -= dy;
        }
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

        sun.update(camera);
        moon.update(camera);
        agent.update(terrain, &camera);

        BeginDrawing();
            float sky = cosf(sun.angle) + 1;
            unsigned char skyColor = Remap(sky, 0, 2, 10, 125);
            ClearBackground(
                {skyColor, skyColor, static_cast<unsigned char>(skyColor + 10),
                222});

            BeginMode3D(camera);
                DrawModel(model, {0, 0, 0}, terrain.scale, {30, 30, 30, 255});
                DrawModel(agent.model, agent.position, 1, BLUE);
                DrawSphereEx(sun.light.position, 1.f, 8, 8, sun.light.color);
                DrawSphereEx(moon.light.position, .8f, 8, 8, RAYWHITE);
            EndMode3D();

            DrawFPS(2, 2);
        EndDrawing();
    }

    UnloadModel(model);
    UnloadShader(shader);

    CloseWindow();
    return 0;
}
