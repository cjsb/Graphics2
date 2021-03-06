#include "Light.h"
#include <glm/gtc/matrix_transform.inl>
#include "imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include "Cubemap.h"
#include "IO/ModelImporter.h"
#include <glm/gtx/component_wise.inl>

using namespace gl;

Light::Light(glm::vec3 color, glm::vec3 direction, float smFar ,glm::ivec2 shadowMapRes) // DIRECTIONAL
: m_type(LightType::directional), m_shadowMapRes(shadowMapRes), m_smFar(smFar),
m_shadowTexture(std::make_shared<Texture>(GL_TEXTURE_2D, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR)), m_shadowMapFBO(GL_DEPTH_ATTACHMENT, *m_shadowTexture),
m_genShadowMapProgram("lightTransform.vert", "smAlpha.frag", BufferBindings::g_definitions)
{
    checkParameters();

    // init shadowMap
    m_shadowTexture->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_shadowTexture->getName(), GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_shadowTexture->getName(), GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    m_shadowTexture->initWithoutData(m_shadowMapRes.x, m_shadowMapRes.y, GL_DEPTH_COMPONENT32F);

    m_modelUniform = std::make_shared<Uniform<glm::mat4>>("ModelMatrix", glm::mat4(1.0f));
    m_lightSpaceUniform = std::make_shared<Uniform<glm::mat4>>("lightSpaceMatrix", glm::mat4(1.0f));

    m_genShadowMapProgram.addUniform(m_modelUniform);
    m_genShadowMapProgram.addUniform(m_lightSpaceUniform);

    // init gpu struct
    m_gpuLight.type = static_cast<int>(m_type);
    m_gpuLight.color = color;
    m_gpuLight.direction = direction;
    m_gpuLight.shadowMap = m_shadowTexture->generateHandle();

    recalculateLightSpaceMatrix();
}

Light::Light(glm::vec3 color, glm::vec3 position, float constant, float linear, float quadratic, float smFar, glm::ivec2 shadowMapRes) // POINT
: m_type(LightType::point), m_shadowMapRes(shadowMapRes), m_smFar(smFar),
m_shadowTexture(std::make_shared<Cubemap>(GL_LINEAR, GL_LINEAR)), m_shadowMapFBO(GL_DEPTH_ATTACHMENT, *m_shadowTexture),
m_genShadowMapProgram({
    Shader("transform.vert", GL_VERTEX_SHADER, BufferBindings::g_definitions),
    Shader("omnidirectional.geom", GL_GEOMETRY_SHADER, BufferBindings::g_definitions),
    Shader("omnidirectional.frag", GL_FRAGMENT_SHADER, BufferBindings::g_definitions) })
{
    checkParameters();

    // TODO THIS IS TEMPORARY; IMPLEMENT OMNIDIR. SHADOW MAPS
    // init shadowMap 
    auto currentCubemap = std::static_pointer_cast<Cubemap>(m_shadowTexture);
    currentCubemap->initWithoutData(m_shadowMapRes.x, m_shadowMapRes.y, GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT, 6);

    m_lightPosUniform = std::make_shared<Uniform<glm::vec3>>("lightPos", position);

    m_modelUniform = std::make_shared<Uniform<glm::mat4>>("ModelMatrix", glm::mat4(1.0f));
    m_lightSpaceUniform = std::make_shared<Uniform<glm::mat4>>("lightSpaceMatrix", glm::mat4(1.0f));

    m_genShadowMapProgram.addUniform(m_modelUniform);
    m_genShadowMapProgram.addUniform(m_lightSpaceUniform);
    m_genShadowMapProgram.addUniform(m_lightPosUniform);

    // init gpu struct
    m_gpuLight.type = static_cast<int>(m_type);
    m_gpuLight.color = color;
    m_gpuLight.position = position;
    m_gpuLight.constant = constant;
    m_gpuLight.linear = linear;
    m_gpuLight.quadratic = quadratic;
    m_gpuLight.shadowMap = currentCubemap->generateHandle();
    recalculateLightSpaceMatrix();

    std::cout << "Shadow maps for point lights are not supported yet\n";
    //throw std::runtime_error("POINT LIGHTS NOT SUPPORTED YET");
}

Light::Light(glm::vec3 color, glm::vec3 position, glm::vec3 direction, float constant, float linear, float quadratic, float cutOff, float outerCutOff, float smFar, glm::ivec2 shadowMapRes) // SPOT
    : m_type(LightType::spot), m_shadowMapRes(shadowMapRes), m_smFar(smFar),
    m_shadowTexture(std::make_shared<Texture>(GL_TEXTURE_2D, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR)), m_shadowMapFBO(GL_DEPTH_ATTACHMENT, *m_shadowTexture),
   m_genShadowMapProgram("lightTransform.vert", "smAlpha.frag", BufferBindings::g_definitions)
{
    checkParameters();

    // init shadowMap
    m_shadowTexture->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_shadowTexture->getName(), GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_shadowTexture->getName(), GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    m_shadowTexture->initWithoutData(m_shadowMapRes.x, m_shadowMapRes.y, GL_DEPTH_COMPONENT32F);

    m_modelUniform = std::make_shared<Uniform<glm::mat4>>("ModelMatrix", glm::mat4(1.0f));
    m_lightSpaceUniform = std::make_shared<Uniform<glm::mat4>>("lightSpaceMatrix", glm::mat4(1.0f));

    m_genShadowMapProgram.addUniform(m_modelUniform);
    m_genShadowMapProgram.addUniform(m_lightSpaceUniform);

    // init gpu struct
    m_gpuLight.type = static_cast<int>(m_type);
    m_gpuLight.color = color;
    m_gpuLight.position = position;
    m_gpuLight.direction = direction;
    m_gpuLight.constant = constant;
    m_gpuLight.linear = linear;
    m_gpuLight.quadratic = quadratic;
    m_gpuLight.cutOff = cutOff;
    m_gpuLight.outerCutOff = outerCutOff;
    m_gpuLight.shadowMap = m_shadowTexture->generateHandle();

    recalculateLightSpaceMatrix();
}

void Light::renderShadowMap(const std::vector<std::shared_ptr<Mesh>>& meshes)
{
    if (!m_hasShadowMap)
        return;

    recalculateLightSpaceMatrix();

    //store old viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    //set sm settings
    m_genShadowMapProgram.use();
    glViewport(0, 0, m_shadowMapRes.x, m_shadowMapRes.y);
    m_shadowMapFBO.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    if (m_type == LightType::point && m_lightPosUniform->getContent() != m_gpuLight.position)
        m_lightPosUniform->setContent(m_gpuLight.position);

    GLuint srIndex[] = { 1 };
    glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, srIndex);

    //render scene
    std::for_each(meshes.begin(), meshes.end(), [this](auto& mesh)
    {
        m_modelUniform->setContent(mesh->getModelMatrix());
        m_genShadowMapProgram.updateUniforms();
        mesh->draw();
    });

    m_shadowTexture->generateMipmap();

    //restore previous rendering settings
    m_shadowMapFBO.unbind();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glCullFace(GL_BACK);
}

void Light::renderShadowMap(const ModelImporter& mi)
{
    if (!m_hasShadowMap)
        return;

    recalculateLightSpaceMatrix();

    //store old viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    //set sm settings
    m_genShadowMapProgram.use();
    glViewport(0, 0, m_shadowMapRes.x, m_shadowMapRes.y);
    m_shadowMapFBO.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    if (m_type == LightType::point && m_lightPosUniform->getContent() != m_gpuLight.position)
        m_lightPosUniform->setContent(m_gpuLight.position);

    //render scene
    mi.multiDraw(m_genShadowMapProgram);

    m_shadowTexture->generateMipmap();

    //restore previous rendering settings
    m_shadowMapFBO.unbind();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glCullFace(GL_BACK);
}

void Light::renderShadowMapCulled(const ModelImporter& mi)
{
    if (!m_hasShadowMap)
        return;

    // no culling for point lights
    if (m_type == LightType::point)
    {
        std::cout << "Shadow maps for point lights not supported\n";
    }

    recalculateLightSpaceMatrix();

    //store old viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    //set sm settings
    m_genShadowMapProgram.use();
    glViewport(0, 0, m_shadowMapRes.x, m_shadowMapRes.y);
    m_shadowMapFBO.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    if (m_type == LightType::point && m_lightPosUniform->getContent() != m_gpuLight.position)
        m_lightPosUniform->setContent(m_gpuLight.position);

    //render scene
    mi.multiDrawCulled(m_genShadowMapProgram, m_gpuLight.lightSpaceMatrix);

    m_shadowTexture->generateMipmap();

    //restore previous rendering settings
    m_shadowMapFBO.unbind();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glCullFace(GL_BACK);
}

void Light::recalculateLightSpaceMatrix()
{
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (glm::length(glm::cross(m_gpuLight.direction, up)) < 0.01f)
    {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    if (m_type == LightType::directional)
    {
        if (!m_outerSceneBoundingBox.has_value())
        {
            m_lightProjection = glm::ortho(-2000.0f, 2000.0f, -2000.0f, 2000.0f, 0.1f, m_smFar);
            m_gpuLight.position = glm::vec3(0.0f, 2000.0f, 0.0f);
        }
        else
        {
            glm::mat2x4 b = m_outerSceneBoundingBox.value();

            const float bboxSize = glm::length(b[1] - b[0]);
            const glm::vec3 bboxCenter = 0.5f * (b[1] + b[0]);
            m_gpuLight.position = bboxCenter + 0.5f * bboxSize * glm::normalize(-m_gpuLight.direction);

            //glm::vec2 lowest2D(b[0].x, b[0].z);
            //glm::vec2 highest2D(b[1].x, b[1].z);
            //glm::vec2 center((lowest2D.x + lowest2D.y) / 2.0f, (highest2D.x + highest2D.y) / 2.0f);
            //glm::vec2 lowerCorner = center - (1.5f * abs(lowest2D));
            //glm::vec2 upperCorner = center + (1.5f * abs(highest2D));
            //float min = compMin(lowerCorner);
            //float max = compMax(upperCorner);

            const glm::vec2 bb = glm::vec2(glm::compMin(b[0]), glm::compMax(b[1]));
            const float min = bb.x - 0.244f * abs(bb.x); //0.244 = (sqrt(3)-1)/3
            const float max = bb.y + 0.244f * abs(bb.y);
            
            m_lightProjection = glm::ortho(min, max, min, max, 0.1f, bboxSize);
        }
    }
    else if (m_type == LightType::spot) 
    {
        // NOTE: ACOS BECAUSE CUTOFF HAS COS BAKED IN
        m_lightProjection = glm::perspective(2.0f*glm::acos(m_gpuLight.outerCutOff), static_cast<float>(m_shadowMapRes.x) / static_cast<float>(m_shadowMapRes.y), 0.1f, m_smFar);
    }
    else if (m_type == LightType::point)
    {
        // TODO is cutoff supposed to be used here? or 90 degrees (cube)?
        m_lightProjection = glm::perspective(glm::radians(90.0f), static_cast<float>(m_shadowMapRes.x) / static_cast<float>(m_shadowMapRes.y), 0.1f, m_smFar);
        m_lightView = glm::mat4(1.0f); // calculate finished matrix in shader
    }

    m_lightView = glm::lookAt(m_gpuLight.position,
        m_gpuLight.position + m_gpuLight.direction,
        up);

    m_gpuLight.lightSpaceMatrix = m_lightProjection * m_lightView;
    m_lightSpaceUniform->setContent(m_gpuLight.lightSpaceMatrix);
}

void Light::checkParameters()
{
    // TODO implement this if there are constructors that do not initalize all the params for the specific type
}

const GPULight& Light::getGpuLight() const
{
    return m_gpuLight;
}

void Light::setPosition(glm::vec3 pos)
{
    m_gpuLight.position = pos;
    recalculateLightSpaceMatrix();
}

void Light::setColor(glm::vec3 col)
{
    m_gpuLight.color = col;
}

void Light::setCutoff(float cutoff)
{
    m_gpuLight.cutOff = cutoff;
    recalculateLightSpaceMatrix();
}

void Light::setDirection(glm::vec3 dir)
{
    m_gpuLight.direction = dir;
    recalculateLightSpaceMatrix();
}


void Light::setConstant(float constant)
{
    m_gpuLight.constant = constant;
}

float Light::getConstant() const
{
    return m_gpuLight.constant;
}

void Light::setLinear(float linear)
{
    m_gpuLight.linear = linear;
}

float Light::getLinear() const
{
    return m_gpuLight.linear;
}

void Light::setQuadratic(float quadratic)
{
    m_gpuLight.quadratic = quadratic;
}

float Light::getQuadratic() const
{
    return m_gpuLight.quadratic;
}

glm::vec3 Light::getPosition() const
{
    return m_gpuLight.position;
}

glm::vec3 Light::getColor() const
{
    return m_gpuLight.color;
}

float Light::getCutoff() const
{
    return m_gpuLight.cutOff;
}

void Light::setOuterCutoff(float cutOff)
{
    m_gpuLight.outerCutOff = cutOff;
}

float Light::getOuterCutoff() const
{
    return m_gpuLight.outerCutOff;
}

void Light::setPCFKernelSize(int size)
{
    m_gpuLight.pcfKernelSize = size;
}

int Light::getPCFKernelSize() const
{
    return m_gpuLight.pcfKernelSize;
}

LightType Light::getType() const
{
    return m_type;
}

void Light::setOuterBoundingBox(const glm::mat2x4& outerBoundingBox)
{
    m_outerSceneBoundingBox = std::make_optional(outerBoundingBox);
}

glm::vec3 Light::getDirection() const
{
    return m_gpuLight.direction;
}


bool Light::showLightGUI(const std::string& name)
{
    ImGui::Begin("Light GUI");
    const bool lightChanged = showLightGUIContent(name);
    ImGui::End();

    return lightChanged;
}

bool Light::showLightGUIContent(const std::string& name)
{
    std::array<std::string, 3> lightTypeNames = { "Directional", "Point", "Spot" };

    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
    std::stringstream fullName;
    fullName << name << " (Type: " << lightTypeNames[static_cast<int>(m_type)] << ")";
    bool lightChanged = false;
    bool matNeedsUpdate = false;
	if (ImGui::CollapsingHeader(fullName.str().c_str()))
	{
		if (ImGui::DragFloat3((std::string("Color ") + name).c_str(), value_ptr(m_gpuLight.color)))
		{
			lightChanged = true;
		}
		if (ImGui::SliderInt((std::string("PCF Kernel Size ") + name).c_str(), &m_gpuLight.pcfKernelSize, 0, 10))
		{
			lightChanged = true;
		}
		if (m_type == LightType::directional || m_type == LightType::spot)
		{
			if (ImGui::SliderFloat3((std::string("Direction ") + name).c_str(), value_ptr(m_gpuLight.direction), -1.0f, 1.0f))
			{
				lightChanged = true;
				matNeedsUpdate = true;
			}
		}
		if (m_type == LightType::spot)
		{
			if (ImGui::SliderFloat((std::string("Cutoff ") + name).c_str(), &m_gpuLight.cutOff, 0.0f, glm::radians(90.0f)))
			{
				lightChanged = true;
				matNeedsUpdate = true;
				if (m_gpuLight.cutOff < m_gpuLight.outerCutOff)
					m_gpuLight.outerCutOff = m_gpuLight.cutOff - 0.001f;
			}
			if (ImGui::SliderFloat((std::string("Outer cutoff ") + name).c_str(), &m_gpuLight.outerCutOff, 0.0f, glm::radians(90.0f)))
			{
				lightChanged = true;
				matNeedsUpdate = true;
				if (m_gpuLight.cutOff < m_gpuLight.outerCutOff)
					m_gpuLight.cutOff = m_gpuLight.outerCutOff + 0.001f;
			}
		}
		if (m_type == LightType::spot || m_type == LightType::point)
		{
			if (ImGui::DragFloat3((std::string("Position ") + name).c_str(), value_ptr(m_gpuLight.position)))
			{
				lightChanged = true;
				matNeedsUpdate = true;
			}
			if (ImGui::SliderFloat((std::string("Constant ") + name).c_str(), &m_gpuLight.constant, 0.0f, 1.0f))
			{
				lightChanged = true;
			}
			if (ImGui::SliderFloat((std::string("Linear ") + name).c_str(), &m_gpuLight.linear, 0.0f, 0.25f))
			{
				lightChanged = true;
			}
			if (ImGui::SliderFloat((std::string("Quadratic ") + name).c_str(), &m_gpuLight.quadratic, 0.0f, 0.1f))
			{
				lightChanged = true;
			}
		}
	}
    if (matNeedsUpdate) recalculateLightSpaceMatrix();

    return lightChanged;
}