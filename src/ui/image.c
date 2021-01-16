
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "gui.h"
#include "gui_internal.h"





HT(int) image_names;
TexArray* image_textures;

GLuint vaoImage, vboImage;

ShaderProgram* imageProg;
ShaderProgram* rtProg;






	


/*




void gui_Image_Init(char* file) {
	
	char** paths; 
	int len;
	
	void* iter;
	char* texName;
	json_value_t* j_texPath;
	json_file_t* jsf;
	
	HT_init(&image_names, 4);
	
	jsf = json_load_path(file);
	
	len = json_obj_length(jsf->root);
	paths = malloc(sizeof(*paths) * len + 1);
	paths[len] = 0;
	
	iter = NULL; 
	int i = 0;
	while(json_obj_next(jsf->root, &iter, &texName, &j_texPath)) {
		
		char* name = strdup(texName);
		json_as_string(j_texPath, paths + i);
		
		HT_set(&image_names, name, i);
		
		i++;
	}
	
	// 128x128 is hardcoded for now
	image_textures = loadTexArray(paths);
	
	for(i = 0; i < len; i++) free(paths[i]);
	free(paths);
	
	
	// BUG: double free error somewhere in here
	//json_free(jsf->root);
	//free(jsf);
	
	
	
	// gl stuff
	imageProg = loadCombinedProgram("guiImage");
	rtProg = loadCombinedProgram("guiRenderTarget");
	
	
	// image VAO
	VAOConfig opts[] = {
		// per vertex
		{0, 2, GL_FLOAT, 0, GL_FALSE}, // position/tex coords
		
		{0, 0}
	};
	
	vaoImage = makeVAO(opts);
	
	
	glBindVertexArray(vaoImage);
	
	glGenBuffers(1, &vboImage);
	glBindBuffer(GL_ARRAY_BUFFER, vboImage);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*4, 0);
	
	Vector2 data[] = {
		{0,0},
		{0,1},
		{1,0},
		{1,1}
	};
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glexit("");
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glexit("");
	
	
	
}*/




/*
void updarePos(GUIImage* go, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	GUIHeader* h = &go->header;
		
	Vector2 tl = gui_calcPosGrav(h, grp);
	h->absTopLeft = tl;
	h->absClip = grp->clip;
	h->absZ = grp->baseZ + h->z;
	
	// TODO: relTopLeft, absClip
	
	GUIRenderParams grp2 = {
		.size = h->size,
		.offset = tl,
		.clip = h->absClip,
		.baseZ = h->absZ,
	};
	
	VEC_EACH(&h->children, ind, child) {
		GUIHeader_updatePos(child, &grp2, pfp);
	}
	
}
*/


static void render(GUIImage* im, PassFrameParams* pfp) {
	
	//just a clipped box
	
	Vector2 tl = im->header.absTopLeft; //gui_calcPosGrav(&im->header, grp);
	
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(im->header.gm, 1);
	if(!im->texHandle) {
		*v = (GUIUnifiedVertex){
			
			.pos.t = tl.y,
			.pos.l = tl.x,
			.pos.b = tl.y + im->header.size.y,
			.pos.r = tl.x + im->header.size.x,
			
			.clip.l = im->header.absClip.min.x,
			.clip.t = im->header.absClip.min.y,
			.clip.r = im->header.absClip.max.x,
			.clip.b = im->header.absClip.max.y,
			
			.texIndex1 = im->texIndex,
			.texIndex2 = 0,
			.texFade = .5,
			
			.guiType = 2, // simple image
			
			.texOffset1 = { im->offsetNorm.x * 65535, im->offsetNorm.y * 65535 },
	// 		.texOffset1 = { .1 * 65535, .1 * 65535 },
			.texOffset2 = 0,
			.texSize1 = { im->sizeNorm.x * 65535, im->sizeNorm.y * 65535 },
	// 		.texSize1 = { .5 * 65535, .5 * 65535 },
			.texSize2 = 0,
			
			.fg = {255, 128, 64, 255},
			.bg = {64, 128, 255, 255},
			
		};
	}
	else {
		*v = (GUIUnifiedVertex){
			
			.pos.t = tl.y,
			.pos.l = tl.x,
			.pos.b = tl.y + im->header.size.y,
			.pos.r = tl.x + im->header.size.x,
			
			.clip.l = im->header.absClip.min.x,
			.clip.t = im->header.absClip.min.y,
			.clip.r = im->header.absClip.max.x,
			.clip.b = im->header.absClip.max.y,
			
			.texIndex1 = VEC_LEN(&im->header.gm->texHandles),
			.texIndex2 = 0,
			.texFade = .5,
			
			.guiType = im->texHandle ? 3 : 2, // bindless texture
			
			.texOffset1 = { 0, 0 },
	// 		.texOffset1 = { .1 * 65535, .1 * 65535 },
			.texOffset2 = 0,
			.texSize1 = { 65535, 65535 },
	// 		.texSize1 = { .5 * 65535, .5 * 65535 },
			.texSize2 = 0,
			
			.fg = {255, 128, 64, 255},
			.bg = {64, 128, 255, 255},
			
		};
		
		VEC_PUSH(&im->header.gm->texHandles, im->texHandle);
	}
//	printf("%p\n", im->texHandle);
// 	
	
}



GUIImage* GUIImage_new(GUIManager* gm, char* name) {

	
	float tbh = .03; // titleBarHeight
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
	};
	
	
	GUIImage* im;
	pcalloc(im);
	
	gui_headerInit(&im->header, gm, &static_vt, NULL);
	
// 	im->header.hitbox.min.x = pos.x;
// 	im->header.hitbox.min.y = pos.y;
// 	im->header.hitbox.max.x = pos.x + size.x;
// 	im->header.hitbox.max.y = pos.y + size.y;
	
	if(name) {
		TextureAtlasItem* it;
		if(HT_get(&gm->ta->items, name, &it)) {
			printf("could not find gui image '%s'\n", name);
		}
		else {
			im->offsetNorm = it->offsetNorm;
			im->sizeNorm = it->sizeNorm;
			im->texIndex = it->index;
			
			im->header.size.x = it->sizePx.x;
			im->header.size.y = it->sizePx.y;
		}
		printf("text index: %d\n", it->index);
	}
	
	im->customTexID = 0;
	
	return im;
}








static void rt_render(GUIRenderTarget* im, PassFrameParams* pfp) {
	
	//just a clipped box
	
	Vector2 tl = im->header.absTopLeft; //gui_calcPosGrav(&im->header, grp);
	
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(im->header.gm, 1);

	*v = (GUIUnifiedVertex){
		
		.pos.t = tl.y,
		.pos.l = tl.x,
		.pos.b = tl.y + im->header.size.y,
		.pos.r = tl.x + im->header.size.x,
		
		.clip.l = im->header.absClip.min.x,
		.clip.t = im->header.absClip.min.y,
		.clip.r = im->header.absClip.max.x,
		.clip.b = im->header.absClip.max.y,
		
		.texIndex1 = VEC_LEN(&im->header.gm->texHandles),
		.texIndex2 = 0,
		.texFade = .5,
		
		.guiType = im->texHandle ? 4 : 0, // bindless texture, upside-down
		
		.texOffset1 = { 0, 0 },
// 		.texOffset1 = { .1 * 65535, .1 * 65535 },
		.texOffset2 = 0,
		.texSize1 = { 65535, 65535 },
// 		.texSize1 = { .5 * 65535, .5 * 65535 },
		.texSize2 = 0,
		
		.fg = {255, 128, 64, 255},
		.bg = {64, 128, 255, 255},
		
	};
	
	VEC_PUSH(&im->header.gm->texHandles, im->texHandle);
}




void guiRenderTargetDelete(GUIRenderTarget* rt) {
	RenderPipeline_destroy(rt->rpl);
	free(rt->rpl);
}

void guiRenderTargetResize(GUIRenderTarget* rt, Vector2 newSz) {
	
// 	RenderPipeline_rebuildFBOs(rt->rpl, (Vector2i){newSz.x, newSz.y});
	
	printf("hack. need to get real pizel size here\n");
	
	RenderPipeline_rebuildFBOs(rt->rpl, (Vector2i){
		rt->header.size.x/* * rt->screenRes.x*/, 
		rt->header.size.y/* * rt->screenRes.y*/
	});
}








GUIRenderTarget* GUIRenderTarget_new(GUIManager* gm, Vector2 pos, Vector2 size, RenderPipeline* rpl) {
	
	GUIRenderTarget* im;
	
	float tbh = .03; // titleBarHeight
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)rt_render,
		.Delete = (void*)guiRenderTargetDelete,
		.Resize = (void*)guiRenderTargetResize
	};
	
	
	im = calloc(1, sizeof(*im));
	CHECK_OOM(im);
	
 	gui_headerInit(&im->header, gm, &static_vt, NULL);
	
	im->header.topleft = pos;
	im->header.size = size;
	im->header.z = 0;
	
	im->texHandle = 0;
	im->rpl = rpl;
	
	// HACK. meh. just put in something so it renders at all in the beginning
	im->screenRes.x = 600;
	im->screenRes.y = 600;
	
	return im;
}


void guiRenderTarget_SetScreenRes(GUIRenderTarget* rt, Vector2i newRes) {
	rt->screenRes = newRes;
	printf("screen resized: %d, %d\n", newRes.x, newRes.y);
	RenderPipeline_rebuildFBOs(rt->rpl, (Vector2i){
		rt->header.size.x * rt->screenRes.x, 
		rt->header.size.y * rt->screenRes.y
	});
	
}

