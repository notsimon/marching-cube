#include <global.hh>

#include <map/map.hh>

#include <string>


#define WIN_W 1024
#define WIN_H 600

#define CAMERA_T_SPEED 10.0f
#define CAMERA_R_SPEED 0.1f // angular speed (degrees)

#define PICK_RAY_LENGTH 20.0f 

// time
double delay ();

// camera

Map* map = 0;

struct {
	H3DNode node;
	Vec3f position;
	Vec3f orientation;
} camera = {0, Vec3f(0, 8, 0), Vec3f(0, 0, 0)};

// events

void keyboard_listener (int key, int state) {
	if (state == GLFW_RELEASE)
		return;
	
	switch (key) {
		case 'R':
			h3dSetOption(H3DOptions::WireframeMode, !h3dGetOption(H3DOptions::WireframeMode));
			h3dSetOption(H3DOptions::DebugViewMode, false);
			break;
		case 'F':
			h3dSetOption(H3DOptions::WireframeMode, false);
			h3dSetOption(H3DOptions::DebugViewMode, !h3dGetOption(H3DOptions::DebugViewMode));
			break;
	}
}

void mouse_position_listener (int mx, int my) {
	camera.orientation[0] = - CAMERA_R_SPEED * my; // rotation autour de l'axe x (donc verticale)
	camera.orientation[1] = - fmod(CAMERA_R_SPEED * mx, 360); // rotation autour de l'axe y (donc horizontale)
}

void mouse_button_listener (int, int) {
	
}

int edit_ray (Vec3f& p) {
	bool mouse_left = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	bool mouse_right = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

	if ((mouse_left && !mouse_right) || (!mouse_left && mouse_right)) {
		Vec3f d; // position and direction of the picking ray
		
		h3dutPickRay(camera.node, 0.5, 0.5, &p[0], &p[1], &p[2], &d[0], &d[1], &d[2]); // get the picking ray of the center of the screen
		d.normalize(); // set ray length to maximum reachable by player
		d *= PICK_RAY_LENGTH;

		if (h3dCastRay(H3DRootNode, p[0], p[1], p[2], d[0], d[1], d[2], 1) > 0) { // cast the ray, stop to the 1st collision
			H3DNode node;
			h3dGetCastRayResult(0, &node, 0, (float*)&p); // recover node and intersection point
			
			// OPTIM check if the intersection point is not available directly in model-space in H3D
			//const mat4f* m;
			//h3dGetNodeTransMats(node, 0, (const float**)&m); // both matrix representation are in column major mode (internal H3D and mat)
			//mat4f inv = mat4f::inverse(*m); // world to model matrix
			
			//cp = chunk_coord(p);
			//p = inv * p; // inverse the matrix and apply it to the position to recover the model-space position
		}
		else return 0;
	} else return 0;

	return mouse_left ? 300
		 : mouse_right ? -300
		 : 0;
}

// main

int main() {
	// init
	glfwInit();

	if (!glfwOpenWindow(WIN_W, WIN_H, 8, 8, 8, 8, 24, 8, GLFW_WINDOW)) {
		glfwTerminate();
		return 1;
	}
	
	glfwDisable(GLFW_MOUSE_CURSOR);

	glfwSetKeyCallback(keyboard_listener);
	glfwSetMousePosCallback(mouse_position_listener);
	glfwSetMouseButtonCallback(mouse_button_listener);

	// Initialize engine
	h3dInit();

	// Add pipeline resource
	H3DRes pipeRes = h3dAddResource(H3DResTypes::Pipeline, "pipelines/mine.pipeline.xml", 0);
	// Add model resource
	//H3DRes sphere_scene = h3dAddResource(H3DResTypes::SceneGraph, "models/sphere/sphere.scene.xml", 0);
	// Font texture
	//H3DRes font_tex = h3dAddResource(H3DResTypes::Material, "overlays/font.material.xml", 0);
	//H3DRes panel_material = h3dAddResource(H3DResTypes::Material, "overlays/panel.material.xml", 0);
	// Load added resources
	h3dutLoadResourcesFromDisk("."); // important!

	// Add model to scene
	H3DNode world = h3dAddGroupNode(H3DRootNode, "world");
	
	//H3DNode sphere = h3dAddNodes(terrain, sphere_scene);
	//h3dSetNodeTransform(sphere, 0, 0, 0, 0, 0, 0, 5, 5, 5);


	// Add light source
	H3DNode light = h3dAddLightNode(H3DRootNode, "Light1", 0, "LIGHTING", 0);
	h3dSetNodeTransform(light, 0, 6, 10, -10, 0, 0, 1, 1, 1);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 0, 1.0f );
	h3dSetNodeParamF(light, H3DLight::ColorF3, 1, 1.0f );
	h3dSetNodeParamF(light, H3DLight::ColorF3, 2, 1.0f );
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, 1.0f );

	// Add camera
	camera.node = h3dAddCameraNode(H3DRootNode, "Camera", pipeRes);
	// Setup viewport and render target sizes
	h3dSetNodeParamI(camera.node, H3DCamera::ViewportXI, 0);
	h3dSetNodeParamI(camera.node, H3DCamera::ViewportYI, 0);
	h3dSetNodeParamI(camera.node, H3DCamera::ViewportWidthI, WIN_W);
	h3dSetNodeParamI(camera.node, H3DCamera::ViewportHeightI, WIN_H);
	h3dSetupCameraView(camera.node, 45.0f, (float)WIN_W / WIN_H, 0.1f, 2048.0f);

	h3dResizePipelineBuffers(pipeRes, WIN_W, WIN_H);
	
	//H3DRes panel_material2 = h3dAddResource(H3DResTypes::Material, "overlays/panel.material.xml", 0);
	
	// MAP
	map = new Map(world);
	
	// MAIN LOOP
	
	while (!glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED)) {
		// Increase animation time
	    double t = delay();

		map->update(camera.position);

		// HUD
		//h3dutShowText("0.01a", 0.01, 0.01, 0.03f, 1, 1, 1, font_tex);
		//h3dutShowFrameStats(font_tex, panel_material2, H3DUTMaxStatMode);

		// inputs
		// EDIT
		Vec3f p;
		int v = edit_ray(p);
		if (v != 0) map->modify(p, v * t);

		// MOVE
		if (glfwGetKey('E')) {
			h3dSetNodeTransform(light, camera.position[0], camera.position[1], camera.position[2], camera.orientation[0], camera.orientation[1], camera.orientation[2], 1, 1, 1);
		}

		if (glfwGetKey('W')) { // forward
			camera.position[0] += -sinf(radian(camera.orientation[1])) * cosf(-radian(camera.orientation[0])) * CAMERA_T_SPEED * t;
			camera.position[1] += -sinf(-radian(camera.orientation[0])) * CAMERA_T_SPEED * t;
			camera.position[2] += -cosf(radian(camera.orientation[1])) * cosf(-radian(camera.orientation[0])) * CAMERA_T_SPEED * t;
		}

		if (glfwGetKey('S')) { // backward
			camera.position[0] += sinf(radian(camera.orientation[1])) * cosf(-radian(camera.orientation[0])) * CAMERA_T_SPEED * t;
			camera.position[1] += sinf(-radian(camera.orientation[0])) * CAMERA_T_SPEED * t;
			camera.position[2] += cosf(radian(camera.orientation[1])) * cosf(-radian(camera.orientation[0])) * CAMERA_T_SPEED * t;
		}

		if (glfwGetKey('A')) { // left
			camera.position[0] += -sinf(radian(camera.orientation[1] + 90)) * CAMERA_T_SPEED * t;
			camera.position[2] += -cosf(radian(camera.orientation[1] + 90)) * CAMERA_T_SPEED * t;
		}

		if (glfwGetKey('D')) { // right
			camera.position[0] += sinf(radian(camera.orientation[1] + 90)) * CAMERA_T_SPEED * t;
			camera.position[2] += cosf(radian(camera.orientation[1] + 90)) * CAMERA_T_SPEED * t;
		}
		
		h3dSetNodeTransform(camera.node, camera.position[0], camera.position[1], camera.position[2], camera.orientation[0], camera.orientation[1], 0, 1, 1, 1);

	    // Render scene
	    h3dRender(camera.node);

	    // Finish rendering of frame
	    h3dFinalizeFrame();
		h3dClearOverlays();
		glfwSwapBuffers();
	}
	
	h3dutDumpMessages();
	h3dRelease();
   
	glfwCloseWindow();
	glfwTerminate();
}

inline double delay () {
	static double last = 0;
	double current = glfwGetTime();
	double ellapsed = current - last;
	
	last = current;
	if (ellapsed < 0.03)
		glfwSleep(0.03 - ellapsed);
	
	return (glfwGetKey('P')) ? 0 : ellapsed;
}