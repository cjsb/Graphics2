#include "Mesh.h"
#include <GLFW/glfw3.h>
#include "Binding.h"
#include <numeric>
#include <execution>

Mesh::Mesh(aiMesh* assimpMesh, bool useOwnBuffers) : m_vertexBuffer(GL_ARRAY_BUFFER), m_normalBuffer(GL_ARRAY_BUFFER), m_texCoordBuffer(GL_ARRAY_BUFFER), m_indexBuffer(GL_ELEMENT_ARRAY_BUFFER)
{
    if (!assimpMesh->HasNormals() || /* !assimpMesh->HasTextureCoords(0)  || */ !assimpMesh->HasFaces())
    {
        throw std::runtime_error("Mesh must have normals, tex coords, faces");
    }

    m_vertices.resize(assimpMesh->mNumVertices);
    m_normals.resize(assimpMesh->mNumVertices);
    m_texCoords.resize(assimpMesh->mNumVertices);

#pragma omp parallel for
    for (int64_t i = 0; i < static_cast<int64_t>(assimpMesh->mNumVertices); i++)
    {
        const auto aivec = assimpMesh->mVertices[i];
        const glm::vec3 vertex(aivec.x, aivec.y, aivec.z);
        m_vertices.at(i) = vertex;

        const auto ainorm = assimpMesh->mNormals[i];
        const glm::vec3 normal(ainorm.x, ainorm.y, ainorm.z);
        m_normals.at(i) = normal;

        if (assimpMesh->HasTextureCoords(0))
        {
            const aiVector3D aitex = assimpMesh->mTextureCoords[0][i];
            const glm::vec3 tex(aitex.x, aitex.y, aitex.z);
            m_texCoords.at(i) = tex;
        }
    }

    for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
    {
        const auto face = assimpMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            m_indices.push_back(face.mIndices[j]);
        }
    }

    m_materialIndex = assimpMesh->mMaterialIndex;

    if(useOwnBuffers)
    {
        m_vertexBuffer.setStorage(m_vertices, GL_DYNAMIC_STORAGE_BIT);
        m_normalBuffer.setStorage(m_normals, GL_DYNAMIC_STORAGE_BIT);
        m_texCoordBuffer.setStorage(m_texCoords, GL_DYNAMIC_STORAGE_BIT);
        m_indexBuffer.setStorage(m_indices, GL_DYNAMIC_STORAGE_BIT);
        m_vao.connectBuffer(m_vertexBuffer, BufferBindings::VertexAttributeLocation::vertices, 3, GL_FLOAT, GL_FALSE);
        m_vao.connectBuffer(m_normalBuffer, BufferBindings::VertexAttributeLocation::normals, 3, GL_FLOAT, GL_FALSE);

        if (assimpMesh->HasTextureCoords(0))
            m_vao.connectBuffer(m_texCoordBuffer, BufferBindings::VertexAttributeLocation::texCoords, 3, GL_FLOAT, GL_FALSE);

        m_vao.connectIndexBuffer(m_indexBuffer);
    }

    calculateBoundingBox();
}

Mesh::Mesh(std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& normals, std::vector<unsigned>& indices)
    : m_vertices(vertices), m_normals(normals), m_indices(indices),
      m_vertexBuffer(GL_ARRAY_BUFFER), m_normalBuffer(GL_ARRAY_BUFFER), m_texCoordBuffer(GL_ARRAY_BUFFER), m_indexBuffer(GL_ELEMENT_ARRAY_BUFFER)
{
    // TODO add version without normals (?) currently "faking" no normals because no empty buffers are allowed
    m_vertexBuffer.setStorage(m_vertices, GL_DYNAMIC_STORAGE_BIT);
    m_normalBuffer.setStorage(m_normals, GL_DYNAMIC_STORAGE_BIT);
    m_indexBuffer.setStorage(m_indices, GL_DYNAMIC_STORAGE_BIT);
    m_vao.connectBuffer(m_vertexBuffer, BufferBindings::VertexAttributeLocation::vertices, 3, GL_FLOAT, GL_FALSE);
    m_vao.connectBuffer(m_normalBuffer, BufferBindings::VertexAttributeLocation::normals, 3, GL_FLOAT, GL_FALSE);
    m_vao.connectIndexBuffer(m_indexBuffer);

    calculateBoundingBox();
}

void Mesh::draw() const
{
    if(m_enabledForRendering)
    {
        forceDraw();
    }
}

void Mesh::forceDraw() const
{
    m_vao.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, nullptr);
}

void Mesh::setModelMatrix(const glm::mat4& modelMatrix)
{
    m_modelMatrix = modelMatrix;
}

const glm::mat4& Mesh::getModelMatrix() const
{
    return m_modelMatrix;
}

unsigned Mesh::getMaterialID() const
{
    return m_materialID;
}

unsigned Mesh::getMaterialIndex() const
{
    return m_materialIndex;
}

void Mesh::setMaterialID(const unsigned materialID)
{
    m_materialID = materialID;
}

const std::vector<glm::vec3>& Mesh::getVertices() const
{
    if (m_vertices.empty())
        throw std::runtime_error("This mesh has no vertices!");

    return m_vertices;
}

const std::vector<glm::vec3>& Mesh::getNormals() const
{
    if (m_vertices.empty())
        throw std::runtime_error("This mesh has no normals!");

    return m_normals;
}

const std::vector<glm::vec3>& Mesh::getTexCoords() const
{
    if (m_vertices.empty())
        throw std::runtime_error("This mesh has no texture coordinates!");

    return m_texCoords;
}

const std::vector<unsigned int>& Mesh::getIndices() const
{
    if (m_vertices.empty())
        throw std::runtime_error("This mesh has no indices!");

    return m_indices;
}

const glm::mat2x3& Mesh::getBoundingBox() const
{
    return m_boundingBox;
}

const glm::mat2x3& Mesh::calculateBoundingBox()
{
    auto minMaxFun = util::make_overload(
        [](glm::mat2x3 b1, glm::mat2x3 b2) {return glm::mat2x3(glm::min(b1[0], b2[0]), glm::max(b1[1], b2[1])); },
        [](glm::vec3 b1, glm::vec3 b2) {return glm::mat2x3(glm::min(b1, b2), glm::max(b1, b2)); },
        [](glm::vec3 b1, glm::mat2x3 b2) {return glm::mat2x3(glm::min(b1, b2[0]), glm::max(b1, b2[1])); },
        [](glm::mat2x3 b1, glm::vec3 b2) {return glm::mat2x3(glm::min(b1[0], b2), glm::max(b1[1], b2)); }
    );

    m_boundingBox = std::reduce(std::execution::par, getVertices().begin(), getVertices().end(),
        glm::mat2x3(glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::lowest())), minMaxFun);

    return m_boundingBox;
}

void Mesh::setEnabledForRendering(bool enable)
{
    m_enabledForRendering = enable;
}

bool Mesh::isEnabledForRendering() const
{
    return m_enabledForRendering;
}
