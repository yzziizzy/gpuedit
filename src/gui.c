

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "window.h"
#include "app.h"
#include "texture.h"
#include "hash.h"

#include "gui.h"
#include "gui_internal.h"

#include "utilities.h"

// // FontConfig
// #include "text/fcfg.h"

// for sdf debugging
#include "dumpImage.h"


// VEC(GUIObject*) gui_list; 
// VEC(GUIObject*) gui_reap_queue; 


GUIObject* guiBaseHitTest(GUIObject* go, Vector2 absTestPos);


static void preFrame(PassFrameParams* pfp, GUIManager* gm);
static void draw(GUIManager* gm, GLuint progID, PassDrawParams* pdp);
static void postFrame(GUIManager* gm);



GUIManager* GUIManager_alloc(GlobalSettings* gs) {
	GUIManager* gm;
	pcalloc(gm);
	
	GUIManager_init(gm, gs);
	
	return gm;
}



static void updatePosRoot(GUIHeader* gh, GUIRenderParams* always_null, PassFrameParams* pfp) {
	GUIRenderParams grp = {
		.size = gh->size,
		.offset = {0,0},
		.clip = {{0,0}, gh->size},
		.baseZ = 0,
	};
	
	VEC_EACH(&gh->children, ind, child) {
		GUIHeader_updatePos(child, &grp, pfp);
	}
}

static void renderRoot(GUIHeader* gh, PassFrameParams* pfp) {
	GUIHeader_renderChildren(gh, pfp);
}

// static GUIObject* hitTestRoot(GUIObject* go, Vector2 absTestPos) {
// 	GUIHeader* gh = &go->header;
// 	float highestZ = 0;
// 	int i;
// 	for(i = 0; i < VEC_LEN(&gh->children); i++) {
// 		GUIObject* kid = GUIObject_hitTest(VEC_ITEM(&gh->children, i), testPos);
// 		if(kid) {
// 			printf("hit: %p\n", kid);
// 			return kid;
// 		}
// 	}
// 	
// 	// the root window does not respond to hits itself
// 	return NULL;
// }

// _init is always called before _initGL
void GUIManager_init(GUIManager* gm, GlobalSettings* gs) {
	
	static struct gui_vtbl root_vt = {
		.UpdatePos = updatePosRoot,
		.Render = renderRoot,
// 		.HitTest = hitTestRoot,
	};
	
	VEC_INIT(&gm->reapQueue);
	
	gm->maxInstances = gs->GUIManager_maxInstances;
	
	gm->elementCount = 0;
	gm->elementAlloc = 64;
	gm->elemBuffer = calloc(1, sizeof(*gm->elemBuffer) * gm->elementAlloc);
	
	gm->fm = FontManager_alloc(gs);
	
	gm->ta = TextureAtlas_alloc(gs);
	gm->ta->width = 256;
	TextureAtlas_addFolder(gm->ta, "pre", "assets/ui/icons", 0);
	TextureAtlas_finalize(gm->ta);
	
	
	gm->root = calloc(1, sizeof(GUIHeader));
	gui_headerInit(gm->root, NULL, &root_vt); // TODO: vtable?
}


void GUIManager_initGL(GUIManager* gm, GlobalSettings* gs) {
	static VAOConfig vaoConfig[] = {
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // top, left, bottom, right
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // tlbr clipping planes
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_FALSE}, // tex indices 1&2, tex fade, gui type
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex offset 1&2
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex size 1&2
		
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // fg color
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // bg color
		
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // z-index, alpha, opts 1-2
		
		{0, 0, 0}
	};

	
	gm->vao = makeVAO(vaoConfig);
	glBindVertexArray(gm->vao);
	
	int stride = calcVAOStride(0, vaoConfig);

	PCBuffer_startInit(&gm->instVB, gm->maxInstances * stride, GL_ARRAY_BUFFER);
	updateVAO(0, vaoConfig); 
	PCBuffer_finishInit(&gm->instVB);
	
	///////////////////////////////
	// font texture
	
	glGenTextures(1, &gm->fontAtlasID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->fontAtlasID);
	glerr("bind font tex");
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0); // no mipmaps for this; it'll get fucked up
	glerr("param font tex");
	
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, gm->fm->atlasSize, gm->fm->atlasSize, VEC_LEN(&gm->fm->atlas));
	
	VEC_EACH(&gm->fm->atlas, ind, at) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 
			0, 0, ind, // offsets
			gm->fm->atlasSize, gm->fm->atlasSize, 1, 
			GL_RED, GL_UNSIGNED_BYTE, at);
		glerr("load font tex");
	}
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	
	
	///////////////////////////////
	// regular texture atlas
	
	glGenTextures(1, &gm->atlasID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->atlasID);
	glerr("bind font tex");
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0); // no mipmaps for this; it'll get fucked up
	glerr("param font tex");
	
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, gm->ta->width, gm->ta->width, VEC_LEN(&gm->ta->atlas));
	
	char buf [50];
	
	VEC_EACH(&gm->ta->atlas, ind, at2) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 
			0, 0, ind, // offsets
			gm->ta->width, gm->ta->width, 1, 
			GL_RGBA, GL_UNSIGNED_BYTE, at2);
		glerr("load font tex");	
	}
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	
	//////////////////////////////////
	
}





static void writeCharacterGeom(GUIUnifiedVertex* v, struct charInfo* ci, float sz, float adv, float line) {
	
	float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
	float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
	float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
	float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
	
	v->pos.t = 200 + line - ci->topLeftOffset.y * sz;
	v->pos.l= 200 + adv + ci->topLeftOffset.x * sz;
	v->pos.b = 200 + line + ci->size.y * sz - ci->topLeftOffset.y * sz;
	v->pos.r = 200 + adv + ci->size.x * sz + ci->topLeftOffset.x * sz;
	
	v->guiType = 1; // text
	
	v->texOffset1.x = offx * 65535.0;
	v->texOffset1.y = offy * 65535.0;
	v->texSize1.x = widx *  65535.0;
	v->texSize1.y = widy * 65535.0;
	v->texIndex1 = ci->texIndex;
}





// returns 0 if the line wraps, 1 if the text ended before reaching the end of the line 
int scanTextLine2(GUIFont* f, char* txt, float maxWidth,
	int* _numSpaces, 
	int* _numInternalWordBreaks, 
	int* _firstChar,
	int* _lastChar,
	float* _safeLen
) {
	int i;
	int eot = 0;
	int firstChar = 0;
	float adv = 0.0;
	
	// consume leading whitespace
	for(i = 0; txt[i] != 0; i++) {
		char c = txt[i];
		if(c != ' ' && c != '\t') break;
	}
	
	// check for end of text
	if(txt[i] == 0) {
		eot = 1;
		goto EOT;
	}
	
	firstChar = i;
	if(_firstChar) *_firstChar = firstChar;
	
	
	// consume one word at a time
	for(i = firstChar; txt[i] != 0; i++) {
		float wordAdv = 0.0;;
		int lastWordChar;
		
		// look for the end of the next word
		int j;
		for(j = 0; txt[i + j] != 0; j++) {
			char c = txt[i + j];
			
			if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				break;
			}
			
			struct charInfo* ci = &f->regular[c];
			
			wordAdv += ci->advance;
		}
		
		// check for end of text
		// BUG: do something here
		if(txt[i + j] == 0) {
			eot = 1;
		}
		
		
		lastWordChar = i + j - 1;
		
		// check for overflow
		if(adv + wordAdv > maxWidth) {
			
		}
		
		
		// consume trailing whitespace, contracting as desired
		int k;
		for(k = 0; txt[i+j+k] != 0; k++) {
			char c = txt[i+j+k];
			if(c != ' ' && c != '\t') break;
			
		}
		
		// check for end of text
		// BUG: do something
		if(txt[i+j+k] == 0) {
			eot = 1;
		}
		
		
		
		i += j;
	}
	
	// check for end of text
	if(txt[i] == 0) {
		eot = 1;
		goto EOT;
	}
	
	
EOT: // end of text reached early
	
	return 1;
}

void scanTextLine(GUIFont* f, char* txt, float maxWidth, 
	int* numSpaces, 
	int* lastChar,
	float* safeLen
) {
	
	int spacenum = 0;
	float adv = 0.0;
	
	int i;
	

	int hasNonSpace = 0;
	for(i = 0; txt[i] != 0; i++) {
		char c = txt[i];
		struct charInfo* ci = &f->regular[c];
		
		
		adv += ci->advance; // BUG: needs sdfDataSize added in?
		if(isspace(c)) spacenum++;
// 		
// 		if(hasNonSpace) {
			if(adv >= maxWidth) {
				break;
			}
// 			if(c == '\n') break;
// 		}
		
	}
	
	if(adv > maxWidth) {
		int hasSpace = 0;
		while(i >= 0) {
			if(isspace(txt[i])) {
				hasSpace = 1;
				spacenum--;
			}
			else if(hasSpace) {
				break;
			}
			i--;
			adv -= f->regular[txt[i]].advance;
		}
	}
	// i should be the last non-space character before the last space
	
	if(lastChar) *lastChar = i;
	if(safeLen) *safeLen = adv;
	if(numSpaces) *numSpaces = spacenum;
}


GUITextArea_draw(GUIManager* gm, GUIFont* f) {
	
	int n = gm->elementCount;
	GUIUnifiedVertex* v;// = gm->elemBuffer + n;
	
	char* txt = "( j | l )When in the Course of human events, it becomes necessary for one people to" \
	" dissolve the political bands which have connected them with another, and to assume among" \
	" the powers of the earth, the separate and equal station to which the Laws of Nature and" \
	" of Nature's God entitle them, a decent respect to the opinions of mankind requires that" \
	" they should declare the causes which impel them to the separation.\n" \
	" We hold these truths to be self-evident, that all men are created equal, that they are" \
	" endowed by their Creator with certain unalienable Rights, that among these are Life," \
	" Liberty and the pursuit of Happiness.\n" \
	" That to secure these rights, Governments are instituted among Men, deriving their just" \
	" powers from the consent of the governed, That whenever any Form of Government becomes" \
	" destructive of these ends, it is the Right of the People to alter or to abolish it, and" \
	" to institute new Government, laying its foundation on such principles and organizing its" \
	" powers in such form, as to them shall seem most likely to effect their Safety and" \
	" Happiness. Prudence, indeed, will dictate that Governments long established should not" \
	" be changed for light and transient causes; and accordingly all experience hath shewn," \
	" that mankind are more disposed to suffer, while evils are sufferable, than to right" \
	" themselves by abolishing the forms to which they are accustomed. But when a long train" \
	" of abuses and usurpations, pursuing invariably the same Object evinces a design to reduce" \
	" them under absolute Despotism, it is their right, it is their duty, to throw off such" \
	" Government, and to provide new Guards for their future security.";
	
	
	
	
	int len = strlen(txt);
	
	float adv = 0.0;
	float size = 1;
	
	float maxw = 650;
	float lineh = 21;
	float line = 0;
	
	
	int line_num = 0;
	for(int i = 0; i < len; i++) {
		int numSpaces; 
		int lastChar;
		float safeLen;
		
		line_num++;
		
		scanTextLine(f, txt + i, maxw, &numSpaces, &lastChar, &safeLen);
	//	printf("%d> ns:%d, lc:%d, slen:%f\n", line_num, numSpaces, lastChar, safeLen);
		
		// broken
		float spaceadv = f->regular[' '].advance;
		//spaceadv = (maxw - safeLen) / (numSpaces - 1);
		adv = maxw - safeLen - spaceadv;
		
		int onlySpace = 1;
		for(int n = 0; n <= lastChar; n++) {
			char c = txt[i + n];
			// skip leading space on a new line
			if(!isspace(c)) onlySpace = 0; 
			if(onlySpace) {
				continue;
			}
			
			
			struct charInfo* ci = &f->regular[c];
			
			
			if(c != ' ') {
				v = GUIManager_checkElemBuffer(gm, 1);
				writeCharacterGeom(v, ci, size, adv, line);
				adv += ci->advance * size; // BUG: needs sdfDataSize added in?
				v->fg = (struct Color4){255, 128, 64, 255},
				//v++;
				gm->elementCount++;
			}
			else {
				adv += spaceadv;
			}
			
			
			
		}
		
		i += lastChar;
		
		adv = 0;
		line += lineh;
	}
	
	
	//exit(1);
}




static void preFrame(PassFrameParams* pfp, GUIManager* gm) {
	GUIUnifiedVertex* vmem = PCBuffer_beginWrite(&gm->instVB);
	if(!vmem) {
		printf("attempted to update invalid PCBuffer in GUIManager\n");
		return;
	}
	
	
	gm->elementCount = 0;
	
	
	GUIRenderParams grp = {
		.offset = {0,0}, 
		.size = {800,800},
		.clip = {(0,0),{800,800}},
	};
	gm->root->h.size = (Vector2){800, 800};
	
	GUIHeader_updatePos(gm->root, &grp, pfp);
	
	GUIHeader_render(gm->root, pfp);
	
	
	
	
	GUIFont* f = gm->fm->helv;
	
// 	GUITextArea_draw(gm, f);
// 	
// 	printf("\n elementCount: %d \n\n", gm->elementCount);
// 	memcpy(vmem, gm->elemBuffer, gm->elementCount * sizeof(*gm->elemBuffer));
// 		
// 	

	//just a clipped box
// 	*vmem = (GUIUnifiedVertex){
// 		.pos = {200, 200, 900, 200+650},
// 		.clip = {150, 110, 800, 600},
// 		
// 		.texIndex1 = 0,
// 		.texIndex2 = 0,
// 		.texFade = .5,
// 		.guiType = 0, // window
// 		
// 		.texOffset1 = 0,
// 		.texOffset2 = 0,
// 		.texSize1 = 0,
// 		.texSize2 = 0,
// 		
// 		.fg = {255, 128, 64, 255},
// 		.bg = {64, 128, 255, 255},
// 	};
// 	
// 	
// 	vmem++;
//	GUITextArea_draw(gm, f);

	memcpy(vmem, gm->elemBuffer, gm->elementCount * sizeof(*gm->elemBuffer));

	
	
	/*
	float off = gm->fm->helv->regular['I'].texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
	float offy = gm->fm->helv->regular['I'].texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
	float wid = gm->fm->helv->regular['I'].texNormSize.x;//TextRes_charWidth(gm->font, 'A');
	float widy = gm->fm->helv->regular['I'].texNormSize.y;//TextRes_charWidth(gm->font, 'A');
	*vmem = (GUIUnifiedVertex){
		.pos = {200, 200, 
			200 + gm->fm->helv->regular['I'].size.y * 5,
			200 + gm->fm->helv->regular['I'].size.x * 5
			
		},
		.clip = {150, 110, 800, 600},
		
		.texIndex1 = 0,
		.texIndex2 = 0,
		.texFade = .5,
		.guiType = 1, // text
		
// 		.texOffset1 = { off * 65535.0, 0},
		.texOffset1 = { off * 65535.0, offy * 65535.0},
		.texOffset2 = {0, 0},
// 		.texSize1 = { wid * 65535.0, 65535},
		.texSize1 = { wid *  65535.0, widy * 65535.0},
		.texSize2 = {5000, 5000},
		
		.fg = {255, 128, 64, 255},
		.bg = {64, 128, 255, 255},
	};
	
	*/
	
	//printf("FONT INFO: %f, %f\n", off, wid);
	
}

static void draw(GUIManager* gm, GLuint progID, PassDrawParams* pdp) {
	size_t offset;
	

// 	if(mdi->uniformSetup) {
// 		(*mdi->uniformSetup)(mdi->data, progID);
// 	}
	GLuint ts_ul = glGetUniformLocation(progID, "fontTex");
	GLuint ta_ul = glGetUniformLocation(progID, "atlasTex");
	
	glActiveTexture(GL_TEXTURE0 + 29);
	glUniform1i(ts_ul, 29);
	glexit("text sampler uniform");
 	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->fontAtlasID);
//  	glBindTexture(GL_TEXTURE_2D, gt->font->textureID); // TODO check null ptr
 	glexit("bind texture");



	glActiveTexture(GL_TEXTURE0 + 28);
	glUniform1i(ta_ul, 28);
	glexit("text sampler uniform");
 	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->atlasID);
//  	glBindTexture(GL_TEXTURE_2D, gt->font->textureID); // TODO check null ptr
 	glexit("bind texture");
	
	// ------- draw --------
	
	glBindVertexArray(gm->vao);
	
	PCBuffer_bind(&gm->instVB);
	offset = PCBuffer_getOffset(&gm->instVB);
	
	glDrawArrays(GL_POINTS, offset / sizeof(GUIUnifiedVertex), gm->elementCount);
	
	glexit("");
}



static void postFrame(GUIManager* gm) {
	PCBuffer_afterDraw(&gm->instVB);
}






RenderPass* GUIManager_CreateRenderPass(GUIManager* gm) {
	
	RenderPass* rp;
	PassDrawable* pd;

	pd = GUIManager_CreateDrawable(gm);

	rp = calloc(1, sizeof(*rp));
	RenderPass_init(rp);
	RenderPass_addDrawable(rp, pd);
	//rp->fboIndex = LIGHTING;
	
	return rp;
}


PassDrawable* GUIManager_CreateDrawable(GUIManager* gm) {
	
	PassDrawable* pd;
	static ShaderProgram* prog = NULL;
	
	if(!prog) {
		prog = loadCombinedProgram("guiUnified");
		glexit("");
	}
	
	
	pd = Pass_allocDrawable("GUIManager");
	pd->data = gm;
	pd->preFrame = preFrame;
	pd->draw = (PassDrawFn)draw;
	pd->postFrame = postFrame;
	pd->prog = prog;
	
	return pd;;
}





/*
	//arial = LoadFont("Arial", 32, NULL);
	glerr("clearing before text program load");
	
	printf("text prog %d\n", textProg->id);
	unsigned int colors[] = {
		0xFF0000FF, 2,
		0x00FF00FF, 4,
		0x0000FFFF, INT_MAX
	};
	
	//strRI = prepareText(arial, "FPS: --", -1, colors);
	strRI = prepareText(arialsdf, "FPS: --", -1, colors);
	
	
		snprintf(frameCounterBuf, 128, "dtime:  %.2fms", sdtime);
// 		snprintf(frameCounterBuf, 128, "dtime:  %.2fms", gs->perfTimes.draw * 1000);
		
		//printf("--->%s\n", frameCounterBuf);
		
		updateText(strRI, frameCounterBuf, -1, fpsColors);
		*/
	




// add root objects to the root list, record the parent otherwise
void GUIRegisterObject_(GUIHeader* o, GUIHeader* parent) {
	int i;
	
	if(!parent) {
		parent = o->gm->root;
	}
	o->parent = parent;
	i = VEC_FIND(&parent->children, &o);
	if(i < 0) {
		VEC_PUSH(&parent->children, o);
	}
}




void guiDelete(GUIObject* go) {
	go->h.deleted = 1;
	
// 	VEC_PUSH(&gui_reap_queue, go);
	
	for(int i = 0; i < VEC_LEN(&go->h.children); i++) {
		//guiTextRender(VEC_DATA(&gui_list)[i], gs);
		guiDelete(VEC_ITEM(&go->h.children, i));
	}
	

	
	if(go->h.vt->Delete)
		go->h.vt->Delete(go);
} 

void guiReap(GUIObject* go) {
	if(!go->h.deleted) {
// 		Log("Attempting to reap non-deleted GUI Object");
		return;
	}
	
	// remove from parent
	guiRemoveChild(go->h.parent, go);
	
	if(go->h.vt->Reap)
		return go->h.vt->Reap(go);
	
}

void guiResize(GUIHeader* gh, Vector2 newSz) {
	if(gh->deleted) return;
	
	if(gh->vt->Resize) {
		gh->vt->Resize((GUIObject*)gh, newSz);
	}
	else {
		gh->size = newSz;
	}
	
	// parents need to resize their children
} 

// NOT SMT SAFE; NO LOCKS
int guiRemoveChild(GUIObject* parent, GUIObject* child) {
	
	if(!parent || !child) return 0;
	
	int i = VEC_FIND(&parent->h.children, child);
	if(i < 0) return 1;
	
	VEC_RM(&parent->h.children, i);
	
	return 0;
}





void gui_headerInit(GUIHeader* gh, GUIManager* gm, struct gui_vtbl* vt) {
	VEC_INIT(&gh->children);
	gh->gm = gm;
	gh->vt = vt;
	
	gh->scale = 1.0;
	gh->alpha = 1.0;
	gh->z = 0.0;
}




void GUIHeader_updatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp) {
	if(go->h.vt->UpdatePos)
		go->h.vt->UpdatePos(go, grp, pfp);
	else
		gui_defaultUpdatePos(&go->h, grp, pfp);
}

void GUIManager_updatePos(GUIManager* gm, PassFrameParams* pfp) {
	GUIHeader_updatePos(gm->root, NULL, pfp);
}

// grp is data about the parent's positioning. 
// the child must calculate its own position based on the parent's info passed in.
// this info is then passed down to its children
// the default behavior is:
//   position according to gravity
//   no extra clipping
//   add z values together
void gui_defaultUpdatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	GUIHeader* h = &go->h;
	
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



void GUIObject_triggerClick(GUIObject* go, Vector2 testPos) {
	if(go->h.onClick)
		go->h.onClick(go, testPos);
}


GUIObject* GUIManager_triggerClick(GUIManager* gm, Vector2 testPos) {
	GUIObject* go = GUIManager_hitTest(gm, testPos);
	
	if(go) GUIObject_triggerClick(go, testPos);
	
	return go;
}


GUIObject* GUIManager_hitTest(GUIManager* gm, Vector2 testPos) {
	GUIObject* go = GUIObject_hitTest(gm->root, testPos);
	return go == gm->root ? NULL : go;
}


GUIObject* GUIObject_hitTest(GUIObject* go, Vector2 absTestPos) {
	if(go->h.vt->HitTest)
		return go->h.vt->HitTest(go, absTestPos);
	
	return gui_defaultHitTest(&go->h, absTestPos);
}


// default hit testing handler that only operates on the header
GUIObject* gui_defaultHitTest(GUIHeader* h, Vector2 absTestPos) {
	
	if(!(absTestPos.x >= h->absTopLeft.x && 
		absTestPos.y >= h->absTopLeft.y &&
		absTestPos.x <= (h->absTopLeft.x + h->size.x) && 
		absTestPos.y <= (h->absTopLeft.y  + h->size.y))) {
		
		return NULL;
	}
	
	int i;
	GUIObject* bestKid = NULL;
	for(i = 0; i < VEC_LEN(&h->children); i++) {
		GUIObject* kid = GUIObject_hitTest(VEC_ITEM(&h->children, i), absTestPos);
		if(kid) {
			if(!bestKid) {
				bestKid = kid;
			}
			else {
				if(kid->h.absZ > bestKid->h.absZ) bestKid = kid;
			}
		}
	}
	
	return bestKid ? bestKid : (GUIObject*)h;
}




/*
void GUIObject_triggerClick(GUIObject* go, GUIEvent* e) {
	
	if(!go) return;
	e->currentTarget = go;
	
	if(c->h.onClick)
		c->h.onClick(go, e);
	
// 	e->currentTarget = c->h.parent;
	
// 	guiTriggerClick(e);
}*/


// old

void guiSetClientSize(GUIObject* go, Vector2 cSize) {
	if(go->h.vt->SetClientSize)
		return go->h.vt->SetClientSize(go, cSize);
}

Vector2 guiGetClientSize(GUIObject* go) {
	if(go->h.vt->GetClientSize)
		return go->h.vt->GetClientSize(go);
	
	return (Vector2){-1,-1};
} 

Vector2 guiRecalcClientSize(GUIObject* go) {
	if(go->h.vt->RecalcClientSize)
		return go->h.vt->RecalcClientSize(go);
	
	return (Vector2){-1,-1};
} 


void guiAddClient(GUIObject* parent, GUIObject* child) {
	if(parent->h.vt->AddClient)
		parent->h.vt->AddClient(parent, child);
} 

void guiRemoveClient(GUIObject* parent, GUIObject* child) {
	if(parent->h.vt->RemoveClient)
		parent->h.vt->RemoveClient(parent, child);
} 







///////////// internals //////////////


GUIUnifiedVertex* GUIManager_checkElemBuffer(GUIManager* gm, int count) {
	if(gm->elementAlloc < gm->elementCount + count) {
		gm->elementAlloc = MAX(gm->elementAlloc * 2, gm->elementAlloc + count);
		gm->elemBuffer = realloc(gm->elemBuffer, sizeof(*gm->elemBuffer) * gm->elementAlloc);
	}
	
	return gm->elemBuffer + gm->elementCount;
}

GUIUnifiedVertex* GUIManager_reserveElements(GUIManager* gm, int count) {
	GUIUnifiedVertex* v = GUIManager_checkElemBuffer(gm, count);
	gm->elementCount += count;
	return v;
}


void GUIHeader_render(GUIHeader* gh, PassFrameParams* pfp) {
	if(gh->hidden || gh->deleted) return;
	
	if(gh->vt->Render)
		gh->vt->Render((GUIObject*)gh, pfp);
	else
		GUIHeader_renderChildren(gh, pfp);
} 

void GUIHeader_renderChildren(GUIHeader* gh, PassFrameParams* pfp) {
// 	if(gh->hidden || gh->deleted) return;

	VEC_EACH(&gh->children, i, obj) {
		GUIHeader_render(&obj->h, pfp);
	}
}


// returns absolute top/left offset coordinates for a gui object based on the RP from the parent
Vector2 gui_calcPosGrav(GUIHeader* h, GUIRenderParams* grp) {
	/*
	printf("grav: %d - %f,%f, %f,%f, %f,%f, %f,%f, \n",
		   h->gravity,
			grp->offset.x,
			grp->offset.y,
			h->topleft.x,
			h->topleft.y,
			grp->size.x,
			grp->size.y,
			h->size.x,
			h->size.y
	);
	*/
	switch(h->gravity) {
		default:
		case GUI_GRAV_TOP_LEFT:
			return (Vector2){grp->offset.x + h->topleft.x, grp->offset.y + h->topleft.y}; 
		case GUI_GRAV_CENTER_LEFT:
			return (Vector2){
				grp->offset.x + h->topleft.x, 
				grp->offset.y + h->topleft.y + (grp->size.y / 2) - (h->size.y / 2)
			}; 
		case GUI_GRAV_BOTTOM_LEFT:
			return (Vector2){
				grp->offset.x + h->topleft.x, 
				grp->offset.y + h->topleft.y + (grp->size.y) - (h->size.y)
			};
		case GUI_GRAV_CENTER_BOTTOM:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x / 2) - (h->size.x / 2), 
				grp->offset.y + h->topleft.y + (grp->size.y) - (h->size.y)
			};
		case GUI_GRAV_BOTTOM_RIGHT:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x) - (h->size.x), 
				grp->offset.y + h->topleft.y + (grp->size.y) - (h->size.y)
			};
		case GUI_GRAV_CENTER_RIGHT:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x) - (h->size.x), 
				grp->offset.y + h->topleft.y + (grp->size.y / 2) - (h->size.y / 2)
			};
		case GUI_GRAV_TOP_RIGHT:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x) - (h->size.x), 
				grp->offset.y + h->topleft.y
			};
		case GUI_GRAV_CENTER_TOP:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x / 2) - (h->size.x / 2), 
				grp->offset.y + h->topleft.y
			};
		case GUI_GRAV_CENTER:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x / 2) - (h->size.x / 2), 
				grp->offset.y + h->topleft.y + (grp->size.y / 2) - (h->size.y / 2)
			};
	}
}




// returns child coordinates from parent coordinates
// pt is in parent coordinates
Vector2 gui_parent2ChildGrav(GUIHeader* child, GUIHeader* parent, Vector2 pt) {
	
	// pretend the parent is a root element
	GUIRenderParams grp = {
		.size = parent->size, // parent
		.offset = {0, 0},
	};

	// child's top left in parent coordinates
	Vector2 ctl = gui_calcPosGrav(child, &grp);
	printf("ctl: %f, %f | %f, %f\n", pt.x, pt.y, ctl.x, ctl.y);
	return (Vector2) {
		.x = pt.x - ctl.x,
		.y = pt.y - ctl.y
	};
}








GUIObject* GUIObject_findChild(GUIObject* obj, char* childName) {
	if(!obj) return NULL;
	VEC_EACH(&obj->h.children, i, child) {
		if(child->h.name && 0 == strcmp(child->h.name, childName)) return child;
	}
	return NULL;
}
