#shader VERTEX


#version 330 core

layout (location = 0) in vec4 lt_rb_in;
layout (location = 1) in vec4 clip_in;
layout (location = 2) in ivec4 tex_type_in;
layout (location = 3) in vec4 tex_off_in;
layout (location = 4) in vec4 tex_size_in;

layout (location = 5) in vec4 fg_color_in;
layout (location = 6) in vec4 bg_color_in;

layout (location = 7) in vec4 z_a_rot_in;

uniform ivec2 targetSize;


out Vertex {
	vec4 lt_rb;
	vec4 lt_rb_abs;
	vec4 clip;
	vec2 wh;
	float opacity;
	int guiType;
	vec4 fg_color;
	vec4 bg_color;
	vec2 texOffset1;
	vec2 texSize1;
	int texIndex1;
	float rot;
	
} vertex;

vec4 toNDC(vec4 positiveNorm) {
	return (positiveNorm * 2) - 1;
}


void main() {
	
	// convert to NDC
	vertex.lt_rb = toNDC(lt_rb_in / vec4(targetSize.xy, targetSize.xy));
	vertex.lt_rb_abs = vec4(
		lt_rb_in.x,
		targetSize.y - lt_rb_in.y,
		lt_rb_in.z,
		targetSize.y - lt_rb_in.w
	);

	// flip y
	vertex.clip = vec4(
		clip_in.x,
		targetSize.y - clip_in.w, // w and y are swapped on purpose
		clip_in.z,
		targetSize.y - clip_in.y
	);
	
	vertex.wh = vec2(abs(lt_rb_in.x - lt_rb_in.z), abs(lt_rb_in.y - lt_rb_in.w));
	vertex.opacity = .7; 
	
	vertex.guiType = tex_type_in.w;
	
	vertex.fg_color = fg_color_in;
	vertex.bg_color = bg_color_in;
	
	vertex.texOffset1 = tex_off_in.xy;
	vertex.texSize1 = tex_size_in.xy;
	vertex.texIndex1 = int(tex_type_in.x);
	
	vertex.rot = z_a_rot_in.z;
}






#shader GEOMETRY

#version 330 core
#extension GL_ARB_shading_language_420pack: require

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform ivec2 targetSize;

in Vertex {
	vec4 lt_rb;
	vec4 lt_rb_abs;
	vec4 clip;
	vec2 wh;
	float opacity;
	int guiType;
	vec4 fg_color;
	vec4 bg_color;
	vec2 texOffset1;
	vec2 texSize1;
	int texIndex1;
	float rot;
} vertex[];


out vec3 gs_tex;
flat out float gs_opacity;
flat out vec4 gs_clip; 
flat out vec4 gs_fg_color; 
flat out vec4 gs_bg_color; 
flat out int  gs_guiType;
flat out vec4 gs_geom;
     out vec3 gs_bary;

#define MISC_COPY_OUTPUT \
	gs_opacity = vertex[0].opacity; \
	gs_clip = vertex[0].clip; \
	gs_guiType = vertex[0].guiType; \
	gs_fg_color = vertex[0].fg_color; \
	gs_bg_color = vertex[0].bg_color;

void main() {
	//if(vertex[0].opacity == 0.0) return;
	
	// triangles
	if(vertex[0].guiType == 6) {
		
		float c = cos(vertex[0].rot);
		float s = sin(vertex[0].rot);
 		mat2 rm = { vec2(c, -s), vec2(s, c)};
		vec2 center = vertex[0].lt_rb.xy * vec2(1, -1);
		
		float th = ((targetSize.y - vertex[0].lt_rb_abs.w)) * (2.0 / 3.0);
		float hw = vertex[0].lt_rb_abs.z;
		vec2 base1 = ((vec2(-hw, -th) * rm) / targetSize) + center;
		vec2 base2 = ((vec2(hw, -th) * rm) / targetSize) + center;
		vec2 top = ((vec2(0, th*2) * rm) / targetSize) + center;
		
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gs_bary = vec3(hw, 0, 0);
		gl_Position = vec4(base1.xy, 0, 1);
		EmitVertex();
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gs_bary = vec3(0, hw, 0);
		gl_Position = vec4(base2.xy, 0, 1);
		EmitVertex();
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gs_bary = vec3(0, 0, th * 2.0); // BUG scaling is not quite right
		gl_Position = vec4(top.xy, 0, 1);
		EmitVertex();
		
		
	}
	if(vertex[0].guiType >= 10 && vertex[0].guiType < 20) { // lines
		
		vec2 l1 = vertex[0].lt_rb_abs.xy * vec2(1, 1);
		vec2 l2 = vertex[0].lt_rb_abs.zw * vec2(1, 1);

		l1 = vec2(l1.x, targetSize.y - l1.y);
		l2 = vec2(l2.x, targetSize.y - l2.y);
		
		vec2 tang = l2 - l1;
		vec2 norm = normalize(vec2(tang.y, -tang.x));
		
		float width = vertex[0].texIndex1;
		
		vec2 p1 = (l1 + (width * norm)) / targetSize;
		vec2 p2 = (l1 + (width * -norm)) / targetSize;
		vec2 p3 = (l2 + (width * norm)) / targetSize;
		vec2 p4 = (l2 + (width * -norm)) / targetSize;
		
		vec2 off = vec2(-1, 1);
		
		p1 = (p1 * vec2(2, -2)) + off;
		p2 = (p2 * vec2(2, -2)) + off;
		p3 = (p3 * vec2(2, -2)) + off;
		p4 = (p4 * vec2(2, -2)) + off;
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(0, 1, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(p1, 0, 1);
		EmitVertex();

		
		MISC_COPY_OUTPUT
		gs_tex = vec3(0, -1, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(p2, 0, 1);
		EmitVertex();
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(1, 1, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(p3, 0, 1);
		EmitVertex();

		MISC_COPY_OUTPUT
		gs_tex = vec3(1, -1, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(p4, 0, 1);
		EmitVertex();
		
	}
	else { // boxes/everything else

		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(vertex[0].lt_rb.x, -vertex[0].lt_rb.y, 0, 1);
		EmitVertex();
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1.x + vertex[0].texSize1.x, vertex[0].texOffset1.y, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(vertex[0].lt_rb.z, -vertex[0].lt_rb.y, 0, 1);
		EmitVertex();
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1.x, vertex[0].texOffset1.y + vertex[0].texSize1.y, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(vertex[0].lt_rb.x, -vertex[0].lt_rb.w, 0, 1);
		EmitVertex();
		
		MISC_COPY_OUTPUT
		gs_tex = vec3(vertex[0].texOffset1 + vertex[0].texSize1, vertex[0].texIndex1);
		gs_geom = vertex[0].lt_rb_abs;
		gl_Position = vec4(vertex[0].lt_rb.z, -vertex[0].lt_rb.w, 0, 1);
		EmitVertex();
	}
	
	EndPrimitive(); 
}



#shader FRAGMENT

#version 330


layout(location = 0) out vec4 out_Color;

in vec3 gs_tex;
flat in float gs_opacity;
flat in vec4 gs_clip; 
flat in vec4 gs_fg_color; 
flat in vec4 gs_bg_color; 
flat in int gs_guiType; 
flat in vec4 gs_geom; 

in vec3 gs_bary;

uniform sampler2DArray fontTex;
uniform sampler2DArray atlasTex;


void main(void) {
	
// 	out_Color  = vec4(1,.1,.1, 1);
// 	return;
	// clipping
	if(gl_FragCoord.x < gs_clip.x || gl_FragCoord.x > gs_clip.z
		|| gl_FragCoord.y < gs_clip.y || gl_FragCoord.y > gs_clip.w) {
		
// 		out_Color = vec4(1,.1,.1,.4);
// 		return;
		
		discard;
	}
	
	
	if(gs_guiType == 0) { // just a rectangle
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
//  			out_Color = vec4(gs_tex.xy, 0, 1);
// 			return; // show the overdraw
			discard;
		};
		
		//if(dd < .35) discard;
		out_Color = vec4(gs_fg_color.rgb, a); 
// 		out_Color = vec4(.9,.9,.9, a); 
		return;
	}
	else if(gs_guiType == 2) { // simple image
		out_Color = texture(atlasTex, gs_tex);
		return;
	}
	// 3 is for render targets
	else if(gs_guiType == 4) { // square-bordered rectangle 
// 		out_Color = gs_fg_color;
		out_Color = gs_bg_color;
		
		float bwidth = gs_tex.z;
		
		if(gs_geom.x + bwidth > gl_FragCoord.x ||
			gs_geom.z < gl_FragCoord.x + bwidth ||
			gl_FragCoord.y > gs_geom.y - bwidth || 
			gl_FragCoord.y < gs_geom.w + bwidth) {
			out_Color = gs_fg_color;
		}
		
		if(out_Color.w < 0.01) discard;
		
		return;
	}
	else if(gs_guiType == 6) { // triangle
		out_Color = gs_bg_color;
		
		float bwidth = gs_tex.z;
		
		if(gs_bary.x > bwidth && gs_bary.y > bwidth && gs_bary.z > bwidth) {
			out_Color = gs_fg_color;
		}
		
		if(out_Color.w < 0.01) discard;
		
		return;
		
	}
	else if(gs_guiType == 7) { // ellipse
		
		
		out_Color = gs_bg_color;
		
		// (x2 / a2) + (y2 / b2) = 1
		
		float w = gs_geom.z - gs_geom.x;
		float h = gs_geom.w - gs_geom.y;
		
		float a = w / 2;
		float b = h / 2;
		
		float x = gl_FragCoord.x - gs_geom.x - a;
		float y = gl_FragCoord.y - gs_geom.y - b;
		
		float r = ((x*x) / (a*a)) + ((y*y) / (b*b));
		if(r > 1) {
			discard; // TODO: antialiasing
		}
		
		
		// this algorithm is not mathematically correct but it's close enough for rendering.
		
		// solve for the intersection of the line (0,0),(x,y) and the ellipse: +/-(x1,y1).
		float q = a*b / sqrt(a*a*y*y + b*b*x*x);
		
		float x1 = q * x;
		float y1 = q * y;
		
		// there are two solutions. 
		// get the distance from (x,y) to each
		float d1 = distance(vec2(x1, y1), vec2(x, y));
		float d2 = distance(vec2(-x1, -y1), vec2(x, y));
		
		// take the one closest to (x,y)
		float d = min(d1, d2);
		
		// d is approximately the distance from (x,y) to the ellipse.
		if(d < gs_tex.z) {
			out_Color = gs_fg_color;
		}
		
		if(out_Color.w < 0.01) discard;
		
		return;
	}
	else if(gs_guiType == 10) { // solid line
		
		float ty = abs(gs_tex.y); 
		float a = smoothstep(1.0/gs_tex.z, 1.0, ty);
		out_Color = vec4(gs_fg_color.xyz, a / gs_fg_color.a);
		
		return;
	}
	else if(gs_guiType == 11) { // faded line
		
		float ty = abs(gs_tex.y); 
		float a = smoothstep(1.0, 0.0, ty);
		out_Color = vec4(gs_fg_color.xyz, a / gs_fg_color.a);
		
		return;
	}
	else if(gs_guiType == 12) { // faded line, rounded ends
		/*
		need extended lines
		
		float ty = abs(gs_tex.y); 
		float a = smoothstep(1.0, 0.0, ty);
		out_Color = vec4(gs_fg_color.xyz, a / gs_fg_color.a);
		
		return;
		*/
	}
	
	// gradients
	// right triangles
	// diamonds or general rotation
	
	
	// error fallthrough debug value
	out_Color = vec4(1,.1,.1, .4);
	
	
	
	
	
}
