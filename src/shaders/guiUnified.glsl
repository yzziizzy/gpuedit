#shader VERTEX


#version 430 core

layout (location = 0) in vec4 lt_rb_in;
layout (location = 1) in vec4 clip_in;
layout (location = 2) in ivec4 tex_type_in;
layout (location = 3) in vec4 tex_off_in;
layout (location = 4) in vec4 tex_size_in;

layout (location = 5) in vec4 fg_color_in;
layout (location = 6) in vec4 bg_color_in;

uniform ivec2 targetSize;


out Vertex {
	vec4 lt_rb;
	vec4 clip;
	vec2 wh;
	float opacity;
	int guiType;
	vec4 fg_color;
	vec4 bg_color;
	vec2 texOffset1;
	vec2 texSize1;
	int texIndex1;
	
} vertex;

vec4 toNDC(vec4 positiveNorm) {
	return (positiveNorm * 2) - 1;
}


void main() {
	
	// convert to NDC
 	vertex.lt_rb = toNDC(lt_rb_in / vec4(targetSize.xy, targetSize.xy));
//	vertex.lt_rb = vec4(.5, .5, -.5, -.5);
	
	// convert to clip space
	vertex.clip = clip_in;
	vertex.wh = vec2(abs(lt_rb_in.x - lt_rb_in.z), abs(lt_rb_in.y - lt_rb_in.w)) / 1000;
	vertex.opacity = .7; 
	
	vertex.guiType = tex_type_in.w;
	
	vertex.fg_color = fg_color_in;
	vertex.bg_color = bg_color_in;
	
	vertex.texOffset1 = tex_off_in.xy;
	vertex.texSize1 = tex_size_in.xy;
	vertex.texIndex1 = int(tex_type_in.x);
}






#shader GEOMETRY

#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in Vertex {
	vec4 lt_rb;
	vec4 clip;
	vec2 wh;
	float opacity;
	int guiType;
	vec4 fg_color;
	vec4 bg_color;
	vec2 texOffset1;
	vec2 texSize1;
	int texIndex1;
	
} vertex[];


out vec3 gs_tex;
flat out float gs_opacity;
flat out vec4 gs_clip; 
flat out vec4 gs_fg_color; 
flat out vec4 gs_bg_color; 
flat out int gs_guiType;



void main() {
	//if(vertex[0].opacity == 0.0) return;
	
	
	gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
	gs_opacity = vertex[0].opacity;
	gs_clip = vertex[0].clip;
	gs_guiType = vertex[0].guiType;
	gs_fg_color = vertex[0].fg_color;
	gs_bg_color = vertex[0].bg_color;
	gl_Position = vec4(vertex[0].lt_rb.x, -vertex[0].lt_rb.y, 0, 1);
	EmitVertex();

	
	gs_tex = vec3(vertex[0].texOffset1.x + vertex[0].texSize1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
	gs_opacity = vertex[0].opacity;
	gs_clip = vertex[0].clip;
	gs_guiType = vertex[0].guiType;
	gs_fg_color = vertex[0].fg_color;
	gs_bg_color = vertex[0].bg_color;
	gl_Position = vec4(vertex[0].lt_rb.z, -vertex[0].lt_rb.y, 0, 1);
	EmitVertex();
	
	gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y + vertex[0].texSize1.y, vertex[0].texIndex1);
	gs_opacity = vertex[0].opacity;
	gs_clip = vertex[0].clip;
	gs_guiType = vertex[0].guiType;
	gs_fg_color = vertex[0].fg_color;
	gs_bg_color = vertex[0].bg_color;
	gl_Position = vec4(vertex[0].lt_rb.x, -vertex[0].lt_rb.w, 0, 1);
	EmitVertex();

	gs_tex = vec3(vertex[0].texOffset1 + vertex[0].texSize1, vertex[0].texIndex1);
	gs_opacity = vertex[0].opacity;
	gs_clip = vertex[0].clip;
	gs_guiType = vertex[0].guiType;
	gs_fg_color = vertex[0].fg_color;
	gs_bg_color = vertex[0].bg_color;
	gl_Position = vec4(vertex[0].lt_rb.z, -vertex[0].lt_rb.w, 0, 1);
	EmitVertex();

	
	EndPrimitive(); 
}



#shader FRAGMENT

#version 430


layout(location = 0) out vec4 out_Color;

in vec3 gs_tex;
flat in float gs_opacity;
flat in vec4 gs_clip; 
flat in vec4 gs_fg_color; 
flat in vec4 gs_bg_color; 
flat in int gs_guiType; 


uniform sampler2DArray fontTex;
uniform sampler2DArray atlasTex;


void main(void) {
	
	// clipping
	if(gl_FragCoord.x < gs_clip.x || gl_FragCoord.x > gs_clip.z
		|| gl_FragCoord.y < gs_clip.y || gl_FragCoord.y > gs_clip.w) {
		
		out_Color = vec4(1,.1,.1,.4);
		return;
		
	//	discard;
	}
	
	//out_Color = vec4(1,.1,.1, 1);
	//return;
	
// 	out_Color = texture(textures, vec3(gs_tex.xy, 0)) * vec4(1.0, 1.0, 1.0, gs_opacity); //vs_norm;

	/*
		// fade the edges
	float ei1 = smoothstep(0.0, borderWidth, tc.x);
	float ei2 = 1.0 - smoothstep(1.0 - borderWidth, 1.0, tc.x);
	float ei3 = smoothstep(0.0, borderWidth, tc.y);
	float ei4 = 1.0 - smoothstep(1.0 - borderWidth, 1.0, tc.y);
	
	vec4 edgeFactor = vec4(min(min(ei1, ei2), min(ei3, ei4)), 0,0,1).rrra;
	
	float bi1 = smoothstep(0.0, fadeWidth, tc.x);
	float bi2 = 1.0 - smoothstep(1.0 -fadeWidth, 1.0, tc.x);
	float bi3 = smoothstep(0.0, fadeWidth, tc.y);
	float bi4 = 1.0 - smoothstep(1.0 - fadeWidth, 1.0, tc.y);
	
	vec4 borderFactor = vec4(min(min(bi1, bi2), min(bi3, bi4)), 0,0,1).rrra;
	vec3 bc = borderColor.rgb; 
	float ba = borderColor.a;
	
	FragColor = vec4(mix(bc * ba, color, borderFactor.r), alpha * edgeFactor);
	
	*/
	
	
	
	if(gs_guiType == 0) { // just a rectangle
// 		out_Color = gs_fg_color;
		out_Color = gs_bg_color;
		
		
		return;
	}
	else if(gs_guiType == 1) { // text
		
		float dd;
		float d = dd = texture(fontTex, gs_tex).r;
/*		
		out_Color = vec4(d,d,d, 1.0); 
		return;
		*/
		float a;
		
		
		if(d > .75) {
			d = 1;// (d - .75) * -4;
		}
		else {
			d = (d / 3) * 4;
		}
		d = 1 - d;

		a = smoothstep(0.35, 0.9, abs(d));
// 		a = step(0.65, abs(d));
		
		if(a < 0.01) {
// 			out_Color = vec4(gs_tex.xy, 0, 1);
			//return; // show the overdraw
			discard;
		};
		
		//if(dd < .35) discard;
// 		out_Color = vec4(gs_fg_color.rgb, a); 
		out_Color = vec4(.9,.9,.9, a); 
		return;
	}
	else if(gs_guiType == 2) { // simple image
		out_Color = texture(atlasTex, gs_tex);
		return;
	}
	
	
	
	out_Color = vec4(1,.1,.1, .4);
	
	
	
	
	
}
