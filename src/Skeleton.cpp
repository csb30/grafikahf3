//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Cseh Balint Istvan
// Neptun : WRNJPE
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include <iostream>
#include "framework.h"

float rnd() { return (float)rand() / RAND_MAX; }

//deriválás
//---------------------------
template<class T> struct Dnum { // Dual numbers for automatic derivation
//---------------------------
	float f; // function value
	T d;  // derivatives
	Dnum(float f0 = 0, T d0 = T(0)) { f = f0, d = d0; }
	Dnum operator+(Dnum r) { return Dnum(f + r.f, d + r.d); }
	Dnum operator-(Dnum r) { return Dnum(f - r.f, d - r.d); }
	Dnum operator*(Dnum r) {
		return Dnum(f * r.f, f * r.d + d * r.f);
	}
	Dnum operator/(Dnum r) {
		return Dnum(f / r.f, (r.f * d - r.d * f) / r.f / r.f);
	}
};

// Elementary functions prepared for the chain rule as well
template<class T> Dnum<T> Exp(Dnum<T> g) { return Dnum<T>(expf(g.f), expf(g.f)*g.d); }
template<class T> Dnum<T> Sin(Dnum<T> g) { return  Dnum<T>(sinf(g.f), cosf(g.f)*g.d); }
template<class T> Dnum<T> Cos(Dnum<T>  g) { return  Dnum<T>(cosf(g.f), -sinf(g.f)*g.d); }
template<class T> Dnum<T> Tan(Dnum<T>  g) { return Sin(g) / Cos(g); }
template<class T> Dnum<T> Sinh(Dnum<T> g) { return  Dnum<T>(sinh(g.f), cosh(g.f)*g.d); }
template<class T> Dnum<T> Cosh(Dnum<T> g) { return  Dnum<T>(cosh(g.f), sinh(g.f)*g.d); }
template<class T> Dnum<T> Tanh(Dnum<T> g) { return Sinh(g) / Cosh(g); }
template<class T> Dnum<T> Log(Dnum<T> g) { return  Dnum<T>(logf(g.f), g.d / g.f); }
template<class T> Dnum<T> Pow(Dnum<T> g, float n) {
	return  Dnum<T>(powf(g.f, n), n * powf(g.f, n - 1) * g.d);
}

typedef Dnum<vec2> Dnum2;

const int tessellationLevel = 20;

//---------------------------
struct Camera { // 3D camera
//---------------------------
	vec3 wEye, wLookat, wVup;   // extrinsic
	float fov, asp, fp, bp;		// intrinsic
public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = 75.0f * (float)M_PI / 180.0f;
		fp = 0.1f; bp = 100; //frontpane, backpane
	}
	mat4 V() { // view matrix: translates the center to the origin
		vec3 w = normalize(wEye - wLookat);
		vec3 u = normalize(cross(wVup, w));
		vec3 v = cross(w, u);
		return TranslateMatrix(wEye * (-1)) * mat4(u.x, v.x, w.x, 0,
			                                       u.y, v.y, w.y, 0,
			                                       u.z, v.z, w.z, 0,
			                                       0,   0,   0,   1);
	}

	mat4 P() { // projection matrix
		return mat4(1 / (tan(fov / 2)*asp), 0,                0,                      0,
			        0,                      1 / tan(fov / 2), 0,                      0,
			        0,                      0,                -(fp + bp) / (bp - fp), -1,
			        0,                      0,                -2 * fp*bp / (bp - fp),  0);
	}
};

//---------------------------
struct Material {
//---------------------------
	vec3 kd, ks, ka;
	float shininess;
};

//---------------------------
struct Light {
//---------------------------
	vec3 La, Le;
	vec4 wLightPos; // homogeneous coordinates, can be at ideal point
};


//---------------------------
struct RenderState {
//---------------------------
	mat4	           MVP, M, Minv, V, P;
	Material *         material;
	std::vector<Light> lights;
	vec3	           wEye;
};

//---------------------------
class Shader : public GPUProgram {
//---------------------------
public:
	virtual void Bind(RenderState state) = 0;

	void setUniformMaterial(const Material& material, const std::string& name) {
		setUniform(material.kd, name + ".kd");
		setUniform(material.ks, name + ".ks");
		setUniform(material.ka, name + ".ka");
		setUniform(material.shininess, name + ".shininess");
	}

	void setUniformLight(const Light& light, const std::string& name) {
		setUniform(light.La, name + ".La");
		setUniform(light.Le, name + ".Le");
		setUniform(light.wLightPos, name + ".wLightPos");
	}
};


//---------------------------
class PhongShader : public Shader {
//---------------------------
	const char * vertexSource = R"(
		#version 330
		precision highp float;

		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
		};

		uniform mat4  MVP, M, Minv; // MVP, Model, Model-inverse
		uniform Light[8] lights;    // light sources
		uniform int   nLights;
		uniform vec3  wEye;         // pos of eye

		layout(location = 0) in vec3  vtxPos;            // pos in modeling space
		layout(location = 1) in vec3  vtxNorm;      	 // normal in modeling space
		layout(location = 2) in vec2  vtxUV;

		out vec3 wNormal;		    // normal in world space
		out vec3 wView;             // view in world space
		out vec3 wLight[8];		    // light dir in world space
		out vec2 texcoord;

		void main() {
			gl_Position = vec4(vtxPos, 1) * MVP; // to NDC
			// vectors for radiance computation
			vec4 wPos = vec4(vtxPos, 1) * M;
			for(int i = 0; i < nLights; i++) {
				wLight[i] = lights[i].wLightPos.xyz * wPos.w - wPos.xyz * lights[i].wLightPos.w;
			}
		    wView  = wEye * wPos.w - wPos.xyz;
		    wNormal = (Minv * vec4(vtxNorm, 0)).xyz;
		    texcoord = vtxUV;
		}
	)";

	// fragment shader in GLSL
	const char * fragmentSource = R"(
		#version 330
		precision highp float;

		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
		};

		struct Material {
			vec3 kd, ks, ka;
			float shininess;
		};

		uniform Material material;
		uniform Light[8] lights;    // light sources
		uniform int   nLights;
		uniform sampler2D diffuseTexture;

		in  vec3 wNormal;       // interpolated world sp normal
		in  vec3 wView;         // interpolated world sp view
		in  vec3 wLight[8];     // interpolated world sp illum dir
		in  vec2 texcoord;

        out vec4 fragmentColor; // output goes to frame buffer

		void main() {
			vec3 N = normalize(wNormal);
			vec3 V = normalize(wView);
			if (dot(N, V) < 0) N = -N;	// prepare for one-sided surfaces like Mobius or Klein
			vec3 texColor = vec3(1,1,1);
			vec3 ka = material.ka * texColor;
			vec3 kd = material.kd * texColor;

			vec3 radiance = vec3(0, 0, 0);
			for(int i = 0; i < nLights; i++) {
				vec3 L = normalize(wLight[i]);
				vec3 H = normalize(L + V);
				float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);
				// kd and ka are modulated by the texture
				radiance += ka * lights[i].La +
                           (kd * texColor * cost + material.ks * pow(cosd, material.shininess)) * lights[i].Le;
			}
			fragmentColor = vec4(radiance, 1);
		}
	)";
public:
	PhongShader() { create(vertexSource, fragmentSource, "fragmentColor"); }

	void Bind(RenderState state) {
		Use(); 		// make this program run
		setUniform(state.MVP, "MVP");
		setUniform(state.M, "M");
		setUniform(state.Minv, "Minv");
		setUniform(state.wEye, "wEye");
		setUniformMaterial(*state.material, "material");

		setUniform((int)state.lights.size(), "nLights");
		for (unsigned int i = 0; i < state.lights.size(); i++) {
			setUniformLight(state.lights[i], std::string("lights[") + std::to_string(i) + std::string("]"));
		}
	}
};

//---------------------------
class Geometry {
//---------------------------
protected:
	unsigned int vao, vbo;        // vertex array object
public:
	Geometry() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo); // Generate 1 vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
	virtual void Draw() = 0;
	~Geometry() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

class FlatSurface : public Geometry {
    struct VertexData {
        vec3 position, normal;
        vec2 texcoord;
    };

    unsigned int vtxnum;
public:
    FlatSurface() { vtxnum = 0; }


    VertexData GenVertexData(float x, float y, float z, vec3 normal) {
        VertexData vtxData;
        vtxData.texcoord = vec2(0,0);
        vtxData.position = vec3(x, y, z);
        vtxData.normal = normal;
        return vtxData;
    }

    vec3 side(vec3 p1, vec3 p2){
        return p1-p2;
    }

    void create(std::vector<vec3> points) {
        vtxnum = points.size();
        std::vector<VertexData> vtxData;	// vertices on the CPU
        vec3 normal = vec3(0,0,1);
        for (int i = 0; i < vtxnum-2; i++) {
            normal = cross(side(points[i], points[i+1]), side(points[i+1], points[i+2]));
            float x = (float)points[i].x;
            float y = (float)points[i].y;
            float z = (float)points[i].z;
            vtxData.push_back(GenVertexData(x,y,z,normal));
        }
        normal = cross(side(points[vtxnum-3], points[vtxnum-2]), side(points[vtxnum-2], points[vtxnum-1]));
        vtxData.push_back(GenVertexData((float)points[vtxnum-2].x, (float)points[vtxnum-2].y, (float)points[vtxnum-2].z,normal));
        vtxData.push_back(GenVertexData((float)points[vtxnum-1].x, (float)points[vtxnum-1].y, (float)points[vtxnum-1].z,normal));

        glBufferData(GL_ARRAY_BUFFER, vtxnum * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);
        // Enable the vertex attribute arrays
        glEnableVertexAttribArray(0);  // attribute array 0 = POSITION
        glEnableVertexAttribArray(1);  // attribute array 1 = NORMAL
        glEnableVertexAttribArray(2);  // attribute array 2 = TEXCOORD0
        // attribute array, components/attribute, component type, normalize?, stride, offset
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texcoord));
    }

    void Draw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vtxnum+1);
    }
};

//---------------------------
class ParamSurface : public Geometry {
//---------------------------
	struct VertexData {
		vec3 position, normal;
		vec2 texcoord;
	};

	unsigned int nVtxPerStrip, nStrips;
public:
	ParamSurface() { nVtxPerStrip = nStrips = 0; }

	virtual void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) = 0;

	VertexData GenVertexData(float u, float v) {
		VertexData vtxData;
		vtxData.texcoord = vec2(u, v);
		Dnum2 X, Y, Z;
		Dnum2 U(u, vec2(1, 0)), V(v, vec2(0, 1));
		eval(U, V, X, Y, Z);
		vtxData.position = vec3(X.f, Y.f, Z.f);
		vec3 drdU(X.d.x, Y.d.x, Z.d.x), drdV(X.d.y, Y.d.y, Z.d.y);
		vtxData.normal = cross(drdU, drdV);
		return vtxData;
	}

	void create(int N = tessellationLevel, int M = tessellationLevel) {
		nVtxPerStrip = (M + 1) * 2;
		nStrips = N;
		std::vector<VertexData> vtxData;	// vertices on the CPU
		for (int i = 0; i < N; i++) {
			for (int j = 0; j <= M; j++) {
				vtxData.push_back(GenVertexData((float)j / M, (float)i / N));
				vtxData.push_back(GenVertexData((float)j / M, (float)(i + 1) / N));
			}
		}
		glBufferData(GL_ARRAY_BUFFER, nVtxPerStrip * nStrips * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);
		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(0);  // attribute array 0 = POSITION
		glEnableVertexAttribArray(1);  // attribute array 1 = NORMAL
		glEnableVertexAttribArray(2);  // attribute array 2 = TEXCOORD0
		// attribute array, components/attribute, component type, normalize?, stride, offset
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texcoord));
	}

	void Draw() {
		glBindVertexArray(vao);
		for (unsigned int i = 0; i < nStrips; i++) glDrawArrays(GL_TRIANGLE_STRIP, i *  nVtxPerStrip, nVtxPerStrip);
	}
};

//---------------------------
class Sphere : public ParamSurface {
//---------------------------
public:
	Sphere() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * (float)M_PI, V = V * (float)M_PI;
		X = Cos(U) * Sin(V); Y = Sin(U) * Sin(V); Z = Cos(V);
	}
};

//---------------------------
class Tractricoid : public ParamSurface {
//---------------------------
public:
	Tractricoid() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		const float height = 3.0f;
		U = U * height, V = V * 2 * M_PI;
		X = Cos(V) / Cosh(U); Y = Sin(V) / Cosh(U); Z = U - Tanh(U);
	}
};

//---------------------------
class Cylinder : public ParamSurface {
//---------------------------
public:
	Cylinder() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * M_PI, V = V * 2 - 1.0f;
		X = Cos(U); Y = Sin(U); Z = V;
	}
};

//---------------------------
class Torus : public ParamSurface {
//---------------------------
public:
	Torus() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		const float R = 1, r = 0.5f;
		U = U * 2.0f * M_PI, V = V * 2.0f * M_PI;
		Dnum2 D = Cos(U) * r + R;
		X = D * Cos(V); Y = D * Sin(V); Z = Sin(U) * r;
	}
};

//---------------------------
class Mobius : public ParamSurface {
//---------------------------
public:
	Mobius() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		const float R = 1, width = 0.5f;
		U = U * M_PI, V = (V - 0.5f) * width;
		X = (Cos(U) * V + R) * Cos(U * 2);
		Y = (Cos(U) * V + R) * Sin(U * 2);
		Z = Sin(U) * V;
	}
};

//---------------------------
class Klein : public ParamSurface {
//---------------------------
	const float size = 1.5f;
public:
	Klein() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * M_PI * 2, V = V * M_PI * 2;
		Dnum2 a = Cos(U) * (Sin(U) + 1) * 0.3f;
		Dnum2 b = Sin(U) * 0.8f;
		Dnum2 c = (Cos(U) * (-0.1f) + 0.2f);
		X = a + c * ((U.f > M_PI) ? Cos(V + M_PI) : Cos(U) * Cos(V));
		Y = b + ((U.f > M_PI) ? 0 : c * Sin(U) * Cos(V));
		Z = c * Sin(V);
	}
};

//---------------------------
class Boy : public ParamSurface {
//---------------------------
public:
	Boy() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = (U - 0.5f) * M_PI, V = V * M_PI;
		float r2 = sqrt(2.0f);
		Dnum2 denom = (Sin(U * 3)*Sin(V * 2)*(-3 / r2) + 3) * 1.2f;
		Dnum2 CosV2 = Cos(V) * Cos(V);
		X = (Cos(U * 2) * CosV2 * r2 + Cos(U) * Sin(V * 2)) / denom;
		Y = (Sin(U * 2) * CosV2 * r2 - Sin(U) * Sin(V * 2)) / denom;
		Z = (CosV2 * 3) / denom;
	}
};

//---------------------------
class Dini : public ParamSurface {
//---------------------------
	Dnum2 a = 1.0f, b = 0.15f;
public:
	Dini() { create(); }

	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 4 * M_PI, V = V * (1 - 0.1f) + 0.1f;
		X = a * Cos(U) * Sin(V);
		Y = a * Sin(U) * Sin(V);
		Z = a * (Cos(V) + Log(Tan(V / 2))) + b * U + 3;
	}
};

class Square : public FlatSurface {
public:
    Square() {
        std::vector<vec3> points;
        points.push_back(vec3(-0.5f,-0.5f,0));
        points.push_back(vec3(0.5f,-0.5f,0));
        points.push_back(vec3(0.5f,0.5f,0));
        points.push_back(vec3(-0.5f,0.5f,0));
        points.push_back(vec3(-0.5f,-0.5f,0));
        create(points);
    }
};

class Pyramid : public FlatSurface {
public:
    Pyramid() {
        std::vector<vec3> points;
        //lap 1
        points.push_back(vec3(0,1,0));
        points.push_back(vec3(-1,0,1));
        points.push_back(vec3(-1,0,-1));

        //lap2
        points.push_back(vec3(0,1,0));
        points.push_back(vec3(-1,0,-1));
        points.push_back(vec3(1,0,-1));

        //lap3
        points.push_back(vec3(0,1,0));
        points.push_back(vec3(1,0,-1));
        points.push_back(vec3(1,0,1));

        //lap4
        points.push_back(vec3(0,1,0));
        points.push_back(vec3(1,0,1));
        points.push_back(vec3(-1,0,1));
        create(points);
    }
};

//---------------------------
struct Object {
//---------------------------
	Shader *   shader;
	Material * material;
	Geometry * geometry;
	vec3 scale, translation, rotationAxis;
	float rotationAngle;
public:
	Object(Shader * _shader, Material * _material, Geometry * _geometry) :
		scale(vec3(1, 1, 1)), translation(vec3(0, 0, 0)), rotationAxis(0, 0, 1), rotationAngle(0) {
		shader = _shader;
		material = _material;
		geometry = _geometry;
	}


	virtual void SetModelingTransform(mat4& M, mat4& Minv) {
		M = ScaleMatrix(scale) * RotationMatrix(rotationAngle, rotationAxis) * TranslateMatrix(translation);
		Minv = TranslateMatrix(-translation) * RotationMatrix(-rotationAngle, rotationAxis) * ScaleMatrix(vec3(1 / scale.x, 1 / scale.y, 1 / scale.z));
	}

	void Draw(RenderState state) {
		mat4 M, Minv;
		SetModelingTransform(M, Minv);
		state.M = M;
		state.Minv = Minv;
		state.MVP = state.M * state.V * state.P;
		state.material = material;
		shader->Bind(state);
		geometry->Draw();
	}

	virtual void Animate(float tstart, float tend) { rotationAngle = 0.8f * tend; }

    void move(float tstart, float tend, vec3 dir){
        translation = translation - dir * (tstart-tend);
    }
};

class Track {
    std::vector<Object*> tracks;
    float r;
    float w;
    float k;
    float loffset;
    Material * material1;
    Shader * phongShader;
    Geometry * square;
public:
    Track(float width, float height, float xoffset){
        phongShader = new PhongShader();
        square = new Square();
        r=height/2;
        w=xoffset;
        k = r * 2 * M_PI + 2 * 3 * r;
        loffset = 0;

        material1 = new Material;
        material1->kd = vec3(198.0f/255, 198.0f/255, 198.0f/255);
        material1->ks = vec3(0.0f, 0.0f, 0.0f);
        material1->ka = vec3(35.0f/255,35.0f/255,35.0f/255);
        material1->shininess = 100;


        for (float l=0; l<k; l+=(k/30)){
            Object * track = new Object(phongShader, material1, square);
            track->translation = genpos(l);
            track->scale = vec3(width, 0.05f, 1);
            track->rotationAxis = vec3(1, 0, 0);
            track->rotationAngle = 90.0f * M_PI / 180 + genangle(l);
            tracks.push_back(track);
        }
    }

    void moveTrack(float amount){
        int i = 0;
        loffset += amount;
        if(loffset>=k) loffset = 0;
        if(loffset<0) loffset = k;
        for (float l=0; l<k; l+=(k/30)){
            if(l+loffset<k){
                tracks[i]->translation = genpos(l+loffset);
                tracks[i]->rotationAngle = 90.0f * M_PI / 180 + genangle(l+loffset);
            } else {
                tracks[i]->translation = genpos(l+loffset-k);
                tracks[i]->rotationAngle = 90.0f * M_PI / 180 + genangle(l+loffset-k);
            }
            i++;
        }
    }

    vec3 genpos(float l){
        if(0<l and l<3*r){
            float z = l;
            float x = w/2;
            float y = 0;
            return vec3(x, y+0.1f, z);
        }
        else if(l<(3*r + M_PI*r)){
            float dl = l-3*r;
            float z = -r*sin(dl/r);
            float x = w/2;
            float y = 2*r-r*(1-cos(dl/r));
            return vec3(x,y+0.1f,z);
        }
        else if(l<(3*r + M_PI*r + 3*r)){
            l-=(3*r + M_PI*r);
            float z = (3*r)-l;
            float x = w/2;
            float y = 2*r;
            return vec3(x, y+0.1f, z);
        } else {
            float dl = l-3*r;
            float z = r*sin(dl/r) + 3*r;
            float x = w/2;
            float y = r*(1-cos(dl/r));
            return vec3(x,y+0.1f,z);
        }
    }

    float genangle(float l){
        if(0<l and l<3*r){
            return 0;
        }
        else if(l<(3*r + M_PI*r)){
            float dl = l-3*r;
            return -dl/r;
        }
        else if(l<(3*r + M_PI*r + 3*r)){
            return 0;
        }
        else {
            float dl = l-3*r;
            return -dl/r;
        }
    }

    void Draw(RenderState state){
        for (Object * track : tracks) track->Draw(state);
    }
};

class Tank {
    Track* tracks[2];
    Tank(float width, float height){
        tracks[0] = new Track(0.3f, height,width);
        tracks[1] = new Track(0.3f, height,-width);
    }

    void Draw(RenderState state){
        for (Track * track : tracks) track->Draw(state);
    }
};

//---------------------------
class Scene {
//---------------------------
	std::vector<Object *> objects;
	Camera camera; // 3D camera
	std::vector<Light> lights;
    Object * floorobj;
    Track * track;
public:
	void Build() {
		// Shaders
		Shader * phongShader = new PhongShader();

		// Materials
		Material * material0 = new Material;
		material0->kd = vec3(0, 0, 0);
		material0->ks = vec3(0, 0, 0);
		material0->ka = vec3(196.0f/255, 176.0f/255, 33.0f/255);
		material0->shininess = 100;

		Material * material1 = new Material;
		material1->kd = vec3(198.0f/255, 198.0f/255, 198.0f/255);
		material1->ks = vec3(0.0f, 0.0f, 0.0f);
		material1->ka = vec3(35.0f/255,35.0f/255,35.0f/255);
		material1->shininess = 100;

		// Geometries
        Geometry * square = new Square();
        //Geometry * triangle = new Triangle();
        Geometry * pyramid = new Pyramid();


		floorobj = new Object(phongShader, material0, square);
		floorobj->translation = vec3(0, 0, 0);
		floorobj->scale = vec3(200.0f, 200.0f, 1.0f);
        floorobj->rotationAxis = vec3(1, 0, 0);
        floorobj->rotationAngle = 90.0f * M_PI / 180;

        track = new Track(0.3f, 0.4f,0.5f);

        for(int i=0; i<100; i++){
            float x = 200*rnd()-100;
            float z = 200*rnd()-100;

            Object * pyramidobj = new Object(phongShader, material1, pyramid);
            pyramidobj->translation = vec3(x, 0, z);
            pyramidobj->scale = vec3(3.0f, 5.0f, 3.0f);
            pyramidobj->rotationAxis = vec3(0, 1, 0);
            objects.push_back(pyramidobj);
        }

/*		Object * triangleobj = new Object(phongShader, material0, texture4x8, triangle);
		triangleobj->translation = vec3(0, 3, 0);
		triangleobj->scale = vec3(0.7f, 0.7f, 0.7f);
		triangleobj->rotationAxis = vec3(1, 0, 0);
		objects.push_back(triangleobj);*/

		// Camera
		camera.wEye = vec3(0, 1, 2);
		camera.wLookat = vec3(0, 0, 0);
		camera.wVup = vec3(0, 1, 0);

        lights.resize(1);
        lights[0].wLightPos = vec4(5, 5, 0, 0);	// ideal point -> directional light source
        lights[0].La = vec3(1, 1, 1);
        lights[0].Le = vec3(1, 1, 1);

		/*// Lights
		lights.resize(3);
		lights[0].wLightPos = vec4(5, 5, 4, 0);	// ideal point -> directional light source
		lights[0].La = vec3(0.1f, 0.1f, 1);
		lights[0].Le = vec3(3, 0, 0);

		lights[1].wLightPos = vec4(5, 10, 20, 0);	// ideal point -> directional light source
		lights[1].La = vec3(0.2f, 0.2f, 0.2f);
		lights[1].Le = vec3(0, 3, 0);

		lights[2].wLightPos = vec4(-5, 5, 5, 0);	// ideal point -> directional light source
		lights[2].La = vec3(0.1f, 0.1f, 0.1f);
		lights[2].Le = vec3(0, 0, 3);*/
	}

	void Render() {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		state.lights = lights;
        floorobj->Draw(state);
        track->Draw(state);
		for (Object * obj : objects) obj->Draw(state);
	}

	void Animate(float tstart, float tend) {
		//for (Object * obj : objects) obj->Animate(tstart, tend);
        objects[1]->Animate(tstart,tend);
	}

    void move(float tstart, float tend, vec3 dir){
        //for (Object * obj : objects) obj->move(tstart, tend, dir);
    }

    void movecam(vec3 dir){
        camera.wEye = camera.wEye + dir;
    }

    void moveTrack(float tstart, float tend, float v){
        track->moveTrack((tend-tstart)*v);
    }
};

Scene scene;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	scene.Build();
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0.5f, 0.5f, 0.8f, 1.0f);							// background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
	scene.Render();
	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {

}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) { }

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { }

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	static float tend = 0;
	const float dt = 0.1f; // dt is ”infinitesimal”
	float tstart = tend;
	tend = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

	for (float t = tstart; t < tend; t += dt) {
		float Dt = fmin(dt, tend - t);
		//scene.Animate(t, t + Dt);
        scene.move(t, t+Dt, vec3(0,0,1));
        scene.moveTrack(tstart, t+Dt, 0.1f);
	}
	glutPostRedisplay();
}