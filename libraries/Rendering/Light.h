#pragma once

#include <optional>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include "Buffer.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "Mesh.h"
#include "FrameBuffer.h"


class ModelImporter;
using namespace gl;

struct GPULight
{
    glm::mat4 lightSpaceMatrix;
    glm::vec3 color;                // all
    int type = -1;                  // 0 directional, 1 point light, 2 spot light
    glm::vec3 position;             // spot, point
    float constant = -1.0f;         // spot, point
    glm::vec3 direction;            // dir, spot
    float linear = -1.0f;           // spot, point
    uint64_t shadowMap;
    float quadratic = -1.0f;        // spot, point
    float cutOff = -1.0f;           // spot
    float outerCutOff = -1.0f;      // spot
    int pcfKernelSize = 1;
    int32_t pad1, pad2;
};

enum class LightType : int
{
    directional = 0,
    point = 1,
    spot = 2
};

class Light
{
public:

    Light(glm::vec3 color, glm::vec3 direction, float smFar = 3000.0f, glm::ivec2 shadowMapRes = glm::ivec2(4096, 4096)); // DIRECTIONAL
    Light(glm::vec3 color, glm::vec3 position, float constant, float linear, float quadratic, float smFar = 3000.0f, glm::ivec2 shadowMapRes = glm::ivec2(512, 512)); // POINT
    Light(glm::vec3 color, glm::vec3 position, glm::vec3 direction, float constant, float linear, float quadratic, float cutOff, float outerCutOff, float smFar = 3000.0f, glm::ivec2 shadowMapRes = glm::ivec2(4096, 4096)); // SPOT

    void renderShadowMap(const std::vector<std::shared_ptr<Mesh>>& meshes);
    void renderShadowMap(const ModelImporter& mi);
    void renderShadowMapCulled(const ModelImporter& mi);

    const GPULight& getGpuLight() const;

    void recalculateLightSpaceMatrix();

    bool showLightGUI(const std::string& name = "Light");
	bool showLightGUIContent(const std::string& name = "Light");

    // getters & setters 
    void setColor(glm::vec3 col);
    glm::vec3 getColor() const;

    void setPosition(glm::vec3 pos);
    glm::vec3 getPosition() const;

    void setDirection(glm::vec3 dir);
    glm::vec3 getDirection() const;

    void setConstant(float constant);
    float getConstant() const;

    void setLinear(float linear);
    float getLinear() const;

    void setQuadratic(float quadratic);
    float getQuadratic() const;

    void setCutoff(float cutoff);
    float getCutoff() const;

    void setOuterCutoff(float cutOff);
    float getOuterCutoff() const;

    void setPCFKernelSize(int size);
    int getPCFKernelSize() const;

    LightType getType() const;

    void setOuterBoundingBox(const glm::mat2x4& outerBoundingBox);

private:
    void checkParameters();

    LightType m_type;

    bool m_hasShadowMap = true;
    glm::ivec2 m_shadowMapRes;
    float m_smFar;

    glm::mat4 m_lightProjection;
    glm::mat4 m_lightView;

    std::shared_ptr<Texture> m_shadowTexture;
    FrameBuffer m_shadowMapFBO;
    ShaderProgram m_genShadowMapProgram;

    std::optional<glm::mat2x4> m_outerSceneBoundingBox;

    std::shared_ptr<Uniform<glm::mat4>> m_modelUniform;
    std::shared_ptr<Uniform<glm::mat4>> m_lightSpaceUniform;
    std::shared_ptr<Uniform<glm::vec3>> m_lightPosUniform;

    GPULight m_gpuLight;
};