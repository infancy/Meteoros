#include "model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/ObjLoadingLibraries/tiny_obj_loader.h"

Model::Model(VulkanDevice* device, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
	: device(device), vertices(vertices), indices(indices)
{
	if (vertices.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->vertices.data(), vertices.size() * sizeof(Vertex), 
										VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
	}

	if (indices.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->indices.data(), indices.size() * sizeof(uint32_t), 
										VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
	}

	modelBufferObject.modelMatrix = glm::mat4(1.0f);
	BufferUtils::CreateBufferFromData(device, commandPool, &modelBufferObject, sizeof(ModelBufferObject), 
									VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, modelBuffer, modelBufferMemory);
}

Model::Model(VulkanDevice* device, VkCommandPool commandPool, const std::string model_path, const std::string texture_path)
{
	LoadModel(model_path);

	if (vertices.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->vertices.data(), vertices.size() * sizeof(Vertex),
										VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
	}

	if (indices.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->indices.data(), indices.size() * sizeof(uint32_t),
										VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
	}

	modelBufferObject.modelMatrix = glm::mat4(1.0f);
	BufferUtils::CreateBufferFromData(device, commandPool, &modelBufferObject, sizeof(ModelBufferObject),
									VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, modelBuffer, modelBufferMemory);

	SetTexture(device, commandPool, texture_path, texture, textureMemory, textureView, textureSampler);
}

Model::~Model()
{
	if (indices.size() > 0) 
	{
		vkDestroyBuffer(device->GetVkDevice(), indexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), indexBufferMemory, nullptr);
	}

	if (vertices.size() > 0) 
	{
		vkDestroyBuffer(device->GetVkDevice(), vertexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), vertexBufferMemory, nullptr);
	}

	vkDestroyBuffer(device->GetVkDevice(), modelBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), modelBufferMemory, nullptr);

	if (textureView != VK_NULL_HANDLE) {
		vkDestroyImageView(device->GetVkDevice(), textureView, nullptr);
	}

	if (textureSampler != VK_NULL_HANDLE) {
		vkDestroySampler(device->GetVkDevice(), textureSampler, nullptr);
	}
}

void Model::SetTexture(VulkanDevice* device, VkCommandPool commandPool, const std::string texture_path,
					VkImage& textureImage, VkDeviceMemory& textureImageMemory, 
					VkImageView& textureImageView, VkSampler& textureSampler)
{
	Image::loadTexture(device, commandPool, texture_path.c_str(), textureImage, textureImageMemory,
					VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Image::createImageView(device, textureImageView, textureImage,
						VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	Image::createSampler(device, textureSampler);
}
void Model::LoadModel(const std::string model_path)
{
	// The attrib container holds all of the positions, normals and texture coordinates 
	// in its attrib.vertices, attrib.normals and attrib.texcoords vectors.
	tinyobj::attrib_t attrib;
	// The shapes container contains all of the separate objects and their faces.
	std::vector<tinyobj::shape_t> shapes;
	// OBJ models can define a material and texture per face --> ignored for now
	std::vector<tinyobj::material_t> materials;
	std::string err;

	//Faces in OBJ files can actually contain an arbitrary number of vertices, whereas our application can only render triangles. 
	//Luckily the LoadObj has an optional parameter to automatically triangulate such faces, which is enabled by default.
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model_path.c_str())) {
		throw std::runtime_error(err);
	}

	for (const auto& shape : shapes) 
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.position = glm::vec4(attrib.vertices[3 * index.vertex_index + 0],
										attrib.vertices[3 * index.vertex_index + 1],
										attrib.vertices[3 * index.vertex_index + 2], 
										1.0f);
			
			vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

			//The origin of texture coordinates in Vulkan is the top-left corner, whereas 
			//the OBJ format assumes the bottom-left corner. 
			//Solve this by flipping the vertical component of the texture coordinates
			vertex.texCoord = glm::vec2(attrib.texcoords[2 * index.texcoord_index + 0],
										1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);

			vertices.push_back(vertex);
			indices.push_back(indices.size());
		}
	}
}

const std::vector<Vertex>& Model::getVertices() const
{
	return vertices;
}
const std::vector<uint32_t>& Model::getIndices() const
{
	return indices;
}

VkBuffer Model::getVertexBuffer() const
{
	return vertexBuffer;
}
VkBuffer Model::getIndexBuffer() const
{
	return indexBuffer;
}

//VkBuffer* Model::getPointerToVertexBuffer() const
//{
//	return vertexBuffer;
//}
//VkBuffer* Model::getPointerToIndexBuffer() const
//{
//	return indexBuffer;
//}

uint32_t Model::getVertexBufferSize() const
{
	return static_cast<uint32_t>(vertices.size() * sizeof(vertices[0]));
}
uint32_t Model::getIndexBufferSize() const
{
	return static_cast<uint32_t>(indices.size() * sizeof(indices[0]));
}

const ModelBufferObject& Model::getModelBufferObject() const
{
	return modelBufferObject;
}

VkBuffer Model::GetModelBuffer() const
{
	return modelBuffer;
}
VkImageView Model::GetTextureView() const
{
	return textureView;
}
VkSampler Model::GetTextureSampler() const
{
	return textureSampler;
}