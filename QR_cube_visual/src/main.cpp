// main.cpp
//
// Application entry point: creates the GLFW window + OpenGL 3.3 core
// context, generates all 27 cubies' geometry once and concatenates it
// into ONE shared VBO/EBO (drawn via glDrawElementsBaseVertex per cubie,
// through a single shared VAO and shader program -- see the "combined
// buffer" comment below for why this satisfies the "single shared
// VAO/VBO/EBO reused across all 27 draw calls" requirement even though
// each cubie's pattern geometry differs), wires up the orbit camera and
// keyboard/mouse input, and runs the main render loop.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "Camera.hpp"
#include "Cubie.hpp"
#include "CubeFaceData.hpp"
#include "CubieMeshGenerator.hpp"
#include "InputController.hpp"
#include "RubiksCube.hpp"
#include "Shader.hpp"

namespace {

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// Determines, for one cubie at the given initial grid position, which of
// its 6 local faces are externally visible on the assembled 3x3x3 puzzle.
// A local face is visible exactly when its corresponding grid coordinate
// is at the extreme of the range, e.g. +X is visible iff pos.x == 1.
std::array<bool, qrcube::kFaceCount> computeVisibleFaces(glm::ivec3 pos) {
    std::array<bool, qrcube::kFaceCount> vis{};
    vis[static_cast<int>(qrcube::FaceId::PosX)] = (pos.x == 1);
    vis[static_cast<int>(qrcube::FaceId::NegX)] = (pos.x == -1);
    vis[static_cast<int>(qrcube::FaceId::PosY)] = (pos.y == 1);
    vis[static_cast<int>(qrcube::FaceId::NegY)] = (pos.y == -1);
    vis[static_cast<int>(qrcube::FaceId::PosZ)] = (pos.z == 1);
    vis[static_cast<int>(qrcube::FaceId::NegZ)] = (pos.z == -1);
    return vis;
}

} // namespace

int main() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    const int kInitialWidth = 1280;
    const int kInitialHeight = 800;
    GLFWwindow* window = glfwCreateWindow(kInitialWidth, kInitialHeight,
                                           "3D QR Rubik's Cube", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Failed to initialize GLAD\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glViewport(0, 0, kInitialWidth, kInitialHeight);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);

    // ------------------------------------------------------------------
    // Build all 27 cubies' geometry once, concatenated into one shared
    // vertex buffer and one shared index buffer.
    // ------------------------------------------------------------------
    qrcube::CubieMeshParams meshParams;
    std::vector<qrcube::Vertex> combinedVertices;
    std::vector<uint32_t> combinedIndices;
    std::vector<qrcube::Cubie> cubies;
    cubies.reserve(27);

    std::mt19937 patternRng(12345u); // deterministic so the cube looks the
                                      // same every run; change the seed (or
                                      // swap makePseudoRandomFaceData for
                                      // makeRingFaceData / your own data) to
                                      // customize the look -- see README.

    int cubieSeedCounter = 0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                glm::ivec3 gridPos(x, y, z);
                auto visibleFaces = computeVisibleFaces(gridPos);

                std::array<qrcube::CubeFaceData, qrcube::kFaceCount> faceData;
                for (int f = 0; f < qrcube::kFaceCount; ++f) {
                    if (visibleFaces[static_cast<size_t>(f)]) {
                        // Each visible face gets its own deterministic
                        // pseudo-random pattern -- this is where you'd
                        // instead plug in real QR-derived bit data; see
                        // README "Customizing the pattern data".
                        unsigned int seed = static_cast<unsigned int>(cubieSeedCounter * 7 + f + 1);
                        faceData[static_cast<size_t>(f)] = qrcube::makePseudoRandomFaceData(seed);
                    }
                }

                qrcube::CubieMesh mesh = qrcube::generateCubieMesh(meshParams, visibleFaces, faceData);

                qrcube::MeshRange range;
                range.baseVertex = static_cast<int32_t>(combinedVertices.size());
                range.indexOffsetBytes = static_cast<uint32_t>(combinedIndices.size() * sizeof(uint32_t));
                range.indexCount = static_cast<uint32_t>(mesh.indices.size());

                combinedVertices.insert(combinedVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
                combinedIndices.insert(combinedIndices.end(), mesh.indices.begin(), mesh.indices.end());

                cubies.emplace_back(gridPos, range);
                ++cubieSeedCounter;
            }
        }
    }

    qrcube::RubiksCube cube(std::move(cubies));

    // ------------------------------------------------------------------
    // Upload the combined geometry into ONE shared VAO/VBO/EBO. All 27
    // draw calls per frame reuse this same VAO and these same buffer
    // objects (via glDrawElementsBaseVertex with each cubie's own
    // baseVertex/indexOffset/indexCount) -- geometry is never regenerated
    // or re-uploaded per frame.
    // ------------------------------------------------------------------
    unsigned int vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(combinedVertices.size() * sizeof(qrcube::Vertex)),
                 combinedVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(combinedIndices.size() * sizeof(uint32_t)),
                 combinedIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(qrcube::Vertex),
                           reinterpret_cast<void*>(offsetof(qrcube::Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(qrcube::Vertex),
                           reinterpret_cast<void*>(offsetof(qrcube::Vertex, normal)));

    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // Shader, camera, input.
    // ------------------------------------------------------------------
    qrcube::Shader shader;
    try {
        shader = qrcube::Shader("shaders/cubie.vert", "shaders/cubie.frag");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Shader error: %s\n", e.what());
        glfwTerminate();
        return EXIT_FAILURE;
    }

    qrcube::Camera camera;
    qrcube::InputController input(window, camera, cube);

    double lastTime = glfwGetTime();

    // ------------------------------------------------------------------
    // Main render loop.
    // ------------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        // Guard against huge dt spikes (e.g. window was dragged/paused).
        if (dt > 0.1f) dt = 0.1f;

        glfwPollEvents();
        input.update();
        cube.update(dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight > 0) ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f;

        glm::mat4 view = camera.viewMatrix();
        glm::mat4 projection = camera.projectionMatrix(aspect);
        glm::vec3 viewPos = camera.position();
        glm::vec3 lightPos = viewPos + glm::vec3(2.0f, 4.0f, 3.0f);

        shader.use();
        shader.setMat4("uView", view);
        shader.setMat4("uProjection", projection);
        shader.setVec3("uViewPos", viewPos);
        shader.setVec3("uLightPos", lightPos);
        shader.setVec3("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        shader.setVec3("uObjectColor", glm::vec3(0.82f, 0.78f, 0.65f));

        glBindVertexArray(vao);
        for (int i = 0; i < cube.cubieCount(); ++i) {
            glm::mat4 model = cube.computeModelMatrix(i);
            glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));

            shader.setMat4("uModel", model);
            int normalMatLoc = glGetUniformLocation(shader.id(), "uNormalMatrix");
            glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, &normalMatrix[0][0]);

            const qrcube::MeshRange& range = cube.cubieAt(i).meshRange();
            glDrawElementsBaseVertex(
                GL_TRIANGLES, static_cast<GLsizei>(range.indexCount), GL_UNSIGNED_INT,
                reinterpret_cast<void*>(static_cast<uintptr_t>(range.indexOffsetBytes)),
                range.baseVertex);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glfwTerminate();
    return EXIT_SUCCESS;
}
