#include "mesh.h"

namespace deep 
{
	Mesh::Mesh(SDL_GPUDevice* device)
	{
		m_device = device;
		// create the vertex buffer
		SDL_GPUBufferCreateInfo bufferInfo{};
		bufferInfo.size = sizeof(vertices);
		bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		m_vertexBuffer = SDL_CreateGPUBuffer(m_device, &bufferInfo);

		// create a transfer buffer to upload to the vertex buffer
		SDL_GPUTransferBufferCreateInfo transferInfo{};
		transferInfo.size = sizeof(vertices);
		transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		m_transferBuffer = SDL_CreateGPUTransferBuffer(m_device, &transferInfo);

		// fill the transfer buffer
		Vertex* data = (Vertex*)SDL_MapGPUTransferBuffer(m_device, m_transferBuffer, false);
		
		SDL_memcpy(data, (void*)vertices, sizeof(vertices));

		// data[0] = vertices[0];
		// data[1] = vertices[1];
		// data[2] = vertices[2];

		SDL_UnmapGPUTransferBuffer(m_device, m_transferBuffer);

		// start a copy pass
		SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(m_device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

		// where is the data
		SDL_GPUTransferBufferLocation location{};
		location.transfer_buffer = m_transferBuffer;
		location.offset = 0;
		
		// where to upload the data
		SDL_GPUBufferRegion region{};
		region.buffer = m_vertexBuffer;
		region.size = sizeof(vertices);
		region.offset = 0;

		// upload the data
		SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

		// end the copy pass
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(commandBuffer);
	}

	void Mesh::destroy() 
	{
		SDL_ReleaseGPUBuffer(m_device, m_vertexBuffer);
    	SDL_ReleaseGPUTransferBuffer(m_device, m_transferBuffer);
	}

	SDL_GPUBuffer* Mesh::getVertexBuffer(){
		return m_vertexBuffer;
	}
}