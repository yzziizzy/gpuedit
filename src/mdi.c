

#include "common_gl.h"
#include "common_math.h"

#include "mdi.h"
#include "utilities.h"



static void preFrame(PassFrameParams* pfp, MultiDrawIndirect* mdi);
static void core_draw(MultiDrawIndirect* mdi);
static void draw(MultiDrawIndirect* mdi, GLuint progID, PassDrawParams* pdp);
static void postFrame(MultiDrawIndirect* mdi);



MultiDrawIndirect* MultiDrawIndirect_alloc(VAOConfig* vaoConfig, int maxInstances, char* label) {
	MultiDrawIndirect* mdi;
	
	pcalloc(mdi);
	MultiDrawIndirect_init(mdi, vaoConfig, maxInstances, label);
	
	return mdi;
}




void MultiDrawIndirect_init(MultiDrawIndirect* mdi, VAOConfig* vaoConfig, int maxInstances, char* label) {
	
	if(label) mdi->label = label;
	
	mdi->primMode = GL_TRIANGLES;
	mdi->isIndexed = 0;
	
	VEC_INIT(&mdi->meshes);
	mdi->maxInstances = maxInstances;
	mdi->vaoConfig = vaoConfig;
	
	char* instLabelAdd = ", mdi instVB"; 
	char* instLabel = calloc(1, strlen(mdi->label) + strlen(instLabelAdd) + 1);
	strcat(instLabel, mdi->label);
	strcat(instLabel, instLabelAdd);
	
	char* cmdsLabelAdd = ", mdi drawCmds"; 
	char* cmdsLabel = calloc(1, strlen(mdi->label) + strlen(cmdsLabelAdd) + 1);
	strcat(cmdsLabel, mdi->label);
	strcat(cmdsLabel, cmdsLabelAdd);
	
	mdi->instVB.label = instLabel; 
	mdi->indirectCmds.label = cmdsLabel; 
}

void MultiDrawIndirect_initGL(MultiDrawIndirect* mdi) {
	
	mdi->vao = makeVAO(mdi->vaoConfig);
	glBindVertexArray(mdi->vao);
	
	int stride = calcVAOStride(1, mdi->vaoConfig);
	
	PCBuffer_startInit(&mdi->instVB, mdi->maxInstances * stride, GL_ARRAY_BUFFER);
	updateVAO(1, mdi->vaoConfig); 
	PCBuffer_finishInit(&mdi->instVB);
	
	
	PCBuffer_startInit(
		&mdi->indirectCmds, 
		16 * sizeof(DrawElementsIndirectCommand), // carefull here
		GL_DRAW_INDIRECT_BUFFER
	);
	PCBuffer_finishInit(&mdi->indirectCmds);
}





// should only used for initial setup
void MultiDrawIndirect_updateGeometry(MultiDrawIndirect* mdi) {
	
	int i;
	size_t offset;
	
	if(mdi->totalVertices == 0) {
		printf("no geometry found in MDI:updateGeometry\n");
		return;
	}
	
	glBindVertexArray(mdi->vao);
	
	// vertex buffer
	if(glIsBuffer(mdi->geomVBO)) glDeleteBuffers(1, &mdi->geomVBO);
	glGenBuffers(1, &mdi->geomVBO);
	
	glBindBuffer(GL_ARRAY_BUFFER, mdi->geomVBO);
	size_t stride = updateVAO(0, mdi->vaoConfig);

	glBufferStorage(GL_ARRAY_BUFFER, mdi->totalVertices * stride, NULL, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
	glexit("");
	
	
	void* buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	glexit("");
	
	offset = 0;
	for(i = 0; i < VEC_LEN(&mdi->meshes); i++) {
		MDIDrawInfo* di = VEC_ITEM(&mdi->meshes, i);
		
		memcpy(buf + offset, di->vertices, di->vertexCount * stride);
		offset += di->vertexCount * stride;
	}
	
	
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	
	// index buffers
	if(mdi->isIndexed) {
		
		if(glIsBuffer(mdi->geomIBO)) glDeleteBuffers(1, &mdi->geomIBO);
		glGenBuffers(1, &mdi->geomIBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdi->geomIBO);
		
		glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, mdi->totalIndices * mdi->indexSize, NULL, GL_MAP_WRITE_BIT);

		uint16_t* ib = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
		
		offset = 0;
		for(i = 0; i < VEC_LEN(&mdi->meshes); i++) {
			MDIDrawInfo* di = VEC_ITEM(&mdi->meshes, i);
			memcpy(ib + offset, di->indices, di->indexCount * mdi->indexSize);
			offset += di->indexCount;
		}
		
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	
	glexit(__FILE__);
}



// returns the index if the mesh
int MultiDrawIndirect_addMesh(MultiDrawIndirect* mdi, MDIDrawInfo* di) {
	int index;
	
	VEC_PUSH(&mdi->meshes, di);
	mdi->totalVertices += di->vertexCount;
	mdi->totalIndices += di->indexCount;
	index = VEC_LEN(&mdi->meshes);
	
	return index - 1;
}











static void preFrame(PassFrameParams* pfp, MultiDrawIndirect* mdi) {
	int index_offset = 0;
	int vertex_offset = 0;
	int instance_offset = 0;
	int mesh_index;
	void* vmem = PCBuffer_beginWrite(&mdi->instVB);
	
	if(!vmem) {
		printf("attempted to update invalid MDI\n");
		return;
	}

	// BUG: bounds checking on pcbuffer inside instanceSetup()
	if(mdi->instanceSetup) {
		(*mdi->instanceSetup)(mdi->data, vmem, VEC_DATA(&mdi->meshes), VEC_LEN(&mdi->meshes), pfp);
	}
	
	// BUG: bounds checking on pcbuffer
	
	// set up the indirect draw commands
	if(mdi->isIndexed) {
		
		DrawElementsIndirectCommand* cmdsi = PCBuffer_beginWrite(&mdi->indirectCmds);
		
		for(mesh_index = 0; mesh_index < VEC_LEN(&mdi->meshes); mesh_index++) {
			MDIDrawInfo* di = VEC_ITEM(&mdi->meshes, mesh_index);
				
			cmdsi[mesh_index].firstIndex = index_offset; // offset of this mesh into the instances
			cmdsi[mesh_index].count = di->indexCount; // number of polys
			
			// offset into instanced vertex attributes
			cmdsi[mesh_index].baseInstance = (mdi->maxInstances * ((mdi->instVB.nextRegion) % PC_BUFFER_DEPTH)) + instance_offset; 
			// number of instances
			cmdsi[mesh_index].instanceCount = di->numToDraw; 
			cmdsi[mesh_index].baseVertex = vertex_offset;
			
			index_offset += di->indexCount;
			vertex_offset += di->vertexCount;
			instance_offset += di->numToDraw;
		}
		
	}
	else {
		
		DrawArraysIndirectCommand* cmds = PCBuffer_beginWrite(&mdi->indirectCmds);
		
		for(mesh_index = 0; mesh_index < VEC_LEN(&mdi->meshes); mesh_index++) {
			MDIDrawInfo* di = VEC_ITEM(&mdi->meshes, mesh_index);
				
			cmds[mesh_index].first = vertex_offset; // offset of this mesh into the instances
			cmds[mesh_index].count = di->vertexCount; // number of polys
			
			// offset into instanced vertex attributes
			cmds[mesh_index].baseInstance = (mdi->maxInstances * ((mdi->instVB.nextRegion) % PC_BUFFER_DEPTH)) + instance_offset; 
			// number of instances
			cmds[mesh_index].instanceCount = di->numToDraw; 
			
			instance_offset += di->numToDraw;
			vertex_offset += di->vertexCount;
		}
	}
	
}



static void core_draw(MultiDrawIndirect* mdi);



// this one has to handle different views, such as shadow mapping and reflections
static void draw(MultiDrawIndirect* mdi, GLuint progID, PassDrawParams* pdp) {
		
	if(mdi->uniformSetup) {
		(*mdi->uniformSetup)(mdi->data, progID);
	}

	core_draw(mdi);
}

static void core_draw(MultiDrawIndirect* mdi) {
	size_t cmdOffset;
	
	glBindVertexArray(mdi->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mdi->geomVBO);
	
	if(mdi->isIndexed)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdi->geomIBO);
	
	PCBuffer_bind(&mdi->instVB);
	PCBuffer_bind(&mdi->indirectCmds);
	
	cmdOffset = PCBuffer_getOffset(&mdi->indirectCmds);
	
	if(mdi->isIndexed) {
		glMultiDrawElementsIndirect(mdi->primMode, GL_UNSIGNED_SHORT, (void*)cmdOffset, VEC_LEN(&mdi->meshes), 0);
	}
	else {
		glMultiDrawArraysIndirect(mdi->primMode, (void*)cmdOffset, VEC_LEN(&mdi->meshes), 0);
	}
	glexit("multidrawarraysindirect");
}


static void postFrame(MultiDrawIndirect* mdi) {
	PCBuffer_afterDraw(&mdi->instVB);
	PCBuffer_afterDraw(&mdi->indirectCmds);
}









RenderPass* MultiDrawIndirect_CreateRenderPass(MultiDrawIndirect* m, ShaderProgram* prog) {
	
	RenderPass* rp;
	PassDrawable* pd;

	pd = MultiDrawIndirect_CreateDrawable(m, prog);

	rp = calloc(1, sizeof(*rp));
	RenderPass_init(rp);
	RenderPass_addDrawable(rp, pd);
	//rp->fboIndex = LIGHTING;
	
	return rp;
}


PassDrawable* MultiDrawIndirect_CreateDrawable(MultiDrawIndirect* m, ShaderProgram* prog) {
	PassDrawable* pd;

	pd = Pass_allocDrawable("MDI");
	pd->data = m;
	pd->preFrame = (void*)preFrame;
	pd->draw = (PassDrawFn)draw;
	pd->postFrame = (void*)postFrame;
	pd->prog = prog;
	
	return pd;
}


